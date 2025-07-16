//#if (XY_HTTP) && (XY_FOTA)
#if 1
#include "xy_http_client.h"
#include "at_http.h"
#include "http_api.h"
#include "xy_at_api.h"
#include "xy_fota.h"
#include "at_http_fota.h"

#define HTTP_PROTO_PREFIX    "http://"
#define HTTPS_PROTO_PREFIX   "https://"

#define HTTP_CONTENT_LENGTH  "Content-Length: "
#define DOWNLOAD_UNIT_BYTES    1024       //HTTP包数据大小
     
osThreadId_t g_http_fota_handle = NULL;

//文件路径，如"/users/f"
char *g_xydelta_path = NULL; //"/resource/xyDelta"     

//HTTP服务器地址，如"http://139.224.131.190:3000"
char *g_server_host = NULL; //"http://fota.iinplus.com"                       

//下载差分文件的信息
download_info_struct g_download_info = {0};

//HTTPS服务器证书，用于认证
static char *g_sever_cert = NULL;

//服务器路径
static char *g_sever_url = NULL;

static void get_http_header(char *out_buf, int len)
{

    if (out_buf == NULL)
    {
        xy_printf("[XY_FOTA]get_http_header:out buffer should not null!");
        return;
    }

    memset(out_buf, 0x00, len);

    xy_printf("[XY_FOTA]download index: %d, total: %d.", g_download_info.download_unit_index, g_download_info.download_unit_num);
    sprintf((char *)out_buf, "Range: bytes=%d-%d\r\n", 
            g_download_info.download_unit_index * DOWNLOAD_UNIT_BYTES,
            (g_download_info.download_unit_index+1) * DOWNLOAD_UNIT_BYTES - 1);

}

static int http_header_parse(char *data)
{
    char *signptr = 0;
    int size_str_len = 0;
    char size_str[8];
    if (data == NULL)
    {
        xy_printf("[XY_FOTA]http_header_parse:out buffer should not null!");
        return 0;
    }

    signptr = strstr(data, "\r\n");
    size_str_len = signptr - data - strlen(HTTP_CONTENT_LENGTH);
    memset(size_str, 0x00,8);
    memcpy(size_str, data + strlen(HTTP_CONTENT_LENGTH), size_str_len);
    
    return atoi(size_str);
}

int get_xyDelta_size_by_http(xy_http_method_e protocol)
{
    int http_id = 0;
    char *recv_header_buf = NULL;
    unsigned int recv_header_len = 0;
    int delta_size = 0;
    
    /* host填写时必须带前缀http:// 或者https:// */
    /* 用户名 密码 选填*/
    http_id = xy_http_create(g_server_host, NULL, NULL);
    if(http_id == XY_ERR)
        goto error;
    
    if(protocol == BY_HTTPS)
    {
        /* HTTPS：服务器证书必填 */
        if(g_sever_cert == NULL ||xy_http_cfg(http_id, SEVER_CERT,g_sever_cert, HEX_CHARACTER))
            goto error;
    }

    /* 建立socket连接，每次请求前都应先调用此接口*/
    if(xy_http_connect(http_id))
        goto error;

    /* 发起request请求， method可选*/
    if(xy_http_request(http_id, HTTP_METHOD_HEAD, g_xydelta_path))
        goto error;

more_header:
    recv_header_buf = xy_zalloc(DOWNLOAD_UNIT_BYTES);
    /* 接受header，HTTP_FOREVER 表示永久等待，可选择其他时间（单位ms） */
    recv_header_len = xy_http_recv_header(http_id , recv_header_buf, DOWNLOAD_UNIT_BYTES, HTTP_FOREVER);
    if(recv_header_len > 0)
    {
        /* 此处处理接收到的header，查找差分文件的大小*/
        delta_size =  http_header_parse(strstr(recv_header_buf, HTTP_CONTENT_LENGTH));
    }
    xy_free(recv_header_buf);

    /* 查询是否还有header待接收*/
    if( header_recvlist_empty(http_id) != XY_TRUE )
    {
        goto more_header;
    }

error:
    if(http_id >= 0)
    {
        xy_http_close(http_id);
    }  
    return delta_size;
}


