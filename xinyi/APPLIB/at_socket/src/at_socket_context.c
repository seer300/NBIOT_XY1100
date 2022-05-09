/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "softap_macro.h"
#if AT_SOCKET

#include "at_com.h"
#include "at_global_def.h"
#include "at_socket.h"
#include "at_socket_context.h"
#include "factory_nv.h"
#include "lwip/apps/sntp.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "net_app_resume.h"
#include "xy_at_api.h"
#include "xy_flash.h"
#include "xy_net_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
int g_at_sck_report_mode = BUFFER_WITH_HINT;
int g_ipdata_mode = HEX_ASCII_STRING;
uint8_t g_data_recv_mode = ASCII_STRING;
uint8_t g_data_send_mode = ASCII_STRING;
osThreadId_t at_rcv_thread_id = NULL;
osMutexId_t g_socket_mux = NULL;
osMutexId_t sninfo_mux = NULL;
struct sock_context *sock_ctx[SOCK_NUM];
struct sock_sn_node sn_info = {0};
int32_t g_backup_udp_once = -1;
udp_context_t *g_udp_context = NULL;

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
/**
 * @brief 设置socket sequence的默认状态，用于socket初始化
 */
void set_socket_sequence_default_state(unsigned char sock_id)
{
	int i = 0;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (sock_ctx[i] != NULL && sock_ctx[i]->sock_id == sock_id)
		{
			int j = 0;
			for (j = 0; j < SEQUENCE_MAX; j++)
			{
				sock_ctx[i]->sequence_state[j] = SEND_STATUS_UNUSED;
			}
		}
	}
}

/**
 * @brief 根据socket id查找对应socket 上下文id
 * @return -1表示未找到对应socket上下文
 */
int find_sock_ctx_id_by_sock_id(int sock_id)
{
	int i = 0;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (sock_ctx[i] != NULL && sock_ctx[i]->sock_id == sock_id)
			return i;
	}
	return -1;
}

/**
 * @brief 根据socket fd查找对应socket 上下文id
 * @return -1表示未找到对应socket上下文
 */
int find_sock_ctx_id_by_sock_fd(int sock_fd)
{
	int i = 0;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (sock_ctx[i] != NULL && sock_ctx[i]->fd == sock_fd)
			return i;
	}
	return -1;
}

