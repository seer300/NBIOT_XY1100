
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "at_onenet.h"
#include "xy_cis_api.h"
#include "at_global_def.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "net_app_resume.h"
#include "onenet_backup_proc.h"
#include "cloud_utils.h"
#include "xy_net_api.h"
#include "softap_nv.h"
#if IS_DSP_CORE
#include "xy_cis_api.h"
#include <float.h>
#endif

/*******************************************************************************
 *						  Local function declarations						   *
 ******************************************************************************/

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
//int cis_net_attached = 0;

onenet_context_config_t onenet_context_configs[CIS_REF_MAX_NUM] = {0};
onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM] = {0};

uint8_t g_ONENET_ACK_TIMEOUT = 4;
//static int onenet_out_fd;
osMutexId_t g_onenet_mutex= NULL;
osSemaphoreId_t g_cis_del_sem = NULL;
osSemaphoreId_t g_cis_rcv_sem = NULL;
osSemaphoreId_t cis_recovery_sem = NULL;
osThreadId_t onenet_resume_task_id = NULL;
extern char *g_Remote_AT_Rsp;
extern bool ota_status_flag; 
extern void xy_fota_state_hook();
extern char* cis_cfg_tool(char* ip,unsigned int port,char is_bs,char* authcode,char is_dtls,char* psk,int *cfg_out_len);
//pump

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
static const cis_err_msg_t cis_err_msg_tbl[] = {
    ERR_CIS_IT(CIS_ERROR_STR_LEN),
    ERR_CIS_IT(CIS_PARAM_ERROR),
    ERR_CIS_IT(CIS_STATUS_ERROR),
    ERR_CIS_IT(CIS_UNKNOWN_ERROR),
};
/*******************************************************************************
 *						Inline function implementations 					   *
 ******************************************************************************/

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
static char *get_cis_err_string(int errno)
{
    int i;
    for (i = 0; i < sizeof(cis_err_msg_tbl) / sizeof(cis_err_msg_tbl[0]); ++i)
    {
        if (cis_err_msg_tbl[i].errcode == errno)
            return cis_err_msg_tbl[i].errname;
    }
    return NULL;
}
static int onet_set_rai(const char* raiType,uint8_t* raiflag)
{
	int ret = 0;
	if(raiType == NULL || raiflag == NULL)
		return -1;

	if(!strcmp(raiType, "0X200") || !strcmp(raiType, "0x200"))
	{
	    *raiflag = PS_SOCK_RAI_NO_UL_DL_FOLLOWED;
	}
	else if(!strcmp(raiType, "0X400") || !strcmp(raiType, "0x400"))
	{
	    *raiflag = PS_SOCK_ONLY_DL_FOLLOWED;
	}
	else if(strlen(raiType) == 0 ||!strcmp(raiType, "0"))
	{
	    *raiflag = PS_SOCK_RAI_NO_INFO;
	}	
	else
	{
		ret = -1;
	}
	xy_printf("[CIS]rai(%d)", *raiflag);
	return ret;
}
int onet_resume_task()
{
	osThreadAttr_t thread_attr = {0};

	if(!is_onenet_task_running(0) && onenet_resume_task_id==NULL)
	{
        if(cis_recovery_sem == NULL)
            cis_recovery_sem = osSemaphoreNew(0xFFFF, 0, NULL);
		thread_attr.name	   = "resume_onenet";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = 0x800;
		onenet_resume_task_id = osThreadNew((osThreadFunc_t)(onenet_resume_task),NULL, &thread_attr);
		if(cis_recovery_sem != NULL)
			osSemaphoreAcquire(cis_recovery_sem, osWaitForever);
	}
	
	return 0;
}

unsigned char get_coap_result_code(unsigned char  at_result_code)
{
    unsigned int index = 0;
    const struct result_code_map code_map[]=
    {
        { 1,  CIS_COAP_205_CONTENT },
        { 2,  CIS_COAP_204_CHANGED},
        { 11, CIS_COAP_400_BAD_REQUEST },
        { 12, CIS_COAP_401_UNAUTHORIZED },
        { 13, CIS_COAP_404_NOT_FOUND },
        { 14, CIS_COAP_405_METHOD_NOT_ALLOWED },
        { 15, CIS_COAP_406_NOT_ACCEPTABLE }
    };
    for(; index < sizeof(code_map)/sizeof(struct result_code_map); index++){
        if(code_map[index].at_result_code == at_result_code){
            return code_map[index].coap_result_code;
        }
    }
    return CIS_COAP_503_SERVICE_UNAVAILABLE;   
}

int check_coap_result_code(int code, enum onenet_rsp_type type)
{
	int res = -1;
	switch (type) {
    	case RSP_READ:
    	case RSP_OBSERVE:
    	case RSP_DISCOVER:
    		if (code == 1 || code == 11 || code == 12 
    			|| code == 13 || code == 14 || code == 15) {
    			res = 0;
    		}
    		break;
    	case RSP_WRITE:
    	case RSP_EXECUTE:
    	case RSP_SETPARAMS:
    		if (code == 2 || code == 11 || code == 12 
    			|| code == 13 || code == 14) {
    			res = 0;
    		}
    		break;
    	default:
    		break;
	}
	return res;
}

static char * gen_at_ok()
{
    char *at_result = xy_zalloc(strlen(AT_RSP_OK) + 1);
    memcpy(at_result, AT_RSP_OK, strlen(AT_RSP_OK));

    return at_result;
}
static char * gen_at_cis_error(unsigned int err_code)
{
    char *at_result = NULL;
#if VER_QUCTL260
    at_result = xy_zalloc(CIS_ERROR_STR_LEN);
	if(g_softap_fac_nv->cmee_mode == 0)
		snprintf(at_result, CIS_ERROR_STR_LEN, "\r\nERROR\r\n");
	else if(g_softap_fac_nv->cmee_mode == 1)
		snprintf(at_result, CIS_ERROR_STR_LEN, "\r\n+CIS ERROR: %d\r\n", err_code);
	else
		snprintf(at_result, CIS_ERROR_STR_LEN, "\r\n+CIS ERROR: %s\r\n", get_cis_err_string(err_code));
#else
    at_result = xy_zalloc(CIS_ERROR_STR_LEN);
	if(g_softap_fac_nv->cmee_mode == 0)
		snprintf(at_result, CIS_ERROR_STR_LEN, "\r\nERROR\r\n");
	else if(g_softap_fac_nv->cmee_mode == 1)
		snprintf(at_result, CIS_ERROR_STR_LEN, "\r\n+CIS ERROR:%d\r\n", err_code);
	else
		snprintf(at_result, CIS_ERROR_STR_LEN, "\r\n+CIS ERROR:%s\r\n", get_cis_err_string(err_code));
#endif
    return at_result;
}

int onet_at_get_read_value(char * at_buf, struct onenet_read * param)
{
	int ret = -1;
	char *src_data = NULL;
	char *stop_str = NULL;

	if (param->value_type == cis_data_type_integer) {
		if (param->len != 2 && param->len != 4 && param->len != 8)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_float) {
		if (param->len != 4 && param->len != 8)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_bool) {
		if (param->len != 1)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_string) {
		if (param->len > STR_VALUE_LEN)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_opaque) {
		if ((param->len * 2) > OPAQUE_VALUE_LEN)
			goto  ERR_PROC;
		//param->len = param->len * 2;
	} else {
		goto  ERR_PROC;
	}

	param->value = xy_zalloc(strlen(at_buf));

	if(param->value_type == cis_data_type_string)
	{
		if (get_ascii_data(",,,,,,,,%s",(char *) at_buf,param->len,param->value) != AT_OK) 
			goto  ERR_PROC;

		if(strlen(param->value) != param->len)
			goto  ERR_PROC;

	}
	else
	{
		if(param->value_type == cis_data_type_opaque)
		{
			src_data = xy_zalloc(strlen(at_buf));
			if (at_parse_param_2(",,,,,,,,%s", (char *)at_buf, (void *)&src_data) != AT_OK) 
			{
				goto  ERR_PROC;
			}

			if(param->len*2 != strlen(src_data) || strlen(src_data) == 0)
			{
				goto  ERR_PROC;
			}
			if (hexstr2bytes(src_data, param->len * 2, param->value, param->len) == -1) 
			{
				goto  ERR_PROC;
			}
		}
		else
		{
			if (at_parse_param_2(",,,,,,,,%s", (char *)at_buf, (void *)&param->value) != AT_OK) 
				goto  ERR_PROC;

			if(strlen(param->value) == 0)
				goto  ERR_PROC;
			
			if(param->value_type == cis_data_type_integer)
			{
				if((param->len == 2) && (((int)strtol(param->value, &stop_str, 10) <INT16_MIN) || ((int)strtol(param->value,NULL,10) >INT16_MAX)))
				{
					goto  ERR_PROC;
				}
				else if((param->len == 4) && (((long long)strtoll(param->value, &stop_str,10) <INT32_MIN) || ((long long)strtoll(param->value,NULL,10) >INT32_MAX)))
				{
					goto  ERR_PROC;
				}
				else if((param->len == 8) && (((long long)strtoll(param->value, &stop_str,10) <INT64_MIN) || ((long long)strtoll(param->value,NULL,10) >INT64_MAX)))
				{
					goto  ERR_PROC;
				}

				if(strlen(stop_str) != 0)
					goto  ERR_PROC;
	
			}
			else if(param->value_type == cis_data_type_float)
			{
				if((param->len == 4) && (atof(param->value) >FLT_MAX))
				{
					goto  ERR_PROC;
				}
				else if((param->len == 8) && (atof(param->value) >DBL_MAX))
				{
					goto  ERR_PROC;
				}
			}
			else if(param->value_type == cis_data_type_bool)
			{
				if((int)strtol(param->value,NULL,10)>1 || (int)strtol(param->value,NULL,10)<0)
					goto  ERR_PROC;
			}

		}		

	}	
	ret = 0;
ERR_PROC:	
	if(src_data != NULL)
		xy_free(src_data);

	return ret;
}

int onet_at_get_notify_value(char * at_buf, struct onenet_notify * param)
{
	int ret = -1;
	char *src_data = NULL;
	char *stop_str = NULL;

	//memcpy(at_buf,src_str,strlen(src_str));

	if (param->value_type == cis_data_type_integer) {
		if (param->len != 2 && param->len != 4 && param->len != 8)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_float) {
		if (param->len != 4 && param->len != 8)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_bool) {
		if (param->len != 1)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_string) {
		if (param->len > STR_VALUE_LEN)
			goto  ERR_PROC;
	} else if (param->value_type == cis_data_type_opaque) {
		if ((param->len * 2) > OPAQUE_VALUE_LEN)
			goto  ERR_PROC;
		//param->len = param->len * 2;
	} else {
		goto  ERR_PROC;
	}

	param->value = xy_zalloc(strlen(at_buf));

	if(param->value_type == cis_data_type_string)
	{
		if (get_ascii_data(",,,,,,,%s,", (char *)at_buf,param->len,param->value) != AT_OK) 
			goto  ERR_PROC;

		if(strlen(param->value) != param->len)
			goto  ERR_PROC;
		

	}
	else
	{
		if(param->value_type == cis_data_type_opaque)
		{
			src_data = xy_zalloc(strlen(at_buf));
			if (at_parse_param_2(",,,,,,,%s", (char *)at_buf, (void *)&src_data) != AT_OK) 
			{
				goto  ERR_PROC;
			}

			if(param->len*2 != strlen(src_data) || strlen(src_data) == 0)
			{
				goto  ERR_PROC;
			}
			if (hexstr2bytes(src_data, param->len * 2, param->value, param->len) == -1) 
			{
				goto  ERR_PROC;
			}
		}
		else
		{
			if (at_parse_param_2(",,,,,,,%s", (char *)at_buf, (void *)&param->value) != AT_OK) 
				goto  ERR_PROC;
			if(param->value_type == cis_data_type_integer)
			{
				if((param->len == 2) && (((int)strtol(param->value, &stop_str,10) <INT16_MIN) || ((int)strtol(param->value,NULL,10) >INT16_MAX)))
				{
					goto  ERR_PROC;
				}
				else if((param->len == 4) && (((long long)strtoll(param->value, &stop_str,10) <INT32_MIN) || ((long long)strtoll(param->value,NULL,10) >INT32_MAX)))
				{
					goto  ERR_PROC;
				}
				else if((param->len == 8) && (((long long)strtoll(param->value, &stop_str,10) <INT64_MIN) || ((long long)strtoll(param->value,NULL,10) >INT64_MAX)))
				{
					goto  ERR_PROC;
				}

				if(strlen(stop_str) != 0)
					goto  ERR_PROC;
			
			}
			else if(param->value_type == cis_data_type_float)
			{
				if((param->len == 4) && (atof(param->value) >FLT_MAX))
				{
					goto  ERR_PROC;
				}
				else if((param->len == 8) && (atof(param->value) >DBL_MAX))
				{
					goto  ERR_PROC;
				}
			}
			else if(param->value_type == cis_data_type_bool)
			{
				if((int)strtol(param->value,NULL,10)>1 || (int)strtol(param->value,NULL,10)<0)
					goto  ERR_PROC;
			}

		}

	}

	ret = 0;
ERR_PROC:	
	if(src_data != NULL)
		xy_free(src_data);

	return ret;

}

int onet_check_reqType_uri(et_callback_type_t type, int msgId,int objId, int insId, int resId)
{
	st_context_t* context = (st_context_t*)onenet_context_refs[0].onenet_context;
	st_request_t * request = NULL;

	cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
	request = (st_request_t *)CIS_LIST_FIND(context->requestList, msgId);
	cloud_mutex_unlock(&context->lockRequest);

	if(request == NULL)
		return -1;

	if(request->type != type)
		return -1;

	if(request->uri.flag & URI_FLAG_OBJECT_ID)
	{
		if(request->uri.objectId != objId)
			return -1;
	}

	if(request->uri.flag & URI_FLAG_INSTANCE_ID)
	{
		if(request->uri.instanceId != insId)
			return -1;
	}

	if(request->uri.flag & URI_FLAG_RESOURCE_ID)
	{
		if(request->uri.resourceId != resId)
			return -1;
	}

	return 0;
}

