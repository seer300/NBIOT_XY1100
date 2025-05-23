/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Manuel Sangoi - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Baijie & Longrong, China Mobile - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "cis_internals.h"
#include "cis_log.h"
#include "at_onenet.h"
#include "onenet_backup_proc.h"
#include "xy_proxy.h"
#include "oss_nv.h"
#include "net_app_resume.h"
#include "rtc_tmr.h"
#if CIS_ENABLE_DM
#include "dm_endpoint.h"

#endif
#define MAX_LOCATION_LENGTH 10      // utils_strlen("/rd/65534") + 1

extern unsigned int  g_cis_keepalive_update;
//[XY]Add for onenet loop
extern osSemaphoreId_t cis_poll_sem;

static int prv_getRegistrationQuery(st_context_t * contextP,
                                    st_server_t * server,
                                    char * buffer,
                                    size_t length)
{
    int index;
    int res;

    (void) server;

    index = utils_stringCopy(buffer, length, "?lwm2m=1.0&ep=");
    if (index < 0)
    {
        return 0;
    }
    res = utils_stringCopy(buffer + index, length - index, contextP->endpointName);
    if (res < 0) 
    {
        return 0;
    }
    index += res;

    // res = utils_stringCopy(buffer + index, length - index, "&b=U");

    switch (server->binding)
    {
    case BINDING_U:
        res = utils_stringCopy(buffer + index, length - index, "&b=U");
        break;
    case BINDING_UQ:
        res = utils_stringCopy(buffer + index, length - index, "&b=UQ");
        break;
    case BINDING_S:
        res = utils_stringCopy(buffer + index, length - index, "&b=S");
        break;
    case BINDING_SQ:
        res = utils_stringCopy(buffer + index, length - index, "&b=SQ");
        break;
    case BINDING_US:
        res = utils_stringCopy(buffer + index, length - index, "&b=US");
        break;
    case BINDING_UQS:
        res = utils_stringCopy(buffer + index, length - index, "&b=UQS");
        break;
    default:
        res = -1;
    }

    if (res < 0) 
    {
        return 0;
    }

    return index + res;
}

extern void utils_send_timer_del();
static void prv_handleRegistrationReply(st_transaction_t * transacP,
                                        void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    st_server_t * targetP = (st_server_t*)(transacP->server);
    st_context_t* context = (st_context_t*)transacP->userData;
    if(targetP == NULL && context == NULL){
        LOGE("ERROR:Registration update failed,server is NULL");
        return;
    }

    if(targetP == NULL){
        LOGE("ERROR:Registration failed,server is NULL");
        return;
    }


    if (targetP->status == STATE_REG_PENDING)
    {
        cis_time_t tv_sec = utils_gettime_s();
        if (tv_sec >= 0)
        {
            targetP->registration = tv_sec;
            context->lifetimeWarnningTime = 0;
        }

        if (packet != NULL && packet->code == COAP_201_CREATED)
        {
            core_callbackEvent(context,CIS_EVENT_REG_SUCCESS,NULL);
#if VER_QUCTL260
            context->reg_times = 0;
#endif
            targetP->status = STATE_REGISTERED;
            if (NULL != targetP->location)
            {
                cis_free(targetP->location);
            }
            targetP->location = coap_get_multi_option_as_string(packet->location_path);

            if(context->cloud_platform == CLOUD_PLATFORM_ANDLINK || (context->cloud_platform == CLOUD_PLATFORM_COMMON && context->platform_common_type == CLOUD_PLATFORM_COMMON_ANDLINK))
            {
                st_observed_t* observe = NULL;
                st_uri_t uriP = {0};
                uri_make(19, 1, 0 ,&uriP);
                observe = prv_getObserved(context, &uriP);
                observe->actived = true;
                observe->format = LWM2M_CONTENT_OPAQUE;
                observe->msgid = packet->mid;
                observe->tokenLen = packet->token_len;
                cis_memcpy(observe->token, packet->token, packet->token_len);
                observe->lastTime = utils_gettime_s();
            }

            LOGI("Registration successful");
        }
        else if(packet != NULL)
        {
            core_callbackEvent(context,CIS_EVENT_REG_FAILED,NULL);
#if VER_QUCTL260
            context->reg_times++;
#endif
            targetP->status = STATE_REG_FAILED;
            LOGE("ERROR:Registration failed");
        }else
        {
            core_callbackEvent(context,CIS_EVENT_REG_TIMEOUT,NULL);
			core_updatePumpState(context, PUMP_STATE_UNREGISTER);

            targetP->status = STATE_REG_FAILED;
            LOGE("ERROR:Registration timeout");
        
        }
    }
}