void nonblock(int fd)
{
	int fl = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int connect_network(struct sock_context *sock_param)
{
	struct addrinfo hint = {0};
	struct addrinfo *result = NULL;
	if(sock_param->af_type == 1)
		hint.ai_family = AF_INET6;
	else
		hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_DGRAM;
	char port_str[7] = {0};

	snprintf(port_str, sizeof(port_str) - 1, "%d", sock_param->remote_port);
	if (getaddrinfo(sock_param->remote_ip, port_str, &hint, &result) != 0) 
	{
        softap_printf(USER_LOG, WARN_LOG, "socket[%d] getaddrinfo err:%d", sock_param->sock_id, errno);
        return 0;
	}

	if (connect(sock_param->fd, result->ai_addr, result->ai_addrlen) == -1) 
	{
	    freeaddrinfo(result);
        softap_printf(USER_LOG, WARN_LOG, "socket[%d] connect err:%d", sock_param->sock_id, errno);
        return 0;
	}
	
	freeaddrinfo(result);
	return 1;
}

//socket creat
int at_socket_create(struct sock_context *sock_param, int *fd)
{
	struct sockaddr_in bind_addr = {0};
	int context = -1;
	unsigned short local_port = sock_param->local_port;

	if (sock_param->net_type == 1 && sock_param->af_type == 0)
	{
		context = socket(AF_INET /*srv.family*/, SOCK_DGRAM, IPPROTO_UDP);
	}
	else if (sock_param->net_type == 1 && sock_param->af_type == 1)
	{
		context = socket(AF_INET6 /*srv.family*/, SOCK_DGRAM, IPPROTO_UDP);
	}

	if (sock_param->net_type == 0 && sock_param->af_type == 0)
	{
		context = socket(AF_INET /*srv.family*/, SOCK_STREAM, IPPROTO_TCP);
	}
	else if (sock_param->net_type == 0 && sock_param->af_type == 1)
	{
		context = socket(AF_INET6 /*srv.family*/, SOCK_STREAM, IPPROTO_TCP);
	}

	if (context == -1)
	{
		return ATERR_PARAM_INVALID;
	}
	*fd = context;

	if(sock_param->af_type == 1)
		bind_addr.sin_family = AF_INET6;
	else
		bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(local_port);
	if (bind(*fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1)
	{
		softap_printf(USER_LOG, WARN_LOG, "sock bind errno:%d", errno);
        close(*fd);
		return ATERR_NOT_NET_CONNECT;
	}

	struct timeval tv;
    tv.tv_sec = SOCK_CREATE_TIMEOUT;
    tv.tv_usec = 0;

    setsockopt(*fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));

	return AT_OK;
}

/**
 * @brief 根据socket 上下文索引删除socket 上下文
 */
int del_socket_ctx_by_index(int idx, bool report)
{
	osMutexAcquire(g_socket_mux, osWaitForever);
	if (sock_ctx[idx] != NULL)
	{
        uint8_t net_type = sock_ctx[idx]->net_type;
        close(sock_ctx[idx]->fd);
		struct rcv_data_nod *nod_next = NULL;
		struct rcv_data_nod *temp = sock_ctx[idx]->data_list;
		while (temp != NULL)
		{
			nod_next = temp->next;
			if (temp->data)
			{
				xy_free(temp->data);
			}
			if (temp->sockaddr_info)
				xy_free(temp->sockaddr_info);
			xy_free(temp);
			temp = nod_next;
		}
		if (sock_ctx[idx]->remote_ip != NULL)
			xy_free(sock_ctx[idx]->remote_ip);

		del_allsnnode_by_socketid(sock_ctx[idx]->sock_id);
		xy_free(sock_ctx[idx]);
        sock_ctx[idx] = NULL;
        if (report)
            at_report_socket_state(idx, net_type);
        osMutexRelease(g_socket_mux);
        return XY_OK;
    }
    else
    {
        osMutexRelease(g_socket_mux);
        return XY_ERR;
    }
}

int find_maxfd_in_use()
{
	int i;
	int max_fd = -1;

	for (i = 0; i < SOCK_NUM; i++)
	{
		if (sock_ctx[i] != NULL && sock_ctx[i]->fd >= 0 && sock_ctx[i]->fd > max_fd)
		{
			max_fd = sock_ctx[i]->fd;
		}
	}
	return max_fd;
}

int add_new_rcv_data_node(int skt_id, int rcv_len, char *buf, struct sockaddr_in *sockaddr_info)
{
    int len_sum = 0;
    struct rcv_data_nod *temp;
    struct rcv_data_nod *rcv_nod = xy_zalloc(sizeof(struct rcv_data_nod));
    rcv_nod->data = buf;
    rcv_nod->len = rcv_len;
    rcv_nod->sockaddr_info = sockaddr_info;
    rcv_nod->next = NULL;

    osMutexAcquire(g_socket_mux, osWaitForever);
    temp = sock_ctx[skt_id]->data_list;
    if (temp == NULL)
        sock_ctx[skt_id]->data_list = rcv_nod;
    else
    {
        while (temp->next != NULL)
        {
            len_sum += temp->len;
            temp = temp->next;
        }
        len_sum += temp->len;
        if (len_sum + rcv_len < SOCK_RCV_BUF_MAX)
            temp->next = rcv_nod;
        else
        {
            /* 缓存超过4000且未取出,丢弃新收到的数据*/
            xy_free(rcv_nod);
            if (sockaddr_info != NULL)
                xy_free(sockaddr_info);
            xy_free(buf);
            osMutexRelease(g_socket_mux);
            return XY_ERR;
        }
    }
    osMutexRelease(g_socket_mux);
    return XY_OK;
}

int get_free_sock_id()
{
	int i;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (sock_ctx[i] == NULL)
		{
			return i;
		}
	}
	return -1;
}

