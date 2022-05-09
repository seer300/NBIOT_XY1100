#include "xy_at_api.h"
#include "at_ps_cmd.h"
#include "at_global_def.h"
#include "softap_nv.h"
#include "factory_nv.h"
#include "oss_nv.h"
#include "low_power.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "lwip/netdb.h"
#if XY_DM && WAKAAMA
#include "xiny_er-coap-13.h"
#endif
static unsigned char *modul_ver = NULL;
static char *requestbuf = NULL;
static unsigned int requestLen = 0;
osThreadId_t g_telecom_dm_TskHandle =NULL;
static char telecom_timer_flag = 0;

#define DM_HOST 		"zzhc.vnet.cn"
#define DM_HOST_PATH 	"nb"
#define DM_PORT 		5683
#define DM_DEFAULT_SERVER  0x9614632A				//电信DM默认服务器42.99.20.150

#if XY_DM && WAKAAMA

static void FormJson(unsigned char *buf,unsigned char regver,unsigned char* immeid,unsigned char* modul_ver,unsigned char* versionExt,unsigned char* sim1iccid,unsigned char* sim1lteimsi)
{	
	int n;	
	n = sprintf((char *)buf,"{\"REGVER\":\"%d\",",regver);		
	n += sprintf((char *)buf+n,"\"MEID\":\"%s\",",immeid);		
	n += sprintf((char *)buf+n,"\"MODEL\":\"%s\",",modul_ver);	
	n += sprintf((char *)buf+n,"\"SWVER\":\"%s\",",versionExt);			
	n += sprintf((char *)buf+n,"\"SIM1ICCID\":\"%s\",",sim1iccid);			
	n += sprintf((char *)buf+n,"\"SIM1LTEIMSI\":\"%s\"}",sim1lteimsi);		
}

static int recv_process(int sockfd)
{
	int result = -1;
	struct timeval tv = {120,0};
	fd_set readfds;
	uint8_t coap_error_code = xiny_NO_ERROR;
	xiny_coap_packet_t message[1];

	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	
	result = select(sockfd + 1, &readfds, NULL, NULL, &tv);
	softap_printf(USER_LOG, WARN_LOG, "[DM]select ret(%d): %d %s\n", result, errno, strerror(errno));
	if (result < 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "[DM]Error select(): %d %s\n", errno, strerror(errno));
        return -1;
	}
	else if (result > 0)
	{
		uint8_t buffer[512];
		int numBytes;

		/*
		* If an event happens on the socket
		*/
		if (FD_ISSET(sockfd, &readfds))
		{
			struct sockaddr_storage addr;
			socklen_t addrLen;

			addrLen = sizeof(addr);

			/*
			* We retrieve the data received
			*/
			numBytes = recvfrom(sockfd, (char*)buffer, 512, 0, (struct sockaddr *)&addr, &addrLen);

			if (numBytes < 0)
			{
				softap_printf(USER_LOG, WARN_LOG, "Error in recvfrom(): %d %s\n", errno, strerror(errno));
			}
			else if (numBytes > 0)
			{
				coap_error_code = xiny_coap_parse_message(message, buffer, numBytes);
				/*
				char *content = (char*)xy_zalloc(message->payload_len + 1); 
				memcpy(content, message->payload, message->payload_len);
				xy_printf("[DM]%s", content);
				*/
				if (coap_error_code == xiny_NO_ERROR)
				{
					switch (message->type)
		            {
		            case xiny_COAP_TYPE_ACK:
		            {
						if(strstr(message->payload, "Success") || strstr(message->payload, "\"0\""))
							result = 0;
						else
							result = -1;
		                break;
		            }

		            default:
						result = -1;
		                break;
		            }
					xiny_coap_free_header(message);
				}
			}
		}

	}
	else
	{
		softap_printf(USER_LOG, WARN_LOG, "[DM]select() timeout\n");
		return -1;
	}

	return result;
}

