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
#if TELECOM_VER
#include "xy_utils.h"
#include "at_ps_cmd.h"
#include "agent_tiny_demo.h"
#include "liblwm2m.h"
#include "object_comm.h"
#include "cdp_backup.h"
#include "atiny_fota_state.h"
#include "atiny_context.h"
#include "at_cdp.h"
#include "softap_nv.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "low_power.h"
#include "net_app_resume.h"
#include "xy_proxy.h"
#include "xy_cdp_api.h"
#include "xy_net_api.h"
#include "lwip/ip_addr.h"
#include "xy_atc_interface.h"
#include "atiny_socket.h"
#include "agenttiny.h"
#if defined WITH_AT_FRAMEWORK
#include "at_frame/xy_at_api.h"
#endif


#define DEFAULT_SERVER_IPV4 "49.4.85.232"/* OceanConnect */

extern osSemaphoreId_t  cdp_recovery_sem;
extern osSemaphoreId_t  cdp_wait_sem;
extern osSemaphoreId_t cdp_api_sendasyn_sem;
extern osMutexId_t g_observe_mutex;
extern osMutexId_t g_transaction_mutex;

#if !VER_QUCTL260
extern int g_send_status;
#endif
extern cdp_fota_info_t* g_fota_info;
extern uint8_t* g_endpointName;
extern atiny_fota_manager_s *g_fota_manager;

//CDP全局信息结构体指针
handle_data_t *g_phandle = NULL;
//设置CDP的lifetime的全局变量，可考虑优化去除
int cdp_lifetime = 0;
//static atiny_device_info_t g_device_info;
//static atiny_param_t g_atiny_params;

upstream_info_t *upstream_info = NULL;
downstream_info_t *downstream_info = NULL;

osMutexId_t g_upstream_mutex = NULL;
osMutexId_t g_downstream_mutex = NULL;

osThreadId_t g_lwm2m_TskHandle = NULL;
osThreadId_t g_lwm2m_recv_TskHandle = NULL;

int cdp_deregister_flag = 0;
int cdp_register_fail_flag = 0;

//AT指令发送数据时携带的全局seq_num,将该值带入到cdp发送接口之前
static uint8_t g_seq_num;
//AT指令发送数据时携带的全局rai类型,将该值带入到cdp发送接口之前
static uint8_t  g_raiflag;
//AT+QLWULDATAEX指令特有标志位，用于区别其他发送指令发送con包时的状态上报
static uint8_t  g_conEX_send_flag = 0;
//AT+QLWULDATAEX指令特有标志位，用于发送con时的状态上报携带的当前seq_num值
static uint8_t  g_cur_seq_num = 0;

int domain_check(char *domain)
{
	if((domain[0] <= 'z'&&domain[0] >= 'a')
		||(domain[0] <= 'Z' && domain[0] >= 'A'))
		return XY_OK;
	return XY_ERR;
}

void cdp_con_flag_init(int type, uint16_t seq_num)
{
	if(type != cdp_NON && type != cdp_NON_RAI && type != cdp_NON_WAIT_REPLY_RAI)
	{
		cdp_set_cur_seq_num(seq_num);
		g_conEX_send_flag = 1;
	}
}

void cdp_con_flag_deint()
{
	g_conEX_send_flag = 0;
}

int cdp_get_con_send_flag()
{
	return g_conEX_send_flag;
}


void cdp_set_seq_and_rai(uint8_t raiflag,uint8_t seq_num)
{
	g_raiflag = raiflag;
	g_seq_num = seq_num;
}


void cdp_get_seq_and_rai(uint8_t* raiflag,uint8_t* seq_num)
{
	*raiflag =  g_raiflag;
	*seq_num =  g_seq_num;
}

uint8_t cdp_get_seq_num()
{
	return g_seq_num;
}

void cdp_set_cur_seq_num(uint8_t seq_num)
{
	g_cur_seq_num = seq_num;
}
uint8_t cdp_get_cur_seq_num()
{
	return g_cur_seq_num;
}

void cdp_QLWULDATASTATUS_report(uint8_t seq_num)
{
	if(cdp_get_con_send_flag())
	{
#if VER_QUECTEL
		char *rsp_cmd = xy_zalloc(40);
		if(!seq_num)
			sprintf(rsp_cmd, "\r\n+QLWULDATASTATUS:%d\r\n",g_send_status);
		else
			sprintf(rsp_cmd, "\r\n+QLWULDATASTATUS:%d,%d\r\n",g_send_status,seq_num);
		send_urc_to_ext(rsp_cmd);
		xy_free(rsp_cmd);
#endif
	cdp_con_flag_deint();
	}
}



//用于设置当前lwm2m事件值，便于AT+QLWEVTIND状态查询
void cdp_set_report_event(xy_module_type_t type,const char *arg, int code)
{
	switch(type)
	{
		case MODULE_LWM2M:
		{
			if(code == XY_STATE_REGISTERED)
			{
				g_softap_var_nv->cdp_lwm2m_event_status = 0;//注册完成
			}
			else if(code == XY_STATE_UPDATE_DONE)
				g_softap_var_nv->cdp_lwm2m_event_status = 2;//更新完成
			else if(code == XY_STATE_BS_PENDING)
				g_softap_var_nv->cdp_lwm2m_event_status = 4;//Bootstrap完成
			else if(code == XY_RECV_UPDATE_PKG_URL_NEEDED)
				g_softap_var_nv->cdp_lwm2m_event_status = 6;//通知设备接收更新包url
			else if(code == XY_DOWNLOAD_COMPLETED)
				g_softap_var_nv->cdp_lwm2m_event_status = 7;//通知设备下载完成
#if VER_QUECTEL || VER_QUCTL260
			else if(code == XY_STATE_DEREGISTERED)
			{
				g_softap_var_nv->cdp_lwm2m_event_status = 1;//去注册完成
			}
			else if(code == XY_STATE_REG_FAILED)
			{
				if(g_softap_var_nv->cdp_lwm2m_event_status != 10) //服务器拒绝
					g_softap_var_nv->cdp_lwm2m_event_status = 8;//注册失败
			}
#else
			else
			{
				g_softap_var_nv->cdp_lwm2m_event_status = 255;//未初始化
			}
#endif
			break;
		}
		
		case MODULE_URI:
		{
			if(code == OBSERVE_SUBSCRIBE)
			{
				if(dm_isUriOpaqueHandle((lwm2m_uri_t *)arg))
					g_softap_var_nv->cdp_lwm2m_event_status = 3;//订阅对象19/0/0完成
				else if (((lwm2m_uri_t*)arg)->objectId == 5 && ((lwm2m_uri_t*)arg)->instanceId == 0 && ((lwm2m_uri_t*)arg)->resourceId == 3)
					g_softap_var_nv->cdp_lwm2m_event_status = 5;//5/0/3资源订阅完成
			}
			else if(code == OBSERVE_UNSUBSCRIBE)
			{
				g_softap_var_nv->cdp_lwm2m_event_status = 9; //取消订阅对象19/0/0
			}
			break;
		}
		default:
			break;
	}	
}


//检查当前observe 是否包含/19/0/0
int app_data_report_check()
{
	lwm2m_observed_t *targetP = NULL;
	lwm2m_context_t *contextP = NULL;
	int observe19 = XY_ERR;
	
	
	if(g_phandle != NULL)
	{
		contextP = g_phandle->lwm2m_context;
		if(contextP == NULL)
			return XY_ERR;
		
		for (targetP = contextP->observedList ; targetP != NULL ; targetP = targetP->next)
		{
			if (!dm_isUriOpaqueHandle(&(targetP->uri)))
				continue;
			observe19 = XY_OK;
		}
		return observe19;
	}
	return observe19;
}


