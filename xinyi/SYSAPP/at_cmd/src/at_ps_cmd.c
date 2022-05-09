/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ps_cmd.h"
#include "at_com.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "at_worklock.h"
#include "atc_ps.h"
#include "dsp_process.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "softap_nv.h"
#include "xy_at_api.h"
#include "xy_atc_interface.h"
#include "xy_proxy.h"
#include "xy_ps_api.h"
#include "xy_rtc_api.h"
#include "xy_system.h"
#include "xy_utils.h"
#if AT_SOCKET
#include "at_socket_context.h"
#endif

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
char g_OOS_flag = 0;
unsigned char *g_ueiccid = NULL;
short g_ps_pdn_state = 0;
/*AT+XYRAI触发的空包携带RAI指示*/
int g_null_udp_rai = 0;
unsigned char g_working_cid = INVALID_CID;
int8_t g_netifIp_type = NON_IP;
int g_check_valid_sim = 1;
int g_uicc_type = UICC_UNKNOWN;
osSemaphoreId_t g_out_OOS_sem = NULL;
int g_Echo_mode = 0;
extern int convert_wall_time2(char *data, char *time, struct xy_wall_clock *wall_time);

/*******************************************************************************
 *                       Local function implementations	                   *
 ******************************************************************************/
static int send_null_udp_paket(char *remote_ip,unsigned short remote_port)
{	
	int s = -1;
	struct sockaddr_in remote_sockaddr = {0};

	s = socket(AF_INET/*srv.family*/, SOCK_DGRAM, IPPROTO_UDP);	

	remote_sockaddr.sin_family = AF_INET;
	remote_sockaddr.sin_port = htons(remote_port);
	if(1 != inet_aton(remote_ip, &remote_sockaddr.sin_addr))
	{
		close(s);
		softap_printf(USER_LOG, WARN_LOG, "xyari open null udp socket fail");
		return XY_ERR;
	}
	
	softap_printf(USER_LOG, WARN_LOG, "xyrai open null udp socket success");

	if (sendto2(s, "A", 1, 0, (struct sockaddr *)&remote_sockaddr, sizeof(struct sockaddr_in), 0, 1) < 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "send_errno:%d", errno);
		close(s);
		return XY_ERR;
	}

	softap_printf(USER_LOG, WARN_LOG, "xyrai null udp send success");
	close(s);
	softap_printf(USER_LOG, WARN_LOG, "xyrai close null udp socket success");
	return XY_OK;
}

/*******************************************************************************
 *                       Global function implementations	                   *
 ******************************************************************************/
void at_NSET_POWERTEST()
{
	g_softap_fac_nv->wfi_enable = 0;
	g_softap_fac_nv->lpm_standby_enable = 0;
	g_softap_fac_nv->deepsleep_enable = 1;
	g_softap_fac_nv->need_start_dm = 1;
	g_softap_fac_nv->ver_type = 1;
	g_softap_fac_nv->keep_retension_mem = 0;
	g_softap_fac_nv->backup_threshold = 0;
	g_softap_fac_nv->off_debug = 1;
	g_softap_fac_nv->down_data = 1;
	SAVE_SOFTAP_FAC();
}

int at_AT_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
	(void) prsp_cmd;

	if (DSP_IS_NOT_LOADED())
		return AT_END;
	else
		return AT_FORWARD;
}

//AT+XYRAI=<remote IP>,<remote port>,send null packet and rai=1
int at_XYRAI_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		char remote_ip[20] = {0};
		unsigned short remote_port = 0;
		
		if (!ps_netif_is_ok()) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto out;
		}
		if ((at_parse_param("%20s,%2d", at_buf, remote_ip, &remote_port) != AT_OK) || remote_port == 0 || !strcmp(remote_ip, ""))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto out;
		}
		g_null_udp_rai = 1;
		if (send_null_udp_paket(remote_ip, remote_port) != XY_OK)
		{
			g_null_udp_rai = 0;
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		}
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		char *remote_ip = "1.1.1.1";
		unsigned short remote_port = 36000;
		
		if (!ps_netif_is_ok()) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto out;			
		}
		g_null_udp_rai = 1;
		if (send_null_udp_paket(remote_ip, remote_port) != XY_OK)
		{
			g_null_udp_rai = 0;
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		}
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
out:
	return AT_END;
}