static int dm_socket_client(void)
{
	struct sockaddr_in server = {0};		
	int fd = 0;
	int rc;
	struct addrinfo *result, hint = {0};
	char ip_buff[16] = {0};

	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_DGRAM;
	
	softap_printf(USER_LOG, WARN_LOG, "[DM]begin to get telecom server addr and telecom domain: %s\n",DM_HOST);
	
	rc = getaddrinfo(DM_HOST, NULL, &hint, &result);
	if (rc != 0) {
		softap_printf(USER_LOG, WARN_LOG, "dns retry1! rc=%d, errno=%d\n", rc, errno);
		server.sin_addr.s_addr=DM_DEFAULT_SERVER; 	//电信DM默认服务器42.99.20.150
	}
	else {
		memcpy(&server.sin_addr, &(((struct sockaddr_in *)(result->ai_addr))->sin_addr), sizeof(struct in_addr));
		freeaddrinfo(result);
	}
	softap_printf(USER_LOG, WARN_LOG, "[DM]get telecom server getaddrinfo success: %s\n", inet_ntop(AF_INET,&server.sin_addr.s_addr,ip_buff,16));
	
	server.sin_family = AF_INET;
	server.sin_port = htons(DM_PORT);

	softap_printf(USER_LOG, WARN_LOG, "[DM]begin create socket\n");
	fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if ( fd < 0 ) {
		softap_printf(USER_LOG, WARN_LOG, "[DM]error,fd=%d, errno=%d\n",fd,errno);		
		return -1;
	}
	else
	{
		rc = sendto(fd, requestbuf, requestLen, 0, (struct sockaddr *)&server, sizeof(struct sockaddr));
		if(rc < 0)
		{
			close(fd);
			return -1;
		}
		else
		{
			rc = recv_process(fd);
		}
	}	
	close(fd);
	return (rc);

}

static void dm_make_packet(char* nccid_dm)
{
	int i=0;
	char *base64;	 
	int n;
	requestLen = 0;
	xiny_coap_packet_t* pkt = NULL;
	char *temp_buff = xy_zalloc(512);
	base64 = xy_zalloc(384);
	char *imei_temp = xy_zalloc(16);
	char *imsi_temp = xy_zalloc(16);
	
	xy_get_IMEI(imei_temp,16);
	xy_get_IMSI(imsi_temp,16);

	FormJson((unsigned char *)temp_buff,g_softap_fac_nv->regver, imei_temp, modul_ver,
		g_softap_fac_nv->versionExt, nccid_dm, imsi_temp); 
	dm_print_str(temp_buff);
	base64_encode(temp_buff,base64,strlen(temp_buff));		

	for(;i<strlen(base64);i++)
	{
		temp_buff[n+i]= base64[i];
	}

	//申请packet空间
	pkt = (xiny_coap_packet_t *)xy_zalloc(sizeof(xiny_coap_packet_t));

	//配置请求信息：CON，POST，MID，TOKEN
	uint16_t coap_msgid = (uint16_t)xy_rand();
	xiny_coap_init_message(pkt, xiny_COAP_TYPE_CON, xiny_COAP_POST, coap_msgid);
	
	uint8_t temp_token[6];
	time_t tv_sec = xy_rtc_get_sec(0);

	// xiny_initialize first 6 bytes, leave the last 2 random
	temp_token[0] = coap_msgid;
	temp_token[1] = coap_msgid >> 8;
	temp_token[2] = tv_sec;
	temp_token[3] = tv_sec >> 8;
	temp_token[4] = tv_sec >> 16;
	temp_token[5] = tv_sec >> 24;
	xiny_coap_set_header_token(pkt, temp_token, 6);

	//参照电信白皮书配置规范制定option和base64加密后的payload
	xiny_coap_set_header_uri_host(pkt, DM_HOST);
	xiny_coap_set_header_uri_path(pkt, DM_HOST_PATH);
	xiny_coap_set_header_content_type(pkt, xiny_APPLICATION_JSON);	
	xiny_coap_set_payload(pkt, temp_buff, strlen(temp_buff));

	requestLen = xiny_coap_serialize_get_size(pkt);
	if (requestLen == 0)
	{
		xiny_free_multi_option(pkt->uri_path);
		xy_free(pkt);
		goto end;
	}
	
	requestbuf = (uint8_t *)xy_zalloc(requestLen);
	if (requestbuf == NULL)
	{
		xiny_free_multi_option(pkt->uri_path);
		xy_free(pkt);
		goto end;
	}
	
	requestLen = xiny_coap_serialize_message(pkt, requestbuf);

end:
	xy_free(imei_temp);
	xy_free(imsi_temp);
	xy_free(base64);
	if(pkt != NULL)
	{
		xiny_coap_free_header(pkt);
		xy_free(pkt);
	}
}