void cdp_softap_var_nv_init()
{
	//cdp事件上报和nnmi 默认置1
	g_softap_var_nv->cdp_lwm2m_event_status = 255;
#if VER_QUECTEL || VER_QUCTL260
	g_softap_var_nv->cdp_nnmi = 1;
#endif
}



char *get_cdp_server_ip_addr_str()
{
    return (char *)g_softap_fac_nv->cloud_server_ip;
}

int set_cdp_server_ip_addr_str(char *ip_addr_str)
{
	ip_addr_t ipaddr = {0};
	uint8_t ip_tpye = xy_get_netifIpType();
	unsigned int ip6addr = 0;
	
	if(ipaddr_aton(ip_addr_str, &ipaddr) == 1) // 1:ip,0:domain
	{
	    if(	(xy_ipaddr_check(ip_addr_str, IPV4_TYPE) && xy_ipaddr_check(ip_addr_str, IPV6_TYPE))
			||(ip_tpye == IPV4_TYPE && ipaddr.type != IPADDR_TYPE_V4)
			|| (ip_tpye == IPV6_TYPE && ipaddr.type != IPADDR_TYPE_V6))
    	{
			return XY_ERR;
    	}
	}
	else
	{
		if(domain_check(ip_addr_str) == XY_ERR)
			return XY_ERR;
	}
	
	if(strcmp(g_softap_fac_nv->cloud_server_ip,ip_addr_str))
	{	
		memset(g_softap_fac_nv->cloud_server_ip, 0x00, sizeof(g_softap_fac_nv->cloud_server_ip));
		memcpy(g_softap_fac_nv->cloud_server_ip, ip_addr_str, strlen(ip_addr_str));
		SAVE_FAC_PARAM(cloud_server_ip);
	}    
    return XY_OK;
}

u16_t get_cdp_server_port()
{
    return g_softap_fac_nv->cloud_server_port;
}

int set_cdp_server_port(u16_t port)
{
    if(g_softap_fac_nv->cloud_server_port != port)
    {
        g_softap_fac_nv->cloud_server_port = port;
        SAVE_FAC_PARAM(cloud_server_port);
    }
    return 0;
}


int set_cdp_server_settings(char *ip_addr_str, u16_t port)
{
    //todo: check if we are allowed to configure cdp server settings
    if(ip_addr_str != NULL && set_cdp_server_ip_addr_str(ip_addr_str) < 0)
    	return XY_ERR;

    if (set_cdp_server_port(port) < 0)
        return XY_ERR;   

    return XY_OK;
}


int reconfigure_cdp()
{
    /*if (memcmp(cdp_server_settings.ip_addr_str, g_softap_fac_nv->cloud_server_ip, 16) == 0 && 
        cdp_server_settings.port == g_softap_fac_nv->cloud_server_port)
    {
        //delete_lwm2m_task();
    }
    else
    {
        delete_lwm2m_task();

        memcpy(cdp_server_settings.ip_addr_str, g_softap_fac_nv->cloud_server_ip, 16);
        cdp_server_settings.port = g_softap_fac_nv->cloud_server_port;

        //creat_lwm2m_task();
    }*/

    return 0;
}

int is_cdp_running()
{
    if (cdp_handle_exist() && g_phandle != NULL
        && g_phandle->lwm2m_context->state > STATE_REGISTERING)
        return 1;
    else
        return 0;
}


// cdp relative, send message via lwm2m 19/0/0
int send_message_via_lwm2m(char *data, int data_len, cdp_msg_type_e type, uint8_t seq_num)
{
    buffer_list_t *node = NULL;
    if (data == NULL || data_len == 0)
    {
        //upstream_info->sent_num++;
        return 0;
    }

    osMutexAcquire(g_upstream_mutex, portMAX_DELAY);

    if(upstream_info->pending_num >= DATALIST_MAX_LEN)
        goto failed;

    node = (buffer_list_t*)xy_malloc(sizeof(buffer_list_t));
    if (node == NULL)
    {
        goto failed;
    }
	memset(node, 0x00, sizeof(buffer_list_t));
	
    

    node->data = (char*)xy_malloc(data_len);
    if (node->data == NULL)
    {
        goto failed;
    }
    memcpy(node->data, data, data_len);
    node->data_len = data_len;
    node->type = type;
	node->seq_num = seq_num;
    if (upstream_info->tail == NULL)
    {
        upstream_info->head = node;
        upstream_info->tail = node;
    }
    else
    {
        upstream_info->tail->next = node;
        upstream_info->tail = node;
    }
    
    if(type == cdp_NON || type == cdp_NON_RAI || type == cdp_NON_WAIT_REPLY_RAI)
    {
        if(seq_num == 0)
    	{
        	upstream_info->sent_num++;
    	}
		else
			upstream_info->pending_num++;
    }
	else
	{
        upstream_info->pending_num++;
#if VER_QUCTL260
		g_softap_var_nv->g_send_status = 1;
#else
		g_send_status = 1;
#endif
	}
    osMutexRelease(g_upstream_mutex);
    if(cdp_wait_sem != NULL)
        osSemaphoreRelease(cdp_wait_sem);  

    return 0;

failed:
    if (node != NULL)
    {
        if (node->data != NULL)
        {
            xy_free(node->data);
        }
        xy_free(node);
    }
    upstream_info->error_num++;
    osMutexRelease(g_upstream_mutex);
    return -1;
}

typedef struct stream_info_t
{
	int datalen;
	char data[1024] 
}stream_info;

//return the oldest buffered data, data length returned by output argument
char *get_message_via_lwm2m(int *data_len)
{
    buffer_list_t *node;
    char *data = NULL;
#if VER_QUCTL260
	int len = 0;
	char *buffer = NULL;
	if(downstream_info == NULL && NET_NEED_RECOVERY(CDP_TASK)) //走恢复
	{
		downstream_info = xy_malloc(sizeof(downstream_info_t));
		memset(downstream_info, 0, sizeof(downstream_info_t));

		if(g_downstream_mutex == NULL)
    		cloud_mutex_create(&g_downstream_mutex);

		osMutexAcquire(g_downstream_mutex, portMAX_DELAY);
		//从flash读取缓存数据并挂到链表上
		if(cdp_downbuffered_resume() != XY_OK)
		{
			osMutexRelease(g_downstream_mutex);
			return NULL;
		}
		
		//读节点
		if (downstream_info->buffered_num > 0)
	    {
	        node = downstream_info->head;
	        downstream_info->head = downstream_info->head->next;
	        downstream_info->buffered_num--;
			downstream_info->buffered_total -= node->data_len; //缓存已读出，需要减掉
	        if (downstream_info->buffered_num == 0)
	        {
	            downstream_info->tail = NULL;
	        }
	        *data_len = node->data_len;
	        data = node->data;
	        xy_free(node);
			osMutexRelease(g_downstream_mutex);
	        return data;
	    }
		osMutexRelease(g_downstream_mutex);
	}
	else if(downstream_info != NULL)
	{
		osMutexAcquire(g_downstream_mutex, portMAX_DELAY);
	    if (downstream_info->buffered_num > 0)
	    {
	        node = downstream_info->head;
	        downstream_info->head = downstream_info->head->next;
	        downstream_info->buffered_num--;
			downstream_info->buffered_total -= node->data_len; //缓存已读出，需要减掉
	        if (downstream_info->buffered_num == 0)
	        {
	            downstream_info->tail = NULL;
	        }
	        *data_len = node->data_len;
	        data = node->data;
	        xy_free(node);
	        osMutexRelease(g_downstream_mutex);
	        return data;
	    }
	    osMutexRelease(g_downstream_mutex);
	}
#else
	if(downstream_info == NULL)
		return NULL;
	osMutexAcquire(g_downstream_mutex, portMAX_DELAY);
	if (downstream_info->buffered_num > 0)
	{
		node = downstream_info->head;
		downstream_info->head = downstream_info->head->next;
		downstream_info->buffered_num--;
		if (downstream_info->buffered_num == 0)
		{
			downstream_info->tail = NULL;
		}
		*data_len = node->data_len;
		data = node->data;
		xy_free(node);
		osMutexRelease(g_downstream_mutex);
		return data;
	}
	osMutexRelease(g_downstream_mutex);
#endif
    return NULL;
}

