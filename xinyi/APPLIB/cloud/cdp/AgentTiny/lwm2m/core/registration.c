/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

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
 *    Scott Bertin - Please refer to git log
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
#if TELECOM_VER

#include "internals.h"
#include "cdp_backup.h"
#include "atiny_context.h"
#include "net_app_resume.h"
#include "rtc_tmr.h"
#include "xy_rtc_api.h"
#include "xy_ps_api.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_LOCATION_LENGTH 10      // strlen("/rd/65534") + 1
extern int cdp_deregister_flag;
extern osSemaphoreId_t cdp_wait_sem;

#ifdef LWM2M_CLIENT_MODE

static int rsrp, sinr,txpower,CellID;
static int prv_getRegistrationQueryLength(lwm2m_context_t *contextP,
        xy_lwm2m_server_t *server)
{
    int index;
    int res;
    char buffer[21];
	char nccid[21]={0};
	char chiptype[15]={0};
	char modulever[21]={0};
	char imsi[IMSI_LEN+1]={0};
	char verext[29]={0};
	char str_nccid[30]={0};
	char str_rsrp[20]={0};
	char str_sinr[20]={0};
	char str_txpower[20]={0};
	char str_cellid[25]={0};
	char str_imsi[25]={0};
	char str_verext[35] = {0};
	char str_moduulever[30]={0};
	
    index = strlen(QUERY_STARTER CDP_QUERY_VERSION_FULL QUERY_DELIMITER QUERY_NAME);//?1.0&ep=
    index += strlen(contextP->endpointName);

    if (NULL != contextP->msisdn)
    {
        index += strlen(QUERY_DELIMITER QUERY_SMS);
        index += strlen(contextP->msisdn);
    }

    switch (server->binding)
    {
    case XY_BINDING_U:
        index += strlen("&b=U");
        break;
    case XY_BINDING_UQ:
        index += strlen("&b=UQ");
        break;
    case XY_BINDING_S:
        index += strlen("&b=S");
        break;
    case XY_BINDING_SQ:
        index += strlen("&b=SQ");
        break;
    case XY_BINDING_US:
        index += strlen("&b=US");
        break;
    case XY_BINDING_UQS:
        index += strlen("&b=UQS");
        break;
    default:
        return 0;
    }

    if (0 != server->lifetime)
    {
        index += strlen(QUERY_DELIMITER QUERY_LIFETIME);//&lt=
        res = xy_utils_intToText(server->lifetime, (uint8_t *)buffer, sizeof(buffer));
        if (res == 0) return 0;
        index += res;
    }

    
	if(xy_get_IMSI(imsi, IMSI_LEN) != XY_OK)
		return 0;
	
	if(xy_get_NCCID(nccid,21) !=XY_OK)
		return 0;
	
	if(xy_get_VERSIONEXT(verext, 28) != XY_OK)
		return 0;
	
	if(xy_get_MODULVER(modulever, 20)!=XY_OK)
		return 0;
	
	sprintf(str_imsi,"&imsi=%s",imsi);
	sprintf(str_nccid,"&iccid=%s",nccid);
	sprintf(str_verext,"&sv=%s",verext);
	sprintf(chiptype,"&chip=%s",CHIPTYPE);
	char *tmp = strstr(modulever, "-");
	if(tmp != NULL)
		sprintf(str_moduulever,"&module=%s",tmp+1);
	else
		sprintf(str_moduulever,"&module=%s",modulever);


	ril_serving_cell_info_t *rcv_servingcell_info = xy_zalloc(sizeof(ril_serving_cell_info_t));
	if(xy_get_servingcell_info(rcv_servingcell_info) != XY_OK)
	{
		xy_free(rcv_servingcell_info); 
		return 0;
	}
		
    rsrp = rcv_servingcell_info->Signalpower/10;
	sinr = rcv_servingcell_info->SNR/10;
	txpower = rcv_servingcell_info->TXpower/10;
	CellID = rcv_servingcell_info->CellID;
	sprintf(str_rsrp,"&rsrp=%d",rsrp);
	sprintf(str_sinr,"&sinr=%d",sinr);
	sprintf(str_txpower,"&txpower=%d",txpower); 
	sprintf(str_cellid,"&cellid=%d",CellID);
	xy_free(rcv_servingcell_info); 

	index+= strlen(str_imsi);
	index+= strlen(str_nccid);
	index+= strlen(str_verext);
	index+= strlen(chiptype);
	index+= strlen(str_moduulever);
	index+= strlen(str_rsrp);
	index+= strlen(str_sinr);
	index+= strlen(str_txpower);
	index+= strlen(str_cellid);

    return index + 1;
}


