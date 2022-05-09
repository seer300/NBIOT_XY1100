#include "softap_macro.h"

#if AT_SOCKET
#include "at_com.h"
#include "at_global_def.h"
#include "at_socket.h"
#include "at_socket_context.h"
#include "lwip/sockets.h"
#include "net_app_resume.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "xy_utils.h"

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
/**
 * @brief 获取当前socket缓存中的数据大小
 */
int get_remaining_socket_buffer_size(int socket_ctx_id)
{
	if (sock_ctx[socket_ctx_id] != NULL && sock_ctx[socket_ctx_id]->data_list != NULL)
	{
		return sock_ctx[socket_ctx_id]->data_list->len;
	}
	return 0;
}

/**
 * @brief 从给定socket缓存中读取指定长度数据
 * @note 若指定长度小于buffer中数据长度，读取指定长度数据，相应地减少buffer数据长度
 * @note 若指定长度大于等于buffer中数据长度，读取buffer中所有数据
 */
void read_data_from_socket_buffer(int skt_index, int *pread_len, char **prsp_cmd)
{
	osMutexAcquire(g_socket_mux, osWaitForever);

	recv_data_node_t *temp = sock_ctx[skt_index]->data_list;
	char *rcv_data = NULL;
	char *remote_ip = xy_zalloc(REMOTE_SERVER_LEN);

	if (temp != NULL)
	{
		*prsp_cmd = xy_zalloc(120 + temp->len * 2);

		if (sock_ctx[skt_index]->af_type == 1)
			inet_ntop(AF_INET6, &(temp->sockaddr_info->sin_addr), remote_ip, REMOTE_SERVER_LEN);
		else
			inet_ntop(AF_INET, &(temp->sockaddr_info->sin_addr), remote_ip, REMOTE_SERVER_LEN);

		int old_i = ntohs(temp->sockaddr_info->sin_port);

		if (temp->len <= *pread_len)
		{
#if VER_QUECTEL
			sprintf(*prsp_cmd, "\r\n%d,%s,%d,%d,", skt_index, remote_ip, old_i, temp->len);
#else
			sprintf(*prsp_cmd, "\r\n+NSORF:%d,%s,%d,%d,", skt_index, remote_ip, old_i, temp->len);
#endif
			bytes2hexstr(temp->data, temp->len, *prsp_cmd + strlen(*prsp_cmd), temp->len * 2 + 1);

			sock_ctx[skt_index]->data_list = temp->next;
			if (temp->data)
				xy_free(temp->data);
			if (temp->sockaddr_info)
				xy_free(temp->sockaddr_info);
			xy_free(temp);

			sprintf(*prsp_cmd + strlen(*prsp_cmd), ",%d\r\n", get_remaining_socket_buffer_size(skt_index));
			sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");

			if (g_at_sck_report_mode == BUFFER_WITH_HINT && (sock_ctx[skt_index]->data_list != NULL))
			{
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+NSONMI:%d,%d\r\n", skt_index, sock_ctx[skt_index]->data_list->len);
			}
			else if ((sock_ctx[skt_index]->data_list) == NULL)
			{
				sock_ctx[skt_index]->firt_recv = 0;
			}
		}
		else
		{
			rcv_data = xy_zalloc(temp->len - *pread_len + 1);

			memcpy(rcv_data, temp->data + *pread_len, temp->len - *pread_len);
#if VER_QUECTEL
			sprintf(*prsp_cmd, "\r\n%d,%s,%d,%d,", skt_index, remote_ip, old_i, *pread_len);
#else
			sprintf(*prsp_cmd, "\r\n+NSORF:%d,%s,%d,%d,", skt_index, remote_ip, old_i, *pread_len);
#endif
			bytes2hexstr(temp->data, *pread_len, *prsp_cmd + strlen(*prsp_cmd), *pread_len * 2 + 1);

			xy_free(temp->data);

			temp->data = rcv_data;
			temp->len = temp->len - *pread_len;

			sprintf(*prsp_cmd + strlen(*prsp_cmd), ",%d\r\n", get_remaining_socket_buffer_size(skt_index));
			sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
		}
	}
	else
	{
		*pread_len = 0;
	}
	xy_free(remote_ip);
	osMutexRelease(g_socket_mux);
}

/**
 * @brief  at命令内部调用的tcp数据发送接口
 * @param  at_buf  at命令参数
 * @param  prsp_cmd at命令返回指针
 * @param  withrai 是否携带rai标志，1表示携带，0表示不携带
 * @param  is_ext  是否是拓展命令，1表示拓展命令，0表示非拓展命令
 * @note tcp发送rai标志是无效的，暂不支持
 */
