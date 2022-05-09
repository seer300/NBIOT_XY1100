#include "softap_macro.h"

#if AT_SOCKET

#include "at_ctl.h"
#include "at_global_def.h"
#include "at_socket.h"
#include "at_socket_context.h"
#include "lwip/sockets.h"
#include "xy_net_api.h"
#include "xy_system.h"
#include "xy_utils.h"

void at_sock_recv_thread(void)
{
    int ret, i;
    int max_fd;
	fd_set readfds, exceptfds;
	struct timeval tv;
    tv.tv_sec = SOCK_RECV_TIMEOUT;
    tv.tv_usec = 0;

	xy_regist_psnetif_callback(EVENT_PSNETIF_INVALID, netif_down_close_socket);

    while (1)
    {
        FD_ZERO(&readfds); 
		FD_ZERO(&exceptfds);
        max_fd = -1;

        for (i = 0; i < SOCK_NUM; i++) 
		{
			if (sock_ctx[i] != NULL && sock_ctx[i]->quit == 1 && sock_ctx[i]->fd >= 0)
			{
                del_socket_ctx_by_index(i, false);
#if VER_QUCTL260
                char *close_info = xy_zalloc(32);
                sniprintf(close_info, 32, "\r\nCLOSE OK\r\n");
                at_write_all_to_uart(close_info, strlen(close_info));
                xy_free(close_info);
#endif //VER_QUCTL260
                continue;
			}
            else if (sock_ctx[i] != NULL && sock_ctx[i]->is_deact == 1 && sock_ctx[i]->fd >= 0)
            {
                del_socket_ctx_by_index(i, true);
                continue;
			}
			else if (sock_ctx[i] != NULL && sock_ctx[i]->quit != 1 && sock_ctx[i]->fd >= 0) 
			{
                if (max_fd < sock_ctx[i]->fd)
                {
					max_fd=sock_ctx[i]->fd;
				}
				FD_SET(sock_ctx[i]->fd, &readfds);
				FD_SET(sock_ctx[i]->fd, &exceptfds);
			}
		}

		if (max_fd < 0) 
		{
            goto out;
		}

        ret = select(max_fd + 1, &readfds, NULL, &exceptfds, &tv);
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

        for (i = 0; i < SOCK_NUM; i++)
        {
            if (sock_ctx[i] == NULL)
                continue;

            if (sock_ctx[i] != NULL && FD_ISSET(sock_ctx[i]->fd, &exceptfds))
            {
                softap_printf(USER_LOG, WARN_LOG, "except exist,force to close socket[%d]", i);
                del_socket_ctx_by_index(i, true);
                continue;
			}
            if (sock_ctx[i] != NULL && !FD_ISSET(sock_ctx[i]->fd, &readfds))
            {
                continue;
            }

            int len = 0;
			int read_len = 0;
			char *buf = NULL;
			char *rsp_cmd = NULL;
			char *temp_buf = NULL;
			struct sockaddr_in *remote_info = NULL;
			socklen_t fromlen = sizeof(struct sockaddr_in);

			ioctl(sock_ctx[i]->fd, FIONREAD, &len);  //get the size of received data
            softap_printf(USER_LOG, WARN_LOG, "socket[%d] fionread len:%d", i, len);
            if (len == 0)
            {
                softap_printf(USER_LOG, WARN_LOG, "socket[%d] recv 0 BYTES,force to close socket", i);
                del_socket_ctx_by_index(i, true);
				continue;
            }

            buf = xy_zalloc(len+1);
			remote_info = xy_zalloc(sizeof(struct sockaddr_in));
            read_len = recvfrom(sock_ctx[i]->fd, buf, len, MSG_DONTWAIT, (struct sockaddr *)remote_info, &fromlen);
            softap_printf(USER_LOG, WARN_LOG, "socket[%d] recv len:%d", i, read_len);
			if (read_len < 0)
            {
                if (errno == EWOULDBLOCK) // time out
                {
                	xy_free(buf);
					xy_free(remote_info);
                    continue;
                }
                else
                {
                    softap_printf(USER_LOG, WARN_LOG, "socket[%d] read - BYTES,force to close socket", i);
                    del_socket_ctx_by_index(i, true);
					xy_free(buf);
					xy_free(remote_info);
                    continue;
                }
            }
            else if (read_len == 0)
            {
                softap_printf(USER_LOG, WARN_LOG, "socket[%d]read 0 BYTES,force to close socket", i);
                del_socket_ctx_by_index(i, true);
				xy_free(buf);
				xy_free(remote_info);
				continue;
            }

            if (remote_info->sin_family != AF_INET)
            {
                softap_printf(USER_LOG, WARN_LOG, "socket[%d]recv ipv6 packet and drop", i);
                xy_free(buf);
				xy_free(remote_info);
				continue;
			}
            if (sock_ctx[i]->recv_ctl == 0)
            {
				softap_printf(USER_LOG, WARN_LOG, "socket[%d]recv_ctl is 0,drop data", i);
				xy_free(buf);
				xy_free(remote_info);
				continue;
			}

            if (xy_Remote_AT_Req(buf))
            {
				xy_free(buf);
			 	xy_free(remote_info);
			 	continue;
			}

#if VER_QUCTL260
            //+QIURC: "recv",<connectID>,<current_recv_length>,<data>
            //仅支持直吐模式
            if (sock_ctx[i]->accessmode == 1)
            {
                if (g_data_recv_mode == ASCII_STRING)
                {
                    rsp_cmd = xy_zalloc(80 + read_len);
                    snprintf(rsp_cmd, 80 + read_len, "\r\n+QIURC: \"recv\",%d,%d,\"%s\"\r\n", sock_ctx[i]->sock_id, read_len, buf);
                }
                else if (g_data_recv_mode == HEX_ASCII_STRING)
                {
                    temp_buf = xy_zalloc(read_len * 2 + 1);
                    bytes2hexstr(buf, read_len, temp_buf, read_len * 2 + 1);
                    rsp_cmd = xy_zalloc(80 + read_len * 2);
                    snprintf(rsp_cmd, 80 + read_len * 2, "\r\n+QIURC: \"recv\",%d,%d,\"%s\"\r\n", sock_ctx[i]->sock_id, read_len, temp_buf);
                }

                xy_free(buf);
                xy_free(remote_info);
            }
#else
			if (g_at_sck_report_mode == HINT_WITH_REMOTE_INFO)
			{
                char *remote_ip = xy_zalloc(40);
                inet_ntop(AF_INET, &(remote_info->sin_addr), remote_ip, 40);
                temp_buf = xy_zalloc(read_len * 2 + 1);
                bytes2hexstr(buf, read_len, temp_buf, read_len * 2 + 1);
                rsp_cmd = xy_zalloc(80 + read_len * 2);
                snprintf(rsp_cmd, 80 + read_len * 2, "\r\n+NSONMI:%d,%s,%d,%d,%s\r\n", sock_ctx[i]->sock_id, remote_ip, ntohs(remote_info->sin_port), read_len, temp_buf);
                xy_free(buf);
                xy_free(remote_info);
                xy_free(remote_ip);
            }
			else if (g_at_sck_report_mode == HINT_NO_REMOTE_INFO)
			{
                temp_buf = xy_zalloc(read_len * 2 + 1);
                bytes2hexstr(buf, read_len, temp_buf, read_len * 2 + 1);
                rsp_cmd = xy_zalloc(80 + read_len * 2);
                snprintf(rsp_cmd, 80 + read_len * 2, "\r\n+NSONMI:%d,%d,%s\r\n", i, read_len, temp_buf);
                xy_free(buf);
                xy_free(remote_info);
            }
			else if (add_new_rcv_data_node(i, read_len, buf, remote_info) == XY_OK)
			{
				//+NSONMI:<socket>,<length>
				if (g_at_sck_report_mode == BUFFER_WITH_HINT || g_at_sck_report_mode == BUFFER_NO_HINT)
				{
                    softap_printf(USER_LOG, WARN_LOG, "socket[%d] recv %d length downlink data!!!", i, read_len);
                    if (g_at_sck_report_mode == BUFFER_WITH_HINT && sock_ctx[i]->firt_recv == 0)
					{
                        char *mid_rsp_cmd = xy_zalloc(36);
                        snprintf(mid_rsp_cmd, 36, "\r\n+NSONMI:%d,%d\r\n", i, read_len);
                        send_rsp_str_to_ext(mid_rsp_cmd);
                        xy_free(mid_rsp_cmd);
                        osMutexAcquire(g_socket_mux, osWaitForever);
                        sock_ctx[i]->firt_recv = 1;
                        osMutexRelease(g_socket_mux);
                    }
				}
			}
			else
			{
				rsp_cmd = xy_zalloc(60);
                snprintf(rsp_cmd, 60, "\r\n+NSONMI:drop %d bytes pkt\r\n", read_len);
            }
#endif //VER_QUCTL260
            if (temp_buf != NULL)
                xy_free(temp_buf);
            if (rsp_cmd != NULL)
            {
                send_rsp_str_to_ext(rsp_cmd);
                xy_free(rsp_cmd);
            }
        }
    }
out:
    xy_deregist_psnetif_callback(EVENT_PSNETIF_INVALID, netif_down_close_socket);
    softap_printf(USER_LOG, WARN_LOG, "socket recv thread exit!!!");
    at_rcv_thread_id = NULL;
    osThreadExit();
}

#endif //AT_SOCKET
