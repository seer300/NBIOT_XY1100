#include <string.h>
#include <stdint.h>
#include "xy_http_client.h"
#include "at_http.h"
#include "xy_at_api.h"
#include <errno.h>
#include "lwip/sockets.h"
#include "http_api.h"
#include "xy_proxy.h"

void add_sockerr_node(http_context_reference_t *http_context_ref )
{
	Http_recv_t *recv_list = xy_zalloc(sizeof(Http_recv_t));
	recv_list->type = HTTP_SOCKERR;
	recv_list->more = 0;

	vTaskSuspendAll();
	http_add_list_end(&http_context_ref->http_context->recv_list, recv_list);
	xTaskResumeAll();

	osSemaphoreRelease(http_context_ref->http_context->http_header_sem);
	osSemaphoreRelease(http_context_ref->http_context->http_body_sem);

}


void at_http_recv(void* argument)
{
	int ret;
	int max_fd;
	fd_set readfds, exceptfds;
	struct timeval tv;
	http_context_reference_t *http_context_ref = (http_context_reference_t*)argument;
	http_context_t *http_context = http_context_ref->http_context;
	char *rsp_cmd = NULL;

    tv.tv_sec = SOCK_RECV_TIMEOUT;
    tv.tv_usec = 0;

	while(1) {
		if (http_context_ref->quit == 1)
		{
			http_close(http_context);
			break;
		}

		if (http_context->secured)
		{
			while(http_context->recv_list_length >= HTTP_RECV_NODE_NUMBER)
			{
				osDelay(50);
			}
			ret = xy_tls_read(http_context->tls_context, http_context->buf + http_context->current_recv_len,
					http_context->left_recv_len, 10000 /*ms*/);
			http_context->data_recv_ever = 1;		
			if (ret == -2) // timeout
			{
				continue;
			}
			else if (ret == -3) // MBEDTLS_ERR_SSL_WANT_READ
			{
				continue;
			}
			else if (ret < 0)
			{
				softap_printf(USER_LOG, WARN_LOG, "tls read error");
				http_close(http_context);
				rsp_cmd = xy_zalloc(32);
				sprintf(rsp_cmd, "\r\n+HTTPDICONN:%d,%d,%d,%d\r\n", http_context->http_id, -1, ret, errno);
				send_urc_to_ext(rsp_cmd);
				xy_free(rsp_cmd);
				goto out;
			}
			else if (ret == 0)
			{
				softap_printf(USER_LOG, WARN_LOG, "read 0 byte, connection is closed by peer end");
				http_close(http_context);
				rsp_cmd = xy_zalloc(32);
				sprintf(rsp_cmd, "\r\n+HTTPDICONN:%d,%d\r\n", http_context->http_id, -2);
				send_urc_to_ext(rsp_cmd);
				xy_free(rsp_cmd);
				goto out;
			}
			http_context->current_recv_len += ret;
			http_context->left_recv_len -= ret;
			if (handle_http_data(http_context) < 0) {
				softap_printf(USER_LOG, WARN_LOG, "error processing http data");
				goto out;
			}
			if(http_context->left_recv_len == 0)
			{
				http_context->history_processed_len += http_context->content_length_processed;
				http_context->left_recv_len = MAX_HTTP_BUF_LEN;
				http_context->current_recv_len = 0;
				http_context->content_length_processed  = 0;
			}
			continue;
		}

		FD_ZERO(&readfds);
		FD_ZERO(&exceptfds);

		max_fd = http_context->sock;
		FD_SET(http_context->sock, &readfds);
		FD_SET(http_context->sock, &exceptfds);

		if (max_fd < 0)
		{
            goto out;
		}

		ret = select(max_fd + 1, &readfds, NULL, &exceptfds, &tv);
		if (ret < 0){
			if (errno == EINTR)
			{
				continue;
			}
            else if(errno == EBADF)
            {
                //one or more sockets may be closed asynchronously by AT+NSOCL
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

		if(FD_ISSET(http_context->sock, &exceptfds))
		{
			softap_printf(USER_LOG, WARN_LOG, "except exist, force to close socket");
			http_close(http_context);
			rsp_cmd = xy_zalloc(32);
			sprintf(rsp_cmd, "\r\n+HTTPDICONN:%d,%d\r\n", http_context->http_id, -1);
			send_rsp_str_to_ext(rsp_cmd);
			xy_free(rsp_cmd);
			break;
		}

		if (!FD_ISSET(http_context->sock, &readfds)) {
			continue;
		}

		int read_len;
		while(http_context->recv_list_length >= HTTP_RECV_NODE_NUMBER)
		{
			osDelay(50);
		}
		read_len = recv(http_context->sock, http_context->buf + http_context->current_recv_len,
				http_context->left_recv_len, MSG_DONTWAIT);

		http_context->data_recv_ever = 1;
		if (read_len < 0)
		{
			if (errno == EWOULDBLOCK) // time out
			{
				continue;
			}
			else
			{
				softap_printf(USER_LOG, WARN_LOG, "read - bytes,force to close socket:%d",read_len);
				http_close(http_context);
				rsp_cmd = xy_zalloc(32);
				softap_printf(USER_LOG, WARN_LOG, "read 0 byte, connection is closed by peer end");
				sprintf(rsp_cmd, "\r\n+HTTPDICONN:%d,%d,%d\r\n", http_context->http_id, -1, errno);
				send_urc_to_ext(rsp_cmd);
				xy_free(rsp_cmd);
				goto out;
			}
		}
		else if (read_len == 0) {
			softap_printf(USER_LOG, WARN_LOG, "read 0 byte, connection is closed by peer end");
			http_close(http_context);
			rsp_cmd = xy_zalloc(32);
			sprintf(rsp_cmd, "\r\n+HTTPDICONN:%d,%d\r\n", http_context->http_id, -2);
			send_urc_to_ext(rsp_cmd);
			xy_free(rsp_cmd);
			goto out;
		} else {
			http_context->current_recv_len += read_len;
			http_context->left_recv_len -= read_len;
			if (handle_http_data(http_context) < 0) {
				softap_printf(USER_LOG, WARN_LOG, "error processing http data");
				goto out;
			}
			if(http_context->left_recv_len == 0)
			{
				http_context->history_processed_len += http_context->content_length_processed;
				http_context->left_recv_len = MAX_HTTP_BUF_LEN;
				http_context->current_recv_len = 0;
				http_context->content_length_processed  = 0;
			}
		}
	}
out:
	add_sockerr_node(http_context_ref);
	softap_printf(USER_LOG, WARN_LOG, "http recv thread exit!!!");
	http_context_ref->http_recv_thread_id = 0;
	osThreadExit();
}
