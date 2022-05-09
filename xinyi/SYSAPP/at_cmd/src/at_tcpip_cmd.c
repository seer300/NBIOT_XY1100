/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_tcpip_cmd.h"
#include "at_com.h"
#include "at_global_def.h"
#include "at_ps_cmd.h"
#include "lwip/apps/sntp.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/timeouts.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "rtc_tmr.h"
#include "softap_nv.h"
#include "xy_at_api.h"
#include "xy_net_api.h"
#include "xy_rtc_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef enum dns_query_type
{
	CMDNS = 0,
	QDNS,
	XDNS,
    BC26DNS,
} dns_query_type_t;

typedef struct dns_query_info
{
	char *domain_name;
	uint8_t query_type : 4;   //@see @ref dns_query_type
	uint8_t disable_cache : 4;
	uint16_t timeout;
} dns_query_info_t;

typedef enum ntp_type
{
	CMNTP = 0,
	QNTP,
	XNTP,
} ntp_type_t;
typedef struct ntp_param
{
	char *domain_name;
	uint8_t update_rtc;
    uint8_t ntp_type;
    uint16_t port;
	int timeout;
} ntp_param_t;

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
int g_double_trance = 0;		//内部测试使用，非0表示需要多次往tcpip发送下行数据
int g_double_trance_num = 1;	//内部测试使用，表示往tcpip发送下行数据的次数，默认值为1

/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/
osThreadId_t query_ntp_thread_id = NULL;
osThreadId_t query_dns_thread_id = NULL;

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
static void query_ntp_task(void *para)
{
	ntp_param_t *cmntpparm = (ntp_param_t *)para;
	struct sntp_time *ptimemap = NULL;
	int zone_sec = 0;
	char *rsp_cmd = NULL;

    ptimemap = gettimebysntp(cmntpparm->domain_name, cmntpparm->port, cmntpparm->timeout);
    if (ptimemap == NULL)
    {
        rsp_cmd = xy_zalloc(32);
        if (cmntpparm->ntp_type == CMNTP)
        {
            int errno_num = 0;
            if (errno == -2)
                errno_num = 2;
            else if (errno == -3)
                errno_num = 1;
            else
                errno_num = errno;
            sprintf(rsp_cmd, "\r\n+CMNTP:%d\r\n", errno_num);
        }
        else if(cmntpparm->ntp_type == QNTP)
        {
            int errcode = get_bc26_tcpip_errcode(errno);
            sprintf(rsp_cmd, "\r\n+QNTP: %d\r\n", errcode);
        }
		goto END;
	}
    if (g_softap_var_nv->g_zone != 0)
        zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;
    else
        zone_sec = 8 * 60 * 60;

    char zone[4] = {0};
	char positive_zone;

    if (g_softap_var_nv->g_zone >= 0)
    {
		positive_zone = g_softap_var_nv->g_zone;
		if(g_softap_var_nv->g_zone >= 10)
			sprintf(zone,"+%2d",positive_zone);
		else
			sprintf(zone,"+%1d",positive_zone);
	}
	else
	{
		positive_zone = 0 - g_softap_var_nv->g_zone;
		if(g_softap_var_nv->g_zone <= -10)
			sprintf(zone,"-%2d",positive_zone);
		else
			sprintf(zone,"-%1d",positive_zone);
	}
			
	uint64_t sec = ptimemap->sec;
	struct rtc_time rtctime={0};

    xy_gmtime_r(sec * 1000, &rtctime);
    rsp_cmd = xy_zalloc(100);
    if (cmntpparm->ntp_type == CMNTP)
    {
        strcpy(rsp_cmd, "\r\n+CMNTP:0,");
        sprintf(rsp_cmd + strlen(rsp_cmd), "\"%02lu/%02lu/%02lu,%02lu:%02lu:%02lu%s\"\r\n", rtctime.tm_year - 2000, rtctime.tm_mon, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec, zone);
    }
    else if (cmntpparm->ntp_type == QNTP)
    {
        strcpy(rsp_cmd, "\r\n+QNTP: 0,");
        sprintf(rsp_cmd + strlen(rsp_cmd), "\"%02lu/%02lu/%02lu,%02lu:%02lu:%02lu\"\r\n", rtctime.tm_year - 2000, rtctime.tm_mon, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec);
    }
    if (cmntpparm->update_rtc == 1)
        reset_universal_timer(&rtctime,zone_sec);
	
END:
	send_rsp_str_to_ext(rsp_cmd);
	xy_free(rsp_cmd);
    if (cmntpparm != NULL && cmntpparm->domain_name != NULL)
        xy_free(cmntpparm->domain_name);
    if (cmntpparm != NULL)
        xy_free(cmntpparm);

    query_ntp_thread_id = NULL;
	osThreadExit();
}

