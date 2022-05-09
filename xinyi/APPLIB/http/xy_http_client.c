#include "xy_http_client.h"
#include "http_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http_api.h"
#include "errno.h"

#include "stringx.h"
#include "urlparser.h"

#include "lwip/sockets.h"
#include "xy_proxy.h"
#if XY_DTLS
#include "tls_interface.h"
#endif

char Is_Recvlist_Empty(Http_recv_t *recv_list)
{
	Http_recv_t * recv_temp = recv_list;

	if(recv_temp == NULL)
		return XY_TRUE;
	else
		return XY_FALSE;
}

Http_recv_t *http_get_next_node(Http_recv_t *recv_list)
{
	Http_recv_t * recv_temp = recv_list;
	if(recv_temp == NULL)
	{
		return NULL;
	}
	else
	{
		return recv_temp->pNext;
	}
}

int http_delete_node(http_context_t *http_context, Http_recv_t *recv_node)
{
	Http_recv_t * recv_temp = NULL;

	recv_temp = http_context->recv_list;
	if( recv_temp == recv_node)
	{
		http_context->recv_list = recv_temp->pNext;
		if(recv_temp->info != NULL)
			xy_free(recv_temp->info);
		if(recv_temp->payload != NULL)
			xy_free(recv_temp->payload);
		if(recv_temp != NULL)
			xy_free(recv_temp);
	}
	
	for( ; recv_temp->pNext != NULL; recv_temp = recv_temp->pNext )
	{
		if(recv_temp->pNext == recv_node)
		{
			recv_temp->pNext = recv_node->pNext;
			if(recv_node->info != NULL)
				xy_free(recv_node->info);
			if(recv_node->payload != NULL )
				xy_free(recv_node->payload);
			if(recv_node != NULL )
				xy_free(recv_node);
		}
	}
	http_context->recv_list_length -= 1;
	return 0;
}

int http_add_list_head(Http_recv_t **list, Http_recv_t * httpdata)
{
	Http_recv_t *header_temp;
	if(*list == NULL)
	{
		httpdata->pNext = NULL;
		*list = httpdata;
	}
	else
	{
		header_temp = *list;
		httpdata->pNext = header_temp;
		*list = httpdata;
	}
	((http_context_t *)((char *)list - ((unsigned long)&((http_context_t *)0)->recv_list)))->recv_list_length += 1;
	return 0;
}

int http_add_list_end(Http_recv_t **list, Http_recv_t * httpdata)
{
	Http_recv_t *header_temp;
	if(*list == NULL)
	{
		httpdata->pNext = NULL;
		*list = httpdata;
	}
	else
	{
		header_temp = *list;
		while(header_temp->pNext != NULL)
		{
			header_temp = header_temp->pNext;
		}
		httpdata->pNext = NULL;
		header_temp->pNext = httpdata;
	}
	((http_context_t *)((char *)list - ((unsigned long)&((http_context_t *)0)->recv_list)))->recv_list_length += 1;
	return 0;
}

Http_recv_t *New_Recv_Node( char http_type, char more_data, char * next_node, unsigned short len)
{
	Http_recv_t *recv_list = xy_zalloc(sizeof(Http_recv_t));

	recv_list->type = http_type;
	recv_list->pNext = next_node;
	recv_list->info = xy_zalloc(64);
	recv_list->more = more_data;
	recv_list->len = len;

	return recv_list;
}

int Add_To_RecvLink(http_context_t *http_context, Http_recv_t *recv_list, char http_type, char info_print, char payload_print)
{
	if(http_context->print_or_not == 1)
	{
		if(info_print == 1)
		{
			send_urc_to_ext(recv_list->info);
			xy_free(recv_list->info);
		}
		if(payload_print == 1)
		{
			strcat(recv_list->payload, "\r\n");
			send_urc_to_ext(recv_list->payload);
			xy_free(recv_list->payload);
		}
	}else
	{
		vTaskSuspendAll();
		http_add_list_end(&http_context->recv_list, recv_list);
		xTaskResumeAll();
		if(http_type == HTTP_BODY)
		{
			osSemaphoreRelease(http_context->http_body_sem);
		}
		else if(http_type == HTTP_HEADER)
		{
			osSemaphoreRelease(http_context->http_header_sem);
		}
		else
			xy_assert(0);
	}
	return 0;
}

int Print_Mode_Convert(	Http_recv_t *recv_list, char print_mode, unsigned short len, unsigned int src_addr)
{
	unsigned int * temp = NULL;
	if (print_mode == 0)
	{
		recv_list->payload = xy_zalloc(len + 100);
		memcpy(recv_list->payload, src_addr, len);
	}
	else
	{
		recv_list->payload = xy_zalloc(len * 2 + 100);
		temp = xy_zalloc(len * 2 + 1 + 200);
		bytes2hexstr(src_addr, len, temp, len * 2 + 1);
		memcpy(recv_list->payload, temp, len * 2);
		xy_free(temp);
	}
	return 0;
}