int is_all_socket_exit()
{
	int i = 0;
	for (i = 0; i < SOCK_NUM; i++)
		if (sock_ctx[i] != NULL)
			return 0;
	return 1;
}

void add_sninfo_node(unsigned char socket_id, int len, unsigned char sequence, uint32_t pre_sn)
{
	osMutexAcquire(sninfo_mux, osWaitForever);
	int idx = find_sock_ctx_id_by_sock_id(socket_id);
	struct sock_sn_node *nod;
	struct sock_sn_node *cur_head, *pre;
	if(idx == -1)
		xy_assert(0);

	cur_head = &sn_info;
    pre = cur_head;
    while (cur_head != NULL)
    {
        pre = cur_head;
        cur_head = cur_head->next;
    }
	nod = xy_zalloc(sizeof(struct sock_sn_node));
	nod->data_len = len;
	nod->per_sn = pre_sn;
	nod->sequence = sequence;
	nod->socket_id = socket_id;
	nod->net_type = sock_ctx[idx]->net_type;
	pre->next = nod;
	osMutexRelease(sninfo_mux);
}

void del_sninfo_node(unsigned char socket_id, unsigned char sequence)
{
	osMutexAcquire(sninfo_mux, osWaitForever);
	struct sock_sn_node *cur_nod, *pre;

	cur_nod = &sn_info;
    pre = cur_nod;
	cur_nod = cur_nod->next;

    while (cur_nod != NULL)
    {
        if(cur_nod->socket_id == socket_id && cur_nod->sequence == sequence)
    	{
            pre->next = cur_nod->next;
    		xy_free(cur_nod);
    		osMutexRelease(sninfo_mux);
    		return;
    	}
        pre = cur_nod;
        cur_nod = cur_nod->next;
    }
    osMutexRelease(sninfo_mux);
}

int find_match_udp_node(unsigned char socket_id, unsigned char sequence)
{
	struct sock_sn_node *cur_nod;

	cur_nod = &sn_info;
	cur_nod = cur_nod->next;

    while (cur_nod != NULL)
    {
        if(cur_nod->net_type == 1 && cur_nod->socket_id == socket_id && cur_nod->sequence == sequence)
    	{
    		return 1;
    	}
        cur_nod = cur_nod->next;
    }
	return 0;
}

int find_match_tcp_node(unsigned char socket_id, uint32_t ack, unsigned char *psequence)
{
	struct sock_sn_node *cur_nod;

	cur_nod = &sn_info;
	cur_nod = cur_nod->next;

    while (cur_nod != NULL)
    {
		if(cur_nod->net_type == 0 && cur_nod->socket_id == socket_id && ack-cur_nod->per_sn == cur_nod->data_len)
    	{
#if VER_QUCTL260
            int sock_ctx_id = find_sock_ctx_id_by_sock_id(socket_id);
            sock_ctx[sock_ctx_id]->acked += cur_nod->data_len;
#endif
            *psequence = cur_nod->sequence;
    		return 1;
    	}
        cur_nod = cur_nod->next;
    }
	return 0;
}

int check_same_sequence(int sockid,unsigned char seq_num)
{
	struct sock_sn_node *cur_nod;
	int idx = find_sock_ctx_id_by_sock_id(sockid);
	if(idx == -1)
		return 0;

	if(seq_num == 0)
		return 0;

	cur_nod = &sn_info;
	cur_nod = cur_nod->next;

    while (cur_nod != NULL)
    {
        if(cur_nod->net_type == sock_ctx[idx]->net_type && cur_nod->socket_id == sockid && cur_nod->sequence == seq_num)
    	{
    		return 1;
    	}

        cur_nod = cur_nod->next;
    }
	return 0;
}