extern int xy_Remote_AT_Req(char *req_at);
extern char *g_Remote_AT_Rsp;

void cdp_downbuffered_clear()
{
	if((cdp_downbuffered_num_resume() == XY_OK) && (downstream_info->received_num > 0))
	{
		//业务退出，清空flash
		xy_printf("erase dirty data\r\n");
		xy_flash_erase(FOTA_BACKUP_BASE, DOWNSTREAM_LEN_MAX*8); 	
	}
}

int cdp_downbuffered_num_resume()
{
	if(downstream_info == NULL)
	{
		downstream_info = xy_malloc(sizeof(downstream_info_t));
		memset(downstream_info, 0, sizeof(downstream_info_t));
	}
	xy_flash_read(FOTA_BACKUP_BASE, &downstream_info->buffered_num, 4*sizeof(int));
	if(downstream_info->buffered_num > DATALIST_MAX_LEN)//防止数据读取异常
	{
		xy_printf("downstream read flash error num = %d",downstream_info->buffered_num);
		return XY_ERR;
	}
	return XY_OK;
}

int cdp_downbuffered_resume()
{
	int len = 0;
	int buffer_num = 0;
	buffer_list_t *node = NULL;
	//取出缓存数量
	if(cdp_downbuffered_num_resume() != XY_OK)
		return XY_ERR;
	
	//循环读
	stream_info *down_stream_info_read = xy_malloc(DATALIST_MAX_LEN *sizeof(stream_info));
	memset(down_stream_info_read, 0, DATALIST_MAX_LEN * sizeof(stream_info));
	
	for(int i = 0; i < downstream_info->buffered_num; i++) //downstream_info->buffered_num = 5
	{
		if(i == 0)
			len = 0;
		else
			len += down_stream_info_read[i-1].datalen + sizeof(int); //长度为上一次节点的长度(datalen和data)

		xy_flash_read(FOTA_BACKUP_BASE + 4*sizeof(int) + len , &down_stream_info_read[i].datalen, sizeof(int)); //data_len
		xy_flash_read(FOTA_BACKUP_BASE + 4*sizeof(int) +sizeof(int) + len, down_stream_info_read[i].data, down_stream_info_read[i].datalen); //data
	}
	//节点挂到链表上
	buffer_num = downstream_info->buffered_num;
	for(int i = 0; i < buffer_num; i++)
	{
		node = (buffer_list_t*)xy_malloc(sizeof(buffer_list_t));
        memset(node, 0x00, sizeof(buffer_list_t));
        node->data = (char*)xy_malloc(down_stream_info_read[i].datalen);
		node->data_len = down_stream_info_read[i].datalen;
        memset(node->data, 0x00, node->data_len);
        memcpy(node->data, down_stream_info_read[i].data, node->data_len);
       
        if (downstream_info->tail == NULL)
        {
            downstream_info->head = node;
            downstream_info->tail = node;
        }
        else
        {
            downstream_info->tail->next = node;
            downstream_info->tail = node;  
        }
	}
	xy_free(down_stream_info_read);
	return XY_OK;
}


void cdp_downbuffered_save()
{
	/*
	if(g_phandle->lwm2m_context->state != STATE_READY)
		return;
	*/
	
	buffer_list_t* node = NULL;
	int buffered_num = 0;
	int len = 0;
	if(downstream_info != NULL && downstream_info->received_num > 0) //收到数据了才开始写
	{
		xy_flash_erase(FOTA_BACKUP_BASE, DOWNSTREAM_LEN_MAX*8); //写之前先擦一下，防止有脏数据
		xy_flash_write(FOTA_BACKUP_BASE, &downstream_info->buffered_num, 4*sizeof(int));
		
		//取节点
		buffered_num = downstream_info->buffered_num;
		stream_info *down_stream_info = xy_malloc(DATALIST_MAX_LEN*sizeof(stream_info));
		memset(down_stream_info, 0x00, DATALIST_MAX_LEN*sizeof(stream_info));
		while(downstream_info->buffered_num > 0)
	    {
	       	node = downstream_info->head;
	        downstream_info->head = downstream_info->head->next;
	        down_stream_info[downstream_info->buffered_num-1].datalen = node->data_len;
	        memcpy(down_stream_info[downstream_info->buffered_num-1].data, node->data, node->data_len);
	        xy_free(node);
			downstream_info->buffered_num--;
	        if (downstream_info->buffered_num == 0)
	        {
	            downstream_info->tail = NULL;
	        }    
	    }
	
		//循环写flash
		for(int i = 0; i < buffered_num; i++)
		{
			if(i == 0)
				len = 0;
			else
				len += (down_stream_info[buffered_num-1-(i-1)].datalen + sizeof(int));
			xy_flash_write(FOTA_BACKUP_BASE + 4*sizeof(int) + len, &down_stream_info[buffered_num-1-i].datalen, (down_stream_info[buffered_num-1-i].datalen + sizeof(int)));
		}
		xy_free(down_stream_info);
		
		}
	
}