static void at_socket_send_tcp(char *at_buf, char **prsp_cmd, bool withrai, bool is_ext)
{
	xy_assert(at_buf != NULL);
	(void)withrai;
	if (!ps_netif_is_ok()) //检测网卡是否激活
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		return;
	}

	int16_t sequence_no = -1;
	int32_t socket_index = -1;
	int32_t socket_ctx_index = -1;
	int32_t rai_type = RAI_DATA_EXCEPTION; //默认值
	uint32_t data_len = 0;
	uint32_t pre_sn = 0;
	char *hex_stream_data = NULL; //"1A1B1C" => '\1A''\1B''\1C'
	char rai_flag[7] = {0};
	char *hex_str_data = xy_zalloc(strlen(at_buf));
	socket_context_t *socket_ctx = NULL;

	if (is_ext)
	{
		 //AT+NSOSDEX=<socket>,<flag>,<sequece>,<length>,<data>
		if (at_parse_param("%d,%7s,%2d,%d,%s", at_buf, &socket_index, rai_flag, &sequence_no, &data_len, hex_str_data) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
	}
	else
	{
		 //AT+NSOSD=<socket>,<length>,<data>[,<flag>[,<sequence>]]	
		if (at_parse_param("%d,%d,%s,%7s,%2d", at_buf, &socket_index, &data_len, hex_str_data, rai_flag, &sequence_no) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
	}

	//参数检测
	if (socket_index < 0 || socket_index >= SOCK_NUM)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}

	if (data_len > AT_SOCKET_MAX_DATA_LEN || 2 * data_len != (int)strlen(hex_str_data))
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}
#if VER_QUECTEL
	if (sequence_no > SEQUENCE_MAX || sequence_no == 0 || sequence_no < -1 || check_same_sequence(socket_index, (unsigned char)sequence_no) == 1)
#else
	if (sequence_no > SEQUENCE_MAX || sequence_no < -1 || check_same_sequence(socket_index, (unsigned char)sequence_no) == 1)
#endif
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}
    if (sequence_no == -1)
        sequence_no = 0;
    if ((socket_ctx_index = find_sock_ctx_id_by_sock_id(socket_index)) == -1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto END_PROC;
	}

	socket_ctx = sock_ctx[socket_ctx_index];

	if (socket_ctx == NULL || socket_ctx->net_type != 0) //UDP
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}

	if (data_len > 0)
	{
		hex_stream_data = xy_zalloc(data_len);
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}

	if (hexstr2bytes(hex_str_data, data_len * 2, hex_stream_data, data_len) == -1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}

	if (!strcmp(rai_flag, "0X100") || !strcmp(rai_flag, "0x100") || !strcmp(rai_flag, "0"))
	{
		rai_type = RAI_DATA_EXCEPTION;
	}
	else if (!strcmp(rai_flag, "0X200") || !strcmp(rai_flag, "0x200"))
	{
		rai_type = RAI_DATA_REL_UP;
	}
	else if (!strcmp(rai_flag, "0X400") || !strcmp(rai_flag, "0x400"))
	{
		rai_type = RAI_DATA_REL_DOWN;
	}
	else if (strlen(rai_flag) > 0)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
    }
    else if (strlen(rai_flag) == 0)
    {
        /* AT+NSOSDEX中rai_flag为必选参数,无参输入报错; AT+NSOSD中rai_flag为可选参数, 不能因为无参就报错 */
        if (is_ext || (!is_ext && at_strnchr(at_buf, ',', 3) != NULL))
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto END_PROC;
        }  
    }

    if (sequence_no != 0)
	{
		ioctl(socket_ctx->fd, FIOREADSN, &pre_sn);
	}

	//if have remote AT Req,will replace data by AT response for TEST
	xy_Remote_AT_Rsp(&hex_stream_data, &data_len);

	int ret = send2(socket_ctx->fd, hex_stream_data, data_len, 0, sequence_no, rai_type);
	if (ret <= 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "socket[%d] tcp send errno:%d", socket_ctx_index, errno);
		if (errno == ENOTCONN)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_CONN_NOT_CONNECTED);
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			if (sequence_no != 0)
			{
				socket_ctx->sequence_state[sequence_no - 1] = SEND_STATUS_FAILED;
			}
		}
		goto END_PROC;
	}

	socket_ctx->sended_size += data_len;
	if (sequence_no != 0)
	{
		add_sninfo_node(socket_index, data_len, sequence_no, pre_sn);
		socket_ctx->sequence_state[sequence_no - 1] = SEND_STATUS_SENDING;
	}

	*prsp_cmd = xy_malloc(32);
	snprintf(*prsp_cmd, 32, "\r\n%d,%d\r\n\r\nOK\r\n", socket_ctx->sock_id, data_len);

END_PROC:
	if (hex_stream_data)
		xy_free(hex_stream_data);
	if (hex_str_data)
		xy_free(hex_str_data);
	return;
}