static int prv_getRegistrationQuery(lwm2m_context_t *contextP,
                                    xy_lwm2m_server_t *server,
                                    char *buffer,
                                    size_t length)
{
    int index;
    int res;
	char nccid[21]={0};
	char chiptype[15]={0};
	char modulever[21]={0};
	char imsi[IMSI_LEN+1]={0};
	char verext[29]={0};
	char str_nccid[30]={0};
	char str_rsrp[20]={0};
	char str_sinr[20]={0};
	char str_txpower[20]={0};
	char str_cellid[25]={0};
	char str_imsi[25]={0};
	char str_verext[35] = {0};
	char str_moduulever[30]={0};
	

    index = xy_utils_stringCopy(buffer, length, QUERY_STARTER CDP_QUERY_VERSION_FULL QUERY_DELIMITER QUERY_NAME);
    if (index < 0) return 0;
    res = xy_utils_stringCopy(buffer + index, length - index, contextP->endpointName);
    if (res < 0) return 0;
    index += res;

    if (NULL != contextP->msisdn)
    {
        res = xy_utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_SMS);
        if (res < 0) return 0;
        index += res;
        res = xy_utils_stringCopy(buffer + index, length - index, contextP->msisdn);
        if (res < 0) return 0;
        index += res;
    }

    switch (server->binding)
    {
    case XY_BINDING_U:
        res = xy_utils_stringCopy(buffer + index, length - index, "&b=U");
        break;
    case XY_BINDING_UQ:
        res = xy_utils_stringCopy(buffer + index, length - index, "&b=UQ");
        break;
    case XY_BINDING_S:
        res = xy_utils_stringCopy(buffer + index, length - index, "&b=S");
        break;
    case XY_BINDING_SQ:
        res = xy_utils_stringCopy(buffer + index, length - index, "&b=SQ");
        break;
    case XY_BINDING_US:
        res = xy_utils_stringCopy(buffer + index, length - index, "&b=US");
        break;
    case XY_BINDING_UQS:
        res = xy_utils_stringCopy(buffer + index, length - index, "&b=UQS");
        break;
    default:
        res = -1;
    }
    if (res < 0) return 0;
    index += res;

	

    if (0 != server->lifetime)
    {
        res = xy_utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_LIFETIME);
        if (res < 0) return 0;
        index += res;
        res = xy_utils_intToText(server->lifetime, (uint8_t *)buffer + index, length - index);
        if (res == 0) return 0;
        index += res;
    }


	if(xy_get_IMSI(imsi, IMSI_LEN) != XY_OK)
		return 0;
		
	if(xy_get_NCCID(nccid,21) !=XY_OK)
		return 0;
	
	if(xy_get_VERSIONEXT(verext, 28) != XY_OK)
		return 0;
	
	if(xy_get_MODULVER(modulever, 20)!=XY_OK)
		return 0;
	
	sprintf(str_imsi,"&imsi=%s",imsi);
	res = xy_utils_stringCopy(buffer + index, length - index, str_imsi);
	if (res < 0) return 0;
    index += res;
	
	sprintf(str_nccid,"&iccid=%s",nccid);
	res = xy_utils_stringCopy(buffer + index, length - index, str_nccid);
	if (res < 0) return 0;
    index += res;
	
	sprintf(str_verext,"&sv=%s",verext);
	res = xy_utils_stringCopy(buffer + index, length - index, str_verext);
	if (res < 0) return 0;
    index += res;
	
	sprintf(chiptype,"&chip=%s",CHIPTYPE);
	res = xy_utils_stringCopy(buffer + index, length - index, chiptype);
	if (res < 0) return 0;
    index += res;
	
	char *tmp = strstr(modulever, "-");
	if(tmp != NULL)
		sprintf(str_moduulever,"&module=%s",tmp+1);
	else
		sprintf(str_moduulever,"&module=%s",modulever);
	res = xy_utils_stringCopy(buffer + index, length - index, str_moduulever);
	if (res < 0) return 0;
    index += res;


	sprintf(str_rsrp,"&rsrp=%d",rsrp);
	sprintf(str_sinr,"&sinr=%d",sinr);
	sprintf(str_txpower,"&txpower=%d",txpower); 
	sprintf(str_cellid,"&cellid=%d",CellID);
	
	res = xy_utils_stringCopy(buffer + index, length - index, str_rsrp);
	if (res < 0) return 0;
    index += res;
	
	res = xy_utils_stringCopy(buffer + index, length - index, str_sinr);
	if (res < 0) return 0;
    index += res;
	
	res = xy_utils_stringCopy(buffer + index, length - index, str_txpower);
	if (res < 0) return 0;
    index += res;
	
	res = xy_utils_stringCopy(buffer + index, length - index, str_cellid);
	if (res < 0) return 0;
    index += res;
	

    if(index < (int)length)
    {
        buffer[index++] = '\0';
    }
    else
    {
        return 0;
    }

    return index;
}

static void prv_handleRegistrationReply(lwm2m_transaction_t *transacP,
                                        void *message)
{
    coap_packet_t *packet = (coap_packet_t *)message;
    xy_lwm2m_server_t *targetP = (xy_lwm2m_server_t *)(transacP->userData);

    LOG("come into  prv_handleRegistrationReply!!");
    if (targetP->status == XY_STATE_REG_PENDING)
    {
        time_t tv_sec = lwm2m_gettime();
        if (tv_sec)
        {
            targetP->registration = tv_sec;
        }
        if (packet != NULL && packet->code == COAP_201_CREATED)
        {
            targetP->status = XY_STATE_REGISTERED;
            if (NULL != targetP->location)
            {
                lwm2m_free(targetP->location);
            }
            targetP->location = xy_coap_get_multi_option_as_string(packet->location_path);

            LOG("Registration successful");
            
            if(cdp_wait_sem != NULL)
                osSemaphoreRelease(cdp_wait_sem);
        }
        else
        {
            if(packet != NULL)
				g_softap_var_nv->cdp_lwm2m_event_status = 10; //服务器拒绝
			else
				g_softap_var_nv->cdp_lwm2m_event_status = ATINY_REG_FAIL;
			targetP->status = XY_STATE_REG_FAILED;
            LOG("Registration failed");
        }
    }
}

