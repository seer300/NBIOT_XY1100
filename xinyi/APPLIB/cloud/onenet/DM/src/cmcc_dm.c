#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include <cis_def.h>
#include <cis_api.h>
#include <cis_if_sys.h>
#include <cis_def.h>
#include <cis_api.h>
#include "cis_internals.h"
#include "rtc_tmr.h"
#include "factory_nv.h"
#include "at_onenet.h"
#include "onenet_backup_proc.h"
#include "net_app_resume.h"
#include "xy_system.h"
#include "softap_nv.h"
#include "at_ps_cmd.h"

#define DEFAULT_DM_LIFETIME (24*60*60) // seconds
osSemaphoreId_t cmccdm_sem = NULL;
extern osSemaphoreId_t cis_poll_sem;
extern osMutexId_t g_onenet_mutex;
osThreadId_t g_cmcc_dm_TskHandle = NULL;
osThreadId_t g_cmcc_redm_TskHandle = NULL;
osThreadId_t g_cmcc_dmupdate_TskHandle = NULL;
osSemaphoreId_t g_cmccdm_update_sem = NULL;
static int isResumed_flag = 0;
unsigned int g_cmcc_dm_rtcflag = 0;

//static const Options default_dm_config = {" ", " ", "v1.0", "M100000329", "43648As94o1K8Otya74T2719D51cmy58", 4, ""};
static DMOptions default_dm_config = {" ", " ", "v2.0", "M100000648", "x0g5g1q2LDm252162y2fZhgK8Cu8QZ7l", 4, "", 
"XY1100"};

static const uint8_t default_dm_config_hex[] = {
    0x13, 0x00, 0x43,
    0xf1, 0x00, 0x03,
    0xf2, 0x00, 0x35,
        0x04, 0x00 /*mtu*/, 0x11 /*Link & bind type*/, 0x00 /*BS DTLS ENABLED*/,
        0x00, 0x05 /*apn length*/, 0x43, 0x4d, 0x49, 0x4f, 0x54 /*apn: CMIOT*/,
        0x00, 0x00 /*username length*/, /*username*/
        0x00, 0x00 /*password length*/, /*password*/
        0x00, 0x10 /*host length*/, 0x31, 0x31, 0x37, 0x2e, 0x31, 0x36, 0x31, 0x2e,
    0x32, 0x2e, 0x37, 0x3a, 0x35, 0x36, 0x38, 0x33 /*host: 117.161.2.7:5683*/,
      0x00, 0x0f /*userdata length*/, 0x41, 0x75, 0x74, 0x68, 0x43, 0x6f, 0x64, 0x65, 
      0x3a, 0x3b, 0x50, 0x53, 0x4b, 0x3a, 0x3b /*userdata: AuthCode:;PSK:;*/,
    0xf3, 0x00, 0x08,
        0xe4 /*log config*/, 0x00, 0xc8 /*LogBufferSize: 200*/,
        0x00, 0x00 /*userdata length*//*userdata*/
};
st_context_t *g_dm_context = NULL;

int cmcc_dm_register(st_context_t *dm_context, int lifetime);

cis_coapret_t dm_onRead_cb(void* context,cis_uri_t* uri,cis_mid_t mid)
{
	(void) context;
	(void) uri;
	(void) mid;

    return CIS_CALLBACK_CONFORM;
}

cis_coapret_t dm_onWrite_cb(void* context,cis_uri_t* uri,const cis_data_t* value,cis_attrcount_t attrcount,cis_mid_t mid)
{
	(void) context;
	(void) uri;
	(void) value;
	(void) attrcount;
	(void) mid;

    return CIS_CALLBACK_CONFORM;
}

cis_coapret_t dm_onExec_cb(void* context,cis_uri_t* uri,const uint8_t* buffer,uint32_t length,cis_mid_t mid)
{
	(void) context;
	(void) uri;
	(void) buffer;
	(void) length;
	(void) mid;

    return CIS_CALLBACK_CONFORM;
}

cis_coapret_t dm_onObserve_cb(void* context,cis_uri_t* uri,bool flag,cis_mid_t mid)
{
	(void) context;
	(void) uri;
	(void) flag;
	(void) mid;

    return CIS_CALLBACK_CONFORM;
}

cis_coapret_t dm_onDiscover_cb(void* context,cis_uri_t* uri,cis_mid_t mid)
{
	(void) context;
	(void) uri;
	(void) mid;

    return CIS_CALLBACK_CONFORM;
}

cis_coapret_t dm_onSetParams_cb(void* context,cis_uri_t* uri,cis_observe_attr_t parameters,cis_mid_t mid)
{
	(void) context;
	(void) uri;
	(void) parameters;
	(void) mid;

    return CIS_CALLBACK_CONFORM;
}


static void cmccdm_rtc_timeout_cb(void *para)
{
	(void) para;
	xy_printf("[CMCCDM] Just user RTC callback\n");
	//RTC唤醒后，协议栈可能还处于PSM态，不加锁，业务代码若有delay，则会进入idle，进而可能进入深睡
    xy_work_lock(LOCK_XY_LOCAL);
	g_cmcc_dm_rtcflag = 1;
	cmcc_dm_init();
}

