/**
 * @file xy_net_api.c
 * @brief 
 * @version 1.0
 * @date 2021-04-26
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 * 
 */

#include "xy_net_api.h"
#include "at_ps_cmd.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "xy_at_api.h"
#include "xy_proxy.h"
#include "xy_system.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct bc26_err_map
{
    int lwip_err;
    int bc26_err;
} bc26_err_map_t;

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
bc26_err_map_t bc26_errmap_table[] = {

    {EAI_FAIL, NET_DNS_PARSE_FAIL},
    {HOST_NOT_FOUND, NET_DNS_PARSE_FAIL},
    {EAI_NONAME, NET_PARAM_ERR},
    {EAI_SERVICE, NET_PARAM_ERR},
    {SNTP_ERR_TIMEOUT, NET_OP_TIMEOUT},
    {SNTP_ERR_DNS, NET_DNS_PARSE_FAIL},

    {0, 0} //can not delete!!!
};

static int
hex_digit_value (char ch)
{
  if ('0' <= ch && ch <= '9')
    return ch - '0';
  if ('a' <= ch && ch <= 'f')
    return ch - 'a' + 10;
  if ('A' <= ch && ch <= 'F')
    return ch - 'A' + 10;
  return -1;
}

static int
inet_pton4 (const char *src, const char *end, unsigned char *dst)
{
  int saw_digit, octets, ch;
  unsigned char tmp[4], *tp;

  saw_digit = 0;
  octets = 0;
  *(tp = tmp) = 0;
  while (src < end)
    {
      ch = *src++;
      if (ch >= '0' && ch <= '9')
        {
          unsigned int new = *tp * 10 + (ch - '0');

          if (saw_digit && *tp == 0)
            return 0;
          if (new > 255)
            return 0;
          *tp = new;
          if (! saw_digit)
            {
              if (++octets > 4)
                return 0;
              saw_digit = 1;
            }
        }
      else if (ch == '.' && saw_digit)
        {
          if (octets == 4)
            return 0;
          *++tp = 0;
          saw_digit = 0;
        }
      else
        return 0;
    }
  if (octets < 4)
    return 0;
  memcpy (dst, tmp, 4);
  return 1;
}

static int
inet_pton6 (const char *src, const char *src_endp, unsigned char *dst)
{
  unsigned char tmp[16], *tp, *endp, *colonp;
  const char *curtok;
  int ch;
  unsigned int xdigits_seen;	/* Number of hex digits since colon.  */
  unsigned int val;

  tp = memset (tmp, '\0', 16);
  endp = tp + 16;
  colonp = NULL;

  /* Leading :: requires some special handling.  */
  if (src == src_endp)
    return 0;
  if (*src == ':')
    {
      ++src;
      if (src == src_endp || *src != ':')
        return 0;
    }

  curtok = src;
  xdigits_seen = 0;
  val = 0;
  while (src < src_endp)
    {
      ch = *src++;
      int digit = hex_digit_value (ch);
      if (digit >= 0)
	{
	  if (xdigits_seen == 4)
	    return 0;
	  val <<= 4;
	  val |= digit;
	  if (val > 0xffff)
	    return 0;
	  ++xdigits_seen;
	  continue;
	}
      if (ch == ':')
	{
	  curtok = src;
	  if (xdigits_seen == 0)
	    {
	      if (colonp)
		return 0;
	      colonp = tp;
	      continue;
	    }
	  else if (src == src_endp)
            return 0;
	  if (tp + 2 > endp)
	    return 0;
	  *tp++ = (unsigned char) (val >> 8) & 0xff;
	  *tp++ = (unsigned char) val & 0xff;
	  xdigits_seen = 0;
	  val = 0;
	  continue;
	}
      if (ch == '.' && ((tp + 4) <= endp)
          && inet_pton4 (curtok, src_endp, tp) > 0)
	{
	  tp += 4;
	  xdigits_seen = 0;
	  break;  /* '\0' was seen by inet_pton4.  */
	}
      return 0;
    }
  if (xdigits_seen > 0)
    {
      if (tp + 2 > endp)
	return 0;
      *tp++ = (unsigned char) (val >> 8) & 0xff;
      *tp++ = (unsigned char) val & 0xff;
    }
  if (colonp != NULL)
    {
      /* Replace :: with zeros.  */
      if (tp == endp)
        /* :: would expand to a zero-width field.  */
        return 0;
      unsigned int n = tp - colonp;
      memmove (endp - n, colonp, n);
      memset (colonp, 0, endp - n - colonp);
      tp = endp;
    }
  if (tp != endp)
    return 0;
  memcpy (dst, tmp, 16);
  return 1;
}

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
int xy_dns_set(ip4_addr_t *pridns, ip4_addr_t *secdns)
{
	ip_addr_t pridns_t = {0};
	ip_addr_t secdns_t = {0};
	char pridns_str[IP4ADDR_STRLEN_MAX] = {0};
	char secdns_str[IP4ADDR_STRLEN_MAX] = {0};

	if (pridns->addr != 0 && secdns->addr != 0)
	{
		memcpy(&pridns_t.u_addr.ip4, pridns, sizeof(ip4_addr_t));
		pridns_t.type = IPADDR_TYPE_V4;
		memcpy(&secdns_t.u_addr.ip4, secdns, sizeof(ip4_addr_t));
		secdns_t.type = IPADDR_TYPE_V4;

		dns_setserver(0, &pridns_t);
		dns_setserver(1, &secdns_t);
        xy_set_default_dns_server(&pridns_t, 0);
        xy_set_default_dns_server(&secdns_t, 1);

        psNetifEventInd(EVENT_PSNETIF_DNS_CHNAGED);
		return XY_OK;
	}
	else if (pridns->addr != 0)
	{
		memcpy(&pridns_t.u_addr.ip4, pridns, sizeof(ip4_addr_t));
		pridns_t.type = IPADDR_TYPE_V4;

        dns_setserver(0, &pridns_t);
        xy_set_default_dns_server(&pridns_t, 0);
        
		psNetifEventInd(EVENT_PSNETIF_DNS_CHNAGED);	
		return XY_OK;
	}
	return XY_ERR;
}