int get_fota_data_by_http(xy_http_method_e protocol, char *out_buf)
{
    int http_id = 0;
    char send_header[128];
    char *recv_buf = NULL;
    int recv_len = 0;
    int total_len = 0;
    /* host填写时必须带前缀http:// 或者https:// */
    /* 用户名 密码 选填*/
    http_id = xy_http_create(g_server_host, NULL, NULL);
    if(http_id == XY_ERR)
        goto error;
    
    if(protocol == BY_HTTPS)
    {
        /* HTTPS：服务器证书必填 */
        if(g_sever_cert == NULL || xy_http_cfg(http_id, SEVER_CERT, g_sever_cert, HEX_CHARACTER))
            goto error;
    }

    get_http_header(send_header, 128);
    if(xy_http_header(http_id, send_header, ESCAPE_MECHANISM))
        goto error;

    /* 建立socket连接，每次请求前都应先调用此接口*/
    if(xy_http_connect(http_id))
        goto error;

    /* 发起request请求， method可选*/
    if(xy_http_request(http_id, HTTP_METHOD_GET, g_xydelta_path))
		goto error;

more_body:
    recv_buf = xy_zalloc(DOWNLOAD_UNIT_BYTES);
    /* 接受body，HTTP_FOREVER 表示永久等待，可选择其他时间（单位ms） */
    recv_len = xy_http_recv_body(http_id , recv_buf, DOWNLOAD_UNIT_BYTES, HTTP_FOREVER);
    if(recv_len > 0 )
    {
        memcpy(out_buf + total_len, recv_buf, recv_len);
        total_len += recv_len;
    }

    xy_free(recv_buf); 

    /* 查询是否还有body待接收*/
    if( body_recvlist_empty(http_id) != XY_TRUE )
    {
    	goto more_body;
    }
 
error:
    if (http_id >= 0)
    {
        xy_http_close(http_id);
    }
    return total_len;
}

static int parse_http_uri(char *uri, int uri_len, char **parsed_host, char **parsed_path)
{
    char *host, *path;
    int  path_len, proto_len;
    int http_proto = BY_HTTP;
    
    if(!uri || *uri == '\0' || uri_len <= 0)
    {
        goto error;
    }

    if(0 == strncmp(uri, HTTP_PROTO_PREFIX, strlen(HTTP_PROTO_PREFIX))
        || 0 == strncmp(uri, HTTPS_PROTO_PREFIX, strlen(HTTPS_PROTO_PREFIX)))
    {
        if(0 == strncmp(uri, HTTPS_PROTO_PREFIX, strlen(HTTPS_PROTO_PREFIX)))
        {
            proto_len = strlen(HTTPS_PROTO_PREFIX);
        	http_proto = BY_HTTPS;
        }
        else
        {
            proto_len = strlen(HTTP_PROTO_PREFIX);
        }
            
        host = uri + proto_len;
        if(*host == '\0') // eg. just "http://"
            goto error;

        path = strchr(host, '/');
        if(NULL == path)
            goto error;
    }
    else
    {
    	xy_printf("unsupported proto");
        goto error;
    }

    path_len = uri_len - (path - uri);

    if(*parsed_host != NULL)
        xy_free(*parsed_host);
    *parsed_host = (char *)xy_malloc((path - uri) + 1);

    if(*parsed_path != NULL)
        xy_free(*parsed_path);
    *parsed_path = (char *)xy_malloc(path_len + 1);
    if(!(*parsed_host) || !(*parsed_path))
    {
    	xy_printf("xy_malloc failed");
        goto error;
    }
	memcpy(*parsed_host, uri, (path - uri));
    (*parsed_host)[path - uri] = '\0';
    
    memcpy(*parsed_path, path, path_len);
    (*parsed_path)[path_len] = '\0';

    return http_proto;
error:
	if(*parsed_host != NULL)
	{
		xy_free(*parsed_host);
		*parsed_host = NULL;
	}
	if(*parsed_path != NULL)
	{
		xy_free(*parsed_path);
		*parsed_path = NULL;
	}
	return -1;
}