#define PRV_QUERY_BUFFER_LENGTH 200

// send the registration for the server
static coap_status_t prv_register(st_context_t * contextP)
{
    
    char* query = NULL;
    uint8_t* payload = NULL;  
    int query_length = 0;
    int payload_length = 0;
    st_transaction_t * transaction = NULL;
    coap_status_t result = COAP_NO_ERROR; 

    st_server_t* server = contextP->server;
    query = (char*)cis_malloc(CIS_COFNIG_REG_QUERY_SIZE);
    if(query == NULL){
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }
    payload = (uint8_t*)cis_malloc(CIS_COFNIG_REG_PAYLOAD_SIZE);
    if(payload == NULL){
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    payload_length = object_getRegisterPayload(contextP, payload, CIS_COFNIG_REG_PAYLOAD_SIZE - 1);
    if (payload_length == 0) 
    {
       result = COAP_500_INTERNAL_SERVER_ERROR;
       goto TAG_END;
    }
    
    query_length = prv_getRegistrationQuery(contextP, contextP->server, query, CIS_COFNIG_REG_QUERY_SIZE);

    if (query_length == 0) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    if (0 != contextP->lifetime)
    {
        int res;

        res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, QUERY_DELIMITER QUERY_LIFETIME);
        if (res < 0) 
        {
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
        res = utils_intCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, contextP->lifetime);
        if (res < 0) 
        {
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
    }

    if (NULL == server->sessionH) return COAP_503_SERVICE_UNAVAILABLE;
	
    transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, contextP->nextMid++, 4, NULL);
    if (transaction == NULL) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    coap_set_header_uri_path(transaction->message, "/"URI_REGISTRATION_SEGMENT);
    coap_set_header_uri_query(transaction->message, query);
    coap_set_header_content_type(transaction->message, LWM2M_CONTENT_LINK);
#if CIS_ENABLE_AUTH
#if CIS_ENABLE_DM
    if(contextP->isDM!=TRUE&&contextP->authCode!=NULL)
#else
	 if(contextP->authCode!=NULL)
#endif
     coap_set_header_auth_code( transaction->message, contextP->authCode );
#endif
    coap_set_payload(transaction->message, payload, payload_length);
    transaction->callback = prv_handleRegistrationReply;
    transaction->userData = (void *)contextP;
    transaction->server = contextP->server;
    contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);


    server->status = STATE_REG_PENDING;

    if ( ! transaction_send(contextP, transaction, PS_SOCK_RAI_NO_INFO)) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }


TAG_END:
    cis_free(query);
    cis_free(payload);
    return result;
}

static void prv_handleRegistrationUpdateReply(st_transaction_t * transacP,
                                              void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    st_server_t * targetP = (st_server_t *)(transacP->server);
    st_context_t* context = (st_context_t*)transacP->userData;
    if(targetP == NULL && context == NULL){
        LOGE("ERROR:Registration update failed,server is NULL");
        return;
    }

    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        cis_time_t tv_sec = utils_gettime_s();
        if (tv_sec >= 0)
        {
            targetP->registration = tv_sec;
            context->lifetimeWarnningTime = 0;
        }
        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = STATE_REGISTERED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, (void *)transacP->mID);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, NULL);
            //onenet_bak_updateInfo(context, tv_sec);
            LOGD("Registration update successful");
        }
        else if (packet != NULL)
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, (void *)transacP->mID);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, NULL);
            LOGE("ERROR:Registration update failed");
        }
        else
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, (void *)transacP->mID);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, NULL);
            LOGE("ERROR:Registration update failed");
        }
    }
}

