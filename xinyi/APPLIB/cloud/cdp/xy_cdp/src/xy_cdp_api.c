#include "softap_macro.h"
#if TELECOM_VER
#include "xy_utils.h"
#include "xy_at_api.h"
#include "cdp_backup.h"
#include "cc.h"
#include "lwip/inet.h"
#include "xy_cdp_api.h"
#include "agenttiny.h"
#include "atiny_context.h"
#include "agent_tiny_demo.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "net_app_resume.h"

extern int cdp_lifetime;
extern int cdp_register_fail_flag;

volatile int cdp_send_mid = 0;
cdp_send_asyn_ack g_cdp_send_asyn_ack = NULL;
cdp_downstream_callback g_cdp_downstream_callback = NULL;
uint8_t *g_endpointName = NULL;

int cdp_cloud_setting(char *ip_addr_str, int port)
{
	if((strlen(ip_addr_str) <= 0)||(strlen(ip_addr_str) >= 40) || (port < 0) || (port > 65535))
	{
		xy_printf("[cdp_cloud_setting]:ip or port error!");
		return XY_ERR;
	}

    if(set_cdp_server_settings(ip_addr_str, (u16_t)port))
    {
        xy_printf("[cdp_cloud_setting]:cdp server setting error!");
        return XY_ERR;
    }

    return XY_OK;
}


int cdp_register(int lifetime, int timeout)
{
    //wait PDP active
	if(xy_wait_tcpip_ok(timeout) == XY_ERR)
    {
        xy_printf("[cdp_register]:Network disconnection!");
        return XY_ERR;
    }

    //if cdp is registered after deepsleep to resume cdp task
    resume_net_app(CDP_TASK);

    if(!is_cdp_running())
    {
      //  if(lifetime < 120 || lifetime > LWM2M_DEFAULT_LIFETIME * 30)
        if(lifetime < 15 || lifetime > LWM2M_DEFAULT_LIFETIME * 30)
        {
            xy_printf("[cdp_register]:cdp server lifetime is error!");
			cdp_lifetime = 0;
            return XY_ERR;
        }
        else
        {
            cdp_lifetime = lifetime;    
        }
        
        if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
        {
            xy_printf("[cdp_register]:cdp server addr is empty!");
            return XY_ERR;
        }
        else if(start_cdp_lwm2m() == 0)
        {
            xy_printf("[cdp_register]:registed failed!");
            return XY_ERR;
        }
    }
    else
    {
        xy_printf("[cdp_register]:cdp have registed!");
        return XY_OK;
    }    
    
    if(cdp_api_register_sem == NULL)
        cdp_api_register_sem = osSemaphoreNew(0xFFFF, 0, NULL);

    while (osSemaphoreAcquire(cdp_api_register_sem, 0) == osOK) {};
    if (osSemaphoreAcquire(cdp_api_register_sem, timeout * 1000) != osOK)
    {
        cdp_register_fail_flag = 1;
        while(is_cdp_running())
        {
            osDelay(100);
        }

        xy_printf("[cdp_register]:cdp register fail, timeout!");
        return XY_ERR;
    }

    handle_data_t *handle = (handle_data_t *)g_phandle;
    if(handle == NULL)
    {
        xy_printf("[cdp_register]:cdp register fail!");
        return XY_ERR;
    }
    
    xy_printf("[cdp_register]:cdp register success!");
    return XY_OK;
}


int cdp_deregister(int timeout)
{
    //wait PDP active
	if(xy_wait_tcpip_ok(timeout) == XY_ERR)
    {
        xy_printf("[cdp_deregister]:Network disconnection!");
        return XY_ERR;
    }

    //if cdp is registered after deepsleep to resume cdp task
    resume_net_app(CDP_TASK);

    if(deregister_lwm2m_task() == 0)
    {
        xy_printf("[cdp_deregister]:cdp deregister failed!");
        return XY_ERR;
    }

    if(cdp_api_deregister_sem == NULL)
        cdp_api_deregister_sem = osSemaphoreNew(0xFFFF, 0, NULL);
    
    while (osSemaphoreAcquire(cdp_api_deregister_sem, 0) == osOK) {};
    osSemaphoreAcquire(cdp_api_deregister_sem, timeout*1000);
  
    xy_printf("[cdp_deregister]:cdp deregister success!");
    return XY_OK;
}