int onet_check_reqType(et_callback_type_t type, int msgId)
{

	int result = -1;
	st_context_t* context = (st_context_t*)onenet_context_refs[0].onenet_context;
	st_request_t * request = NULL;

	cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
	request = (st_request_t *)CIS_LIST_FIND(context->requestList, msgId);
	cloud_mutex_unlock(&context->lockRequest);

	if(request != NULL && request->type == type)
	{
	   	result = 0;
	}
	else{
		result = -1;
	}

	return result;
}

onenet_context_config_t* find_proper_onenet_context_config(int totalsize, int index, int currentsize)
{
    int i;
    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (onenet_context_configs[i].config_hex != NULL && onenet_context_configs[i].total_len == totalsize 
            && onenet_context_configs[i].index == index + 1 && onenet_context_configs[i].offset + currentsize <= totalsize)
        {
            return &onenet_context_configs[i];
        }
    }
    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (onenet_context_configs[i].config_hex == NULL)
        {
            return &onenet_context_configs[i];
        }
    }
    return NULL;
}

int is_onenet_state_ready(unsigned int ref)
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    if (ref >= CIS_REF_MAX_NUM)
    {
        return 0;
    }
    onenet_context_ref = &onenet_context_refs[ref];

    if (onenet_context_ref->onenet_context->stateStep == PUMP_STATE_READY && onenet_context_ref->onenet_context->server->status == STATE_REGISTERED)
    {
        return 1;
    }
    return 0;
}
int is_onenet_task_running(unsigned int ref)
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    if (ref >= CIS_REF_MAX_NUM)
    {
        return 0;
    }
    onenet_context_ref = &onenet_context_refs[ref];
    //if (onenet_context_ref == NULL)
    //{
    //    return 0;
    //}
    if (onenet_context_ref->onenet_context == NULL || onenet_context_ref->onet_at_thread_id == NULL)
    {
        return 0;
    }
    return 1;
}

void free_onenet_context_ref(onenet_context_reference_t *onenet_context_ref)
{
    onenet_context_ref->onenet_context = NULL;
    //onenet_context_ref->onet_at_thread_id = -1;
    onenet_context_ref->thread_quit = 0;
    g_cis_downstream_cb = NULL;
}

int onet_deinit(onenet_context_reference_t *onenet_context_ref)
{

	onenet_context_ref->thread_quit = 1;
	return 0;
}

int onet_init(onenet_context_reference_t *onenet_context_ref, onenet_context_config_t *onenet_context_config)
{
    int ret = 0;
    
    ret = cis_init((void **)&onenet_context_ref->onenet_context, onenet_context_config->config_hex, onenet_context_config->total_len, NULL);
    if (ret != CIS_RET_OK) 
    {
		onet_deinit(onenet_context_ref);
		return -1;
	}
    onenet_context_ref->onenet_context_config = onenet_context_config;
    return 0;
}

int get_free_onet_context_ref()
{
	int i;

    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (onenet_context_refs[i].onenet_context == NULL && onenet_context_refs[i].onet_at_thread_id == NULL)
        {
            return (int)&onenet_context_refs[i];
        }
    }
    return NULL;
}
extern osSemaphoreId_t cis_poll_sem;
static cis_time_t onet_get_lifewait(st_context_t *onenet_context, cis_time_t cur_sec)
{
	cis_time_t lifetime;
	cis_time_t interval;
	cis_time_t lasttime;
	cis_time_t notifytime;
	cis_time_t wait_sec = 0;
	cis_time_t step = 0;
	lifetime = onenet_context->lifetime;
	lasttime = onenet_context->server->registration;

	if(lifetime > COAP_MAX_TRANSMIT_WAIT)
	{
		notifytime = (cis_time_t)COAP_MAX_TRANSMIT_WAIT;
	}
	else if(lifetime > COAP_MIN_TRANSMIT_WAIT)
	{
		notifytime = COAP_MIN_TRANSMIT_WAIT;
	}
	else
	{
		notifytime = (cis_time_t)(lifetime / 2);
	}

	interval = lasttime + lifetime - cur_sec; 
	softap_printf(USER_LOG, WARN_LOG, "[CIS]lifewait interval (%d)s, notify(%d)", interval, notifytime);

    if(interval <= 0)
    {
        wait_sec = 0;
    }
    else
    {
    	if(g_softap_fac_nv->save_cloud )
    		step = lifetime - CLOUD_LIFETIME_DELTA(lifetime);
    	else
    		step = notifytime;
        xy_printf("[CIS] life timer step (%d)s", step);
        while((interval-step) <= 0)
        {
            step = step>>1;
        }

        wait_sec = interval - step;
    }

	xy_printf("[CIS] life timer wait_sec (%d)s", wait_sec);
	return wait_sec;
}

void onet_at_pump(void* param)
{
    onenet_context_reference_t *onenet_context_ref = (onenet_context_reference_t *)param;
    unsigned int ret;
	st_transaction_t * transacP = NULL;
	cis_time_t cur_sec;
	cis_time_t wait_sec;
	cis_time_t tmp_sec;

    while (onenet_context_ref != NULL && !onenet_context_ref->thread_quit && onenet_context_ref->onenet_context != NULL) {
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_pump(onenet_context_ref->onenet_context);
		//[XY]Add for onenet loop
		if(cis_poll_sem != NULL && onenet_context_ref->onenet_context->stateStep == PUMP_STATE_READY
			&& onenet_context_ref->onenet_context->server->status == STATE_REGISTERED)
		{
	        osMutexRelease(g_onenet_mutex);
			cur_sec = utils_gettime_s();
			wait_sec = onet_get_lifewait(onenet_context_ref->onenet_context, cur_sec);
			if(wait_sec <= 0)
			{
				continue;
			}

			transacP = onenet_context_ref->onenet_context->transactionList;
			if(transacP != NULL)
			{				
				tmp_sec = transacP->retransTime - cur_sec;
				if(tmp_sec <= 0)
					continue;
				else
				{
					xy_printf("[CIS]wait_sec(%d), tmp_sec(%d)", wait_sec, tmp_sec);
					if(tmp_sec <= wait_sec)
						osSemaphoreAcquire(cis_poll_sem, tmp_sec * 1000);
					else
						osSemaphoreAcquire(cis_poll_sem, wait_sec * 1000);
				}										
			}
			else
			{
				osSemaphoreAcquire(cis_poll_sem, wait_sec * 1000);
			}
		}
		else
		{
	        osMutexRelease(g_onenet_mutex);
			osDelay(50);
        }

	}
    
out:
	if(onenet_context_ref->onenet_context_config != NULL)
	{
		if (onenet_context_ref->onenet_context_config->config_hex != NULL)
		{
			xy_free(onenet_context_ref->onenet_context_config->config_hex);
			memset(onenet_context_ref->onenet_context_config, 0, sizeof(onenet_context_config_t));
		}
	}

    if (onenet_context_ref->onenet_context != NULL)
        cis_deinit((void **)&onenet_context_ref->onenet_context);
    free_onenet_context_ref(onenet_context_ref);
	xy_printf("onenetdown\n");
    //softap_TaskDelete_Index(tsk_onenet);
	onenet_context_ref->onet_at_thread_id = NULL;
	osThreadExit();
}

int onenet_miplcreate()
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_config_t *onenet_context_config = NULL;
	osThreadAttr_t thread_attr = {0};

	xy_printf("[CIS]recovery onenet");

    onenet_context_ref = get_free_onet_context_ref();
    if (onenet_context_ref == NULL) {
        goto failed;
    }

    onenet_context_config = &onenet_context_configs[onenet_context_ref->ref];

    if(g_onenet_regInfo->cloud_platform == CLOUD_PLATFORM_ANDLINK)
    {
#if ANDLINK_VER
        extern void andlink_at_pump(void* param);
        if (al_config_init((void **)&onenet_context_ref->onenet_context,g_onenet_regInfo->net_info.remote_ip,g_onenet_regInfo->net_info.remote_port,g_onenet_regInfo->onenet_user_config.al_bs_flag) < 0)
        {
            xy_printf("[ANDLINK]andlink_init failed");
            goto failed;
        }
        thread_attr.name = "andlink_thead";
        thread_attr.priority   = XY_OS_PRIO_NORMAL1;
        thread_attr.stack_size = 0x1000;
        onenet_context_ref->onet_at_thread_id = osThreadNew ((osThreadFunc_t)(andlink_at_pump),onenet_context_ref, &thread_attr);
#endif
    }
    else
    {
        if(g_softap_fac_nv->onenet_config_len == 0)
            goto failed;

        if (onenet_context_config->config_hex != NULL)
        {
            xy_free(onenet_context_config->config_hex);
        }
        memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
        onenet_context_config->config_hex = xy_zalloc(g_softap_fac_nv->onenet_config_len);
        memcpy(onenet_context_config->config_hex, g_softap_fac_nv->onenet_config_hex, g_softap_fac_nv->onenet_config_len);
        onenet_context_config->total_len = g_softap_fac_nv->onenet_config_len;
        onenet_context_config->offset = onenet_context_config->total_len;
        onenet_context_config->index = 0;

        if(g_onenet_regInfo->cloud_platform == CLOUD_PLATFORM_ONENET)
        {
            if (onet_init(onenet_context_ref, onenet_context_config) < 0)
            {
                xy_printf("[CIS]onet_init failed");
                goto failed;
            }
            thread_attr.name = "onenet_tk";

        }
        else if(g_onenet_regInfo->cloud_platform == CLOUD_PLATFORM_COMMON)
        {
#if LWM2M_COMMON_VER
            init_xy_lwm2m_config();
            xy_lwm2m_config->endpoint_name = xy_zalloc(strlen(g_onenet_regInfo->endpointname) + 1);
            strcpy(xy_lwm2m_config->endpoint_name, g_onenet_regInfo->endpointname);

            if (xy_lwm2m_init(onenet_context_ref, onenet_context_config) < 0)
            {
                xy_printf("[CIS]lwm2m init failed");
                goto failed;
            }
            thread_attr.name = "xy_lwm2m_tk";
#endif
        }

        thread_attr.priority   = XY_OS_PRIO_NORMAL1;
        thread_attr.stack_size = 0x1000;
        onenet_context_ref->onet_at_thread_id = osThreadNew ((osThreadFunc_t)(onet_at_pump),onenet_context_ref, &thread_attr);
    }

	return 0;
failed:
    if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
        onenet_context_config->config_hex = NULL;
		memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    }
	return  -1;
}



//static cis_ret_t onet_on_read_cb(void* context, cis_uri_t* uri, cis_instcount_t instcount, cis_mid_t mid)
cis_coapret_t onet_on_read_cb(void* context,cis_uri_t* uri,cis_mid_t mid)
{
	char *at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE);

	// (void)context;
	st_context_t *contextP = (st_context_t *)context;

	if (CIS_URI_IS_SET_RESOURCE(uri))
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (xy_lwm2m_config->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"read\",%d,%d,%d,%d\r\n",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_READ;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"read\",%d,%d,%d,%d",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
		{
#if	VER_QUCTL260
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLREAD: %d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLREAD:%d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId);
#endif
		}
	}
	else
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (xy_lwm2m_config->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"read\",%d,%d,%d,%d\r\n",
						 mid, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1, -1);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_READ;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"read\",%d,%d,%d,%d",
						 mid, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1, -1);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
		{
#if VER_QUECTEL
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLREAD: %d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1, -1);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLREAD:%d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1, -1);
#endif
		}
	}

	//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
#if LWM2M_COMMON_VER
	if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
	{
	}
	else
#endif
		send_rsp_str_to_ext(at_str);
	xy_free(at_str);
	return CIS_CALLBACK_CONFORM;
}
void get_write_flag(int i, int maxi, int *flag)
{
	if (i == 0) {
		*flag = 0;
	} else if (i == maxi) {
		*flag = 1;
	} else {
		*flag = 2;
	}

}
extern int xy_Remote_AT_Req(char *req_at);
cis_ret_t onet_notify_data(st_context_t *onenet_context, struct onenet_notify *param);
extern int xy_get_observeMsgId(st_context_t *contextP, cis_uri_t *uriP);

//attention: onenet server only support cis_data_type_opaque
cis_ret_t onet_miplnotify_req(st_context_t *onenet_context, struct onenet_notify *param)
{
	cis_ret_t ret = CIS_RET_INVILID;
	ret = onet_notify_data(onenet_context, param);
	return ret;
}

