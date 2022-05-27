#include "softap_macro.h"

#if AT_SOCKET&&VER_QUCTL260
#include "at_com.h"
#include "at_global_def.h"
#include "at_ps_cmd.h"
#include "at_socket_bc26.h"
#include "at_socket_context.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "net_app_resume.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "xy_at_api.h"
#include "xy_passthrough.h"
#include "xy_system.h"
#include "xy_utils.h"

osThreadId_t passthr_sock_send_thd = NULL;
osMessageQueueId_t passthr_sock_msg_q = NULL;

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct passthr_socket_msg
{
    uint32_t data_len;
    socket_context_t *ctx;
    char data[0];
} passthr_socket_msg_t;

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
static int bc26_get_free_sequence(int sock_ctx_id)
{
    int i = 1;
    for (; i <= 255; i++)
    {
        if (sock_ctx[sock_ctx_id]->sequence_state[i - 1] != SEND_STATUS_SENDING)
        {
            return i;
        }
    }
    return -1;
}

int passthr_socket_data_proc(char *data, int len, void *param)
{
    if (data == NULL || len == 0 || passthr_sock_msg_q == NULL)
        xy_assert(0);

    /* 开始处理透传数据前，需根据发送类型判断已接收数据长度是否合法，比如十六进制格式下数据长度必须可以整除2 */
    int data_len = len;
    if (g_data_send_mode == HEX_ASCII_STRING)
    {
        if (len % 2 != 0)
            return XY_ERR;
        else
            data_len = len / 2;
    }

    /* 数据交由用户线程处理，不得在透传线程中处理 */
    passthr_socket_msg_t *msg = xy_zalloc(sizeof(passthr_socket_msg_t) + len + 1);
    msg->data_len = data_len;
    msg->ctx = (socket_context_t*)param;
    memcpy(msg->data, data, len);
    osMessageQueuePut(passthr_sock_msg_q, &msg, 0, osWaitForever);

    return XY_OK;
}

int bc26_socket_send_data(char *data, uint32_t len, int rai_flag, socket_context_t *ctx)
{
    xy_assert(ctx != NULL);
    softap_printf(USER_LOG, WARN_LOG, "bc26_socket_send_data:%d,%s", len, data);
    int16_t sequence_no = -1;
    uint32_t pre_sn = 0;
    int32_t sock_ctx_id = -1;
    char *hex_data = NULL;

    if ((sock_ctx_id = find_sock_ctx_id_by_sock_id(ctx->sock_id)) == -1)
    {
        softap_printf(USER_LOG, WARN_LOG, "bc26_socket_send_data sock ctx err");
        return XY_ERR;
    }

    /* 获取可用的sequence num */
    if ((sequence_no = bc26_get_free_sequence(sock_ctx_id)) == -1)
    {
        softap_printf(USER_LOG, WARN_LOG, "bc26_socket_send_data find no avail seq");
        return XY_ERR;
    }

    if (g_data_send_mode == HEX_ASCII_STRING)
    {
        if (strlen(data) != len * 2)
        {
            softap_printf(USER_LOG, WARN_LOG, "bc26_socket_send_data len error for hex string");
            return XY_ERR;
        }
        hex_data = xy_zalloc(len);
        if (hexstr2bytes(data, len * 2, hex_data, len) == -1)
        {
            if (hex_data != NULL)
                xy_free(hex_data);
            softap_printf(USER_LOG, WARN_LOG, "bc26_socket_send_data hex2byte error for hex string");
            return XY_ERR;
        }
    }
    else
    {
        if (strlen(data) != len)
        {
            softap_printf(USER_LOG, WARN_LOG, "bc26_socket_send_data len error for ascii string");
            return XY_ERR;
        }
        else
        {
            hex_data = xy_zalloc(len);
            memcpy(hex_data, data, len);
        }
        
    }

    if (sequence_no != 0 && ctx->net_type == 0)
    {
		ioctl(ctx->fd, FIOREADSN, &pre_sn);
	}

    int ret = send2(ctx->fd, hex_data, len, 0, sequence_no, rai_flag);
    if (ret <= 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "socket id[%d] bc26_socket_send_data errno:%d", ctx->sock_id, errno);
        if (sequence_no != 0)
        {
            ctx->sequence_state[sequence_no - 1] = SEND_STATUS_FAILED;
        }
        if (hex_data != NULL)
            xy_free(hex_data);
        return XY_ERR;
    }

    ctx->sended_size += ret;
    if (sequence_no != 0)
    {
        add_sninfo_node(ctx->sock_id, ret, sequence_no, pre_sn);
        ctx->sequence_state[sequence_no - 1] = SEND_STATUS_SENDING;
    }

    if (hex_data != NULL)
        xy_free(hex_data);
        
    return XY_OK;
}