int cdp_send_syn(char *data, int len, int msg_type)
{
    int errr_num = -1;
    
    if(data == NULL)
    {
        xy_printf("[cdp_send_syn]:data is NULL!");
        return XY_ERR;
    }
    
    if(len>MAX_REPORT_DATA_LEN || len<=0)
    {
        xy_printf("[cdp_send_syn]:data len error!");
        return XY_ERR;
    }
        
    if (!ps_netif_is_ok()) 
    {
        xy_printf("[cdp_send_syn]:Network disconnection!");
        return XY_ERR;
    }

    resume_net_app(CDP_TASK);
    
    handle_data_t *handle = (handle_data_t *)g_phandle;
    lwm2m_context_t *context = handle->lwm2m_context;
    if ((is_cdp_running() == 0) 
            || (context->state != STATE_READY))
    {
        xy_printf("[cdp_send_syn]:cdp not exist!");
        return XY_ERR;
    }

	if(app_data_report_check() == XY_ERR)
		return XY_ERR;

    if(get_upstream_message_pending_num())
    {
        xy_printf("[cdp_send_syn]:Data is being sent!");
        return XY_ERR;
    }
        
    errr_num = get_upstream_message_error_num();
    if (send_message_via_lwm2m(data, len, msg_type,0))
        return XY_ERR;

   	while(get_upstream_message_pending_num())
	{
		osDelay(100);
	}

	if(get_upstream_message_error_num() - errr_num != 0)
	{
		return XY_ERR;
	}

    xy_printf("[cdp_send_syn]:Data is send success!"); 
    return XY_OK;
}

int cdp_send_syn_with_rai(char *data, int len, int msg_type)
{
    if(msg_type == cdp_CON)
        return cdp_send_syn(data, len, cdp_CON_WAIT_REPLY_RAI);
    else
        return cdp_send_syn(data, len, cdp_NON_RAI);
}

int cdp_send_asyn(char *data, int len, int msg_type)
{
    int pending_num = -1;
    int mid = -1;
    
    if(data == NULL)
    {
        xy_printf("[cdp_send_asyn]:data is NULL!");
        return XY_ERR;
    }
    
    if(len>MAX_REPORT_DATA_LEN || len<=0)
    {
        xy_printf("[cdp_send_asyn]:data len error!");
        return XY_ERR;
    }
        
    if (!ps_netif_is_ok()) 
    {
        xy_printf("[cdp_send_asyn]:Network disconnection!");
        return XY_ERR;
    }

    resume_net_app(CDP_TASK);
    
    handle_data_t *handle = (handle_data_t *)g_phandle;
    lwm2m_context_t *context = handle->lwm2m_context;
    if ((is_cdp_running() == 0) 
            || (context->state != STATE_READY))
    {
        xy_printf("[cdp_send_syn]:cdp not exist!");
        return XY_ERR;
    }

	if(app_data_report_check() == XY_ERR)
		return XY_ERR;

    pending_num = get_upstream_message_pending_num();
    if(pending_num >= 8)
    {
        xy_printf("[cdp_send_asyn]:The data link list is full!");
        return XY_ERR;
    }

    cdp_send_mid = -1;
    if (send_message_via_lwm2m(data, len, msg_type,0))
        return XY_ERR;

    if(NULL != cdp_api_sendasyn_sem)
    {
        osSemaphoreAcquire(cdp_api_sendasyn_sem, 10*1000);
        mid = cdp_send_mid;
    }
    
    return mid;
}