int new_message_indication(char *data, int data_len)
{
    buffer_list_t *node = NULL;
    char *rsp_cmd = NULL;
    char *data_copy = NULL;

    //If opencpu is used, the data is not cached and is output directly through the hook function
    //opencpu与AT隔离
    if(g_cdp_downstream_callback != NULL)
    {
    	g_cdp_downstream_callback(data, data_len);
		return 0;
    }

    osMutexAcquire(g_downstream_mutex, portMAX_DELAY);
    downstream_info->received_num++;
    data_copy = xy_zalloc(data_len + 1);
    if(data_copy == NULL)
    {
    	goto failed;
    }
    memcpy(data_copy, data, data_len);
    xy_Remote_AT_Req(data_copy);
	xy_free(data_copy);
	if(g_Remote_AT_Rsp != NULL)
	{
		send_message_via_lwm2m(g_Remote_AT_Rsp, strlen(g_Remote_AT_Rsp), 1,0);
		xy_free(g_Remote_AT_Rsp);
		g_Remote_AT_Rsp = NULL;
	}

    if (g_softap_var_nv->cdp_nnmi == 0 || g_softap_var_nv->cdp_nnmi == 2) // 0:no indications, buffer the data, 2:indications only
    {

#if VER_QUCTL260
		//记录缓存总数
    	downstream_info->buffered_total += data_len;
#endif
        if(downstream_info->buffered_num < DATALIST_MAX_LEN
#if VER_QUCTL260
		&& downstream_info->buffered_total <= DOWNSTREAM_BUFFER_TOTAL
#endif
		)
    	{
    		node = (buffer_list_t*)xy_malloc(sizeof(buffer_list_t));
	        if (node == NULL)
	        {
	            goto failed;
	        }
	        memset(node, 0x00, sizeof(buffer_list_t));
	        node->data = (char*)xy_malloc(data_len);
	        if (node->data == NULL)
	        {
	            goto failed;
	        }
	        memset(node->data, 0x00, data_len);
	        memcpy(node->data, data, data_len);
	        node->data_len = data_len;
	        if (downstream_info->tail == NULL)
	        {
	            downstream_info->head = node;
	            downstream_info->tail = node;
	        }
	        else
	        {
	            downstream_info->tail->next = node;
	            downstream_info->tail = node;
	        }
	        downstream_info->buffered_num++;
			
#if VER_QUCTL260
			if (g_softap_var_nv->cdp_nnmi == 2)
	        {
	            rsp_cmd = (char*)xy_zalloc(30);
				if (rsp_cmd != NULL)
	            {
	                snprintf(rsp_cmd, 30, "\r\n+NNMI\r\n");
	                send_rsp_str_to_ext(rsp_cmd);
	            }
	        }
#endif
    	}
		else
		{
		
#if VER_QUCTL260
	            rsp_cmd = (char*)xy_zalloc(30);
				snprintf(rsp_cmd, 30, "\r\n+NNMI: \"recv\",buff full\r\n");
				send_rsp_str_to_ext(rsp_cmd);
				downstream_info->buffered_total -= data_len; //丢弃以后，不属于缓存，需要减掉
#endif
			downstream_info->dropped_num++;
		}

#if !VER_QUCTL260
		if (g_softap_var_nv->cdp_nnmi == 2)
        {
            rsp_cmd = (char*)xy_zalloc(16);
            if (rsp_cmd != NULL)
            {
                snprintf(rsp_cmd, 16, "\r\n+NNMI\r\n");
                send_rsp_str_to_ext(rsp_cmd);
            }
        }
#endif
    }
    else if (g_softap_var_nv->cdp_nnmi == 1)
    {
        rsp_cmd = (char*)xy_zalloc(data_len*2+28);
        if (rsp_cmd == NULL)
        {
            //OutputTraceMessage(1, "malloc failed!");
            goto failed;
        }
        snprintf(rsp_cmd, 128, "\r\n+NNMI: %d,",data_len);
        bytes2hexstr(data, data_len, rsp_cmd + strlen(rsp_cmd), data_len*2+1);
        strcat(rsp_cmd + strlen(rsp_cmd), "\r\n");
        send_rsp_str_to_ext(rsp_cmd);
    }


    if (rsp_cmd != NULL)
	{
		xy_free(rsp_cmd);
		rsp_cmd = NULL;
	}

    osMutexRelease(g_downstream_mutex);
    return 0;
    
failed:
    if (node != NULL)
    {
        if (node->data != NULL)
        {
            xy_free(node->data);
            node->data = NULL;
        }
        xy_free(node);
        node = NULL;
    }
    if (rsp_cmd != NULL)
	{
		xy_free(rsp_cmd);
		rsp_cmd = NULL;
	}
    osMutexRelease(g_downstream_mutex);
    return -1;
}

int cdp_update_proc(xy_lwm2m_server_t *targetP)
{
	if(targetP->status != XY_STATE_REGISTERED)
		return XY_ERR;

	osMutexAcquire(g_transaction_mutex, portMAX_DELAY);
	targetP->status = XY_STATE_REG_UPDATE_NEEDED;
	osMutexRelease(g_transaction_mutex);

	if(cdp_wait_sem != NULL)
	{
        osSemaphoreRelease(cdp_wait_sem);  
	} 

    return 0;
}

int get_upstream_message_pending_num()
{
    return upstream_info->pending_num;
}

int get_upstream_message_sent_num()
{
    return upstream_info->sent_num;
}

int get_upstream_message_error_num()
{
    return upstream_info->error_num;
}

void get_downstram_info(downstream_info_t *downstream_info_temp)
{
	if(CDP_NEED_RECOVERY(g_softap_var_nv->cdp_lwm2m_event_status))
	{	
		if(xy_flash_read(FOTA_BACKUP_BASE,downstream_info_temp, sizeof(downstream_info_t)) !=XY_OK)
			memset(downstream_info_temp, 0x00, sizeof(downstream_info_t));
	}
	else
	{
		xy_flash_erase(FOTA_BACKUP_BASE, DOWNSTREAM_LEN_MAX*8);
		memset(downstream_info_temp, 0x00, sizeof(downstream_info_t));
	}
}
int get_downstream_message_buffered_num()
{
	int buffered_num = 0;
#if VER_QUCTL260
	if(!CDP_NEED_RECOVERY(g_softap_var_nv->cdp_lwm2m_event_status))
		return buffered_num;
	
	if(downstream_info == NULL)
	{
		if(NET_NEED_RECOVERY(CDP_TASK))
		{
			downstream_info_t *downstream_info_temp = xy_malloc(sizeof(downstream_info_t));
			get_downstram_info(downstream_info_temp);
			buffered_num = (downstream_info_temp->buffered_num > 0) ? downstream_info_temp->buffered_num : 0;
			xy_free(downstream_info_temp);
		}
	}
	else
#endif
		buffered_num = downstream_info->buffered_num;

    return buffered_num;
}

int get_downstream_message_received_num()
{
	int received_num = 0;
#if VER_QUCTL260
	if(!CDP_NEED_RECOVERY(g_softap_var_nv->cdp_lwm2m_event_status))
		return received_num;
	
	if(downstream_info == NULL)
	{
		if(NET_NEED_RECOVERY(CDP_TASK))
		{
			downstream_info_t *downstream_info_temp = xy_malloc(sizeof(downstream_info_t));
			get_downstram_info(downstream_info_temp);
			received_num = (downstream_info_temp->received_num > 0) ? downstream_info_temp->received_num : 0;
			xy_free(downstream_info_temp);
		}
	}
	else
#endif
		received_num = downstream_info->received_num;
    return received_num;
}

int get_downstream_message_dropped_num()
{
	int dropped_num = 0;
#if VER_QUCTL260
	if(!CDP_NEED_RECOVERY(g_softap_var_nv->cdp_lwm2m_event_status))
		return dropped_num;
	
	if(downstream_info == NULL)
	{
		if(NET_NEED_RECOVERY(CDP_TASK))
		{
			downstream_info_t *downstream_info_temp = xy_malloc(sizeof(downstream_info_t));
			get_downstram_info(downstream_info_temp);
			dropped_num = (downstream_info_temp->dropped_num > 0) ? downstream_info_temp->dropped_num : 0;
			xy_free(downstream_info_temp);
		}
	}
	else
#endif
        //20230217 MG add: avoid dropped_num minus
		dropped_num = (downstream_info->dropped_num > 0) ? downstream_info->dropped_num : 0;

    return dropped_num;
}

//检测从flash里面读取是否为全FF值
int check_flash_readbuf(char* buf, unsigned int len)
{
	int i = len;

	while(i--)
	{
		if(buf[i] != 0xFF)
			return XY_OK;
	}

	return XY_ERR;
}