void passthr_socket_send_proc(void* param)
{
    UNUSED_ARG(param);
    passthr_socket_msg_t *msg = NULL;
    while (1)
    {
        osMessageQueueGet(passthr_sock_msg_q, (void *)(&msg), NULL, osWaitForever);
        bc26_socket_send_data(msg->data, msg->data_len, 0, (socket_context_t *)msg->ctx);
        xy_free(msg);
    }
}

void passthrough_socket_init()
{
    if (passthr_sock_msg_q == NULL)
    {
        passthr_sock_msg_q = osMessageQueueNew(10, sizeof(void *), NULL);
    }
    if (passthr_sock_send_thd == NULL)
    {
        osThreadAttr_t thread_attr = {0};
        thread_attr.name = "sck_pthr";
        thread_attr.priority = osPriorityNormal;
        thread_attr.stack_size = 0x800;
        passthr_sock_send_thd = osThreadNew((osThreadFunc_t)passthr_socket_send_proc, NULL, &thread_attr);
    }
}

static void bc26_socket_open_task(void* param)
{
    socket_context_t *ctx = (socket_context_t *)param;
    struct addrinfo hint = {0};
    struct addrinfo *result = NULL;
    char *prsp = xy_zalloc(64);
    int fd = -1;
    int ctx_id = get_free_sock_id();
    int ret = NET_OP_OK;

    if (ctx_id < 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "bc26_socket_open no free socket id");
        ret = NET_OP_NOT_ALLOW;
        goto ERROR_EXIT;
    }

    if (ctx->net_type == 1)
    {
        hint.ai_protocol = IPPROTO_UDP;
        hint.ai_socktype = SOCK_DGRAM;
    }
    else
    {
        hint.ai_protocol = IPPROTO_TCP;
        hint.ai_socktype = SOCK_STREAM;
    }

    hint.ai_family = AF_UNSPEC;
    uint8_t strPort[7] = {0};
    snprintf(strPort, sizeof(strPort) - 1, "%d", ctx->remote_port);

    if (getaddrinfo(ctx->remote_ip, strPort, &hint, &result) != 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "bc26_socket_open getaddrinfo err:%d", errno);
        ret = NET_SOCK_CREATE_ERR;
        goto ERROR_EXIT;
    }
    ctx->af_type = (result->ai_family == AF_INET6) ? 1 : 0;

    /* 创建socket */
    if ((fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
    {
        softap_printf(USER_LOG, WARN_LOG, "bc26_socket_open create err:%d", errno);
        freeaddrinfo(result);
        ret = NET_SOCK_CREATE_ERR;
        goto ERROR_EXIT;
    }
    ctx->fd = fd;

    /* 绑定socket */
    struct sockaddr_in bind_addr = {0};
    bind_addr.sin_family = result->ai_family;
    bind_addr.sin_port = htons(ctx->local_port);
    if (bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1)
    {
        softap_printf(USER_LOG, WARN_LOG, "bc26_socket_open sock bind errno:%d", errno);
        freeaddrinfo(result);
        close(fd);
        ret = NET_SOCK_BIND_ERR;
        goto ERROR_EXIT;
    }

    /* 设置socket接收数据的超时时间 */
    struct timeval tv;
    tv.tv_sec = SOCK_CREATE_TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));

    /* bind时如果local port为0，lwip会随机为local分配一个port，此处获取一下pcb中实际记录的local port */
    get_local_port_by_fd(fd, &(ctx->local_port));

    /* 连接远端socket */
    if (connect(fd, result->ai_addr, result->ai_addrlen) == -1)
    {
        softap_printf(USER_LOG, WARN_LOG, "bc26_socket_open socket connect err:%d", errno);
        freeaddrinfo(result);
        close(fd);
        ret = NET_SOCK_CONNECT_ERR;
        goto ERROR_EXIT;
    }

    nonblock(fd);
    ctx->sock_state = BC26_SOCK_CONNECTED;
    sock_ctx[ctx_id] = ctx;

    /* 建立socket数据收发线程 */
    if (at_rcv_thread_id == NULL)
    {
        osThreadAttr_t thread_attr = {0};
        thread_attr.name = "sck_rcv";
        thread_attr.priority = XY_OS_PRIO_NORMAL1;
        thread_attr.stack_size = 0x800;
        at_rcv_thread_id = osThreadNew((osThreadFunc_t)(at_sock_recv_thread), NULL, &thread_attr);
    }

    freeaddrinfo(result);

