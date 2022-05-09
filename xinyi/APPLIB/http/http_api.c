#include "xy_http_client.h"
#include "xy_utils.h"
#include "at_http.h"
#include "lwip/errno.h"
#include "lwip/sockets.h"
#include "http_api.h"
http_context_reference_t http_context_refs[HTTP_CONTEXT_REF_NUM];
/** 
* @brief       创建HTTP/HTTPS实例
* @attention   目前仅支持一个实例
* @param 	   host		     ：此API将解析host，获取到domain_name和port参数
* @param   	   auth_user	 ：授权的用户名
* @param   	   auth_passwd	 ：授权的密码
* @return      成功返回	 ： 返回已成功初始化的http id
* @return      失败返回	 ： XY_ERR
**/

int xy_http_create(char * host,char * auth_user, char * auth_passwd)
{
	volatile int http_id = 0;
	char ret = XY_OK;

	/* 目前仅支持一个实例, 多请求需要先CLOSE.*/
	ret = get_free_http_context_ref(&http_id);
	if (ret == XY_ERR){
		return XY_ERR;
	}

	ret = Http_User_Info(&http_context_refs[http_id], host, auth_user,  auth_passwd);
	if (ret == XY_ERR){
		return XY_ERR;
	}
	return http_id;
}


/** 
* @brief       HTTPS需要通过此命令配置实例
* @attention   仅HTTPS需要使用此AT命令
* @param 	   http_id：由at_http_create创建的http实例id
* @param 	   type：cfg类型如下
*				typedef enum type{
*				 	SEVER_CERT = 1,   		// 服务器 cert 证书
*				 	CLIENT_CERT = 2,  		// 客户端 cert 证书
*				 	CLIENT_PK = 3	  		// 客户端 pk
*					PRINT_MODE = 8			// 配置打印模式:  1 代表用16进制模式打印收到的字符， 0 代表字符串模式
*				} http_cfg_type_e;
* @param   	   data
* @param   	   encode_method：加密方式如下
*				typedef enum encode_method{
*					ESCAPE_MECHANISM = 0, 	// 转义字符串
*					HEX_CHARACTER = 1	  	// 十六进制字符串
*				} http_encode_method_e;
* @return      成功返回	 ： XY_OK
* @return      失败返回	 ： XY_NOK
**/
int xy_http_cfg(int http_id, int type, char *data, int encode_method)
{
	char *temp_data = NULL;
	int len;
	http_context_reference_t *http_context_ref = NULL;

	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);
	
	http_context_ref = &http_context_refs[http_id];

	if (http_context_ref->http_context == NULL)
		goto failed;

	if (type != SEVER_CERT && type != CLIENT_CERT && type != CLIENT_PK && type != PRINT_MODE)
		goto failed;

	if ((data != NULL) && (strcmp(data, "") != 0)) {
		/* 编码模式 0， 直接取值*/
		if (encode_method == 0) {
			len = strlen(data);
			temp_data = xy_zalloc(len + 1);
			if (temp_data == NULL) {
				goto failed;
			}
			strcpy(temp_data, data);
		}
		/* 编码模式 1， 16进制数转化为ascii码*/
		else if(encode_method == 1) {
			len = strlen(data) / 2;
			temp_data = xy_zalloc(len + 1);
			if (temp_data == NULL) {
				goto failed;
			}
			if (hexstr2bytes(data, len * 2, temp_data, len) < 0) {
				goto failed;
			}
		} else {
			goto failed;
		}

		/*根据不同的type传参，决定实例的不同配置请求*/
		/* 按不同的请求，先删除上次保存的实例，然后将data传参填入实例结构体中。 */
		switch (type) {
			case 1: // 服务器cert证书
			{
				if (http_context_ref->http_context->server_cert == NULL) {
					http_context_ref->http_context->server_cert = temp_data;
				} else {
					xy_free(http_context_ref->http_context->server_cert);
					http_context_ref->http_context->server_cert = temp_data;
				}
				break;
			}
			case 2: // 客户端cert证书
			{
				if (http_context_ref->http_context->client_cert == NULL) {
					http_context_ref->http_context->client_cert = temp_data;
				} else {
					xy_free(http_context_ref->http_context->client_cert);
					http_context_ref->http_context->client_cert = temp_data;
				}
				break;
			}
			case 3: // 客户端 private key
			{
				if (http_context_ref->http_context->client_pk == NULL) {
					http_context_ref->http_context->client_pk = temp_data;
				} else {
					xy_free(http_context_ref->http_context->client_pk);
					http_context_ref->http_context->client_pk = temp_data;
				}
				break;
			}
			case 8:// 打印模式配置： 1 代表用16进制模式打印收到的字符， 0 代表字符串模式
			{
				http_context_ref->http_context->print_mode = (int)strtol(temp_data,NULL,10);		
				break;
			}
			default:
				goto failed;
		}
	}
	return XY_OK;