static void query_dns_task(void *dnsquery)
{
    dns_query_info_t *dns_query = (dns_query_info_t *)dnsquery;
	char *rsp_cmd = NULL;
	softap_printf(USER_LOG, WARN_LOG, "domain_name:%s\r\n", dns_query->domain_name);

	if (dns_query->query_type == CMDNS && dns_query->timeout != 0)
		reset_dns_tmr_interval(dns_query->timeout * 1000);

	struct addrinfo hint = {0};
	struct addrinfo *result = NULL;
	struct addrinfo *tmp = NULL;
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_DGRAM;
    int ret = 0;

    if ((ret = getaddrinfo(dns_query->domain_name, NULL, &hint, &result)) != 0)
	{
		softap_printf(USER_LOG, WARN_LOG, "getaddrinfo errno:%d\r\n", ret);
        rsp_cmd = xy_zalloc(32);

		if(dns_query->query_type == XDNS)
			strcpy(rsp_cmd, "\r\n+XDNS:QUERY_DNS_FAILED\r\n");
		else if(dns_query->query_type == CMDNS)
			strcpy(rsp_cmd, "\r\n+CMDNS:QUERY_DNS_FAILED\r\n");
		else if (dns_query->query_type == QDNS)
			strcpy(rsp_cmd, "\r\n+QDNS:QUERY_DNS_FAILED\r\n");
		else if (dns_query->query_type == BC26DNS)
		{
            snprintf(rsp_cmd, 32, "\r\n+QIDNSGIP: %d\r\n", get_bc26_tcpip_errcode(ret));
        }
            
		goto END;
	}

	int output_len = 40;
    int ip_num = 0;

    for (tmp = result; tmp != NULL; tmp = tmp->ai_next)
	{
		if (tmp->ai_family == AF_INET)
			output_len += 16;
		else if (tmp->ai_family == AF_INET6)
			output_len += 40;
        ip_num++;
    }

	rsp_cmd = xy_zalloc(output_len);

	if (dns_query->query_type == CMDNS)
		strcpy(rsp_cmd, "\r\n+CMDNS:");
	else if (dns_query->query_type == QDNS)
		strcpy(rsp_cmd, "\r\n+QDNS:");
	else if (dns_query->query_type == XDNS)
		strcpy(rsp_cmd, "\r\n+XDNS:");
	else if (dns_query->query_type == BC26DNS)
    {
        sprintf(rsp_cmd, "\r\n+QIDNSGIP: 0,%d,0\r\n+QIDNSGIP: ", ip_num);
    }

    for (tmp = result; tmp != NULL; tmp = tmp->ai_next)
	{
		if (tmp->ai_family == AF_INET)
		{
			char ip_addr[16] = {0};
			struct sockaddr_in* addr = (struct sockaddr_in*)tmp->ai_addr;
			snprintf(rsp_cmd + strlen(rsp_cmd), 20, "%s,", inet_ntop(AF_INET, &addr->sin_addr, ip_addr, 16));
		}
		else if (tmp->ai_family == AF_INET6)
		{
			char ip6_addr[40] = {0};
			struct sockaddr_in6* addr6 = (struct sockaddr_in6*)tmp->ai_addr;
			snprintf(rsp_cmd + strlen(rsp_cmd), 42, "%s,", inet_ntop(AF_INET6, &addr6->sin6_addr, ip6_addr, 40));
		}
	}
	strcpy(rsp_cmd + strlen(rsp_cmd) - 1, "\r\n");
	if (dns_query->disable_cache == 1)
	{
		//QDNS=2,<hostname> 不保留缓存
		dns_clear_cache(dns_query->domain_name);
	}
	freeaddrinfo(result);
END:
	send_rsp_str_to_ext(rsp_cmd);
	softap_printf(USER_LOG, WARN_LOG, "query_dns_task %d exit", dns_query->query_type);

	if (rsp_cmd != NULL)
		xy_free(rsp_cmd);
	if (dns_query != NULL && dns_query->domain_name != NULL)
		xy_free(dns_query->domain_name);
	if (dns_query != NULL)
		xy_free(dns_query);

	query_dns_thread_id = NULL;
	osThreadExit();
}