void del_allsnnode_by_socketid(int socket_id)
{
	osMutexAcquire(sninfo_mux, osWaitForever);
	struct sock_sn_node *cur_nod, *pre;
	char *sequence_rsp = NULL;

	cur_nod = &sn_info;
    pre = cur_nod;
	cur_nod = cur_nod->next;

    while (cur_nod != NULL)
    {
        if(cur_nod->socket_id == socket_id)
    	{
			sock_ctx[socket_id]->sequence_state[cur_nod->sequence - 1] = SEND_STATUS_FAILED;

			sequence_rsp = (char*)xy_zalloc(30);
#if VER_QUCTL260
            snprintf(sequence_rsp, 30, "\r\nSEND FAIL\r\n");
#else
            sprintf(sequence_rsp, "\r\n+NSOSTR:%d,%d,%d\r\n", socket_id, cur_nod->sequence, 0);
#endif
            send_rsp_str_to_ext(sequence_rsp); 
			xy_free(sequence_rsp);
			
            pre->next = cur_nod->next;
    		xy_free(cur_nod);
			cur_nod = pre->next;
			continue;
    	}
        pre = cur_nod;
        cur_nod = cur_nod->next;
    }
    osMutexRelease(sninfo_mux);
}

#if 0
void xy_read_node_from_readlist(int skt_index, char *buf, int *pread_len)
{
	osMutexAcquire(g_socket_mux, osWaitForever);

	struct	rcv_data_nod *temp = sock_ctx[skt_index]->data_list;
	int temp_len = 0;
	char *rcv_data = NULL;
	int buf_size = 0;
	
	if(temp == NULL)
	{
		*pread_len = 0;
		goto END;
	}
	while(temp != NULL)
	{
		temp_len += temp->len;
		if(temp_len<=*pread_len)
		{
			memcpy(buf+buf_size,temp->data,temp->len);
			buf_size += temp->len;
			sock_ctx[skt_index]->data_list = temp->next;
			if(temp->data)
				xy_free(temp->data);
			if(temp->sockaddr_info)
				xy_free(temp->sockaddr_info);
			xy_free(temp);
			temp = sock_ctx[skt_index]->data_list;
			if(temp_len == *pread_len)
				goto END;
		}
		else
		{
			memcpy(buf+buf_size,temp->data,temp->len-(temp_len-*pread_len));
			buf_size += temp->len-(temp_len-*pread_len);
			rcv_data = xy_zalloc(temp_len-*pread_len+1);
			memcpy(rcv_data,temp->data+temp->len-(temp_len-*pread_len),temp_len-*pread_len);
			xy_free(temp->data);
			temp->data = rcv_data;
			temp->len = temp_len-*pread_len;
			goto END;
		}
	}
	if(temp_len<*pread_len)
		*pread_len = temp_len;
END:
	osMutexRelease(g_socket_mux);
}
#endif

int at_report_socket_state(int id, int net_type)
{
#if (VER_QUECTEL||VER_QUCTL260)
    if (net_type == 1)
        return XY_OK;
#endif //VER_QUECTEL

	char *report_buf = xy_zalloc(30);
#if VER_QUCTL260
    snprintf(report_buf, 30, "\r\n+QIURC: \"closed\",%d\r\n", id);
#else
    snprintf(report_buf, 30, "\r\n+NSOCLI:%d\r\n", id);
#endif //VER_QUCTL260
    send_rsp_str_to_ext(report_buf);
	xy_free(report_buf);
	return XY_OK;
}

