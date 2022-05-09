/**
 * @file at_passthrough.c
 * @brief 
 * @version 1.0
 * @date 2021-04-29
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 * 
 */

#include "xy_at_api.h"
#include "xy_net_api.h"
#include "xy_passthrough.h"
#include "at_passthrough.h"
#include "xy_utils.h"
#if AT_SOCKET
#include "at_socket_context.h"
#include "lwip/sockets.h"
#endif //AT_SOCKET

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
passthr_socket_ctx_t *g_socket_passthr_ctx = NULL;
osThreadId_t g_sock_trans_send_rcv_thread_id = NULL;
osMutexId_t g_up_data_list_mux = NULL;
extern int rcvd_passthr_stream_proc(char *buf, uint32_t data_len);

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
#if AT_SOCKET
void passthr_netif_down_report_socket_state()
{
	g_socket_passthr_ctx->is_deact = 1;
}

void passthr_netif_down_register()
{
	xy_regist_psnetif_callback(EVENT_PSNETIF_INVALID, xy_exitSocketPassthroughMode);
}

void passthr_netif_down_deregister()
{
	xy_deregist_psnetif_callback(EVENT_PSNETIF_INVALID, xy_exitSocketPassthroughMode);
}

void restore_passthr_hook()
{
	if (g_socket_passthr_ctx != NULL)
		g_socket_passthr_ctx->hook_cache = g_at_passthr_hook;
}

void delete_passthr_socket_ctx()
{
	struct up_data_list *cur_head, *pre;
	if(g_socket_passthr_ctx == NULL)
		return;

	if(g_socket_passthr_ctx->remote_ip != NULL)
		xy_free(g_socket_passthr_ctx->remote_ip);
	
	osMutexAcquire(g_up_data_list_mux, osWaitForever);
	
	cur_head = g_socket_passthr_ctx->data_list;

    while (cur_head != NULL)
    {
    	if(cur_head->data != NULL)
			xy_free(cur_head->data);
        pre = cur_head;
        cur_head = cur_head->next;
		xy_free(pre);
    }
	g_socket_passthr_ctx->data_list = NULL;
	osMutexRelease(g_up_data_list_mux);
	
	xy_free(g_socket_passthr_ctx);
	g_socket_passthr_ctx = NULL;
	osMutexDelete(g_up_data_list_mux);
	g_up_data_list_mux = NULL;
}

void send_uplink_data()
{
	if(g_socket_passthr_ctx == NULL || g_socket_passthr_ctx->data_list == NULL)
		return;

	osMutexAcquire(g_up_data_list_mux, osWaitForever);
	
	struct up_data_list *cur_head, *pre;
	int ret = -1;
	
	cur_head = g_socket_passthr_ctx->data_list;

    while (cur_head != NULL)
    {
    	if(cur_head->data != NULL)
    	{
	    	ret = send(g_socket_passthr_ctx->sock_fd, cur_head->data, cur_head->len, 0);
			if (ret <= 0) {
				softap_printf(USER_LOG, WARN_LOG, "send_errno:%d", errno);
			}
			xy_free(cur_head->data);
    	}
        pre = cur_head;
        cur_head = cur_head->next;
		xy_free(pre);
    }
	g_socket_passthr_ctx->data_list = NULL;

	osMutexRelease(g_up_data_list_mux);
}