int at_ECHOMODE_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_ACTIVE)
	{
		int echo_mode_temp = -1;
        if (at_parse_param("%d", at_buf, &echo_mode_temp) != AT_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }
        if (echo_mode_temp != 0 && echo_mode_temp != 1)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }
        g_Echo_mode = echo_mode_temp;
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
	return AT_END;
}

/**
	AT+NSET="STORAGE_OPER",X
	用于入库测试，只输一条命令
	当X值不为0，则自动适配AT+NV=SET,POWERTEST,0 ,保存出厂NV，但不做软重启
*/
int at_NSET_req(char *at_buf, char **prsp_cmd)
{
	(void)prsp_cmd;
	if(g_req_type == AT_CMD_REQ)
	{
		softap_printf(USER_LOG, WARN_LOG, "only used in ruku!!!");
		char cmd[16] = {0};
		int  value = 0;
		at_parse_param("%s,%d", at_buf, cmd, &value);
		if(!strcmp(cmd, "STORAGE_OPER") &&  value != 0)
		{
			at_NSET_POWERTEST();
		}
	}
	return AT_FORWARD;
}

//+CGATT？命令，根据netif状态上报网络结果
int at_CGATT_req(char *at_buf, char **prsp_cmd)
{
	(void)at_buf;
	if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(70);
		if(g_factory_nv->tNvData.tNasNv.ucAtHeaderSpaceFlg == 1)
			sprintf(*prsp_cmd, "\r\n+CGATT: %d\r\n\r\nOK\r\n",ps_netif_is_ok()?1:0);
		else
			sprintf(*prsp_cmd, "\r\n+CGATT:%d\r\n\r\nOK\r\n",ps_netif_is_ok()?1:0);
		return AT_END;
	}
	else
		return AT_FORWARD;
}

//+CGEV: ME PDN DEACT 0
void urc_CGEV_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_CGEV_IND_STRU));
	ATC_MSG_CGEV_IND_STRU *cgev_urc = (ATC_MSG_CGEV_IND_STRU*)xy_malloc(sizeof(ATC_MSG_CGEV_IND_STRU));
	memcpy(cgev_urc, param, paramLen);
	uint8_t cid = cgev_urc->stCgevPara.ucCid;
	softap_printf(USER_LOG, WARN_LOG, "cgev urc eventId:%d,cid:%d", cgev_urc->ucCgevEventId, cgev_urc->stCgevPara.ucCid);

	switch(cgev_urc->ucCgevEventId)
	{
		case D_ATC_CGEV_ME_PDN_ACT:
		{
			g_OOS_flag = 0;
			g_ps_pdn_state |= (1 << cid);
		}
			break;
		case D_ATC_CGEV_NW_PDN_DEACT:
		case D_ATC_CGEV_ME_PDN_DEACT:
		{
			g_OOS_flag = 0;
			g_ps_pdn_state &= (~(1 << cid));
			if (g_working_cid != INVALID_CID && cid != g_working_cid)
			{
				softap_printf(USER_LOG, WARN_LOG, "current cid is not working cid and return");
				if (is_netif_active(cid))
					send_msg_2_proxy(PROXY_MSG_PS_PDP_DEACT, &cid, sizeof(cid));
				return;
			}
			g_working_cid = INVALID_CID;
			g_netifIp_type = NON_IP;
			send_msg_2_proxy(PROXY_MSG_PS_PDP_DEACT, &cid, sizeof(cid));
		}
			break;
		case D_ATC_CGEV_OOS:
		{
			g_OOS_flag = 1;
			psNetifEventInd(EVENT_PSNETIF_ENTER_OOS);
		}
			break;
		case D_ATC_CGEV_IS:
		{
			g_OOS_flag = 0;
			osSemaphoreRelease(g_out_OOS_sem);
			psNetifEventInd(EVENT_PSNETIF_ENTER_IS);
		}
			break;
		default:
			break;
	}
	xy_free(cgev_urc);
}