#if LWM2M_COMMON_VER
static void prv_handleRegistrationUpdateReply_common_lifetime(st_transaction_t *transacP,
                                                              void *message)
{
    coap_packet_t *packet = (coap_packet_t *)message;
    st_server_t *targetP = (st_server_t *)(transacP->server);
    st_context_t *context = (st_context_t *)transacP->userData;
    if (targetP == NULL && context == NULL)
    {
        LOGE("ERROR:Registration update failed,server is NULL");
        return;
    }

    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        cis_time_t tv_sec = utils_gettime_s();
        if (tv_sec >= 0)
        {
            targetP->registration = tv_sec;
            context->lifetimeWarnningTime = 0;
        }
        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = STATE_REGISTERED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, (void *)-2);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, NULL);
            //onenet_bak_updateInfo(context, tv_sec);
            LOGD("Registration update successful");
        }
        else if(packet != NULL)
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, (void *)transacP->mID);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, NULL);
            LOGE("ERROR:Registration update failed");
        }
        else
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, (void *)transacP->mID);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, NULL);
            LOGE("ERROR:Registration update failed");
        }
    }
}

static void prv_handleRegistrationUpdateReply_common_binding(st_transaction_t *transacP,
                                                             void *message)
{
    coap_packet_t *packet = (coap_packet_t *)message;
    st_server_t *targetP = (st_server_t *)(transacP->server);
    st_context_t *context = (st_context_t *)transacP->userData;
    if (targetP == NULL && context == NULL)
    {
        LOGE("ERROR:Registration update failed,server is NULL");
        return;
    }

    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        cis_time_t tv_sec = utils_gettime_s();
        if (tv_sec >= 0)
        {
            targetP->registration = tv_sec;
            context->lifetimeWarnningTime = 0;
        }
        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = STATE_REGISTERED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, (void *)-3);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, NULL);
            //onenet_bak_updateInfo(context, tv_sec);
            LOGD("Registration update successful");
        }
        else if (packet != NULL)
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, (void *)transacP->mID);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, NULL);
            LOGE("ERROR:Registration update failed");
        }
        else
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, (void *)transacP->mID);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, NULL);
            LOGE("ERROR:Registration update failed");
        }
    }
}

static void prv_handleRegistrationUpdateReply_common_auto(st_transaction_t *transacP, void *message)
{
    coap_packet_t *packet = (coap_packet_t *)message;
    st_server_t *targetP = (st_server_t *)(transacP->server);
    st_context_t *context = (st_context_t *)transacP->userData;
    if (targetP == NULL && context == NULL)
    {
        LOGE("ERROR:Registration update failed,server is NULL");
        return;
    }

    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        cis_time_t tv_sec = utils_gettime_s();
        if (tv_sec >= 0)
        {
            targetP->registration = tv_sec;
            context->lifetimeWarnningTime = 0;
        }
        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = STATE_REGISTERED;
            //Take regStatus for baking cis info
            // SET_ONENET_REGINFO_PARAM(regStatus, 1);
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, (void *)-1);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_SUCCESS, NULL);
            //onenet_bak_updateInfo(context, tv_sec);
            LOGD("Registration update successful");
        }
        else if (packet != NULL)
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, (void *)-1);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_FAILED, NULL);
            LOGE("ERROR:Registration update failed");
        }
        else
        {
            targetP->status = STATE_REG_FAILED;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, (void *)-1);
            }
            else
                core_callbackEvent(context, CIS_EVENT_UPDATE_TIMEOUT, NULL);
            LOGE("ERROR:Registration update failed");
        }
    }
}
#endif