cis_coapret_t onet_on_write_cb(void* context,cis_uri_t* uri, const cis_data_t* value,cis_attrcount_t attrcount,cis_mid_t mid)
{
	int i;
	struct onenet_notify paramATRsp = {0};
	char *at_str = NULL;
	char *strBuffer = NULL;
	int str_len = 0;
	// (void)context;
	st_context_t *contextP = (st_context_t *)context;

	for (i = attrcount - 1; i >= 0; i--) {
		cis_data_t *data = value + (attrcount - 1 - i);
		int flag = 0;
		get_write_flag(i,attrcount - 1,&flag);
		switch (data->type) {
		//目前下发的所有写类型数据均按不透明型数据处理
		case cis_data_type_opaque: {
			str_len = ON_WRITE_LEN + data->asBuffer.length * 2;
			strBuffer = xy_zalloc(data->asBuffer.length * 2+1);
			at_str = xy_zalloc(str_len);
			xy_printf("[CIS]data:(%s)", data->asBuffer.buffer);
			xy_Remote_AT_Req(data->asBuffer.buffer);			
			if (bytes2hexstr((char *)data->asBuffer.buffer, data->asBuffer.length, strBuffer, data->asBuffer.length * 2+1) <= 0 ){
				break;
			}
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (xy_lwm2m_config->access_mode == 0)
					snprintf(at_str, str_len, "\r\n+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%s,%d\r\n",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, strBuffer, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,%s,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, strBuffer, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
#if VER_QUCTL260
				if(g_softap_fac_nv->write_format == 0)
					snprintf(at_str, str_len, "\r\n+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%s,%d,%d\r\n",
						 	CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, strBuffer, flag, i);
				else
					snprintf(at_str, str_len, "\r\n+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%s,%d,%d\r\n",
						 	CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, data->asBuffer.buffer, flag, i);
#else
				snprintf(at_str, str_len, "\r\n+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%s,%d,%d\r\n",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 data->asBuffer.length, strBuffer, flag, i);
#endif
			}
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
#if LWM2M_COMMON_VER
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
			{
			}
			else
#endif
				send_rsp_str_to_ext(at_str);

			if(g_Remote_AT_Rsp != NULL)
			{
				cis_memset(&paramATRsp, 0, sizeof(paramATRsp));

				paramATRsp.ref = 0;
				paramATRsp.objId = uri->objectId;
				paramATRsp.insId = uri->instanceId;
				paramATRsp.resId = data->id;
				paramATRsp.value_type = cis_data_type_string;
				paramATRsp.index = 0;
				paramATRsp.flag = 0;
				paramATRsp.ackid = 0;
				paramATRsp.value = g_Remote_AT_Rsp;
				paramATRsp.len = strlen(g_Remote_AT_Rsp);
				cis_memcpy(paramATRsp.value, g_Remote_AT_Rsp, strlen(g_Remote_AT_Rsp));	

				paramATRsp.msgId = xy_get_observeMsgId(onenet_context_refs[0].onenet_context, uri);
				xy_printf("[CIS]Remote AT mid(%d)", paramATRsp.msgId);
				if(paramATRsp.msgId == -1)
				{
					if(paramATRsp.value != NULL)
					{
						xy_free(paramATRsp.value);
						g_Remote_AT_Rsp = NULL;
					}
					break;
				}

				onet_miplnotify_req(onenet_context_refs[paramATRsp.ref].onenet_context, &paramATRsp);
				if(paramATRsp.value != NULL)
				{
					xy_free(paramATRsp.value);
					g_Remote_AT_Rsp = NULL;
				}
			}			
			break;
		}
		case cis_data_type_string: {
			str_len = ON_WRITE_LEN + data->asBuffer.length;
			at_str = xy_zalloc(str_len);
			xy_Remote_AT_Req(data->asBuffer.buffer);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (xy_lwm2m_config->access_mode == 0)
					snprintf(at_str, str_len, "\r\n+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,\"%s\",%d\r\n",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, data->asBuffer.buffer, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,\"%s\",%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, data->asBuffer.buffer, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
				snprintf(at_str, str_len, "\r\n+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,\"%s\",%d,%d\r\n",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 data->asBuffer.length, data->asBuffer.buffer, flag, i);
				
			}
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
#if LWM2M_COMMON_VER
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
			{
			}
			else
#endif
				send_rsp_str_to_ext(at_str);

			if(g_Remote_AT_Rsp != NULL)
			{
				cis_memset(&paramATRsp, 0, sizeof(paramATRsp));

				paramATRsp.ref = 0;
				paramATRsp.objId = uri->objectId;
				paramATRsp.insId = uri->instanceId;
				paramATRsp.resId = data->id;
				paramATRsp.value_type = cis_data_type_string;
				paramATRsp.index = 0;
				paramATRsp.flag = 0;
				paramATRsp.ackid = 0;
				paramATRsp.value = g_Remote_AT_Rsp;
				paramATRsp.len = strlen(g_Remote_AT_Rsp);
				cis_memcpy(paramATRsp.value, g_Remote_AT_Rsp, strlen(g_Remote_AT_Rsp));	

				paramATRsp.msgId = xy_get_observeMsgId(onenet_context_refs[0].onenet_context, uri);
				xy_printf("[CIS]Remote AT mid(%d)", paramATRsp.msgId);
				if(paramATRsp.msgId == -1)
				{
					if(paramATRsp.value != NULL)
					{
						xy_free(paramATRsp.value);
						g_Remote_AT_Rsp = NULL;
					}
				}

				onet_miplnotify_req(onenet_context_refs[paramATRsp.ref].onenet_context, &paramATRsp);
				if(paramATRsp.value != NULL)
				{
					xy_free(paramATRsp.value);
					g_Remote_AT_Rsp = NULL;
				}
			}
			break;
		}
		case cis_data_type_integer:
		{
			at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (xy_lwm2m_config->access_mode == 0)
					snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%d,%d\r\n",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 4, (int)data->value.asInteger, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,%d,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 4, (int)data->value.asInteger, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 4, (int)data->value.asInteger, flag, i);
			}
#if LWM2M_COMMON_VER
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
			{
			}
			else
#endif
				send_rsp_str_to_ext(at_str);
			break;
		}
		case cis_data_type_float:
		{
			at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (xy_lwm2m_config->access_mode == 0)
					snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%.3f,%d\r\n",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 8, data->value.asFloat, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,%.3f,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 8, data->value.asFloat, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%.3f,%d,%d\r\n",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 8, data->value.asFloat, flag, i);
			}
#if LWM2M_COMMON_VER
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
			{
			}
			else
#endif
				send_rsp_str_to_ext(at_str);
			break;
		}
		case cis_data_type_bool:
		{
			at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (xy_lwm2m_config->access_mode == 0)
					snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%d,%d\r\n",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 1, data->value.asBoolean, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,%d,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 1, data->value.asBoolean, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 1, data->value.asBoolean, flag, i);
			}
#if LWM2M_COMMON_VER
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
			{
			}
			else
#endif
				send_rsp_str_to_ext(at_str);
			break;
		}
		default:
			return CIS_CALLBACK_METHOD_NOT_ALLOWED;
		}

		if(at_str != NULL)
			xy_free(at_str);
		if(strBuffer != NULL)
			xy_free(strBuffer);

	}
	
	return CIS_CALLBACK_CONFORM;
}

//static cis_ret_t onet_on_execute_cb(void* context, cis_uri_t* uri, const uint8_t* buffer, uint32_t length, cis_mid_t mid)
cis_coapret_t onet_on_execute_cb(void* context, cis_uri_t* uri, const uint8_t* value, uint32_t length,cis_mid_t mid)
{
	char *at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE + length + 5);

	// (void)context;
	st_context_t *contextP = (st_context_t *)context;

	if (length > 0)
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (xy_lwm2m_config->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"execute\",%d,%d,%d,%d\r\n",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_EXECUTE;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"execute\",%d,%d,%d,%d",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
		{
#if VER_QUCTL260
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEXECUTE: %d,%d,%d,%d,%d,%d,\"",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId, length);
			memcpy(at_str + strlen(at_str), value, length);
			strcat(at_str, "\"\r\n");

#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEXECUTE:%d,%d,%d,%d,%d,%d,",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId, length);
			memcpy(at_str + strlen(at_str), value, length);
			strcat(at_str, "\r\n");
#endif
			
		}
	}
	else
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (xy_lwm2m_config->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"execute\",%d,%d,%d,%d\r\n",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_EXECUTE;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"execute\",%d,%d,%d,%d",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
#if VER_QUCTL260
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEXECUTE: %d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEXECUTE:%d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId);
#endif

	}
#if LWM2M_COMMON_VER
	//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
	if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
	{
	}
	else
#endif
		send_rsp_str_to_ext(at_str);
	xy_free(at_str);

	return CIS_CALLBACK_CONFORM;
}
cis_coapret_t onet_on_observe_cb(void* context, cis_uri_t* uri, bool flag, cis_mid_t mid)
{
    char *at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE);

	// (void)context;
	st_context_t *contextP = (st_context_t *)context;

	if (CIS_URI_IS_SET_RESOURCE(uri))
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (xy_lwm2m_config->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"observe\",%d,%d,%d,%d,%d\r\n",
						 mid, flag, uri->objectId, uri->instanceId, uri->resourceId);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_OBSERVE;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"observe\",%d,%d,%d,%d,%d",
						 mid, flag, uri->objectId, uri->instanceId, uri->resourceId);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
#if VER_QUCTL260
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLOBSERVE: %d,%d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, flag, uri->objectId, uri->instanceId, uri->resourceId);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLOBSERVE:%d,%d,%d,%d,%d,%d\r\n",
					 CIS_REF, mid, flag, uri->objectId, uri->instanceId, uri->resourceId);
#endif
	}
	else
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (xy_lwm2m_config->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+QLAURC: \"observe\",%d,%d,%d,%d,-1\r\n",
						 mid, flag, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_OBSERVE;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"observe\",%d,%d,%d,%d,-1",
						 mid, flag, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
#if VER_QUCTL260
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLOBSERVE: %d,%d,%d,%d,%d,-1\r\n",
					 CIS_REF, mid, flag, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLOBSERVE:%d,%d,%d,%d,%d,-1\r\n",
					 CIS_REF, mid, flag, uri->objectId, CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1);		
#endif
	}

    if(g_softap_fac_nv->obs_autoack)
    {
        cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, mid, get_coap_result_code(1),0);
    }
#if LWM2M_COMMON_VER
	//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
	if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->access_mode == 1)
	{
	}
	else
#endif
	send_rsp_str_to_ext(at_str);
    xy_free(at_str);

	return CIS_CALLBACK_CONFORM;
}

void onet_on_parameter_cb_ex(uint8_t toSet, uint8_t flag,char *str_params, const char *print,int param, uint8_t type)
{
    if ((toSet & flag) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
		if (type)
			snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), print, (float)param);
		else
        	snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), print, param);
    }
}

cis_coapret_t onet_on_parameter_cb(void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid)
{
    char *at_str = xy_zalloc(MAX_SET_PARAM_AT_SIZE);
    char *str_params = xy_zalloc(MAX_SET_PARAM_SIZE);
    cis_iid_t ins_id = CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1;
    cis_rid_t res_id = CIS_URI_IS_SET_RESOURCE(uri) ? uri->resourceId : -1;
	//uint8_t toSet = parameters.toSet;

	(void) context;

    if ((parameters.toSet & ATTR_FLAG_MIN_PERIOD) != 0) {
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "pmin=%d",  parameters.minPeriod);
    }
    if ((parameters.toSet & ATTR_FLAG_MAX_PERIOD) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "pmax=%d",  parameters.maxPeriod);
    }
	if ((parameters.toSet & ATTR_FLAG_LESS_THAN) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "lt=%.1f",  parameters.lessThan);
    }
    if ((parameters.toSet & ATTR_FLAG_GREATER_THAN) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "gt=%.1f",  parameters.greaterThan);
    }   
    if ((parameters.toSet & ATTR_FLAG_STEP) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "st=%.1f",  parameters.step);
    }
#if VER_QUCTL260
    snprintf(at_str, MAX_SET_PARAM_AT_SIZE, "\r\n+MIPLPARAMETER: %d,%d,%d,%d,%d,%d,\"%s\"\r\n", CIS_REF, mid, uri->objectId, 
             ins_id, res_id, strlen(str_params), str_params);
#else
	snprintf(at_str, MAX_SET_PARAM_AT_SIZE, "\r\n+MIPLPARAMETER:%d,%d,%d,%d,%d,%d,\"%s\"\r\n", CIS_REF, mid, uri->objectId, 
             ins_id, res_id, strlen(str_params), str_params);
#endif
    //at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
    send_rsp_str_to_ext(at_str);
    xy_free(str_params);
    xy_free(at_str);
    return CIS_CALLBACK_CONFORM;
}
//static cis_ret_t onet_on_discover_cb(void* context,cis_uri_t* uri, cis_instcount_t instcount,cis_mid_t mid)
cis_coapret_t onet_on_discover_cb(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    char *at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE);

    (void) context;

#if VER_QUCTL260
	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLDISCOVER: %d,%d,%d\r\n", CIS_REF, mid, uri->objectId); //len parameter
#else
	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLDISCOVER:%d,%d,%d\r\n", CIS_REF, mid, uri->objectId);
#endif

	//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
	send_rsp_str_to_ext(at_str);
    xy_free(at_str);
	return CIS_CALLBACK_CONFORM;
}
#if LWM2M_COMMON_VER
extern int convert_cis_event_to_common_urc(char *at_str, int max_len, int cis_eid, void *param);
#endif
//static void onet_on_event_cb(void* context, cis_evt_t eid)
void onet_on_event_cb(void* context, cis_evt_t eid, void* param)
{
	char *at_str = NULL;

	// (void) context;
	st_context_t *contextP = (st_context_t *)context;

	if (eid == CIS_EVENT_UPDATE_SUCCESS || eid == CIS_EVENT_UPDATE_FAILED
		|| eid == CIS_EVENT_UPDATE_TIMEOUT) {
//		if (g_auto_update) {
//			g_auto_update = false;
//			return;
//		}
	}
	
	at_str = xy_zalloc(MAX_ONE_NET_AT_SIZE);
	if(eid == CIS_EVENT_RECOVERY_DONE)
	{
		snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+CLOUD:2\r\n");
		send_rsp_str_to_ext(at_str);
		
		xy_free(at_str);
		return ;
	}

	xy_fota_state_hook(eid - 40);
	onenet_resume_state_process(eid);
	//change fota report information
    switch(eid)
    {
        case CIS_EVENT_FIRMWARE_DOWNLOADING:
    	{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE DOWNLOADING");
           	break;
    	}
        case CIS_EVENT_FIRMWARE_DOWNLOAD_FAILED:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE DOWNLOAD FAILED");
           	break;
		}
        case CIS_EVENT_FIRMWARE_DOWNLOADED:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE DOWNLOADED");
           	break;
		}
        case CIS_EVENT_FIRMWARE_UPDATING:
    	{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE UPDATING");
           	break;
    	}
        case CIS_EVENT_FIRMWARE_UPDATE_SUCCESS:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE UPDATE SUCCESS");
           	break;
		}
        case CIS_EVENT_FIRMWARE_UPDATE_FAILED:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE UPDATE FAILED");
           	break;
		}
        case CIS_EVENT_FIRMWARE_UPDATE_OVER:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE UPDATE OVER");
           	break;
		}
        case CIS_EVENT_FIRMWARE_ERASE_SUCCESS:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+FIRMWARE ERASE SUCCESS");
           	break;
		}
        default:
        {
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (convert_cis_event_to_common_urc(at_str, MAX_ONE_NET_AT_SIZE, eid, param) < 0)
				goto out;
#endif
		}
		else
		{
#if VER_QUCTL260
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEVENT: %d,%d", CIS_REF, eid);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "\r\n+MIPLEVENT:%d,%d", CIS_REF, eid);
#endif
		}
		break;
	}
	}
	if (contextP->cloud_platform != CLOUD_PLATFORM_COMMON)
	{
		switch (eid)
		{
		case CIS_EVENT_NOTIFY_SUCCESS:
		case CIS_EVENT_NOTIFY_FAILED:
		case CIS_EVENT_UPDATE_NEED:
		case CIS_EVENT_RESPONSE_FAILED:
        {
	       snprintf(at_str + strlen(at_str), MAX_ONE_NET_AT_SIZE - strlen(at_str), ",%d", (int)param);
           break;
        }

		default:
		{
			break;
		}
		}
	}
	snprintf(at_str + strlen(at_str), MAX_ONE_NET_AT_SIZE - strlen(at_str), "\r\n");