static void dm_timeout_proc(void *para)
{
	(void) para;
	telecom_timer_flag = 1;
	softap_printf(USER_LOG, WARN_LOG, "[DM]start dm again!!!\n");
	//RTC唤醒后，协议栈可能还处于PSM态，不加锁，业务代码若有delay，则会进入idle，进而可能进入深睡
	xy_work_lock(LOCK_XY_LOCAL);
	telecom_dm_init();
}


char *telecom_need_start_dm()
{
	char* nccid_dm_temp = NULL;
	
	nccid_dm_temp = xy_zalloc(UCCID_LEN);
	xy_get_NCCID(nccid_dm_temp,UCCID_LEN);
	
	if(nccid_dm_temp != NULL)
		softap_printf(USER_LOG, WARN_LOG, "[DM]dm iccid:[%s],nv store iccid:[%s]\n",nccid_dm_temp,g_softap_var_nv->dm_cfg.ue_iccid);
	else
		softap_printf(USER_LOG, WARN_LOG, "[DM]dm iccid:[NULL],nv store iccid:[%s]\n",g_softap_var_nv->dm_cfg.ue_iccid);
	
	if(nccid_dm_temp == NULL || strncmp((char *)nccid_dm_temp,(char *)(g_softap_var_nv->dm_cfg.ue_iccid),20) == 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "[DM]do not need start dm\n");		
		if(nccid_dm_temp == NULL)
			softap_printf(USER_LOG, WARN_LOG, "g_ueiccid is NULL,do not dm\n");
		if(nccid_dm_temp != NULL)
			xy_free(nccid_dm_temp);
		return NULL;
	}
	else
	{			
		softap_printf(USER_LOG, WARN_LOG, "[DM]need start dm\n");
		softap_printf(USER_LOG, WARN_LOG, "xy_iccid:%s\r\n",(char *)g_softap_var_nv->dm_cfg.ue_iccid);
	
		return nccid_dm_temp;
	}
}