static coap_status_t prv_updateRegistration(st_context_t * contextP,
                                  bool withObjects, uint8_t raiflag)
{
    char* query = NULL;
    uint8_t* payload = NULL;
    int payload_length = 0;
    int query_length = 0;
    st_transaction_t * transaction = NULL;
    coap_status_t result = COAP_NO_ERROR;
    st_server_t* server = contextP->server;
	uint8_t *dmQuery=NULL;
    int dm_query_length=0;	
    if (contextP->transactionList != NULL)
    {
        return COAP_NO_ERROR;
    }

    query = (char*)cis_malloc(CIS_COFNIG_REG_QUERY_SIZE);
    if(query == NULL){
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }
    

    transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, contextP->nextMid++, 4, NULL);
    if (transaction == NULL) 
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;

    }

    coap_set_header_uri_path(transaction->message, server->location);
#if CIS_ENABLE_DM

  if(contextP->isDM==TRUE)
  {
			dm_query_length = prv_getDmUpdateQueryLength(contextP, server);
			if(dm_query_length == 0)
			{
				result =COAP_500_INTERNAL_SERVER_ERROR;
				goto TAG_END;
			}
			dmQuery = (uint8_t *)cis_malloc(dm_query_length);
			if(!dmQuery)
			{
				result =COAP_500_INTERNAL_SERVER_ERROR;
				goto TAG_END;
			}
			if(prv_getDmUpdateQuery(contextP, server, dmQuery, dm_query_length) != dm_query_length)
			{
				cis_free(dmQuery);
				result =COAP_500_INTERNAL_SERVER_ERROR;
				goto TAG_END;
			}
			coap_set_header_uri_query(transaction->message, (const char *)dmQuery);
			cis_free(dmQuery);
	
  }
 else
 #endif
 {
#if CIS_ENABLE_AUTH
    if(contextP->authCode!=NULL)
     coap_set_header_auth_code( transaction->message, contextP->authCode );
#endif

    if (0 != contextP->lifetime)
    {
        int res;

        res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, QUERY_LIFETIME);
        if (res < 0) 
        {
            transaction_free(transaction);
            result =COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
        res = utils_intCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, contextP->lifetime);
        if (res < 0) 
        {
            transaction_free(transaction);
            result =COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;

        coap_set_header_uri_query(transaction->message, query);
    }

}

    if (withObjects == TRUE)
    {
        payload = (uint8_t*)cis_malloc(CIS_COFNIG_REG_PAYLOAD_SIZE);
        if(payload == NULL){
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        payload_length = object_getRegisterPayload(contextP, payload, CIS_COFNIG_REG_PAYLOAD_SIZE);
        if (payload_length == 0)
        {
            transaction_free(transaction);
            result =COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        coap_set_payload(transaction->message, payload, payload_length);
    }

    transaction->callback = prv_handleRegistrationUpdateReply;
    transaction->userData = (void *) contextP;
    transaction->server = contextP->server;

    contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);
    server->status = STATE_REG_UPDATE_PENDING;

    if (transaction_send(contextP, transaction, raiflag))
    {
        result = COAP_NO_ERROR;
        goto TAG_END;
    }

TAG_END:
    cis_free(payload);
    cis_free(query);
    return result;
}