#if !SPECIAL_USER1
	send_rsp_str_to_ext(at_str);
#endif
out:
	//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
	xy_free(at_str);
    if(g_cis_downstream_cb != NULL)
        g_cis_downstream_cb(CALLBACK_TYPE_EVENT, NULL, NULL, NULL, eid, cis_data_type_undefine, NULL, 0);

}

int onet_register(void* context, unsigned int lifetime)
{
	cis_callback_t callback;
	callback.onRead = onet_on_read_cb;
	callback.onWrite = onet_on_write_cb;
	callback.onExec = onet_on_execute_cb;
	callback.onObserve = onet_on_observe_cb;
    callback.onDiscover = onet_on_discover_cb;
	callback.onSetParams = onet_on_parameter_cb;
	callback.onEvent = onet_on_event_cb;

    osThreadSetLowPowerFlag(onenet_context_refs[0].onet_at_thread_id, osLowPowerPermit);//功耗考虑：调用close之后，标志位置0不参与standby唤醒，注册时需要置1
	return cis_register(context, lifetime, &callback);
}

cis_ret_t onet_mipladdobj_req(st_context_t *onenet_context, struct onenet_addobj *para)
{
	cis_ret_t ret = 0;
	cis_inst_bitmap_t bitmap = {0};
	uint8_t * instPtr;
	uint16_t instBytes;
	int i;
	cis_res_count_t  rescount;
	//make bitmap;
	bitmap.instanceCount = para->insCount;
	//bitmap.instanceBitmap = addobj->insBitmap; //"1101010111"

	instBytes = (bitmap.instanceCount - 1) / 8 + 1;
	instPtr = (uint8_t*)cis_malloc(instBytes);
	memset(instPtr, 0, instBytes);

	for (i = 0; i < bitmap.instanceCount; i++) {
		uint8_t instBytePos = i / 8;
		uint8_t instByteOffset = 7 - (i % 8);
		if (*(para->insBitmap + i) == '1') {
			instPtr[instBytePos] += 0x01 << instByteOffset;
		}
	}
	bitmap.instanceBitmap = instPtr;
	bitmap.instanceBytes = (para->insCount - 1) / 8 + 1;

	rescount.attrCount = para->attrCount;
    rescount.actCount = para->actCount;

	ret = cis_addobject(onenet_context, para->objId, &bitmap, &rescount);

	xy_free(instPtr);
	return ret;
}

cis_ret_t onet_read_param(struct onenet_read *param, cis_data_t* dataP)
{
    dataP->type = (cis_datatype_t)param->value_type;
    dataP->id = param->resId;
    if (dataP->type == cis_data_type_integer) {
        dataP->value.asInteger = (long long)strtoll(param->value,NULL,10);//atoll
    } else if (dataP->type == cis_data_type_float) {
        dataP->value.asFloat = atof((char *)param->value);
    } else if (dataP->type == cis_data_type_bool) {
        dataP->value.asBoolean = (int)strtol(param->value,NULL,10);
    } else if (dataP->type == cis_data_type_string) {
        dataP->asBuffer.length = param->len;
        dataP->asBuffer.buffer = (uint8_t*)(param->value);//(uint8_t*)strdup(param->value);
    } else if (dataP->type == cis_data_type_opaque) {
        dataP->asBuffer.length = param->len;
        dataP->asBuffer.buffer = (uint8_t*)(param->value);//(uint8_t*)strdup(param->value);
    } else {
        return CIS_CALLBACK_NOT_FOUND;
    }
    return CIS_RET_OK;
}

cis_ret_t onet_read_data(st_context_t *onenet_context, struct onenet_read *param)
{
	cis_data_t tmpdata = {0};
	cis_uri_t uri = {0};
	cis_ret_t ret = 0;
	cis_coapret_t result = CIS_RESPONSE_CONTINUE;
	
	if (param->flag != 0 && param->index == 0) {
		return CIS_RET_PARAMETER_ERR;
	}

	uri.objectId = param->objId;
	uri.instanceId = param->insId;
	uri.resourceId = param->resId;
	cis_uri_update(&uri);

	if ((ret = onet_read_param(param, &tmpdata)) != CIS_RET_OK) {
		return ret;
	}
	
	if (param->flag == 0 && param->index == 0) {
		result = CIS_RESPONSE_READ;
	}
	
	ret = cis_response(onenet_context, &uri, &tmpdata, param->msgId, result, param->raiflag);

	return ret;
}

cis_ret_t onet_notify_data(st_context_t *onenet_context, struct onenet_notify *param)
{
	cis_data_t tmpdata = {0};
	cis_uri_t uri = {0};
	cis_ret_t ret = 0;
	cis_coapret_t result = CIS_NOTIFY_CONTINUE;
	
	if (param->flag != 0 && param->index == 0) {
		return CIS_RET_PARAMETER_ERR;
	}

	if (param->ackid > 65535)
		return CIS_RET_PARAMETER_ERR;

	uri.objectId = param->objId;
	uri.instanceId = param->insId;
	uri.resourceId = param->resId;
	cis_uri_update(&uri);

	if ((ret = onet_read_param((struct onenet_read *)param, &tmpdata)) != CIS_RET_OK) {
		return ret;
	}

	if (param->flag == 0 && param->index == 0) {
		result = CIS_NOTIFY_CONTENT;
	}
	
	ret = cis_notify(onenet_context, &uri, &tmpdata, param->msgId, result, 1, param->ackid,param->raiflag);

	return ret;
}

//opt: onenet_context or ref as a parameter
cis_ret_t onet_miplread_req(st_context_t *onenet_context, struct onenet_read *param)
{
	cis_ret_t ret = CIS_RET_INVILID;

	ret = onet_read_data(onenet_context, param);
	return ret;
}

bool isPureNumAndAlpha(char *buf)
{
    while(*buf != '\0')
    {
        if(*buf < '0' || (*buf > '9' &&  *buf < 'A') || (*buf > 'Z' && * buf < 'a') || *buf > 'z')
        	return false;
        buf++;
    }

    return true;
}

/*****************************************************************************
 Function    : at_proc_miplconf_req
 Description : config onenet
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLCONFIG=<mode>,<parameter1>[,<parameter2>]
 *****************************************************************************/
#if VER_QUCTL260
int at_proc_miplconfig_req(char *at_buf, char **rsp_cmd)
{
	uint8_t mode = 0;
	uint8_t bs_mode = 1;
	uint8_t recv_data_format = -1;
	uint8_t auto_update = 1;
	uint16_t port = 5683;
	uint8_t ip_addr[16] = {'1','8','3','.','2','3','0','.','4','0','.','3','9','\0'};

	char *cis_user_config = NULL;
	uint32_t cfg_len = 0;

	if(g_req_type == AT_CMD_REQ)
	{
		//onenet任务开启状态下，不允许参数改变设置
		if (is_onenet_task_running(0) || NET_NEED_RECOVERY(ONENET_TASK))
		{
			*rsp_cmd = gen_at_cis_error(CIS_STATUS_ERROR);
			return AT_END;
		}

		memset(ip_addr, 0, sizeof(ip_addr));

		if (at_parse_param("%1d,%1d,%1d,%s,%2d,%1d", at_buf, &mode, &recv_data_format, &bs_mode, ip_addr, &port, &auto_update)!= AT_OK
				|| mode != 0 || (recv_data_format != 0 && recv_data_format != 1) || (bs_mode != 0 && bs_mode != 1)
				|| port < 1 || port > 65535 || (auto_update != 0 && auto_update != 1))
		{
			goto param_error;
		}

		if(strlen(ip_addr) == 0)
		{
			memset(ip_addr, 0, sizeof(ip_addr));
			if(bs_mode == 1)
				memcpy(ip_addr, "183.230.40.39", sizeof(ip_addr));
			else
				memcpy(ip_addr, "183.230.40.40", sizeof(ip_addr));
		}
		else
		{
			if(xy_ipaddr_check(ip_addr,IPV4_TYPE) == XY_ERR)
				goto param_error;
		}

		g_softap_fac_nv->keep_cloud_alive = auto_update;
		SAVE_FAC_PARAM(keep_cloud_alive);
		g_softap_fac_nv->write_format = recv_data_format;
		SAVE_FAC_PARAM(write_format);

		cis_user_config = cis_cfg_tool(ip_addr, port, bs_mode, NULL, NULL, NULL, &cfg_len);

		g_softap_fac_nv->onenet_config_len = cfg_len;
		memset(g_softap_fac_nv->onenet_config_hex, 0, sizeof(g_softap_fac_nv->onenet_config_hex));
		memcpy(g_softap_fac_nv->onenet_config_hex, cis_user_config, cfg_len);
		SAVE_FAC_PARAM(onenet_config_hex);
		SAVE_FAC_PARAM(onenet_config_len);
		if(cis_user_config)
			xy_free(cis_user_config);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		ciscfg_context_t *cfg_tmp = NULL;
		int len = 0;
		char port_tmp[10] = {0};
		uint8_t *tmp_ptr = NULL;

		//若config_hex存在，查询时从其中获取
		if(g_softap_fac_nv->onenet_config_len > 0)
		{
			if(cis_config_init(&cfg_tmp, g_softap_fac_nv->onenet_config_hex, g_softap_fac_nv->onenet_config_len) == 0)
			{
				tmp_ptr = strchr(cfg_tmp->cfgNet.host.data, ':');
				if(tmp_ptr != NULL)
				{
					len = tmp_ptr - cfg_tmp->cfgNet.host.data;
					memset(ip_addr, 0, sizeof(ip_addr));
					memcpy(ip_addr, cfg_tmp->cfgNet.host.data, len);
					memcpy(port_tmp, tmp_ptr+1, cfg_tmp->cfgNet.host.len - len - 1);
					port = (int)strtol(port_tmp,NULL,10);
				}
				bs_mode = cfg_tmp->cfgNet.bs_enabled;
			}

			if(cfg_tmp != NULL)
				xy_free(cfg_tmp);
		}

		*rsp_cmd = xy_zalloc(200);
		snprintf(*rsp_cmd, 200, "\r\n+MIPLCONFIG: 0,%d,%d,%s,%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->write_format, bs_mode, ip_addr, port, g_softap_fac_nv->keep_cloud_alive);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(70);
		snprintf(*rsp_cmd, 70, "\r\n+MIPLCONFIG: (0),(0,1),(0,1),\"ip\",\"port\"\r\n\r\nOK\r\n");
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_STATUS_ERROR);
	}

	return AT_END;

	param_error:
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
		return AT_END;
}