static void bc26_get_dns_server(int index, char* ipaddr)
{
    ip_addr_t addr = {0};
    if (xy_get_dns_server_by_ipaddr(index, &addr) != XY_OK)
    {
        return;
    }

    if (ip_addr_isany(&addr))
    {
        return;
    }
    else
    {
        ipaddr_ntoa_r(&addr, ipaddr, XY_IPADDR_STRLEN_MAX);
    }
    return;
}

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
//AT+XDNSCFG=<pri_dns>[,<sec_dns>]
int at_XDNSCFG_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
        char *pri_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);
        char *sec_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);

        if (at_parse_param("%46s,%46s", at_buf, pri_dns, sec_dns) != AT_OK)
		{
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto XCFG_END;
        }

        bool pridns_set = (strlen(pri_dns) != 0) ? true : false;
        bool secdns_set = (strlen(sec_dns) != 0) ? true : false;

		if (!pridns_set)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto XCFG_END;
		}

		if (xy_set_dns_server(pri_dns, 0) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto XCFG_END;
		}

        if (xy_set_default_dns_server2(pri_dns, 0) != XY_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto XCFG_END;
        }

		if (secdns_set && xy_set_dns_server(sec_dns, 1) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto XCFG_END;
		}

		if (secdns_set && xy_set_default_dns_server2(sec_dns, 1) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto XCFG_END;
		}
XCFG_END:
		if (pri_dns != NULL)
			xy_free(pri_dns);
		if (sec_dns != NULL)
			xy_free(sec_dns);
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(120);
        char *pri_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);
        char *sec_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);

        xy_get_dns_server(0, pri_dns);
        xy_get_dns_server(1, sec_dns);

        snprintf(*prsp_cmd, 120, "\r\n+XDNSCFG:%s,%s\r\n\r\nOK\r\n", pri_dns, sec_dns);

        xy_free(pri_dns);
        xy_free(sec_dns);
    }
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