void fota_autoreg_info_save()
{
	int ret = -1;
	ret = cdp_bak_regInfo(g_phandle->lwm2m_context);
    if (ret != CLOUD_SAVE_NEED_WRITE)
    {
        return ret;
    }

	if(g_softap_fac_nv->cdp_dtls_switch == 1)
	{
		//存放pskid,DTLS需要用到pskid
		memcpy(g_cdp_regInfo->pskid, g_softap_fac_nv->cdp_pskid, 16);
		//存放PSK
		memcpy(g_cdp_regInfo->psk, (char *)g_softap_fac_nv->cloud_server_auth, 16);
		g_cdp_regInfo->nat_type = g_softap_fac_nv->cdp_dtls_nat_type;
	}

	memset(g_cdp_regInfo->net_info.remote_ip,0x0,40);
    memcpy(g_cdp_regInfo->net_info.remote_ip,(char *)g_softap_fac_nv->cloud_server_ip,40);
	
	xy_flash_write(FOTA_STATEINFO_FLASH_BASE + sizeof(cdp_fota_info_t),g_cdp_regInfo, sizeof(cdp_regInfo_t));
    softap_printf(USER_LOG, WARN_LOG, "fota_autoreg_info_save, over");
}

int fota_autoreg_info_recover()
{
	if(g_cdp_regInfo == NULL)
		g_cdp_regInfo = xy_malloc(sizeof(cdp_regInfo_t));
	
	if(xy_flash_read(FOTA_STATEINFO_FLASH_BASE + sizeof(cdp_fota_info_t), (unsigned char *)g_cdp_regInfo, sizeof(cdp_regInfo_t))!= XY_OK)
	{
		memset(g_cdp_regInfo, 0x00, sizeof(cdp_regInfo_t));
		return XY_ERR;
	}

	g_softap_fac_nv->cdp_lifetime = g_cdp_regInfo->lifetime;
	
	if(strlen(g_cdp_regInfo->net_info.remote_ip) != 0 && check_flash_readbuf(g_cdp_regInfo->net_info.remote_ip, strlen(g_cdp_regInfo->net_info.remote_ip)) == XY_OK)
    	set_cdp_server_settings(g_cdp_regInfo->net_info.remote_ip, g_cdp_regInfo->net_info.remote_port);

	//检测是否为dtls模式，设置dtls标志位
	if(strlen(g_cdp_regInfo->psk) != 0 && check_flash_readbuf(g_cdp_regInfo->psk, strlen(g_cdp_regInfo->psk)) == XY_OK)
	{
		g_softap_fac_nv->cdp_dtls_switch = 1;
		SAVE_FAC_PARAM(cdp_dtls_switch);
		memcpy(g_softap_fac_nv->cdp_pskid,g_cdp_regInfo->pskid,16);
		SAVE_FAC_PARAM(cdp_pskid);
		memcpy(g_softap_fac_nv->cloud_server_auth,g_cdp_regInfo->psk,16);
		SAVE_FAC_PARAM(cloud_server_auth);
		g_softap_fac_nv->cdp_dtls_nat_type = g_cdp_regInfo->nat_type;
		SAVE_FAC_PARAM(cdp_dtls_nat_type);
	}

	//只使用info，不走恢复流程
	g_cdp_regInfo->resumeFlag = 0;

	creat_lwm2m_task();
    softap_printf(USER_LOG, WARN_LOG, "fota_autoreg_info_recover, over");
    return XY_OK;
}

int cdp_fota_autoreg_check()
{
	if(g_fota_info != NULL)
	{
		if(g_fota_info->fota_upgrade_info.upgrade_state == OTA_SUCCEED || g_fota_info->fota_upgrade_info.upgrade_state == OTA_FAILED)
			return XY_OK;
	}
	return XY_ERR;
}

void cdp_netif_event_callback(PsNetifStateChangeEvent event)
{
	osThreadAttr_t thread_attr = {0};

    softap_printf(USER_LOG, WARN_LOG, "cdp_netif_event_callback, netif up");

    if(cdp_fota_autoreg_check() == XY_OK && !cdp_handle_exist() &&
        get_sys_up_stat() != UTC_WAKEUP && get_sys_up_stat() != EXTPIN_WAKEUP && g_cdp_resume_TskHandle == NULL)
	{
        if(fota_autoreg_info_recover() != XY_OK)
    	{
    		softap_printf(USER_LOG, WARN_LOG, "fota_autoreg_info_recover failed");
        	return;
    	}
	}


    //20230310 MG
	if((/*get_sys_up_stat() == UTC_WAKEUP ||*/ get_sys_up_stat() == EXTPIN_WAKEUP) && CDP_NEED_RECOVERY(g_softap_var_nv->cdp_lwm2m_event_status))
	{
		static uint8_t register_notify = 0;
		if(!register_notify)
		{
			atiny_event_notify(ATINY_BC26_RECOVERY, NULL, 0);
			register_notify = 1;
		}
		return;
	}


    //自注册模式
    if(g_softap_fac_nv->cdp_register_mode == 1 && !cdp_handle_exist() &&
        get_sys_up_stat() != UTC_WAKEUP && get_sys_up_stat() != EXTPIN_WAKEUP && g_cdp_resume_TskHandle == NULL)
    {
        thread_attr.name	   = "cdp_attach_resume";
        thread_attr.priority   = XY_OS_PRIO_NORMAL1;
        thread_attr.stack_size = 0x400;
        g_cdp_resume_TskHandle = osThreadNew((osThreadFunc_t)(cdp_attach_resume_process), NULL, &thread_attr);
        return;
    }
#if VER_QUECTEL
	else if(g_softap_fac_nv->cdp_register_mode == 0 && !cdp_handle_exist() &&
        get_sys_up_stat() != UTC_WAKEUP && get_sys_up_stat() != EXTPIN_WAKEUP && g_cdp_resume_TskHandle == NULL)
	{
		static uint8_t register_notify = 0;
		if(!register_notify)
		{
			atiny_event_notify(ATINY_REGISTER_NOTIFY, NULL, 0);
			register_notify = 1;
		}
		return;
	}
#endif
	else if(g_softap_fac_nv->cdp_register_mode == 2)
		return;

    //保活模式，IP变化触发update
    if(g_cdp_resume_TskHandle == NULL && g_softap_fac_nv->save_cloud && (NET_NEED_RECOVERY(CDP_TASK) || is_cdp_running()))
    {
        thread_attr.name	   = "cdp_netif_up_resume";
        thread_attr.priority   = XY_OS_PRIO_NORMAL1;
        thread_attr.stack_size = 0x400;
        g_cdp_resume_TskHandle = osThreadNew((osThreadFunc_t)(cdp_netif_up_resume_process), NULL, &thread_attr);
    }
}

