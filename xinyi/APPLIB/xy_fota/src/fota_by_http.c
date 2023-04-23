#if  HTTP

#if XY_FOTA
#include "xy_fota.h"
#endif
#include "fota_by_http.h"

//#include <stdio.h> 
//#include <stdlib.h>
//#include <time.h> 
//#include <errno.h>  
//#include <string.h> 
#include "lwip/sockets.h"
//#include <stdarg.h>
#include "lwip/netdb.h"
#include "xy_net_api.h"

#define STRBUFSIZE 512
#define STRBUF1024SIZE 1024

uint32_t g_range_offset = 0;
extern char *g_http_host;
extern char *g_http_path;
extern short unsigned int g_http_port;

int httpurl_parse(char *url, char **host, short unsigned int *port, char **path)
{
    char *host_ptr = NULL;
    char *port_ptr = NULL;
    char *path_ptr = NULL;
	char *tail_path_ptr = NULL;
	int host_len = 0;
	int path_len = 0;

    if (url == NULL)
        return XY_ERR;
	
	if(strchr(url, '"') == NULL)
	    return XY_ERR;
	
	if(strstr(url, "http") == NULL && strstr(url, "https") == NULL)
		return XY_ERR;
	
	#if 0
	/* 后面如果需要使用https，需要增加http与https的判断*/
	if (!strncmp(url, "http", strlen("http"))) {
		*port = 80;	
	} 
	else if (!strncmp(url, "http", strlen("http"))) {
		*port = 443;
	}
	else {
		return XY_ERR; /* URL is invalid */
	}
	#endif
	
    host_ptr = (char *) strstr(url, "://");
    if (host_ptr == NULL)
        return XY_ERR; /* URL is invalid */

    host_ptr += 3;
    //printf("host ptr %s\r\n", host_ptr);
    port_ptr = strchr(host_ptr, ':');
    if ( port_ptr != NULL) 
	{
        host_len = port_ptr - host_ptr;
		//printf("host len %d\r\nhost_ptr %s\r\n", host_len, host_ptr);
		*host = xy_zalloc(host_len + 1);
	    memcpy(*host, host_ptr, host_len);
		(*host)[host_len] = '\0';
		//printf("out host %s\r\n", *host);
        port_ptr++;
	    if (sscanf(port_ptr, "%hu", port) != 1)
            goto err;
		//printf("out port %d\r\n", *port);
    }
	else
		return XY_ERR;
	
    path_ptr = strchr(host_ptr, '/');
	tail_path_ptr = strrchr(host_ptr, '"');
	//printf("path ptr %s\r\n", path_ptr);
    if(path_ptr != NULL && tail_path_ptr != NULL) 
        path_len = tail_path_ptr - path_ptr;
	else
		goto err;

    //printf("path len %d\r\n", path_len);
	*path = malloc(path_len + 1);
	memcpy(*path, path_ptr, path_len + 1);
	(*path)[path_len] = '\0';
	//printf("out path %s\r\n", *path);
	
    return XY_OK;

err:
	if(*host != NULL)
		free(*host);
	*host = NULL;

	return XY_ERR;
}