#if VER_QUCTL260
//AT+QIDNSCFG=<contextID>,<pridnsaddr>[,<secdnsaddr>]
//AT+QIDNSCFG=<contextID>查询
int at_QIDNSCFG_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
        ip_addr_t pridns = {0};
        ip_addr_t secdns = {0};
        char *pri_dns = NULL;
        char *sec_dns = NULL;
        int contextId = 0;

        /* dns解析正在执行不允许做配置操作 */
        if (query_dns_thread_id != NULL)
		{
			*prsp_cmd = BC26_AT_ERR_BUILD();
			return AT_END;
		}

        pri_dns = (char *)xy_zalloc(64);
        sec_dns = (char *)xy_zalloc(64);

        if (at_parse_param("%d(0-0),%64s,%64s", at_buf, &contextId, pri_dns, sec_dns) != AT_OK)
		{
			*prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        bool pridns_set = (strlen(pri_dns) != 0) ? true : false;
        bool secdns_set = (strlen(sec_dns) != 0) ? true : false;

        if (!pridns_set && secdns_set)
        {
 			*prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }
 
        /* pridns和secdns不输入表示查询 */
        if (!pridns_set && !secdns_set)
        {
            //maybe AT+QIDNSCFG=0, error!!
            if (strchr(at_buf,',') != NULL)
            {
                *prsp_cmd = BC26_AT_ERR_BUILD();
                goto END_PROC;
            }

            char *pri_dnsv6 = (char *)xy_zalloc(64);
            char *sec_dnsv6 = (char *)xy_zalloc(64);

            bc26_get_dns_server(0, pri_dns);
            bc26_get_dns_server(1, sec_dns);
            bc26_get_dns_server(2, pri_dnsv6);
            bc26_get_dns_server(3, sec_dnsv6);

            *prsp_cmd = xy_zalloc(160);
            snprintf(*prsp_cmd, 160, "\r\n+QIDNSCFG: %d,\"%s\",\"%s\",\"%s\",\"%s\"\r\n\r\nOK\r\n", contextId, pri_dns, sec_dns, pri_dnsv6, sec_dnsv6);
            xy_free(pri_dnsv6);
            xy_free(sec_dnsv6);
            goto END_PROC;
        }
        
        /* pridns字符串ip数据合法性检测 */
        if (xy_ipaddr_check(pri_dns, IPV4_TYPE) != XY_OK && xy_ipaddr_check(pri_dns, IPV6_TYPE) != XY_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        ipaddr_aton(pri_dns, &pridns);
        if (pridns.type == IPADDR_TYPE_V4)
        {
            xy_set_dns_server_by_ipaddr(&pridns, 0);
        }
        else if (pridns.type == IPADDR_TYPE_V6)
        {
            xy_set_dns_server_by_ipaddr(&pridns, 2);
        }
        else
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }       

        if (secdns_set)
        {
            if (xy_ipaddr_check(sec_dns, IPV4_TYPE) != XY_OK && xy_ipaddr_check(sec_dns, IPV6_TYPE) != XY_OK)
            {
                *prsp_cmd = BC26_AT_ERR_BUILD();
                goto END_PROC;
            }

            ipaddr_aton(sec_dns, &secdns);
            if (secdns.type == IPADDR_TYPE_V4)
            {
                xy_set_dns_server_by_ipaddr(&secdns, 1);
            }
            else if (secdns.type == IPADDR_TYPE_V6)
            {
                xy_set_dns_server_by_ipaddr(&secdns, 3);
            }
            else
            {
                *prsp_cmd = BC26_AT_ERR_BUILD();
                goto END_PROC;
            }
        }

END_PROC:
        if (pri_dns != NULL)
            xy_free(pri_dns);
        if (sec_dns != NULL)
            xy_free(sec_dns);
        return AT_END;
	}
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(64);
        snprintf(*prsp_cmd, 64, "\r\n+QIDNSCFG: (0-11),<pridnsaddr>,<secdnsaddr>\r\n\r\nOK\r\n");
    }
	else
		*prsp_cmd = BC26_AT_ERR_BUILD();
	return AT_END;
}
#else
//AT+QIDNSCFG=<pri_dns>[,<sec_dns>]
int at_QIDNSCFG_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		if (query_dns_thread_id != NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}

        char *pri_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);
        char *sec_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);

        if (at_parse_param("%46s,%46s", at_buf, pri_dns, sec_dns) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		bool pridns_set = (strlen(pri_dns) != 0) ? true : false;
		bool secdns_set = (strlen(sec_dns) != 0) ? true : false;

		if (!pridns_set)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto QCFG_END;
		}

		/* QIDNSCFG配置dns不保存到nv */
		if (xy_set_dns_server(pri_dns, 0) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto QCFG_END;
		}

		if (secdns_set && xy_set_dns_server(sec_dns, 1) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}

QCFG_END:
		if (pri_dns != NULL)
			xy_free(pri_dns);
		if (sec_dns != NULL)
			xy_free(sec_dns);
		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(120);
        char *pri_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);
        char *sec_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);

        xy_get_dns_server(0, pri_dns);
        xy_get_dns_server(1, sec_dns);
        snprintf(*prsp_cmd, 120, "\r\nPrimaryDns:%s\nSecondaryDns:%s\r\n\r\nOK\r\n", pri_dns, sec_dns);

        xy_free(pri_dns);
        xy_free(sec_dns);
    }
    else if (g_req_type == AT_CMD_TEST)
        return AT_END;
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

#endif //VER_QUCTL260