#else
int at_proc_miplconfig_req(char *at_buf, char **rsp_cmd)
{
    uint8_t mode = -1;
	uint8_t tmp = 0;
    uint8_t ack_timeout = 0;
    uint8_t obs_autoack = -1;
    uint8_t write_format = -1;
    uint8_t buf_cfg = -1;
    uint8_t buf_URC_mode = -1;
    uint32_t cfg_len = 0;
    char* cis_user_config = NULL;
    static uint8_t bs_enable = true;
    static uint8_t auth_enable = 0;
    static uint8_t dtls_enable = 0;
    static uint16_t port = 5683;
    static uint8_t ip_addr[16] = {'1','8','3','.','2','3','0','.','4','0','.','3','9','\0'};
    static uint8_t auth_code[20] = {0};
    static uint8_t psk[20] = {0};

    if(g_req_type == AT_CMD_REQ)
    {
        //onenet任务开启状态下，不允许参数改变设置
        if (is_onenet_task_running(0) || NET_NEED_RECOVERY(ONENET_TASK)) {
            *rsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            return AT_END;
        }

        if (at_parse_param("%1d",at_buf,&mode)!= AT_OK || mode > 7 )
        {
            goto param_error;
        }

        switch(mode)
        {
            case 0:
                //关闭引导模式，并配置接入服务器 IP 地址和端口号
                memset(ip_addr, 0, sizeof(ip_addr));
                if (at_parse_param(",%s,%2d",at_buf,ip_addr,&port)!= AT_OK || !strcmp(ip_addr,"") 
                    || port < 1 ||port > 65535 || xy_ipaddr_check(ip_addr,IPV4_TYPE) == XY_ERR)
                {
                    goto param_error;
                }
                bs_enable =false;
                cis_user_config = cis_cfg_tool(ip_addr,port,bs_enable,auth_code,dtls_enable,psk,&cfg_len);
                break;
            case 1:
                //开启引导模式，并配置引导服务器 IP 地址和端口号。默认的引导服务器 IP 地 址为 183.230.40.39，端口号为 5683
                memset(ip_addr, 0, sizeof(ip_addr));
                if (at_parse_param(",%s,%2d",at_buf,ip_addr,&port)!= AT_OK || port < 1 ||port > 65535
                    || xy_ipaddr_check(ip_addr,IPV4_TYPE) == XY_ERR)
                {
                    goto param_error;
                }
                if(!strcmp(ip_addr,""))
                    memcpy(ip_addr,"183.230.40.39",sizeof(ip_addr));
                if(port == 0)
                    port = 5683;
                bs_enable =true;
                cis_user_config = cis_cfg_tool(ip_addr,port,bs_enable,auth_code,dtls_enable,psk,&cfg_len);
                break;
            case 2:
                //设置 CoAP 协议的 ACK_TIMEOUT 参数，ACK_TIMEOUT 的默认值是 4 秒
                if (at_parse_param(",%1d,%1d",at_buf,&tmp,&ack_timeout)!= AT_OK || ack_timeout < 2 ||ack_timeout > 20 || tmp != 1)
                {
                    goto param_error;
                }
                g_softap_fac_nv->ack_timeout = g_ONENET_ACK_TIMEOUT = ack_timeout;
                SAVE_FAC_PARAM(ack_timeout);
                return AT_END;
            case 3:
                //设置是否启用自动响应订阅请求
                if (at_parse_param(",%1d",at_buf,&obs_autoack)!= AT_OK || obs_autoack > 1)
                {
                    goto param_error;
                }
                g_softap_fac_nv->obs_autoack = obs_autoack;
                SAVE_FAC_PARAM(obs_autoack);
                return AT_END;
            case 4:
                //设置是否使能鉴权接入并配置鉴权码(1-16个英文或数字)
                memset(auth_code, 0, sizeof(auth_code));
                if (at_parse_param(",%1d,%20s",at_buf,&auth_enable,auth_code)!= AT_OK || (auth_enable != 0 && auth_enable != 1))
                {
                    goto param_error;
                }
                if(auth_enable == 0)
                    memset(auth_code,0,sizeof(auth_code));
                else if(!strcmp(auth_code,"") || strlen(auth_code) > 16 ||!isPureNumAndAlpha(auth_code))
                    goto param_error;

                cis_user_config = cis_cfg_tool(ip_addr,port,bs_enable,auth_code,dtls_enable,psk,&cfg_len);
                break;
            case 5:
                //设置是否启用 DTLS 模式并配置PSK(8-16个英文或数字)
                goto param_error;   //暂不支持此功能，配置报错
                memset(psk, 0, sizeof(psk));
                if (at_parse_param(",%1d,%20s",at_buf,&dtls_enable,psk)!= AT_OK || (dtls_enable != 0 && dtls_enable != 1))
                {
                    goto param_error;
                }
                if(dtls_enable == 0)
                    memset(psk,0,sizeof(psk));
                else if(!strcmp(psk,"") || strlen(psk) < 8 || strlen(psk) > 16 || !isPureNumAndAlpha(psk) )
                    goto param_error;

                cis_user_config = cis_cfg_tool(ip_addr,port,bs_enable,auth_code,dtls_enable,psk,&cfg_len);

                break;
            case 6:
                //设置接收写数据的输出格式
                if (at_parse_param(",%1d",at_buf,&write_format)!= AT_OK || (write_format != 1 && write_format != 0))
                {
                    goto param_error;
                }
                g_softap_fac_nv->write_format = write_format;
                SAVE_FAC_PARAM(write_format);
                return AT_END;
            case 7:
                //下行数据缓存配置
                goto param_error;   //暂不支持此功能，配置报错
                if (at_parse_param(",%1d,%1d",at_buf,&buf_cfg,&buf_URC_mode)!= AT_OK || buf_cfg > 3||buf_URC_mode > 1 )
                {
                    goto param_error;
                }
                onenet_context_refs[0].onenet_user_config.buf_cfg = buf_cfg;
                onenet_context_refs[0].onenet_user_config.buf_URC_mode = buf_URC_mode;
                return AT_END;
        }

        //store factory_nv
        g_softap_fac_nv->onenet_config_len = cfg_len;
        memset(g_softap_fac_nv->onenet_config_hex, 0, sizeof(g_softap_fac_nv->onenet_config_hex));
        memcpy(g_softap_fac_nv->onenet_config_hex, cis_user_config, cfg_len);
        SAVE_FAC_PARAM(onenet_config_hex);
        SAVE_FAC_PARAM(onenet_config_len);
        if(cis_user_config)
            xy_free(cis_user_config);
        return  AT_END;
        param_error:
            *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    else if(g_req_type == AT_CMD_QUERY)
    {
        onenet_context_config_t config_tmp = {0};
        ciscfg_context_t *cfg_tmp = NULL;
        int len = 0;
        char port_tmp[10] = {0};
        uint8_t *tmp_ptr = NULL;
        
        //若config_hex存在，查询时从其中获取
        if(g_softap_fac_nv->onenet_config_len > 0)
        {
            config_tmp.config_hex = xy_zalloc(g_softap_fac_nv->onenet_config_len);
            memcpy(config_tmp.config_hex, g_softap_fac_nv->onenet_config_hex, g_softap_fac_nv->onenet_config_len);
            config_tmp.total_len = g_softap_fac_nv->onenet_config_len;

            if(cis_config_init(&cfg_tmp, config_tmp.config_hex, config_tmp.total_len) == 0)
            {
                tmp_ptr = strchr(cfg_tmp->cfgNet.host.data, ':');
                if(tmp_ptr != NULL) 
                {
                    len = tmp_ptr - cfg_tmp->cfgNet.host.data;
                    memset(ip_addr, 0, sizeof(ip_addr));
                    memcpy(ip_addr, cfg_tmp->cfgNet.host.data, len);
                    memcpy(port_tmp, tmp_ptr+1, cfg_tmp->cfgNet.host.len - len - 1);
                    port = (int)strtol(port_tmp,NULL,10);
                }
                bs_enable = cfg_tmp->cfgNet.bs_enabled;
                //user_data= "auth_code:xxxx;psk:xxxx;"
                tmp_ptr = strchr(cfg_tmp->cfgNet.user_data.data, ':');
                len = (uint8_t *)strchr(cfg_tmp->cfgNet.user_data.data, ';') - tmp_ptr - 1;
                memset(auth_code, 0, sizeof(auth_code));
                memcpy(auth_code, tmp_ptr+1, len);
            }

            if(config_tmp.config_hex != NULL)
                xy_free(config_tmp.config_hex); 
            if(cfg_tmp != NULL)
                xy_free(cfg_tmp);
        }

        *rsp_cmd = xy_zalloc(200);
        if(strlen(ip_addr) == 0)
            snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "\r\n+MIPLCONFIG:%d,NULL,%d\r\n", bs_enable,port);
        else
            snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "\r\n+MIPLCONFIG:%d,%s,%d\r\n", bs_enable,ip_addr,port);
        snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:2,%d\r\n", g_ONENET_ACK_TIMEOUT);
        snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:3,%d\r\n", g_softap_fac_nv->obs_autoack);
        if(strlen(auth_code) != 0)
            snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:4,1,%s\r\n",auth_code);
        else
            snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:4,0\r\n");
        if(dtls_enable)
            snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:5,1,%s\r\n",psk);
        else
            snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:5,0\r\n");
        snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:6,%d\r\n\r\nOK\r\n", g_softap_fac_nv->write_format );
        //snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:7,%d,%d\r\n\r\nOK\r\n", g_onenet_config_data->buf_cfg, g_onenet_config_data->buf_URC_mode);
    }
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(70);
		snprintf(*rsp_cmd, 70, "\r\n+MIPLCONFIG: <mode>,<parameter1>[,<parameter2>]\r\n\r\nOK\r\n");
	}
	else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }

    return AT_END;
}
#endif

//+MIPLCREATE=<totalsize>,<config>,<index>,<currentsize>,<flag>
int at_onenet_create_req(char *at_buf, char **rsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
#if VER_QUCTL260
    	*rsp_cmd = gen_at_cis_error(CIS_STATUS_ERROR);
#else
    	return at_proc_miplconf_req(at_buf,rsp_cmd);
#endif

    }
    else if(g_req_type == AT_CMD_ACTIVE)
    {
        return at_proc_miplcreate_req(at_buf,rsp_cmd);
    }
    else
    {
        *rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
    }
    return AT_END;
}

/*****************************************************************************
 Function    : at_proc_miplcreate_req
 Description : use the last configuration to create an instance of the basic communication suite
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLCREATE
 *****************************************************************************/
int at_proc_miplcreate_req(char *at_buf, char **rsp_cmd)
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_config_t *onenet_context_config = NULL;
    unsigned int errorcode = CIS_STATUS_ERROR;
	osThreadAttr_t thread_attr = {0};

    if(g_softap_fac_nv->onenet_config_len == 0)
        goto param_error;

    if(!ps_netif_is_ok()) {
#if VER_QUCTL260        
		errorcode = CIS_UNKNOWN_ERROR;
#else 
		errorcode = ATERR_NOT_NET_CONNECT;  
#endif     
		goto failed;
    }

    resume_net_app(ONENET_TASK);

    onenet_context_ref = (onenet_context_reference_t *)get_free_onet_context_ref();
    if (onenet_context_ref == NULL) {
        goto status_error;//todo 主动上报 区分602
    }
#if TELECOM_VER
    if (is_cdp_running() )
    {
        goto task_error;
    }
#endif

    onenet_context_config = &onenet_context_configs[onenet_context_ref->ref];
    if (onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
    }
    memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    onenet_context_config->config_hex = xy_zalloc(g_softap_fac_nv->onenet_config_len);
    memcpy(onenet_context_config->config_hex, g_softap_fac_nv->onenet_config_hex, g_softap_fac_nv->onenet_config_len);
    onenet_context_config->total_len = g_softap_fac_nv->onenet_config_len;
    onenet_context_config->offset = onenet_context_config->total_len;
    onenet_context_config->index = 0;

    if (onet_init(onenet_context_ref, onenet_context_config) < 0)
    {
#if VER_QUCTL260
		errorcode = CIS_SDK_ERROR;
		goto failed;
#else
    	goto status_error;
#endif
    }
    else
    {
        *rsp_cmd = xy_zalloc(32);
		thread_attr.name	   = "onenet_tk";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = 0x1000;
        onenet_context_ref->onet_at_thread_id = osThreadNew ((osThreadFunc_t)(onet_at_pump),onenet_context_ref, &thread_attr);
#if VER_QUCTL260
        sprintf(*rsp_cmd, "\r\n+MIPLCREATE: %d\r\n\r\nOK\r\n", onenet_context_ref->ref);
#else
        sprintf(*rsp_cmd, "\r\n+MIPLCREATE:%d\r\n\r\nOK\r\n", onenet_context_ref->ref);
#endif
    }

    return AT_END;
param_error:
    errorcode = CIS_PARAM_ERROR;
    goto failed;
status_error:
    errorcode = CIS_STATUS_ERROR;
    goto failed;
task_error:
#if VER_QUCTL260        
	errorcode = CIS_STATUS_ERROR;
#else 
	errorcode = ATERR_NOT_ALLOWED; 
#endif 
    goto failed;

failed:
    if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
        memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    }
    *rsp_cmd = gen_at_cis_error(errorcode);
    return  AT_END;
}

/*****************************************************************************
 Function    : at_proc_miplconf_req
 Description : create an instance of the basic communication suite
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLCREATE=<totalsize>,<config>,<index>,<currentsize>,<flag>
 *****************************************************************************/
int at_proc_miplconf_req(char *at_buf, char **rsp_cmd)
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_config_t *onenet_context_config = NULL;
    int  totalsize = 0;
    char *config_str = xy_zalloc(strlen(at_buf));
    char *config = NULL;
    int  index = -1;
    int  currentsize = 0;
    int  flag = -1;
    void *p[] = {&totalsize,config_str, &index, &currentsize, &flag};
    unsigned int errorcode = CIS_STATUS_ERROR;
	osThreadAttr_t thread_attr = {0};

    if(!ps_netif_is_ok()) {
        errorcode = ATERR_NOT_NET_CONNECT;
        goto failed;
    }

    resume_net_app(ONENET_TASK);

    if (at_parse_param_2("%d,%s,%d,%d,%d", at_buf, p) != AT_OK)
    {
        goto param_error;
    }

    if (totalsize <= 0 || currentsize <= 0 || index < 0 || flag < 0 || flag > 2 
		|| strlen(config_str) != currentsize * 2 || strlen(config_str) >= 120 * 2)
    {
        goto param_error;
    }

    config = xy_zalloc(currentsize);

    if (-1 == hexstr2bytes(config_str, currentsize * 2, config, currentsize))
    {
        goto param_error;
    }
#if TELECOM_VER
    if (is_cdp_running(0))
    {
        goto task_error;
    }
#endif

    onenet_context_config = find_proper_onenet_context_config(totalsize, index, currentsize);

    if (onenet_context_config == NULL)
    {
        goto status_error;
    }

    if (onenet_context_config->config_hex == NULL)
    {
        memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
        onenet_context_config->config_hex = xy_zalloc(totalsize);
        onenet_context_config->total_len = totalsize;
    }

    memcpy(onenet_context_config->config_hex + onenet_context_config->offset, config, currentsize);
    onenet_context_config->offset += currentsize;
    onenet_context_config->index = index;

    if (index ==0 && flag == 0)
    {
        if (onenet_context_config->total_len != onenet_context_config->offset)
        {
            goto param_error;
        }
        onenet_context_ref = get_free_onet_context_ref();
        if (onenet_context_ref == NULL) {
            goto status_error;
        }

        if (onet_init(onenet_context_ref, onenet_context_config) < 0)
        {
            goto status_error;
        }
        else
        {
            *rsp_cmd = xy_zalloc(32);
            sprintf(*rsp_cmd, "\r\n+MIPLCREATE:%d\r\n\r\nOK\r\n", onenet_context_ref->ref);
			
			thread_attr.name	   = "onenet_tk";
			thread_attr.priority   = XY_OS_PRIO_NORMAL1;
			thread_attr.stack_size = 0x1000;
            onenet_context_ref->onet_at_thread_id = osThreadNew((osThreadFunc_t)(onet_at_pump), onenet_context_ref, &thread_attr);

            //store factory_nv
            g_softap_fac_nv->onenet_config_len = onenet_context_config->total_len;
            memset(g_softap_fac_nv->onenet_config_hex, 0, sizeof(g_softap_fac_nv->onenet_config_hex));
            xy_assert(onenet_context_config->total_len < sizeof(g_softap_fac_nv->onenet_config_hex));
            memcpy(g_softap_fac_nv->onenet_config_hex, onenet_context_config->config_hex, onenet_context_config->total_len);
            SAVE_FAC_PARAM(onenet_config_hex);
            SAVE_FAC_PARAM(onenet_config_len);
        }
    }

    if (config_str != NULL)
        xy_free(config_str);
    if (config != NULL)
        xy_free(config);
    return  AT_END;
param_error:
    errorcode = CIS_PARAM_ERROR;
    goto failed;
status_error:
    errorcode = CIS_STATUS_ERROR;
    goto failed;
task_error:
    errorcode = ATERR_NOT_ALLOWED;
    goto failed;

failed:
    *rsp_cmd = gen_at_cis_error(errorcode);
    if (config_str != NULL)
        xy_free(config_str);
    if (config != NULL)
        xy_free(config);
    if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
        onenet_context_config->config_hex = NULL;
        memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    }

    if(onenet_context_ref != NULL)
    {
        if (onenet_context_ref->onenet_context != NULL)
            cis_deinit((void **)&onenet_context_ref->onenet_context);

        if (onenet_context_ref->onet_at_thread_id != NULL)
        {
            osThreadTerminate(onenet_context_ref->onet_at_thread_id);
            onenet_context_ref->onet_at_thread_id = NULL;
        }
        free_onenet_context_ref(onenet_context_ref);

    }

    return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLDELETE=<ref>
 Description : delete an instance of the basic communication suite
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLDELETE=<ref>
 *****************************************************************************/