extern osMessageQueueId_t http_create_msg_q;
extern http_context_reference_t http_context_refs[];
void at_http_parse(void* argument)
{
	char ret = XY_OK;
	Http_Para_t *msg = NULL;
	http_context_reference_t *http_context_ref = NULL;

	while(1)
	{
		osMessageQueueGet(http_create_msg_q, (void *)(&msg), NULL, osWaitForever);
		if(msg == NULL)
		{
            xy_assert(0);
            continue;
		}
		msg->respond_buf = xy_malloc(100);
		switch(msg->flag)
		{
			case HTTP_REQUEST_REQ:
				http_context_ref = &http_context_refs[msg->http_id];
				if (http_context_ref->http_context == NULL){
					sprintf(msg->respond_buf, "\r\n+BADREQUEST\r\n");
					goto freeReqMsg;
				}

				ret = xy_http_connect(msg->http_id);
				if(ret == XY_ERR){
					sprintf(msg->respond_buf,  "\r\n+BADREQUEST\r\n");
					goto freeReqMsg;
				}

				ret = xy_http_request(msg->http_id, msg->method, msg->path);
				if (ret < 0) {
					sprintf(msg->respond_buf,  "\r\n+BADREQUEST\r\n");
					goto freeReqMsg;
				}
				sprintf(msg->respond_buf, "\r\n+REQUESTSUCCESS\r\n");

			freeReqMsg:
				if(msg->path != NULL)
					xy_free(msg->path);
				break;
				
			default:
				break;
		}
		send_urc_to_ext(msg->respond_buf);
		xy_free(msg->respond_buf);
		xy_free(msg);
	}
}
int get_free_http_context_ref(char *http_id)
{
	char tmp_id = *http_id;
	http_context_reference_t *http_context_ref = NULL;

	for (tmp_id = 0; tmp_id < HTTP_CONTEXT_REF_NUM; tmp_id++)
	{
		if (http_context_refs[tmp_id].http_context == NULL && http_context_refs[tmp_id].http_recv_thread_id == 0 && http_context_refs[tmp_id].http_at_thread_id == 0)
			break;
	}
	if( tmp_id == HTTP_CONTEXT_REF_NUM )
		return XY_ERR;

	http_context_ref = &http_context_refs[tmp_id];
	memset(http_context_ref, 0, sizeof(http_context_reference_t));

	http_context_ref->http_context = (http_context_t*)xy_zalloc(sizeof(http_context_t));
	if (http_context_ref->http_context == NULL)
		return XY_ERR;

	http_context_ref->http_context->sock = -1;
	http_context_ref->http_context->left_recv_len = MAX_HTTP_BUF_LEN;
	http_context_ref->http_context->recv_list_length = 0;
	http_context_ref->http_context->http_id = tmp_id;
	*http_id = tmp_id;
	return XY_OK;
}
int parse_http_host(http_context_t *http_context, char *host)
{
	char *cur = host;
	char *temp_cur = NULL;
	char *temp_cur1 = NULL;
	int has_port = 0;

	if (strncmp(host, "http", strlen("http")) != 0) {			
		goto failed;
	} else {
		cur = strchr(cur, ':');
		if ( NULL == cur ) {
			return -1;
		}
		if (*(cur + 1) != '/' || *(cur + 2) != '/') {
			return -1;
		}
		cur += 3;
	}


	/* 没有":" 则没有port,
		 有":" 没有"/" 直接取冒号后的值作为port
		 有":" 有"/"   取反斜杠后的值作为port
	*/
	temp_cur1 = strchr(cur, ':');
	temp_cur = strchr(cur, '/');

	if (temp_cur1 == NULL) {
		temp_cur = cur + strlen(cur);
	} else {
		if (temp_cur == NULL) {
			has_port = 1;
			temp_cur = temp_cur1;
		} else {
			if (temp_cur1 < temp_cur) {
				has_port = 1;
				temp_cur = temp_cur1;
			} else {

			}
		}
	}

	/* 解析头部的域名, 如果实例中仍保留著上一次的域名,且上次和本次的域名不一致,则删除遗留域名*/
	/*申请内存,保存本次域名*/
	if (http_context->domain_name != NULL && strncmp(http_context->domain_name, cur, temp_cur - cur) != 0) {
		xy_free(http_context->domain_name);
	}
	http_context->domain_name = xy_zalloc(temp_cur - cur + 1);
	strncpy(http_context->domain_name, cur, temp_cur - cur);

	/* 有port: 填入port
	   没有port: http 默认port80   https默认port443
	*/
	if (has_port) {
		http_context->port = (int)strtol(temp_cur1 + 1,NULL,10);
	} else {
		if (http_context->secured)
			http_context->port = 443;
		else
			http_context->port = 80;
	}

	if (http_context->port <= 0)
	{
		goto failed;
	}

	return 0;
failed:
	if (http_context->domain_name != NULL)
		xy_free(http_context->domain_name);
	http_context->domain_name = NULL;
	return -1;
}

int Http_User_Info(http_context_reference_t *http_context_ref, char * host,char * auth_user, char * auth_passwd)
{
	if (strncmp(host, "https", strlen("https")) == 0) {
		http_context_ref->http_context->secured = 1;
	}

	/*解析host传参, 得到port 和 domain_name */
	if (parse_http_host(http_context_ref->http_context, host) < 0) {
		goto failed;
	}

	/*保存host参数 */
	http_context_ref->http_context->host = xy_zalloc(strlen(host) + 1);
	if (http_context_ref->http_context->host == NULL) {
		goto failed;
	}
	strcpy(http_context_ref->http_context->host, host);

	/*保存auth_user参数 */
	if ((auth_user != NULL) && (strcmp(auth_user, "") != 0)) {
		http_context_ref->http_context->auth_user = xy_zalloc(strlen(auth_user) + 1);
		if (http_context_ref->http_context->auth_user == NULL) {
			goto failed;
		}
		strcpy(http_context_ref->http_context->auth_user, auth_user);
	}

	/*保存auth_passwd参数 */
	if ((auth_passwd != NULL) && (strcmp(auth_passwd, "") != 0)) {
		http_context_ref->http_context->auth_passwd = xy_zalloc(strlen(auth_passwd) + 1);
		if (http_context_ref->http_context->auth_passwd == NULL) {
			goto failed;
		}
		strcpy(http_context_ref->http_context->auth_passwd, auth_passwd);
	}

	http_context_ref->http_context->http_header_sem = osSemaphoreNew(0xFFFF, 0, NULL);
	http_context_ref->http_context->http_body_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	return XY_OK;
	
failed:
	if (http_context_ref != NULL) {
		if (http_context_ref->http_context->domain_name != NULL) {
			xy_free(http_context_ref->http_context->domain_name);
			http_context_ref->http_context->domain_name = NULL;
		}
		if (http_context_ref->http_context->host != NULL) {
			xy_free(http_context_ref->http_context->host);
			http_context_ref->http_context->host = NULL;
		}
		if (http_context_ref->http_context->auth_user != NULL) {
			xy_free(http_context_ref->http_context->auth_user);
			http_context_ref->http_context->auth_user = NULL;
		}
		if (http_context_ref->http_context->auth_passwd != NULL) {
			xy_free(http_context_ref->http_context->auth_passwd);
			http_context_ref->http_context->auth_passwd = NULL;
		}
		if(http_context_ref->http_context->print_or_not == 1)
		{
			if(http_context_ref->http_at_thread_id != 0)
			{
				osThreadTerminate(http_context_ref->http_at_thread_id);
				http_context_ref->http_at_thread_id = 0;
			}
		}
		if (http_context_ref->http_context != NULL) {
			xy_free(http_context_ref->http_context);
			http_context_ref->http_context = NULL;
		}
	}
	return XY_ERR;
}