//AT+XDNS=<domain_name>，支持同步和异步两种方式
int at_XDNS_req(char *at_buf, char **prsp_cmd)
{
	osThreadAttr_t thread_attr = {0};

	if (g_req_type == AT_CMD_REQ)
	{
		char *domain_name = NULL;
        
		if (xy_wait_tcpip_ok(0) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			return AT_END;
		}

		domain_name = xy_zalloc(strlen(at_buf));
		if (at_parse_param("%s", at_buf, domain_name) != AT_OK || strlen(domain_name) > REMOTE_SERVER_LEN)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(domain_name);
			return AT_END;
		}

		//异步方式，即先报OK，后报URC；格式为：AT+XDNS=XX->OK->+XDNS:
		if(0)
		{
			dns_query_info_t *xdns = NULL;
			xdns = xy_zalloc(sizeof(dns_query_info_t));
			xdns->domain_name = xy_zalloc(strlen(domain_name)+1);
			memcpy(xdns->domain_name,domain_name,strlen(domain_name));
			xdns->query_type = XDNS;
			xdns->disable_cache = 0;
            xdns->timeout = 0;
            xy_free(domain_name);

			if (query_dns_thread_id != NULL)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
				xy_free(xdns->domain_name);
				xy_free(xdns);
				return AT_END;
			}
			thread_attr.name	   = "query_dns";
			thread_attr.priority   = osPriorityNormal1;
			thread_attr.stack_size = 4 * 1024;
			query_dns_thread_id = osThreadNew((osThreadFunc_t)(query_dns_task), xdns, &thread_attr);
		}
		//同步方式,先报URC，再报OK；格式为：AT+XDNS=XX->+XDNS:->OK
		else
		{
            struct addrinfo hint = {0};
            struct addrinfo *result = NULL;
            struct addrinfo *tmp = NULL;
            hint.ai_family = AF_UNSPEC;  //maybe modify later
            hint.ai_socktype = SOCK_DGRAM;
            if (getaddrinfo(domain_name, NULL, &hint, &result) != 0)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
                xy_free(domain_name);
                return AT_END;
            }

            int output_len = 15;

            for (tmp = result; tmp != NULL; tmp = tmp->ai_next)
            {
                if (tmp->ai_family == AF_INET)
                    output_len += 16;
                else if (tmp->ai_family == AF_INET6)
                    output_len += 40;
            }

            *prsp_cmd = xy_zalloc(output_len);
            strcpy(*prsp_cmd, "\r\n+XDNS:");

            for (tmp = result; tmp != NULL; tmp = tmp->ai_next)
            {
                if (tmp->ai_family == AF_INET)
                {
                    char ip_addr[16] = {0};
                    struct sockaddr_in *addr = (struct sockaddr_in *)tmp->ai_addr;
                    snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "%s,", inet_ntop(AF_INET, &addr->sin_addr, ip_addr, 16));
                }
                else if (tmp->ai_family == AF_INET6)
                {
                    char ip6_addr[40] = {0};
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)tmp->ai_addr;
                    snprintf(*prsp_cmd + strlen(*prsp_cmd), 42, "%s,", inet_ntop(AF_INET6, &addr6->sin6_addr, ip6_addr, 40));
                }
            }
            freeaddrinfo(result);
            strcpy(*prsp_cmd + strlen(*prsp_cmd) - 1, "\r\n\r\nOK\r\n");
			xy_free(domain_name);
		}
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+CMDNS=<domain_name>[,<pri_dns>,<sec_dns>,<port>,<interval_time>]  先报OK，再报+CMDNS：
int at_CMDNS_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
		char *domain_name = NULL;
		char* pri_dns = NULL;
		char* sec_dns = NULL;
		int port = 0;
		int interval_time = 30;
		dns_query_info_t *cmdns = NULL;
		osThreadAttr_t thread_attr = {0};
		
		if (xy_wait_tcpip_ok(0) != XY_OK) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			return AT_END;
		}

		if (query_dns_thread_id != NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}

        pri_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);
        sec_dns = xy_zalloc(XY_IPADDR_STRLEN_MAX);
        domain_name = xy_zalloc(strlen(at_buf));
		 	
		if (at_parse_param("%s,%46s,%46s,%d,%d", at_buf, domain_name, pri_dns, sec_dns, &port, &interval_time) != AT_OK) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto CMDNS_END;
		}

        if (strlen(domain_name) > REMOTE_SERVER_LEN || strlen(domain_name) == 0)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto CMDNS_END;
        }

		/* CMDNS配置dns保存到文件系统 */
		bool pridns_set = (strlen(pri_dns) != 0) ? true : false;
		bool secdns_set = (strlen(sec_dns) != 0) ? true : false;

		if (pridns_set && xy_set_dns_server(pri_dns, 0) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto CMDNS_END;
		}

        if (pridns_set && xy_set_default_dns_server2(pri_dns, 0) != XY_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto CMDNS_END;
        }

        if ((pridns_set && secdns_set) && xy_set_dns_server(sec_dns, 1) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto CMDNS_END;
		}

        if ((pridns_set && secdns_set) && xy_set_default_dns_server2(sec_dns, 1) != XY_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto CMDNS_END;
        }

        cmdns = xy_zalloc(sizeof(dns_query_info_t));
		cmdns->domain_name = xy_zalloc(strlen(domain_name) + 1);
		memcpy(cmdns->domain_name, domain_name, strlen(domain_name));
		cmdns->query_type = CMDNS; 
		cmdns->disable_cache = 0;
		cmdns->timeout = interval_time;
		
		thread_attr.name = "query_dns";
		thread_attr.priority   = osPriorityNormal1;
		thread_attr.stack_size = 4 * 1024;
		query_dns_thread_id = osThreadNew((osThreadFunc_t)(query_dns_task), cmdns, &thread_attr);