void dm_onEvent_cb(void* context, cis_evt_t id, void* param)
{
	unsigned int lifetimer = 0;

	(void) param;

    switch(id)
    {
        case CIS_EVENT_REG_SUCCESS:
        case CIS_EVENT_UPDATE_SUCCESS:			
        {
        	xy_printf("cmcc dm event(%d):register/update successed", id);
            if(g_softap_fac_nv->dm_inteval_time == 0)
				lifetimer = DEFAULT_DM_LIFETIME;
			else
				lifetimer = g_softap_fac_nv->dm_inteval_time*60;

            //DM update完成，释放锁进入深睡
            if(g_cmcc_dm_rtcflag == 1 )
            {
                xy_work_unlock(LOCK_XY_LOCAL);
				g_cmcc_dm_rtcflag = 0;
            }

            xy_rtc_timer_create(RTC_TIMER_CMCCDM, lifetimer-3, cmccdm_rtc_timeout_cb, NULL);
            break;
        }
        case CIS_EVENT_UPDATE_NEED:
        {
            //cis_update_reg(context, LIFETIME_INVALID, 0);
            break;
        }
        case CIS_EVENT_UNREG_DONE:
        {
            //DM update完成，释放锁进入深睡
            if(g_cmcc_dm_rtcflag == 1 )
            {
                xy_work_unlock(LOCK_XY_LOCAL);
				g_cmcc_dm_rtcflag = 0;
            }
            break;
        }
        default:
        {
            xy_printf("cmcc dm unhandled event: %d", id);
            break;
        }
    }
}

extern onenet_regInfo_t *g_onenet_regInfo;
static void cmccdm_resume_context(st_context_t* ctx)
{
    int i;
    st_observed_t *observed;
    onenet_observed_t *backup_observed;
    st_object_t * objectP;
    onenet_object_t *backup_object;
    cis_inst_bitmap_t bitmap = {0};
    cis_res_count_t  rescount;
    st_object_t * securityObjP = NULL;
    cis_list_t * securityInstP;
    security_instance_dup_t * targetP;
    st_data_t * dataP;
    //st_context_t* ctx = onenet_context_refs[0].onenet_context;

    //osMutexAcquire(g_onenet_mutex, osWaitForever);

    ctx->lifetime = g_onenet_regInfo->life_time;
    ctx->isDM = TRUE;
	
	//ctx->registerEnabled = true;
	//cis_memcpy(&ctx->callback, &callback, sizeof(cis_callback_t));
    ctx->callback.onRead = dm_onRead_cb;
	ctx->callback.onWrite = dm_onWrite_cb;
	ctx->callback.onExec = dm_onExec_cb;
	ctx->callback.onObserve = dm_onObserve_cb;
    ctx->callback.onDiscover = dm_onDiscover_cb;
	ctx->callback.onSetParams = dm_onSetParams_cb;
	ctx->callback.onEvent = dm_onEvent_cb;

	xy_printf("[CMCCDM]resume dm context, obj count(%d)", g_onenet_regInfo->object_count);
    for (i = 0; i <g_onenet_regInfo->object_count; i++)
    {
        backup_object = &g_onenet_regInfo->onenet_object[i];
    	
    	//make bitmap;
    	bitmap.instanceCount = backup_object->instValidCount;
    	bitmap.instanceBitmap = backup_object->instBitmapPtr;
    	bitmap.instanceBytes = backup_object->instBitmapBytes;

    	rescount.attrCount = backup_object->attributeCount;
        rescount.actCount = backup_object->actionCount;

    	cis_addobject(ctx, backup_object->objID, &bitmap, &rescount);
    }

    securityInstP = (cis_list_t *)ctx->instSecurity;
    while (securityInstP != NULL)
    {
        targetP = (security_instance_dup_t *)securityInstP;
        if (targetP->instanceId == g_onenet_regInfo->onenet_security_instance.instanceId)
        {
            if (targetP->host != NULL) cis_free(targetP->host);
            targetP->host = (char *)cis_malloc(strlen(g_onenet_regInfo->onenet_security_instance.host) + 1);
            strcpy(targetP->host, g_onenet_regInfo->onenet_security_instance.host);
#if CIS_ENABLE_DTLS
            // todo
#endif
            targetP->isBootstrap = g_onenet_regInfo->onenet_security_instance.isBootstrap;
            targetP->shortID = g_onenet_regInfo->onenet_security_instance.shortID;
            targetP->clientHoldOffTime = g_onenet_regInfo->onenet_security_instance.clientHoldOffTime;
            targetP->securityMode = g_onenet_regInfo->onenet_security_instance.securityMode;
#if CIS_ENABLE_DTLS
#if CIS_ENABLE_PSK
            // todo
#endif
#endif
            break;
        }
        securityInstP = securityInstP->next;
    }

    //ctx->lasttime = utils_gettime_s();
    ctx->server = management_makeServerList(ctx, false);
    management_createNetwork(ctx, ctx->server);
    management_connectServer(ctx, ctx->server);
    ctx->server->registration = g_onenet_regInfo->last_update_time;
    ctx->server->status = STATE_REGISTERED;
    ctx->server->location = xy_zalloc(strlen((const char *)g_onenet_regInfo->location) + 1);
    strcpy(ctx->server->location, (const char *)g_onenet_regInfo->location);
    ctx->nextMid = g_softap_var_nv->onenetNextMid;

	xy_printf("[CMCCDM]resume dm context, obsv count(%d)", g_onenet_regInfo->observed_count);
    for (i = 0; i <g_onenet_regInfo->observed_count; i++)
    {
        backup_observed = &g_onenet_regInfo->observed[i];
        observed = prv_getObserved(ctx, &backup_observed->uri);

        observed->actived = true;
        observed->msgid = backup_observed->msgid;
        observed->tokenLen = backup_observed->token_len;
        cis_memcpy(observed->token, backup_observed->token, backup_observed->token_len);
        observed->lastTime = backup_observed->last_time;
        observed->counter = backup_observed->counter;
        observed->lastMid = backup_observed->lastMid;
        observed->format = backup_observed->format;
    }

    core_updatePumpState(ctx, PUMP_STATE_READY);
    ctx->registerEnabled = true;


    //osMutexRelease(g_onenet_mutex);
}