static int http_get(char *hostname, unsigned short hostport, char *query)
{
	int sockfd, ret, i, h;
	int recv_cnt = 0;

	struct addrinfo *res;
	struct sockaddr_in *sinp;
	const char *addr;
	char abuf[INET_ADDRSTRLEN];
	struct sockaddr_in serv_addr;

	char str1[STRBUFSIZE], str2[STRBUFSIZE], buf[STRBUFSIZE], *pstr = NULL;
	fd_set t_set1;
	struct timeval tv;
	int wtstart = 0, contentlen = 0;
	int rt = 0;
	int total_len = 0;
	int record_contentlen = 0;
	char *urc = NULL;

    /* 等待驻网成功*/
    if(xy_wait_tcpip_ok(10) != XY_OK)
    {
    	xy_printf("[HTTP FOTA]wait tcp/ip timeout(10s)");
		goto error_exit;
	}

	//printf("\n%s download from http://%s:%d%s\n",  __FUNCTION__, hostname, hostport, query);
	xy_printf("[HTTP FOTA]%s download from http://%s:%d%s",  __FUNCTION__, hostname, hostport, query);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		//printf("%d:%s - socket failed\n", __LINE__, __FUNCTION__);
		xy_printf("[HTTP FOTA]%d:%s - socket failed", __LINE__, __FUNCTION__);
		return 0;
	}

	if (getaddrinfo(hostname, NULL, NULL, &res) != 0)
	{
		//printf("%d:%s - getaddrinfo %s  failed\n", __LINE__, __FUNCTION__, hostname);
		xy_printf("[HTTP FOTA]%d:%s - getaddrinfo %s  failed", __LINE__, __FUNCTION__, hostname);
		goto error_exit;
	}

    /* 填充serv addr:<family> <addr> <port> */
	sinp = (struct sockaddr_in *)res->ai_addr;//sockaddr转换成sockaddr_in
	addr = (const char *)inet_ntop(AF_INET, &sinp->sin_addr, abuf, INET_ADDRSTRLEN);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_aton(addr, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(hostport);

	unsigned long ul = 1;
	ioctl(sockfd, FIONBIO, (unsigned long*)&ul);
	
	connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	/*
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
		// printf("连接到服务器失败,connect error %d!\n", errno); 
		logfile("%d:%s - connect %s 1 failed, %d\n", __LINE__, __FUNCTION__, hostname, errno);
		goto error_exit;
	}
	*/

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	FD_ZERO(&t_set1);
	FD_SET(sockfd, &t_set1);
	h = select(sockfd + 1, NULL, &t_set1, NULL, &tv);
	if (h <= 0) {
		//printf("%d:%s - connect %s failed\n", __LINE__, __FUNCTION__, hostname);
		xy_printf("[HTTP FOTA]%d:%s - connect %s failed", __LINE__, __FUNCTION__, hostname);
		goto error_exit;
	}

	ul = 0;
	ioctl(sockfd, FIONBIO, (unsigned long*)&ul);

	//printf("%s - connect %s:%d success\n",__FUNCTION__, hostname, hostport);
	//xy_printf("[HTTP FOTA]%s - connect %s:%d success",__FUNCTION__, hostname, hostport);
	
	//发送数据，组织必要header信息
	memset(str1, 0x0, STRBUFSIZE);
	//strcat(str1, "GET /check_fw?curver=1.0.0 HTTP/1.1\n"); 
	sprintf(str2, "GET %s HTTP/1.1\n", query);
	strcat(str1, str2);
	sprintf(str2, "Host: %s\n", hostname);
	strcat(str1, str2); 
	sprintf(str2, "Range: bytes=%d-\n", g_range_offset);
	strcat(str1, str2);
	//strcat(str1, "Range: bytes=0-\n");
	strcat(str1, "Content-Type: application/x-www-form-urlencoded\n");
	strcat(str1, "\n\n");
	
	//printf("%s\n", str1);
	xy_printf("[HTTP FOTA]header str %s\n", str1);

	ret = send(sockfd, str1, strlen(str1),0);
	if (ret < 0) 
	{
		//printf("%d:%s - send data to %s failed\n", __LINE__, __FUNCTION__, hostname);
		xy_printf("%d:%s - send data to %s failed", __LINE__, __FUNCTION__, hostname);
		goto error_exit;
	}

	FD_ZERO(&t_set1);
	FD_SET(sockfd, &t_set1);

    //不需要文件操作，最多在send成功后初始化flash的ota区域

	//tm = time(NULL);
	while (1) {
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		h = 0;
		FD_ZERO(&t_set1);
		FD_SET(sockfd, &t_set1);

		h = select(sockfd + 1, &t_set1, NULL, NULL, &tv);

		if (h <= 0)
		{
			//printf("\n%d:%s - recv data from %s failed\n", __LINE__, __FUNCTION__, hostname);
			xy_printf("[HTTP FOTA]%d:%s - recv data from %s failed", __LINE__, __FUNCTION__, hostname);
			goto error_exit;
		};

		if (h > 0) 
		{
			i = 0;
			memset(buf, 0, STRBUFSIZE);
			i = recv(sockfd, buf, STRBUFSIZE,0);
			//printf("\r\nrecv cnt %d len %d buf len %d\r\n", recv_cnt++, i, strlen(buf));
			xy_printf("[HTTP FOTA]recv cnt %d len %d", recv_cnt++, i);
			
			if (i == 0)
			{
				//printf("\n%d:%s - recv data from %s  failed\n", __LINE__, __FUNCTION__, hostname);
				xy_printf("[HTTP FOTA]%d:%s - recv data from %s  failed", __LINE__, __FUNCTION__, hostname);
				goto error_exit;
			}

			if (rt == 0)
			{
				int rlength = 2;
				//xy_printf("[HTTP FOTA]%d:%s --rt:%d- socket read i:%d \n",__LINE__,__FUNCTION__,rt,i);
				if (contentlen == 0 && wtstart == 0)
				{

					pstr = strstr(buf, "Content-Length: ");
					if (pstr != NULL) {
						contentlen = atoi(pstr + strlen("Content-Length: "));
						record_contentlen = contentlen;
					}
					else {
						//printf("\n%d:%s - recv data from %s 4 failed\n", __LINE__, __FUNCTION__, hostname);
						xy_printf("[HTTP FOTA]%d:%s - recv data from %s 4 failed", __LINE__, __FUNCTION__, hostname);
						goto error_exit;
					}
					//printf("\n%d:%s --Content-Length::%d \n", __LINE__, __FUNCTION__, contentlen);
					send_urc_to_ext("\r\n+QIND: \"FOTA\",\"HTTPSTART\"\r\n");
				}
				pstr = strstr(buf, "\n\n");
				if (!pstr) {
					/* \r\n\r\n header与content以空行分割 */
					pstr = strstr(buf, "\r\n\r\n");
					rlength = 4;
				}
				
				if (pstr)
				{
				    //printf("\r\n1111total len %d, i %d, rlength %d pstr-buf %d\r\n", total_len, i, rlength, pstr - buf);
				    xy_printf("[HTTP FOTA]total len %d, i %d, rlength %d pstr-buf %d", total_len, i, rlength, pstr - buf);
					
					if(ota_write_to_flash(total_len + g_range_offset, pstr + rlength, i - (pstr - buf) - rlength) != XY_OK)
					{
						xy_printf("[HTTP FOTA]begin flash write error");
						goto error_exit;
					}
					
				    total_len += i - (pstr - buf) - rlength;
					//第一次读取除了header，还有部分content内容，需要减去该content长度
					contentlen -= total_len;
					
					//printf("\r\n\12345 total len %d contentlen %d\r\n", total_len, contentlen);
					xy_printf("[HTTP FOTA]total len %d contentlen %d", total_len, contentlen);
					wtstart = 1;
					rt = -1;
				}
				//printf("%d:%s --rt:%d- wtstart:%d\n",__LINE__,__FUNCTION__,rt, wtstart);
				//xy_printf("[HTTP FOTA]%d:%s --rt:%d- wtstart:%d\n",__LINE__,__FUNCTION__,rt, wtstart);
			}
			
			else if(wtstart == 1)
			{
				if (i > contentlen)
				{
					i = contentlen;
					contentlen = 0;
				}

				if(ota_write_to_flash(total_len + g_range_offset, buf, i) != XY_OK)
				{
					xy_printf("[HTTP FOTA]flash write error");
					goto error_exit;
				}
				
				if(total_len < record_contentlen)
				    total_len += i;
				
				//printf("\r\ntotal len %d content len %d\r\n", total_len, contentlen);
				xy_printf("[HTTP FOTA]total len %d content len %d", total_len, contentlen);

				//downing process +QIND: "FOTA","DOWNLOADING", 1% 
                urc = xy_zalloc(64);
				snprintf(urc, 64, "\r\n+QIND: \"FOTA\",\"DOWNLOADING\", %.0f%%\r\n", (float)total_len/record_contentlen * 100);
				send_urc_to_ext(urc);
				xy_free(urc);
				
				if (contentlen > 0)
					contentlen -= i;
			}


			if ((wtstart == 1) && (contentlen == 0))
			{
				//printf("%d:%s --rt:%d- total len %d\n",__LINE__,__FUNCTION__,rt, total_len);
				break;
			}

		}
	}
	
	close(sockfd);
	
	//如果已经写入总长度等于content length
	if(record_contentlen == total_len)
	{
		//printf("\r\nwrite flash all\r\n");
		//+QIND: "FOTA","DOWNLOADING", 100% 
		send_urc_to_ext("\r\n+QIND: \"FOTA\",\"DOWNLOAD SUCCESS\"\r\n");
		if(ota_delta_check() != XY_OK)
		{
			xy_printf("[HTTP FOTA]delta check fail");
			goto error_exit;
		}
		g_range_offset = 0;
		ota_update_start();
	}

	//printf("Download ok ...\n");
	xy_printf("[HTTP FOTA]download ok");

	return 1;

error_exit:
	
	close(sockfd);
	if(g_http_host != NULL){
		xy_free(g_http_host);
		g_http_host = NULL;
	}
	if(g_http_path != NULL){
		xy_free(g_http_path);
		g_http_path = NULL;
	}
	
    //下载进度总是向后推进的,应该保存到NV
	g_range_offset += total_len;
	send_urc_to_ext("\r\n+QIND: \"FOTA\",\"FAIL\"\r\n");
	
	return 0;
}

void fota_by_http_task()
{
	//AT+QFOTADL="http://iotsvr.meigsmart.com:9090/download/xyDelta"
	//AT+QFOTADL="http://iotsvr.meigsmart.com:9090/download/xyDelta_downgrade"
	//printf("\r\nhost %s\r\nport %d\r\npath %s\r\n\r\n", g_http_host, g_http_port, g_http_path);
	int rtn;
	rtn = http_get(g_http_host, g_http_port, g_http_path);
	
	osThreadExit();
}

void fota_by_http()
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "fota_https_task_demo";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 0x3000;
    osThreadNew((osThreadFunc_t)(fota_by_http_task),NULL,&thread_attr);
}

#endif

