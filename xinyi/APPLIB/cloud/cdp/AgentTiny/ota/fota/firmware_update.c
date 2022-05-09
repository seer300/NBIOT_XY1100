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
#include "softap_macro.h"
#include "xy_utils.h"
#if TELECOM_VER
#include "internals.h"
#include "atiny_socket.h"
#include "connection.h"
#include "agent_tiny_demo.h"
#include "agenttiny.h"
#include "atiny_log.h"
#include "package.h"
#include "firmware_update.h"
#include "flag_manager.h"
#include "factory_nv.h"
#define FW_BLOCK_SIZE (512)

#define COAP_PROTO_PREFIX "coap://"
#define COAPS_PROTO_PREFIX "coaps://"

static char *g_ota_uri = NULL;
static pack_storage_device_api_s *g_fota_storage_device = NULL;
fw_update_record_t g_fw_update_record = {0};

firmware_update_notify g_firmware_update_notify = NULL;
void *g_firmware_update_notify_param = NULL;

int *firmware_download_ctx = NULL;
int *server_ctx = NULL;
extern int g_FOTAing_flag;

int start_new_req(lwm2m_transaction_t *transacP);

static void firmware_download_reply(lwm2m_transaction_t *transacP,
                                    void *message)
{
    coap_packet_t *packet = (coap_packet_t *)message;
    lwm2m_context_t *contextP = (lwm2m_context_t *)(transacP->userData);
    //lwm2m_transaction_t *transaction;
    uint32_t len = 0;
    uint32_t block_num = 0;
    uint8_t block2_more = 0;
    uint16_t block_size = FW_BLOCK_SIZE;
    uint32_t block_offset = 0;
    int ret = 0;
    lwm2m_observe_info_t observe_info;

    if(NULL == message)
    {
        ATINY_LOG(LOG_INFO, "transaction timeout");
        ret = FIRMWARE_DOWNING_TIMEOUT;
        goto failed_exit;
    }

    if(1 != xy_coap_get_header_block2(message, &block_num, &block2_more, &block_size, &block_offset))
    {
        ATINY_LOG(LOG_ERR, "xy_coap_get_header_block2 failed");
        ret = FIRMWARE_DOWNING_URI_ERR;
        goto failed_exit;
    }
    ATINY_LOG(LOG_ERR, "block_num : %lu, block2_more : %lu, block_offset : %lu, payload_len is %u",
              block_num, (uint32_t)block2_more, block_offset, packet->payload_len);

    if(0 == block_num)
    {
        g_fw_update_record.in_use = 1;
    }

    g_fw_update_record.block_size = block_size;
    g_fw_update_record.block_num = block_num;
    g_fw_update_record.block_offset = block_offset;
    g_fw_update_record.block_more = block2_more;

	xy_printf("[CDP]size:%d,num:%d,offset:%d,more:%d", block_size, block_num, block_offset, block2_more);
    len = (uint32_t)(packet->payload_len);
    if(g_fota_storage_device && g_fota_storage_device->write_software)
    {
        ret = g_fota_storage_device->write_software(g_fota_storage_device, block_offset, packet->payload, len);
        if(ret != 0)
        {
            ret = FIRMWARE_DOWNING_FILE_ERR;
            goto failed_exit;
        }
    }
    else if(NULL == g_fota_storage_device)
        ATINY_LOG(LOG_ERR, "g_fota_storage_device NULL");
    else
        ATINY_LOG(LOG_ERR, "g_fota_storage_device->write_software NULL");

    len = block_offset + (uint32_t)(packet->payload_len);



    if(block2_more)
    {
    	//not last pkt, start new quest
        if(start_new_req(transacP) != 0)
        {
            ATINY_LOG(LOG_ERR, "start new fota request failed!!!");
            ret = FIRMWARE_DOWNING_TIMEOUT;
            goto failed_exit;
        }
    }
    else
    {				
        ret = ATINY_ERR;
        if(g_fota_storage_device && g_fota_storage_device->write_software_end)
        {
            ret = g_fota_storage_device->write_software_end(g_fota_storage_device, PACK_DOWNLOAD_OK, len);
            if(ret != 0)
            {
                ret = FIRMWARE_DOWNING_CHECK_ERR;
                goto failed_exit;
            }
        }
        else if(NULL == g_fota_storage_device)
            ATINY_LOG(LOG_ERR, "g_fota_storage_device NULL");
        else
            ATINY_LOG(LOG_ERR, "g_fota_storage_device->write_software_end NULL");

        if((firmware_download_ctx != NULL) && (firmware_download_ctx != server_ctx))
        {
            xy_lwm2m_server_t *server = registration_get_registered_server(contextP);
            connection_t * conp= (connection_t *)server->sessionH;
            conp->net_context = server_ctx;
        }
		g_FOTAing_flag = 0;
		
        if(g_firmware_update_notify)
            g_firmware_update_notify(FIRMWARE_UPDATE_RST_SUCCESS, g_firmware_update_notify_param);
        ATINY_LOG(LOG_ERR, "download success");

    }
    return;
    
failed_exit:
	clean_firmware_record();
    if(g_fota_storage_device && g_fota_storage_device->write_software_end)
        (void)g_fota_storage_device->write_software_end(g_fota_storage_device, PACK_DOWNLOAD_FAIL, len);
    else if(NULL == g_fota_storage_device)
        ATINY_LOG(LOG_ERR, "g_fota_storage_device NULL");
    else
        ATINY_LOG(LOG_ERR, "g_fota_storage_device->write_software_end NULL");
    ATINY_LOG(LOG_INFO, "g_firmware_update_notify FIRMWARE_UPDATE_RST_FAILED");
    if((firmware_download_ctx != NULL) && (firmware_download_ctx != server_ctx))
    {
        xy_lwm2m_server_t *server = registration_get_registered_server(contextP);
        connection_t * conp= (connection_t *)server->sessionH;
        conp->net_context = server_ctx;
    }
	g_FOTAing_flag = 0;
    if(g_firmware_update_notify)
        g_firmware_update_notify(ret, g_firmware_update_notify_param);
    return;
}