int cmcc_dm_register(st_context_t *dm_context, int lifetime)
{
    cis_callback_t callback;
	callback.onRead = dm_onRead_cb;
	callback.onWrite = dm_onWrite_cb;
	callback.onExec = dm_onExec_cb;
	callback.onObserve = dm_onObserve_cb;
    callback.onDiscover = dm_onDiscover_cb;
	callback.onSetParams = dm_onSetParams_cb;
	callback.onEvent = dm_onEvent_cb;

    return cis_register(dm_context, lifetime, &callback);
}

void cmcc_dm_run(void)
{
    int need_reg= 0;
    uint32_t current_sec;
    st_context_t *dm_context = NULL;

    //除首次上电和CMCC RTC之外的唤醒，验证恢复标志位，无恢复标志位则重新注册
    if(get_sys_up_stat() != POWER_ON && g_cmcc_dm_rtcflag == 0)
    {
        if(g_onenet_regInfo == NULL && !NET_NEED_RECOVERY(ONENET_TASK))
            need_reg = 1;
        else
        {
            g_cmcc_dm_TskHandle = NULL;
            osThreadExit();
        }
    }
    else if(g_onenet_regInfo == NULL && NET_NEED_RECOVERY(ONENET_TASK))
    {
        onenet_read_regInfo(OFFSET_NETINFO_PARAM(onenet_regInfo));
        g_softap_var_nv->app_recovery_flag&=(~(1<<ONENET_TASK));
    }

    if(xy_wait_tcpip_ok(2*60) == XY_ERR)
    {
        if(g_cmcc_dm_rtcflag == 1 )
        {
            xy_work_unlock(LOCK_XY_LOCAL);
        }
        g_cmcc_dm_TskHandle = NULL;
        osThreadExit();
    }
	extern int	g_uicc_type;
	if(g_uicc_type == UICC_UNKNOWN)
	{
		xy_get_UICC_TYPE(&g_uicc_type);
	}
	
	if(g_uicc_type != UICC_MOBILE)
	{
		if(g_cmcc_dm_rtcflag == 1 )
	    {
	        xy_work_unlock(LOCK_XY_LOCAL);
	    }
        g_cmcc_dm_TskHandle = NULL;
	    osThreadExit();
	}

    if (cis_init((void **)&dm_context, (void *)default_dm_config_hex, sizeof(default_dm_config_hex), (void*)&default_dm_config) != CIS_RET_OK)
    {
        if(g_cmcc_dm_rtcflag == 1 )
        {
            xy_work_unlock(LOCK_XY_LOCAL);
        }
        g_cmcc_dm_TskHandle = NULL;
        osThreadExit();
    }

	g_dm_context = dm_context;

	if(g_cmcc_dm_rtcflag == 1 && need_to_resume_onenet())
	{
		cmccdm_resume_context(dm_context);
	}
	else
	{
		if(g_softap_fac_nv->dm_inteval_time == 0)
			cmcc_dm_register(dm_context, DEFAULT_DM_LIFETIME);
		else	
		    cmcc_dm_register(dm_context, g_softap_fac_nv->dm_inteval_time*60);		
	}

    while (dm_context != NULL && dm_context->registerEnabled == TRUE)
    {
		if(cis_pump(dm_context) == PUMP_RET_CUSTOM) {
		    osDelay(1000);
        } else {
        	osDelay(10);
        }
	}
    g_cmcc_dm_TskHandle = NULL;
    osThreadExit();
}

void cmcc_dm_init()
{
	osThreadAttr_t thread_attr = {0};
	if (g_cmcc_dm_TskHandle == NULL)
	{
		softap_printf(USER_LOG, WARN_LOG,"start cmcc dm task!!!");
		thread_attr.name	   = "cmcc_dm";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = 0x1000;
		g_cmcc_dm_TskHandle = osThreadNew((osThreadFunc_t)(cmcc_dm_run), NULL, &thread_attr);
	}
}

#endif