int http_context_clear(http_context_t *http_context)
{
	if (http_context->host != NULL) {
		xy_free(http_context->host);
	}
	if (http_context->domain_name != NULL) {
		xy_free(http_context->domain_name);
	}
	if (http_context->auth_user != NULL) {
		xy_free(http_context->auth_user);
	}
	if (http_context->auth_passwd != NULL) {
		xy_free(http_context->auth_passwd);
	}
	if (http_context->server_cert != NULL) {
		xy_free(http_context->server_cert);
	}
	if (http_context->client_cert != NULL) {
		xy_free(http_context->client_cert);
	}
	if (http_context->client_pk != NULL) {
		xy_free(http_context->client_pk);
	}
	if (http_context->content != NULL) {
		xy_free(http_context->content);
	}
	if (http_context->custom_header != NULL) {
		xy_free(http_context->custom_header);
	}
	osSemaphoreDelete (http_context->http_body_sem);
	osSemaphoreDelete (http_context->http_header_sem);
	memset(http_context, 0, sizeof(http_context_t));
	return 0;
}

int http_context_destroy(http_context_t *http_context)
{
	http_context_clear(http_context);
	xy_free(http_context);
	return 0;
}

/* GET %s HTTP/1.1\r\nHost:%s\r\n  + "Authorization: Basic %s\r\n + custom_header */
/* 其中后半部分为user + passwd经过base64编码后的内容*/
char* make_http_get_content(http_context_t *http_context, char *path)
{
	char *http_content = NULL;
	int custom_header_len = 0;
	int auth_len = 0;
	char *auth = NULL;
	char *base64 = NULL;
	char *auth_header = NULL;

	if (http_context->custom_header != NULL)
		custom_header_len = strlen(http_context->custom_header);

	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		auth_len = strlen(http_context->auth_user) + strlen(http_context->auth_passwd) + 4;
		auth = xy_zalloc(auth_len);
		// calculate base64 encoded auth len
		if (auth_len % 3 == 0) {
			auth_len = auth_len / 3 * 4;
		} else {
			auth_len = auth_len / 3 * 4 + 1;
		}
	}
	http_content = (char*)xy_zalloc(512 + custom_header_len + auth_len);
	if (http_content == NULL)
		goto failed;

	sprintf(http_content, "GET %s HTTP/1.1\r\nHost:%s\r\n"
			/*"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/86.0.4240.75\r\n"*/,
			path, http_context->domain_name);

	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		sprintf(auth, "%s:%s", http_context->auth_user, http_context->auth_passwd);
		base64 = http_base64_encode(auth);
		if (base64 == NULL)
			goto failed;
		auth_header = (char*)xy_zalloc(128 + strlen(base64));
		if (auth_header == NULL)
			goto failed;
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		sprintf(http_content + strlen(http_content), "%s", auth_header);
	}

	if (http_context->custom_header != NULL) {
		sprintf(http_content + strlen(http_content), "%s", http_context->custom_header);
	}

	strcat(http_content, "\r\n");

	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);

	return http_content;

failed:
	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);
	if (http_content != NULL)
		xy_free(http_content);
	return NULL;
}

/* POST/PUT %s HTTP/1.1\r\nHost:%s\r\n  + "Authorization: Basic %s\r\n + custom_header + content */
/* 其中后半部分为user + passwd经过base64编码后的内容*/
char* make_http_post_put_content(http_context_t *http_context, char *path, int is_post)
{
	char *http_content = NULL;
	int content_length = strlen(http_context->content);
	int custom_header_len = 0;
	char *auth = NULL;
	int auth_len = 0;
	char *base64 = NULL;
	char *auth_header = NULL;

	if (http_context->custom_header != NULL)
		custom_header_len = strlen(http_context->custom_header);
	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		auth_len = strlen(http_context->auth_user) + strlen(http_context->auth_passwd) + 4;
		auth = xy_zalloc(auth_len);
		// calculate base64 encoded auth len
		if (auth_len % 3 == 0) {
			auth_len = auth_len / 3 * 4;
		} else {
			auth_len = auth_len / 3 * 4 + 1;
		}
	}

	http_content = (char*)xy_zalloc(512 + custom_header_len + auth_len + content_length);
	if (http_content == NULL)
		goto failed;

	// Content-Type:text/plain application/x-www-form-urlencoded
	sprintf(http_content, "%s %s HTTP/1.1\r\nHost:%s\r\n"
			/*"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/86.0.4240.75\r\n"*/,
			is_post? "POST" : "PUT", path, http_context->host);

	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		sprintf(auth, "%s:%s", http_context->auth_user, http_context->auth_passwd);
		base64 = http_base64_encode(auth);
		if (base64 == NULL)
			goto failed;
		auth_header = (char*)xy_zalloc(128 + strlen(base64));
		if (auth_header == NULL)
			goto failed;
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		sprintf(http_content + strlen(http_content), "%s", auth_header);
	}

	if (http_context->custom_header != NULL) {
		sprintf(http_content + strlen(http_content), "%s", http_context->custom_header);
	}

	strcat(http_content, "\r\n");

	if (http_context->content != NULL) {
		strcat(http_content, http_context->content);
	}

	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);

	return http_content;