#if LWM2M_COMMON_VER
static coap_status_t prv_updateRegistration_common(st_context_t *contextP, bool withObjects, uint16_t *mid, int change_lifetime, uint8_t raiflag)
{
    char *query = NULL;
    uint8_t *payload = NULL;
    int payload_length = 0;
    int query_length = 0;
    st_transaction_t *transaction = NULL;
    coap_status_t result = COAP_NO_ERROR;
    st_server_t *server = contextP->server;
    uint8_t *dmQuery = NULL;
    int dm_query_length = 0;
    int res;
    char *at_str = NULL;
    if (contextP->transactionList != NULL)
    {
        return COAP_NO_ERROR;
    }

    query = (char *)cis_malloc(CIS_COFNIG_REG_QUERY_SIZE);
    if (query == NULL)
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    *mid = contextP->nextMid++;
    transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, *mid, 4, NULL);
    if (transaction == NULL)
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    coap_set_header_uri_path(transaction->message, server->location);

    if (change_lifetime && 0 != contextP->lifetime)
    {

        res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, QUERY_LIFETIME);
        if (res < 0)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
        res = utils_intCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, contextP->lifetime);
        if (res < 0)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
    }
    else
    {
        switch (server->binding)
        {
        case BINDING_U:
            res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "b=U");
            break;
        case BINDING_UQ:
            res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "b=UQ");
            break;
        case BINDING_S:
            res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "b=S");
            break;
        case BINDING_SQ:
            res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "b=SQ");
            break;
        case BINDING_US:
            res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "b=US");
            break;
        case BINDING_UQS:
            res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "b=UQS");
            break;
        default:
            res = -1;
        }

        if (res < 0)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
    }
    coap_set_header_uri_query(transaction->message, query);

    if (withObjects == TRUE)
    {
        payload = (uint8_t *)cis_malloc(CIS_COFNIG_REG_PAYLOAD_SIZE);
        if (payload == NULL)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        payload_length = object_getRegisterPayload(contextP, payload, CIS_COFNIG_REG_PAYLOAD_SIZE);
        if (payload_length == 0)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        coap_set_payload(transaction->message, payload, payload_length);
    }
    if (change_lifetime)
        transaction->callback = prv_handleRegistrationUpdateReply_common_lifetime;
    else
        transaction->callback = prv_handleRegistrationUpdateReply_common_binding;
    transaction->userData = (void *)contextP;
    transaction->server = contextP->server;

    contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);
    server->status = STATE_REG_UPDATE_PENDING;

    if (transaction_send(contextP, transaction, raiflag))
    {
        result = COAP_NO_ERROR;
        if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
        {
            at_str = xy_malloc(64);
            snprintf(at_str, 64, "\r\n+QLAUPDATE: %d,%d\r\n", convert_coap_code_to_commond_result_code(result), *mid);
            send_rsp_str_to_ext(at_str);
            xy_free(at_str);
        }
        goto TAG_END;
    }

TAG_END:
    cis_free(payload);
    cis_free(query);
    return result;
}
static coap_status_t prv_updateRegistration_common_auto(st_context_t *contextP, bool withObjects, uint8_t raiflag)
{
    char *query = NULL;
    uint8_t *payload = NULL;
    int payload_length = 0;
    int query_length = 0;
    st_transaction_t *transaction = NULL;
    coap_status_t result = COAP_NO_ERROR;
    st_server_t *server = contextP->server;
    uint8_t *dmQuery = NULL;
    int dm_query_length = 0;
    if (contextP->transactionList != NULL)
    {
        return COAP_NO_ERROR;
    }

    query = (char *)cis_malloc(CIS_COFNIG_REG_QUERY_SIZE);
    if (query == NULL)
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, contextP->nextMid++, 4, NULL);
    if (transaction == NULL)
    {
        result = COAP_500_INTERNAL_SERVER_ERROR;
        goto TAG_END;
    }

    coap_set_header_uri_path(transaction->message, server->location);

    if (0 != contextP->lifetime)
    {
        int res;

        res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, QUERY_LIFETIME);
        if (res < 0)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;
        res = utils_intCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, contextP->lifetime);
        if (res < 0)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        query_length += res;

        // switch (server->binding)
        // {
        // case BINDING_U:
        //     res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "&b=U");
        //     break;
        // case BINDING_UQ:
        //     res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "&b=UQ");
        //     break;
        // case BINDING_S:
        //     res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "&b=S");
        //     break;
        // case BINDING_SQ:
        //     res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "&b=SQ");
        //     break;
        // case BINDING_US:
        //     res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "&b=US");
        //     break;
        // case BINDING_UQS:
        //     res = utils_stringCopy(query + query_length, PRV_QUERY_BUFFER_LENGTH - query_length, "&b=UQS");
        //     break;
        // default:
        //     res = -1;
        // }

        // if (res < 0)
        // {
        //     transaction_free(transaction);
        //     result = COAP_500_INTERNAL_SERVER_ERROR;
        //     goto TAG_END;
        // }
        // query_length += res;

        coap_set_header_uri_query(transaction->message, query);
    }

    if (withObjects == TRUE)
    {
        payload = (uint8_t *)cis_malloc(CIS_COFNIG_REG_PAYLOAD_SIZE);
        if (payload == NULL)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        payload_length = object_getRegisterPayload(contextP, payload, CIS_COFNIG_REG_PAYLOAD_SIZE);
        if (payload_length == 0)
        {
            transaction_free(transaction);
            result = COAP_500_INTERNAL_SERVER_ERROR;
            goto TAG_END;
        }
        coap_set_payload(transaction->message, payload, payload_length);
    }

    transaction->callback = prv_handleRegistrationUpdateReply_common_auto;
    transaction->userData = (void *)contextP;
    transaction->server = contextP->server;

    contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);
    server->status = STATE_REG_UPDATE_PENDING;

    if (transaction_send(contextP, transaction, raiflag))
    {
        result = COAP_NO_ERROR;
        goto TAG_END;
    }