int xy_dns_set2(char *pri_dns, char *sec_dns)
{
	ip4_addr_t pridns = {0};
	ip4_addr_t secdns = {0};
	if(strlen(pri_dns) != 0 || strlen(sec_dns) != 0)
	{
		if(strcmp(pri_dns,""))
			inet_pton(AF_INET, pri_dns, &pridns);
		
		if(strcmp(sec_dns,""))
			inet_pton(AF_INET, sec_dns, &secdns);
		if(xy_dns_set(&pridns,&secdns) != XY_OK)
		{
			return XY_ERR;
		}
		return XY_OK;
	}
	return XY_ERR;
}

int xy_getIP4Addr(char *ipAddr, int addrLen)
{
	if (xy_wait_tcpip_ok(0) == XY_ERR || ipAddr == NULL || addrLen < XY_IP4ADDR_STRLEN)
		return XY_ERR;

	if (inet_ntop(AF_INET, &netif_default->ip_addr.u_addr.ip4, ipAddr, addrLen) == NULL)
		return XY_ERR;

	return XY_OK;
}

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
int xy_get_ip4addr(unsigned int *ip4addr)
{
	if (xy_wait_tcpip_ok(1) == XY_ERR)
		return XY_ERR;
	
	*ip4addr = (unsigned int)(netif_default->ip_addr.u_addr.ip4.addr);
	return XY_OK;
}

int8_t xy_get_netifIpType()
{
	return g_netifIp_type;
}

int xy_getIP6Addr(char *ip6Addr, int addrLen)
{
    if (ps_netif_is_ok() == 0 || (g_netifIp_type != IPV6_TYPE && g_netifIp_type != IPV46_TYPE))
    {
        return XY_ERR;
    }
    if (ip6Addr == NULL || addrLen < XY_IP6ADDR_STRLEN)
    {
        return XY_ERR;
    }

    if (netif_ip6_addr_state(netif_default, 1) != IP6_ADDR_PREFERRED)
    {
        /* 有效的ipv6地址尚未获取，返回协议栈上报的本地链路地址 */
        if (inet_ntop(AF_INET6, &netif_ip6_addr(netif_default, 0)->addr, ip6Addr, addrLen) == NULL)
            return XY_ERR;
    }
    else
    {
        if (inet_ntop(AF_INET6, &netif_ip6_addr(netif_default, 1)->addr, ip6Addr, addrLen) == NULL)
            return XY_ERR;
    }
    softap_printf(USER_LOG, WARN_LOG, "getIP6Addr:%s", ip6Addr);
    return XY_OK;
}