void urc_CSCON_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	UNUSED_ARG(param);
	UNUSED_ARG(paramLen);
}

/**
 * +XYIPDNS: 
 *<cid_num>,<cid>,<PDP_type>[,<PDP_address>,"",<primary_dns>,<secondary_dns>],
 *[<cid>,<PDP_type>[,<PDP_address>,"",<primary_dns>,<secondary_dns>]]
*/
void urc_XYIPDNS_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(param != NULL && paramLen == sizeof(ATC_MSG_XYIPDNS_IND_STRU));
	ATC_MSG_XYIPDNS_IND_STRU *xyipdns_urc = (ATC_MSG_XYIPDNS_IND_STRU*)xy_malloc(sizeof(ATC_MSG_XYIPDNS_IND_STRU));
	memcpy(xyipdns_urc, param, paramLen);

	struct pdp_active_info pdp_info = { 0 };
	uint8_t pdp_type = 0;
	uint8_t dns_index = 0;

	dns_reset_servers();
	g_working_cid = pdp_info.cid = xyipdns_urc->stPara.ucCid;
	pdp_type = xyipdns_urc->stPara.ucPdpType;
	softap_printf(USER_LOG, WARN_LOG, "xyipdns urc cid:%d, type:%d", pdp_info.cid, pdp_type);

	switch(pdp_type)
	{
		case D_PDP_TYPE_IPV4:
		{
			g_netifIp_type = pdp_info.ip46flag = IPV4_TYPE;
			memcpy(&pdp_info.ip4_addr, (ip4_addr_t *)xyipdns_urc->stPara.aucIPv4Addr, sizeof(xyipdns_urc->stPara.aucIPv4Addr));

			ip_addr_t pridns = {0};
            ip_addr_t secdns = {0};
            pridns.type = IPADDR_TYPE_V4;
			secdns.type = IPADDR_TYPE_V4;
			memcpy(&pridns.u_addr.ip4, (ip4_addr_t *)xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4, sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4));
			memcpy(&secdns.u_addr.ip4, (ip4_addr_t *)xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4, sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4));

			//若协议栈没有上报v4 dns地址，采用factory nv默认配置的dns地址
            ip_addr_t ref_pridns = {0};
            ip_addr_t ref_secdns = {0};
            xy_get_default_dns_server(&ref_pridns, IPADDR_TYPE_V4, 0);
            xy_get_default_dns_server(&ref_secdns, IPADDR_TYPE_V4, 1);

			if (!ip_addr_cmp(&pridns, IP4_ADDR_ANY))
				xy_set_dns_server_by_ipaddr(&pridns, dns_index++);
			else if (!ip_addr_isany(&ref_pridns))
				xy_set_dns_server_by_ipaddr(&ref_pridns, dns_index++);

			if (!ip_addr_cmp(&secdns, IP4_ADDR_ANY))
				xy_set_dns_server_by_ipaddr(&secdns, dns_index++);
			else if (!ip_addr_isany(&ref_secdns))
				xy_set_dns_server_by_ipaddr(&ref_secdns, dns_index++);

			//debug info
			char *ip4_addr = xy_zalloc(16);
			inet_ntop(AF_INET, &pdp_info.ip4_addr, ip4_addr, 16);
			softap_printf(USER_LOG, WARN_LOG, "ip4 addr %s", ip4_addr);
			xy_free(ip4_addr);
		}
			break;
		case D_PDP_TYPE_IPV6:
		{
#if LWIP_IPV6
			g_netifIp_type = pdp_info.ip46flag = IPV6_TYPE;
			memcpy(&pdp_info.ip6_addr, (ip6_addr_t*)(xyipdns_urc->stPara.aucIPv6Addr), sizeof(xyipdns_urc->stPara.aucIPv6Addr));

			//判断v6地址有效性
			if (pdp_info.ip6_addr.addr[0] == 0 && pdp_info.ip6_addr.addr[1] == 0 && pdp_info.ip6_addr.addr[2] == 0 && pdp_info.ip6_addr.addr[3] == 0)
			{
				softap_printf(USER_LOG, WARN_LOG, "ipv6 addr is all 0");
				goto error;
			}

            ip_addr_t pridns6 = {0};
			ip_addr_t secdns6 = {0};
			pridns6.type = IPADDR_TYPE_V6;
			memcpy(&pridns6.u_addr.ip6, (ip6_addr_t*)(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6));
			secdns6.type = IPADDR_TYPE_V6;
			memcpy(&secdns6.u_addr.ip6, (ip6_addr_t*)(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6));
            //若协议栈没有上报v6 dns地址，采用factory nv默认配置的dns地址
            ip_addr_t ref_pridns6 = {0};
            ip_addr_t ref_secdns6 = {0};
            xy_get_default_dns_server(&ref_pridns6, IPADDR_TYPE_V6, 0);
            xy_get_default_dns_server(&ref_secdns6, IPADDR_TYPE_V6, 1);

			if (!ip_addr_cmp(&pridns6, IP6_ADDR_ANY))
				xy_set_dns_server_by_ipaddr(&pridns6, dns_index++);
			else if (!ip_addr_isany(&ref_pridns6))
				xy_set_dns_server_by_ipaddr(&ref_pridns6, dns_index++);

			if (!ip_addr_cmp(&secdns6, IP6_ADDR_ANY))
				xy_set_dns_server_by_ipaddr(&secdns6, dns_index++);
			else if (!ip_addr_isany(&ref_secdns6))
				xy_set_dns_server_by_ipaddr(&ref_secdns6, dns_index++);
#endif
		}
			break;
		case D_PDP_TYPE_IPV4V6:
		{
#if LWIP_IPV4 && LWIP_IPV6
			g_netifIp_type = pdp_info.ip46flag = IPV46_TYPE;
			memcpy(&pdp_info.ip4_addr, (ip4_addr_t *)(xyipdns_urc->stPara.aucIPv4Addr), sizeof(xyipdns_urc->stPara.aucIPv4Addr));
			memcpy(&pdp_info.ip6_addr, (ip6_addr_t *)(xyipdns_urc->stPara.aucIPv6Addr), sizeof(xyipdns_urc->stPara.aucIPv6Addr));

			//判断ipv4/v6地址有效性
			if((pdp_info.ip4_addr.addr == IPADDR_ANY) || 
				(pdp_info.ip6_addr.addr[0] == 0 && pdp_info.ip6_addr.addr[1] == 0 && pdp_info.ip6_addr.addr[2] == 0 && pdp_info.ip6_addr.addr[3] == 0))
			{
				softap_printf(USER_LOG, WARN_LOG, "ipv4 addr or ipv6 addr is all 0");
				goto error;
			}

			ip_addr_t pridns = {0};
			ip_addr_t secdns = {0};
			ip_addr_t pridns6 = {0};
			ip_addr_t secdns6 = {0};

			pridns.type = IPADDR_TYPE_V4;
			memcpy(&pridns.u_addr.ip4, (ip4_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4), sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4));
			pridns6.type = IPADDR_TYPE_V6;
			memcpy(&pridns6.u_addr.ip6, (ip6_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6));
			secdns.type = IPADDR_TYPE_V4;
			memcpy(&secdns.u_addr.ip4, (ip4_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4), sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4));
			secdns6.type = IPADDR_TYPE_V6;
			memcpy(&secdns6.u_addr.ip6, (ip6_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6));

            //若协议栈没有上报v4v6 dns地址，采用factory nv默认配置的dns地址
            ip_addr_t ref_pridns = {0};
            ip_addr_t ref_secdns = {0};
            xy_get_default_dns_server(&ref_pridns, IPADDR_TYPE_V4, 0);
            xy_get_default_dns_server(&ref_secdns, IPADDR_TYPE_V4, 1);
            ip_addr_t ref_pridns6 = {0};
            ip_addr_t ref_secdns6 = {0};
            xy_get_default_dns_server(&ref_pridns6, IPADDR_TYPE_V6, 0);
            xy_get_default_dns_server(&ref_secdns6, IPADDR_TYPE_V6, 1);

            if (!ip_addr_cmp(&pridns, IP4_ADDR_ANY))
                xy_set_dns_server_by_ipaddr(&pridns, dns_index++);
            else if (!ip_addr_isany(&ref_pridns))
                xy_set_dns_server_by_ipaddr(&ref_pridns, dns_index++);

            if (!ip_addr_cmp(&secdns, IP4_ADDR_ANY))
                xy_set_dns_server_by_ipaddr(&secdns, dns_index++);
            else if (!ip_addr_isany(&ref_secdns))
                xy_set_dns_server_by_ipaddr(&ref_secdns, dns_index++);

            if (!ip_addr_cmp(&pridns6, IP6_ADDR_ANY))
                xy_set_dns_server_by_ipaddr(&pridns6, dns_index++);
            else if (!ip_addr_isany(&ref_pridns6))
                xy_set_dns_server_by_ipaddr(&ref_pridns6, dns_index++);

            if (!ip_addr_cmp(&secdns6, IP6_ADDR_ANY))
                xy_set_dns_server_by_ipaddr(&secdns6, dns_index++);
            else if (!ip_addr_isany(&ref_secdns6))
                xy_set_dns_server_by_ipaddr(&ref_secdns6, dns_index++);