/**
 * @brief  at命令内部调用的udp数据发送接口
 * @param  at_buf  at命令参数
 * @param  prsp_cmd at命令返回指针
 * @param  withrai 是否携带rai标志，1表示携带，0表示不携带
 * @param  is_ext  是否是拓展命令，1表示拓展命令,0表示非拓展命令
 * @note rai标志位 @see @ref SOCKET_RAI_TYPE
 */
static void at_socket_send_udp(char *at_buf, char **prsp_cmd, bool withrai, bool is_ext)
{
	xy_assert(at_buf != NULL);

	if (!ps_netif_is_ok()) //检测网卡是否激活
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		return;
	}

	int32_t socket_index = -1;
	int32_t socket_ctx_index = -1;
	int32_t rai_type = RAI_DATA_EXCEPTION; //默认值
	uint32_t data_len = 0;
	int16_t sequence_no = -1;
	int32_t remote_port = -1;
	char *hex_stream_data = NULL;   //"1A1B1C" => '\1A''\1B''\1C'
	char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
	char rai_flag[7] = {0};
	char *hex_str_data = xy_zalloc(strlen(at_buf));
	socket_context_t *socket_ctx = NULL;
    struct sockaddr_in remote_sockaddr = {0};
    struct sockaddr_in6 remote_v6_sockaddr = {0};

    if (!is_ext && !withrai) 
	{
		//AT+NSOST=<socket>,<remote_addr>,<remote_port>, <length>,<data>[,<sequence>]
		if (at_parse_param("%d,%47s,%d,%d,%s,%2d", at_buf, &socket_index, remote_addr,
						   &remote_port, &data_len, hex_str_data, &sequence_no) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
	}
	else if (is_ext && !withrai)
	{
		//AT+NSOSTEX=<socket>,<remote_addr>,<remote_port>,<sequence>,<length>,<data>
		if (at_parse_param("%d,%47s,%d,%2d,%d,%s", at_buf, &socket_index, remote_addr,
						   &remote_port, &sequence_no, &data_len, hex_str_data) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
	}
	else if (!is_ext && withrai)
	{
		//AT+NSOSTF=<socket>,<remote_addr>,<remote_port>,<flag>,<length>,<data>[,<sequence>]
		if (at_parse_param("%d,%47s,%d,%7s,%d,%s,%2d", at_buf, &socket_index, remote_addr,
						   &remote_port, rai_flag, &data_len, hex_str_data, &sequence_no) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
	}
	else if (is_ext && withrai)
	{
		//AT+NSOSTFEX=<socket>,<remote_addr>,<remote_port>,<flag>,<sequence>,<length>,<data>
		if (at_parse_param("%d,%47s,%d,%7s,%2d,%d,%s", at_buf, &socket_index, remote_addr,
						   &remote_port, rai_flag, &sequence_no, &data_len, hex_str_data) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
	}

	//参数检测
	if (socket_index < 0 || socket_index >= SOCK_NUM)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}
	if (data_len > AT_SOCKET_MAX_DATA_LEN || data_len * 2 != (int)strlen(hex_str_data) || data_len <= 0)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}
#if VER_QUECTEL
	if (sequence_no > SEQUENCE_MAX || sequence_no == 0 || sequence_no < -1 || check_same_sequence(socket_index, (unsigned char)sequence_no) == 1)
#else
	if (sequence_no > SEQUENCE_MAX || sequence_no < -1 || check_same_sequence(socket_index, (unsigned char)sequence_no) == 1)
#endif
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}
    if (sequence_no == -1)
        sequence_no = 0;
	if (remote_port > 65535 || remote_port < 0)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}
	if ((socket_ctx_index = find_sock_ctx_id_by_sock_id(socket_index)) == -1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto END_PROC;
	}
	if (withrai)
	{
		if (!strcmp(rai_flag, "0X100") || !strcmp(rai_flag, "0x100") || !strcmp(rai_flag, "0"))
		{
			rai_type = RAI_DATA_EXCEPTION;
		}
		else if (!strcmp(rai_flag, "0X200") || !strcmp(rai_flag, "0x200"))
		{
			rai_type = RAI_DATA_REL_UP;
		}
		else if (!strcmp(rai_flag, ("0X400")) || !strcmp(rai_flag, "0x400"))
		{
			rai_type = RAI_DATA_REL_DOWN;
		}
		else if (strlen(rai_flag) >= 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
	}

	socket_ctx = sock_ctx[socket_index];
	if (socket_ctx == NULL)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}

	if (socket_ctx->net_type != 1) //TCP
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}
    else
    {
        /* 检测remote ipaddr字符串地址有效性 */
        if ((socket_ctx->af_type == 0 && xy_ipaddr_check(remote_addr, IPV4_TYPE) != XY_OK) ||
            (socket_ctx->af_type == 1 && xy_ipaddr_check(remote_addr, IPV6_TYPE) != XY_OK))
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto END_PROC;
        }
    }
    
	if (socket_ctx->remote_ip != NULL)
		xy_free(socket_ctx->remote_ip);

	socket_ctx->remote_ip = xy_zalloc(strlen(remote_addr) + 1);	
	strcpy(socket_ctx->remote_ip, remote_addr);
	socket_ctx->remote_port = (unsigned short)remote_port;

    if (socket_ctx->af_type == 1)  //ipv6
    {
        remote_v6_sockaddr.sin6_family = AF_INET6;
        remote_v6_sockaddr.sin6_port = htons(socket_ctx->remote_port);
        if (1 != inet_pton(AF_INET6, socket_ctx->remote_ip, &remote_v6_sockaddr.sin6_addr))
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto END_PROC;
        }
    }
    else
    {
        remote_sockaddr.sin_family = AF_INET;
        remote_sockaddr.sin_port = htons(socket_ctx->remote_port);
        if (1 != inet_pton(AF_INET, socket_ctx->remote_ip, &remote_sockaddr.sin_addr))
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto END_PROC;
        }
    }

	hex_stream_data = xy_zalloc(data_len);
	if (hexstr2bytes(hex_str_data, data_len * 2, hex_stream_data, data_len) == -1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto END_PROC;
	}

	//if have remote AT Req,will replace data by AT response for TEST
	xy_Remote_AT_Rsp(&hex_stream_data, &data_len);

    int ret = -1;
    if (socket_ctx->af_type == 1)
    {
        ret = sendto2(socket_ctx->fd, hex_stream_data, data_len, 0, (struct sockaddr *)&remote_v6_sockaddr, sizeof(struct sockaddr_in6), sequence_no, rai_type);
    }
    else
    {
        ret = sendto2(socket_ctx->fd, hex_stream_data, data_len, 0, (struct sockaddr *)&remote_sockaddr, sizeof(struct sockaddr_in), sequence_no, rai_type);
    }

    if (ret <= 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "socket[%d] udp send errno:%d", socket_index, errno);
		if (errno == EHOSTUNREACH)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			if (sequence_no != 0)
			{
				socket_ctx->sequence_state[sequence_no - 1] = SEND_STATUS_FAILED;
			}
		}
		goto END_PROC;
	}

	if (sequence_no != 0)
	{
		add_sninfo_node(socket_index, data_len, sequence_no, 0);
		socket_ctx->sequence_state[sequence_no - 1] = SEND_STATUS_SENDING;
	}
	socket_ctx->sended_size += data_len;

	*prsp_cmd = xy_malloc(32);