int xy_get_ip6addr(unsigned int *ip6addr)
{
	if (xy_wait_tcpip_ok(1) == XY_ERR || ip6addr == NULL)
		return XY_ERR;

	if (netif_ip6_addr_state(netif_default, 1) != IP6_ADDR_PREFERRED)
		return XY_ERR;

    /* 整型ipv6地址大小为16字节 */
	memcpy(ip6addr, netif_ip6_addr(netif_default, 1)->addr, 16);
	return XY_OK;
}

int xy_get_ipaddr(uint8_t ipType, ip_addr_t *ipaddr)
{
	if (xy_wait_tcpip_ok(1) == XY_ERR || ipaddr == NULL)
		return XY_ERR;

	if (ipType == IPADDR_TYPE_V4)
	{
        ip_addr_copy(*ipaddr, netif_default->ip_addr);
	}
	else if (ipType == IPADDR_TYPE_V6)
	{
		if (netif_ip6_addr_state(netif_default, 1) != IP6_ADDR_PREFERRED)
			return XY_ERR;
        ip_addr_copy(*ipaddr, netif_default->ip6_addr[1]);
	}
	else
	{
		return XY_ERR;
	}	

	return XY_OK;
}

//该接口内部禁止调用softap_printf，因为此接口可能在云保存过程中（IDLE线程中）被调用到，可调用send_debug_str_to_at_uart接口。
int xy_wait_tcpip_ok(int timeout)
{
    int timeout_compare = 0;
	if (osKernelIsRunningIdle() == osOK && timeout > 0)
    {
        timeout = 0;
    }

    if (g_netifIp_type == NON_IP)
    {
        return XY_ERR;
    }
    
    while ((g_netifIp_type == IPV4_TYPE) ? ps_netif_is_ok() == 0 : netif_ip6_addr_state(netif_default, 1) != IP6_ADDR_PREFERRED)
    {
        if (timeout > 0 && strstr(osThreadGetName(osThreadGetId()), XY_PROXY_THREAD_NAME))
        {
            if (g_softap_fac_nv->off_debug == 0)
                xy_assert(0);
            else
            {
                send_debug_str_to_at_uart("xy_wait_tcpip_ok ERR! IN PROXY TASK");
                return XY_ERR;
            }      
        }

        if (g_check_valid_sim == 0)
        {
        	send_debug_str_to_at_uart("xy_wait_tcpip_ok ERR! NO SIM CARD");
            return  XY_ERR;
        }

        if (timeout == 0)
        {
        	send_debug_str_to_at_uart("xy_wait_tcpip_ok ERR! PDP not active");
            return XY_ERR;
        }
        else if (timeout == (int)(osWaitForever))
        {
            osDelay(100);
        }
        else
        {
            osDelay(100);
            timeout_compare += 100;
            if (timeout_compare >= timeout * 1000)
            {
            	send_debug_str_to_at_uart("xy_wait_tcpip_ok ERR! PDP not active");
                return XY_ERR;
            }
        }
    }
    return XY_OK;
}

int xy_get_netifInfo(PsNetifInfo *netifInfo)
{
	if (netifInfo == NULL || ps_netif_is_ok() == 0)
		return XY_ERR;

	netifInfo->ipType = g_netifIp_type;
	netifInfo->workingCid = g_working_cid;

	if (g_netifIp_type == IPV4_TYPE || g_netifIp_type == IPV46_TYPE)
		netifInfo->ipv4Info.ip = netif_default->ip_addr.u_addr.ip4;

	const ip_addr_t *dns_pri = dns_getserver(0);
	const ip_addr_t *dns_sec = dns_getserver(1);
	netifInfo->ipv4Info.pri_dns = dns_pri->u_addr.ip4;
	netifInfo->ipv4Info.sec_dns = dns_sec->u_addr.ip4;
	
	return XY_OK;
}