ERROR_EXIT:
    snprintf(prsp, 64, "\r\n+QIOPEN: %d,%d\r\n", ctx->sock_id, ret);
    send_urc_to_ext(prsp);
    xy_free(prsp);
    if (ret != NET_OP_OK)
    {
        if (ctx != NULL && ctx->remote_ip != NULL)
            xy_free(ctx->remote_ip);
        if (ctx != NULL)
            xy_free(ctx);
    }
    osThreadExit();
}

//AT+QICFG="dataformat"[,<send_data_format>,<recv_data_format>]
int at_QICFG_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        char tip[12] = {0};
        uint8_t send_mode = 0xFF;
        uint8_t recv_mode = 0xFF;
        uint8_t tip_size = strlen("dataformat");
        if (at_parse_param("%12s,%1d[0-1],%1d[0-1]", at_buf, tip, &send_mode, &recv_mode) != XY_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }

        if (strncmp(tip, "dataformat", tip_size) || strlen(tip) != tip_size)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }

        /* sendmode和recvmode都不输入表示查询 */
        if (send_mode == 0xFF && recv_mode == 0xFF)
        {
            *prsp_cmd = xy_zalloc(64);
            sniprintf(*prsp_cmd, 64, "\r\n+QICFG: \"dataformat\",%d,%d\r\n\r\nOK\r\n", g_data_send_mode, g_data_recv_mode);
            return AT_END;
        }

        if (send_mode != 0xFF)
            g_data_send_mode = send_mode;
        if (recv_mode != 0xFF)
            g_data_recv_mode = recv_mode;

        //Todo:保存nv
        g_softap_fac_nv->data_format = g_data_send_mode | (g_data_recv_mode << 1);
        SAVE_FAC_PARAM(data_format);
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(64);
        sniprintf(*prsp_cmd, 64, "\r\n+QICFG: \"dataformat\",(0,1),(0,1)\r\n\r\nOK\r\n");
    }
    else
    {
        *prsp_cmd = BC26_AT_ERR_BUILD();
    }

    return AT_END;
}