int at_proc_mipldel_req(char *at_buf, char **rsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int ref = -1;
        void *param[]  = { &ref };
        int errorcode = CIS_PARAM_ERROR;

        if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
            goto failed;
        }

        resume_net_app(ONENET_TASK);


        if (at_parse_param_2("%d", at_buf, param) != AT_OK) {
            goto param_error;
        }

        if(ref < 0 || ref >= CIS_REF_MAX_NUM)
            goto param_error;

        if (!is_onenet_task_running(ref)) {
            if(onenet_context_configs[ref].config_hex != NULL)
            {
                xy_free(onenet_context_configs[ref].config_hex);
                memset(&onenet_context_configs[ref], 0, sizeof(onenet_context_config_t));
                *rsp_cmd = gen_at_ok();
                return AT_END;
            }
            else
            {
#if VER_QUCTL260
            	errorcode = CIS_UNKNOWN_ERROR;
                goto failed;
#else
				goto param_error;
#endif
            }
        }


#if VER_QUCTL260
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        cis_unregister(onenet_context_refs[ref].onenet_context);
        osMutexRelease(g_onenet_mutex);
        while (onenet_context_refs[ref].onenet_context != NULL && onenet_context_refs[ref].onenet_context->registerEnabled != 0)
        {
            osSemaphoreAcquire(g_cis_del_sem, osWaitForever);
        }
#endif
        onet_deinit(&onenet_context_refs[ref]);
#if !IS_DSP_CORE
		if(CHECK_SDK_TYPE(OPERATION_VER))
			xy_work_unlock(LOCK_EXT);
#endif		
        *rsp_cmd = gen_at_ok();
        return AT_END;
    param_error:
        errorcode = CIS_PARAM_ERROR;
        goto failed;
    status_error:
        errorcode = CIS_STATUS_ERROR;
        goto failed;
    failed:
        *rsp_cmd = gen_at_cis_error(errorcode);
        return AT_END;
    }
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(40);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 40, "\r\n+MIPLDELETE: (0)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 40, "\r\n+MIPLDELETE: <ref>\r\n\r\nOK\r\n");
#endif
	}
    else
    {
        *rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
    }
    return AT_END;
}

/*****************************************************************************
 Function    : +MIPLOPEN=<ref>,<lifetime>[,<timeout>]
 Description : used for registration to register the onenet platform
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLOPEN=<ref>,<lifetime>,[<timeout>]
 *****************************************************************************/
int at_proc_miplopen_req(char *at_buf, char **rsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int ref = 0;
        int lifetime = 0;
#if VER_QUCTL260
        int timeout = -1;
#else
        int timeout = 0;
#endif
        void *param[]  = { &ref , &lifetime, &timeout};
        unsigned int errorcode = CIS_PARAM_ERROR;
        int ret= CIS_RET_ERROR;

        if(!ps_netif_is_ok()) {
            goto net_error;
        }

        resume_net_app(ONENET_TASK);
#if VER_QUCTL260
        if (at_parse_param_2("%d,%d,%d", at_buf, param) != AT_OK)
			goto param_error;

        if(lifetime < LIFETIME_LIMIT_MIN || lifetime > LIFETIME_LIMIT_MAX)
        {
        	if(lifetime == 0)
        		lifetime = 3600;
        	else
        		goto param_error;
        }

        if(onenet_context_refs[ref].onenet_context->reg_times > 0)
        {
        	errorcode = CIS_UNKNOWN_ERROR;
        	goto failed;
        }

#else
        if (at_parse_param_2("%d,%d,%d", at_buf, param) != AT_OK 
			|| lifetime < LIFETIME_LIMIT_MIN || lifetime > LIFETIME_LIMIT_MAX) {
        		goto param_error;
        }
#endif

        if(ref < 0 || ref >= CIS_REF_MAX_NUM)
            goto param_error;


#if VER_QUCTL260
        if(timeout == -1)
			onenet_context_refs[0].onenet_user_config.reg_timeout = 0x1E;
		else if (timeout >= 1 && timeout <= 0xFFFF)
            onenet_context_refs[0].onenet_user_config.reg_timeout = timeout;
		else
			goto param_error;
#endif

        if (!is_onenet_task_running(ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
        }

        if(onenet_context_refs[ref].onenet_context->registerEnabled == true)
            goto status_error;
		
#if !IS_DSP_CORE
		if(CHECK_SDK_TYPE(OPERATION_VER))
			xy_work_lock(LOCK_EXT);
#endif

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_register(onenet_context_refs[ref].onenet_context, lifetime);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
#if VER_QUCTL260
        {
        	errorcode = CIS_SDK_ERROR;
            goto failed;
        }
#else
            goto param_error;
#endif
        *rsp_cmd = gen_at_ok();
        return AT_END;
    param_error:
#if !IS_DSP_CORE
		if(CHECK_SDK_TYPE(OPERATION_VER))
			xy_work_unlock(LOCK_EXT);
#endif
        errorcode = CIS_PARAM_ERROR;
        goto failed;
    net_error:
        errorcode = CIS_UNKNOWN_ERROR;
        goto failed;
    status_error:
        errorcode = CIS_STATUS_ERROR;
    failed:
        *rsp_cmd = gen_at_cis_error(errorcode);
        return AT_END;
    }
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(70);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 70, "\r\n+MIPLOPEN: (0),(0xF-0xFFFFFFF),(1-0xFFFF)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 70, "\r\n+MIPLOPEN: <ref>,<lifetime>[,<timeout>]\r\n\r\nOK\r\n");
#endif
	}
    else
    {
        *rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
    }
    return AT_END;
}


/*****************************************************************************
 Function    : +MIPLADDOBJ=<ref>,<objectid>,<instancecount>,<instancebitmap>,<attributecount>,<actioncount>
 Description : add a lwm2m object
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLADDOBJ=<ref>,<objectid>,<instancecount>,<instancebitmap>,
               <attributecount>,<actioncount>
 *****************************************************************************/
int at_proc_mipladdobj_req(char *at_buf, char **rsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_addobj param = {-1,-1,-1,0,-1,-1};
        param.insBitmap = xy_zalloc(strlen(at_buf));
        void *params[]  = {&param.ref, &param.objId, &param.insCount, param.insBitmap, &param.attrCount, &param.actCount};
        unsigned int errorcode = CIS_PARAM_ERROR;
        int ret= CIS_RET_ERROR;

        if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
            goto failed;
#endif
        }

        resume_net_app(ONENET_TASK);

        if (at_parse_param_2("%d,%d,%d,%s,%d,%d", at_buf, params) != AT_OK) {
            goto param_error;
        }

		
        if(param.ref<0  || param.ref >= CIS_REF_MAX_NUM || param.objId < 0 || param.objId > 65535
            || param.insCount < 0 || param.insCount > 8 || strlen(param.insBitmap) != param.insCount
            || param.attrCount < 0 || param.attrCount > 65535 || param.actCount < 0 || param.actCount > 65535)
        {
            goto param_error;
        }

        if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_mipladdobj_req(onenet_context_refs[param.ref].onenet_context,&param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_NO_ERROR) {
#if VER_QUCTL260
        	errorcode = CIS_SDK_ERROR;
            goto failed;
#else
            goto param_error;
#endif
        }

        if (param.insBitmap != NULL)
            xy_free(param.insBitmap);
        *rsp_cmd = gen_at_ok();
        return AT_END;
    param_error:
        errorcode = CIS_PARAM_ERROR;
        goto failed;
    status_error:
        errorcode = CIS_STATUS_ERROR;
    failed:
        if (param.insBitmap != NULL)
            xy_free(param.insBitmap);
        *rsp_cmd = gen_at_cis_error(errorcode);
        return  AT_END;
    }
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(100);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 100, "\r\n+MIPLADDOBJ: (0),(0-0xFFFF),(0-0xFFFF),<instanceBitmap>,(0-0xFFFF),(0-0xFFFF)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 100, "\r\n+MIPLADDOBJ: <ref>,<objID>,<inscount>,<insbitmap>,<attrcount>,<actcount>\r\n\r\nOK\r\n");
#endif
	}
    else
    {
        *rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
    }

    return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLDELOBJ=<ref>,<objectid>
 Description : delete a lwm2m object
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLDELOBJ=<ref>,<objectid>
 *****************************************************************************/
int at_proc_mipldelobj_req(char *at_buf, char **rsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_delobj param = {0};
		param.ref = -1;
		param.objId = -1;		
        void *params[]  = {&param.ref, &param.objId};
        unsigned int errorcode = CIS_PARAM_ERROR;
        int ret= CIS_RET_ERROR;

        if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
            goto failed;
        }

        resume_net_app(ONENET_TASK);

        if (at_parse_param_2("%d,%d", at_buf, params) != AT_OK) {
            goto param_error;
        }

        if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.objId < 0)
            goto param_error;

        if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_delobject(onenet_context_refs[param.ref].onenet_context, param.objId);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_NO_ERROR) {
#if VER_QUCTL260
        	if(ret == COAP_404_NOT_FOUND)
        		errorcode = CIS_UNKNOWN_ERROR;
        	else
        		errorcode = CIS_SDK_ERROR;

            goto failed;
#else
            goto param_error;
#endif
        }
        *rsp_cmd = gen_at_ok();
        return AT_END;
    param_error:
        errorcode = CIS_PARAM_ERROR;
        goto failed;
    status_error:
        errorcode = CIS_STATUS_ERROR;
    failed:
        *rsp_cmd = gen_at_cis_error(errorcode);
        return  AT_END;
    }
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(50);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 50, "\r\n+MIPLDELOBJ: (0),(0-0xFFFF)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 50, "\r\n+MIPLDELOBJ: <ref>,<objID>\r\n\r\nOK\r\n");
#endif
	}
    else
    {
        *rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
    }

    return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLDELOBJ=<ref>,<objectid>
 Description : used for deregistration to unregister the onenet platform
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLCLOSE=<ref>
 *****************************************************************************/
int at_proc_miplclose_req(char *at_buf, char **rsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int ref = -1;
        unsigned int errorcode = CIS_PARAM_ERROR;
        void *param[]  = {&ref};
        int ret= CIS_RET_ERROR;
#if VER_QUCTL260
        ret= CIS_RET_OK;
#endif
        if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
            goto failed;
        }

        resume_net_app(ONENET_TASK);

        if (at_parse_param_2("%d", at_buf, param) != AT_OK) {
            goto param_error;
        }

        if(ref < 0 || ref >= CIS_REF_MAX_NUM)
            goto param_error;

        if (!is_onenet_task_running(ref)) {
#if VER_QUCTL260
        	errorcode = CIS_UNKNOWN_ERROR;
            goto failed;
#else
			goto param_error;
#endif       
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
#if VER_QUCTL260
        if (onenet_context_refs[ref].onenet_context->registerEnabled == true || onenet_context_refs[ref].onenet_context->reg_times > 0) {
        	onenet_context_refs[ref].onenet_context->reg_times = 0;
        	ret = cis_unregister(onenet_context_refs[ref].onenet_context);
        }
#else
        if (onenet_context_refs[ref].onenet_context->registerEnabled == true)
            ret = cis_unregister(onenet_context_refs[ref].onenet_context);

#endif
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK) {
#if VER_QUCTL260
        	errorcode = CIS_SDK_ERROR;
            goto failed;
#else
			goto param_error;
#endif
        }

        *rsp_cmd = gen_at_ok();
        return AT_END;
    param_error:
        errorcode = CIS_PARAM_ERROR;
        goto failed;
    status_error:
        errorcode = CIS_STATUS_ERROR;
    failed:
        *rsp_cmd = gen_at_cis_error(errorcode);
        return  AT_END;
    }
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(40);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 40, "\r\n+MIPLCLOSE: (0)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 40, "\r\n+MIPLCLOSE: <ref>\r\n\r\nOK\r\n");
#endif
	}
    else
    {
        *rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
    }

    return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLNOTIFY=<ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,<valuetype>,<len>,<value>,<index>,<flag>[,<ackid>[,<raimode>]]
 Description : notify the basic communication suite of a numerical change
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLNOTIFY=<ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,
 	 	 	   <valuetype>,<len>,<value>,<index>,<flag>[,<ackid>[,<raimode>]]
 *****************************************************************************/
int at_proc_miplnotify_req(char *at_src_buf, char **rsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		st_observed_t* observe = NULL;
		struct onenet_notify param = {0};
		cis_ret_t ret = CIS_RET_INVILID;
	    unsigned int errorcode = CIS_PARAM_ERROR;
		char raiType[8] = {0};
		char *at_buf = NULL;
#if VER_QUECTEL
        st_uri_t uriP = {0};
#endif
		param.ref = -1;
		param.msgId = -1;
		param.objId = -1;
		param.insId = -1;
		param.resId = -1;
		param.index = -1;
		param.flag = -1;
		void *params[]  = {&param.ref, &param.msgId, &param.objId, &param.insId, &param.resId, &param.value_type, &param.len};
		void *params1[]  = {&param.index, &param.flag, &param.ackid, raiType};
		
		if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
			goto failed;
		}
		
        resume_net_app(ONENET_TASK);

		if(param.len > NET_MAX_USER_DATA_LEN){
#if VER_QUCTL260
            errorcode = CIS_PARAM_ERROR;
#else
            errorcode = ATERR_NOT_ALLOWED;
#endif
			goto failed;
		}

		//因为该命令内部含明文字符串参数，为了避免该参数内部含双引号或逗号的特殊字符，进而进行内存拷贝，以保证get_ascii_data执行转义不影响原始AT命令内容，从而也保证后续的参数能够使用at_parse_param来解析
		at_buf = xy_malloc(strlen(at_src_buf) + 1);
		strcpy(at_buf, at_src_buf);
		
		if (at_parse_param_2("%d,%d,%d,%d,%d,%d,%d", at_buf, params) != AT_OK) {
			goto param_error;
		}
		
		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msgId < 0 
			|| param.insId < 0 || param.insId > 8 || param.objId < 0 || param.objId > 65535
			|| param.resId < 0 || param.resId > 65535)
			goto param_error;	
		
		//value
		if (0 != onet_at_get_notify_value(at_buf, &param)) {
			goto param_error;
		}

#if VER_QUCTL260
		if(at_parse_param(",,,,,,,,%d,%d,%d,%1d", at_buf, &param.index, &param.flag, &param.ackid, &param.raiflag) != AT_OK
				|| param.index < 0 || param.flag < 0 || param.flag > 2 || param.raiflag < 0 || param.raiflag > 2)
			goto param_error;