failed:
	if (temp_data != NULL) {
		xy_free(temp_data);
	}
	return XY_ERR;
}

/** 
* @brief       设置HTTP 头部信息。
* @attention   HTTP 协议要求 header 字段之间以\r\n 分隔
* @attention   当输入的 header 字段为空时，将清空此前已输入 header，否则新输入的 header 将添加在上一次 header 输入之后。
* @param 	   http_id：由at_http_create创建的http实例id
* @param   	   header：将要配置的头部信息
* @param   	   encode_method：加密方式如下
*				typedef enum encode_method{
*					ESCAPE_MECHANISM = 0, 	// 转义字符串
*					HEX_CHARACTER = 1	  	// 十六进制字符串
*				} http_encode_method_e;
* @return      成功返回：XY_OK
* @return      失败返回：XY_ERR
**/
int xy_http_header(int http_id, char *header, int encode_method)
{
	http_context_reference_t *http_context_ref = NULL;
	char *temp_header_content = NULL;
	char *temp = NULL;
	int len;

	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);
	
	http_context_ref = &http_context_refs[http_id];

	if (http_context_ref->http_context == NULL)
		goto failed;

	/* 若header参数非空，根据encode_method编码方式解析读取到的字符串*/
	if ((header != NULL) && (strcmp(header, "") != 0)) {
		if (encode_method == 0) {
			len = strlen(header);
			temp_header_content = xy_zalloc(len + 1);
			if (temp_header_content == NULL) {
				goto failed;
			}
			strcpy(temp_header_content, header);
		}
		else if(encode_method == 1) {
			len = strlen(header) / 2;
			temp_header_content = xy_zalloc(len + 1);
			if (temp_header_content == NULL) {
				goto failed;
			}
			if (hexstr2bytes(header, len * 2, temp_header_content, len) < 0) {
				goto failed;
			}
		} else {
			goto failed;
		}

		/* 当输入的 header 字段非空时，新输入的 header 将添加在上一次 header 输入之后。 */
		if (http_context_ref->http_context->custom_header == NULL) {
			http_context_ref->http_context->custom_header = temp_header_content;
		} else {
			temp = xy_zalloc(strlen(http_context_ref->http_context->custom_header) + strlen(temp_header_content) + 1);
			strcpy(temp, http_context_ref->http_context->custom_header);
			strcpy(temp + strlen(http_context_ref->http_context->custom_header), temp_header_content);
			http_context_ref->http_context->custom_header = temp;
			xy_free(temp_header_content);
		}
	}
	/* 当输入的 header 字段为空时，将清空此前已输入 header */
	else {
		if(http_context_ref->http_context->custom_header != NULL)
			xy_free(http_context_ref->http_context->custom_header);
		http_context_ref->http_context->custom_header = NULL;
	}

out:
	return XY_OK;