//AT+QIOPEN=<contextID>,<connectID>,<service_type>,<host>,<remote_port>[,<local_port>[,<access_mode>]]
int at_QIOPEN_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        uint8_t working_cid = 0;
        uint8_t socket_id = 0;
        char service_type[8] = {0};
        char *host = NULL;
        uint16_t remote_port = 0;
        uint16_t local_port = 0;
        uint8_t access_mode = 1;

        if (xy_wait_tcpip_ok(0) != XY_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }

        host = xy_zalloc(XY_BC26_SERVER_MAX);
        socket_context_t *socket_ctx = xy_zalloc(sizeof(socket_context_t));

        if (at_parse_param("%1d(0-0),%1d(0-4),%8s,%150s,%2d(0-65535),%2d[0-65535],%1d[1-1]", at_buf, &working_cid, &socket_id, service_type, host, &remote_port, &local_port, &access_mode) != XY_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        if (!strncmp(service_type, "TCP", strlen("TCP")))
        {
            socket_ctx->net_type = 0;
        }
        else if (!strncmp(service_type, "UDP", strlen("UDP")))
        {
            socket_ctx->net_type = 1;
        }
        else
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        if (strlen(host) == 0)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        if (get_free_sock_id() < 0)
        {
            //no availabe socket context, all used!
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        /* 对应socket id已在使用中 */
        if (find_sock_ctx_id_by_sock_id(socket_id) != -1)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        socket_ctx->sock_id = socket_id;
        socket_ctx->accessmode = access_mode;
        socket_ctx->cid = g_working_cid;
        socket_ctx->remote_port = remote_port;
        socket_ctx->local_port = local_port;
        socket_ctx->local_port_ori = local_port;
        socket_ctx->recv_ctl = 1;

        socket_ctx->remote_ip = xy_zalloc(strlen(host) + 1);
        memcpy(socket_ctx->remote_ip, host, strlen(host));
        set_socket_sequence_default_state(socket_ctx->sock_id);

        osThreadAttr_t thread_attr = {0};
        thread_attr.name = "sck_open";
        thread_attr.priority = XY_OS_PRIO_NORMAL1;
        thread_attr.stack_size = 0x800;
        osThreadNew((osThreadFunc_t)(bc26_socket_open_task), socket_ctx, &thread_attr);

        if (host != NULL)
            xy_free(host);
        return AT_END;

END_PROC:
        if (host != NULL)
            xy_free(host);
        if (socket_ctx != NULL)
            xy_free(socket_ctx);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(128);
        sniprintf(*prsp_cmd, 128, "\r\n+QIOPEN: (0-%d),(0-%d),\"TCP/UDP\",<host>,<remote_port>,<local_port>,(0-1)\r\n\r\nOK\r\n", BC26_MAX_CID_NUM, SOCK_NUM - 1);
    }
    else
    {
        *prsp_cmd = BC26_AT_ERR_BUILD();
    }
    return AT_END;
}

//AT+QICLOSE=<connectID>
int at_QICLOSE_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        uint8_t sockId;
        int sock_ctx_id;
        if (at_parse_param("%1d(0-4)", at_buf, &sockId) != XY_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }

        if ((sock_ctx_id = find_sock_ctx_id_by_sock_id(sockId)) == -1)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }

        if (sock_ctx[sock_ctx_id] != NULL && sock_ctx[sock_ctx_id]->fd >= 0)
        {
            sock_ctx[sock_ctx_id]->quit = 1;
            sock_ctx[sock_ctx_id]->sock_state = BC26_SOCK_CLOSING;
            softap_printf(USER_LOG, WARN_LOG, "try to close socket[%d] and quit flag set 1!!!", sock_ctx_id);
        }
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(32);
        sniprintf(*prsp_cmd, 32, "\r\n+QICLOSE: (0-%d)\r\n\r\nOK\r\n", SOCK_NUM - 1);
    }
    else
    {
        *prsp_cmd = BC26_AT_ERR_BUILD();
    }
    return AT_END;
}