CMDNS_END:
		if (pri_dns != NULL)
			xy_free(pri_dns);
		if (sec_dns != NULL)
			xy_free(sec_dns);
        if (domain_name != NULL)
            xy_free(domain_name);
        return AT_END;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+QDNS=<mode>[,<hostname>]，先报OK，再报+QDNS:
int at_QDNS_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
	    int mode = 0;
		char *domain_name = NULL;
        dns_query_info_t *qdns = NULL;
		osThreadAttr_t thread_attr = {0};

        if (xy_wait_tcpip_ok(0) != XY_OK)
        {
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			return AT_END;
		}

		if (query_dns_thread_id != NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}

		domain_name = xy_zalloc(strlen(at_buf));

		if (at_parse_param("%d(0-2),%s", at_buf, &mode, domain_name) != AT_OK || strlen(domain_name) > REMOTE_SERVER_LEN)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(domain_name);
			return AT_END;
		}

		if (mode == 1)
		{
			if (strlen(domain_name) == 0)
				dns_clear_cache(NULL);
			else
				dns_clear_cache(domain_name);
			xy_free(domain_name);
		}
		else //mode == 0/2
		{
			if (strlen(domain_name) == 0)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				xy_free(domain_name);
				return AT_END;
			}

			qdns = xy_zalloc(sizeof(dns_query_info_t));
			qdns->domain_name = xy_zalloc(strlen(domain_name) + 1);
			memcpy(qdns->domain_name, domain_name, strlen(domain_name));
			qdns->query_type = QDNS;
			qdns->disable_cache = (mode == 2) ? 1 : 0;
			qdns->timeout = 0;
			xy_free(domain_name);

			thread_attr.name = "query_dns";
			thread_attr.priority = osPriorityNormal1;
			thread_attr.stack_size = 4 * 1024;
			query_dns_thread_id = osThreadNew((osThreadFunc_t)(query_dns_task), qdns, &thread_attr);
		}
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//+CMNTP: 0,"19/06/05,08:53:37+32"
int at_CMNTP_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ || g_req_type == AT_CMD_ACTIVE)
	{
		ntp_param_t *cmntpparm = NULL;
		int port = 0;
		int timeout = 0;
		uint8_t update_rtc = 1;
		osThreadAttr_t thread_attr = {0};
#if VER_GXX
		if (g_CTZU_mode != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END;
		}
#else
		if (g_NITZ_mode != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END;
		}
#endif
		if (!ps_netif_is_ok())
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto END;
		}

		char *domain_name = xy_zalloc(strlen(at_buf)+20);

		if (at_parse_param("%s,%d,%1d,%d", at_buf, domain_name, &port, &update_rtc, &timeout) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(domain_name);
			goto END;
		}
		if (strlen(domain_name) > REMOTE_SERVER_LEN || \
			update_rtc != 1 || (timeout > 300 || timeout < 0) || port<0 || port>65535)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(domain_name);
			goto END;
		}

		if(strlen(domain_name) == 0)
			memcpy(domain_name,"ntp1.aliyun.com",strlen("ntp1.aliyun.com"));

		if (query_ntp_thread_id != NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			xy_free(domain_name);
			goto END;
		}

		cmntpparm = xy_zalloc(sizeof(ntp_param_t));
		cmntpparm->domain_name = xy_zalloc(strlen(domain_name)+1);
		memcpy(cmntpparm->domain_name,domain_name,strlen(domain_name));
		cmntpparm->port = port;
		cmntpparm->timeout = timeout;
		cmntpparm->update_rtc = update_rtc;
        cmntpparm->ntp_type = CMNTP;
        xy_free(domain_name);
		
		thread_attr.name	   = "query_ntp";
		thread_attr.priority   = osPriorityNormal1;
		thread_attr.stack_size = 2 * 1024;
		query_ntp_thread_id = osThreadNew((osThreadFunc_t)(query_ntp_task), cmntpparm, &thread_attr);
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
END:
	return AT_END;
}

