
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "softap_macro.h"
#if TELECOM_VER
#include "xy_utils.h"
#include "softap_macro.h"
#include "cdp_backup.h"
#include "flash_adapt.h"
#include "xy_flash.h"
#include "atiny_context.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "xy_system.h"
#include "low_power.h"
#include "net_app_resume.h"
#include "cloud_utils.h"
#include "atiny_context.h"
#include "net_app_resume.h"
#include "xy_proxy.h"
#include "xy_net_api.h"

/*******************************************************************************
 *						  Local function declarations						   *
 ******************************************************************************/

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						Inline function implementations 					   *
 ******************************************************************************/

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
extern unsigned short g_cdp_localPort;
extern int g_FOTAing_flag;
extern osSemaphoreId_t cdp_recovery_sem;
extern osMutexId_t g_upstream_mutex;
unsigned int  g_cdp_keepalive_update = 0;
osThreadId_t g_cdp_timeout_restart_TskHandle = NULL;
osThreadId_t g_cdp_resume_TskHandle = NULL;
osThreadId_t g_cdp_rtc_resume_TskHandle = NULL;
extern osMutexId_t g_cdp_keepalive_update_mutex;
extern osThreadId_t g_lwm2m_TskHandle;

cdp_regInfo_t *g_cdp_regInfo = NULL;

typedef enum {
    CDP_CHECKSUM = 0,
    CDP_VERIFY,
}cdp_verify_type_t;

/*****************************************************************************
 Function    : verify_net_regInfo
 Description : Calculate and check checksum
 Input       : mode   ---Calculate or check
               data   ---databuf
               size   ---datalen
 Output      : NULL
 Return      : 0,success;
 *****************************************************************************/


bool cdp_handle_exist()
{
    if(g_lwm2m_TskHandle !=  NULL)
        return true;
    else
        return false;
}





//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
void cdp_bak_svrInfo(xy_lwm2m_server_t * server)
{
	unsigned int num = strlen(server->location) + 1;
	
    g_cdp_regInfo->net_info.remote_port = g_softap_fac_nv->cloud_server_port;
	memset(g_cdp_regInfo->net_info.remote_ip,0x0,40);
    memcpy(g_cdp_regInfo->net_info.remote_ip,g_phandle->serverip,40);
    g_cdp_regInfo->net_info.local_port = g_cdp_localPort;
    //目前支持V4/V6
    if(xy_get_netifIpType() == IPV4_TYPE)
        xy_get_ipaddr(IPV4_TYPE,&g_cdp_regInfo->net_info.local_ip);
    else
        xy_get_ipaddr(IPV6_TYPE,&g_cdp_regInfo->net_info.local_ip);
	g_cdp_regInfo->regtime = server->registration;
	g_cdp_regInfo->lifetime = server->lifetime;

	num = num < (64-1) ? num : (64-1);
	g_cdp_regInfo->location_len = num;
	memset(g_cdp_regInfo->server_location, 0, num);
	memcpy(g_cdp_regInfo->server_location, server->location, num);
}