void xy_regist_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback)
{
	if (osKernelGetState() != osKernelInactive)
		osMutexAcquire(g_netif_callbacklist_mutex, osWaitForever);

	if (g_netif_callback_list == NULL)
	{
		g_netif_callback_list = (psNetifEventCallbackList *)xy_malloc(sizeof(psNetifEventCallbackList));
		g_netif_callback_list->callback = callback;
		g_netif_callback_list->eventGroup = eventGroup;
		g_netif_callback_list->next = NULL;
	}
	else
	{
		psNetifEventCallbackList *node = (psNetifEventCallbackList *)xy_malloc(sizeof(psNetifEventCallbackList));
		node->callback = callback;
		node->eventGroup = eventGroup;
		node->next = g_netif_callback_list;
		g_netif_callback_list = node;
	}
	if (osKernelGetState() != osKernelInactive)
		osMutexRelease(g_netif_callbacklist_mutex);
}

int xy_deregist_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback)
{
	psNetifEventCallbackList *prev = NULL;
	psNetifEventCallbackList *cur = NULL;
	psNetifEventCallbackList *temp = NULL;

	if(g_netif_callback_list == NULL)
		return XY_ERR;

	osMutexAcquire(g_netif_callbacklist_mutex, osWaitForever);

	if (g_netif_callback_list->eventGroup == eventGroup && g_netif_callback_list->callback == callback)
	{
		temp = g_netif_callback_list;
		g_netif_callback_list = g_netif_callback_list->next;
		xy_free(temp);
	}
	else
	{
		prev = g_netif_callback_list;
		cur =  g_netif_callback_list->next;
		while (cur != NULL)
		{
			if (cur->eventGroup == eventGroup && cur->callback == callback)
			{
				temp = cur;
				prev->next = cur->next;
				xy_free(temp);
				break;
			}
			prev = cur;
			cur = cur->next;
		}
	}
	osMutexRelease(g_netif_callbacklist_mutex);
	return XY_OK;
}

int xy_ps_is_in_oos()
{
	return g_OOS_flag;
}

int xy_ipaddr_check(char* ipaddr, uint8_t iptype)
{
    uint32_t ip_len;
    if (ipaddr == NULL || (ip_len = strlen(ipaddr)) == 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "check ip vaild but ipaddr null");
        return XY_ERR;
    }
    if (iptype != IPV4_TYPE && iptype != IPV6_TYPE)
    {
        softap_printf(USER_LOG, WARN_LOG, "check ip vaild not support type:%d", iptype);
        return XY_ERR;
    }

    if (iptype == IPV4_TYPE && ip_len >= INET_ADDRSTRLEN)
    {
        softap_printf(USER_LOG, WARN_LOG, "check ip vaild v4 addr len overlay:%d", ip_len);
        return XY_ERR;
    }
    if (iptype == IPV6_TYPE && ip_len >= INET6_ADDRSTRLEN)
    {
        softap_printf(USER_LOG, WARN_LOG, "check ip vaild v6 addr len overlay:%d", ip_len);
        return XY_ERR;
    }

    if (iptype == IPV4_TYPE)
    {
        struct sockaddr_in addr;
        if (inet_pton4(ipaddr, ipaddr + ip_len, &addr.sin_addr) != 1)
        {
            softap_printf(USER_LOG, WARN_LOG, "check ip vaild v4 addr[%s] invalid", ipaddr);
            return XY_ERR;
        }
    }
    else
    {
        struct sockaddr_in6 addr;
        if (inet_pton6(ipaddr, ipaddr + ip_len, &addr.sin6_addr) != 1)
        {
            softap_printf(USER_LOG, WARN_LOG, "check ip vaild v6 addr[%s] invalid", ipaddr);
            return XY_ERR;
        }
    }
    softap_printf(USER_LOG, WARN_LOG, "check ip vaild v6 addr[%s] valid", ipaddr);
    return XY_OK;
}

int get_bc26_tcpip_errcode(int lwip_err)
{
    int i = 0;
    for (; bc26_errmap_table[i].lwip_err != 0; i++)
    {
        if (bc26_errmap_table[i].lwip_err == lwip_err)
        {
            return bc26_errmap_table[i].bc26_err;
        }
    }

    return 0;
}