failed:
	if (auth != NULL)
		xy_free(auth);
	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);
	if (http_content != NULL)
		xy_free(http_content);
	return NULL;
}

/* DELETE %s HTTP/1.1\r\nHost:%s\r\n  + "Authorization: Basic %s\r\n + custom_header */
/* 其中后半部分为user + passwd经过base64编码后的内容*/
char* make_http_delete_content(http_context_t *http_context, char *path)
{
	char *http_content = NULL;
	int custom_header_len = 0;
	int auth_len = 0;
	char *auth = NULL;
	char *base64 = NULL;
	char *auth_header = NULL;

	if (http_context->custom_header != NULL)
		custom_header_len = strlen(http_context->custom_header);
	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		auth_len = strlen(http_context->auth_user) + strlen(http_context->auth_passwd) + 4;
		auth = xy_zalloc(auth_len);
		// calculate base64 encoded auth len
		if (auth_len % 3 == 0) {
			auth_len = auth_len / 3 * 4;
		} else {
			auth_len = auth_len / 3 * 4 + 1;
		}
	}
	http_content = (char*)xy_zalloc(512 + custom_header_len + auth_len);
	if (http_content == NULL)
		goto failed;

	sprintf(http_content, "DELETE %s HTTP/1.1\r\nHost:%s\r\n"
			/*"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/86.0.4240.75\r\n"*/, path, http_context->host);

	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		sprintf(auth, "%s:%s", http_context->auth_user, http_context->auth_passwd);
		base64 = http_base64_encode(auth);
		if (base64 == NULL)
			goto failed;
		auth_header = (char*)xy_zalloc(128 + strlen(base64));
		if (auth_header == NULL)
			goto failed;
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		sprintf(http_content + strlen(http_content), "%s", auth_header);
	}

	if (http_context->custom_header != NULL) {
		sprintf(http_content + strlen(http_content), "%s", http_context->custom_header);
	}

	strcat(http_content, "\r\n");

	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);

	return http_content;

failed:
	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);
	if (http_content != NULL)
		xy_free(http_content);
	return NULL;
}

/* HEAD %s HTTP/1.1\r\nHost:%s\r\n  + "Authorization: Basic %s\r\n + custom_header */
/* 其中后半部分为user + passwd经过base64编码后的内容*/
char* make_http_head_content(http_context_t *http_context, char *path)
{
	char *http_content = NULL;
	int custom_header_len = 0;
	int auth_len = 0;
	char *auth = NULL;
	char *base64 = NULL;
	char *auth_header = NULL;

	if (http_context->custom_header != NULL)
		custom_header_len = strlen(http_context->custom_header);
	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		auth_len = strlen(http_context->auth_user) + strlen(http_context->auth_passwd) + 4;
		auth = xy_zalloc(auth_len);
		// calculate base64 encoded auth len
		if (auth_len % 3 == 0) {
			auth_len = auth_len / 3 * 4;
		} else {
			auth_len = auth_len / 3 * 4 + 1;
		}
	}
	http_content = (char*)xy_zalloc(512 + custom_header_len + auth_len);
	if (http_content == NULL)
		goto failed;

	sprintf(http_content, "HEAD %s HTTP/1.1\r\nHost:%s\r\n"
			/*"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/86.0.4240.75\r\n"*/,
			path, http_context->domain_name);

	if (http_context->auth_user != NULL && http_context->auth_passwd != NULL) {
		sprintf(auth, "%s:%s", http_context->auth_user, http_context->auth_passwd);
		base64 = http_base64_encode(auth);
		if (base64 == NULL)
			goto failed;
		auth_header = (char*)xy_zalloc(128 + strlen(base64));
		if (auth_header == NULL)
			goto failed;
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		sprintf(http_content + strlen(http_content), "%s", auth_header);
	}

	if (http_context->custom_header != NULL) {
		sprintf(http_content + strlen(http_content), "%s", http_context->custom_header);
	}

	strcat(http_content, "\r\n");

	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);

	return http_content;

failed:
	if (auth != NULL)
		xy_free(auth);
	if (base64 != NULL)
		xy_free(base64);
	if (auth_header != NULL)
		xy_free(auth_header);
	if (http_content != NULL)
		xy_free(http_content);
	return NULL;
}

int http_request(http_context_t *http_context, char *path, int http_method)
{
	char *http_content = NULL;
	int ret;

	if (http_context == NULL || path == NULL)
	{
		return -1;
	}


	switch (http_method) {
		case HTTP_METHOD_GET:
		{
			/* GET %s HTTP/1.1\r\nHost:%s\r\nConnection:keep-alive  + "Authorization: Basic %s\r\n */
			/* 其中后半部分为user + passwd经过base64编码后的内容*/
			http_content = make_http_get_content(http_context, path);
			break;
		}
		case HTTP_METHOD_POST:
		{
			/*%s %s HTTP/1.1\r\nHost:%s\r\n  +  "Authorization: Basic %s\r\n   +  content*/
			http_content = make_http_post_put_content(http_context, path, 1);
			break;
		}
		case HTTP_METHOD_PUT:
		{
			/* 与POST类似*/
			http_content = make_http_post_put_content(http_context, path, 0);
			break;
		}
		case HTTP_METHOD_DELETE:
		{
			/*"DELETE %s HTTP/1.1\r\nHost:%s\r\n"	*/
			/*"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/86.0.4240.75\r\n");   +  "Authorization: Basic %s\r\n   +  content */
			http_content = make_http_delete_content(http_context, path);
			break;
		}
		case HTTP_METHOD_HEAD:
		{
			/*"HEAD %s HTTP/1.1\r\nHost:%s\r\n"	*/
			/*"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/86.0.4240.75\r\n");   +  "Authorization: Basic %s\r\n   +  content */			
			http_content = make_http_head_content(http_context, path);
			break;
		}

		default:
			goto failed;
	}

again:
	if (http_context->secured)
	{
		ret = xy_tls_write(http_context->tls_context, http_content, strlen(http_content));
		if (ret == -3) // again
		{
			goto again;
		}
		else if (ret < 0)
		{
			softap_printf(USER_LOG, WARN_LOG, "http send failed");
			goto failed;
		}
	}
	else
	{
		ret = send(http_context->sock, http_content, strlen(http_content), 0);
		if (ret < 0) {
			if (errno == EWOULDBLOCK) // again
			{
				goto again;
			}
			else {
				softap_printf(USER_LOG, WARN_LOG, "http send failed");
				goto failed;
			}
		}

	}

	if (http_content != NULL)
		xy_free(http_content);
	return 0;

failed:
	if (http_content != NULL)
		xy_free(http_content);
	return -1;
}