int cdp_init()
{
	cdp_clear();
	
    if (g_softap_fac_nv->cloud_server_port > 65535)
    {
        goto failed;
    }
    if ((strcmp((const char *)g_softap_fac_nv->cloud_server_ip, "") == 0 )
        || g_softap_fac_nv->cloud_server_port == 0)
    {
        goto failed;
    }

	if(g_softap_fac_nv->cdp_dtls_switch == 1 && (!strcmp((const char *)g_softap_fac_nv->cdp_pskid, "") 
		|| !strcmp((const char *)g_softap_fac_nv->cloud_server_auth, "")))
		goto failed;
    
	if(upstream_info == NULL)
	{
    	upstream_info = (upstream_info_t*)xy_malloc(sizeof(upstream_info_t));
    	memset(upstream_info, 0, sizeof(upstream_info_t));
	}

	if(downstream_info == NULL)
	{
		downstream_info = (downstream_info_t*)xy_malloc(sizeof(downstream_info_t));
		memset(downstream_info, 0, sizeof(downstream_info_t));
	}

	if(g_upstream_mutex == NULL)
    	cloud_mutex_create(&g_upstream_mutex);
	
	if(g_downstream_mutex == NULL)
    	cloud_mutex_create(&g_downstream_mutex);
	
	if(g_observe_mutex == NULL)
    	cloud_mutex_create(&g_observe_mutex);

	if(g_transaction_mutex == NULL)
    	cloud_mutex_create(&g_transaction_mutex);

    if(cdp_api_sendasyn_sem == NULL)
        cdp_api_sendasyn_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	if(cdp_wait_sem == NULL)
        cdp_wait_sem = osSemaphoreNew(0xFFFF, 0, NULL);

#if !VER_QUECTEL && !VER_QUCTL260
	g_send_status = NOT_SENT;
#endif

//20230220 MG cancle the cdp downbuffered resume, +NMGS will resume former data
//and look like not read out the total data
#if VER_QUCTL260
	//当需要恢复的时候需要把缓存链表恢复，再此基础上继续挂链表
	if(NET_NEED_RECOVERY(CDP_TASK))
	{
		osMutexAcquire(g_downstream_mutex, portMAX_DELAY);
		//从flash读取缓存数据并挂到链表上
		if(cdp_downbuffered_resume() != XY_OK)
			memset(downstream_info, 0, sizeof(downstream_info_t));
		osMutexRelease(g_downstream_mutex);
	}

#endif

    return 0;

failed:
    cdp_clear();
    return -1;
}

void cdp_stream_clear(uint32_t *mutex, void *stream)
{
    buffer_list_t *node;

    if(stream == NULL || *mutex == NULL)
    	return;

    osMutexAcquire(*mutex, portMAX_DELAY);
    while(((upstream_info_t *)stream)->head != NULL)
    {
        node = ((upstream_info_t *)stream)->head;
        ((upstream_info_t *)stream)->head = ((upstream_info_t *)stream)->head->next;
        if(node->data_len > 0)
            xy_free(node->data);
        xy_free(node);
        if (((upstream_info_t *)stream)->head == NULL)
        {
            ((upstream_info_t *)stream)->tail = NULL;
        }
    }
    osMutexRelease(*mutex);
}


void cdp_downstream_clear(uint32_t *mutex, void *stream)
{
    buffer_list_t *node;

    if(stream == NULL || *mutex == NULL)
    	return;

    osMutexAcquire(*mutex, portMAX_DELAY);
    while(((downstream_info_t *)stream)->head != NULL)
    {
        node = ((downstream_info_t *)stream)->head;
        ((downstream_info_t *)stream)->head = ((downstream_info_t *)stream)->head->next;
        if(node->data_len > 0)
            xy_free(node->data);
        xy_free(node);
        if (((downstream_info_t *)stream)->head == NULL)
        {
            ((downstream_info_t *)stream)->tail = NULL;
        }
    }
    osMutexRelease(*mutex);
}


void cdp_clear()
{
    if (upstream_info != NULL)
	{
		cdp_stream_clear(&g_upstream_mutex, (void*)upstream_info);
#if VER_QUECTEL
		upstream_info ->pending_num = 0;
#else
		xy_free(upstream_info);
		upstream_info = NULL;
#endif
	}   

#if !VER_QUECTEL
    if (downstream_info != NULL)
	{
		cdp_downstream_clear(&g_downstream_mutex, (void*)downstream_info);
		xy_free(downstream_info);
		downstream_info = NULL;
	}
#endif
        
    if (g_upstream_mutex != NULL)
    {
        osMutexDelete(g_upstream_mutex);
        g_upstream_mutex = NULL;
    }

#if !VER_QUECTEL
    if (g_downstream_mutex != NULL)
    {
        osMutexDelete(g_downstream_mutex);
        g_downstream_mutex = NULL;
    }
#endif

    if (g_observe_mutex != NULL)
    {
        osMutexDelete(g_observe_mutex);
        g_observe_mutex = NULL;
    }

    if (g_transaction_mutex != NULL)
    {
        osMutexDelete(g_transaction_mutex);
        g_transaction_mutex = NULL;
    }
}

void ack_callback(atiny_report_type_e type, int cookie, data_send_status_e status, int mid)
{
    atiny_printf("type:%d cookie:%d status:%d\n", type,cookie, status);
	
	//seq_callback 回调时带seq_num con包需要过滤掉
	if(mid == -1 && (cdp_check_con_node(cookie) == XY_OK))
		return;
		
	
	if(NULL != g_cdp_send_asyn_ack && mid != -1)
		g_cdp_send_asyn_ack(mid, status);

	osMutexAcquire(g_upstream_mutex, portMAX_DELAY);
	if (status == SENT_SUCCESS)
	{
		if(mid != -1) //过滤掉NON 带seq_num 的上报
		{
#if VER_QUCTL260
			if(g_softap_var_nv->g_send_status == 5)
#else
			if(g_send_status == 5) //5(平台主动取消订阅/19/0/0)
#endif
			{
#if VER_QUECTEL
				at_send_NSMI(3,cookie);
#endif
				upstream_info->sent_num--;//
				upstream_info->error_num++;
			}
			else
			{
#if VER_QUCTL260
				g_softap_var_nv->g_send_status = 4;
#else
				g_send_status = 4;
#endif
				//是为了区别SENT和空口上报只能选其一,不带seq_num上报SENT,带seq_num走seq_callback上报
				if(cookie == 0)
					at_send_NSMI(0,cookie);
			}
		}
		
		upstream_info->sent_num++;
		upstream_info->pending_num--;
	}
	else if (status == SENT_TIME_OUT)
	{
		if(mid != -1) //过滤掉NON 带seq_num 的上报
		{
#if VER_QUCTL260
			g_softap_var_nv->g_send_status = 3;
#else
			g_send_status = 3;
#endif
			at_send_NSMI(1,cookie);
		}
	
		upstream_info->error_num++;
		upstream_info->pending_num--;
	}
	else if (status == SENT_FAIL)
	{
		if(mid != -1) //过滤掉NON 带seq_num 的上报
		{
#if VER_QUCTL260
			g_softap_var_nv->g_send_status = 2;
#else
			g_send_status = 2;
#endif
			at_send_NSMI(2,cookie);
		}
		
		upstream_info->error_num++;
		upstream_info->pending_num--;
	}
	else if (status == SENT_GET_RST)
	{
		if(mid != -1) //过滤掉NON 带seq_num 的上报
		{
			//是为了区别SENT和空口上报只能选其一,不带seq_num上报SENT,带seq_num走seq_callback上报
#if VER_QUECTEL
			at_send_NSMI(3,cookie);
#endif
		}
		
		upstream_info->error_num++;
		upstream_info->pending_num--;
	}
	cdp_QLWULDATASTATUS_report(cookie); //EX命令的主动上报
	osMutexRelease(g_upstream_mutex);

}