void get_local_port_by_fd(int fd, unsigned short *local_port)
{
	int ret;
	socklen_t sockaddr_len = sizeof(struct sockaddr_storage);
    struct sockaddr_storage sockaddr;

    ret = getsockname(fd, (struct sockaddr *)&sockaddr, &sockaddr_len);
	if (ret < 0) 
	{
		softap_printf(USER_LOG, WARN_LOG, "getsockname fail!!");
		return ;
	}

	if (((struct sockaddr *)&sockaddr)->sa_family == AF_INET)
	{
		*local_port = ntohs(((struct sockaddr_in *)&sockaddr)->sin_port);
	}
	else if (((struct sockaddr *)&sockaddr)->sa_family == AF_INET6)
	{
		*local_port = ntohs(((struct sockaddr_in6 *)&sockaddr)->sin6_port);
	}
}

//AT+SEQUENCE=<socket>,<sequence>
int at_SEQUENCE_req(char* at_buf, char** prsp_cmd)
{
	unsigned short seq_num = 0;
	unsigned short socket_id = 0;
	void *p[] = {&socket_id, &seq_num};

	if (at_parse_param_2("%2d,%2d", at_buf, p) != AT_OK)
		goto err;

	if ((0 == seq_num || seq_num > SEQUENCE_MAX) || (socket_id >= SOCK_NUM || socket_id < 0) || (sock_ctx[socket_id] == NULL))
		goto err;

	*prsp_cmd = xy_zalloc(30);
	snprintf(*prsp_cmd, 30, "\r\n%d\r\n\r\nOK\r\n", sock_ctx[socket_id]->sequence_state[seq_num - 1]);
	return AT_END;			
err:	
	*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//+TCPACK:<socket_id>,<ack_no>
int at_TCPACK_info(char* at_buf)
{
	int conn_socket = -1;
	uint32_t ack = 0;
	unsigned char idx;
	unsigned char sequence = 0;
	char *sequence_rsp = NULL;

	softap_printf(USER_LOG, WARN_LOG, "recv cmd: +TCPACK:%s",at_buf);

	if (at_parse_param("%d,%d", at_buf, &conn_socket, &ack) != AT_OK)
	{
		softap_printf(USER_LOG, WARN_LOG, "+TCPACK:ERR!!!");
		return AT_END;		
	}
	for (idx = 0; idx < SOCK_NUM; idx++)
	{
		if(sock_ctx[idx] != NULL && sock_ctx[idx]->fd == conn_socket)
			break;
	}
	if (idx == SOCK_NUM)
	{
		softap_printf(USER_LOG, WARN_LOG, "tcpack can not match conn_socket");
		return AT_END;
	}
	if (find_match_tcp_node(sock_ctx[idx]->sock_id, ack, &sequence))
	{
		sock_ctx[idx]->sequence_state[sequence - 1] = SEND_STATUS_SUCCESS;
		del_sninfo_node(sock_ctx[idx]->sock_id, sequence);

		sequence_rsp = (char*)xy_zalloc(30);
#if VER_QUCTL260
        snprintf(sequence_rsp, 30, "\r\nSEND OK\r\n");
#else
        sprintf(sequence_rsp, "\r\n+NSOSTR:%d,%d,%d\r\n", sock_ctx[idx]->sock_id, sequence, 1);
#endif
		send_rsp_str_to_ext(sequence_rsp); 
		xy_free(sequence_rsp);
	}
	else
	{
		softap_printf(USER_LOG, WARN_LOG, "tcpack can not find sninfo node");
	}
	return AT_END;
}

int save_udp_context_deepsleep(udp_context_t* udp_info)
{
	int ret;
	int i = 0;
	
	if(g_udp_context == NULL)
	{
		g_udp_context = xy_zalloc(sizeof(udp_context_t));
	}
	
	for(i=0;i<SOCK_NUM;i++)
	{
		g_udp_context->udp_socket[i].socket_idx = -1;
	}
	
	for(i=0;i<SOCK_NUM;i++)
	{
		if(sock_ctx[i] == NULL || sock_ctx[i]->net_type == 0)
		{
			continue;	
		}
		
		//memcpy(&(g_udp_context->udp_socket[i].local_ip),&(sock_ctx[i]->local_ip),sizeof(ip_addr_t));
		g_udp_context->udp_socket[i].net_info.local_port = sock_ctx[i]->local_port;
		g_udp_context->udp_socket[i].recv_ctl = sock_ctx[i]->recv_ctl;
		g_udp_context->udp_socket[i].socket_idx = (char)(sock_ctx[i]->sock_id);
		g_udp_context->udp_socket[i].af_type = sock_ctx[i]->af_type;	
	}
	
	g_udp_context->data_mode = (unsigned char)g_at_sck_report_mode;
	
	if(is_all_socket_exit())
		return CLOUD_SAVE_NONEED_WRITE;
	
    for(i=0;i<SOCK_NUM;i++)
    {
        if(g_udp_context->udp_socket[i].net_info.local_port != 0 && g_udp_context->udp_socket[i].socket_idx == (char)-1)
        {
            memset(&(g_udp_context->udp_socket[i]),0,sizeof(struct udp_context_fd));
            g_udp_context->udp_socket[i].socket_idx = (char)-1;
            //目前仅支持V4
            xy_get_ipaddr(IPADDR_TYPE_V4,&g_udp_context->udp_socket[i].net_info.local_ip);
        }
    }

    memcpy(udp_info,g_udp_context,sizeof(udp_context_t));
	return CLOUD_SAVE_NEED_WRITE;
}

void restore_udp_context_thread()
{
	int res = -1;
	int fd = -1;
	int idx = 0;
	int i = 0;
	struct sock_context *sock_param[SOCK_NUM] = {NULL};
	osThreadAttr_t thread_attr = {0};
	
	if(g_udp_context == NULL || *(int *)g_udp_context == 0xFFFFFFFF)
	{
		softap_printf(USER_LOG, WARN_LOG,"g_udp_context is NULL,exit thread!!");
		goto END;
	}
	
	xy_wait_tcpip_ok(1);
	
	for(i=0;i<SOCK_NUM;i++)
	{
		if(g_udp_context->udp_socket[i].socket_idx == (char)-1 || g_udp_context->udp_socket[i].net_info.local_port == 0)
			continue;
		softap_printf(USER_LOG, WARN_LOG,"g_udp_context->udp_socket[%d].socket_idx:%1d",i,g_udp_context->udp_socket[i].socket_idx);
		xy_assert(g_udp_context->udp_socket[i].socket_idx != (char)-1);
		sock_param[i] = xy_zalloc(sizeof(struct sock_context));
		sock_param[i]->fd = -1;
		idx = g_udp_context->udp_socket[i].socket_idx;
		
		if(sock_ctx[idx] != NULL || idx >= SOCK_NUM)
		{
			xy_assert(0);
			softap_printf(USER_LOG, WARN_LOG,"sock_ctx[idx] not NULL,exit thread!!");
			goto END;
		}	

		sock_param[i]->sock_id = idx;
		sock_param[i]->local_port = (unsigned short)(g_udp_context->udp_socket[i].net_info.local_port);
		//memcpy(&(sock_ctx[i]->local_ip),&(g_udp_context->udp_socket[i].local_ip),sizeof(ip_addr_t));
		sock_param[i]->net_type = 1;
		sock_param[i]->af_type = g_udp_context->udp_socket[i].af_type;
		sock_param[i]->recv_ctl = g_udp_context->udp_socket[i].recv_ctl;

		sock_param[i]->recv_ctl = g_udp_context->udp_socket[i].recv_ctl ;
		g_at_sck_report_mode = g_udp_context->data_mode;
		
	    if ((res = at_socket_create(sock_param[i], &fd)) != AT_OK) 
		{
			if (fd > -1)
				close(fd);
			softap_printf(USER_LOG, WARN_LOG,"udp_nack_err:%d",res);
			xy_free(sock_param[i]);	
			softap_printf(USER_LOG, WARN_LOG,"sock_ctx[%d] creat fail,exit thread!!",i);
			continue;
		}
		sock_param[i]->fd = fd;
		nonblock(fd);
		sock_ctx[idx] = sock_param[i];	
		set_socket_sequence_default_state(sock_param[i]->sock_id);
	}
	if(is_all_socket_exit())
		goto END;

	if (at_rcv_thread_id == NULL)
	{

		thread_attr.name	   = "sck_rcv";
		thread_attr.priority   = XY_OS_PRIO_NORMAL2;
		thread_attr.stack_size = 0x800;
		at_rcv_thread_id = osThreadNew((osThreadFunc_t)(at_sock_recv_thread), NULL, &thread_attr);
	}
	else
	{
		softap_printf(USER_LOG, WARN_LOG, "udp_recv_thread_no_creat");
	}
END:
	return;
}

void init_udp_backup_state(int offset)
{
	if(g_udp_context == NULL)
	{
		g_udp_context = xy_zalloc(sizeof(udp_context_t));
	}
	if(xy_ftl_read(NV_FLASH_NET_BASE + offset, (unsigned char *)g_udp_context, sizeof(udp_context_t)) == XY_ERR)
	{
		xy_free(g_udp_context);
		g_udp_context = NULL;
		softap_printf(USER_LOG, WARN_LOG,"read udp data fail");
		return;
	}
	softap_printf(USER_LOG, WARN_LOG,"read udp data success");
	
}

void creat_udp_backup_context()
{
	if(at_rcv_thread_id != NULL || g_udp_context == NULL)
	{
		softap_printf(USER_LOG, WARN_LOG,"socket  is exited,not backup_socket !!");
		return;
	}
	
	if( g_backup_udp_once == -1)
	{
		restore_udp_context_thread();
		g_backup_udp_once = 0;
		return;
	}

}

void resume_socket_app(int offset)
{
    init_udp_backup_state(offset);
	creat_udp_backup_context();
}

/**
 * @brief 检测新创建的socket有效性
 * @param  flag  0:检测主机端口号和网络类型是否与socket上下文记录重复;1:检测服务端口/地址/网络类型是否与socket上下文记录重复
 * @return XY_OK 表示新建socket有效；XY_ERR表示新建socket与已知socket发生冲突
 */
int check_socket_valid(unsigned char net_type, unsigned short remote_port, char *remote_ip, unsigned short local_port, int flag)
{
	int i;
	if (flag)
	{
		for (i = 0; i < SOCK_NUM; i++)
		{
			if (sock_ctx[i] != NULL)
			{
				if (sock_ctx[i]->net_type == net_type && sock_ctx[i]->remote_port == remote_port && !strcmp(remote_ip, sock_ctx[i]->remote_ip))
					return XY_ERR;
			}
		}
	}
	else
	{
		for (i = 0; i < SOCK_NUM; i++)
		{
			if (sock_ctx[i] != NULL)
			{
				if (sock_ctx[i]->net_type == net_type && sock_ctx[i]->local_port == local_port && local_port != 0)
					return XY_ERR;
			}
		}
	}

	return XY_OK;
}

void netif_down_close_socket(uint32_t eventId)
{
	UNUSED_ARG(eventId);
	int idx = 0;
    osMutexAcquire(g_socket_mux, osWaitForever);
	for (idx = 0; idx < SOCK_NUM; idx++)
	{
		if (sock_ctx[idx] != NULL && sock_ctx[idx]->fd >= 0)
		{
			sock_ctx[idx]->is_deact = 1;
            softap_printf(USER_LOG, WARN_LOG, "socket[%d] quit set 1!!!", idx);
        }		
	}
    osMutexRelease(g_socket_mux);
}

void socket_init()
{	
	g_socket_mux = osMutexNew(NULL);
	sninfo_mux = osMutexNew(NULL);
    g_data_send_mode = g_softap_fac_nv->data_format & 0x1;
    g_data_recv_mode = (g_softap_fac_nv->data_format & 0x2) >> 1;
}

#endif //AT_SOCKET