void http_fota_proc(char *url)
{
    int recv_len = 0;
    int http_proto = BY_HTTP;
    unsigned int total_len = 0;
    char data_buf[DOWNLOAD_UNIT_BYTES];
    char *rsp_cmd = (char*)xy_malloc(40);

    if((http_proto = parse_http_uri(url, strlen(url), &g_server_host, &g_xydelta_path)) < 0)
	{
		goto error;
	}
	
	xy_printf("[XY_FOTA]g_server_host:%s, g_xydelta_path:%s", g_server_host, g_xydelta_path);

    //获取差分包信息
    g_download_info.download_delta_size = get_xyDelta_size_by_http(http_proto);
    if(g_download_info.download_delta_size > 0)
    {
        if(g_download_info.download_delta_size % DOWNLOAD_UNIT_BYTES)
            g_download_info.download_unit_num = (g_download_info.download_delta_size/DOWNLOAD_UNIT_BYTES) + 1;
        else
            g_download_info.download_unit_num = (g_download_info.download_delta_size/DOWNLOAD_UNIT_BYTES);
    }
    else
    {
    	goto error;
    }

    //获取差分包
    for(; g_download_info.download_unit_index < g_download_info.download_unit_num; g_download_info.download_unit_index++)
    {
        recv_len = get_fota_data_by_http(http_proto, data_buf);
        if(recv_len)
        {
            if(ota_write_to_flash(total_len, data_buf, recv_len))
                goto error;
            
            total_len += recv_len;
            xy_printf("[XY_FOTA]get delta size %d %d", recv_len, total_len);
        }
        else
        {
            xy_printf("[XY_FOTA]get delta size fail %d", recv_len);
            goto error;
        }
            
    }

    if(total_len != g_download_info.download_delta_size)
    {
        xy_printf("[XY_FOTA]get delta size error ");
        goto error;
    }

    snprintf(rsp_cmd, 40, "\r\n+HTTPFOTA:DOWNLOAD SUCCESS\r\n");
	send_urc_to_ext(rsp_cmd);
	xy_free(rsp_cmd);
    memset(&g_download_info, 0x00, sizeof(g_download_info));
	return;
error:
	snprintf(rsp_cmd, 40, "\r\n+HTTPFOTA:DOWNLOAD FAILED\r\n");
	send_urc_to_ext(rsp_cmd);
	xy_free(rsp_cmd);
    memset(&g_download_info, 0x00, sizeof(g_download_info));
    return;
}

void http_fota_task(void* argument)
{
    http_fota_proc((char *)argument);

	g_http_fota_handle = NULL;
	osThreadExit();
}

void http_ota_task_init(void)
{
	osThreadAttr_t thd_attr = {0};

	if (g_http_fota_handle != NULL)
        return;
	
	thd_attr.name = "http_ota_task";
	thd_attr.stack_size = 0x3000;
	thd_attr.priority = osPriorityNormal1;

	g_http_fota_handle = osThreadNew((osThreadFunc_t)(http_fota_task), g_sever_url, &thd_attr);
}

/*****************************************************************************
 Function    : at_http_fota
 Description :   
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+HTTPFOTA=<cmd>[,<data>]
               AT+HTTPFOTA=?
               AT+HTTPFOTA?
 *****************************************************************************/