#if VER_QUECTEL
	snprintf(*prsp_cmd, 32, "\r\n%d,%d\r\n\r\nOK\r\n", socket_ctx->sock_id, data_len);
#else
	if (is_ext && withrai)
		snprintf(*prsp_cmd, 32, "\r\n+NSOSTFEX:%d,%d\r\n\r\nOK\r\n", socket_ctx->sock_id, data_len);
	else if (is_ext && !withrai)
		snprintf(*prsp_cmd, 32, "\r\n+NSOSTEX:%d,%d\r\n\r\nOK\r\n", socket_ctx->sock_id, data_len);
	else if (!is_ext && withrai)
		snprintf(*prsp_cmd, 32, "\r\n+NSOSTF:%d,%d\r\n\r\nOK\r\n", socket_ctx->sock_id, data_len);
	else
		snprintf(*prsp_cmd, 32, "\r\n+NSOST:%d,%d\r\n\r\nOK\r\n", socket_ctx->sock_id, data_len);
#endif
	goto OK_END;

END_PROC:
	if (socket_ctx != NULL && socket_ctx->remote_ip != NULL)
	{
		xy_free(socket_ctx->remote_ip);
		socket_ctx->remote_ip = NULL;
	}
OK_END:
	if (hex_stream_data != NULL)
		xy_free(hex_stream_data);
	if (hex_str_data != NULL)
		xy_free(hex_str_data);
	return;
}

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
//AT+NSOCR=<type>,<protocol>,<listenport>[,<receive control>,<af type>,<ip addr>]
int at_NSOCR_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char type[7] = {0};
		char af_type[10] = {0};
		char protocol = 0;
		char ipaddr[XY_IPADDR_STRLEN_MAX] = {0};
		int receive_control = 1;
	    int ctx_id = -1;
	    int ret = 0;
	    int fd = -1;
		int local_port = -1;
		socket_context_t *socket_ctx = xy_zalloc(sizeof(socket_context_t));

#if !VER_QUECTEL		
		if (!ps_netif_is_ok()) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto END_PROC;
		}