//若<query_type>=0,AT+QISTATE=<query_type>,<contextID>
//若<query_type>=1,AT+QISTATE=<query_type>,<connectID>
int at_QISTATE_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        uint8_t query_type = 0;
        int contextId = 0;
        int socketId = 0;
        if (at_parse_param("%1d(0-1)", at_buf, &query_type) != XY_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }

        if (query_type == 0)
        {
            if (at_parse_param(",%d(0-0)", at_buf, &contextId) != XY_OK)
            {
                *prsp_cmd = BC26_AT_ERR_BUILD();
                return AT_END;
            }
            *prsp_cmd = xy_zalloc(320);
            int socket_id = 0;
            int sock_ctx_id = 0;
            for (socket_id = 0; socket_id < SOCK_NUM; socket_id++)
            {
                if ((sock_ctx_id = find_sock_ctx_id_by_sock_id(socket_id)) != -1)
                {
#if 0                
                    if (sock_ctx[sock_ctx_id]->net_type == 0)
                    {
                        snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n+QISTATE: %d,\"TCP\",\"%s\",%d,%d,%d,%d,%d",
                                 socket_id, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, sock_ctx[sock_ctx_id]->local_port_ori,
                                 sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
                    }
                    else
                    {
                        snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n+QISTATE: %d,\"UDP\",\"%s\",%d,%d,%d,%d,%d",
                                 socket_id, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, sock_ctx[sock_ctx_id]->local_port_ori,
                                 sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
                    }
#else
					if (sock_ctx[sock_ctx_id]->net_type == 0)
                    {
                        snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n+QISTATE:%d,\"TCP\",\"%s\",%d,%d,%d,%d",
                                 socket_id, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, 
                                 sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
                    }
                    else
                    {
                        snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n+QISTATE:%d,\"UDP\",\"%s\",%d,%d,%d,%d",
                                 socket_id, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, 
                                 sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
                    }

#endif
                }
            }
            snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n\r\nOK\r\n");
        }
        else
        {
            if (at_parse_param(",%d(0-4)", at_buf, &socketId) != XY_OK)
            {
                *prsp_cmd = BC26_AT_ERR_BUILD();
                return AT_END;
            }
            int sock_ctx_id = 0;
            if ((sock_ctx_id = find_sock_ctx_id_by_sock_id(socketId)) == -1)
            {
                *prsp_cmd = BC26_AT_ERR_BUILD();
                return AT_END;
            }
            *prsp_cmd = xy_zalloc(96);
            if (sock_ctx[sock_ctx_id]->net_type == 0)
            {
                snprintf(*prsp_cmd + strlen(*prsp_cmd), 96, "\r\n+QISTATE: %d,\"TCP\",\"%s\",%d,%d,%d,%d,%d",
                         socketId, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, sock_ctx[sock_ctx_id]->local_port_ori,
                         sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
            }
            else
            {
                snprintf(*prsp_cmd + strlen(*prsp_cmd), 96, "\r\n+QISTATE: %d,\"UDP\",\"%s\",%d,%d,%d,%d,%d",
                         socketId, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, sock_ctx[sock_ctx_id]->local_port_ori,
                         sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
            }
            snprintf(*prsp_cmd + strlen(*prsp_cmd), 96, "\r\n\r\nOK\r\n");
        }
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(64);
        sniprintf(*prsp_cmd, 64, "\r\n+QISTATE: (0,1),(0-%d)\r\n\r\nOK\r\n", SOCK_NUM - 1);
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        //[+QISTATE: <connectID>,<service_type>,<host>,<remote_port>,<local_port>,<socket_state>,<contextID>,<access_mode>]
        *prsp_cmd = xy_zalloc(320);
		int socket_id = 0;
        int sock_ctx_id = 0;
        for (socket_id = 0; socket_id < SOCK_NUM; socket_id++)
		{
			if ((sock_ctx_id = find_sock_ctx_id_by_sock_id(socket_id)) != -1)
            { 
                if (sock_ctx[sock_ctx_id]->net_type == 0)
                {
                    snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n+QISTATE: %d,\"TCP\",\"%s\",%d,%d,%d,%d,%d",
                             socket_id, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, sock_ctx[sock_ctx_id]->local_port_ori,
                             sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
                }
                else
                {
                    snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n+QISTATE: %d,\"UDP\",\"%s\",%d,%d,%d,%d,%d",
                             socket_id, sock_ctx[sock_ctx_id]->remote_ip, sock_ctx[sock_ctx_id]->remote_port, sock_ctx[sock_ctx_id]->local_port_ori,
                             sock_ctx[sock_ctx_id]->sock_state, sock_ctx[sock_ctx_id]->cid, sock_ctx[sock_ctx_id]->accessmode);
                }
            }
        }
        snprintf(*prsp_cmd + strlen(*prsp_cmd), 320, "\r\n\r\nOK\r\n");
    }
    else
    {
        *prsp_cmd = BC26_AT_ERR_BUILD();
    }
    return AT_END;

}