int32_t xy_socket_by_host(char *hostname, uint16_t port, int32_t proto, ip_addr_t* remote_addr)
{
    if (hostname == NULL || strlen(hostname) == 0 || port < 0 || port > 65535)
    {
        return -1;
    }

    if (proto != IPPROTO_UDP && proto != IPPROTO_TCP)
    {
        return -1;
    }

    int32_t fd;
    struct addrinfo hint = {0};
    struct addrinfo *result = NULL;
    hint.ai_protocol = proto;
    if (proto == IPPROTO_UDP)
    {
        hint.ai_socktype = SOCK_DGRAM;
    }
    else
    {
        hint.ai_socktype = SOCK_STREAM;
    }
    hint.ai_family = AF_UNSPEC;
    uint8_t strPort[7] = {0};
    snprintf(strPort, sizeof(strPort) - 1, "%d", port);

    if (getaddrinfo(hostname, strPort, &hint, &result) != 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "xy_get_sockaddr getaddrinfo err:%d", errno);
        return -1;
    }

    if (remote_addr != NULL)
    {
        if (result->ai_family == AF_INET6)
        {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)result->ai_addr;
            remote_addr->type = IPADDR_TYPE_V6;
            memcpy(remote_addr->u_addr.ip6.addr, addr6->sin6_addr.s6_addr, sizeof(addr6->sin6_addr.s6_addr));
            remote_addr->u_addr.ip6.zone = 0;
        }
        else
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
            remote_addr->type = IPADDR_TYPE_V4;
            remote_addr->u_addr.ip4.addr = addr->sin_addr.s_addr;
        }
    }

    if ((fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
    {
        softap_printf(USER_LOG, WARN_LOG, "xy_get_sockaddr socket create err:%d", errno);
        freeaddrinfo(result);
        return -1;
    }

    freeaddrinfo(result);
    return fd;
}

int xy_get_socket_local_info(int32_t fd, ip_addr_t* local_ipaddr, uint16_t* local_port)
{
    if ((local_ipaddr == NULL && local_port == NULL) || ps_netif_is_ok() == 0)
        return XY_ERR;
    struct sockaddr_storage local_addr;
    int sockaddr_len = sizeof(struct sockaddr_storage);
    if (getsockname(fd, (struct sockaddr *)&local_addr, &sockaddr_len) != 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "get sock local addr fail:%d", errno);
        return XY_ERR;
    }

    if (local_addr.ss_family == AF_INET)
    {
		if (local_ipaddr != NULL)
		{
			inet_addr_to_ip4addr(ip_2_ip4(local_ipaddr), &(((struct sockaddr_in *)&local_addr)->sin_addr));
			local_ipaddr->type = IPADDR_TYPE_V4;
		}
        if (local_port != NULL)
			*local_port = ntohs(((struct sockaddr_in *)&local_addr)->sin_port);
	}
    else if (local_addr.ss_family == AF_INET6)
    {
		if (local_ipaddr != NULL)
		{
			inet6_addr_to_ip6addr(ip_2_ip6(local_ipaddr), &(((struct sockaddr_in6 *)&local_addr)->sin6_addr));
			local_ipaddr->type = IPADDR_TYPE_V6;
		}
		if (local_port != NULL)
			*local_port = ntohs(((struct sockaddr_in6 *)&local_addr)->sin6_port);
	}
    else
    {
        return XY_ERR;
    }
    return XY_OK;
}

int xy_set_dns_server(char *dns_addr, int index)
{
	if (index < 0 || index >= DNS_MAX_SERVERS || dns_addr == NULL || strlen(dns_addr) == 0)
	{
		return XY_ERR;
	}

	/* 检测字符串型ip地址数据的有效性，防止非法输入 */
	if (xy_ipaddr_check(dns_addr, IPV4_TYPE) != XY_OK && xy_ipaddr_check(dns_addr, IPV6_TYPE) != XY_OK)
	{
		return XY_ERR;
	}

	ip_addr_t dns_t = {0};
	if (ipaddr_aton(dns_addr, &dns_t) == 0)
		return XY_ERR;
	else
		return xy_set_dns_server_by_ipaddr(&dns_t, index);
}