TAG_END:
    cis_free(payload);
    cis_free(query);
    return result;
}

int registration_update_registration_common(st_context_t *contextP, bool withObjects, uint16_t *mid, int binding_mode, int change_lifetime, uint8_t raiflag)
{
    st_server_t * targetP;
	
    LOGD("update_registration state: %s", STR_STATE(contextP->stateStep));

    targetP = contextP->server;
    if(targetP == NULL)
    {
        // no server found
        LOGE("ERROR: no server found for update registration!");
        return COAP_404_NOT_FOUND;
    }

    if (targetP->status == STATE_REGISTERED)
    {
        targetP->raiflag = raiflag;
        // targetP->binding = binding_mode;
        if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
        {
            if (binding_mode == 0)
            {
                targetP->binding = BINDING_U;
            }
            else
            {
                targetP->binding = BINDING_UQ;
            }
        }
        else
        {
            targetP->binding = BINDING_U;
        }
        prv_updateRegistration_common(contextP, withObjects, mid, change_lifetime, raiflag);
        return COAP_NO_ERROR;
    }
    else
    {
        return COAP_400_BAD_REQUEST;
    }
}

int registration_update_registration_common_auto(st_context_t *contextP, bool withObjects, uint8_t raiflag)
{
    st_server_t *targetP;

    LOGD("update_registration state: %s", STR_STATE(contextP->stateStep));

    targetP = contextP->server;
    if (targetP == NULL)
    {
        // no server found
        LOGE("ERROR: no server found for update registration!");
        return COAP_404_NOT_FOUND;
    }

    if (targetP->status == STATE_REGISTERED)
    {
        targetP->raiflag = raiflag;
        prv_updateRegistration_common_auto(contextP, withObjects, raiflag);
        return COAP_NO_ERROR;
    }
    else
    {
        return COAP_400_BAD_REQUEST;
    }
}
#endif

// update the registration
int registration_update_registration(st_context_t *contextP, bool withObjects, uint8_t raiflag)
{
    st_server_t *targetP;

    LOGD("update_registration state: %s", STR_STATE(contextP->stateStep));

    targetP = contextP->server;
    if (targetP == NULL)
    {
        // no server found
        LOGE("ERROR: no server found for update registration!");
        return COAP_404_NOT_FOUND;
    }

    if (targetP->status == STATE_REGISTERED)
    {
        targetP->raiflag = raiflag;
        if (withObjects)
        {
            targetP->status = STATE_REG_UPDATE_NEEDED_WITHOBJECTS;
        }else{
            targetP->status = STATE_REG_UPDATE_NEEDED;
        }

        //[XY]Add for onenet loop
		if(cis_poll_sem != NULL)
			osSemaphoreRelease(cis_poll_sem);

        return COAP_NO_ERROR;
    }
    else
    {
        return COAP_400_BAD_REQUEST;
    }
}