//正常发送数据       AT+QISEND=<connectID>,<send_length>,<data>[,<rai_mode>]
//透传发送定长数据   AT+QISEND=<connectID>,<send_length>
//透传发送不定长数据 AT+QISEND=<connectID>
//查询已发送、已应答，以及已发送但未应 答数据的总长度 AT+QISEND=<connectID>,0
int at_QISEND_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        int socket_id = -1;
        int data_len = -1;
        int socket_ctx_index = -1;
        int rai_flag = 0;
        char *data = xy_zalloc(strlen(at_buf));
        socket_context_t *socket_ctx = NULL;

        if (g_data_send_mode == ASCII_STRING && at_parse_param("%d(0-4),%d[0-1024],%s,%d[0-2]", at_buf, &socket_id, &data_len, data, &rai_flag) != AT_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        if (g_data_send_mode == HEX_ASCII_STRING && at_parse_param("%d(0-4),%d[0-512],%s,%d[0-2]", at_buf, &socket_id, &data_len, data, &rai_flag) != AT_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        if ((socket_ctx_index = find_sock_ctx_id_by_sock_id(socket_id)) == -1)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        socket_ctx = sock_ctx[socket_ctx_index];

        if (data_len < 0)
        {
            /* 数据模式下发送不定长数据进入透传模式：以ctrl+z或者esc结束 */
            passthrough_socket_init();
            app_passthr_info_t socket_passthr = {0};
            socket_passthr.app_type = APP_SOCKET;
            socket_passthr.recv_len = 0;
            socket_passthr.max_len = AT_SOCKET_PASSTHR_MAX_LEN;
            socket_passthr.param = (void *)socket_ctx;
            socket_passthr.proc = passthr_socket_data_proc;
            into_Passthr_Mode(socket_passthr);
            send_urc_to_ext("\r\n>");
            xy_free(data);
            return AT_ASYN;      
        }
        else if (data_len == 0)
        {
            /* 查询已发送、已应答，以及已发送但未应答数据的总长度 +QISEND: <sent>,<acked>,<nAcked> */
            if (socket_ctx->net_type == 0)
            {
                int unacked = socket_ctx->sended_size - socket_ctx->acked;
                *prsp_cmd = xy_zalloc(32);
                snprintf(*prsp_cmd, 32, "\r\n+QISEND: %d,%d,%d\r\n\r\nOK\r\n", socket_ctx->sended_size, socket_ctx->acked, unacked);
            }
            goto END_PROC;
        }
        else
        {
            /* 透传发送定长数据 */
            if (strlen(data) == 0)
            {
                passthrough_socket_init();
                app_passthr_info_t socket_passthr = {0};
                socket_passthr.app_type = APP_SOCKET;
                if(g_data_send_mode == HEX_ASCII_STRING)
                    socket_passthr.recv_len = data_len * 2;
                else
                    socket_passthr.recv_len = data_len;
                socket_passthr.param = (void *)socket_ctx;
                socket_passthr.proc = passthr_socket_data_proc;
                into_Passthr_Mode(socket_passthr);
                send_urc_to_ext("\r\n>");
                xy_free(data);
                return AT_ASYN;
            }
            /* 正常发送数据 */
            else
            {
                if (bc26_socket_send_data(data, data_len, rai_flag, socket_ctx) == XY_ERR)
                {
                    *prsp_cmd = BC26_AT_ERR_BUILD();
                    goto END_PROC;
                }
            }
        }
END_PROC:
        xy_free(data);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(64);
        snprintf(*prsp_cmd, 64, "\r\n+QISEND: (0-%d),(1-%d),<data>,<rai_mode>\r\n\r\nOK\r\n", SOCK_NUM - 1, AT_SOCKET_MAX_DATA_LEN);
    }
    else
    {
        *prsp_cmd = BC26_AT_ERR_BUILD();
    }

    return AT_END;
}

#endif //AT_SOCKET