//Make new request to downloading fota pkt
int start_new_req(lwm2m_transaction_t *transacP)
{
    int ret = 0;

    lwm2m_transaction_t *transaction = NULL;
    lwm2m_context_t *contextP = (lwm2m_context_t *)(transacP->userData);

    transaction = xy_transaction_new(transacP->peerH, COAP_GET, NULL, NULL, contextP->nextMID++, 4, NULL);
    if(!transaction)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_new failed");
        return -1;
    }

    ret = xy_coap_set_header_uri_path(transaction->message, g_ota_uri);
    if(ret < 0 || NULL == transaction->message->uri_path)
    {
        xy_transaction_free(transaction);
        return -1;
    }
    //xy_coap_set_header_uri_query(transaction->message, query);
    ret = xy_coap_set_header_block2(transaction->message, g_fw_update_record.block_num + 1, 0, g_fw_update_record.block_size);
    if(ret < 0)
    {
        xy_transaction_free(transaction);
        return -1;
    }

    transaction->callback = firmware_download_reply;
    transaction->userData = (void *)contextP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    if(xy_transaction_send(contextP, transaction) != 0)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_send failed");
        return -1;
    }

	return 0;
}

static int record_fw_uri(char *uri, int uri_len)
{
    if(!uri || *uri == '\0' || uri_len <= 0)
    {
        return -1;
    }

    g_fw_update_record.uri = (char *)lwm2m_malloc(uri_len + 1);
    if(NULL == g_fw_update_record.uri)
    {
        ATINY_LOG(LOG_ERR, "lwm2m_malloc failed");
        return -1;
    }
    memcpy(g_fw_update_record.uri, uri, uri_len);
    g_fw_update_record.uri[uri_len] = '\0';
    g_fw_update_record.uri_len = uri_len;

    return 0;
}

static int update_uri_info(char *uri, int uri_len, unsigned char *update_flag)
{
    int ret = -1;

    if(!uri || *uri == '\0' || uri_len <= 0 || !update_flag)
    {
        return -1;
    }

    if(1 == g_fw_update_record.in_use && NULL != g_fw_update_record.uri && 0 == memcmp(g_fw_update_record.uri, uri, uri_len))
    {
        *update_flag = 0;
        return 0;
    }
    else
    {
        if(NULL != g_fw_update_record.uri)
        {
            lwm2m_free(g_fw_update_record.uri);
            g_fw_update_record.uri = NULL;
        }
        memset(&g_fw_update_record, 0x0, sizeof(g_fw_update_record));
        ret = record_fw_uri(uri, uri_len);
        *update_flag = 1;
        if(0 != ret)
        {
            ATINY_LOG(LOG_ERR, "record_fw_uri failed");
            return -1;
        }
        return 0;
    }
}

void set_firmware_update_notify(firmware_update_notify notify_cb, void *param)
{
    g_firmware_update_notify = notify_cb;
    g_firmware_update_notify_param = param;

    return;
}