#else
		if (at_parse_param_2(",,,,,,,,%d,%d,%d,%8s", at_buf, params1) != AT_OK) {
			goto param_error;
		}

		if(param.index < 0 || param.flag < 0 || param.flag > 2)
			goto param_error;	

		if(param.ackid != 0 && (!strcmp(raiType, "0X200") || !strcmp(raiType, "0x200")))
			goto param_error;	

		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif

		if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif		
		}

#if VER_QUECTEL
		uri_make(param.objId, param.insId, param.resId, &uriP);
        observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
        if (observe == NULL)
        {
            uri_make(param.objId, param.insId, URI_INVALID, &uriP);
            observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
        }

        if(observe != NULL)
        	param.msgId = observe->msgid;
#else
        observe = observe_findByMsgid(onenet_context_refs[param.ref].onenet_context, param.msgId);
#endif
        if(observe == NULL)
			goto param_error;
	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = onet_miplnotify_req(onenet_context_refs[param.ref].onenet_context, &param);
	    osMutexRelease(g_onenet_mutex);
		if (ret != CIS_RET_OK && ret != COAP_205_CONTENT)
#if VER_QUCTL260
		 {
        	errorcode = CIS_SDK_ERROR;
            goto failed;
		 }
#else
			goto param_error;
#endif

		*rsp_cmd = gen_at_ok();
		if (param.value != NULL)
			xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
		return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
	    goto failed;
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
	    *rsp_cmd = gen_at_cis_error(errorcode);
		if (param.value != NULL)
			xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
		return  AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(140);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 140, "\r\n+MIPLNOTIFY: (0),(0-0xFFFFFFF),(0-0xFFFF),(0-0xFFFF),(0-0xFFFF),(1-5),(1-0xFF),<value>,(0-10),(0-2),(0-0xFFFF),(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 140, "\r\n+MIPLNOTIFY: <ref>,<msgID>,<objID>,<insID>,<resID>,<value_type>,<len>,<value>,<index>,<flag>[,<ackID>[,<raimode>]]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}

	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLREADRSP=<ref>,<msgid>,<result>[,<objectid>,<instanceid>,<resourceid>,<valuetype>,<len>,<value>,<index>,<flag>[,<raimode>]]
 Description : notify the basic communication suite of an attribute change
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLREADRSP=<ref>,<msgid>,<result>[,<objectid>,<instanceid>,
 	 	 	   <resourceid>,<valuetype>,<len>,<value>,<index>,<flag>[,<raimode>]]
 *****************************************************************************/
int at_proc_miplread_req(char *at_src_buf, char **rsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		struct onenet_read param = {0};
		cis_coapret_t result;
	    unsigned int errorcode = CIS_PARAM_ERROR;
		char raiType[8] = {0};		
	    int ret= CIS_RET_ERROR;
		param.index = -1;
		param.flag = -1;
		char *at_buf = NULL;
		void *params[]  = {&param.ref, &param.msgId, &param.result, &param.objId, &param.insId, &param.resId, &param.value_type, &param.len};
		void *params1[]  = {&param.index, &param.flag, raiType};

		if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
			goto failed;
		}
		if(param.len > NET_MAX_USER_DATA_LEN){
#if VER_QUCTL260
            errorcode = CIS_PARAM_ERROR;
#else
            errorcode = ATERR_NOT_ALLOWED;
#endif
			goto failed;
		}

		//因为该命令内部含明文字符串参数，为了避免该参数内部含双引号或逗号的特殊字符，进而进行内存拷贝，以保证get_ascii_data执行转义不影响原始AT命令内容，从而也保证后续的参数能够使用at_parse_param来解析
		at_buf = xy_malloc(strlen(at_src_buf) + 1);
		strcpy(at_buf, at_src_buf);
		
		if (at_parse_param_2("%d,%d,%d,%d,%d,%d,%d,%d", at_buf, params) != AT_OK) {
			goto param_error;
		}

        if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif	
		}
        
		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msgId < 0
			|| param.insId < 0 || param.resId < 0 || param.len < 0)
			goto param_error;

		if (0 != check_coap_result_code(param.result, RSP_READ))
			goto param_error;

		if(onet_check_reqType_uri(CALLBACK_TYPE_READ, param.msgId, param.objId, param.insId, param.resId) != 0)
			goto param_error;
		
	    if ((result = get_coap_result_code(param.result)) != CIS_COAP_205_CONTENT){
	        osMutexAcquire(g_onenet_mutex, osWaitForever);
	        cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msgId, result,param.raiflag);
	        osMutexRelease(g_onenet_mutex);
	        goto out_ok;
	    }

		//value
		if (0 != onet_at_get_read_value(at_buf, &param)) {
			goto param_error;
		}

#if VER_QUCTL260
		if(at_parse_param(",,,,,,,,,%d,%d,%1d", at_buf, &param.index, &param.flag, &param.raiflag) != AT_OK
				|| param.index < 0 || param.flag < 0 || param.flag > 2 || param.raiflag < 0 || param.raiflag > 2)
			goto param_error;
#else
		if (at_parse_param_2(",,,,,,,,,%d,%d,%8s", at_buf, params1) != AT_OK) {
			goto param_error;
		}
		
		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif

	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = onet_miplread_req(onenet_context_refs[param.ref].onenet_context, &param);
	    osMutexRelease(g_onenet_mutex);
		if (ret != CIS_RET_OK)
#if VER_QUCTL260
		 {
        	errorcode = CIS_SDK_ERROR;
            goto failed;
		 }
#else
			goto param_error;
#endif

	out_ok:
		*rsp_cmd = gen_at_ok();
	    if (param.value != NULL)
		    xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
		return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
	    goto failed;	
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
	    *rsp_cmd = gen_at_cis_error(errorcode);
		if (param.value != NULL)
			xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
		return  AT_END;
	}	
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(140);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 140, "\r\n+MIPLREADRSP: (0),(0-0xFFFFFFF),(1,11-15),(0-0xFFFF),(0-0xFFFF),(0-0xFFFF),(1-5),(1-0xFF),<value>,(0-10),(0-2),(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 140, "\r\n+MIPLREADRSP: <ref>,<msgID>,<result>[,<objID>,<insID>,<resID>,<value_type>,<len>,<value>,<index>,<flag>[,<raimode>]]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}

	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLWRITERSP=<ref>,<msgid>,<result>[,<raimode>]
 Description : notify the basic communication suite of message results written
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLWRITERSP=<ref>,<msgid>,<result>[,<raimode>]
 *****************************************************************************/
int at_proc_miplwrite_req(char *at_buf, char **rsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		struct onenet_write_exe param = {0};
		char raiType[8] = {0};	
		void *params[]  = {&param.ref, &param.msg_id, &param.result, raiType};
	    unsigned int errorcode = CIS_PARAM_ERROR;
	    int ret= CIS_RET_ERROR;

		if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
			goto failed;
		}
#if VER_QUCTL260
		if(at_parse_param("%d,%d,%d,%1d", at_buf, &param.ref, &param.msg_id, &param.result, &param.raiflag) != AT_OK
				|| param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id < 0 || param.raiflag < 0 || param.raiflag > 2)
			goto param_error;
#else
		if (at_parse_param_2("%d,%d,%d,%8s", at_buf, params) != AT_OK) {
			goto param_error;
		}

		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id < 0)
			goto param_error;

		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif
		if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
		}
		
		if (0 != check_coap_result_code(param.result, RSP_WRITE))
			goto param_error;

		if(onet_check_reqType(CALLBACK_TYPE_WRITE, param.msg_id) != 0)
			goto param_error;

	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
	    osMutexRelease(g_onenet_mutex);
		if (ret != CIS_RET_OK)
#if VER_QUCTL260
		 {
        	errorcode = CIS_SDK_ERROR;
            goto failed;
		 }
#else
			goto param_error;
#endif

		*rsp_cmd = gen_at_ok();
		return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
		goto failed;
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
	    *rsp_cmd = gen_at_cis_error(errorcode);
		return  AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(70);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 70, "\r\n+MIPLWRITERSP: (0),(0-0xFFFFFFF),(2,11-14),(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 70, "\r\n+MIPLWRITERSP: <ref>,<msgID>,<result>[,<raimode>]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}

	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLEXECUTERSP=<ref>,<msgid>,<result>[,<raimode>]
 Description : after receiving the message such as '+MIPLEXECUTE', user needs
 	 	 	   to execute the requested action and notify the basic communication
 	 	 	   suite of execution result
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLEXECUTERSP=<ref>,<msgid>,<result>[,<raimode>]
 *****************************************************************************/
int at_proc_miplexecute_req(char *at_buf, char **rsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		struct onenet_write_exe param = {0};
		char raiType[8] = {0};	
		void *params[]  = {&param.ref, &param.msg_id, &param.result, raiType};
	    unsigned int errorcode = CIS_PARAM_ERROR;
	    int ret= CIS_RET_ERROR;

		if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
			goto failed;
		}
#if VER_QUCTL260
		if(at_parse_param("%d,%d,%d,%1d", at_buf, &param.ref, &param.msg_id, &param.result, &param.raiflag) != AT_OK
				|| param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id < 0 || param.raiflag < 0 || param.raiflag > 2)
			goto param_error;
#else
		if (at_parse_param_2("%d,%d,%d,%8s", at_buf, params) != AT_OK) {
			goto param_error;
		}
		
		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id < 0)
			goto param_error;

		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif
		if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
		}
		
		if (0 != check_coap_result_code(param.result, RSP_EXECUTE))
			goto param_error;

		if(onet_check_reqType(CALLBACK_TYPE_EXECUTE, param.msg_id) != 0)
			goto param_error;

	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
	    osMutexRelease(g_onenet_mutex);
		if (ret != CIS_RET_OK)
#if VER_QUCTL260
		 {
        	errorcode = CIS_SDK_ERROR;
            goto failed;
		 }
#else
			goto param_error;
#endif

		*rsp_cmd = gen_at_ok();
		return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
	    goto failed;
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
	    *rsp_cmd = gen_at_cis_error(errorcode);
		return  AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(70);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 70, "\r\n+MIPLEXECUTERSP: (0),(0-0xFFFFFFF),(2,11-14),(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 70, "\r\n+MIPLEXECUTERSP: <ref>,<msgID>,<result>[,<raimode>]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}

	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLEXECUTERSP=<ref>,<msgid>,<result>[,<raimode>]
 Description : notify the basic communication suite whether the observation is valid
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLOBSERVERSP=<ref>,<msgid>,<result>>[,<raimode>]
 *****************************************************************************/
int at_proc_miplobserve_req(char *at_buf, char **rsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		struct onenet_write_exe param = {0};
		char raiType[8] = {0};	

		void *params[]  = {&param.ref, &param.msg_id, &param.result, raiType};
	    unsigned int errorcode = CIS_PARAM_ERROR;
	    int ret= CIS_RET_ERROR;
	    cis_mid_t temp_observeMid;

		if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
			goto failed;
		}
#if VER_QUCTL260
		if(at_parse_param("%d,%d,%d,%1d", at_buf, &param.ref, &param.msg_id, &param.result, &param.raiflag) != AT_OK
				|| param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id < 0 || param.raiflag < 0 || param.raiflag > 2)
			goto param_error;
#else
		if (at_parse_param_2("%d,%d,%d,%8s", at_buf, params) != AT_OK) {
			goto param_error;
		}

		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id < 0)
			goto param_error;

		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif
		if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
		}
		
		if (0 != check_coap_result_code(param.result, RSP_OBSERVE))
			goto param_error;

		if(onet_check_reqType(CALLBACK_TYPE_OBSERVE, param.msg_id) != 0
			&& onet_check_reqType(CALLBACK_TYPE_OBSERVE_CANCEL, param.msg_id) != 0)
			goto param_error;


	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    if (observe_findByMsgid(onenet_context_refs[param.ref].onenet_context, param.msg_id) == NULL) {
	        if (!packet_asynFindObserveRequest(onenet_context_refs[param.ref].onenet_context, param.msg_id, &temp_observeMid)) {
	            osMutexRelease(g_onenet_mutex);
	            goto param_error;
	        }
	    }

	    ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
	    osMutexRelease(g_onenet_mutex);
		if (ret != CIS_RET_OK)
#if VER_QUCTL260
		 {
        	errorcode = CIS_SDK_ERROR;
            goto failed;
		 }
#else
			goto param_error;
#endif

		*rsp_cmd = gen_at_ok();
		return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
	    goto failed;
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
	    *rsp_cmd = gen_at_cis_error(errorcode);
		return  AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(70);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 70, "\r\n+MIPLOBSERVERSP: (0),(0-0xFFFFFFF),(1,11-15),(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 70, "\r\n+MIPLOBSERVERSP: <ref>,<msgID>,<result>[,<raimode>]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}
	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLVER?\r\n     +MIPLVER:2.3.0
 Description : get basic communication suite version
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLVER?
 *****************************************************************************/
int at_proc_miplver_req(char *at_buf, char **rsp_cmd)
{
	(void) at_buf;

	if(g_req_type==AT_CMD_QUERY)
	{
		cis_version_t ver;
		*rsp_cmd = xy_zalloc(32);
		cis_version(&ver);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 32, "\r\n+MIPLVER: %x.%x.%x\r\n%s", ver.major, ver.minor, ver.micro, AT_RSP_OK);
#else
		snprintf(*rsp_cmd, 32, "\r\n+MIPLVER:%x.%x.%x\r\n%s", ver.major, ver.minor, ver.micro, AT_RSP_OK);
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}

	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLDISCOVERRSP=<ref>,<msgid>,<result>,<length>,<valuestring>[,<raimode>]
 Description : set up all the attributes of the specified object of basic communication suite
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLDISCOVERRSP=<ref>,<msgid>,<result>,<length>,<valuestring>[,<raimode>]
 *****************************************************************************/