int xy_set_dns_server_by_ipaddr(ip_addr_t *ip_addr, int index)
{
	if (index < 0 || index >= DNS_MAX_SERVERS || ip_addr == NULL)
	{
		return XY_ERR;
	}

	dns_setserver(index, ip_addr);
	psNetifEventInd(EVENT_PSNETIF_DNS_CHNAGED);
	return XY_OK;
}

int xy_get_dns_server(int index, char* ipaddr)
{
	if (index >= DNS_MAX_SERVERS || ipaddr == NULL)
	{
		return XY_ERR;
	}

	ip_addr_t addr = {0};
	if (xy_get_dns_server_by_ipaddr(index, &addr) != XY_OK)
	{
		return XY_ERR;
	}
	if (ipaddr_ntoa_r(&addr, ipaddr, XY_IPADDR_STRLEN_MAX) == NULL)
	{
		return XY_ERR;
	}
	return XY_OK;
}

int xy_get_dns_server_by_ipaddr(int index, ip_addr_t *ipaddr)
{
	if (index >= DNS_MAX_SERVERS || ipaddr == NULL)
	{
		return XY_ERR;
	}

	*ipaddr = *((ip_addr_t *)dns_getserver(index));
	return XY_OK;
}

int xy_set_default_dns_server2(char *ipaddr, uint8_t isSec)
{
    if (ipaddr == NULL || strlen(ipaddr) == 0)
    {
        return XY_ERR;
    }

    /* 检测字符串型ip地址数据的有效性，防止非法输入 */
    if (xy_ipaddr_check(ipaddr, IPV4_TYPE) != XY_OK && xy_ipaddr_check(ipaddr, IPV6_TYPE) != XY_OK)
    {
        return XY_ERR;
    }

    ip_addr_t dns_t = {0};
	if (ipaddr_aton(ipaddr, &dns_t) == 0)
		return XY_ERR;
	else
		return xy_set_default_dns_server(&dns_t, isSec);
}

int xy_set_default_dns_server(ip_addr_t* ipaddr, uint8_t isSec)
{
    if (ipaddr == NULL)
    {
        return XY_ERR;
    }
    if (ipaddr->type == IPADDR_TYPE_V4)
    {
        if (isSec)
        {
            g_softap_fac_nv->ref_secdns4 = ipaddr->u_addr.ip4.addr;
            SAVE_FAC_PARAM(ref_secdns4);
        }
        else
        {
            g_softap_fac_nv->ref_pridns4 = ipaddr->u_addr.ip4.addr;
            SAVE_FAC_PARAM(ref_pridns4);
        }
    }
    else if (ipaddr->type == IPADDR_TYPE_V6)
    {
        if (isSec)
        {
            memcpy(g_softap_fac_nv->ref_secdns6, ipaddr->u_addr.ip6.addr, sizeof(g_softap_fac_nv->ref_secdns6));
            SAVE_FAC_PARAM(ref_secdns6);
        }
        else
        {
            memcpy(g_softap_fac_nv->ref_pridns6, ipaddr->u_addr.ip6.addr, sizeof(g_softap_fac_nv->ref_pridns6));
            SAVE_FAC_PARAM(ref_pridns6);
        }
    }
    else
    {
        return XY_ERR;
    } 
}

int xy_get_default_dns_server(ip_addr_t* ipaddr, uint8_t iptype, uint8_t isSec)
{
    if (ipaddr == NULL || (iptype != IPADDR_TYPE_V6 && iptype != IPADDR_TYPE_V4))
    {
        return XY_ERR;
    }

    if (iptype == IPADDR_TYPE_V6)
    {
        if (isSec)
        {
            memcpy(ipaddr->u_addr.ip6.addr, g_softap_fac_nv->ref_secdns6, sizeof(g_softap_fac_nv->ref_secdns6));
        }
        else
        {
            memcpy(ipaddr->u_addr.ip6.addr, g_softap_fac_nv->ref_pridns6, sizeof(g_softap_fac_nv->ref_pridns6));
        }
    }
    else if (iptype == IPADDR_TYPE_V4)
    {
        if (isSec)
        {
            ipaddr->u_addr.ip4.addr = g_softap_fac_nv->ref_secdns4;
        }
        else
        {
            ipaddr->u_addr.ip4.addr = g_softap_fac_nv->ref_pridns4;
        }
    }

    ipaddr->type = iptype;
    return XY_OK;
}