int at_XNTP_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		struct sntp_time *ptimemap = NULL;
		char *domain_name = xy_zalloc(strlen(at_buf));
		int timeout = 0;
		int port = 0;
		uint8_t update_rtc = 1;
		int zone_sec = 0;
#if VER_GXX
		if (g_CTZU_mode != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END_PROC;
		}
#else
		if (g_NITZ_mode != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END_PROC;
		}
#endif
		if (!ps_netif_is_ok())
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto END_PROC;
		}
		if (at_parse_param("%s,%d,%1d,%d", at_buf, domain_name, &port, &update_rtc, &timeout) != AT_OK || timeout > 300)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
		if (strlen(domain_name) > REMOTE_SERVER_LEN || update_rtc != 1 || port<0 || port>65535)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto END_PROC;
		}
		if (domain_name[0] == '\0')
			ptimemap = gettimebysntp("ntp1.aliyun.com", port, timeout);
		else
			ptimemap = gettimebysntp(domain_name, port, timeout);
		if (ptimemap == NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto END_PROC;
		}
		if (g_softap_var_nv->g_zone != 0)
			zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;
		else
			zone_sec = 8 * 60 * 60;

		char zone[4] = {0};
		char positive_zone;

		if(g_softap_var_nv->g_zone >= 0)
		{
			positive_zone = g_softap_var_nv->g_zone;
			if(g_softap_var_nv->g_zone >= 10)
				sprintf(zone,"+%2d",positive_zone);
			else
				sprintf(zone,"+%1d",positive_zone);
		}
		else
		{
			positive_zone = 0 - g_softap_var_nv->g_zone;
			if(g_softap_var_nv->g_zone <= -10)
				sprintf(zone,"-%2d",positive_zone);
			else
				sprintf(zone,"-%1d",positive_zone);
		}

		uint64_t sec = ptimemap->sec;
		struct rtc_time rtctime = {0};

		xy_gmtime_r(sec * 1000, &rtctime);
		*prsp_cmd = xy_zalloc(70);
		sprintf(*prsp_cmd, "\r\n+XNTP:%02lu/%02lu/%02lu,%02lu:%02lu:%02lu%s\r\n\r\nOK\r\n", \
			rtctime.tm_year - 2000, rtctime.tm_mon, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec, zone);
		reset_universal_timer(&rtctime, zone_sec);
	END_PROC:
		xy_free(domain_name);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//switch and query fota status