#endif //LWIP_IPV4 && LWIP_IPV6
		}
			break;
		case D_PDP_TYPE_NonIP:
			g_netifIp_type = pdp_info.ip46flag = NON_IP;
			break;
		default:
			break;
	}

	xy_free(xyipdns_urc);
	send_msg_2_proxy(PROXY_MSG_PS_PDP_ACT, &pdp_info, sizeof(struct pdp_active_info));
	return;
error:
	g_netifIp_type = NON_IP;
	xy_free(xyipdns_urc);
	NETDOG_AT_STATICS(dbg_pdp_act_fail++);
	return;
}

//^SIMST:<n>
void urc_SIMST_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_SIMST_IND_STRU));
	ATC_MSG_SIMST_IND_STRU *simst_urc = (ATC_MSG_SIMST_IND_STRU*)xy_malloc(sizeof(ATC_MSG_SIMST_IND_STRU));
	memcpy(simst_urc, param,paramLen);

	softap_printf(USER_LOG, WARN_LOG, "sim urc status:%d", simst_urc->ucSimStatus);

	//zhongyi request send "POWERON" followed by "^SIMST:",see  sys_up_urc()
	if (g_softap_fac_nv->work_mode == 0)
	{
		sys_up_urc();
	}
	
	if(simst_urc->ucSimStatus == 0)
	{
		g_check_valid_sim = 0;
		softap_printf(USER_LOG, WARN_LOG, "no sim card");
	}
	else
	{
		g_check_valid_sim = 1;
	}
	xy_free(simst_urc);
}

