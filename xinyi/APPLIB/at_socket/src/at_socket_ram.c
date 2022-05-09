/**
 * 该文件中代码放在RAM中，可以提升处理速度
 */
#include "softap_macro.h"
#if	AT_SOCKET

#include "xy_at_api.h"
#include "at_global_def.h"
#include "xy_utils.h"
#include "at_socket_context.h"
#include "at_socket.h"
#include "ps_netif_api.h"


//AT+XDSEND=<socket_id>,<remote_ip>,<remote_port>,<size>,<data>
int at_XDSEND_req(char *at_buf)
{
	int sock_id = 0;
	int len = 0;
	int idx;
	int rai_val = 0;  //iniate the value of xy_rai
	char *trans_data = NULL;
	char *prsp_cmd = NULL;
	char *pchr = NULL;
	char *pcur = at_buf;	
	struct sockaddr_in remote_sockaddr = {0};
	char remote_ip[16] = {0};
	int remote_port = 0;
	
	if (!ps_netif_is_ok()) 
	{
		prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		goto ERR_PROC;
	}
	if((pchr = strchr(pcur,',')) == NULL){
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	*pchr = '\0';
	sock_id = (int)strtol(pcur,NULL,10);
	if ((idx = find_sock_ctx_id_by_sock_id(sock_id)) == -1) {
		prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}
	
	pcur = pchr+1;
	if((pchr = strchr(pcur,',')) == NULL){
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	*pchr = '\0';
	if(strlen(pcur) > 15)
	{
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	memcpy(remote_ip,pcur,strlen(pcur));

	pcur = pchr+1;
	if((pchr = strchr(pcur,',')) == NULL){
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	*pchr = '\0';
	remote_port = (int)strtol(pcur,NULL,10);
	if(remote_port < 0 || remote_port > 65535)
	{
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}	
	
	pcur = pchr+1;
	if((pchr = strchr(pcur,',')) == NULL){
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	*pchr = '\0';
	len = (int)strtol(pcur,NULL,10);
	if (len<1 || len>NET_MAX_USER_DATA_LEN) {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	pcur = pchr+1;
	if((pchr = strchr(pcur,'\r')) == NULL){
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	if(len*2 != (pchr-pcur))
	{
		//xy_assert(0);
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	*pchr = '\0';
	trans_data = xy_zalloc(len+1);
	if (g_ipdata_mode == HEX_ASCII_STRING) //hex string
	{
		if (hexstr2bytes(pcur, len * 2, trans_data, len) == -1) {
			prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto  ERR_PROC;
		}
	}
	else
	{
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	remote_sockaddr.sin_family = AF_INET;
	
	remote_sockaddr.sin_port = htons(remote_port);
   if(1 != inet_aton(remote_ip, &remote_sockaddr.sin_addr))
   {
	   *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	   goto ERR_PROC;
   }

   if(sendto2(sock_ctx[idx]->fd, trans_data, len, 0, (struct sockaddr*)&remote_sockaddr, sizeof(struct sockaddr_in),0,rai_val) <= 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "send_errno:%d", errno);
		xy_assert(0);
		prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		goto  ERR_PROC;
	}
	sock_ctx[idx]->sended_size += len;
	prsp_cmd = at_ok_build();
ERR_PROC:
	send_debug_str_to_at_uart(prsp_cmd);
	softap_printf(USER_LOG, WARN_LOG, "uart_send:%s\r\n",prsp_cmd);
	if (trans_data != NULL)
		xy_free(trans_data);
	if(prsp_cmd != NULL)
		xy_free(prsp_cmd);
	return AT_END;
}
#endif