int https_connect(http_context_t *http_context)
{
	int ret;
	tls_shakehand_info_s info = {0};
	tls_establish_info_s establish_info = {0};
	establish_info.ca_cert = http_context->server_cert;
	// '\0' is counted in mbedtls_x509_crt_parse, so we need to +1 here
	establish_info.ca_cert_len = strlen(http_context->server_cert) + 1;

	if (http_context->tls_context != NULL)
	{
		softap_printf(USER_LOG, WARN_LOG, "[HTTP]HTTPS has been created in https_connect");
		return 0;
	}

	if(http_context->client_cert != NULL)
	{
		establish_info.client_cert = http_context->client_cert;
		establish_info.client_cert_len = strlen(http_context->client_cert) + 1;
	}
	if(http_context->client_pk != NULL)
	{
		establish_info.client_pk = http_context->client_pk;
		establish_info.client_pk_len = strlen(http_context->client_pk) + 1;
	}
	if((http_context->client_cert != NULL && http_context->client_pk == NULL) || (http_context->client_cert == NULL && http_context->client_pk != NULL))
	{
		softap_printf(USER_LOG, WARN_LOG, "[HTTP]missing client certificate or private key in https_connect");
		goto failed;	
	}

	http_context->tls_context = xy_tls_ssl_new(&establish_info);
	if (http_context->tls_context == NULL)
	{	
		softap_printf(USER_LOG, WARN_LOG, "[HTTP]xy_tls_ssl_new failed in https_connect");
		goto failed;
	}

	info.host = http_context->domain_name;
	info.port = xy_zalloc(8);
	itoa(http_context->port, info.port, 10);

	if (strcmp(info.port, "") == 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "[HTTP]the port number is empty in https_connect");
		goto failed;
	}

	ret = xy_tls_shakehand(http_context->tls_context, &info);
	if (ret < 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "[HTTP]xy_tls_shakehand failed in https_connect");
		goto failed;
	}

	if (info.port != NULL)
		xy_free(info.port);

	return 0;
failed:
	if (info.port != NULL)
		xy_free(info.port);
	if (http_context->tls_context != NULL)
		xy_tls_ssl_destroy(http_context->tls_context);
	http_context->tls_context = NULL;
	return -1;
}

int http_connect(http_context_t *http_context)
{
	int sock = -1;
	char ip[16] = {0};

	if (http_context->secured)
	{
		return https_connect(http_context);
	}

	if(http_context->sock != -1)
	{
		return 0;
	}
	
	/* Get ip */
	if(hostname_to_ip(http_context->domain_name, ip, 16) < 0)
	{
		return -1;
	}

	struct sockaddr_in remote;
	int tmpres;
	/* Create TCP socket */
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "Can't create TCP socket");
		goto failed;
	}

	/* Set remote->sin_addr.s_addr */
	remote.sin_family = AF_INET;
	tmpres = inet_pton(AF_INET, ip, (void *)(&(remote.sin_addr)));
	if( tmpres < 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "Can't set remote->sin_addr.s_addr");
		goto failed;
	}
	else if(tmpres == 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "Not a valid IP");
		goto failed;
	}
	remote.sin_port = htons(http_context->port);

	/* Connect */
	if(connect(sock, (struct sockaddr *)&remote, sizeof(struct sockaddr_in)) < 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "Could not connect");
		goto failed;
	}

	http_context->sock = sock;

	return 0;

failed:
	if (sock != -1)
		close(sock);
	return -1;
}

int handle_http_header(http_context_t *http_context)
{
	char *cur = NULL;
	int header_len = 0;
	char *rsp_cmd_at = NULL;
	char *temp = NULL;
	Http_recv_t *recv_list;
	/* header 和 body 用\r\n\r\n 分割， 以此找到header len*/
	cur = strstr(http_context->buf, "\r\n\r\n");
	if (cur == NULL) {
		return 0; // continue receiving data until we get all the header
	}
	header_len = cur - http_context->buf + 4;

	/* 定位header中Content-Length关键字， 存在则将其填入结构体，不存在则为chunked模式 */
	cur = http_context->buf;
	cur = strstr(cur, "Content-Length: ");
	if (cur != NULL) {
		http_context->content_length = atoi(cur + strlen("Content-Length: "));
	} else {
		cur = http_context->buf;
		cur = strstr(cur, "Transfer-Encoding: chunked");
		if (cur != NULL) {
			http_context->is_content_chunked = 1;
		}
		else
		{
			http_context->content_length = 0;
		}
	}
	/* AT 命令需求: 打印实例id 以及 长度*/
	recv_list = xy_zalloc(sizeof(Http_recv_t));
	recv_list->pNext = NULL;
	recv_list->type = HTTP_HEADER;
	recv_list->more = 0;
	recv_list->info = xy_zalloc(64);
	recv_list->len = header_len;
	sprintf(recv_list->info, "\r\n+HTTPNMIH:%d,%d,%d\r\n", http_context->http_id, 0, header_len);						// 中间参数0表示:头部长度很短,一次能传输完成

	if (http_context->print_mode == 0)
	{
		recv_list->payload = xy_zalloc(header_len + 100);
		memcpy(recv_list->payload, http_context->buf, header_len);
	}
	else
	{
		recv_list->payload = xy_zalloc(header_len * 2 + 100);
		temp = xy_zalloc(header_len * 2 + 1 + 200);
		bytes2hexstr(http_context->buf, header_len, temp, header_len * 2 + 1);
		memcpy(recv_list->payload, temp, header_len * 2);
	}
	strcat(recv_list->payload, "\r\n");

	if(http_context->print_or_not == 1)
	{
		send_urc_to_ext(recv_list->info);
		xy_free(recv_list->info);
		
		strcat(recv_list->payload, "\r\n");
		send_urc_to_ext(recv_list->payload);
		xy_free(recv_list->payload);
	}else
	{
		vTaskSuspendAll();
		http_add_list_end(&http_context->recv_list, recv_list);
		xTaskResumeAll();
		osSemaphoreRelease(http_context->http_header_sem);
	}

	/* 删除buf中头部内容，将content前移，移动前的部分数据清0*/
	memcpy(http_context->buf, http_context->buf + header_len, http_context->current_recv_len - header_len);
	memset(http_context->buf + http_context->current_recv_len - header_len, 0, header_len);
	/* current_recv_len 更新为去除header头部后的长度， left_ recv_len为最大剩余空间 */
	http_context->current_recv_len = http_context->current_recv_len - header_len;
	http_context->left_recv_len = MAX_HTTP_BUF_LEN - http_context->current_recv_len;			
	if (temp != NULL)
		xy_free(temp);

	return header_len;
}