//+PSINFO:<cell id>,<sim Vcc>
//+PSINFO:HEAP,<dsp heap used MAX>
//PS的出厂NV在DSP核保存；平台的出厂NV必须通过核间消息通过M3核来更改后保存
void urc_PSINFO_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_PSINFO_IND_STRU));
	ATC_MSG_PSINFO_IND_STRU *psinfo_urc = (ATC_MSG_PSINFO_IND_STRU*)xy_malloc(sizeof(ATC_MSG_PSINFO_IND_STRU));
	memcpy(psinfo_urc, param, paramLen);

	softap_printf(USER_LOG, WARN_LOG, "psinfo urc type:%d,value:%d,", psinfo_urc->ucType, psinfo_urc->ucValue);

	if(psinfo_urc->ucType == D_MSG_PSINFO_TYPE_SIMVCC)
	{

		g_softap_fac_nv->sim_vcc_ctrl = psinfo_urc->ucValue;
		SAVE_FAC_PARAM(sim_vcc_ctrl);
	}
	//only save  DSP factory NV,and softap NV save by SAVE_FAC_PARAM
	else if(psinfo_urc->ucType == D_MSG_PSINFO_TYPE_NVWRITE && psinfo_urc->ucValue == 1)
	{
		save_factory_nv();
	}
	else if(psinfo_urc->ucType == D_MSG_PSINFO_TYPE_SOFT_RESET)
	{
		xy_reboot();
	}
	xy_free(psinfo_urc);
}