int cdp_send_asyn_with_rai(char *data, int len, int msg_type)
{
    if(msg_type == cdp_CON)
        return cdp_send_asyn(data, len, cdp_CON_WAIT_REPLY_RAI);
    else
        return cdp_send_asyn(data, len, cdp_NON_RAI);
}

int cdp_send_status_check()
{
    handle_data_t *handle = (handle_data_t *)g_phandle;
    if(handle == NULL)
        return XY_ERR;
    
    if(!is_cdp_running())
        return XY_ERR;

    if((handle->lwm2m_context->observedList == NULL )
        ||(handle->lwm2m_context->state != STATE_READY))
        return XY_ERR;
        
    return XY_OK;
}


int cdp_lifetime_update(int timeout)
{
    int ret = -1;
        
    if (!ps_netif_is_ok()) 
    {
        xy_printf("[cdp_update]:Network disconnection!");
        return XY_ERR;
    }

    resume_net_app(CDP_TASK);
    
    handle_data_t *handle = (handle_data_t *)g_phandle;
    lwm2m_context_t *context = handle->lwm2m_context;
    if ((is_cdp_running() == 0) 
            || (context->state != STATE_READY))
    {
        xy_printf("[cdp_send_syn]:cdp not exist!");
        return XY_ERR;
    }

    xy_lwm2m_server_t *targetP = context->serverList;
    if(targetP == NULL)
    {
        xy_printf("[cdp_send_syn]:cdp not ready!");
        return XY_ERR;
    }

    ret = cdp_update_proc(targetP);
    if (ret == -1)
    {
        return XY_ERR;
    }

    if(cdp_api_update_sem == NULL)
        cdp_api_update_sem = osSemaphoreNew(0xFFFF, 0, NULL);

    while (osSemaphoreAcquire(cdp_api_update_sem, 0) == osOK) {};
    if (osSemaphoreAcquire(cdp_api_update_sem, timeout * 1000) != osOK)
    {
        cdp_register_fail_flag = 1;
        while(is_cdp_running())
        {
            osDelay(100);
        }
        
        xy_printf("[cdp_update] cdp update fail!");
        return XY_ERR;
    }    
    
    return XY_OK;
}

int cdp_rmlft_get(void)
{
    int app_type = -1;
    int remain_lifetime = -1;

    if (g_phandle != NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->state != STATE_READY)
        return XY_ERR;

    if (!cdp_handle_exist())
    {
        if(!NET_NEED_RECOVERY(CDP_TASK))
            return XY_ERR;

        cdp_read_regInfo(OFFSET_NETINFO_PARAM(cdp_regInfo));

        remain_lifetime = g_cdp_regInfo->regtime + g_cdp_regInfo->lifetime - cloud_gettime_s();
    }
    else if ( g_phandle !=NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->serverList != NULL)
    {
        remain_lifetime = g_phandle->lwm2m_context->serverList->registration +
            g_phandle->lwm2m_context->serverList->lifetime - cloud_gettime_s();
    }
    else
        return XY_ERR;
    
    if (remain_lifetime >= 0)
    {
        return remain_lifetime;      
    }
    else
    {
        return XY_ERR;
    }
}


int cdp_callbak_set(cdp_downstream_callback downstream_callback, cdp_send_asyn_ack send_asyn_ack)
{
    g_cdp_send_asyn_ack = send_asyn_ack;
    g_cdp_downstream_callback = downstream_callback;
    return XY_OK;
}


uint8_t * cdp_set_endpoint_name(char * endpointname)
{
	if(endpointname == NULL || (endpointname != NULL && strlen(endpointname) > 255))
    {
        xy_printf("[cdp_set_endpoint_name]:Parameter error!");
        return NULL;
    }
	
	if(g_endpointName != NULL) 
		xy_free(g_endpointName);

	g_endpointName = xy_malloc(strlen(endpointname) + 1);
	memset(g_endpointName, 0x00, strlen(endpointname) + 1);
	memcpy(g_endpointName, endpointname, strlen(endpointname));

	return g_endpointName;
}


#endif

