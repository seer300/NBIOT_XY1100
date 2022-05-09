#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "softap_nv.h"
#include "onenet_backup_proc.h"
#include "flash_adapt.h"
#include "xy_flash.h"
#include "xy_system.h"
#include "low_power.h"
#include "factory_nv.h"
#include "net_app_resume.h"
#include "xy_proxy.h"

unsigned int  g_cis_keepalive_update = 0;
onenet_regInfo_t *g_onenet_regInfo = NULL;
extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];
extern osMutexId_t g_onenet_mutex;
extern osMutexId_t g_cis_keepalive_update_mutex;
extern st_context_t *g_dm_context;
extern int g_FOTAing_flag;
extern unsigned short  g_onenet_localPort;
extern unsigned short  g_onenet_remotePort;
extern osSemaphoreId_t cis_recovery_sem;
extern osSemaphoreId_t cis_poll_sem;
extern osThreadId_t onenet_resume_task_id;
osThreadId_t g_onenet_exception_recovery_TskHandle = NULL;
osThreadId_t g_onenet_resume_TskHandle = NULL;
osThreadId_t g_onenet_rtc_resume_TskHandle = NULL;

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
int is_user_onenet_context(st_context_t* context)
{
    int i;
    if (context == 0) 
        return 0;
#if CIS_ENABLE_DM
	if(context->isDM == TRUE)
	{
		return 1;
	}
#endif	
    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (context == onenet_context_refs[i].onenet_context)
        {
            return 1;
        }
    }

    return 0;
}

extern st_observed_t * prv_getObserved(st_context_t * contextP,st_uri_t * uriP);


int lwm2m_common_resume_config()
{
#if LWM2M_COMMON_VER
    if(xy_lwm2m_config == NULL)
        xy_lwm2m_config = xy_zalloc(sizeof(xy_lwm2m_config_t));

    xy_lwm2m_config->bootstrap_flag = g_onenet_regInfo->lwm2mUserConfig.bootstrap_flag;
    if(strlen(g_onenet_regInfo->lwm2mUserConfig.server_host) != 0)
    {
        if(xy_lwm2m_config->server_host == NULL)
            xy_lwm2m_config->server_host = xy_zalloc(strlen(g_onenet_regInfo->lwm2mUserConfig.server_host)+1);
        strcpy(xy_lwm2m_config->server_host,g_onenet_regInfo->lwm2mUserConfig.server_host);
    }
    memcpy(xy_lwm2m_config->server_ip,g_onenet_regInfo->lwm2mUserConfig.server_ip,sizeof(xy_lwm2m_config->server_ip));
    xy_lwm2m_config->port = g_onenet_regInfo->lwm2mUserConfig.port;

    if(strlen(g_onenet_regInfo->lwm2mUserConfig.endpoint_name) != 0)
    {
        if(xy_lwm2m_config->endpoint_name == NULL)
            xy_lwm2m_config->endpoint_name = xy_zalloc(strlen(g_onenet_regInfo->lwm2mUserConfig.endpoint_name)+1);
        strcpy(xy_lwm2m_config->endpoint_name,g_onenet_regInfo->lwm2mUserConfig.endpoint_name);
    }
    xy_lwm2m_config->lifetime = g_onenet_regInfo->lwm2mUserConfig.lifetime;
    xy_lwm2m_config->security_mode = g_onenet_regInfo->lwm2mUserConfig.security_mode;

    if(strlen(g_onenet_regInfo->lwm2mUserConfig.psk_id) != 0)
    {
        if(xy_lwm2m_config->psk_id == NULL)
            xy_lwm2m_config->psk_id = xy_zalloc(strlen(g_onenet_regInfo->lwm2mUserConfig.psk_id)+1);
        strcpy(xy_lwm2m_config->psk_id,g_onenet_regInfo->lwm2mUserConfig.psk_id);
    }

    if(strlen(g_onenet_regInfo->lwm2mUserConfig.psk) != 0)
    {
        if(xy_lwm2m_config->psk == NULL)
            xy_lwm2m_config->psk = xy_zalloc(strlen(g_onenet_regInfo->lwm2mUserConfig.psk)+1);
        strcpy(xy_lwm2m_config->psk,g_onenet_regInfo->lwm2mUserConfig.psk);
    }

    xy_lwm2m_config->binding_mode = g_onenet_regInfo->lwm2mUserConfig.binding_mode;
    xy_lwm2m_config->ack_timeout = g_onenet_regInfo->lwm2mUserConfig.ack_timeout;
    xy_lwm2m_config->retrans_max_times = g_onenet_regInfo->lwm2mUserConfig.retrans_max_times;
    xy_lwm2m_config->is_auto_ack = g_onenet_regInfo->lwm2mUserConfig.is_auto_ack;
    xy_lwm2m_config->access_mode = g_onenet_regInfo->lwm2mUserConfig.access_mode;
    xy_lwm2m_config->access_mode_alternative = g_onenet_regInfo->lwm2mUserConfig.access_mode_alternative;
    xy_lwm2m_config->platform = g_onenet_regInfo->lwm2mUserConfig.platform;

    xy_lwm2m_config->lifetime_enable = g_onenet_regInfo->lwm2mUserConfig.lifetime_enable;
    xy_lwm2m_config->dtls_mode = g_onenet_regInfo->lwm2mUserConfig.dtls_mode;
    xy_lwm2m_config->dtls_version = g_onenet_regInfo->lwm2mUserConfig.dtls_version;
#endif
    return XY_OK;
}