void passthr_socket_send_recv_thd(void)
{
	int ret;
	int max_fd;
	fd_set readfds, exceptfds;
	struct timeval tv;
    tv.tv_sec = SOCK_RECV_TIMEOUT;
    tv.tv_usec = 0;
	passthr_netif_down_register();
	
	while(1) {
		
		FD_ZERO(&readfds); 
		FD_ZERO(&exceptfds);
		max_fd=-1;
		
		if (g_socket_passthr_ctx != NULL && g_socket_passthr_ctx->quit == 1 && g_socket_passthr_ctx->sock_fd >= 0)
		{
			close(g_socket_passthr_ctx->sock_fd);
			delete_passthr_socket_ctx();
			continue;
		}
		else if(g_socket_passthr_ctx != NULL && g_socket_passthr_ctx->is_deact == 1 && g_socket_passthr_ctx->sock_fd >= 0)
		{
			if (g_at_passthr_hook != NULL)
			{
				xy_exitPassthroughMode();
			}
			close(g_socket_passthr_ctx->sock_fd);
			delete_passthr_socket_ctx();
			continue;
	    }
		else if (g_socket_passthr_ctx != NULL && g_socket_passthr_ctx->quit != 1 && g_socket_passthr_ctx->sock_fd >= 0) 
		{
			if(max_fd < g_socket_passthr_ctx->sock_fd)
			{
				max_fd = g_socket_passthr_ctx->sock_fd;
			}
			FD_SET(g_socket_passthr_ctx->sock_fd, &readfds);
			FD_SET(g_socket_passthr_ctx->sock_fd, &exceptfds);
		}

		if (max_fd < 0) 
		{
            goto out;
		}

		send_uplink_data();
		
		ret = select(max_fd+1,&readfds,NULL,&exceptfds,&tv);
		if (ret < 0)
		{
			if (errno == EBADF)
			{
				xy_assert(0);
			}
			else
			{
				continue;
			}
		}
		else if (ret == 0)
        {
            //we assume select timeouted here
            continue;
        }
		
        if (g_socket_passthr_ctx == NULL)
            continue;

		if(g_socket_passthr_ctx != NULL && FD_ISSET(g_socket_passthr_ctx->sock_fd,&exceptfds))
		{
			softap_printf(USER_LOG, WARN_LOG, "except exist,force to close socket");
			if (g_at_passthr_hook != NULL)
			{
				xy_exitPassthroughMode();
			}
			close(g_socket_passthr_ctx->sock_fd);
			delete_passthr_socket_ctx();
			continue;

		}
		if (g_socket_passthr_ctx != NULL && !FD_ISSET(g_socket_passthr_ctx->sock_fd,&readfds)) {
			continue;
		}

		int len = 0;
		int read_len = 0;
		char *buf = NULL;
	
		ioctl(g_socket_passthr_ctx->sock_fd, FIONREAD, &len);  //get the size of received data 
		softap_printf(USER_LOG, WARN_LOG, "recv len:%d", len);
		if (len == 0) { 
			if (g_at_passthr_hook != NULL)
			{
				xy_exitPassthroughMode();
			}
			close(g_socket_passthr_ctx->sock_fd);
			delete_passthr_socket_ctx();
			softap_printf(USER_LOG, WARN_LOG, "recv 0 BYTES,force to close socket");
			continue;
		}
		
		buf = xy_zalloc(len+1);
		read_len = recv(g_socket_passthr_ctx->sock_fd, buf, len, MSG_DONTWAIT);
		softap_printf(USER_LOG, WARN_LOG, "recv read_len:%d", read_len);
		if (read_len < 0)
        {
            if (errno == EWOULDBLOCK) // time out
            {
            	xy_free(buf);
                continue;
            }
            else
            {
            	softap_printf(USER_LOG, WARN_LOG, "read - BYTES,force to close socket");
				if (g_at_passthr_hook != NULL)
				{
					xy_exitPassthroughMode();
				}
            	close(g_socket_passthr_ctx->sock_fd);

				delete_passthr_socket_ctx();
				xy_free(buf);
                continue;
            }
        }
		else if (read_len == 0) { 
			softap_printf(USER_LOG, WARN_LOG, "read 0 BYTES,force to close socket");
			if (g_at_passthr_hook != NULL)
			{
				xy_exitPassthroughMode();
			}
			close(g_socket_passthr_ctx->sock_fd);
			delete_passthr_socket_ctx();
			xy_free(buf);
			continue;
		}
		
		send_debug_str_to_at_uart(buf);
		xy_free(buf);
		
	}
out:
	passthr_netif_down_deregister();
	softap_printf(USER_LOG, WARN_LOG, "socket recv thread exit!!!");
	g_sock_trans_send_rcv_thread_id = NULL;
	osThreadExit();
}