int parse_firmware_uri(char *uri, int uri_len, char *parsed_host, char *parsed_port)
{
    char *char_p, *path;
    char /**host,*/ *port;
    int path_len, proto_len;
    //char *buf = NULL;
    
    if(!uri || *uri == '\0' || uri_len <= 0)
    {
        return -1;
    }

    if(0 == strncmp(uri, COAP_PROTO_PREFIX, strlen(COAP_PROTO_PREFIX)))
    {
        proto_len = strlen(COAP_PROTO_PREFIX);
        //鏆備笉鑰冭檻query
        char_p = uri + proto_len;
        if(*char_p == '\0') // eg. just "coap://"
            return -1;
        path = strchr(char_p, '/');
        if(NULL == path)
            return -1;
        
        port = strchr(char_p, ':');
        if(NULL == port)
        {
            memcpy(parsed_port, "5683", strlen("5683"));
            memcpy(parsed_host, char_p, path-char_p);
        }
        else
        {
            memcpy(parsed_port, port+1, path-port-1);
            memcpy(parsed_host, char_p, port-char_p);
        }
        
    }
    else if(0 == strncmp(uri, COAPS_PROTO_PREFIX, strlen(COAPS_PROTO_PREFIX)))
    {
        proto_len = strlen(COAPS_PROTO_PREFIX);
        //鏆備笉鑰冭檻query
        char_p = uri + proto_len;
        if(*char_p == '\0') // eg. just "coap://"
            return -1;
        path = strchr(char_p, '/');
        if(NULL == path)
            return -1;
        
        port = strchr(char_p, ':');
        if(NULL == port)
        {
            memcpy(parsed_port, "5684", strlen("5684"));
            memcpy(parsed_host, char_p, path-char_p);
        }
        else
        {
            memcpy(parsed_port, port+1, path-port-1);
            memcpy(parsed_host, char_p, port-char_p);
        }

    }
    else
    {
        ATINY_LOG(LOG_ERR, "unsupported proto");
        return -1;
    }

    xy_printf("[CDP] fota host:%s, port:%s", parsed_host, parsed_port);
    path_len = uri_len - (path - uri);

    g_ota_uri = (char *)lwm2m_malloc(path_len + 1);
    if(!g_ota_uri)
    {
        ATINY_LOG(LOG_ERR, "lwm2m_malloc failed");
        return -1;
    }
    /*lint -esym(668,memcpy) */
    memcpy(g_ota_uri, path, path_len);
    g_ota_uri[path_len] = '\0';

    return 0;
}


int start_firmware_download(lwm2m_context_t *contextP, char *uri,
                            pack_storage_device_api_s *storage_device_p)
{
    lwm2m_transaction_t *transaction;
    unsigned char update_flag = 0;
    char fota_host[16] = {0};
    char fota_port[6] = {0};
    int ret = -1;
    int uri_len;
    xy_lwm2m_server_t *server;

    if(!contextP || !uri || *uri == '\0' || !storage_device_p)
    {
        ATINY_LOG(LOG_ERR, "invalid params");
        return -1;
    }
    ATINY_LOG(LOG_ERR, "start download");
    g_fota_storage_device = storage_device_p;
    uri_len = strlen(uri);
    server = registration_get_registered_server(contextP);
    if(NULL == server)
    {
        ATINY_LOG(LOG_ERR, "registration_get_registered_server failed");
        return -1;
    }

    ret = update_uri_info(uri, uri_len, &update_flag);
    if(0 != ret)
    {
        ;// continue to although update_uri_info failed
    }

    ATINY_LOG(LOG_DEBUG, "update_flag = %u", update_flag);
    if(!(0 == update_flag && NULL != g_ota_uri))
    {
        if(NULL != g_ota_uri)
        {
            lwm2m_free(g_ota_uri);
            g_ota_uri = NULL;
        }
        ret = parse_firmware_uri(uri, uri_len, fota_host, fota_port);
        if(0 != ret)
        {
            ATINY_LOG(LOG_ERR, "parse_firmware_uri failed");
            return -1;
        }
    }

    
    connection_t * conp= (connection_t *)server->sessionH;
    server_ctx = conp->net_context;
	if(strcmp(g_softap_fac_nv->cloud_server_ip, fota_host) || g_softap_fac_nv->cloud_server_port != (int)strtol(fota_port,NULL,10))
    {   
        ATINY_LOG(LOG_INFO, "firmware_download info:%s:%s", fota_host, fota_port);
        if(firmware_download_ctx == NULL)
    	{
        	firmware_download_ctx = atiny_net_connect(fota_host, fota_port, ATINY_PROTO_UDP);
			if(firmware_download_ctx == NULL)
				return -1;
    	}
        conp->net_context = firmware_download_ctx;
    }

    
    transaction = xy_transaction_new(server->sessionH, COAP_GET, NULL, NULL, contextP->nextMID++, 4, NULL);
    if(!transaction)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_new failed");
        return -1;
    }
    ret = xy_coap_set_header_uri_path(transaction->message, g_ota_uri);
    if(ret < 0 || NULL == transaction->message->uri_path)
    {
        xy_transaction_free(transaction);
        return -1;
    }
    //xy_coap_set_header_uri_query(transaction->message, query);
    if(1 == g_fw_update_record.in_use)
    {
        ret = xy_coap_set_header_block2(transaction->message, g_fw_update_record.block_num + 1, 0, g_fw_update_record.block_size);
    }
    else
    {
        ret = xy_coap_set_header_block2(transaction->message, 0, 0, FW_BLOCK_SIZE);
    }
    if(ret != 1)
    {
        xy_transaction_free(transaction);
        return -1;
    }

    transaction->callback = firmware_download_reply;
    transaction->userData = (void *)contextP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    if(xy_transaction_send(contextP, transaction) != 0)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_send failed");
        return -1;
    }

    return 0;
}

void clean_firmware_record(void)
{
    if(NULL != g_ota_uri)
    {
        lwm2m_free(g_ota_uri);
        g_ota_uri = NULL;
    }
    
    if(NULL != g_fw_update_record.uri)
    {
        lwm2m_free(g_fw_update_record.uri);
        g_fw_update_record.uri = NULL;
    }
    memset(&g_fw_update_record, 0x00, sizeof(g_fw_update_record));
}
#endif