#endif //VER_QUECTEL

		if (at_parse_param("%7s,%1d,%d,%d,%10s,%47s", at_buf, type, &protocol, &local_port, &receive_control, af_type, ipaddr) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}

        if (!strncmp(af_type, "AF_INET6", strlen("AF_INET6")) && strlen(af_type) == strlen("AF_INET6"))
        {
			socket_ctx->af_type = 1; //ipv6
		}
        else if ((!strncmp(af_type, "AF_INET", strlen("AF_INET")) && strlen(af_type) == strlen("AF_INET")) || strlen(af_type) == 0)
        {
			socket_ctx->af_type = 0;  //ipv4
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}

#if VER_QUECTEL
        if (strlen(ipaddr) > 0)
        {
            if (socket_ctx->af_type == 1)
            {
                if (xy_ipaddr_check(ipaddr, IPV6_TYPE) != XY_OK)
                {
                    *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
                    goto END_PROC;
                }
            }
            else
            {
                if (xy_ipaddr_check(ipaddr, IPV4_TYPE) != XY_OK)
                {
                    *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
                    goto END_PROC;
                }
            }
        }
#endif //VER_QUECTEL

        if (local_port < 0 || local_port > 65535 || (receive_control != 0 && receive_control != 1))
        {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}	
		socket_ctx->local_port = (unsigned short)local_port;

		if ((protocol != IPPROTO_UDP || strncmp(type,"DGRAM",strlen("DGRAM"))) && \
			(protocol != IPPROTO_TCP || strncmp(type,"STREAM",strlen("STREAM")))) //check protocol and type param
	    {
	    	*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        goto END_PROC;
	    }

        socket_ctx->net_type = (protocol == IPPROTO_UDP) ? 1 : 0;
		if (check_socket_valid(socket_ctx->net_type, socket_ctx->remote_port, socket_ctx->remote_ip, socket_ctx->local_port, 0) != XY_OK)
		{
	    	*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	        goto END_PROC;
	    }
		
	    if ((ctx_id = get_free_sock_id()) < 0)
	    {
	        //no availabe socket context, all used!
	        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto  END_PROC;
	    }
		
	    socket_ctx->sock_id = (unsigned char)ctx_id;
        if ((ret = at_socket_create(socket_ctx, &fd)) != AT_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ret);
            goto END_PROC;
        }
        if (socket_ctx->net_type != 0)
        {
            //udp set socket recv/send non-block
            nonblock(fd);
        }
			
		socket_ctx->fd = fd;
		socket_ctx->recv_ctl = receive_control;
        get_local_port_by_fd(fd, &(socket_ctx->local_port));
        sock_ctx[ctx_id] = socket_ctx;	
		set_socket_sequence_default_state(socket_ctx->sock_id);
        if (at_rcv_thread_id == NULL)
        {
            osThreadAttr_t thread_attr = {0};
            thread_attr.name	   = "sck_rcv";
			thread_attr.priority   = XY_OS_PRIO_NORMAL1;
			thread_attr.stack_size = 0x800;
			at_rcv_thread_id       = osThreadNew((osThreadFunc_t)(at_sock_recv_thread), NULL, &thread_attr);
		}
	    *prsp_cmd = xy_zalloc(24);
#if VER_QUECTEL
		snprintf(*prsp_cmd, 24, "\r\n%d\r\n\r\nOK\r\n", socket_ctx->sock_id);
#else
		snprintf(*prsp_cmd, 24, "\r\n+NSOCR:%d\r\n\r\nOK\r\n", socket_ctx->sock_id);
#endif //VER_QUECTEL
        return AT_END;
END_PROC:
		if (socket_ctx != NULL)
			xy_free(socket_ctx);
		return AT_END;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSOCO=<socket>,<remote_addr>,<remote_port>; only connect TCP
int at_NSOCO_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int socketid = -1;
		int idx = -1;
		char *remote_ip = xy_zalloc(strlen(at_buf));
		int remote_port = 0;	
		
		if (!ps_netif_is_ok()) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto END_PROC;
		}

		if (at_parse_param("%d,%40s,%d", at_buf, &socketid, remote_ip, &remote_port) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}

		if (socketid == -1 || !strcmp(remote_ip, ""))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}

		if (remote_port > 65535 || remote_port <= 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}

		if ((idx = find_sock_ctx_id_by_sock_id(socketid)) == -1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END_PROC;
		}

		if (sock_ctx[idx]->remote_ip != NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END_PROC;
		}

		if (sock_ctx[idx]->net_type == 1)  //udp
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END_PROC;
		}
#if VER_QUECTEL
        else
        {
            /* 检测remote ipaddr字符串地址有效性 */
            if ((sock_ctx[idx]->af_type == 0 && xy_ipaddr_check(remote_ip, IPV4_TYPE) != XY_OK) ||
                (sock_ctx[idx]->af_type == 1 && xy_ipaddr_check(remote_ip, IPV6_TYPE) != XY_OK))
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
                goto END_PROC;
            }
        }