/*chunk编码格式如下：																													   */
/*[chunk size][\r\n][chunk data][\r\n][chunk size][\r\n][chunk data][\r\n][chunk size = 0][\r\n][\r\n]									  */
/*chunk size是以十六进制的ASCII码表示，比如：头部是3134这两个字节，表示的是1和4这两个ascii字符，被http协议解释为十六进制数14，也就是十进制的20，	*/
/*后面紧跟[\r\n](0d 0a)，再接着是连续的20个字节的chunk正文。chunk数据以0长度的chunk块结束，也就是（30 0d 0a 0d 0a）。							*/
int handle_http_body_chunked(http_context_t *http_context)
{
	int temp_len;
	char *temp = NULL;
	char *cur = NULL;
	int i;
	int chunk_len;
	Http_recv_t *recv_list;
     

	if(http_context->history_processed_len == 0)
	{
find_chunklen_again:
		cur = strstr(http_context->buf, "\r\n");
		if (cur == NULL) {
			return 0; // continue receiving data until we get one entire chunk
		}
		else if( cur == http_context->buf)
		{
			memcpy(http_context->buf, http_context->buf + 2, http_context->current_recv_len - 2);
			memset(http_context->buf + http_context->current_recv_len - 2, 0 , 2);
			http_context->current_recv_len -= 2;
			http_context->left_recv_len += 2;

			goto find_chunklen_again;
		}
		else
		{
			chunk_len = 0;
			temp_len = 1;
			for (i = cur - http_context->buf - 1; i >= 0; i--) {
				if (http_context->buf[i] >= '0' && http_context->buf[i] <= '9') {
					chunk_len += (http_context->buf[i] - '0') * temp_len;
					temp_len *= 16;
				}
				else if (http_context->buf[i] >= 'A' && http_context->buf[i] <= 'F')
				{
					chunk_len += (http_context->buf[i] - 0x37) * temp_len;
					temp_len *= 16;
				}
				else if (http_context->buf[i] >= 'a' && http_context->buf[i] <= 'f')
				{
					chunk_len += (http_context->buf[i] - 0x57) * temp_len;
					temp_len *= 16;
				}
				else
					return -1;
			}
		}
		http_context->chunk_len = chunk_len;
		
		if(http_context->current_recv_len >= cur + 2 - http_context->buf + http_context->chunk_len)
		{
			if(http_context->chunk_len == 0)
			{
				recv_list = New_Recv_Node(HTTP_BODY, 0, NULL, http_context->chunk_len);

				sprintf(recv_list->info, "\r\n+HTTPNMIC:%d,0,0,0\r\n\r\n", http_context->http_id);

				Add_To_RecvLink(http_context, recv_list, HTTP_BODY, 1, 0);

				http_context->http_dealing_state = HTTP_DEALING_NONE;
				http_context->chunk_len = 0;
				/* chunk流程结束后有两个\r\n */
				temp_len = cur + 2 - http_context->buf; // two trailing \r\n

				/* 处理可能的下一次请求的内容*/
				memcpy(http_context->buf, http_context->buf + temp_len, http_context->current_recv_len - temp_len);
				memset(http_context->buf + http_context->current_recv_len -temp_len, 0, temp_len);
				http_context->current_recv_len = http_context->current_recv_len - temp_len;
				http_context->left_recv_len = MAX_HTTP_BUF_LEN - http_context->current_recv_len;
				http_context->is_content_chunked = 0;
				http_context->history_processed_len = 0;
				http_context->http_dealing_state = HTTP_DEALING_NONE;
				return handle_http_body_chunked(http_context);
			}
			else
			{
				recv_list = New_Recv_Node(HTTP_BODY, 1, NULL, http_context->chunk_len);
				sprintf(recv_list->info, "\r\n+HTTPNMIC:%d,%d,%d,%d\r\n", http_context->http_id, 1, http_context->chunk_len, recv_list->len);

				Print_Mode_Convert(recv_list, http_context->print_mode, recv_list->len, cur + 2);

				Add_To_RecvLink(http_context, recv_list, HTTP_BODY, 1, 1);

				http_context->http_dealing_state = HTTP_DEALING_BODY;
				temp_len = cur + 2 - http_context->buf + http_context->chunk_len; // one trailing \r\n
				memcpy(http_context->buf, http_context->buf + temp_len, http_context->current_recv_len - temp_len);
				memset(http_context->buf + http_context->current_recv_len -temp_len, 0, temp_len);

				http_context->current_recv_len = http_context->current_recv_len - temp_len;
				http_context->left_recv_len = MAX_HTTP_BUF_LEN - http_context->current_recv_len;
				http_context->history_processed_len = 0;
				http_context->chunk_len = 0;
				return handle_http_body_chunked(http_context);
			}
		}
		else
		{
			recv_list = New_Recv_Node(HTTP_BODY, 1, NULL, http_context->current_recv_len - (cur + 2 - http_context->buf));

			sprintf(recv_list->info, "\r\n+HTTPNMIC:%d,%d,%d,%d\r\n", http_context->http_id, 1, http_context->chunk_len, recv_list->len);

			Print_Mode_Convert(recv_list, http_context->print_mode, recv_list->len, cur + 2);

			Add_To_RecvLink(http_context, recv_list, HTTP_BODY, 1, 1);

			http_context->http_dealing_state = HTTP_DEALING_BODY;
			http_context->current_recv_len = 0;
			http_context->left_recv_len = MAX_HTTP_BUF_LEN;
			http_context->history_processed_len += recv_list->len;
			return 0;
		}
		
	}
	else if(http_context->history_processed_len != 0)
	{
		if(http_context->current_recv_len + http_context->history_processed_len >= http_context->chunk_len)
		{
			recv_list = New_Recv_Node(HTTP_BODY, 1, NULL, http_context->chunk_len - http_context->history_processed_len);

			sprintf(recv_list->info, "\r\n+HTTPNMIC:%d,%d,%d,%d\r\n", http_context->http_id, 1, http_context->chunk_len, recv_list->len);

			Print_Mode_Convert(recv_list, http_context->print_mode, recv_list->len, http_context->buf);

			Add_To_RecvLink(http_context, recv_list, HTTP_BODY, 1, 1);

			memcpy(http_context->buf, http_context->buf + recv_list->len, http_context->current_recv_len - recv_list->len);
			memset(http_context->buf + http_context->current_recv_len - recv_list->len, 0, recv_list->len);

			http_context->http_dealing_state = HTTP_DEALING_BODY;
			http_context->current_recv_len = http_context->current_recv_len + http_context->history_processed_len - http_context->chunk_len;
			http_context->left_recv_len = MAX_HTTP_BUF_LEN - http_context->current_recv_len;
			http_context->history_processed_len = 0;
			http_context->chunk_len = 0;
			return handle_http_body_chunked(http_context);
		}
		else
		{
			recv_list = New_Recv_Node(HTTP_BODY, 1, NULL, http_context->current_recv_len);

			sprintf(recv_list->info, "\r\n+HTTPNMIC:%d,%d,%d,%d\r\n", http_context->http_id, 1, http_context->chunk_len, recv_list->len);

			Print_Mode_Convert(recv_list, http_context->print_mode, recv_list->len, http_context->buf);

			Add_To_RecvLink(http_context, recv_list, HTTP_BODY, 1, 1);

			http_context->http_dealing_state = HTTP_DEALING_BODY;
			http_context->current_recv_len = 0;
			http_context->left_recv_len = MAX_HTTP_BUF_LEN;
			http_context->history_processed_len += recv_list->len;
			return 0;
		}
	}
}