//带seq_num发送到空口的回调
void seq_callback(unsigned long eventId, void *param, int paramLen)
{
	xy_assert(paramLen == sizeof(ATC_MSG_IPSN_IND_STRU));
	if(g_phandle == NULL)
		return;
	
	ATC_MSG_IPSN_IND_STRU *ipsn_urc = (ATC_MSG_IPSN_IND_STRU*)xy_malloc(sizeof(ATC_MSG_IPSN_IND_STRU));
	memcpy(ipsn_urc, param, paramLen);

	unsigned short seq_soc_num = 0;
	char send_status = 0;
	unsigned short socket_fd = 0;
	unsigned short seq_num = 0;
	int soc_ctx_id  = -1;

	seq_soc_num = ipsn_urc->usIpSn;
	send_status = ipsn_urc->ucStatus;
	xy_free(ipsn_urc);

	socket_fd = (unsigned short)((seq_soc_num & 0XFF00) >> 8);
	seq_num = (unsigned short)(seq_soc_num & 0X00FF);
	
	
	if (seq_num > SEQUENCE_MAX || seq_num <= 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "seq_callback unexpected seq_num:%d",seq_num);
		return;
	}

	if (cdp_find_match_udp_node(seq_num) == XY_ERR)
	{
		softap_printf(USER_LOG, WARN_LOG, "find no match udp mode!!!");
		return;
	}

	if(g_softap_fac_nv->cdp_dtls_switch == 1)
	{
		handle_data_t *handle = (handle_data_t *)g_phandle;
		connection_t  *connList = handle->client_data.connList;
		if(connList == NULL)
			return;
		
		mbedtls_ssl_context *ssl = (mbedtls_ssl_context *)connList->net_context;
		if(ssl == NULL)
			return;
		soc_ctx_id = *(int *)ssl->p_bio;
	}
	else
		soc_ctx_id  = *(int *)(g_phandle->client_data.connList->net_context);

	//socket context proc
	if (socket_fd != soc_ctx_id)
		return;

	char *rsp_cmd = xy_zalloc(40);
	if(send_status == SEND_STATUS_SUCCESS)
	{
		ack_callback(APP_DATA, seq_num, SENT_SUCCESS , -1);
		if(g_softap_var_nv->cdp_nsmi == 1)
		{	
			sprintf(rsp_cmd,"\r\n+NSMI:SENT_TO_AIR_INTERFACE,%d\r\n",seq_num);
			send_urc_to_ext(rsp_cmd);
		}
	}
	else if(send_status == SEND_STATUS_FAILED)
	{
		ack_callback(APP_DATA, seq_num, SENT_TIME_OUT, -1);
	}
	
	//回调成功后，需要从cdp_sn_info节点中删掉已使用的seq_num
	cdp_del_sninfo_node(seq_num);
	xy_free(rsp_cmd);
}


/*lint -e550*/
void app_data_report(void)
{
    uint8_t buf[5] = {0, 1, 6, 5, 9};
    data_report_t report_data;
    int ret = 0;
    static int cnt = 0;
    buffer_list_t *temp_node = NULL;
    report_data.buf = buf;
    report_data.cookie = 0;
    report_data.len = sizeof(buf);
    report_data.type = APP_DATA;
    (void)ret;

    if (upstream_info == NULL)
        return;

     osMutexAcquire(g_upstream_mutex, portMAX_DELAY);
     if (upstream_info->head != NULL)
     {
        if(upstream_info->head->type == cdp_NON || upstream_info->head->type == cdp_NON_RAI || upstream_info->head->type == cdp_NON_WAIT_REPLY_RAI)
            report_data.callback = NULL;
        else
            report_data.callback = ack_callback;
        report_data.buf = (uint8_t *)upstream_info->head->data;
        report_data.len = upstream_info->head->data_len;
        report_data.msg_type = upstream_info->head->type;
        report_data.cookie = upstream_info->head->seq_num;
        atiny_printf("app_data_report cookie:%d\n", report_data.cookie);
        ret = atiny_data_report(g_phandle, &report_data);
        temp_node = upstream_info->head->next;
        if (upstream_info->head->data != NULL)
            xy_free(upstream_info->head->data);
        xy_free(upstream_info->head);
        upstream_info->head = temp_node;
        if (upstream_info->head == NULL)
        {
            upstream_info->tail = NULL;
        }
#if VER_QUECTEL
		if(report_data.cookie == 0 && (report_data.msg_type == cdp_NON || report_data.msg_type == cdp_NON_RAI || report_data.msg_type == cdp_NON_WAIT_REPLY_RAI))
			at_send_NSMI(0,0);
#endif
     }
     osMutexRelease(g_upstream_mutex);
}
/*lint +e550*/


uint32_t creat_recv_task()
{
    int uwRet = pdPASS;
	osThreadAttr_t thread_attr = {0};
    if(g_lwm2m_recv_TskHandle == NULL)
    {
		thread_attr.name	   = "lw_down";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = 0x700;
		g_lwm2m_recv_TskHandle = osThreadNew((osThreadFunc_t)(app_downdata_recv), NULL, &thread_attr);
        osThreadSetLowPowerFlag(g_lwm2m_recv_TskHandle, osLowPowerProhibit);
    }
    else 
        uwRet = pdFAIL;

    return uwRet;
}

void cdp_itoa(char* result, size_t bufsize, int number)
{
  const int base = 10;
  char* ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  (void) bufsize;

  //LWIP_UNUSED_ARG(bufsize);

  do {
    tmp_value = number;
    number /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - number * base)];
  } while(number);

   /* Apply negative sign */
  if (tmp_value < 0) {
     *ptr++ = '-';
  }
  *ptr-- = '\0';
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
}


int cdp_get_endpointname(atiny_device_info_t *device_info)
{
	int len;
	if(NET_NEED_RECOVERY(CDP_TASK))
	{
		len = strlen(g_cdp_regInfo->endpointname)+1;
		device_info->endpoint_name = xy_malloc(len);
		memcpy(device_info->endpoint_name, g_cdp_regInfo->endpointname, len);
	}
	else
    {
		if(g_endpointName != NULL)
		{	
			len = strlen(g_endpointName)+1;
			device_info->endpoint_name = xy_malloc(len);
			memcpy(device_info->endpoint_name, g_endpointName,len);
		}
		else
		{	
			device_info->endpoint_name = xy_malloc(16);
			xy_get_IMEI(device_info->endpoint_name, 16);
		}
    }

	if(strlen(device_info->endpoint_name) == 0)
		return XY_ERR;

	return XY_OK;
}