//状态机恢复至READY同时将NV注册信息跟新至链表
int cdp_resume_regInfo(lwm2m_context_t  * contextP)
{
	//xy_printf("[CDP]resume cdp reginfo~");

	lwm2m_observed_t *observedP;
	cdp_lwm2m_observed_t *backup_observed;
	bool allocatedObserver = false;
	lwm2m_watcher_t *watcherP;
	cdp_lwm2m_watcher_t *backup_watcherP;
	xy_lwm2m_server_t *targetP, *tmptargetP;

	int i, j;

	contextP->state = STATE_READY;
	contextP->bsCtrl.state = STATE_INITIAL;
	contextP->nextMID = g_softap_var_nv->cdpNextMid;

	tmptargetP = targetP = contextP->serverList;
	while (targetP != NULL)
	{
		targetP->sessionH = lwm2m_connect_server(targetP->secObjInstID, contextP->userData, false);	
        if(NULL == targetP->sessionH ){
            xy_printf("[CDP]resume cdp reginfo: sessionH error");
            return -1;
        }
		targetP->status = XY_STATE_REGISTERED;
		targetP->registration = g_cdp_regInfo->regtime;
		targetP->lifetime = g_cdp_regInfo->lifetime;
		targetP->location = lwm2m_malloc(g_cdp_regInfo->location_len+1);
		memset(targetP->location, 0, g_cdp_regInfo->location_len+1);
		memcpy(targetP->location, g_cdp_regInfo->server_location, g_cdp_regInfo->location_len);
		targetP = targetP->next;
	}

	for (i = 0; i <g_cdp_regInfo->observed_count; i++)
	{
		backup_observed = &g_cdp_regInfo->observed[i];
		observedP = prv_findObserved(contextP, &backup_observed->uri);
		if (observedP == NULL)
		{
			observedP = (lwm2m_observed_t *)lwm2m_malloc(sizeof(lwm2m_observed_t));
			if (observedP == NULL) 
				return -1;
			allocatedObserver = true;
			memset(observedP, 0, sizeof(lwm2m_observed_t));
			memcpy(&(observedP->uri), &backup_observed->uri, sizeof(lwm2m_uri_t));
			cloud_mutex_lock(&contextP->observe_mutex,MUTEX_LOCK_INFINITY);
			observedP->next = contextP->observedList;
			contextP->observedList = observedP;
			cloud_mutex_unlock(&contextP->observe_mutex);
		}

		for(j = 0; j<backup_observed->wather_count; j++)
		{
			backup_watcherP = &backup_observed->watcherList[j];
			watcherP = observedP->watcherList;				
			if (watcherP == NULL)
			{
			   watcherP = (lwm2m_watcher_t *)lwm2m_malloc(sizeof(lwm2m_watcher_t));
			   if (watcherP == NULL)
			   {
				   if (allocatedObserver == true)
				   {
					   lwm2m_free(observedP);
				   }
				   return -1;
			   }
			   memset(watcherP, 0, sizeof(lwm2m_watcher_t));
			   watcherP->active = backup_watcherP->active;
			   watcherP->update = backup_watcherP->update;
			   watcherP->server = tmptargetP;
			   watcherP->format = backup_watcherP->format;
			   watcherP->tokenLen = backup_watcherP->tokenLen;
			   memcpy(watcherP->token, backup_watcherP->token, backup_watcherP->tokenLen);
			   watcherP->lastTime = backup_watcherP->lastTime;
			   watcherP->lastMid = backup_watcherP->lastMid;
			   watcherP->counter = backup_watcherP->counter;
			   memcpy(&watcherP->lastValue.asInteger, &backup_watcherP->lastValue.asInteger, sizeof(watcherP->lastValue));

			   cloud_mutex_lock(&contextP->observe_mutex,MUTEX_LOCK_INFINITY);
			   watcherP->next = observedP->watcherList;
			   observedP->watcherList = watcherP;
			   cloud_mutex_unlock(&contextP->observe_mutex);
			}
		}
	}

	return 0;
}

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
int cdp_bak_obsvInfo(lwm2m_context_t* context)
{
	//xy_printf("[CDP]backup observe info~");

	lwm2m_observed_t *observed;
	cdp_lwm2m_observed_t *tempObserved;
	lwm2m_watcher_t *watcherP;
	cdp_lwm2m_watcher_t *tempWatcher;
    
	g_softap_var_nv->cdpNextMid = context->nextMID;

	g_cdp_regInfo->observed_count = 0;
	memset(g_cdp_regInfo->observed, 0, sizeof(cdp_lwm2m_observed_t) * CDP_BACKUP_OBSERVE_MAX);

	observed = context->observedList;
	while (observed != NULL)
	{
		tempObserved = &(g_cdp_regInfo->observed[g_cdp_regInfo->observed_count]);
		memcpy(&tempObserved->uri, &observed->uri, sizeof(lwm2m_uri_t));
		g_cdp_regInfo->observed_count++;
		
		tempObserved->wather_count = 0;
		memset(tempObserved->watcherList, 0, sizeof(cdp_lwm2m_watcher_t) * CDP_BACKUP_OBSERVE_MAX);

		watcherP = observed->watcherList;
		while(watcherP != NULL)
		{
			tempWatcher = &(tempObserved->watcherList[tempObserved->wather_count]);
			tempWatcher->active = watcherP->active;
			tempWatcher->counter = watcherP->counter;
			tempWatcher->lastMid = watcherP->lastMid;
			tempWatcher->lastTime = watcherP->lastTime;
			memcpy(tempWatcher->token, watcherP->token, 8);
			tempWatcher->tokenLen = watcherP->tokenLen;
			tempWatcher->update = watcherP->update;
			tempWatcher->counter = watcherP->counter;
			memcpy(&tempWatcher->lastValue.asInteger, &watcherP->lastValue.asInteger, sizeof(watcherP->lastValue));
			tempObserved->wather_count++;
			watcherP = watcherP->next;
		}

		observed = observed->next;
	}

	return 0;
}

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
int cdp_bak_regInfo(lwm2m_context_t* contextP)
{
    if(contextP == NULL)
        return CLOUD_SAVE_NONEED_WRITE;

    if (contextP->state != STATE_READY)
    {
//        send_debug_str_to_at_uart("+DBGINFO:[CDP] reg_status err\r\n");
        return CLOUD_SAVE_NONEED_WRITE;
    }

	if(g_cdp_regInfo == NULL)
		g_cdp_regInfo = xy_malloc(sizeof(cdp_regInfo_t));
	memset(g_cdp_regInfo,0x00,sizeof(cdp_regInfo_t));	
    g_cdp_regInfo->resumeFlag = 1;	
    memcpy(g_cdp_regInfo->endpointname, contextP->endpointName, strlen(contextP->endpointName));
	
	//observe list 信息保存至g_cdp_regInfo节点中
	cdp_bak_obsvInfo(contextP);

	cdp_bak_svrInfo(contextP->serverList);

	return CLOUD_SAVE_NEED_WRITE;
}

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
int cdp_write_regInfo(cdp_regInfo_t* udp_info)
{
    int ret = -1;
	xy_assert(sizeof(cdp_regInfo_t) <= PAGE_AVAILABLE_SIZE);

	ret = cdp_bak_regInfo(g_phandle->lwm2m_context);
    if (ret != CLOUD_SAVE_NEED_WRITE)
    {
        return ret;
    }
    memcpy(udp_info,g_cdp_regInfo,sizeof(cdp_regInfo_t));
	return CLOUD_SAVE_NEED_WRITE;
}