failed:
	if (temp_header_content != NULL) {
		xy_free(temp_header_content);
	}
	return XY_ERR;
}

/** 
* @brief       设置HTTP content 信息。
* @attention   HTTP 协议要求 content 字段之间以\r\n 分隔
* @attention   当输入的 content 字段为空时，将清空此前已输入 content，否则新输入的 content 将添加在上一次 content 输入之后。
* @param 	   http_id：由at_http_create创建的http实例id
* @param   	   content：将要配置的content信息
* @param   	   encode_method：加密方式如下
*				typedef enum encode_method{
*					ESCAPE_MECHANISM = 0, 	// 转义字符串
*					HEX_CHARACTER = 1	  	// 十六进制字符串
*				} http_encode_method_e;
* @return      成功返回：XY_OK
* @return      失败返回：XY_ERR
**/
int xy_http_content(int http_id, char *content, int encode_method)
{
	http_context_reference_t *http_context_ref = NULL;
	char *temp_content = NULL;
	char *temp = NULL;
	int len;

	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);

	http_context_ref = &http_context_refs[http_id];

	if (http_context_ref->http_context == NULL)
		goto failed;

	if ((content != NULL) && (strcmp(content, "") != 0)) {
		if (encode_method == 0) {
			len = strlen(content);
			temp_content = xy_zalloc(len + 1);
			if (temp_content == NULL) {
				goto failed;
			}
			strcpy(temp_content, content);
		}
		else if(encode_method == 1) {
			len = strlen(content) / 2;
			temp_content = xy_zalloc(len + 1);
			if (temp_content == NULL) {
				goto failed;
			}
			if (hexstr2bytes(content, len * 2, temp_content, len) < 0) {
				goto failed;
			}
		} else {
			goto failed;
		}

		if (http_context_ref->http_context->content == NULL) {
			http_context_ref->http_context->content = temp_content;
		} else {
			temp = xy_zalloc(strlen(http_context_ref->http_context->content) + strlen(temp_content) + 1);
			strcpy(temp, http_context_ref->http_context->content);
			strcpy(temp + strlen(http_context_ref->http_context->content), temp_content);
			http_context_ref->http_context->content = temp;
			xy_free(temp_content);
		}
	}
	else {
		if(http_context_ref->http_context->content != NULL)
			xy_free(http_context_ref->http_context->content);
		http_context_ref->http_context->content = NULL;
	}
	return XY_OK;

failed:
	if (temp_content != NULL) {
		xy_free(temp_content);
	}
	return XY_ERR;
}

/** 
* @brief       根据输入的http_id找到对应实例，建立socket的connect连接，并创建一个新线程recv服务器的socket数据。
* @param 	   http_id		：由at_http_create创建的http实例id
* @return      成功	： XY_OK
* @return      失败	： XY_NOK
**/
int xy_http_connect(int http_id)
{
	http_context_reference_t *http_context_ref = NULL;
	osThreadAttr_t thread_attr = {0};
	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);

	http_context_ref = &http_context_refs[http_id];

	if (http_context_ref->http_context == NULL)
		xy_assert(0);

	if (xy_wait_tcpip_ok(10) != XY_OK)
		goto failed;


	if (http_connect(http_context_ref->http_context) < 0) {
		goto failed;
	}


	if (http_context_ref->http_recv_thread_id == 0)
	{
		thread_attr.name	   = "http_recv";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = 0x1000;
		http_context_ref->http_recv_thread_id = osThreadNew((osThreadFunc_t)(at_http_recv), http_context_ref, &thread_attr);
	}
	return XY_OK;
failed:
	return XY_ERR;
}