int at_FOTACTR_req(char *at_buf, char **prsp_cmd)
{
	char mode = 0;
	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%1d", at_buf, &mode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (mode == 0)
		{
			//Open fota
			g_softap_fac_nv->fota_mode = 0;
		}
		else if (mode == 1)
		{
			//Close fota
			g_softap_fac_nv->fota_mode = 1;
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		SAVE_FAC_PARAM(fota_mode);	
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(30);
		sprintf(*prsp_cmd, "\r\n+FOTA:%d\r\n\r\nOK\r\n", g_softap_fac_nv->fota_mode);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

/**
 * AT+TCPIPTEST=<double_trance>,<double_trance_num>
 */
int at_TCPIPTEST_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%d,%d", at_buf, &g_double_trance, &g_double_trance_num) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+QNTP=<contextID>,<server>[,<port>[,<auto_set_time>]]
int at_QNTP_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        if (xy_wait_tcpip_ok(0) != XY_OK)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }
        if (query_ntp_thread_id != NULL)
        {
            *prsp_cmd = BC26_AT_ERR_BUILD();
            return AT_END;
        }

        int contextId = 0;
        char *server = xy_zalloc(XY_BC26_SERVER_MAX);
        uint16_t port = 123;
        uint8_t access_mode = 0;
        if (at_parse_param("%d(0-0),%150s,%2d[1-65535],%1d[0-1]", at_buf, &contextId, server, &port, &access_mode) != AT_OK)
        {
			*prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        ntp_param_t *ntpparam = xy_zalloc(sizeof(ntp_param_t));
        ntpparam->domain_name = xy_zalloc(strlen(server) + 1);
        memcpy(ntpparam->domain_name, server, strlen(server));
        ntpparam->port = port;
		ntpparam->timeout = 0;
		ntpparam->update_rtc = access_mode;
        ntpparam->ntp_type = QNTP;

        osThreadAttr_t thread_attr = {0};
		thread_attr.name	   = "query_ntp";
		thread_attr.priority   = osPriorityNormal1;
		thread_attr.stack_size = 2 * 1024;
		query_ntp_thread_id = osThreadNew((osThreadFunc_t)(query_ntp_task), ntpparam, &thread_attr);

END_PROC:
        if (server != NULL)
            xy_free(server);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_TEST)
    {
		*prsp_cmd = xy_zalloc(64);
		sprintf(*prsp_cmd, "\r\n+QNTP: (0-11),<server>,<port>,(0,1)\r\n\r\nOK\r\n");
    }
    else
    {
        *prsp_cmd = BC26_AT_ERR_BUILD();
    }
    
    return AT_END;
}

//AT+QIDNSGIP=<contextID>,<hostname>
int at_QIDNSGIP_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        int contextId = 0;
        char *domain_name = NULL;
        dns_query_info_t *qdns = NULL;
        if (xy_wait_tcpip_ok(0) != XY_OK)
		{
			*prsp_cmd = BC26_AT_ERR_BUILD();
			return AT_END;
		}

		if (query_dns_thread_id != NULL)
		{
			*prsp_cmd = BC26_AT_ERR_BUILD();
			return AT_END;
		}

        domain_name = xy_zalloc(XY_BC26_SERVER_MAX);

		if (at_parse_param("%d(0-0),%150s", at_buf, &contextId, domain_name) != AT_OK)
		{
			*prsp_cmd = BC26_AT_ERR_BUILD();
            goto END_PROC;
        }

        qdns = xy_zalloc(sizeof(dns_query_info_t));
        qdns->domain_name = xy_zalloc(strlen(domain_name) + 1);
        memcpy(qdns->domain_name, domain_name, strlen(domain_name));
        qdns->query_type = BC26DNS;
        qdns->disable_cache = 0;
        qdns->timeout = 0;

        osThreadAttr_t thread_attr = {0};
        thread_attr.name = "query_dns";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = 4 * 1024;
        query_dns_thread_id = osThreadNew((osThreadFunc_t)(query_dns_task), qdns, &thread_attr);

END_PROC:
        if (domain_name != NULL)
            xy_free(domain_name);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_TEST)
    {
		*prsp_cmd = xy_zalloc(64);
		sprintf(*prsp_cmd, "\r\n+QIDNSGIP: (0-11),<hostname>\r\n\r\nOK\r\n");
    }
    else
    {
        *prsp_cmd = BC26_AT_ERR_BUILD();
    }
    
    return AT_END;
}