/*****************************************************************************
Function    : cdp_resume_task
Description : resume cdp task
Input       : None
Output      : None
Return      : 0
*****************************************************************************/
int cdp_resume_task()
{
   if(!is_cdp_running())
   {
       //不在运行且创建句柄为空才创建
       if (!cdp_handle_exist())
       {
           creat_lwm2m_task();
           osSemaphoreAcquire(cdp_recovery_sem, osWaitForever);
           xy_printf("[CDP]recovery cdp wait SemaphoreAcquire");
       }
   }
   return 0;
}

int cdp_read_regInfo(int offset)
{
	xy_assert(sizeof(cdp_regInfo_t) <= PAGE_AVAILABLE_SIZE);
	
    if(g_cdp_regInfo == NULL)
        g_cdp_regInfo = xy_malloc(sizeof(cdp_regInfo_t));

    softap_printf(USER_LOG, WARN_LOG,"[resume_net_app_by_dl_pkt] cdp read flash \n");
    if(xy_ftl_read(CDP_BACKUP_ADDR + offset, (unsigned char *)g_cdp_regInfo, sizeof(cdp_regInfo_t))!= XY_OK)
    {
        memset(g_cdp_regInfo, 0x00, sizeof(cdp_regInfo_t));
    }
  
    g_cdp_localPort = g_cdp_regInfo->net_info.local_port;

    return 0;	
}


void cdp_resume_lock(void)
{
    osMutexAcquire(g_cdp_keepalive_update_mutex, osWaitForever);
    if(g_softap_fac_nv->save_cloud && g_cdp_keepalive_update == 0)
    {
    	//RTC唤醒后，协议栈可能还处于PSM态，不加锁，业务代码若有delay，则会进入idle，进而可能进入深睡
        xy_work_lock(LOCK_XY_LOCAL);
        g_cdp_keepalive_update = 1;
    }
    osMutexRelease(g_cdp_keepalive_update_mutex);
    return;
}

void cdp_resume_unlock(void)
{
    osMutexAcquire(g_cdp_keepalive_update_mutex, osWaitForever);
    if(g_softap_fac_nv->save_cloud && g_cdp_keepalive_update == 1)
    {
        xy_work_unlock(LOCK_XY_LOCAL);
        g_cdp_keepalive_update = 0;
    }
    osMutexRelease(g_cdp_keepalive_update_mutex);
    return;
}