int handle_http_body(http_context_t *http_context)
{
	int temp_len;
	char *temp = NULL;
	Http_recv_t *recv_list = NULL;
	if (http_context->is_content_chunked) {
		return handle_http_body_chunked(http_context);
	}
	recv_list = xy_zalloc(sizeof(Http_recv_t));
	/* 同一次请求的数据是否收完 */
	/* 接受完： temp_len就是body的总长度，忽略current_recv_len中下一次请求的内容 */
	/* 未接受完： temp_len就是current_recv_len，已接受的总大小 */
	temp_len = http_context->content_length < (http_context->current_recv_len + http_context->history_processed_len)?  // 此处content_length_processed有问题 删掉即可？
			http_context->content_length : (http_context->current_recv_len + http_context->history_processed_len);

	/* 依次打印  id，是否需要继续传输，content_length，此次接受到的数据 四个参数*/
	recv_list->pNext = NULL;
	recv_list->info = xy_zalloc(64);
	sprintf(recv_list->info, "\r\n+HTTPNMIC:%d,%d,%d,%d\r\n", http_context->http_id,
		temp_len < http_context->content_length? 1 : 0, http_context->content_length, temp_len - http_context->history_processed_len -http_context->content_length_processed);

	/* 打印此次接受到的数据内容 */
	if (http_context->print_mode == 0)
	{
		recv_list->payload = xy_zalloc(temp_len - http_context->history_processed_len -http_context->content_length_processed + 100);
		memcpy(recv_list->payload, http_context->buf + http_context->content_length_processed, temp_len - http_context->history_processed_len -http_context->content_length_processed);
	}
	else
	{
		recv_list->payload = xy_zalloc((temp_len - http_context->history_processed_len -http_context->content_length_processed) * 2  + 100);
		temp = xy_zalloc((temp_len - http_context->history_processed_len -http_context->content_length_processed) * 2 + 1  + 200);
		bytes2hexstr(http_context->buf + http_context->content_length_processed, temp_len - http_context->content_length_processed - http_context->history_processed_len ,
				temp, (temp_len - http_context->content_length_processed  - http_context->history_processed_len ) * 2 + 1);
		memcpy(recv_list->payload, temp, (temp_len - http_context->content_length_processed  - http_context->history_processed_len ) * 2);
		xy_free(temp);
	}

	/* 是否还需要继续传输*/
	recv_list->type = HTTP_BODY;
	recv_list->more = temp_len < http_context->content_length? 1 : 0;
	recv_list->len = temp_len - http_context->history_processed_len -http_context->content_length_processed;

	if(http_context->print_or_not == 1)
	{
		send_urc_to_ext(recv_list->info);
		xy_free(recv_list->info);
		
		strcat(recv_list->payload, "\r\n");
		send_urc_to_ext(recv_list->payload);
		xy_free(recv_list->payload);
	}else
	{
		/* 将数据挂链表*/
		vTaskSuspendAll();
		http_add_list_end(&http_context->recv_list, recv_list);
		xTaskResumeAll();
		osSemaphoreRelease(http_context->http_body_sem);
	}

	/*	此次请求的数据未接受完： 标志位置为HTTP_DEALING_BODY*/
	if (temp_len < http_context->content_length) {
		http_context->http_dealing_state = HTTP_DEALING_BODY;
		http_context->content_length_processed = temp_len - http_context->history_processed_len;
	} 
	/*	此次请求完成： current_recv_len中的多余部分为下一次请求的内容，需要继续处理*/
	else {
		http_context->http_dealing_state = HTTP_DEALING_NONE;
		http_context->content_length_processed = 0;
		memcpy(http_context->buf, http_context->buf + temp_len - http_context->history_processed_len, http_context->current_recv_len + http_context->history_processed_len- temp_len);
		memset(http_context->buf + http_context->current_recv_len + http_context->history_processed_len - temp_len, 0, temp_len - http_context->history_processed_len);
		http_context->current_recv_len = http_context->current_recv_len+ http_context->history_processed_len - temp_len;
		http_context->left_recv_len = MAX_HTTP_BUF_LEN - http_context->current_recv_len;
		http_context->history_processed_len = 0;
	}

	return 0;
}