//+CTZEU:<tz>,<dst>,[<utime>]   +CTZEU:+32,0,2019/03/29,21:12:56
void urc_CTZEU_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_LOCALTIMEINFO_IND_STRU));
	ATC_MSG_LOCALTIMEINFO_IND_STRU *ctzeu_urc = (ATC_MSG_LOCALTIMEINFO_IND_STRU*)xy_malloc(sizeof(ATC_MSG_LOCALTIMEINFO_IND_STRU));
	memcpy(ctzeu_urc, param, paramLen);

	int zone_sec = 0;
	struct xy_wall_clock wall_time = {0};
	uint8_t zone = 0;
	uint8_t DayLightTime = 0;
#if VER_GXX
	if (g_CTZU_mode != 1)
	{
		return;
	}
#else
	if (g_NITZ_mode != 1)
	{
		return;
	}
#endif
	wall_time.tm_year = 2000 + (ctzeu_urc->stPara.aucUtAndLtz[0] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[0] & 0xF0) >> 4);
	wall_time.tm_mon = (ctzeu_urc->stPara.aucUtAndLtz[1] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[1] & 0xF0) >> 4);
	wall_time.tm_mday = (ctzeu_urc->stPara.aucUtAndLtz[2] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[2] & 0xF0) >> 4);
	wall_time.tm_hour = (ctzeu_urc->stPara.aucUtAndLtz[3] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[3] & 0xF0) >> 4);
	wall_time.tm_min = (ctzeu_urc->stPara.aucUtAndLtz[4] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[4] & 0xF0) >> 4);
	wall_time.tm_sec = (ctzeu_urc->stPara.aucUtAndLtz[5] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[5] & 0xF0) >> 4);

	zone = (ctzeu_urc->stPara.ucLocalTimeZone & 0x07) * 10 + ((ctzeu_urc->stPara.ucLocalTimeZone & 0xF0) >> 4);
	if(ctzeu_urc->stPara.ucNwDayltSavTimFlg == D_ATC_FLAG_TRUE)
	{
		DayLightTime = (ctzeu_urc->stPara.ucNwDayltSavTim & 3) * 4;
	}

	if((ctzeu_urc->stPara.ucLocalTimeZone & 0x08) == 0)
	{
		zone += DayLightTime;
		g_softap_var_nv->g_zone = zone;
	}
	else
	{
		zone -= DayLightTime;
		g_softap_var_nv->g_zone = -1 * zone;
	}
	
	zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;

	softap_printf(USER_LOG, WARN_LOG, "+CTZEU:%1d,%1d,%d/%d/%d,%d:%d:%d\r\n",\
		zone,DayLightTime,wall_time.tm_year,wall_time.tm_mon,wall_time.tm_mday,wall_time.tm_hour,wall_time.tm_min,wall_time.tm_sec);
	xy_rtc_set_time(&wall_time, zone_sec);
	xy_free(ctzeu_urc);
}