rtc_timeout_cb_t cdp_notice_update_process(void *para)
{
    //不支持单核模式
    if(g_softap_fac_nv->work_mode != 0)
        return;

    send_debug_str_to_at_uart("+DBGINFO:[CDP] notice proxy update\r\n");

    cdp_resume_lock();

    if(cdp_handle_exist())
        return;
    //发消息到proxy线程  执行主动update
   send_msg_2_proxy(PROXY_MSG_RESUME_CDP_UPDATE,NULL,0);
   return;
}
void cdp_timeout_restart_process()
{
    int lifetime = 0;
    send_debug_str_to_at_uart("+DBGINFO:[CDP] timeout_restart\r\n");
    if(g_cdp_regInfo != NULL && g_cdp_regInfo->lifetime > 0)
        lifetime = g_cdp_regInfo->lifetime;
    else
        lifetime = LWM2M_DEFAULT_LIFETIME;
    //update 失败会去注册
    cdp_register(lifetime, COAP_MAX_TRANSMIT_WAIT/2);

    cdp_resume_unlock();
    g_cdp_timeout_restart_TskHandle= NULL;
    osThreadExit();
}

void cdp_attach_resume_process()
{
    softap_printf(USER_LOG, WARN_LOG, "\r\n dynamic start cdp\r\n");
    send_debug_str_to_at_uart("+DBGINFO:[CDP] attach_resume\r\n");

    if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
    {
        softap_printf(USER_LOG, WARN_LOG,"cdp server addr is empty!");
    }
    else if(start_cdp_lwm2m() == 0)
    {
        softap_printf(USER_LOG, WARN_LOG,"registed failed!");
    }

    g_cdp_resume_TskHandle= NULL;
    osThreadExit();
}

void cdp_netif_up_resume_process()
{
    int is_ip_changed = 0;

    is_ip_changed = is_IP_changed(CDP_TASK);
    if(is_ip_changed == IP_IS_CHANGED || is_ip_changed == IP_RECEIVE_ERROR)
    {
        cdp_resume_lock();
        if (!cdp_handle_exist())
        {
			resume_net_app(CDP_TASK);
        }
		
        if (is_cdp_running())
        {
            osMutexAcquire(g_upstream_mutex, portMAX_DELAY);
            cdp_update_proc(((handle_data_t *)g_phandle)->lwm2m_context->serverList);
            osMutexRelease(g_upstream_mutex);
        }
		else
			cdp_resume_unlock();
    }
    g_cdp_resume_TskHandle= NULL;
    osThreadExit();
}

void cdp_rtc_resume_update_process()
{
    int ret = 0;
    ret = resume_net_app(CDP_TASK);

    //未走恢复流程 或者恢复标志位错误且线程未恢复 要及时释放锁
    if(ret != RESUME_SUCCEED )
    {
        //恢复标志位为0，但线程已经在运行，标识业务被其他线程恢复，依靠业务自身线程释放锁
        if(ret == RESUME_FLAG_ERROR && cdp_handle_exist())
            softap_printf(USER_LOG, WARN_LOG, "\r\n [cdp]have resume by other task\r\n");
        else
            cdp_resume_unlock();
    }

    g_cdp_rtc_resume_TskHandle= NULL;
    osThreadExit();
}

void cdp_resume_timer_create(void)
{
    int lifetime = 0;
    if(g_phandle == NULL)
        return;

    lifetime = g_phandle->lwm2m_context->serverList->lifetime;
    if(lifetime <= 0)
        xy_assert(0);
    if(g_softap_fac_nv->save_cloud)
        xy_rtc_timer_create(RTC_TIMER_CDP,(int)CLOUD_LIFETIME_DELTA(lifetime),cdp_notice_update_process,NULL);

    return;
}

void cdp_resume_timer_delete(void)
{
    if(g_softap_fac_nv->save_cloud)
        xy_rtc_timer_delete(RTC_TIMER_CDP);
    return;
}

void cdp_resume_state_process(int code)
{
   switch (code)
   {
    case XY_STATE_REGISTERED:
        cdp_resume_unlock();
        cdp_resume_timer_create();
        break;
    case XY_STATE_REG_FAILED:
        cdp_resume_unlock();
        cdp_resume_timer_delete();//update失败，删除rtc_timer
        break;
    case XY_STATE_UPDATE_DONE:
        cdp_resume_unlock();
        cdp_resume_timer_create();
        break;
    case XY_STATE_DEREGISTERED:
        cdp_resume_unlock();
        cdp_resume_timer_delete();
        break;
    default:
        break;
    }
}

#endif