/** 
* @brief       发送 HTTP 请求
* @param 	   http_id		：由at_http_create创建的http实例id
* @param   	   method		：提供的请求如下
*				typedef enum {
*					HTTP_METHOD_GET = 0,
*					HTTP_METHOD_POST,
*					HTTP_METHOD_PUT,
*					HTTP_METHOD_DELETE,
*					HTTP_METHOD_CUSTOM = 99
*				} http_method_e;
* @param   	  path			:请求的 URL path 绝对路径参数
* @return      成功	： XY_OK
* @return      失败	： XY_NOK
**/
int xy_http_request(int http_id, int http_method, char *path)
{
	char *http_content = NULL;
	int ret;
	http_context_t * http_context = NULL;
		
	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);

	 http_context = ((http_context_reference_t *)&http_context_refs[http_id])->http_context;


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
			/*%s %s HTTP/1.1\r\nHost:%s\r\nConnection:keep-alive\r\nContent-Length:%u\r\nContent-Type:text/plain\r\n  +  "Authorization: Basic %s\r\n   +  content*/
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
			/*"DELETE %s HTTP/1.1\r\nHost:%s\r\nConnection:keep-alive\r\n"	*/
			/*"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/86.0.4240.75\r\n");   +  "Authorization: Basic %s\r\n   +  content */
			http_content = make_http_delete_content(http_context, path);
			break;
		}
		case HTTP_METHOD_HEAD:
		{
			/*"HEAD %s HTTP/1.1\r\nHost:%s\r\nConnection:keep-alive\r\n"	*/
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


/** 
* @brief       获取http的应答数据头
* @param 	   http_id		：由at_http_create创建的http实例id
* @param   	   buf			: 返回http请求的数据本体
* @return      读取到的数据长度
**/
#define HTTP_HEADER 0
#define HTTP_BODY 	1
#define HTTP_FOREVER 0xFFFFFFFF
#define HTTP_NO_WAIT 0

int  xy_http_recv_header(int http_id, char *buf,  int input_len, unsigned int timeout)
{
	int len = input_len;
	Http_recv_t * recv_list = NULL;
	Http_recv_t * tmp_recv = NULL;
	int copylen_last = 0;
	int copylen = 0;
	char already_wait = 0;
	char stat = 0;
	char* len_pos = NULL;
	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);

	recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
again:
	if(recv_list != NULL)
	{
		while(osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_header_sem, 0) == osOK);
		tmp_recv = recv_list;
	}
	else
	{
		if(already_wait == 0)
		{
			// delay (timeout) ms， return no matter how many data got.
			if(timeout != HTTP_FOREVER)
			{
				already_wait = 1;
				stat = osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_header_sem, timeout);
				if(stat == osOK)
				{
					recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
					goto again;
				}
				return 	copylen_last;
			}
			// have to got enough data
			else
			{
				if(copylen_last < input_len)
				{
					if(  ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header == 1 || ((http_context_reference_t *)&http_context_refs[http_id])->http_context->data_recv_ever == 0	)			
					{
						stat = osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_header_sem, timeout);
						if(stat == osOK)
						{
							recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
							goto again;
						}
					}
					else if(  ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header == 0 )
					{
						return 	copylen_last;
					}
					else
						xy_assert(0);
				}
				else if(copylen_last >= input_len)
				{
					return copylen_last;
				}

			}
		}
		return 	copylen_last;
	} 

	if(tmp_recv != NULL)
	{
		if(tmp_recv->type == HTTP_BODY)
		{
			recv_list = recv_list->pNext;
			goto again;
		}
		else if(tmp_recv->type == HTTP_HEADER)
		{	
			copylen = tmp_recv->len;
			if(copylen < len)
			{
				send_urc_to_ext(tmp_recv->info);
				memcpy(buf + copylen_last, tmp_recv->payload , copylen);
				len -= copylen;
				copylen_last += copylen;
				
				vTaskSuspendAll();
				((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header = tmp_recv->more;
				http_delete_node(((http_context_reference_t *)&http_context_refs[http_id])->http_context, tmp_recv);
				xTaskResumeAll();
				recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
				goto again;
			}
			else if(copylen > len)
			{
				vTaskSuspendAll();
				len_pos = strrchr(tmp_recv->info, ',');
				if(len_pos != NULL)
					sprintf(len_pos + 1, "%d\r\n", len);
				xTaskResumeAll();
					
				send_urc_to_ext(tmp_recv->info);
				vTaskSuspendAll();
				((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header = 1;
				memcpy(buf + copylen_last, tmp_recv->payload , len);
				memcpy(tmp_recv->payload, tmp_recv->payload + len, copylen - len);
				memcpy(tmp_recv->payload + copylen - len, 0, len);
				tmp_recv->len = copylen - len;

				len_pos = strrchr(tmp_recv->info, ',');
				if(len_pos != NULL)
					sprintf(len_pos + 1, "%d\r\n", tmp_recv->len);
				xTaskResumeAll();
				copylen_last += len;	
			}
			else
			{
				send_urc_to_ext(tmp_recv->info);
				memcpy(buf + copylen_last, tmp_recv->payload , len);
				vTaskSuspendAll();
				((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header = tmp_recv->more;
				http_delete_node(((http_context_reference_t *)&http_context_refs[http_id])->http_context, tmp_recv);
				xTaskResumeAll();
				copylen_last += len;
			}
				
		}
		else if( http_socket_disconnect(http_id) == XY_TRUE && tmp_recv->type == HTTP_SOCKERR)
		{
			osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_body_sem, 0);
		    vTaskSuspendAll();
			((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header = 0;
			http_delete_node(((http_context_reference_t *)&http_context_refs[http_id])->http_context, tmp_recv);
		    xTaskResumeAll();

			return copylen_last;
		}
	}
	return copylen_last;
}


/** 
* @brief       获取http的应答数据本体
* @param 	   http_id		：由at_http_create创建的http实例id
* @param   	   buf			: 返回http请求的数据头
* @return      读取到的数据长度
**/
int  xy_http_recv_body(int http_id, char *buf,  int input_len, unsigned int timeout)
{
	int len = input_len;
	Http_recv_t * recv_list = NULL;
	Http_recv_t * tmp_recv = NULL;
	int copylen_last = 0;
	int copylen = 0;
	char already_wait = 0;
	char stat = 0;
	char* len_pos = NULL;

	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);

	recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
again:
	if(recv_list != NULL)
	{
		while(osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_body_sem, 0) == osOK);
		tmp_recv = recv_list;
	}
	else
	{
		if(already_wait == 0)
		{
			// delay (timeout) ms， return no matter how many data got.
			if(timeout != HTTP_FOREVER)
			{
				already_wait = 1;
				stat = osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_body_sem, timeout);
				if(stat == osOK)
				{
					recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
					goto again;
				}
				return 	copylen_last;
			}
			// have to got enough data
			else
			{
				if(copylen_last < input_len)
				{
					if(  ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body == 1 )
					{
						stat = osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_body_sem, timeout);
						if(stat == osOK)
						{
							recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
							goto again;
						}
					}
					else if(  ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body == 0 )
					{
						return 	copylen_last;
					}
					else
						xy_assert(0);
				}
				else if(copylen_last >= input_len)
				{
					return copylen_last;
				}

			}
		}
		return 	copylen_last;
	} 

	if(tmp_recv != NULL)
	{
		if(tmp_recv->type == HTTP_HEADER)
		{
			vTaskSuspendAll();
			http_delete_node(((http_context_reference_t *)&http_context_refs[http_id])->http_context, tmp_recv);
			xTaskResumeAll();
			recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
			goto again;
		}
		else if(tmp_recv->type == HTTP_BODY)
		{	
			copylen = tmp_recv->len;

			if(copylen < len)
			{
				send_urc_to_ext(tmp_recv->info);
				memcpy(buf + copylen_last, tmp_recv->payload , copylen);
				len -= copylen;
				copylen_last += copylen;

				vTaskSuspendAll();
				((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body = tmp_recv->more;
				http_delete_node(((http_context_reference_t *)&http_context_refs[http_id])->http_context, tmp_recv);
				xTaskResumeAll();
				recv_list = ((http_context_reference_t *)&http_context_refs[http_id])->http_context->recv_list;
				goto again;
			}
			else if(copylen > len)
			{
				vTaskSuspendAll();
				len_pos = strrchr(tmp_recv->info, ',');
				if(len_pos != NULL)
					sprintf(len_pos + 1, "%d\r\n", len);
				xTaskResumeAll();

				send_urc_to_ext(tmp_recv->info);
				vTaskSuspendAll();
				((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body = 1;
				memcpy(buf + copylen_last, tmp_recv->payload , len);
				memcpy(tmp_recv->payload, tmp_recv->payload + len, copylen - len);
				memcpy(tmp_recv->payload + copylen - len, 0, len);
				tmp_recv->len = copylen - len;

				len_pos = strrchr(tmp_recv->info, ',');
				if(len_pos != NULL)
					sprintf(len_pos + 1, "%d\r\n", tmp_recv->len);
				xTaskResumeAll();
				copylen_last += len;	
			}
			else
			{
				send_urc_to_ext(tmp_recv->info);
				memcpy(buf + copylen_last, tmp_recv->payload , len);

				vTaskSuspendAll();
				((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body = tmp_recv->more;
				http_delete_node(((http_context_reference_t *)&http_context_refs[http_id])->http_context, tmp_recv);
				xTaskResumeAll();
				copylen_last += len;
			}
				
		}
		else if(http_socket_disconnect(http_id) == XY_TRUE && tmp_recv->type == HTTP_SOCKERR)
		{
			osSemaphoreAcquire(((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_header_sem, 0);
		  	 vTaskSuspendAll();
			((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body = 0;
			http_delete_node(((http_context_reference_t *)&http_context_refs[http_id])->http_context, tmp_recv);
		    	xTaskResumeAll();

			return copylen_last;
		}
	}
	return copylen_last;
}

/** 
* @brief       关闭http实例
* @param 	   http_id		：由at_http_create创建的http实例id
* @param   	   buf			: 返回http请求的数据头
**/
int xy_http_close(int http_id)
{
	http_context_reference_t *http_context_ref = NULL;
	http_context_ref = &http_context_refs[http_id];

	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);	

	if (http_context_ref->http_context == NULL)
		xy_assert(0);

	http_context_ref->quit = 1;

	while (http_context_ref->http_context->sock != -1)
	{
		osDelay(100);
	}
	while (http_context_ref->http_context->tls_context != NULL)
	{
		osDelay(100);
	}
	while (http_context_ref->http_recv_thread_id != 0)
	{
		osDelay(100);
	}

	if (http_close(http_context_ref->http_context) < 0) {
		xy_assert(0);
	}

	http_context_clear(http_context_ref->http_context);
	xy_free(http_context_ref->http_context);
	memset(http_context_ref, 0, sizeof(http_context_reference_t));
	return 0;

}

char header_recvlist_empty(int http_id)
{
	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);	

	if( ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header == 0 )
		return XY_TRUE;
	else if( ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_header == 1 )
	{
		return XY_FALSE;
	}
}
char body_recvlist_empty(int http_id)
{
	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);	

	if( ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body == 0 )
		return XY_TRUE;
	else if( ((http_context_reference_t *)&http_context_refs[http_id])->http_context->http_more_body == 1 )
		return XY_FALSE;
}

char http_socket_disconnect(int http_id)
{
	xy_assert(http_id >= 0 && http_id < HTTP_CONTEXT_REF_NUM);	
	
	if(((http_context_reference_t *)&http_context_refs[http_id])->http_context->sock == -1)
		return XY_TRUE;
	else
		return XY_FALSE;
}