void urc_UDPRAI_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_IPSN_IND_STRU));
	ATC_MSG_IPSN_IND_STRU *ipsn_urc = (ATC_MSG_IPSN_IND_STRU*)xy_malloc(sizeof(ATC_MSG_IPSN_IND_STRU));
	memcpy(ipsn_urc, param, paramLen);

	//仅关心最后一个上行报文是否已经被PS处理过，以触发RAI流程
	if(g_softap_fac_nv->close_rai == 0 && g_udp_latest_seq == ipsn_urc->usIpSn)
		g_udp_latest_seq = 0;

	xy_free(ipsn_urc);
}

#if AT_SOCKET
//+IPSEQUENCE:<seq>,<status>
void urc_UDPSN_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_IPSN_IND_STRU));
	ATC_MSG_IPSN_IND_STRU *ipsn_urc = (ATC_MSG_IPSN_IND_STRU*)xy_malloc(sizeof(ATC_MSG_IPSN_IND_STRU));
	memcpy(ipsn_urc, param, paramLen);

	unsigned short seq_soc_num = 0;
	char *sequence_rsp = NULL;
	char send_status = 0;
	unsigned short socket_fd = 0;
	int soc_ctx_id = -1;
	unsigned short seq_num = 0;

	seq_soc_num = ipsn_urc->usIpSn;
	send_status = ipsn_urc->ucStatus;
	xy_free(ipsn_urc);

	socket_fd = (unsigned short)((seq_soc_num & 0XFF00) >> 8);
	seq_num = (unsigned short)(seq_soc_num & 0X00FF);
	softap_printf(USER_LOG, WARN_LOG, "socket_fd:%d",socket_fd);

	//socket context proc
	if ((soc_ctx_id = find_sock_ctx_id_by_sock_fd(socket_fd)) == -1)
		return;
	if (seq_num > SEQUENCE_MAX || seq_num <= 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "UDPSEQUENCE ERR!!!");
		return;
	}

	if (find_match_udp_node(sock_ctx[soc_ctx_id]->sock_id, (unsigned char)seq_num) == 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "find no match udp mode!!!");
		return;
	}

	sock_ctx[soc_ctx_id]->sequence_state[seq_num - 1] = send_status;
	del_sninfo_node(sock_ctx[soc_ctx_id]->sock_id, seq_num);
	sequence_rsp = (char*)xy_zalloc(30);
#if VER_QUCTL260
    if (send_status == 1)
        snprintf(sequence_rsp, 30, "\r\nSEND OK\r\n");
    else
        snprintf(sequence_rsp, 30, "\r\nSEND FAIL\r\n");
#else
	snprintf(sequence_rsp, 30, "\r\n+NSOSTR:%d,%d,%d\r\n", sock_ctx[soc_ctx_id]->sock_id, seq_num, sock_ctx[soc_ctx_id]->sequence_state[seq_num - 1]);
#endif //VER_QUCTL260
    send_rsp_str_to_ext(sequence_rsp); 
	xy_free(sequence_rsp);
}
#endif //AT_SOCKET

void ps_urc_register_callback_init()
{
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_SIMST, urc_SIMST_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_XYIPDNS, urc_XYIPDNS_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_CGEV, urc_CGEV_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_CSCON, urc_CSCON_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_LOCALTIMEINFO, urc_CTZEU_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_PSINFO, urc_PSINFO_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_IPSN, urc_UDPRAI_Callback);
#if AT_SOCKET
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_IPSN, urc_UDPSN_Callback);
#endif
}