#endif //VER_QUECTEL
        		
		sock_ctx[idx]->remote_port = (unsigned short)remote_port;
        sock_ctx[idx]->remote_ip = xy_zalloc(strlen(remote_ip) + 1);
        memcpy(sock_ctx[idx]->remote_ip, remote_ip, strlen(remote_ip));

        if (connect_network(sock_ctx[idx]) == 0)
        {
#if !VER_QUECTEL
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
#endif //VER_QUECTEL
			if (at_rcv_thread_id != NULL && sock_ctx[idx] != NULL && sock_ctx[idx]->quit != 1)
			{
				sock_ctx[idx]->quit = 1;
                softap_printf(USER_LOG, WARN_LOG, "socket[%d] connect fail and quit flag set 1!!!", idx);
#if !VER_QUECTEL
                while (sock_ctx[idx] != NULL && sock_ctx[idx]->fd >= 0)
                {
					osDelay(100);
				}
#endif //VER_QUECTEL
			}
			else
			{
                softap_printf(USER_LOG, WARN_LOG, "socket[%d] connect fail but ctx is NULL!!!", idx);
            }
			goto END_PROC;
		}
		
		nonblock(sock_ctx[idx]->fd);
		xy_free(remote_ip);
		return AT_END;
		
END_PROC:
		if(remote_ip != NULL)
			xy_free(remote_ip);	
		return AT_END;	
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
}

//AT+NSOSD=<socket>,<length>,<data>[,<flag>[,<sequence>]],only used for TCP,sequence must >0
int at_NSOSD_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
		at_socket_send_tcp(at_buf, prsp_cmd, true, false);
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSONMI=<mode>
int at_NSONMI_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int report_mode_temp = -1;
		if (at_parse_param("%d", at_buf, &report_mode_temp) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if ((report_mode_temp < BUFFER_NO_HINT) || (report_mode_temp > HINT_NO_REMOTE_INFO))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		g_at_sck_report_mode = report_mode_temp;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(24);
		snprintf(*prsp_cmd, 24, "\r\n+NSONMI:%d\r\n\r\nOK\r\n", g_at_sck_report_mode);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+NSONMI:(0,1,2,3)\r\n\r\nOK\r\n");
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
	return AT_END;
}

//AT+NSOST=<socket>,<remote_addr>,<remote_port>,<length>,<data>[,<sequence>],only used for UDP,sequence must >0
int at_NSOST_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
		at_socket_send_udp(at_buf, prsp_cmd, false, false);
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSOSTF=<socket>,<remote_addr>,<remote_port>,<flag>,<length>,<data>[,<sequence>],only used for UDP,sequence must >0
int at_NSOSTF_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
		at_socket_send_udp(at_buf, prsp_cmd, true, false);
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSORF=<socket>,<req_length>,<socket>,<ip_addr>,<port>,<length>,<data>,<remaining_length>  OK
int at_NSORF_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int sock_id = 0;
		int req_len = 0;
		int idx;
		recv_data_node_t *old_data = NULL;

		if (at_parse_param("%d,%d", at_buf, &sock_id, &req_len) != AT_OK || sock_id < 0 || sock_id >= SOCK_NUM || req_len <= 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

#if VER_QUECTEL
        if (req_len > AT_SOCKET_MAX_DATA_LEN)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }
#endif //VER_QUECTEL

		int socket_ctx_id = find_sock_ctx_id_by_sock_id(sock_id);
		if (socket_ctx_id == -1 || sock_ctx[socket_ctx_id] == NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}

        if (sock_ctx[socket_ctx_id]->data_list == NULL || sock_ctx[socket_ctx_id]->data_list->len == 0)
        {
#if !VER_QUECTEL
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#endif //VER_QUECTEL
            return AT_END;
        }

        //从缓存buffer中读取指定长度数据
		read_data_from_socket_buffer(socket_ctx_id, &req_len, prsp_cmd);

		if (req_len == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSOCL=<socket>
int at_NSOCL_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int sock_ctx_id;
		int sock_id = 0;

		if (at_parse_param("%d", at_buf, &sock_id) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if ((sock_ctx_id = find_sock_ctx_id_by_sock_id(sock_id)) == -1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}

		if (sock_ctx[sock_ctx_id] != NULL && sock_ctx[sock_ctx_id]->fd >= 0)
		{
			sock_ctx[sock_ctx_id]->quit = 1;
            softap_printf(USER_LOG, WARN_LOG, "try to close socket[%d] and quit flag set 1!!!", sock_ctx_id);
        }
		//等待socket接收线程关闭
		while (sock_ctx[sock_ctx_id] != NULL && sock_ctx[sock_ctx_id]->fd >= 0)
		{
			osDelay(100);
		}
        softap_printf(USER_LOG, WARN_LOG, "socket[%d] ctx remove success!!!", sock_ctx_id);
#if !VER_QUECTEL
		*prsp_cmd = xy_zalloc(36);
	    snprintf(*prsp_cmd, 36, "\r\nOK\r\n\r\n+NSOCLI:%d\r\n", sock_id);
#endif
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NQSOS=<socket_id>[,<socket_id>][,<socket_id>…]
int at_NQSOS_req(char* at_buf, char** prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint32_t i = 0;
        uint32_t j = 0;
        int32_t socketid_array[SOCK_NUM] = {0};
        for (i = 0; i < SOCK_NUM; i++)
        {
            socketid_array[i] = -1;
        }
        sequence_node_t seq_nodes[SOCK_NUM] = {0};

        /* eg:AT+NQSOS=0,1,2,3,4,5,6,7-->error */
        if (at_strnchr(at_buf, ',', SOCK_NUM) != NULL)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }
#if	VER_QUECTEL
        if (at_parse_param("%d(0-6),%d(0-6),%d(0-6),%d(0-6),%d(0-6),%d(0-6),%d(0-6)", at_buf, &socketid_array[0], &socketid_array[1],
                  &socketid_array[2], &socketid_array[3], &socketid_array[4], &socketid_array[5], &socketid_array[6]) != AT_OK)
#else 
        if (at_parse_param("%d(0-1),%d(0-1)", at_buf, &socketid_array[0], &socketid_array[1]) != AT_OK)
#endif //VER_QUECTEL
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }

		for (i = 0; i < SOCK_NUM; i++)
		{
			if (socketid_array[i] != -1)
			{
				seq_nodes[i].socket_ctx_id = find_sock_ctx_id_by_sock_id(socketid_array[i]);
				seq_nodes[i].socket_id = socketid_array[i];
			}
			else
			{
				seq_nodes[i].socket_ctx_id = -1;
				seq_nodes[i].socket_id = -1;
			}
		}

        /* socket参数有效性检测 */
        for (i = 0; i < SOCK_NUM; i++)
        {
            for (j = i + 1; j < SOCK_NUM; j++)
            {
                /* socket参数重复检测 eg:AT+NQSOS=0,1,2,2 */
                if (seq_nodes[i].socket_id != -1 && seq_nodes[i].socket_id == seq_nodes[j].socket_id)
                {
                    *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
                    return AT_END;
                }
            }
        }

		int current_size = 100;
		int rspok_size = strlen("\r\nOK\r\n") + 1;
		int maxUrc_size = strlen("\r\n+NQSOS:9,255\r\n"); //NQSOS URC上报的最大长度
		*prsp_cmd = xy_zalloc(current_size);			  //初始分配100字节，后续动态realloc，避免静态分配可能造成的内存浪费
		for (i = 0; i < SOCK_NUM; i++)
		{
			if (seq_nodes[i].socket_ctx_id != -1 && seq_nodes[i].socket_id != -1)
			{
				int j = 0;
				for (j = 0; j < SEQUENCE_MAX; j++)
				{
					if (sock_ctx[seq_nodes[i].socket_ctx_id]->sequence_state[j] == SEND_STATUS_SENDING)
					{
						int offset = strlen(*prsp_cmd);
						//计算满足NQSOS命令正常返回的长度，需包含所有上报的NQSOS信息及OK信息的长度
						//如果不包含OK信息长度,可能导致OK信息无法返回，出现8007错误
						int valid_len = offset + rspok_size + maxUrc_size + 1;
						if (valid_len > current_size)
						{
							char *new_lloc = xy_zalloc(current_size + 100);
							current_size += 100;
							softap_printf(USER_LOG, WARN_LOG, "NQSOS realloc:%d", current_size);
							memcpy(new_lloc, *prsp_cmd, offset);
							xy_free(*prsp_cmd);
							*prsp_cmd = new_lloc;
						}
#if	VER_QUECTEL
						snprintf(*prsp_cmd + offset, 20, "\r\n+NQSOS:%d,%d", seq_nodes[i].socket_id, j + 1);
#else
						snprintf(*prsp_cmd + offset, 20, "\r\n+NQSOS:%d,%d\r\n", seq_nodes[i].socket_id, j + 1);
#endif //VER_QUECTEL
					}
				}
			}
		}
#if	VER_QUECTEL
        if (strlen(*prsp_cmd) > 0)
		    snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "\r\n\r\nOK\r\n");
        else
