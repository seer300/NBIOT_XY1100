#include <string.h>
#include <stdint.h>

#include "at_http.h"
#include "http_api.h"
#include "xy_http_client.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "at_global_def.h"
#include "xy_http_client.h"

osMessageQueueId_t http_create_msg_q = NULL;
extern http_context_reference_t http_context_refs[];

/** 
* @brief       创建HTTP/HTTPS实例
* @attention   目前仅支持一个实例
* @param 	   host		     ：此API将解析host，获取到domain_name和port参数
* @param   	   auth_user	 ：授权的用户名
* @param   	   auth_passwd	 ：授权的密码
* @return      成功			 : 	打印OK
* @return      失败	     	 ： 打印ERROR
**/
int at_http_create(char *at_buf, char **rsp_cmd)
{
	int http_id = 0;
	int parsed_num;
	char *host = xy_zalloc(128);
	char *auth_user = xy_zalloc(128);
	char *auth_passwd = xy_zalloc(128);
	http_context_reference_t *http_context_ref = NULL;
	int ret = XY_OK;
	osThreadAttr_t thread_attr = {0};

	*rsp_cmd = xy_zalloc(64);

	if (at_parse_param_3("%s,%s,%s", at_buf, &parsed_num, host, auth_user, auth_passwd) != AT_OK) {
		goto stop_create;
	}

	if(parsed_num <= 0 || parsed_num > 3)
	{
		goto stop_create;
	}

	ret = get_free_http_context_ref(&http_id);
	if (ret == XY_ERR){
		goto stop_create;
	}

	http_context_ref = &http_context_refs[http_id];
	http_context_ref->http_context->print_or_not = 1;

	ret = Http_User_Info(http_context_ref, host, auth_user, auth_passwd);
	if (ret == XY_ERR){
		goto stop_create;
	}

	/*AT命令触发的HTTP将转发至AT HTTP线程处理*/
	/* 队列在create中被创建，在close中删除*/
    http_create_msg_q = osMessageQueueNew(32, sizeof(uint32_t *), NULL);
	
	xy_assert(http_context_ref->http_at_thread_id == 0);
	thread_attr.name       = "at_http_parse";
	thread_attr.priority   = XY_OS_PRIO_NORMAL1;
	thread_attr.stack_size = 0x1200;
	http_context_ref->http_at_thread_id = osThreadNew((osThreadFunc_t)(at_http_parse), http_context_ref, &thread_attr);
	
	sprintf(*rsp_cmd, "\r\n+HTTPCREATE:%d\r\n\r\nOK\r\n", http_id);
	xy_free(host);
	xy_free(auth_user);
	xy_free(auth_passwd);
	return AT_END;

stop_create:
	sprintf(*rsp_cmd, "\r\nERROR\r\n");
	xy_free(host);
	xy_free(auth_user);
	xy_free(auth_passwd);
	return AT_END;
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
* @param   	   encode_method：编码方式如下
*				typedef enum encode_method{
*					ESCAPE_MECHANISM = 0, 	// 转义字符串
*					HEX_CHARACTER = 1	  	// 十六进制字符串
*				} http_encode_method_e;
* @return      成功打印	 ： \r\n+HTTPCFG\r\nOK\r\n
* @return      失败打印	 ： \r\n+HTTPCFG\r\nERROR\r\n
**/
int at_http_cfg(char *at_buf, char **rsp_cmd)
{
	int parsed_num;
	int type = -1;
	int http_id = -1;
	int encode_method = ESCAPE_MECHANISM;	// 此处不赋初值会被异常优化
	char *data = xy_zalloc(6 * 1024);
	int ret = XY_OK;

	*rsp_cmd = xy_zalloc(100);

	if (at_parse_param_3("%d,%d,%s,%d", at_buf, &parsed_num, &http_id, &type, data, &encode_method) != AT_OK) {
		goto cfg_error;
	}

	if(parsed_num < 2 || parsed_num > 4)
	{
		goto cfg_error;
	}

	if(http_id < 0 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto cfg_error;
	}

	if (type != SEVER_CERT && type != CLIENT_CERT && type != CLIENT_PK && type != PRINT_MODE)
		goto cfg_error;

	if(parsed_num == 4)
	{
		if(encode_method != ESCAPE_MECHANISM && encode_method != HEX_CHARACTER)
		{
			goto cfg_error;
		}
	}

	ret = xy_http_cfg(http_id, type, data, encode_method);
	if (ret == XY_ERR){
		goto cfg_error;
	}
	sprintf(*rsp_cmd, "\r\n+HTTPCFG\r\n\r\nOK\r\n");
	xy_free(data);
	return AT_END;

cfg_error:
	sprintf(*rsp_cmd, "\r\nERROR\r\n");
	xy_free(data);
	return AT_END;
}

/** 
* @brief       设置或查询 HTTP 头部信息。
* @attention   HTTP 协议要求 header 字段之间以\r\n 分隔
* @attention   单条header长度限制为256字节，超出限制可能导致异常死机。
* @attention   当输入的 header 字段为空时，将清空此前已输入 header，否则新输入的 header 将添加在上一次 header 输入之后。
* @param 	   http_id：由at_http_create创建的http实例id
* @param   	   header：将要配置的头部信息
* @param   	   encode_method：编码方式如下
*				typedef enum encode_method{
*					ESCAPE_MECHANISM = 0, 	// 转义字符串
*					HEX_CHARACTER = 1	  	// 十六进制字符串
*				} http_encode_method_e;
* @output      成功响应则输出：
					先打印OK表示AT命令成功处理
					（1）仅输入id参数
						（1.1）当前信息头部为空
								OK
						（1.2）当前信息头部非空
								返回已输入的HTTP头部信息 +HTTPHEADER:<id>,<length><CR><LF><header>
					（2）id参数和头部格式正确
								OK	
* @output       失败响应则输出：
						ERROR
**/
int at_http_header(char *at_buf, char **rsp_cmd)
{
	int parsed_num;
	int http_id = -1;
	int encode_method = ESCAPE_MECHANISM;
	char *header = xy_zalloc(256);
	http_context_reference_t *http_context_ref = NULL;
	int str_len = 0;
	int ret = XY_OK;

	*rsp_cmd = xy_zalloc(256);

	if (at_parse_param_3("%d,%s,%d", at_buf, &parsed_num, &http_id, header, &encode_method) != AT_OK) {					
		goto header_error;
	}

	if(parsed_num == 0 || parsed_num > 3 ){
		goto header_error;
	}

	if(http_id < 0 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto header_error;
	}

	if( parsed_num == 3)
	{
		if(encode_method != ESCAPE_MECHANISM && encode_method != HEX_CHARACTER)
		{
			goto header_error;
		}
	}

	http_context_ref = &http_context_refs[http_id];
	if (http_context_ref->http_context == NULL){
		goto header_error;
	}else{
		if (parsed_num == 1) {
			if (http_context_ref->http_context->custom_header != NULL)
			{
				str_len += sprintf(*rsp_cmd + str_len, "\r\n+HTTPHEADER:%d,%d\r\n%s",
					http_context_ref->http_context->http_id, strlen(http_context_ref->http_context->custom_header),
					http_context_ref->http_context->custom_header);
			}
		}
		else{
			ret = xy_http_header(http_id, header, encode_method);
			if(ret == XY_ERR){
				goto header_error;
			}
		}
	}
	xy_free(header);
	sprintf(*rsp_cmd + str_len, "\r\nOK\r\n");
	return AT_END;

header_error:
	xy_free(header);
	sprintf(*rsp_cmd, "\r\nERROR\r\n");
	return AT_END;
}

/* 类比header， 功能完全相似*/
int at_http_content(char *at_buf, char **rsp_cmd)
{
	int parsed_num;
	int http_id = -1;
	int encode_method = ESCAPE_MECHANISM;
	char *content = xy_zalloc(512);
	http_context_reference_t *http_context_ref = NULL;
	int ret = XY_OK;
	int str_len = 0;

	*rsp_cmd = xy_zalloc(256);

	if (at_parse_param_3("%d,%s,%d", at_buf, &parsed_num, &http_id, content, &encode_method) != AT_OK) {
		goto content_failed;
	}

	if(parsed_num == 0 || parsed_num > 3 ){
		goto content_failed;
	}

	if(http_id < 0 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto content_failed;
	}

	if( parsed_num == 3)
	{
		if(encode_method != ESCAPE_MECHANISM && encode_method != HEX_CHARACTER)
		{
			goto content_failed;
		}
	}

	http_context_ref = &http_context_refs[http_id];
	if (http_context_ref->http_context == NULL){
		goto content_failed;
	}else{
		if (parsed_num == 1){
			if (http_context_ref->http_context->content != NULL) {
				str_len += sprintf(*rsp_cmd + str_len , "\r\n+HTTPCONTENT:%d,%d\r\n%s\r\n",
						http_context_ref->http_context->http_id, strlen(http_context_ref->http_context->content),
						http_context_ref->http_context->content);
			}
		}else{
			ret = xy_http_content(http_id, content, encode_method);
			if(ret == XY_ERR){
				goto content_failed;
			}
		}				
	}

	xy_free(content);
	sprintf(*rsp_cmd + str_len, "\r\nOK\r\n", http_id);
	return AT_END;

content_failed:
	xy_free(content);
	sprintf(*rsp_cmd, "\r\nERROR\r\n");
	return AT_END;
}

/** 
* @brief       HTTP发送请求
* @param 	   http_id：由at_http_create创建的http实例id
* @param   	   method:提供的请求类型如下：
                      typedef enum {
 	                          HTTP_METHOD_GET = 0,
 	                          HTTP_METHOD_POST,
 	                          HTTP_METHOD_PUT,
 	                          HTTP_METHOD_DELETE,
 	                          HTTP_METHOD_HEAD,
 	                          HTTP_METHOD_CUSTOM = 99
                           } http_method_e;
* @param        服务器端资源路径, ex. “/html/login/index.html”                           
* @output       成功响应则输出：
                        OK
* @output       失败响应则输出：
						ERROR
**/
int at_http_send(char *at_buf, char **rsp_cmd)
{
	int parsed_num;
	int http_id = -1;
	int method = -1;
	char *path= xy_zalloc(256);
	Http_Para_t *create_param = xy_malloc(sizeof(Http_Para_t));

	*rsp_cmd = xy_zalloc(100);

	if (at_parse_param_3("%d,%d,%s",at_buf, &parsed_num, &http_id, &method, path) != AT_OK) {
		goto send_failed;
	}

	if(parsed_num != 3){
		goto send_failed;
	}

	if(http_id < 0 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto send_failed;
	}
	if(method != HTTP_METHOD_GET && method != HTTP_METHOD_POST && method != HTTP_METHOD_PUT && method != HTTP_METHOD_DELETE && method != HTTP_METHOD_HEAD){
		goto send_failed;
	}

	create_param->http_id = http_id;
	create_param->flag = HTTP_REQUEST_REQ;
	create_param->method = method;
	create_param->path = path;

	if(http_create_msg_q != NULL){
		osMessageQueuePut(http_create_msg_q, (const void*)(&create_param), 0, osWaitForever);
	}else{
		goto send_failed;
	}
	
	sprintf(*rsp_cmd, "\r\nOK\r\n");
	return AT_END;

send_failed:
	xy_free(path);
	xy_free(create_param);
	sprintf(*rsp_cmd, "\r\nERROR\r\n");
	return AT_END;
}

/** 
* @brief       删除HTTP/HTTPS实例
* @param 	   http_id：由at_http_create创建的http实例id                       
* @output       成功响应则输出：
                        OK
* @output       失败响应则输出：
						ERROR
**/
int at_http_close(char *at_buf, char **rsp_cmd)
{
	int http_id = -1;
	int parsed_num = 0;
	http_context_reference_t *http_context_ref = NULL;
	*rsp_cmd = xy_zalloc(100);

	if (at_parse_param_3("%d",at_buf, &parsed_num, &http_id) != AT_OK) {
		goto close_failed;
	}

	if(parsed_num != 1){
		goto close_failed;
	}

	if(http_id < 0 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto close_failed;
	}

	http_context_ref = &http_context_refs[http_id];
	if (http_context_ref->http_context == NULL)
		goto close_failed;

	osThreadTerminate(http_context_ref->http_at_thread_id);
	http_context_ref->http_at_thread_id = 0;
	http_context_ref->quit = 1;

	while (http_context_ref->http_context->sock != -1 || http_context_ref->http_context->tls_context != NULL || http_context_ref->http_recv_thread_id != 0)
	{
		osDelay(100);
	}

	http_context_clear(http_context_ref->http_context);
	xy_free(http_context_ref->http_context);
	memset(http_context_ref, 0, sizeof(http_context_reference_t));

	osMessageQueueDelete(http_create_msg_q);
	http_create_msg_q = NULL;  

	sprintf(*rsp_cmd, "\r\nOK\r\n");
	return AT_END;

close_failed:
	sprintf(*rsp_cmd, "\r\nERROR\r\n");
	return AT_END;
}

void at_http_init()
{
	register_app_at_req("AT+HTTPCREATE",       at_http_create);
	register_app_at_req("AT+HTTPCFG",          at_http_cfg);
	register_app_at_req("AT+HTTPHEADER",       at_http_header);
	register_app_at_req("AT+HTTPCONTENT",      at_http_content);
	register_app_at_req("AT+HTTPSEND",         at_http_send);
	register_app_at_req("AT+HTTPCLOSE",        at_http_close);
}