void resume_onenet_context(st_context_t* ctx)
{
    int i;
    st_observed_t *observed;
    onenet_observed_t *backup_observed;
    //st_object_t * objectP;
    onenet_object_t *backup_object;
    cis_inst_bitmap_t bitmap = {0};
    cis_res_count_t  rescount;
    //st_object_t * securityObjP = NULL;
    cis_list_t * securityInstP;
    security_instance_dup_t * targetP;
    //st_data_t * dataP;
    //st_context_t* ctx = onenet_context_refs[0].onenet_context;

    osMutexAcquire(g_onenet_mutex, osWaitForever);

    ctx->cloud_platform = g_onenet_regInfo->cloud_platform;
    ctx->platform_common_type = g_onenet_regInfo->platform_common_type;
    if(g_onenet_regInfo->cloud_platform == CLOUD_PLATFORM_COMMON)
    {
        lwm2m_common_resume_config();
    }
    else
    {
        onenet_resume_user_config();
    }
    ctx->lifetime = g_onenet_regInfo->life_time;
    //ctx->registerEnabled = true;
    //cis_memcpy(&ctx->callback, &callback, sizeof(cis_callback_t));
#if ANDLINK_VER
    if(g_onenet_regInfo->cloud_platform == CLOUD_PLATFORM_ANDLINK)
    {
        ctx->callback.onRead = andlink_on_read_cb;
        ctx->callback.onWrite = andlink_on_write_cb;
        ctx->callback.onExec = andlink_on_execute_cb;
        ctx->callback.onObserve = andlink_on_observe_cb;
        ctx->callback.onDiscover = andlink_on_discover_cb;
        ctx->callback.onSetParams = andlink_on_parameter_cb;
        ctx->callback.onEvent = andlink_on_event_cb;
    }
    else
#endif
    {
        ctx->callback.onRead = onet_on_read_cb;
        ctx->callback.onWrite = onet_on_write_cb;
        ctx->callback.onExec = onet_on_execute_cb;
        ctx->callback.onObserve = onet_on_observe_cb;
        ctx->callback.onDiscover = onet_on_discover_cb;
        ctx->callback.onSetParams = onet_on_parameter_cb;
        ctx->callback.onEvent = onet_on_event_cb;

    }


    /*for (objectP = ctx->objectList; objectP != NULL; objectP = objectP->next)
    {
        if (objectP->objID == CIS_SECURITY_OBJECT_ID)
        {
            securityObjP = objectP;
        }
    }*/

    xy_printf("[CIS]resume cis context, obj count(%d)", g_onenet_regInfo->object_count);
    for (i = 0; i <g_onenet_regInfo->object_count; i++)
    {
        backup_object = &g_onenet_regInfo->onenet_object[i];

        //make bitmap;
        bitmap.instanceCount = backup_object->instBitmapCount;
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
    ctx->server->host = (char*)realloc(ctx->server->host, strlen(g_onenet_regInfo->net_info.remote_ip) + 1);
    memset(ctx->server->host,0,strlen(g_onenet_regInfo->net_info.remote_ip) + 1);
    memcpy(ctx->server->host,g_onenet_regInfo->net_info.remote_ip,strlen(g_onenet_regInfo->net_info.remote_ip) + 1);//保存恢复之前域名会转成IP
    sprintf(ctx->serverPort, "%d", g_onenet_regInfo->net_info.remote_port);
    management_createNetwork(ctx, ctx->server);
    management_connectServer(ctx, ctx->server);
    ctx->server->registration = g_onenet_regInfo->last_update_time;
    ctx->server->status = STATE_REGISTERED;
    ctx->server->location = xy_zalloc(strlen((const char *)g_onenet_regInfo->location) + 1);
    strcpy(ctx->server->location, (const char *)g_onenet_regInfo->location);
    ctx->nextMid = g_softap_var_nv->onenetNextMid;

    xy_printf("[CIS]resume cis context, obsv count(%d)", g_onenet_regInfo->observed_count);
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

    osMutexRelease(g_onenet_mutex);
}

int lwm2m_common_bak_config()
{
#if LWM2M_COMMON_VER
    if(xy_lwm2m_config == NULL)
        return XY_ERR;

    memset(&g_onenet_regInfo->lwm2mUserConfig,0,sizeof(lwm2m_common_user_config_t));
    g_onenet_regInfo->lwm2mUserConfig.bootstrap_flag = xy_lwm2m_config->bootstrap_flag;
    if(xy_lwm2m_config->server_host)
        strcpy(g_onenet_regInfo->lwm2mUserConfig.server_host,xy_lwm2m_config->server_host);
    memcpy(g_onenet_regInfo->lwm2mUserConfig.server_ip,xy_lwm2m_config->server_ip,sizeof(g_onenet_regInfo->lwm2mUserConfig.server_ip));
    g_onenet_regInfo->lwm2mUserConfig.port = xy_lwm2m_config->port;
    if(xy_lwm2m_config->endpoint_name)
        strcpy(g_onenet_regInfo->lwm2mUserConfig.endpoint_name,xy_lwm2m_config->endpoint_name);
    g_onenet_regInfo->lwm2mUserConfig.lifetime = xy_lwm2m_config->lifetime;
    g_onenet_regInfo->lwm2mUserConfig.security_mode = xy_lwm2m_config->security_mode;
    if(xy_lwm2m_config->psk_id)
        strcpy(g_onenet_regInfo->lwm2mUserConfig.psk_id,xy_lwm2m_config->psk_id);
    if(xy_lwm2m_config->psk)
        strcpy(g_onenet_regInfo->lwm2mUserConfig.psk,xy_lwm2m_config->psk);
    g_onenet_regInfo->lwm2mUserConfig.binding_mode = xy_lwm2m_config->binding_mode;

    g_onenet_regInfo->lwm2mUserConfig.ack_timeout = xy_lwm2m_config->ack_timeout;
    g_onenet_regInfo->lwm2mUserConfig.retrans_max_times = xy_lwm2m_config->retrans_max_times;
    g_onenet_regInfo->lwm2mUserConfig.is_auto_ack = xy_lwm2m_config->is_auto_ack;
    g_onenet_regInfo->lwm2mUserConfig.access_mode = xy_lwm2m_config->access_mode;
    g_onenet_regInfo->lwm2mUserConfig.access_mode_alternative = xy_lwm2m_config->access_mode_alternative;
    g_onenet_regInfo->lwm2mUserConfig.platform = xy_lwm2m_config->platform;

    g_onenet_regInfo->lwm2mUserConfig.lifetime_enable = xy_lwm2m_config->lifetime_enable;
    g_onenet_regInfo->lwm2mUserConfig.dtls_mode= xy_lwm2m_config->dtls_mode;
    g_onenet_regInfo->lwm2mUserConfig.dtls_version = xy_lwm2m_config->dtls_version;
#endif  
	return XY_OK;
}


void onenet_bak_user_config()
{
    g_onenet_regInfo->onenet_user_config.buf_cfg= onenet_context_refs[0].onenet_user_config.buf_cfg;
    g_onenet_regInfo->onenet_user_config.buf_URC_mode = onenet_context_refs[0].onenet_user_config.buf_URC_mode;
    g_onenet_regInfo->onenet_user_config.reg_timeout= onenet_context_refs[0].onenet_user_config.reg_timeout;
    g_onenet_regInfo->onenet_user_config.device_type = onenet_context_refs[0].onenet_user_config.device_type;
    g_onenet_regInfo->onenet_user_config.al_bs_flag = onenet_context_refs[0].onenet_user_config.al_bs_flag;
}

void onenet_resume_user_config()
{
    onenet_context_refs[0].onenet_user_config.buf_cfg = g_onenet_regInfo->onenet_user_config.buf_cfg;
    onenet_context_refs[0].onenet_user_config.buf_URC_mode = g_onenet_regInfo->onenet_user_config.buf_URC_mode;
    onenet_context_refs[0].onenet_user_config.reg_timeout = g_onenet_regInfo->onenet_user_config.reg_timeout;
    onenet_context_refs[0].onenet_user_config.device_type = g_onenet_regInfo->onenet_user_config.device_type;
    onenet_context_refs[0].onenet_user_config.al_bs_flag = g_onenet_regInfo->onenet_user_config.al_bs_flag;
}

//we assume checksum has been checked before
int need_to_resume_onenet()
{
    if (g_onenet_regInfo != NULL && g_onenet_regInfo->flag == 1)
    {
        return 1;
    }
    return 0;
}


void onenet_resume_lock(void)
{
    osMutexAcquire(g_cis_keepalive_update_mutex, osWaitForever);
    if(g_softap_fac_nv->save_cloud && g_cis_keepalive_update == 0)
    {
    	//RTC唤醒后，协议栈可能还处于PSM态，不加锁，业务代码若有delay，则会进入idle，进而可能进入深睡
        xy_work_lock(LOCK_XY_LOCAL);
        g_cis_keepalive_update = 1;
    }
    osMutexRelease(g_cis_keepalive_update_mutex);
    return;
}

void onenet_resume_unlock(void)
{
    osMutexAcquire(g_cis_keepalive_update_mutex, osWaitForever);
    if(g_softap_fac_nv->save_cloud && g_cis_keepalive_update == 1)
    {
        xy_work_unlock(LOCK_XY_LOCAL);
        g_cis_keepalive_update = 0;
    }
    osMutexRelease(g_cis_keepalive_update_mutex);
    return;
}

void onenet_resume_task()
{
    int temp_count = 0;

    if (!is_onenet_task_running(g_onenet_regInfo->ref))
    {
        if(onenet_miplcreate() != XY_OK)
        {
            xy_printf("[CIS]onenet_miplcreate failed, give cis_recovery_sem");
            onenet_resume_unlock();
            if(cis_recovery_sem != NULL)
                osSemaphoreRelease(cis_recovery_sem);

            goto out;
        }

        while (!is_onenet_task_running(g_onenet_regInfo->ref))
        {
            if (temp_count++ > 100)
            {
                break;
            }
            osDelay(100);
        }
        if (is_onenet_task_running(g_onenet_regInfo->ref) && need_to_resume_onenet())
        {
            resume_onenet_context(onenet_context_refs[g_onenet_regInfo->ref].onenet_context);
        }
    }
    else
    {
        if(cis_recovery_sem != NULL)
            osSemaphoreRelease(cis_recovery_sem);

        goto out;
    }

out:
    //onenet_resume_task_id = -1;
    //softap_TaskDelete_Index(tsk_onenetre);
    onenet_resume_task_id=NULL;
    osThreadExit();
}

rtc_timeout_cb_t onenet_notice_update_process(void *para)
{
    //不支持单核模式
    if(g_softap_fac_nv->work_mode != 0)
        return;

    send_debug_str_to_at_uart("+DBGINFO:[CIS] notice proxy update\r\n");
    //update前先上锁防止过程中随时睡下去
    onenet_resume_lock();

    if(is_onenet_task_running(0))
        return;
    //发消息到proxy线程  执行主动update
    send_msg_2_proxy(PROXY_MSG_RESUME_ONENET_UPDATE,NULL,0);
    return;
}

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
int onenet_bak_allInfo(st_context_t* context)
{
    st_object_t * objectP = NULL;
    onenet_object_t *temp = NULL;
    //st_object_t * securityObjP = NULL;
    cis_list_t * securityInstP = NULL;
    security_instance_dup_t * targetP = NULL;
	int len = 0;
    st_observed_t *observed = NULL;
    onenet_observed_t *observed_temp = NULL;
    st_server_t *server = NULL;
    unsigned short remotPort = 0;
    
	if (!is_user_onenet_context(context))
		goto failed;
	
    server = context->server;
    if (server == NULL)
		goto failed;
	
    if (context->stateStep != PUMP_STATE_READY)
    {
		send_debug_str_to_at_uart("+DBGINFO:[CIS] regStatus error\r\n");
		goto failed;
    }

	//clear all bak info
	if(g_onenet_regInfo == NULL)
    {
        g_onenet_regInfo = xy_malloc(sizeof(onenet_regInfo_t));
    }
#if CIS_ENABLE_DM
    if(!context->isDM) 
    {
#endif    
        memcpy(g_onenet_regInfo->endpointname, context->endpointName, sizeof(g_onenet_regInfo->endpointname));
        len = strlen(g_onenet_regInfo->endpointname);
        if(len <= 0)
        {
            send_debug_str_to_at_uart("+DBGINFO:[CIS] GetENDPOINTNAME err\r\n");
            goto failed;
        }
#if CIS_ENABLE_DM
	}
#endif

    g_onenet_regInfo->net_info.local_port = g_onenet_localPort;
    g_onenet_regInfo->net_info.remote_port = g_onenet_remotePort;
    memcpy(g_onenet_regInfo->net_info.remote_ip,((cisnet_t)server->sessionH)->host,sizeof(g_onenet_regInfo->net_info.remote_ip));
    //目前仅支持V4
    xy_get_ipaddr(IPADDR_TYPE_V4,&g_onenet_regInfo->net_info.local_ip);
#if CIS_ENABLE_DM
    g_onenet_regInfo->net_info.is_dm = context->isDM;
#endif
	g_onenet_regInfo->ref = 0; // customized, only support one onenet context now
    g_onenet_regInfo->last_update_time = server->registration;
    g_onenet_regInfo->life_time = context->lifetime;
    strcpy((char *)g_onenet_regInfo->location, server->location);
    g_softap_var_nv->onenetNextMid = context->nextMid;

	g_onenet_regInfo->object_count = 0;
    for (objectP = context->objectList; objectP != NULL; objectP = objectP->next)
    {
        if (!std_object_isStdObject(objectP->objID))
        {
            if (g_onenet_regInfo->object_count >= OBJECT_BACKUP_MAX)
            {
				send_debug_str_to_at_uart("+DBGINFO:[CIS] objectcount out\r\n");
			    if(g_softap_fac_nv->save_cloud)
                    onenet_resume_timer_delete();
				goto failed;
            }
            temp = &g_onenet_regInfo->onenet_object[g_onenet_regInfo->object_count];
            temp->objID = objectP->objID;
            temp->instBitmapBytes = objectP->instBitmapBytes;
            temp->instBitmapCount = objectP->instBitmapCount;
            memcpy(temp->instBitmapPtr, objectP->instBitmapPtr, objectP->instBitmapBytes);
            temp->instValidCount = objectP->instValidCount;
            temp->attributeCount = objectP->attributeCount;
            temp->actionCount = objectP->actionCount;

            g_onenet_regInfo->object_count++;
        }
    }
    
    securityInstP = (cis_list_t *)context->instSecurity;
    while (securityInstP != NULL)
    {
        targetP = (security_instance_dup_t *)securityInstP;
        if (!targetP->isBootstrap)
        {
            g_onenet_regInfo->onenet_security_instance.instanceId = targetP->instanceId;
            strcpy(g_onenet_regInfo->onenet_security_instance.host, targetP->host);
#if CIS_ENABLE_DTLS
            // todo
#endif
            g_onenet_regInfo->onenet_security_instance.isBootstrap = targetP->isBootstrap;
            g_onenet_regInfo->onenet_security_instance.shortID = targetP->shortID;
            g_onenet_regInfo->onenet_security_instance.clientHoldOffTime = targetP->clientHoldOffTime;
            g_onenet_regInfo->onenet_security_instance.securityMode = targetP->securityMode;
#if CIS_ENABLE_DTLS
#if CIS_ENABLE_PSK
            // todo
#endif
#endif
            break;
        }
        securityInstP = securityInstP->next;
    }

    observed = context->observedList;
    g_onenet_regInfo->observed_count = 0;
    memset(g_onenet_regInfo->observed, 0, sizeof(onenet_observed_t) * OBSERVE_BACKUP_MAX);
    while (observed != NULL)
    {
        if (g_onenet_regInfo->observed_count >= OBSERVE_BACKUP_MAX)
        {
			send_debug_str_to_at_uart("+DBGINFO:[CIS] observe count out\r\n");
		    if(g_softap_fac_nv->save_cloud)
                onenet_resume_timer_delete();
            goto failed;
        }
        observed_temp = &(g_onenet_regInfo->observed[g_onenet_regInfo->observed_count]);
        if (observed->actived)
        {
            observed_temp->msgid = observed->msgid;
            observed_temp->token_len = observed->tokenLen;
            memcpy(observed_temp->token, observed->token, observed->tokenLen);
            observed_temp->last_time = observed->lastTime;
            observed_temp->counter = observed->counter;
            observed_temp->lastMid = observed->lastMid;
            observed_temp->format = observed->format;
            memcpy(&observed_temp->uri, &observed->uri, sizeof(st_uri_t));
        }
        observed = observed->next;
        g_onenet_regInfo->observed_count++;
    }

    g_onenet_regInfo->flag = 1;
    g_onenet_regInfo->cloud_platform = context->cloud_platform;
    g_onenet_regInfo->platform_common_type = context->platform_common_type;
    if(context->cloud_platform == CLOUD_PLATFORM_COMMON)
    {
        lwm2m_common_bak_config();
    }
    else
    {
        onenet_bak_user_config();
    }
	return CLOUD_SAVE_NEED_WRITE;

failed:
    if(g_onenet_regInfo != NULL)
    	memset(g_onenet_regInfo, 0, sizeof(onenet_regInfo_t)); 
	return CLOUD_SAVE_NONEED_WRITE;
}

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
int onenet_write_regInfo(onenet_regInfo_t* onenet_info)
{
	int ret = -1;
	xy_assert(sizeof(onenet_regInfo_t) <= PAGE_AVAILABLE_SIZE);
#if XY_DM
	if(g_dm_context != NULL && g_dm_context->isDM == TRUE)
	{
	    if ((ret=onenet_bak_allInfo(g_dm_context)) < 0)
		{
			//send_debug_str_to_at_uart("+DBGINFO:[CIS]error:bak allInfo failed, no need to write flash\n");
			return CLOUD_SAVE_NONEED_WRITE;
		}
	}
	else
#endif
	{
	    if ((ret=onenet_bak_allInfo(onenet_context_refs[0].onenet_context)) < 0)
		{
			//send_debug_str_to_at_uart("+DBGINFO:[CIS]error:bak allInfo failed, no need to write flash\n");
			return CLOUD_SAVE_NONEED_WRITE;
		}
	}

    memcpy(onenet_info,g_onenet_regInfo,sizeof(onenet_regInfo_t));
    return CLOUD_SAVE_NEED_WRITE;
}

int onenet_read_regInfo(int offset)
{
	xy_assert(sizeof(onenet_regInfo_t) <= PAGE_AVAILABLE_SIZE);

	if(g_onenet_regInfo == NULL)
	{
		g_onenet_regInfo = xy_zalloc(sizeof(onenet_regInfo_t));
	}

    softap_printf(USER_LOG, WARN_LOG,"[resume_net_app_by_dl_pkt] onenet read flash \n");
    if(xy_ftl_read(ONENET_BACKUP_ADDR + offset, (unsigned char *)g_onenet_regInfo, sizeof(onenet_regInfo_t))!= XY_OK)
    {
        memset(g_onenet_regInfo, 0x00, sizeof(onenet_regInfo_t));
    }
    g_onenet_localPort = g_onenet_regInfo->net_info.local_port;
    g_onenet_remotePort = g_onenet_regInfo->net_info.remote_port;

    return 0;
}

void onenet_resume_process()
{
    int ret = 0;
    softap_printf(USER_LOG, WARN_LOG, "\r\n dynamic start onenet \r\n");

//ip地址变化之后 云业务主动保活需要上锁
    onenet_resume_lock();

    resume_net_app(ONENET_TASK);

    if(is_onenet_task_running(0))
    {
        softap_printf(USER_LOG, WARN_LOG, "[CIS] resume_update");
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_update_reg(onenet_context_refs[0].onenet_context, 0, 0,PS_SOCK_RAI_NO_INFO);
        osMutexRelease(g_onenet_mutex);

        if(ret != XY_OK )
            onenet_resume_unlock();
    }
    else
        onenet_resume_unlock();
}

void onenet_netif_up_resume_process()
{
    int ret = 0;
    unsigned short mid;
    int is_ip_changed = 0;

    is_ip_changed = is_IP_changed(ONENET_TASK);
    if(is_ip_changed == IP_IS_CHANGED || is_ip_changed == IP_RECEIVE_ERROR)
    {
        onenet_resume_lock();
        if (!is_onenet_task_running(0))
        {
            resume_net_app(ONENET_TASK);
        }

        if (is_onenet_task_running(0))
        {
            osMutexAcquire(g_onenet_mutex, osWaitForever);
            if(onenet_context_refs[0].onenet_context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
#if LWM2M_COMMON_VER
                ret = cis_update_reg_common(onenet_context_refs[0].onenet_context, xy_lwm2m_config->lifetime,
                                        xy_lwm2m_config->binding_mode, 0, &mid, 1 , PS_SOCK_RAI_NO_INFO);
#endif
            }
            else
                ret = cis_update_reg(onenet_context_refs[0].onenet_context, 0, 0,PS_SOCK_RAI_NO_INFO);
            osMutexRelease(g_onenet_mutex);

            if(ret != XY_OK )
                onenet_resume_unlock();
        }
        else
            onenet_resume_unlock();
    }

    g_onenet_resume_TskHandle= NULL;
    osThreadExit();
}

void onenet_rtc_resume_process()
{
    int ret = 0;
    ret = resume_net_app(ONENET_TASK);
    softap_printf(USER_LOG, WARN_LOG, "\r\n RTC update %d \r\n",ret);

    //未走恢复流程 或者恢复标志位错误且线程未恢复 要及时释放锁
    if(ret != RESUME_SUCCEED )
    {
        //恢复标志位为0，但线程已经在运行，标识业务被其他线程恢复，依靠业务自身线程释放锁
        if(ret == RESUME_FLAG_ERROR && is_onenet_task_running(0))
            softap_printf(USER_LOG, WARN_LOG, "\r\n [cis]have resume by other task\r\n");
        else
            onenet_resume_unlock();
    }

    g_onenet_rtc_resume_TskHandle= NULL;
    osThreadExit();
}

void onenet_resume_timer_create(void)
{
    int lifetime = 0;
    if(onenet_context_refs[0].onenet_context == NULL)
        return;

    lifetime = onenet_context_refs[0].onenet_context->lifetime;
    if(lifetime <= 0)
        xy_assert(0);
#if	VER_QUCTL260
    if(g_softap_fac_nv->save_cloud && g_softap_fac_nv->keep_cloud_alive)
#else
    if(g_softap_fac_nv->save_cloud)
#endif
    	xy_rtc_timer_create(RTC_TIMER_ONENET,(int)CLOUD_LIFETIME_DELTA(lifetime),onenet_notice_update_process,NULL);

    return;
}

void onenet_resume_timer_delete(void)
{
#if	VER_QUCTL260
    if(g_softap_fac_nv->save_cloud && g_softap_fac_nv->keep_cloud_alive)
#else
    if(g_softap_fac_nv->save_cloud)
#endif
    	xy_rtc_timer_delete(RTC_TIMER_ONENET);

    return;
}

void onenet_resume_state_process(int code)
{
   switch (code)
   {
    case CIS_EVENT_REG_SUCCESS:
        onenet_resume_timer_create();
        break;
    case CIS_EVENT_REG_FAILED:
    case CIS_EVENT_REG_TIMEOUT:
        onenet_resume_timer_delete();
        break;
    case CIS_EVENT_UPDATE_SUCCESS:
        onenet_resume_unlock();
        onenet_resume_timer_create();
        break;
    case CIS_EVENT_UPDATE_FAILED:
    case CIS_EVENT_UPDATE_TIMEOUT://update流程都要判断是否需要释放worklock
        onenet_resume_unlock();
        onenet_resume_timer_delete();
        break;
    case CIS_EVENT_UNREG_DONE:
        onenet_resume_unlock();
        onenet_resume_timer_delete();
        break;
    default:
        break;
    }
}
#endif