int at_http_fota(char *at_buf, char **prsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int cmd = -1;
        char *data = xy_zalloc(strlen(at_buf));

        if(at_parse_param("%d(0-4),%s", at_buf, &cmd, data) != AT_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto exit;
        }

        switch(cmd)
        {
	        case 0://设置服务器证书HEX
	        {
	            if(strlen(data) < 0)
	            {
	                *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	                break;
	            }
	            if(g_sever_cert != NULL)
	            {
	                xy_free(g_sever_cert);
	                g_sever_cert = NULL;
	            }
	            g_sever_cert = xy_zalloc(strlen(data) + 1);
	            strcpy(g_sever_cert, data);
	            break;
	        }
	        case 1://设置服务器的URL
	        {
	            if(strlen(data) < 0)
	            {
	                *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	                break;
	            }
	            if(g_sever_url != NULL)
	            {
	                xy_free(g_sever_url);
	                g_sever_url = NULL;
	            }
	            g_sever_url = xy_zalloc(strlen(data) + 1);
	            strcpy(g_sever_url, data);
	            break;
	        }
	        case 2://下载
	        {
	    		if (!ps_netif_is_ok()) {
	            	*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
	            	break;
	            }
	            if(g_sever_url != NULL)
	            {
#if XY_FOTA
	                http_ota_task_init();
#endif
	            }
	            else
	              *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	            break;
	        }
	        case 3://校验升级
	        {
	            if(ota_delta_check())
	            {
	            	*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	            	break;
	            }

	            xy_printf("[XY_FOTA]update start!");
	            ota_update_start();
	            break;
	        }
	        case 4://查询升级结果
	        {
	            *prsp_cmd = xy_zalloc(40);
	            if(ota_get_update_result() == XY_OK)
	            {
	                snprintf(*prsp_cmd, 40, "\r\n+HTTPFOTA:UPDATE SUCCESS\r\n\r\nOK\r\n");
	            }
	            else if(ota_get_update_result() == XY_ERR)
	            {
	                snprintf(*prsp_cmd, 40, "\r\n+HTTPFOTA:UPDATE FAILED\r\n\r\nOK\r\n");
	            }
				else
				{
	           		 snprintf(*prsp_cmd, 40, "\r\n+HTTPFOTA:NO UPDATE RESULT\r\n\r\nOK\r\n");
				}
	            break;
	        }
	        default:
            	*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        }
	exit:
        xy_free(data);
    }
    else if(g_req_type == AT_CMD_QUERY)//查询证书和URL
    {
        if(g_sever_cert != NULL && g_sever_url != NULL)
        {
            *prsp_cmd = xy_zalloc(40 + strlen(g_sever_cert) + strlen(g_sever_url));
            snprintf(*prsp_cmd, 40 + strlen(g_sever_cert) + strlen(g_sever_url), "\r\n+HTTPFOTA:%s,%s\r\n\r\nOK\r\n", g_sever_url, g_sever_cert);
        }
        else if(g_sever_cert != NULL)
        {
            *prsp_cmd = xy_zalloc(40 + strlen(g_sever_cert));
            snprintf(*prsp_cmd, 40 + strlen(g_sever_cert), "\r\n+HTTPFOTA:,%s\r\n\r\nOK\r\n", g_sever_cert);
        }
        else if(g_sever_url != NULL)
        {
            *prsp_cmd = xy_zalloc(40 + strlen(g_sever_url));
            snprintf(*prsp_cmd, 40 + strlen(g_sever_url), "\r\n+HTTPFOTA:%s,\r\n\r\nOK\r\n", g_sever_url);
        }
        else
        {
            *prsp_cmd = xy_zalloc(40);
            snprintf(*prsp_cmd, 40, "\r\n+HTTPFOTA:,\r\n\r\nOK\r\n");
        }
    }
    else if(g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(40);
        snprintf(*prsp_cmd, 40, "\r\n+HTTPFOTA:(0-4)\r\n\r\nOK\r\n");
    }
    else
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);

    return AT_END;
}
#endif