void passthr_socket_init(struct sock_context *sock_param)
{
	osThreadAttr_t thread_attr = {0};
	
	if(g_socket_passthr_ctx == NULL)
		g_socket_passthr_ctx = xy_zalloc(sizeof(passthr_socket_ctx_t));

	g_socket_passthr_ctx->net_type = sock_param->net_type;
	g_socket_passthr_ctx->remote_port = sock_param->remote_port;
	g_socket_passthr_ctx->sock_fd = sock_param->fd;
	
	g_up_data_list_mux = osMutexNew(NULL);
	if(g_sock_trans_send_rcv_thread_id == NULL)
	{
		thread_attr.name       = "trans_snd_rcv";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = 0x800;
		g_sock_trans_send_rcv_thread_id = osThreadNew((osThreadFunc_t)(passthr_socket_send_recv_thd), NULL, &thread_attr);
	}
}

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
void add_uplink_data_to_list(char *data_buff,int data_len)
{
	osMutexAcquire(g_up_data_list_mux, osWaitForever);
	
	struct up_data_list *nod;
	struct up_data_list *cur_head, *pre;
	if(g_socket_passthr_ctx == NULL)
		xy_assert(0);

	nod = xy_zalloc(sizeof(struct up_data_list));
	nod->data = data_buff;
	nod->len = data_len;
	
	cur_head = g_socket_passthr_ctx->data_list;
	if(cur_head == NULL)
	{
		g_socket_passthr_ctx->data_list = nod;
		goto END;
	}
    pre = cur_head;
	cur_head = cur_head->next;
    while (cur_head != NULL)
    {
        pre = cur_head;
        cur_head = cur_head->next;
    }
	pre->next = nod;
END:
	osMutexRelease(g_up_data_list_mux);
}

//AT+XTRANSSOCKOPEN=<protocol>,<af_type>,<remote_addr>,<remote_port>[,<local_port>]
int at_XTRANSSOCKOPEN_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		char protocal[10] = {0};
		char af_type[10] = {0};
		char *remote_ip = xy_zalloc(strlen(at_buf));
		int remote_port = 0;
		int local_port = 0;
		int fd = -1;
		int res = 0;
		struct sock_context *sock_param = xy_zalloc(sizeof(struct sock_context));

		if (xy_wait_tcpip_ok(0) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto END_PROC;
		}

		if (at_parse_param("%s,%s,%s,%d,%d", at_buf, protocal, af_type, remote_ip, &remote_port, &local_port) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else
		{			
			if(!strcmp(af_type,"AF_INET6"))
			{
				sock_param->af_type = 1; //ipv6
			}
			else if((!strcmp(af_type,"AF_INET")) || (strlen(af_type)==0))
			{
				sock_param->af_type = 0;  //ipv4
			}
			else
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

			if(!strcmp(protocal,"TCP"))
				sock_param->net_type = 0;
			else if(!strcmp(protocal,"UDP"))
				sock_param->net_type = 1;
			else
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

			sock_param->local_port = (unsigned short)local_port;
			
			if ((res = at_socket_create(sock_param, &fd)) != AT_OK) 
			{
				if (fd > -1)
					close(fd);
				*prsp_cmd = AT_ERR_BUILD(res);
				goto END_PROC;
			}
			
			sock_param->fd = fd;
			sock_param->remote_port = (unsigned short)remote_port;
			sock_param->remote_ip = xy_zalloc(strlen(remote_ip)+1);
		
			memcpy(sock_param->remote_ip,remote_ip,strlen(remote_ip));
			if((res=connect_network(sock_param)) == 0)
			{
				close(sock_param->fd); 
				*prsp_cmd = AT_ERR_BUILD(res);
				goto END_PROC;
			}
			nonblock(fd);

            xy_enterPassthroughMode(PASSTHR_SOCKET_PPP, 0, (data_proc_func)(rcvd_passthr_stream_proc));
            passthr_socket_init(sock_param);
            *prsp_cmd = xy_zalloc(40);
			sprintf(*prsp_cmd, "\r\nOK\r\n\r\nCONNECT\r\n");
		}
END_PROC:
		if(sock_param->remote_ip != NULL)
			xy_free(sock_param->remote_ip);
		xy_free(sock_param);
		xy_free(remote_ip);
		return AT_END;
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
}