#endif //VER_QUECTEL
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "\r\nOK\r\n");
		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		sequence_node_t seq_nodes[SOCK_NUM] = {0};
		int i = 0;
		for (i = 0; i < SOCK_NUM; i++)
		{
			seq_nodes[i].socket_ctx_id = find_sock_ctx_id_by_sock_id(i);
			seq_nodes[i].socket_id = i;
		}
		int current_size = 100;
		int rspok_size = strlen("\r\nOK\r\n") + 1;
		int maxUrc_size = strlen("\r\n+NQSOS:9,255\r\n"); //NQSOS URC上报的最大长度
		*prsp_cmd = xy_zalloc(current_size);			  //初始分配100字节，后续动态realloc，避免静态分配可能造成的内存浪费
		for (i = 0; i < SOCK_NUM; i++)
		{
			if (seq_nodes[i].socket_ctx_id == -1)
				continue;
			else if (seq_nodes[i].socket_ctx_id != -1)
			{
				int j = 0;
				for (j = 0; j < SEQUENCE_MAX; j++)
				{
					if (sock_ctx[seq_nodes[i].socket_ctx_id]->sequence_state[j] == SEND_STATUS_SENDING)
					{
						int offset = strlen(*prsp_cmd);
						//计算满足NQSOS命令正常返回的长度，需包含所有上报的NQSOS信息及OK信息的长度
						//如果不包含OK信息长度,可能导致OK信息无法返回，出现8007错误
						int valid_len = offset + rspok_size + maxUrc_size + 1;
						if (valid_len > current_size)
						{
							char *new_lloc = xy_zalloc(current_size + 100);
							current_size += 100;
							softap_printf(USER_LOG, WARN_LOG, "NQSOS realloc:%d", current_size);
							memcpy(new_lloc, *prsp_cmd, offset);
							xy_free(*prsp_cmd);
							*prsp_cmd = new_lloc;
						}
#if	VER_QUECTEL
						snprintf(*prsp_cmd + offset, 20, "\r\n+NQSOS:%d,%d", seq_nodes[i].socket_id, j + 1);
#else
						snprintf(*prsp_cmd + offset, 20, "\r\n+NQSOS:%d,%d\r\n", seq_nodes[i].socket_id, j + 1);
#endif //VER_QUECTEL
					}
				}
			}
		}
#if	VER_QUECTEL
        if (strlen(*prsp_cmd) > 0)
            snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "\r\n\r\nOK\r\n");
        else
#endif //VER_QUECTEL
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "\r\nOK\r\n");
		return AT_END;		
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
}

//AT+NSOSTATUS=<socket id>
int at_NSOSTATUS_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int socket_id = -1;
		if (at_parse_param("%d", at_buf, &socket_id) != AT_OK || socket_id < 0 || socket_id >= SOCK_NUM)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		*prsp_cmd = xy_zalloc(48);
		if (find_sock_ctx_id_by_sock_id(socket_id) == -1)
		{
			snprintf(*prsp_cmd, 48, "\r\n+NSOSTATUS:%d,%d\r\n\r\nOK\r\n", socket_id, SOCKET_STATE_UNAVAIL);		
		}
		else
		{
			snprintf(*prsp_cmd, 48, "\r\n+NSOSTATUS:%d,%d\r\n\r\nOK\r\n", socket_id, SOCKET_STATE_AVAIL);
		}
	}
	else if (g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_zalloc(160);
		int socket_id;
		for (socket_id = 0; socket_id < SOCK_NUM; socket_id++)
		{
			if (find_sock_ctx_id_by_sock_id(socket_id) == -1)
			{
#if VER_QUECTEL
				snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\n+NSOSTATUS:%d,%d", socket_id, SOCKET_STATE_UNAVAIL);
#else 
				snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\n+NSOSTATUS:%d,%d\r\n", socket_id, SOCKET_STATE_UNAVAIL);
#endif //VER_QUECTEL
			}
			else
			{
#if VER_QUECTEL
				snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\n+NSOSTATUS:%d,%d", socket_id, SOCKET_STATE_AVAIL);
#else
				snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\n+NSOSTATUS:%d,%d\r\n", socket_id, SOCKET_STATE_AVAIL);
#endif //VER_QUECTEL
			}
		}
#if VER_QUECTEL
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\n\r\nOK\r\n");
#else
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\nOK\r\n");
#endif //VER_QUECTEL    
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(48);
		snprintf(*prsp_cmd, 48, "\r\n+NSOSTATUS:(0-%d)\r\n\r\nOK\r\n", SOCK_NUM - 1);
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
	return AT_END;
}

//AT+NSOSDEX=<socket>,<flag>,<sequece>,<length>,<data>
int at_NSOSDEX_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
		at_socket_send_tcp(at_buf, prsp_cmd, true, true);
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSOSTEX=<socket>,<remote_addr>,<remote_port>,<sequence>,<length>,<data>
int at_NSOSTEX_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
		at_socket_send_udp(at_buf, prsp_cmd, false, true);
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSOSTFEX=<socket>,<remote_addr>,<remote_port>,<flag>,<sequence>,<length>,<data>
int at_NSOSTFEX_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
		at_socket_send_udp(at_buf, prsp_cmd, true, true);
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

#endif //AT_SOCKET