void agent_tiny_entry(void)
{
    uint32_t uwRet = pdPASS;
    atiny_security_param_t  *iot_security_param = NULL;
    atiny_security_param_t  *bs_security_param = NULL;
    char port_str[8] = {0};

    atiny_device_info_t *device_info = xy_zalloc(sizeof(atiny_device_info_t));
	atiny_param_t *atiny_params =xy_zalloc(sizeof(atiny_param_t));
    
    if(NULL == device_info)
    {
        goto out;
    }

    if(cdp_init() < 0)
    {
    	xy_printf("[CDP]cdp init error");
        goto out;
    }

	if(cdp_get_endpointname(device_info) == XY_ERR)
		goto out;

#ifdef CONFIG_FEATURE_FOTA
    device_info->manufacturer = "Lwm2mFota";
    device_info->dev_type = "Lwm2mFota";
#else
    device_info->manufacturer = "Agent_Tiny";
#endif
    atiny_params->server_params.binding = "U";        //UDP model
    if(g_softap_fac_nv->binding_mode ==2)
		atiny_params->server_params.binding = "UQ";        //UDP queue 模式
		
#if VER_QUECTEL
	if(g_softap_fac_nv->cdp_lifetime)
        atiny_params->server_params.life_time = g_softap_fac_nv->cdp_lifetime;
    else
        atiny_params->server_params.life_time = LWM2M_DEFAULT_LIFETIME;
#elif VER_QUCTL260
	if(g_softap_fac_nv->cdp_lifetime)
        atiny_params->server_params.life_time = g_softap_fac_nv->cdp_lifetime;
	else
		atiny_params->server_params.life_time = LWM2M_DEFAULT_LIFETIME;
#else
    if(cdp_lifetime)
        atiny_params->server_params.life_time = cdp_lifetime;
    else
        atiny_params->server_params.life_time = LWM2M_DEFAULT_LIFETIME;
#endif

    atiny_params->server_params.storing_cnt = 0;

    atiny_params->server_params.bootstrap_mode = BOOTSTRAP_FACTORY;
    atiny_params->server_params.hold_off_time = 10;

    //pay attention: index 0 for iot server, index 1 for bootstrap server.
    iot_security_param = &(atiny_params->security_params[0]);
    bs_security_param = &(atiny_params->security_params[1]);

	if(NET_NEED_RECOVERY(CDP_TASK))
	{
		iot_security_param->server_ip = g_cdp_regInfo->net_info.remote_ip;
		bs_security_param->server_ip  = g_cdp_regInfo->net_info.remote_ip;
	}
	else
	{
		iot_security_param->server_ip = g_softap_fac_nv->cloud_server_ip;
		bs_security_param->server_ip  = g_softap_fac_nv->cloud_server_ip;	
	}
		
	cdp_itoa(port_str, 8, g_softap_fac_nv->cloud_server_port);
    iot_security_param->server_port = port_str; 
    bs_security_param->server_port = port_str;

	if (g_softap_fac_nv->cdp_dtls_switch == 1)
	{
		iot_security_param->psk_Id = g_softap_fac_nv->cdp_pskid;
		bs_security_param->psk_Id = iot_security_param->psk_Id;
		
		iot_security_param->psk = g_softap_fac_nv->cloud_server_auth;
		bs_security_param->psk = iot_security_param->psk;
		
		iot_security_param->psk_len = 16;
		bs_security_param->psk_len = 16;
	}
	else
	{
		iot_security_param->psk_Id = NULL;
		iot_security_param->psk = NULL;
		iot_security_param->psk_len = 0;

		bs_security_param->psk_Id = NULL;
		bs_security_param->psk = NULL;
		bs_security_param->psk_len = 0;
	}

    if(ATINY_OK != atiny_init(atiny_params, &g_phandle))
    {
    	xy_printf("[CDP]cdp atiny_init error");
        goto out;

    }

    uwRet = creat_recv_task();
    if(pdPASS != uwRet)
    {
        xy_printf("[King]cdp creat_recv_task error");
        goto out;
    }

    (void)atiny_bind(device_info, g_phandle);

out:
#if VER_QUCTL260
	cdp_downbuffered_clear();
    //g_send_status = NOT_SENT;
	g_softap_var_nv->g_send_status = NOT_SENT;
#endif
	cdp_clear();
	atiny_deinit(g_phandle);
	g_phandle = NULL;

    if(device_info != NULL)
    {
    	if(device_info->endpoint_name != NULL)
		{
			xy_free(device_info->endpoint_name);
			device_info->endpoint_name = NULL;
		}
			
        xy_free(device_info);
        device_info = NULL;
    }

    if(atiny_params != NULL)
    {
        xy_free(atiny_params);
        atiny_params = NULL;
    }
        
	if (cdp_recovery_sem != NULL)
    {
	    //恢复走异常流程，需要解除信号量的阻塞
	    while (osSemaphoreAcquire(cdp_recovery_sem, 0) == osOK) {};
	    osSemaphoreRelease(cdp_recovery_sem);
//        osSemaphoreDelete(cdp_recovery_sem);
//        cdp_recovery_sem = NULL;
    }

    if (cdp_wait_sem != NULL)
    {
        osSemaphoreDelete(cdp_wait_sem);
        cdp_wait_sem = NULL;
    }

    if (cdp_api_sendasyn_sem != NULL)
    {
        osSemaphoreDelete(cdp_api_sendasyn_sem);
        cdp_api_sendasyn_sem = NULL;
    }

    if(g_fota_manager != NULL)
    {
        xy_free(g_fota_manager);
        g_fota_manager = NULL;
    }

    if(g_fota_info != NULL)
    {
        xy_free(g_fota_info);
        g_fota_info = NULL;
    }
	return;
}

void cdp_lwm2m_process(void)
{
	if(xy_wait_tcpip_ok(60)== XY_ERR)
	{
	    if (cdp_recovery_sem != NULL)
	    {
            //恢复走异常流程，需要解除信号量的阻塞
            while (osSemaphoreAcquire(cdp_recovery_sem, 0) == osOK) {};
            osSemaphoreRelease(cdp_recovery_sem);
	    }
		softap_printf(USER_LOG, WARN_LOG, "cdp volunt timeout!!!");
		//softap_TaskDelete_Index(tsk_lwm2m);
		cdp_resume_unlock();
		g_lwm2m_TskHandle = NULL;
		osThreadExit();
		return ;
	}

    agent_tiny_entry();

    if(1 == cdp_register_fail_flag)
    {
    	lwm2m_notify_even(MODULE_LWM2M, XY_STATE_REG_FAILED, NULL, 0);
		cdp_register_fail_flag = 0;
    }

	//g_lwm2m_TskHandle = -1;
    if(1 == cdp_deregister_flag)
    {
        lwm2m_notify_even(MODULE_LWM2M, XY_STATE_DEREGISTERED, NULL, 0);
		cdp_deregister_flag = 0;
    }
       
	//softap_TaskDelete_Index(tsk_lwm2m);
    cdp_resume_unlock();
	g_lwm2m_TskHandle = NULL;
    osThreadExit();
}

int delete_lwm2m_task()
{
    atiny_mark_deinit(g_phandle);

    return 0;
}


int deregister_lwm2m_task()
{
	if((cdp_handle_exist())
			&& !atiny_mark_deregister(g_phandle))
	{
	    return 1;
	}
	else
		return 0;
}

int creat_lwm2m_task()
{
    int uwRet = XY_OK;
	osThreadAttr_t thread_attr = {0};
	
	thread_attr.name       = "cdp_lw_tk";
	thread_attr.priority   = XY_OS_PRIO_NORMAL1;
	thread_attr.stack_size = 0xE00;
    g_lwm2m_TskHandle = osThreadNew((osThreadFunc_t)(cdp_lwm2m_process), NULL, &thread_attr);
    return uwRet;
}

int start_cdp_lwm2m()
{
	if(is_cdp_running())
	{
		return 0;
	}

    //You have to look not only at the value of g_lwm2m_TskHandle 
    //but also at the value of sys_app_tcb[tsk_lwm2m]
    if (!cdp_handle_exist())
    {
	    if(creat_lwm2m_task() == 0)
            return 1;
    }
	
	return 0;

}

void cdp_lwm2m_init()
{
    static int cdplwm2m_inited = 0;
    if(!cdplwm2m_inited)
    {
		xy_regist_psnetif_callback(EVENT_PSNETIF_VALID, cdp_netif_event_callback);

		//带seq_num发送数据时注册的回调
		xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_IPSN, seq_callback);
		cdplwm2m_inited = 1;
	}

	if(get_sys_up_stat() != UTC_WAKEUP && get_sys_up_stat() != EXTPIN_WAKEUP)
		cdp_softap_var_nv_init();
}
#endif