void telecom_dm_run()
{
#if !IS_DSP_CORE
	 if(get_sys_up_stat()==POWER_ON && CHECK_SDK_TYPE(OPERATION_VER))
	 	xy_work_lock(0); //operator version
#endif
		
	char *nccid_dm = telecom_need_start_dm();
	char *imei_temp = xy_zalloc(16);
	char *imsi_temp = xy_zalloc(16);
	
	xy_get_IMEI(imei_temp,16);
	xy_get_IMSI(imsi_temp,16);

	if(nccid_dm == NULL)
	{
		goto out;
	}

	
	if(modul_ver == NULL)
		modul_ver = xy_zalloc(21);

	if(g_softap_var_nv->dm_cfg.have_retry_num==0)
		goto FIRST_POWERON;
	while(1)
	{
		if (!ps_netif_is_ok()) 
			goto out;
FIRST_POWERON:		
		{
			char i,flag = 0;
			memset(modul_ver,0,21); 
			memcpy(modul_ver, g_softap_fac_nv->modul_ver, strlen((const char *)g_softap_fac_nv->modul_ver));
			for(i = 0; i < 20; i++){
				if(modul_ver[i] == '-' && flag == 0){
					flag = 1;
					continue;
				}
				if(modul_ver[i] == '-')
					modul_ver[i] = ' ';
			}
		}

        if(strlen((const char *)(imei_temp)) == 0)
        {
			softap_printf(USER_LOG, WARN_LOG, "[DM]meid is NULL\n");
			goto out;

        }
		softap_printf(USER_LOG, WARN_LOG, "[DM]meid:[%s]\n",imei_temp);

		if(strlen((const char *)(imsi_temp)) == 0 || (imsi_temp[0] == '0' && imsi_temp[1] == '0' && (imsi_temp[2] == '1' || imsi_temp[2] == '2')))
		{
			softap_printf(USER_LOG, WARN_LOG, "[DM]imsi is invalid\n");
			goto out;

		}
		softap_printf(USER_LOG, WARN_LOG, "[DM]dm imsi:[%s]\n",imsi_temp);

		softap_printf(USER_LOG, WARN_LOG, "[DM]begin to get dns server addr\n");
		ip4_addr_t *ppri_dns = (ip4_addr_t *)dns_getserver(0);
		ip4_addr_t *psec_dns = (ip4_addr_t *)dns_getserver(1);
		if(ppri_dns->addr == 0 && psec_dns->addr == 0)
		{
			ip4_addr_t pridns = {0};
			ip4_addr_t secdns = {0};
			ip_addr_t pridns_t = {0};
			ip_addr_t secdns_t = {0};
			
			softap_printf(USER_LOG, WARN_LOG, "[DM]get dns server addr FAIL and use default dns server\n");
			inet_pton(AF_INET, "218.2.2.2", &pridns);
			inet_pton(AF_INET, "218.4.4.4", &secdns);
			if(pridns.addr != 0)
			{
				memcpy(&pridns_t.u_addr.ip4,&pridns,sizeof(ip4_addr_t));
				pridns_t.type = IPADDR_TYPE_V4;
				dns_setserver(0, &pridns_t);
			}
			if(secdns.addr != 0)
			{
				memcpy(&secdns_t.u_addr.ip4,&secdns,sizeof(ip4_addr_t));
				secdns_t.type = IPADDR_TYPE_V4;
				dns_setserver(1, &secdns_t);
			}
		}
		softap_printf(USER_LOG, WARN_LOG, "[DM]get dns server addr success\n");

		dm_make_packet(nccid_dm);
		if(dm_socket_client() == 0)
		{
			softap_printf(USER_LOG, WARN_LOG, "[DM]dm success and store iccid");
			strncpy((char *)g_softap_var_nv->dm_cfg.ue_iccid,(char *)nccid_dm,20);  //store g_ueiccid,20 byte	
			NETDOG_AT_STATICS(dbg_dm_success_num++);
			if(xy_ftl_write(NV_FLASH_DSP_VOLATILE_BASE,DSP_VOLATILE_ALL_LEN-4,((unsigned char *)g_softap_var_nv),sizeof(softap_var_nv_t))!=XY_OK)
			{
				xy_assert(0);
			}
			g_softap_var_nv->dm_cfg.have_retry_num=0;//成功要复位，重启后要走dm
			send_urc_to_ext("\r\n+DM:SUCCESS\r\n");
			break;
		}
		else
		{
			NETDOG_AT_STATICS(dbg_dm_failed_num++);
			softap_printf(USER_LOG, WARN_LOG, "[DM]Register Fail\n");
			if (g_softap_var_nv->dm_cfg.have_retry_num < g_softap_fac_nv->dm_retry_num)
			{
				g_softap_var_nv->dm_cfg.have_retry_num++;
				xy_rtc_timer_create(RTC_TIMER_CMCCDM, g_softap_fac_nv->dm_retry_time, dm_timeout_proc, NULL);
				break;
			}
			else
			{
				g_softap_var_nv->dm_cfg.have_retry_num=0;//失败也要复位
				break;
			}
		}
	}
out:
	xy_free(imei_temp);
	xy_free(imsi_temp);

	if(nccid_dm != NULL)
		xy_free(nccid_dm);
	if(modul_ver != NULL)
	{
		xy_free(modul_ver);
		modul_ver = NULL;
	}
	if(requestbuf != NULL)
	{
		xy_free(requestbuf);
		requestbuf = NULL;
	}
	requestLen = 0;

	if(telecom_timer_flag == 1)
	{
		telecom_timer_flag = 0;
		xy_work_unlock(LOCK_XY_LOCAL);
	}

	softap_printf(USER_LOG, WARN_LOG, "[DM]dm thread exit\n");
#if !IS_DSP_CORE
	if(get_sys_up_stat()==POWER_ON && CHECK_SDK_TYPE(OPERATION_VER))
	 	xy_work_unlock(0);
#endif
	g_telecom_dm_TskHandle=NULL;
	osThreadExit();
}
#endif


void telecom_dm_init(void)
{		
	if(g_telecom_dm_TskHandle != NULL)
	{
		softap_printf(USER_LOG, WARN_LOG, "[DM] dm aready runing,will return\n");
		return;
	}
#if XY_DM && WAKAAMA
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "tele_dm";
	thread_attr.priority   = XY_OS_PRIO_NORMAL1;
	thread_attr.stack_size = 0x800;
	g_telecom_dm_TskHandle = osThreadNew((osThreadFunc_t)(telecom_dm_run), NULL, &thread_attr);

#endif
}