coap_status_t registration_start(st_context_t * contextP)
{
    st_server_t * targetP;
    uint8_t result;

    result = COAP_NO_ERROR;

    targetP = contextP->server;
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
        if (targetP->status == STATE_CONNECTED
         || targetP->status == STATE_REG_FAILED)
        {
            result = prv_register(contextP);
        }
        targetP = targetP->next;
    }

    return result;
}


/*
 * Returns STATE_REG_PENDING if at least one registration is still pending
 * Returns STATE_REGISTERED if no registration is pending and there is at least one server the client is registered to
 * Returns STATE_REG_FAILED if all registration failed.
 */
et_status_t registration_getStatus(st_context_t * contextP)
{
    st_server_t * targetP;
    et_status_t reg_status;

    targetP = contextP->server;
    reg_status = STATE_REG_FAILED;

    if (targetP != NULL)
    {
        switch (targetP->status)
        {
            case STATE_REGISTERED:
            case STATE_REG_UPDATE_NEEDED:
            case STATE_REG_UPDATE_NEEDED_WITHOBJECTS:
            case STATE_REG_UPDATE_PENDING:
                {
                    if (reg_status == STATE_REG_FAILED)
                    {
                        reg_status = STATE_REGISTERED;
                    }
                }
                break;
            case STATE_REG_PENDING:
                {
                   reg_status = STATE_REG_PENDING;
                }
                break;
            case STATE_DEREG_PENDING:
                {
                    reg_status = STATE_DEREG_PENDING;
                }
                break;
            case STATE_REG_FAILED:
            case STATE_UNCREATED:
            default:
                break;
        }
    }

    return reg_status;
}

static void prv_handleDeregistrationReply(st_transaction_t * transacP,
                                          void * message)
{
    st_server_t * targetP;

    (void) message;

    targetP = (st_server_t *)(transacP->server);
    if (NULL != targetP)
    {
        if (targetP->status == STATE_DEREG_PENDING)
        {
            targetP->status = STATE_UNCREATED;
        }
    }
}

void registration_deregister(st_context_t * contextP)
{
    st_transaction_t * transaction;
    st_server_t* serverP;

    serverP = contextP->server;
    if(serverP != NULL)
    {

        LOGD("State: %s, serverP->status: %s", STR_STATE(contextP->stateStep), STR_STATUS(serverP->status));

        if (serverP->status == STATE_UNCREATED
         || serverP->status == STATE_CREATED
         || serverP->status == STATE_CONNECT_PENDING
         || serverP->status == STATE_CONNECTED
         || serverP->status == STATE_REG_PENDING
         || serverP->status == STATE_DEREG_PENDING
         || serverP->status == STATE_REG_FAILED
         || serverP->location == NULL)
        {
            return;
        }

        transaction = transaction_new(COAP_TYPE_CON, COAP_DELETE, NULL, NULL, contextP->nextMid++, 4, NULL);
        if (transaction == NULL) 
        {
            return;
        }

        coap_set_header_uri_path(transaction->message, serverP->location);
#if CIS_ENABLE_AUTH
#if CIS_ENABLE_DM
    if(contextP->isDM!=TRUE&&contextP->authCode!=NULL)
#else
	 if(contextP->authCode!=NULL)
#endif
        coap_set_header_auth_code( transaction->message, contextP->authCode );
#endif
        transaction->callback = prv_handleDeregistrationReply;
        transaction->userData = (void *) contextP;
        transaction->server = serverP;
        contextP->transactionList = (st_transaction_t *)CIS_LIST_ADD(contextP->transactionList, transaction);
        if (transaction_send(contextP, transaction, PS_SOCK_RAI_NO_INFO) == TRUE)
        {
            serverP->status = STATE_DEREG_PENDING;
        }
    }
}