int at_proc_discover_req(char *at_buf, char **rsp_cmd)
{
	if(g_req_type==AT_CMD_REQ)
	{
	    struct onenet_discover_rsp param = { 0 };
		char *temp_value= xy_zalloc(strlen(at_buf));
		char raiType[8] = {0};	
	    void *params[]  = {&param.ref, &param.msgId, &param.result, &param.length,temp_value, raiType};
		unsigned int errorcode = CIS_PARAM_ERROR;
	    int ret= CIS_RET_ERROR;

		if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
			goto failed;
		}
#if VER_QUCTL260
		if(at_parse_param("%d,%d,%d,%d,%s,%1d", at_buf, &param.ref, &param.msgId, &param.result, &param.length, temp_value, &param.raiflag) != AT_OK
				|| param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msgId < 0 || param.raiflag < 0 || param.raiflag > 2)
			goto param_error;

		if(param.length > 255 || param.length!=strlen(temp_value))
			goto param_error;
#else
	    if (at_parse_param_2("%d,%d,%d,%d,%s,%8s", at_buf, params) != AT_OK) {
			goto param_error;
		}
		
		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msgId < 0)
			goto param_error;

		if(param.length!=strlen(temp_value))
			goto param_error;

		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif

		if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
		}
		
		if (0 != check_coap_result_code(param.result, RSP_DISCOVER))
			goto param_error;
		
	    if (param.length <= STR_VALUE_LEN) {
			param.value = xy_zalloc(param.length + 1);
			params[4] = param.value;
	    } else {
			goto param_error;
	    }
	    if (at_parse_param_2("%d,%d,%d,%d,%s", at_buf, params) != AT_OK) {
			goto param_error;
		}

		if(onet_check_reqType(CALLBACK_TYPE_DISCOVER, param.msgId) != 0)
			goto param_error;
		
	    if(param.result == 1 && param.length > 0) {
	    	char *buft = param.value;
			char *lastp = NULL;
			char *strres = NULL;
	        while ((strres = strtok_r(buft, ";", &lastp)) != NULL) {
				cis_uri_t uri = {0};
				uri.objectId = URI_INVALID;
	            uri.instanceId = URI_INVALID;
	            uri.resourceId = (int)strtol(strres,NULL,10);
	            cis_uri_update(&uri);
	            osMutexAcquire(g_onenet_mutex, osWaitForever);
	            cis_response(onenet_context_refs[param.ref].onenet_context, &uri, NULL, param.msgId, CIS_RESPONSE_CONTINUE,param.raiflag);
	            osMutexRelease(g_onenet_mutex);
				buft = NULL;
			}
	    }

	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msgId, get_coap_result_code(param.result),param.raiflag);
	    osMutexRelease(g_onenet_mutex);
		if (ret != CIS_RET_OK){
#if VER_QUCTL260
        	errorcode = CIS_SDK_ERROR;
            goto failed;
#else
			goto param_error;
#endif
	    }
	    if (param.value != NULL)
		    xy_free(param.value);
		if (temp_value != NULL)
			xy_free(temp_value);
	    *rsp_cmd = gen_at_ok();
	    return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
	    goto failed;
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
		if (param.value != NULL)
			xy_free(param.value);
		if (temp_value != NULL)
			xy_free(temp_value);
	    *rsp_cmd = gen_at_cis_error(errorcode);
		return  AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(100);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 100, "\r\n+MIPLDISCOVERRSP: (0),(0-0xFFFFFFF),(1,11-15),(1-0xFF),<valuestring>,(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 100, "\r\n+MIPLDISCOVERRSP: <ref>,<msgID>,<result>[,<length>,<value_string>[,<raimode>]]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}
	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLPARAMETERRSP=<ref>,<msgid>,<result>[,<raimode>]
 Description : after receiving the message such as '+MIPLPARAMETER', user needs
 	 	 	   to execute the requested action and notify the basic communication
 	 	 	   suite of execution result
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLPARAMETERRSP=<ref>,<msgid>,<result>[,<raimode>]
 *****************************************************************************/
int at_proc_setparam_req(char *at_buf, char **rsp_cmd)
{
	if(g_req_type==AT_CMD_REQ)
	{
	    struct onenet_parameter_rsp param = { 0 };
		char raiType[8] = {0};	
		void *params[]  = {&param.ref, &param.msg_id, &param.result, raiType};
	    unsigned int errorcode = CIS_PARAM_ERROR;
	    int ret= CIS_RET_ERROR;

		if(!ps_netif_is_ok()) {
#if VER_QUCTL260
            errorcode = CIS_UNKNOWN_ERROR;
#else
			errorcode = ATERR_NOT_NET_CONNECT;
#endif
			goto failed;
		}

#if VER_QUCTL260
		if(at_parse_param("%d,%d,%d,%1d", at_buf, &param.ref, &param.msg_id, &param.result, &param.raiflag) != AT_OK
				|| param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id < 0 || param.raiflag < 0 || param.raiflag > 2)
			goto param_error;
#else
	    if (at_parse_param_2("%d,%d,%d,%8s", at_buf, params) != AT_OK) {
			goto param_error;
		}
		
		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.msg_id <=0 )
			goto param_error;

		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif
		
		if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
		}

		if (0 != check_coap_result_code(param.result, RSP_SETPARAMS))
			goto param_error;

		if(onet_check_reqType(CALLBACK_TYPE_OBSERVE_PARAMS, param.msg_id) != 0)
			goto param_error;

	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
	    osMutexRelease(g_onenet_mutex);
		if (ret != CIS_RET_OK) {
#if VER_QUCTL260
        	errorcode = CIS_SDK_ERROR;
            goto failed;
#else
			goto param_error;
#endif
	    }
		*rsp_cmd = gen_at_ok();
		return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
	    goto failed;
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
	    *rsp_cmd = gen_at_cis_error(errorcode);
		return  AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(80);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 80, "\r\n+MIPLPARAMETERRSP: (0),(0-0xFFFFFFF),(2,11-14),(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 80, "\r\n+MIPLPARAMETERRSP: <ref>,<msgID>,<result>[,<raimode>]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}
	return  AT_END;
}

/*****************************************************************************
 Function    : +MIPLPARAMETERRSP=<ref>,<msgid>,<result>[,<raimode>]
 Description : notify the basic communication suite to send active update registration information
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+MIPLUPDATE=<ref>,<lifetime>,<withObjectFlag>[,<raimode>]
 *****************************************************************************/
int at_proc_update_req(char *at_buf, char **rsp_cmd)
{
	if(g_req_type==AT_CMD_REQ)
	{
	    struct onenet_update param = { 0 };
		param.ref = -1;
		param.lifetime = -1;
		param.withObjFlag = -1;		
		char raiType[8] = {0};	
		void *params[]  = {&param.ref, &param.lifetime, &param.withObjFlag, raiType};
	    unsigned int errorcode = CIS_PARAM_ERROR;
	    int ret= CIS_RET_ERROR;

		if(!ps_netif_is_ok()) {
			goto net_error;
		}
		
        resume_net_app(ONENET_TASK);

#if VER_QUCTL260
		if(at_parse_param("%d,%d,%d,%1d", at_buf, &param.ref, &param.lifetime, &param.withObjFlag, &param.raiflag) != AT_OK
				|| param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || param.raiflag < 0 || param.raiflag > 2
				|| (param.withObjFlag!=0&&param.withObjFlag!=1))
			goto param_error;
#else
	    if (at_parse_param_2("%d,%d,%d,%8s", at_buf, params) != AT_OK) {
			goto param_error;
		}

		if(param.ref < 0 || param.ref >= CIS_REF_MAX_NUM || (param.withObjFlag!=0&&param.withObjFlag!=1))
			goto param_error;

		if(!strcmp(raiType, "0X200") || !strcmp(raiType, "0x200"))
			goto param_error;	

		if(onet_set_rai(raiType,&param.raiflag) != 0)
			goto param_error;	
#endif

		if (!is_onenet_task_running(param.ref)) {
#if VER_QUCTL260
            goto status_error;
#else
			goto param_error;
#endif
		}

	    if (param.lifetime == 0) {
	        param.lifetime = DEFAULT_LIFETIME;
	    } else if (param.lifetime < LIFETIME_LIMIT_MIN || param.lifetime > LIFETIME_LIMIT_MAX) {
	        goto param_error;
	    }

	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = cis_update_reg(onenet_context_refs[param.ref].onenet_context, param.lifetime, param.withObjFlag ,param.raiflag);
	    osMutexRelease(g_onenet_mutex);

	    if (ret != COAP_NO_ERROR) {
#if VER_QUCTL260
        	errorcode = CIS_SDK_ERROR;
            goto failed;
#else
			goto param_error;
#endif
	    }

		*rsp_cmd = gen_at_ok();
		return AT_END;
	param_error:
	    errorcode = CIS_PARAM_ERROR;
	    goto failed;
	net_error:
#if VER_QUCTL260
        errorcode = CIS_UNKNOWN_ERROR;
#else
		errorcode = ATERR_NOT_NET_CONNECT;
#endif
		goto failed;	
	status_error:
	    errorcode = CIS_STATUS_ERROR;
	failed:
	    *rsp_cmd = gen_at_cis_error(errorcode);
		return AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_zalloc(80);
#if VER_QUCTL260
		snprintf(*rsp_cmd, 80, "\r\n+MIPLUPDATE: (0),(0x0-0x0FFFFFFF),(0-1),(0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*rsp_cmd, 80, "\r\n+MIPLUPDATE: <ref>,<lifetime>,<with_object_flag>[,<raimode>]\r\n\r\nOK\r\n");
#endif
	}
	else
	{
		*rsp_cmd = gen_at_cis_error(CIS_PARAM_ERROR);
	}

	return AT_END;
}

/*****************************************************************************
 Function    : at_proc_rmlft_req
 Description : Used for get onnet remain lifetime
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+ONETRMLFT
               +ONETRMLFT:86387
               OK
 *****************************************************************************/
int at_proc_rmlft_req(char *at_buf, char **prsp_cmd)
{
    int app_type = 0;
    int remain_lifetime = -1;
	int tmp_time[2] = {0};
	int reg_last_update_time = 0;
	int reg_life_time = 0;

    (void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
#if VER_QUCTL260
        *prsp_cmd = AT_ERR_BUILD(CIS_PARAM_ERROR);
#else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
#endif
        return AT_END;
    }

    if (onenet_context_refs[0].onenet_context != NULL && onenet_context_refs[0].onenet_context->stateStep != PUMP_STATE_READY)
        goto error;

    if (onenet_context_refs[0].onet_at_thread_id == NULL)
    {
        if(!NET_NEED_RECOVERY(ONENET_TASK))
            goto error;

		if(xy_ftl_read(ONENET_BACKUP_ADDR + OFFSET_NETINFO_PARAM(onenet_regInfo.last_update_time), (unsigned char *)tmp_time, sizeof(tmp_time))!= XY_OK)
	    {
	        goto error;
	    }
		reg_last_update_time = tmp_time[0];
		reg_life_time = tmp_time[1];

        remain_lifetime = reg_last_update_time + reg_life_time - cloud_gettime_s();
    }
    else if (onenet_context_refs[0].onenet_context != NULL && onenet_context_refs[0].onenet_context->server != NULL)
    {
        remain_lifetime = onenet_context_refs[0].onenet_context->server->registration +
            onenet_context_refs[0].onenet_context->lifetime - cloud_gettime_s();
    }
    else
        goto error;
    
    *prsp_cmd = xy_zalloc(40);
    if (remain_lifetime >= 0)
    {
        sprintf(*prsp_cmd,"\r\n+ONETRMLFT:%d\r\n", remain_lifetime);        
    }
    else
    {
        sprintf(*prsp_cmd,"\r\n+ONETRMLFT:error\r\n");
    }
    strcat(*prsp_cmd, "\r\nOK\r\n");

	return AT_END;

error:
#if VER_QUCTL260
    *prsp_cmd = AT_ERR_BUILD(CIS_STATUS_ERROR);
#else
	*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#endif
    return AT_END;
}

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
void cis_netif_event_callback(PsNetifStateChangeEvent event)
{
	osThreadAttr_t thread_attr = {0};

    softap_printf(USER_LOG, WARN_LOG, "onenet_netif_event_callback, netif up");
//    cis_net_attached = 1;

    //保活模式，IP变化触发update
    if (g_onenet_resume_TskHandle == NULL && g_softap_fac_nv->save_cloud && (NET_NEED_RECOVERY(ONENET_TASK) || is_onenet_task_running(0)))
    {
        thread_attr.name	   = "onenet_netif_up_resume";
        thread_attr.priority   = XY_OS_PRIO_NORMAL1;
        thread_attr.stack_size = 0x400;
        g_onenet_resume_TskHandle = osThreadNew((osThreadFunc_t)(onenet_netif_up_resume_process), NULL, &thread_attr);
    }
}

static uint16_t  s_cislwm2m_inited = 0;
//suds
void at_onenet_init(void)
{
    int i;
#if VER_QUCTL260
	char* cis_user_config = NULL;
	int cfg_len = 0;
#endif	
	
	if(!s_cislwm2m_inited)
	{
		for (i = 0; i < CIS_REF_MAX_NUM; i++)
		{
			memset(&(onenet_context_refs[i]), 0, sizeof(onenet_context_reference_t));
			onenet_context_refs[i].ref = i;
			onenet_context_refs[i].onet_at_thread_id = NULL;
		}
		memset(onenet_context_configs, 0, sizeof(onenet_context_config_t) * CIS_REF_MAX_NUM);

#if VER_QUCTL260
        if(g_softap_fac_nv->onenet_config_len == 0)
		{
    		cis_user_config = cis_cfg_tool("183.230.40.39", 5683, 1, NULL, 0, NULL, &cfg_len);
    		g_softap_fac_nv->onenet_config_len = cfg_len;
            memset(g_softap_fac_nv->onenet_config_hex, 0, sizeof(g_softap_fac_nv->onenet_config_hex));
            memcpy(g_softap_fac_nv->onenet_config_hex, cis_user_config, cfg_len);
    		if(cis_user_config)
                xy_free(cis_user_config);
        }
		else {
			if(g_softap_fac_nv->ack_timeout > 0 && g_softap_fac_nv->ack_timeout != g_ONENET_ACK_TIMEOUT)
				g_ONENET_ACK_TIMEOUT = g_softap_fac_nv->ack_timeout;
		}
#else
		if(g_softap_fac_nv->ack_timeout > 0 && g_softap_fac_nv->ack_timeout != g_ONENET_ACK_TIMEOUT)
			g_ONENET_ACK_TIMEOUT = g_softap_fac_nv->ack_timeout;
#endif
		//onenet_out_fd = FAR_PS_FD;
		cloud_mutex_create(&g_onenet_mutex);

		xy_regist_psnetif_callback(EVENT_PSNETIF_VALID, cis_netif_event_callback);
		s_cislwm2m_inited = 1;
	}
}

#endif