int at_ATO_req(char *at_buf, char **prsp_cmd)
{
    (void)at_buf;
    //only for passthrough
	if(g_req_type == AT_CMD_ACTIVE)
	{
		if(g_socket_passthr_ctx != NULL && g_socket_passthr_ctx->hook_cache != NULL && g_socket_passthr_ctx->sock_fd >= 0)
		{
            xy_enterPassthroughMode(PASSTHR_SOCKET_PPP, 0, (data_proc_func)(g_socket_passthr_ctx->hook_cache));
			*prsp_cmd = xy_zalloc(40);
			snprintf(*prsp_cmd, 40, "\r\nOK\r\n\r\nCONNECT\r\n");
		}
		else
		{
			*prsp_cmd = xy_zalloc(40);
			snprintf(*prsp_cmd, 40, "\r\nNO CARRIER\r\n\r\nOK\r\n");
		}
	}	
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int at_XTRANSSOCKCLOSE_req(char *at_buf,char **prsp_cmd)
{
	(void)at_buf;
	if(g_req_type == AT_CMD_ACTIVE)
	{
		if(g_socket_passthr_ctx != NULL && g_socket_passthr_ctx->sock_fd != -1)
		{
			g_socket_passthr_ctx->quit = 1;
			softap_printf(USER_LOG, WARN_LOG, "nsocl quit set 1!!!");
		}
		while (g_socket_passthr_ctx != NULL && g_socket_passthr_ctx->sock_fd != -1)
		{
			osDelay(100);
		}
		
		softap_printf(USER_LOG, WARN_LOG, "nsocl fd set -1!!!");
		
		*prsp_cmd = xy_zalloc(10);
	    strcpy(*prsp_cmd, "\r\nOK\r\n");
		return AT_END;
	}	
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

#endif //AT_SOCKET

/**
 * @brief  PPP协议中的ATD切换透传模式的处理函数
 * @note   
 */
int at_ATD_req(char *at_buf,char **prsp_cmd)
{
	//only for transparent
	if(g_req_type == AT_CMD_ACTIVE)
	{
		if(at_strnstr(at_buf,"*98",3) || at_strnstr(at_buf,"*99",3))
		{
			xy_enterPassthroughMode(PASSTHR_NORMAL_PPP, 0, (data_proc_func)(rcvd_passthr_stream_proc));
			*prsp_cmd = xy_zalloc(40);
			snprintf(*prsp_cmd, 40, "\r\nOK\r\n\r\nCONNECTING\r\n");
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
	}	
	return AT_END;
}

/**
 * @brief  对于指定长度的AT命令示例的处理入口，内部解析出指定的长度值，并切换透传模式
 * @note   对于socket、cdp、onenet等数据传输业务，皆可以仿造该命令来实现其他的AT命令透传模式
 * @note   命令格式AT+TRANSPARENTDEMO=<data len>
 */
int at_TRANSPARENTDEMO_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int data_len = 0;

		if (at_parse_param("%d", at_buf, &data_len) != AT_OK) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		else
		{
            xy_enterPassthroughMode(PASSTHR_FIXED_LENGTH, data_len, (data_proc_func)(rcvd_passthr_stream_proc));
        }
		*prsp_cmd = xy_zalloc(40);
		snprintf(*prsp_cmd, 40, "\r\nOK\r\n\r\nCONNECT\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int cloud_passthr_proc(char* buf, uint32_t len, void *param)
{
	UNUSED_ARG(len);
	UNUSED_ARG(param);
	softap_printf(USER_LOG, WARN_LOG, "cloud passthr recv:%s", buf);
    return XY_OK;
}

int at_CLOUDPASSTHR_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int data_len = 0;

		if (at_parse_param("%d", at_buf, &data_len) != AT_OK) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		app_passthr_info_t cloud_passthr = {0};
		cloud_passthr.app_type = APP_COAP;
		cloud_passthr.recv_len = data_len;
		cloud_passthr.proc = cloud_passthr_proc;

		into_Passthr_Mode(cloud_passthr);
		send_urc_to_ext("\r\n>");

		return AT_ASYN;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}