// update the registration if needed
// check if the registration expired
void registration_step(st_context_t * contextP,
                       cis_time_t currentTime)
{
    st_server_t * targetP = contextP->server;

    if (targetP != NULL)
    {
        switch (targetP->status)
        {
        case STATE_REGISTERED:
            {
                cis_time_t lifetime;
                cis_time_t interval;
                cis_time_t lasttime;
                cis_time_t notifytime;

                lifetime = contextP->lifetime;
                lasttime = targetP->registration;

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

                interval = lasttime + lifetime - currentTime;   			
                xy_printf("[CIS]time %d,%d",interval,currentTime);

                if((interval - 1) <= lifetime - (cis_time_t)CLOUD_LIFETIME_DELTA(lifetime))//interval毫秒转换成秒 四舍五入有1s误差
                {
                    //once event
                    if(contextP->lifetimeWarnningTime >= 0)
                    {
#if VER_QUCTL260
                    	//if( g_softap_fac_nv->keep_cloud_alive == 0)  //MG 20221121 hid by LGF
                    	{
                    		if((interval - 1) <= 0)
                    		{
                    			contextP->callback.onEvent(contextP,CIS_EVENT_LIFETIME_TIMEOUT,(void*)interval);
                    			core_updatePumpState(contextP, PUMP_STATE_UNREGISTER);
                    		}
                    		else
                    		{
                    			if(contextP->lifetimeWarnningTime == 0 || currentTime - contextP->lifetimeWarnningTime > interval)
                    			{
                    				contextP->callback.onEvent(contextP,CIS_EVENT_UPDATE_NEED,(void*)interval);
                    				contextP->lifetimeWarnningTime = currentTime;
                    			}
                    		}

                    	}
                    	//else	//MG 20221121 hid by LGF
                    	{
#endif
							LOGE("ERROR:lifetime timeout occurred.");
		//                        contextP->lifetimeWarnningTime = INFINITE_TIMEOUT;  //赋值-1之后 并发场景发送列表不为空update失败不会再次走进该流程
		#if LWM2M_COMMON_VER
							if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && xy_lwm2m_config->lifetime_enable == 1)
							{
								prv_updateRegistration_common_auto(contextP, false, targetP->raiflag);
							}
							else
		#endif
							{
								if (contextP->cloud_platform == CLOUD_PLATFORM_ONENET || contextP->cloud_platform == CLOUD_PLATFORM_ANDLINK)
									prv_updateRegistration(contextP, false,targetP->raiflag);
							}
#if VER_QUCTL260
                    	}
#endif
                    }

                }
                else if(interval <= notifytime && !g_softap_fac_nv->save_cloud)
                {
                    //a half of last interval time;
                    if(contextP->lifetimeWarnningTime == 0 ||
                        currentTime - contextP->lifetimeWarnningTime > interval)
                    {
                        LOGW("WARNNING:lifetime timeout warnning time:%ds",interval);
                        contextP->callback.onEvent(contextP,CIS_EVENT_UPDATE_NEED,(void*)interval);
                        contextP->lifetimeWarnningTime = currentTime;
                    }
                }

            }
            break;
        case STATE_REG_UPDATE_NEEDED:
            prv_updateRegistration(contextP, false, targetP->raiflag);
            break;
        case STATE_REG_UPDATE_NEEDED_WITHOBJECTS:
            prv_updateRegistration(contextP, true, targetP->raiflag);
            break;
        case STATE_REG_FAILED:
            if (contextP->server->sessionH != NULL)
            {
                cisnet_disconnect((cisnet_t)contextP->server->sessionH);
                cisnet_destroy((cisnet_t)contextP->server->sessionH);
                contextP->server->sessionH = NULL;
            }
            break;

        default:
            break;
        }
    }

}

#endif