// send the registration for a single server
static uint8_t prv_register(lwm2m_context_t *contextP,
                            xy_lwm2m_server_t *server)
{
    char *query;
    int query_length;
    uint8_t *payload;
    int payload_length;
	
    lwm2m_transaction_t *transaction;

    payload_length = xy_object_getRegisterPayloadBufferLength(contextP);
    if(payload_length == 0) return COAP_500_INTERNAL_SERVER_ERROR;
    payload = (uint8_t *)lwm2m_malloc(payload_length);
    if(!payload) return COAP_500_INTERNAL_SERVER_ERROR;
    payload_length = xy_object_getRegisterPayload(contextP, payload, payload_length);
    if(payload_length == 0)
    {
        lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
	
    query_length = prv_getRegistrationQueryLength(contextP, server);
    if(query_length == 0)
    {
        lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    query = (char *)lwm2m_malloc(query_length);
    if(!query)
    {
        lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    if(prv_getRegistrationQuery(contextP, server, query, query_length) != query_length)
    {
        lwm2m_free(payload);
        lwm2m_free(query);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if (server->sessionH == NULL)
    {
        server->sessionH = lwm2m_connect_server(server->secObjInstID, contextP->userData, false);
    }

    if (NULL == server->sessionH)
    {
        lwm2m_free(payload);
        lwm2m_free(query);
        return COAP_503_SERVICE_UNAVAILABLE;
    }

    transaction = xy_transaction_new(server->sessionH, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL);
    if (transaction == NULL)
    {
        lwm2m_free(payload);
        lwm2m_free(query);
        return COAP_503_SERVICE_UNAVAILABLE;
    }
    //    coap_pdu_t *pdu = (coap_pdu_t *)(transaction->message);

    xy_coap_set_header_uri_path(transaction->message, "/"URI_REGISTRATION_SEGMENT);
    xy_coap_set_header_uri_query(transaction->message, query);
    xy_coap_set_header_content_type(transaction->message, XY_LWM2M_CONTENT_LINK);
    xy_coap_set_payload(transaction->message, payload, payload_length);

    transaction->callback = prv_handleRegistrationReply;
    transaction->userData = (void *) server;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);
    if (xy_transaction_send(contextP, transaction) != 0)
    {
        lwm2m_free(payload);
        lwm2m_free(query);
        return COAP_503_SERVICE_UNAVAILABLE;
    }

    lwm2m_free(payload);
    lwm2m_free(query);

    server->status = XY_STATE_REG_PENDING;

    return COAP_NO_ERROR;
}

extern int cdp_register_fail_flag;
static void prv_handleRegistrationUpdateReply(lwm2m_transaction_t *transacP,
        void *message)
{
    coap_packet_t *packet = (coap_packet_t *)message;
    xy_lwm2m_server_t *targetP = (xy_lwm2m_server_t *)(transacP->userData);

    if (targetP->status == XY_STATE_REG_UPDATE_PENDING)
    {
        time_t tv_sec = lwm2m_gettime();
        if (tv_sec )
        {
            targetP->registration = tv_sec;
        }

        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = XY_STATE_REGISTERED;
            LOG("Registration update successful");
            g_phandle->lwm2m_context->serverList->registration = tv_sec;

            lwm2m_notify_even(MODULE_LWM2M, XY_STATE_UPDATE_DONE, NULL, 0);

            if(cdp_wait_sem != NULL)
                osSemaphoreRelease(cdp_wait_sem);
        }
        else
        {
            targetP->status = XY_STATE_REG_FAILED;
			//MG 20230607
			/* force to set flag and quit thread
			** if not, reg update fail then reg. double tiemout
			*/
			cdp_register_fail_flag = 1;
			//MG END
            LOG("Registration update failed");
        }
    }
}

static int prv_updateRegistration(lwm2m_context_t *contextP,
                                  xy_lwm2m_server_t *server,
                                  bool withObjects)
{
    lwm2m_transaction_t *transaction;
    uint8_t *payload = NULL;
    int payload_length;

    transaction = xy_transaction_new(server->sessionH, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    xy_coap_set_header_uri_path(transaction->message, server->location);
//    xy_cdp_set_raiflag(transaction->message, cdp_CON_WAIT_REPLY_RAI);

    if (withObjects == true)
    {
        payload_length = xy_object_getRegisterPayloadBufferLength(contextP);
        if(payload_length == 0)
        {
            xy_transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        payload = (uint8_t *)lwm2m_malloc(payload_length);
        if(!payload)
        {
            xy_transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        payload_length = xy_object_getRegisterPayload(contextP, payload, payload_length);
        if(payload_length == 0)
        {
            xy_transaction_free(transaction);
            lwm2m_free(payload);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        xy_coap_set_payload(transaction->message, payload, payload_length);
    }

    transaction->callback = prv_handleRegistrationUpdateReply;
    transaction->userData = (void *) server;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    if (xy_transaction_send(contextP, transaction) == 0)
    {
        server->status = XY_STATE_REG_UPDATE_PENDING;
    }

    if (withObjects == true)
    {
        lwm2m_free(payload);
    }

    return COAP_NO_ERROR;
}

// update the registration of a given server
int lwm2m_update_registration(lwm2m_context_t *contextP,
                              uint16_t shortServerID,
                              bool withObjects)
{
    xy_lwm2m_server_t *targetP;
    uint8_t result;

    LOG_ARG("State: %s, shortServerID: %d", XY_STR_STATE(contextP->state), shortServerID);

    result = COAP_NO_ERROR;

    targetP = contextP->serverList;
    if (targetP == NULL)
    {
        if (xy_object_getServers(contextP, false) == -1)
        {
            LOG("No server found");
            return COAP_404_NOT_FOUND;
        }
    }
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
        if (shortServerID != 0)
        {
            if (targetP->shortID == shortServerID)
            {
                // found the server, trigger the update transaction
                if (targetP->status == XY_STATE_REGISTERED
                        || targetP->status == XY_STATE_REG_UPDATE_PENDING)
                {
                    if (withObjects == true)
                    {
                        targetP->status = XY_STATE_REG_FULL_UPDATE_NEEDED;
                    }
                    else
                    {
                        targetP->status = XY_STATE_REG_UPDATE_NEEDED;
                    }
                    return COAP_NO_ERROR;
                }
                else if ((targetP->status == XY_STATE_REG_FULL_UPDATE_NEEDED)
                         || (targetP->status == XY_STATE_REG_UPDATE_NEEDED))
                {
                    // if REG (FULL) UPDATE is already set, returns COAP_NO_ERROR
                    if (withObjects == true)
                    {
                        targetP->status = XY_STATE_REG_FULL_UPDATE_NEEDED;
                    }
                    return COAP_NO_ERROR;
                }
                else
                {
                    return COAP_400_BAD_REQUEST;
                }
            }
        }
        else
        {
            if (targetP->status == XY_STATE_REGISTERED
                    || targetP->status == XY_STATE_REG_UPDATE_PENDING)
            {
                if (withObjects == true)
                {
                    targetP->status = XY_STATE_REG_FULL_UPDATE_NEEDED;
                }
                else
                {
                    targetP->status = XY_STATE_REG_UPDATE_NEEDED;
                }
            }
        }
        targetP = targetP->next;
    }

    if (shortServerID != 0
            && targetP == NULL)
    {
        // no server found
        result = COAP_404_NOT_FOUND;
    }

    return result;
}
//serverlist��objectlist�Ѿ���ֵ
uint8_t xy_registration_start(lwm2m_context_t *contextP)
{
    xy_lwm2m_server_t *targetP;
    uint8_t result;

    LOG_ARG("State: %s", XY_STR_STATE(contextP->state));

    result = COAP_NO_ERROR;

    targetP = contextP->serverList;
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
        if (targetP->status == XY_STATE_DEREGISTERED
                || targetP->status == XY_STATE_REG_FAILED)
        {
            result = prv_register(contextP, targetP);
        }
        targetP = targetP->next;
    }

    return result;
}


/*
 * Returns XY_STATE_REG_PENDING if at least one registration is still pending
 * Returns XY_STATE_REGISTERED if no registration is pending and there is at least one server the client is registered to
 * Returns XY_STATE_REG_FAILED if all registration failed.
 */
xy_lwm2m_status_t xy_registration_getStatus(lwm2m_context_t *contextP)
{
    xy_lwm2m_server_t *targetP;
    xy_lwm2m_status_t reg_status;

    LOG_ARG("State: %s", XY_STR_STATE(contextP->state));

    targetP = contextP->serverList;
    reg_status = XY_STATE_REG_FAILED;

    while (targetP != NULL)
    {
        LOG_ARG("targetP->status: %s", XY_STR_STATUS(targetP->status));
        switch (targetP->status)
        {
        case XY_STATE_REGISTERED:
        case XY_STATE_REG_UPDATE_NEEDED:
        case XY_STATE_REG_FULL_UPDATE_NEEDED:
        case XY_STATE_REG_UPDATE_PENDING:
            reg_status = XY_STATE_REGISTERED;
            break;

        case XY_STATE_REG_PENDING:
            reg_status = XY_STATE_REG_PENDING;
            break;

        case XY_STATE_DEREG_PENDING:
            reg_status = XY_STATE_DEREG_PENDING;
            break;

        case XY_STATE_REG_FAILED:
            break;
            
        case XY_STATE_DEREGISTERED:
            reg_status = XY_STATE_DEREGISTERED;
            break;
        
        default:
            break;
        }
        LOG_ARG("reg_status: %s", XY_STR_STATUS(reg_status));

        targetP = targetP->next;
    }

    return reg_status;
}

static void prv_handleDeregistrationReply(lwm2m_transaction_t *transacP,
        void *message)
{
    xy_lwm2m_server_t *targetP;

    (void) message;

    targetP = (xy_lwm2m_server_t *)(transacP->userData);
    if (NULL != targetP)
    {
        if (targetP->status == XY_STATE_DEREG_PENDING)
        {
            if (transacP->ack_received)
            {
                targetP->status = XY_STATE_DEREGISTERED;
                LOG("Deregistration Success!");
            }
            else
                LOG("Deregistration failed!");

            cdp_deregister_flag = 1;
            //delete_lwm2m_task();
            if(cdp_wait_sem != NULL)
                osSemaphoreRelease(cdp_wait_sem);
        }
    }
}

void xy_registration_deregister(lwm2m_context_t *contextP,
                             xy_lwm2m_server_t *serverP)
{
    lwm2m_transaction_t *transaction;

    LOG_ARG("State: %s, serverP->status: %s", XY_STR_STATE(contextP->state), XY_STR_STATUS(serverP->status));

    if (serverP->status == XY_STATE_DEREGISTERED
            || serverP->status == XY_STATE_REG_PENDING
            || serverP->status == XY_STATE_DEREG_PENDING
            || serverP->status == XY_STATE_REG_FAILED
            || serverP->location == NULL)
    {
        return;
    }

    transaction = xy_transaction_new(serverP->sessionH, COAP_DELETE, NULL, NULL, contextP->nextMID++, 4, NULL);
    if (transaction == NULL) return;

    xy_coap_set_header_uri_path(transaction->message, serverP->location);

    transaction->callback = prv_handleDeregistrationReply;
    transaction->userData = (void *) serverP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);
    if (xy_transaction_send(contextP, transaction) == 0)
    {
        serverP->status = XY_STATE_DEREG_PENDING;
    }
}

void xy_registration_reset(lwm2m_context_t *contextP,
                        xy_lwm2m_server_t *serverP)
{
    //xy_registration_deregister(contextP, serverP);
    if (serverP->sessionH != NULL)
    {
        lwm2m_close_connection(serverP->sessionH, contextP->userData);
        serverP->sessionH = NULL;
    }
    serverP->status = XY_STATE_DEREGISTERED;
}

xy_lwm2m_server_t *registration_get_registered_server(lwm2m_context_t *contextP)
{
    xy_lwm2m_server_t *targetP;

    if(NULL == contextP)
    {
        LOG("null pointer");
        return NULL;
    }

    targetP = contextP->serverList;
    while (targetP != NULL)
    {
        if ((XY_STATE_REGISTERED == targetP->status)
                || (XY_STATE_REG_UPDATE_PENDING == targetP->status)
                || (XY_STATE_REG_UPDATE_NEEDED == targetP->status)
                || (XY_STATE_REG_FULL_UPDATE_NEEDED == targetP->status))
        {
            return targetP;
        }
        targetP = targetP->next;
    }

    return NULL;
}

#endif

#ifdef LWM2M_SERVER_MODE
static void prv_freeClientObjectList(lwm2m_client_object_t *objects)
{
    while (objects != NULL)
    {
        lwm2m_client_object_t *objP;

        while (objects->instanceList != NULL)
        {
            lwm2m_list_t *target;

            target = objects->instanceList;
            objects->instanceList = objects->instanceList->next;
            lwm2m_free(target);
        }

        objP = objects;
        objects = objects->next;
        lwm2m_free(objP);
    }
}

static int prv_getParameters(multi_option_t *query,
                             char **nameP,
                             uint32_t *lifetimeP,
                             char **msisdnP,
                             xy_lwm2m_binding_t *bindingP,
                             char **versionP)
{
    *nameP = NULL;
    *lifetimeP = 0;
    *msisdnP = NULL;
    *bindingP = XY_BINDING_UNKNOWN;
    *versionP = NULL;

    while (query != NULL)
    {
        if (lwm2m_strncmp((char *)query->data, QUERY_NAME, QUERY_NAME_LEN) == 0)
        {
            if (*nameP != NULL) goto error;
            if (query->len == QUERY_NAME_LEN) goto error;

            *nameP = (char *)lwm2m_malloc(query->len - QUERY_NAME_LEN + 1);
            if (*nameP != NULL)
            {
                memcpy(*nameP, query->data + QUERY_NAME_LEN, query->len - QUERY_NAME_LEN);
                (*nameP)[query->len - QUERY_NAME_LEN] = 0;
            }
        }
        else if (lwm2m_strncmp((char *)query->data, QUERY_SMS, QUERY_SMS_LEN) == 0)
        {
            if (*msisdnP != NULL) goto error;
            if (query->len == QUERY_SMS_LEN) goto error;

            *msisdnP = (char *)lwm2m_malloc(query->len - QUERY_SMS_LEN + 1);
            if (*msisdnP != NULL)
            {
                memcpy(*msisdnP, query->data + QUERY_SMS_LEN, query->len - QUERY_SMS_LEN);
                (*msisdnP)[query->len - QUERY_SMS_LEN] = 0;
            }
        }
        else if (lwm2m_strncmp((char *)query->data, QUERY_LIFETIME, QUERY_LIFETIME_LEN) == 0)
        {
            int i;

            if (*lifetimeP != 0) goto error;
            if (query->len == QUERY_LIFETIME_LEN) goto error;

            for (i = QUERY_LIFETIME_LEN ; i < query->len ; i++)
            {
                if (query->data[i] < '0' || query->data[i] > '9') goto error;
                *lifetimeP = (*lifetimeP * 10) + (query->data[i] - '0');
            }
        }
        else if (lwm2m_strncmp((char *)query->data, QUERY_VERSION, QUERY_VERSION_LEN) == 0)
        {
            if (*versionP != NULL) goto error;
            if (query->len == QUERY_VERSION_LEN) goto error;

            *versionP = (char *)lwm2m_malloc(query->len - QUERY_VERSION_LEN + 1);
            if (*versionP != NULL)
            {
                memcpy(*versionP, query->data + QUERY_VERSION_LEN, query->len - QUERY_VERSION_LEN);
                (*versionP)[query->len - QUERY_VERSION_LEN] = 0;
            }
        }
        else if (lwm2m_strncmp((char *)query->data, QUERY_BINDING, QUERY_BINDING_LEN) == 0)
        {
            if (*bindingP != XY_BINDING_UNKNOWN) goto error;
            if (query->len == QUERY_BINDING_LEN) goto error;

            *bindingP = xy_utils_stringToBinding(query->data + QUERY_BINDING_LEN, query->len - QUERY_BINDING_LEN);
        }
        query = query->next;
    }

    return 0;

error:
    if (*nameP != NULL) lwm2m_free(*nameP);
    if (*msisdnP != NULL) lwm2m_free(*msisdnP);
    if (*versionP != NULL) lwm2m_free(*versionP);

    return -1;
}

static uint16_t prv_splitLinkAttribute(uint8_t *data,
                                       uint16_t length,
                                       uint16_t *keyStart,
                                       uint16_t *keyLength,
                                       uint16_t *valueStart,
                                       uint16_t *valueLength)
{
    uint16_t index;
    uint16_t end;

    index = 0;
    while (index < length && data[index] == ' ') index++;
    if (index == length) return 0;

    if (data[index] == REG_ATTR_SEPARATOR)
    {
        index++;
    }
    if (index == length) return 0;

    *keyStart = index;

    while (index < length && data[index] != REG_ATTR_EQUALS) index++;
    if (index == *keyStart || index == length) return 0;

    *keyLength = index - *keyStart;

    index++;
    while (index < length && data[index] == ' ') index++;
    if (index == length) return 0;

    *valueStart = index;

    while (index < length && data[index] != REG_ATTR_SEPARATOR) index++;
    end = index;

    index--;
    while (index > *valueStart && data[index] == ' ') index--;
    if (index == *valueStart) return 0;

    *valueLength = index - *valueStart + 1;

    return end;
}

static int prv_parseLinkAttributes(uint8_t *data,
                                   uint16_t length,
                                   bool *supportJSON,
                                   char **altPath)
{
    uint16_t index;
    uint16_t pathStart;
    uint16_t pathLength;
    bool isValid;

    isValid = false;

    // Expecting application/link-format (RFC6690)
    // leading space were removed before. Remove trailing spaces.
    while (length > 0 && data[length - 1] == ' ') length--;

    // strip open tag
    if (length >= 2 && data[0] == REG_URI_START)
    {
        data += 1;
        length -= 1;
    }
    else
    {
        return 0;
    }

    pathStart = 0;
    index = length - 1;
    while (index > 0 && data[index] != REG_URI_END) index--;
    // link attributes are required
    if (index == 0 || index == length - 1) return 0;

    // If there is a preceding /, remove it
    if (data[pathStart] == '/')
    {
        pathStart += 1;
    }
    pathLength = index - pathStart;

    index++;
    if (index >= length || data[index] != REG_ATTR_SEPARATOR) return 0;
    index++;

    while (index < length)
    {
        uint16_t result;
        uint16_t keyStart;
        uint16_t keyLength;
        uint16_t valueStart;
        uint16_t valueLength;

        result = prv_splitLinkAttribute(data + index, length - index, &keyStart, &keyLength, &valueStart, &valueLength);
        if (result == 0) return 0;

        if (keyLength == REG_ATTR_TYPE_KEY_LEN
                && 0 == lwm2m_strncmp(REG_ATTR_TYPE_KEY, (char *)data + index + keyStart, keyLength))
        {
            if (isValid == true) return 0; // declared twice
            if (valueLength != REG_ATTR_TYPE_VALUE_LEN
                    || 0 != lwm2m_strncmp(REG_ATTR_TYPE_VALUE, (char *)data + index + valueStart, valueLength))
            {
                return 0;
            }
            isValid = true;
        }
        else if (keyLength == REG_ATTR_CONTENT_KEY_LEN
                 && 0 == lwm2m_strncmp(REG_ATTR_CONTENT_KEY, (char *)data + index + keyStart, keyLength))
        {
            if (*supportJSON == true) return 0; // declared twice
            if (valueLength == CDP_REG_ATTR_CONTENT_JSON_LEN
                    && 0 == lwm2m_strncmp(CDP_REG_ATTR_CONTENT_JSON, (char *)data + index + valueStart, valueLength))
            {
                *supportJSON = true;
            }
            else
            {
                return 0;
            }
        }
        // else ignore this one

        index += result;
    }

    if (isValid == false) return 0;

    if (pathLength != 0)
    {
        *altPath = (char *)lwm2m_malloc(pathLength + 1);
        if (*altPath == NULL) return 0;
        memcpy(*altPath, data + pathStart, pathLength);
        (*altPath)[pathLength] = 0;
    }

    return 1;
}

static int prv_getId(uint8_t *data,
                     uint16_t length,
                     uint16_t *objId,
                     uint16_t *instanceId)
{
    int value;
    uint16_t limit;

    // Expecting application/link-format (RFC6690)
    // leading space were removed before. Remove trailing spaces.
    while (length > 0 && data[length - 1] == ' ') length--;

    // strip open and close tags
    if (length >= 1 && data[0] == REG_URI_START && data[length - 1] == REG_URI_END)
    {
        data += 1;
        length -= 2;
    }
    else
    {
        return 0;
    }

    // If there is a preceding /, remove it
    if (length >= 1 && data[0] == '/')
    {
        data += 1;
        length -= 1;
    }

    limit = 0;
    while (limit < length && data[limit] != '/') limit++;
    value = xy_uri_getNumber(data, limit);
    if (value < 0 || value >= LWM2M_MAX_ID) return 0;
    *objId = value;

    if (limit < length)
    {
        limit += 1;
        data += limit;
        length -= limit;

        if (length > 0)
        {
            value = xy_uri_getNumber(data, length);
            if (value >= 0 && value < LWM2M_MAX_ID)
            {
                *instanceId = value;
                return 2;
            }
            else
            {
                return 0;
            }
        }
    }

    return 1;
}

static lwm2m_client_object_t *prv_decodeRegisterPayload(uint8_t *payload,
        uint16_t payloadLength,
        bool *supportJSON,
        char **altPath)
{
    uint16_t index;
    lwm2m_client_object_t *objList;
    bool linkAttrFound;

    *altPath = NULL;
    *supportJSON = false;
    objList = NULL;
    linkAttrFound = false;
    index = 0;

    while (index <= payloadLength)
    {
        uint16_t start;
        uint16_t length;
        int result;
        uint16_t id;
        uint16_t instance;

        while (index < payloadLength && payload[index] == ' ') index++;
        if (index == payloadLength) break;

        start = index;
        while (index < payloadLength && payload[index] != REG_DELIMITER) index++;
        length = index - start;

        result = prv_getId(payload + start, length, &id, &instance);
        if (result != 0)
        {
            lwm2m_client_object_t *objectP;

            objectP = (lwm2m_client_object_t *)lwm2m_list_find((lwm2m_list_t *)objList, id);
            if (objectP == NULL)
            {
                objectP = (lwm2m_client_object_t *)lwm2m_malloc(sizeof(lwm2m_client_object_t));
                if (objectP == NULL) goto error;
                memset(objectP, 0, sizeof(lwm2m_client_object_t));
                objectP->id = id;
                objList = (lwm2m_client_object_t *)LWM2M_LIST_ADD(objList, objectP);
            }
            if (result == 2)
            {
                lwm2m_list_t *instanceP;

                instanceP = lwm2m_list_find(objectP->instanceList, instance);
                if (instanceP == NULL)
                {
                    instanceP = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
                    if(instanceP == NULL) goto error;
                    memset(instanceP, 0, sizeof(lwm2m_list_t));
                    instanceP->id = instance;
                    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, instanceP);
                }
            }
        }
        else if (linkAttrFound == false)
        {
            result = prv_parseLinkAttributes(payload + start, length, supportJSON, altPath);
            if (result == 0) goto error;

            linkAttrFound = true;
        }
        else goto error;

        index++;
    }

    return objList;

error:
    if (*altPath != NULL)
    {
        lwm2m_free(*altPath);
        *altPath = NULL;
    }
    prv_freeClientObjectList(objList);

    return NULL;
}

static lwm2m_client_t *prv_getClientByName(lwm2m_context_t *contextP,
        char *name)
{
    lwm2m_client_t *targetP;

    targetP = contextP->clientList;
    while (targetP != NULL && strcmp(name, targetP->name) != 0)
    {
        targetP = targetP->next;
    }

    return targetP;
}

void registration_freeClient(lwm2m_client_t *clientP)
{
    LOG("Entering");
    if (clientP->name != NULL) lwm2m_free(clientP->name);
    if (clientP->msisdn != NULL) lwm2m_free(clientP->msisdn);
    if (clientP->altPath != NULL) lwm2m_free(clientP->altPath);
    prv_freeClientObjectList(clientP->objectList);
    while(clientP->observationList != NULL)
    {
        lwm2m_observation_t *targetP;

        targetP = clientP->observationList;
        clientP->observationList = clientP->observationList->next;
        lwm2m_free(targetP);
    }
    lwm2m_free(clientP);
}

static int prv_getLocationString(uint16_t id,
                                 char location[MAX_LOCATION_LENGTH])
{
    int index;
    int result;

    memset(location, 0, MAX_LOCATION_LENGTH);

    result = xy_utils_stringCopy(location, MAX_LOCATION_LENGTH, "/"URI_REGISTRATION_SEGMENT"/");
    if (result < 0) return 0;
    index = result;

    result = xy_utils_intToText(id, (uint8_t *)location + index, MAX_LOCATION_LENGTH - index);
    if (result == 0) return 0;

    return index + result;
}

uint8_t registration_handleRequest(lwm2m_context_t *contextP,
                                   lwm2m_uri_t *uriP,
                                   void *fromSessionH,
                                   coap_packet_t *message,
                                   coap_packet_t *response)
{
    uint8_t result;
    time_t tv_sec;

    LOG_URI(uriP);
    tv_sec = lwm2m_gettime();
    if (tv_sec < 0) return COAP_500_INTERNAL_SERVER_ERROR;

    switch(message->code)
    {
    case COAP_POST:
    {
        char *name = NULL;
        uint32_t lifetime;
        char *msisdn;
        char *altPath;
        char *version;
        xy_lwm2m_binding_t binding;
        lwm2m_client_object_t *objects;
        bool supportJSON;
        lwm2m_client_t *clientP;
        char location[MAX_LOCATION_LENGTH];

        if (0 != prv_getParameters(message->uri_query, &name, &lifetime, &msisdn, &binding, &version))
        {
            return COAP_400_BAD_REQUEST;
        }
        if (message->content_type != (coap_content_type_t)XY_LWM2M_CONTENT_LINK
                && message->content_type != (coap_content_type_t)XY_LWM2M_CONTENT_TEXT)
        {
            return COAP_400_BAD_REQUEST;
        }

        objects = prv_decodeRegisterPayload(message->payload, message->payload_len, &supportJSON, &altPath);

        switch (uriP->flag & LWM2M_URI_MASK_ID)
        {
        case 0:
            // Register operation
            // Version is mandatory
            if (version == NULL)
            {
                if (name != NULL) lwm2m_free(name);
                if (msisdn != NULL) lwm2m_free(msisdn);
                return COAP_400_BAD_REQUEST;
            }
            // Endpoint client name is mandatory
            if (name == NULL)
            {
                lwm2m_free(version);
                if (msisdn != NULL) lwm2m_free(msisdn);
                return COAP_400_BAD_REQUEST;
            }
            // Object list is mandatory
            if (objects == NULL)
            {
                lwm2m_free(version);
                lwm2m_free(name);
                if (msisdn != NULL) lwm2m_free(msisdn);
                return COAP_400_BAD_REQUEST;
            }
            // version must be 1.0
            if (strlen(version) != LWM2M_VERSION_LEN
                    || lwm2m_strncmp(version, LWM2M_VERSION, LWM2M_VERSION_LEN))
            {
                lwm2m_free(version);
                lwm2m_free(name);
                if (msisdn != NULL) lwm2m_free(msisdn);
                return COAP_412_PRECONDITION_FAILED;
            }

            if (lifetime == 0)
            {
                lifetime = LWM2M_DEFAULT_LIFETIME;
            }

            clientP = prv_getClientByName(contextP, name);
            if (clientP != NULL)
            {
                // we reset this registration
                lwm2m_free(clientP->name);
                if (clientP->msisdn != NULL) lwm2m_free(clientP->msisdn);
                if (clientP->altPath != NULL) lwm2m_free(clientP->altPath);
                prv_freeClientObjectList(clientP->objectList);
                clientP->objectList = NULL;
            }
            else
            {
                clientP = (lwm2m_client_t *)lwm2m_malloc(sizeof(lwm2m_client_t));
                if (clientP == NULL)
                {
                    lwm2m_free(name);
                    lwm2m_free(altPath);
                    if (msisdn != NULL) lwm2m_free(msisdn);
                    prv_freeClientObjectList(objects);
                    return COAP_500_INTERNAL_SERVER_ERROR;
                }
                memset(clientP, 0, sizeof(lwm2m_client_t));
                clientP->internalID = lwm2m_list_newId((lwm2m_list_t *)contextP->clientList);
                contextP->clientList = (lwm2m_client_t *)LWM2M_LIST_ADD(contextP->clientList, clientP);
            }
            clientP->name = name;
            clientP->binding = binding;
            clientP->msisdn = msisdn;
            clientP->altPath = altPath;
            clientP->supportJSON = supportJSON;
            clientP->lifetime = lifetime;
            clientP->endOfLife = tv_sec + lifetime;
            clientP->objectList = objects;
            clientP->sessionH = fromSessionH;

            if (prv_getLocationString(clientP->internalID, location) == 0)
            {
                xy_registration_freeClient(clientP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }
            if (xy_coap_set_header_location_path(response, location) == 0)
            {
                xy_registration_freeClient(clientP);
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            if (contextP->monitorCallback != NULL)
            {
                contextP->monitorCallback(clientP->internalID, NULL, COAP_201_CREATED, XY_LWM2M_CONTENT_TEXT, NULL, 0, contextP->monitorUserData);
            }
            result = COAP_201_CREATED;
            break;

        case LWM2M_URI_FLAG_OBJECT_ID:
            clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, uriP->objectId);
            if (clientP == NULL) return COAP_404_NOT_FOUND;

            // Endpoint client name MUST NOT be present
            if (name != NULL)
            {
                lwm2m_free(name);
                if (msisdn != NULL) lwm2m_free(msisdn);
                return COAP_400_BAD_REQUEST;
            }

            if (binding != XY_BINDING_UNKNOWN)
            {
                clientP->binding = binding;
            }
            if (msisdn != NULL)
            {
                if (clientP->msisdn != NULL) lwm2m_free(clientP->msisdn);
                clientP->msisdn = msisdn;
            }
            if (lifetime != 0)
            {
                clientP->lifetime = lifetime;
            }
            // client IP address, port or MSISDN may have changed
            clientP->sessionH = fromSessionH;

            if (objects != NULL)
            {
                lwm2m_observation_t *observationP;

                // remove observations on object/instance no longer existing
                observationP = clientP->observationList;
                while (observationP != NULL)
                {
                    lwm2m_client_object_t *objP;
                    lwm2m_observation_t *nextP;

                    nextP = observationP->next;

                    objP = (lwm2m_client_object_t *)lwm2m_list_find((lwm2m_list_t *)objects, observationP->uri.objectId);
                    if (objP == NULL)
                    {
                        observationP->callback(clientP->internalID,
                                               &observationP->uri,
                                               COAP_202_DELETED,
                                               XY_LWM2M_CONTENT_TEXT, NULL, 0,
                                               observationP->userData);
                        observe_remove(observationP);
                    }
                    else
                    {
                        if ((observationP->uri.flag & LWM2M_URI_FLAG_INSTANCE_ID) != 0)
                        {
                            if (lwm2m_list_find((lwm2m_list_t *)objP->instanceList, observationP->uri.instanceId) == NULL)
                            {
                                observationP->callback(clientP->internalID,
                                                       &observationP->uri,
                                                       COAP_202_DELETED,
                                                       XY_LWM2M_CONTENT_TEXT, NULL, 0,
                                                       observationP->userData);
                                observe_remove(observationP);
                            }
                        }
                    }

                    observationP = nextP;
                }

                prv_freeClientObjectList(clientP->objectList);
                clientP->objectList = objects;
            }

            clientP->endOfLife = tv_sec + clientP->lifetime;

            if (contextP->monitorCallback != NULL)
            {
                contextP->monitorCallback(clientP->internalID, NULL, COAP_204_CHANGED, XY_LWM2M_CONTENT_TEXT, NULL, 0, contextP->monitorUserData);
            }
            result = COAP_204_CHANGED;
            break;

        default:
            return COAP_400_BAD_REQUEST;
        }
    }
    break;

    case COAP_DELETE:
    {
        lwm2m_client_t *clientP;

        if ((uriP->flag & LWM2M_URI_MASK_ID) != LWM2M_URI_FLAG_OBJECT_ID) return COAP_400_BAD_REQUEST;

        contextP->clientList = (lwm2m_client_t *)LWM2M_LIST_RM(contextP->clientList, uriP->objectId, &clientP);
        if (clientP == NULL) return COAP_400_BAD_REQUEST;
        if (contextP->monitorCallback != NULL)
        {
            contextP->monitorCallback(clientP->internalID, NULL, COAP_202_DELETED, XY_LWM2M_CONTENT_TEXT, NULL, 0, contextP->monitorUserData);
        }
        xy_registration_freeClient(clientP);
        result = COAP_202_DELETED;
    }
    break;

    default:
        return COAP_400_BAD_REQUEST;
    }

    return result;
}

void lwm2m_set_monitoring_callback(lwm2m_context_t *contextP,
                                   lwm2m_result_callback_t callback,
                                   void *userData)
{
    LOG("Entering");
    contextP->monitorCallback = callback;
    contextP->monitorUserData = userData;
}
#endif

extern int cdp_register_fail_flag;
// for each server update the registration if needed
// for each client check if the registration expired
void xy_registration_step(lwm2m_context_t *contextP,
                       time_t currentTime,
                       time_t *timeoutP)
{
#ifdef LWM2M_CLIENT_MODE
    xy_lwm2m_server_t *targetP = contextP->serverList;

    LOG_ARG("contextP State: %s", XY_STR_STATE(contextP->state));

    targetP = contextP->serverList;
    while (targetP != NULL)
    {
        LOG_ARG("targetP Status: %s", XY_STR_STATUS(targetP->status));
        switch (targetP->status)
        {
        case XY_STATE_REGISTERED:
        {
            time_t nextUpdate;
            int32_t  interval;

            nextUpdate = targetP->lifetime;

            if (CLOUD_LIFETIME_DELTA(targetP->lifetime) < nextUpdate)
            {
                nextUpdate = CLOUD_LIFETIME_DELTA(targetP->lifetime);
            }
            else
            {
                nextUpdate = nextUpdate >> 1;
            }

            interval = (int32_t)(targetP->registration + nextUpdate) - (int32_t)currentTime;
            if (0 >= interval)
            {
                LOG("Updating registration");
                prv_updateRegistration(contextP, targetP, false);
            }
            else if (interval < *timeoutP)
            {
                *timeoutP = interval;
            }

        }
        break;

        case XY_STATE_REG_UPDATE_NEEDED:
            prv_updateRegistration(contextP, targetP, false);
            break;

        case XY_STATE_REG_FULL_UPDATE_NEEDED:
            prv_updateRegistration(contextP, targetP, true);
            break;

        case XY_STATE_REG_FAILED:
            cdp_register_fail_flag = 1;
            break;

        default:
            break;
        }
        targetP = targetP->next;
    }

#endif
#ifdef LWM2M_SERVER_MODE
    lwm2m_client_t *clientP;

    LOG("Entering");
    // monitor clients lifetime
    clientP = contextP->clientList;
    while (clientP != NULL)
    {
        lwm2m_client_t *nextP = clientP->next;

        if (clientP->endOfLife <= currentTime)
        {
            contextP->clientList = (lwm2m_client_t *)LWM2M_LIST_RM(contextP->clientList, clientP->internalID, NULL);
            if (contextP->monitorCallback != NULL)
            {
                contextP->monitorCallback(clientP->internalID, NULL, COAP_202_DELETED, XY_LWM2M_CONTENT_TEXT, NULL, 0, contextP->monitorUserData);
            }
            xy_registration_freeClient(clientP);
        }
        else
        {
            time_t interval;

            interval = clientP->endOfLife - currentTime;

            if (*timeoutP > interval)
            {
                *timeoutP = interval;
            }
        }
        clientP = nextP;
    }
#endif

}
#endif