int handle_http_data(http_context_t *http_context)
{
	int ret;

	if (http_context->http_dealing_state == HTTP_DEALING_NONE || http_context->http_dealing_state == HTTP_DEALING_HEADER)
	{
		/* 返回值0 表示header未接受完，   它值表示header len*/
		ret = handle_http_header(http_context);
		if (ret == 0) {
			/* 继续未完成的header*/
			http_context->http_dealing_state = HTTP_DEALING_HEADER;
		} else {
			/* 头部处理完成，开始处理content*/
			/* content长度不为0*/
			if (http_context->content_length > 0 || http_context->is_content_chunked) {
				/* 收到的content不为0： 收到数据	（1）*/
				if (http_context->current_recv_len > 0) {
					return handle_http_body(http_context);
				}
				 /* 收到的content不为0 ： 有数据但暂时没收到	（2）*/
				else {
					http_context->http_dealing_state = HTTP_DEALING_FINISH_HEADER;
				}
			}
			/* content长度为0，包内容为空，此包已处理完成。但仍收到数据，因此需要处理下一包*/
			else {
				http_context->http_dealing_state = HTTP_DEALING_NONE;
				if (http_context->current_recv_len > 0) {
					return handle_http_data(http_context); // return or continue dealing next http package?	
				}
			}
		}
	}
	/* （2） 继续recv并收到数据*/
	else if (http_context->http_dealing_state == HTTP_DEALING_FINISH_HEADER) {		//与上文配合
		return handle_http_body(http_context);
	} 
	/* （1） 继续处理body*/
	else if (http_context->http_dealing_state == HTTP_DEALING_BODY) {					//与上文配合
		return handle_http_body(http_context);
	}
	return 0;
}

int https_close(http_context_t *http_context)
{
	if (http_context->tls_context != NULL)
		xy_tls_ssl_destroy(http_context->tls_context);
	http_context->tls_context = NULL;
	return 0;
}

int http_close(http_context_t *http_context)
{
	if (http_context->secured)
	{
		https_close(http_context);
	}
	else
	{
		if (http_context->sock == -1) {
			return 0; // treat this as sock already closed
		}
		close(http_context->sock);
		http_context->sock = -1;
	}

	http_context->http_dealing_state = HTTP_DEALING_NONE;
	http_context->current_recv_len = 0;
	http_context->left_recv_len = MAX_HTTP_BUF_LEN;
	http_context->current_processed_len = 0;
	http_context->content_length = 0;
	http_context->content_length_processed = 0;
	return 0;
}

int http_recv(http_context_t *http_context, unsigned char *buf, int size, int timeout)
{
	int ret;
	int max_fd;
	fd_set readfds, exceptfds;
	struct timeval tv;
	if (http_context->secured)
	{
		ret = xy_tls_read(http_context->tls_context, buf, size, timeout * 1000 /*ms*/);
		if (ret == -2) // timeout
		{
			return HTTP_RET_TIMEOUT;
		}
		else if (ret == -3) // MBEDTLS_ERR_SSL_WANT_READ
		{
			return HTTP_RET_AGAIN;
		}
		else if (ret < 0) // error
		{
			return HTTP_RET_ERROR;
		}
		else if (ret == 0) // connection is closed by peer end
		{
			return HTTP_RET_REMOTE_CLOSED;
		}
		return ret;
	}
	else
	{
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_ZERO(&exceptfds);

		max_fd = http_context->sock;
		FD_SET(http_context->sock, &readfds);
		FD_SET(http_context->sock, &exceptfds);

		if (max_fd < 0)
		{
			return -1;
		}

		ret = select(max_fd + 1, &readfds, NULL, &exceptfds, &tv);
		if (ret < 0){
			if (errno == EINTR)
			{
				return HTTP_RET_AGAIN;
			}
			else if(errno == EBADF)
			{
				//one or more sockets may be closed asynchronously by AT+NSOCL
				xy_assert(0);
				return HTTP_RET_ERROR;
			}
			else
			{
				//xy_assert(0);
				return HTTP_RET_ERROR;
			}
		}
		else if (ret == 0)
		{
			//we assume select timeouted here
			return HTTP_RET_TIMEOUT;
		}

		if(FD_ISSET(http_context->sock, &exceptfds))
		{
			softap_printf(USER_LOG, WARN_LOG, "except exist, force to close socket");
			return HTTP_RET_ERROR;
		}

		if (!FD_ISSET(http_context->sock, &readfds)) {
			return HTTP_RET_TIMEOUT;
		}

		ret = recv(http_context->sock, buf, size, MSG_DONTWAIT);

		if (ret < 0)
		{
			if (errno == EWOULDBLOCK) // time out
			{
				return HTTP_RET_TIMEOUT;
			}
			else
			{
				return HTTP_RET_ERROR;
			}
		}
		else if (ret == 0) {
			return HTTP_RET_REMOTE_CLOSED;
		} else {
			return ret;
		}
	}
}



