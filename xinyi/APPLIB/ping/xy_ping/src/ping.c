#include "softap_macro.h"
#if XY_PING

#include "at_ps_cmd.h"
#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "lwip/ip6.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "net_utils.h"
#include "oss_nv.h"
#include "ping.h"
#include "ps_netif_api.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "xy_net_api.h"

#define EVENT_PSNETIF_PING (EVENT_PSNETIF_IPV4_VALID | EVENT_PSNETIF_INVALID)
int now_in_ping = 0;

ping_info_t ping_info;
extern ping_info_t ping6_info;

typedef struct
{
    char *data;
    int len;
}  ping_packet_t;

u16_t identifier;
u16_t seqno;
int g_ping_stop;
osMessageQueueId_t ping_packet_q = NULL;
osThreadId_t at_ping_thread_id = NULL;

ip4_addr_t* getlocalip()
{
	struct netif *ps_eth = find_active_netif();
	if(ps_eth == NULL)
		return NULL;
    return (ip4_addr_t *)(&ps_eth->ip_addr);
}

int send_to_ps_directly(char *data, int len,int rai_val)
{
    struct pbuf *p;
    int ret = 0;

    p = (struct pbuf*)xy_zalloc(sizeof(struct pbuf));
    
    p->payload = data;
    p->len = len;
    p->tot_len = len;
    p->soc_id = 0;
	g_udp_send_rai[0] = rai_val;

    softap_printf(USER_LOG, WARN_LOG, "ping pkt send rai:%d", rai_val);

    if (send_ip_packet_to_ps_net(netif_default, p) < 0)
    {
        ret = -1;
    }
    if (p != NULL)
    {
        xy_free(p);
    }

    return ret;
}

int send_packet_to_ping(char *data, int len)
{
    ping_packet_t *ping_packet = NULL;

    ping_packet = (ping_packet_t*)xy_zalloc(sizeof(ping_packet_t));

    ping_packet->data = data;

    ping_packet->len = len;

	if(ping_packet_q == NULL)
		goto failed;
	
    osMessageQueuePut(ping_packet_q, &ping_packet, 0, osWaitForever);

    return XY_OK;
failed:
	if (ping_packet->data != NULL)
    {
        xy_free(ping_packet->data);
    }

    if (ping_packet != NULL)
    {
        xy_free(ping_packet);
    }

    return XY_ERR;
}

int recv_from_ps_directly(char *data, int timeout_in_ms,int malloc_len)
{
    ping_packet_t *ping_packet = NULL;
    int len; 

    if (osMessageQueueGet(ping_packet_q, &ping_packet, NULL, timeout_in_ms) != osOK)
    {
        softap_printf(USER_LOG, WARN_LOG, "queue read timeout, so Ping timeout");
		char *rsp_cmd = (char*)xy_zalloc(32);
	    snprintf(rsp_cmd, 32, "\r\nPing timeout\r\n");
	    send_rsp_str_to_ext(rsp_cmd);
	    xy_free(rsp_cmd);
        return -1;
    }

    len = ping_packet->len;
    memcpy(data, ping_packet->data, malloc_len);
    if (ping_packet->data != NULL)
    {
        xy_free(ping_packet->data);
    }

    if (ping_packet != NULL)
    {
        xy_free(ping_packet);
    }
    return len;
}

int is_expected_ping_response(char *packet, int packet_len, ip4_addr_t *ip_dest, u8_t *reply_ttl, uint8_t* first_recv_flag)
{
    struct ip_hdr *iphdr = NULL;
    struct icmp_echo_hdr *icmpechohdr = NULL;
    ip4_addr_p_t src_origin, dst_origin;
    int ip_hdr_len = 0;
	ip4_addr_t *ip_local;

    iphdr = (struct ip_hdr*)packet;
    if ((((iphdr->_v_hl >> 4) & 0x0F) != 4) || (iphdr->_proto != IPPROTO_ICMP) || (ntohs(iphdr->_len) != packet_len) || g_ping_stop || ((ip_local = getlocalip()) == NULL))
    {
        return 0;
    }

    memcpy(&src_origin, ip_local, sizeof(ip4_addr_t));
    memcpy(&dst_origin, ip_dest, sizeof(ip4_addr_t));

    if (src_origin.addr != iphdr->dest.addr || dst_origin.addr != iphdr->src.addr)
    {
        return 0;
    }

    ip_hdr_len = (iphdr->_v_hl & 0x0F) * 4;

    if (*first_recv_flag == 0)
    {
        icmpechohdr = (struct icmp_echo_hdr*)(packet + ip_hdr_len);
        /* icmp头部检测 */
        if (((lwip_ntohs(IPH_OFFSET(iphdr)) & IP_OFFMASK) == 0)
             && (icmpechohdr->type != ICMP_ER || icmpechohdr->code != 0 || icmpechohdr->id != net_tool_htons(identifier) || icmpechohdr->seqno != net_tool_htons(seqno)))
        {
            softap_printf(USER_LOG, WARN_LOG, "icmp hdr check fail");
            return 0;
        }

        if ((lwip_ntohs(IPH_OFFSET(iphdr)) & IP_OFFMASK) != 0)
        {
            softap_printf(USER_LOG, WARN_LOG, "NB first recv icmp non-first frag");
            *reply_ttl = iphdr->_ttl;
            return packet_len - ip_hdr_len;
        }

        /* 检测ip包是否分片 */
        if ((IPH_OFFSET(iphdr) & PP_HTONS(IP_MF)) == 0)
        {
            softap_printf(USER_LOG, WARN_LOG, "no fragment");       
        }
        else
        {
            softap_printf(USER_LOG, WARN_LOG, "more fragment first");
            /* 标记收到第一个分片 */
            *first_recv_flag = 1;
        }
        *reply_ttl = iphdr->_ttl;
        return packet_len - ip_hdr_len - (int)(sizeof(struct icmp_echo_hdr));
    }
    else
    {
        softap_printf(USER_LOG, WARN_LOG, "more fragment");
        *reply_ttl = iphdr->_ttl;
        return packet_len - ip_hdr_len;
    } 
}

void init_ping_ipv4()
{
    identifier = 0;
    seqno = osKernelGetTickCount();
    memset(&ping_info, 0, sizeof(ping_info_t));

    g_ping_stop = 0;

    if (ping_packet_q != NULL)
    {
        ping_packet_t* elem = NULL;
		while (osMessageQueueGet(ping_packet_q, &elem, NULL, 0) == osOK)
		{
			if (elem->data != NULL)
		    {
		        xy_free(elem->data);
		    }
			if (elem != NULL)
		    {
		        xy_free(elem);
		    }
		}
    }

	if (ping_packet_q == NULL)
        ping_packet_q = osMessageQueueNew(20, sizeof(void *), NULL);
}

void ping_netif_event_callback(PsNetifStateChangeEvent event)
{
    if (event == EVENT_PSNETIF_IPV4_VALID)
    {
		g_ping_stop = 0;
        softap_printf(USER_LOG, WARN_LOG, "ping_netif_event_callback, netif up");
    }
    else
    {
		g_ping_stop = 1;
        osSemaphoreRelease(g_out_OOS_sem); //put semaphore when netif down
        softap_printf(USER_LOG, WARN_LOG, "ping_netif_event_callback, netif down");
    }
}

int _ping_ipv4(char *host_ip, uint16_t data_len, uint8_t timeout, uint32_t ping_num, int interval_time, int rai_val)
{
	ip4_addr_t ip_dest;
	int ret = 0;
    int i;
	char *rsp_cmd = NULL;
    char *buffer = NULL;
    int ip_packet_len = 0;
    char *recv_data = NULL;
    int recv_len = 0;
	int malloc_len=0;
    uint32_t start_time, end_time, rtt, total_rtt;
    uint8_t reply_ttl;
	uint8_t is_timedout = 0; 
    struct ip_hdr *iphdr = NULL;
    struct icmp_echo_hdr *icmpechohdr = NULL;
    char *icmp_data = NULL;
	int rcv_timeout = 0;
	ip4_addr_t *ip_local;
    struct addrinfo *result = NULL;
    struct addrinfo hint = {0};
    int getaddr_ret = 0;

    //域名解析获取远端ip
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = 0;
    if ((getaddr_ret = getaddrinfo(host_ip, NULL, &hint, &result)) != 0)
    {
        send_rsp_str_to_ext("\r\nget ipv4 dst addr failed and end nping\r\n");
        softap_printf(USER_LOG, WARN_LOG, "get ipv4 dst addr failed and result_dns:%d\n", getaddr_ret);
        return XY_ERR;
    }

    xy_regist_psnetif_callback(EVENT_PSNETIF_PING, ping_netif_event_callback);

    memcpy(&ip_dest.addr, &(((struct sockaddr_in *)(result->ai_addr))->sin_addr.s_addr), sizeof(struct in_addr));
	freeaddrinfo(result);

    total_rtt = 0;
    init_ping_ipv4();

    ip_packet_len = sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr) + data_len;
    buffer = (char*)xy_zalloc(ip_packet_len);

	malloc_len = sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr);
    recv_data = (char*)xy_zalloc(malloc_len);

    if ((ip_local = getlocalip()) == NULL)
    {
        softap_printf(USER_LOG, WARN_LOG, "net is null");
        goto out;
    }

    //ip header填充
    iphdr = (struct ip_hdr*)buffer;
    iphdr->_v_hl = 0x45;
    iphdr->_tos = 0x00;
    iphdr->_len = net_tool_htons(ip_packet_len);
    iphdr->_id = 0;
    iphdr->_ttl = 64;
    iphdr->_proto = IPPROTO_ICMP;
    iphdr->_chksum = 0; // later
    memcpy(&iphdr->src, ip_local, sizeof(ip4_addr_t));
    memcpy(&iphdr->dest, &ip_dest, sizeof(ip4_addr_t));
    // softap_printf(USER_LOG, WARN_LOG, "dest addr = 0x%X\n", iphdr->dest.addr);
    iphdr->_chksum = net_tool_htons(net_tool_standard_chksum(iphdr, sizeof(struct ip_hdr)));

    //icmp header填充
    icmpechohdr = (struct icmp_echo_hdr*)(buffer + sizeof(struct ip_hdr));
    icmpechohdr->type = ICMP_ECHO;
    icmpechohdr->code = 0;
    icmpechohdr->chksum = 0; // later
    icmpechohdr->id = net_tool_htons(identifier);

    //填充数据
    icmp_data = buffer + sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr);
    for (i = 0; i < data_len; i++)
    {
        icmp_data[i] = i + 'a';
    }

    softap_printf(USER_LOG, WARN_LOG, "start send ping\n");
	softap_printf(USER_LOG, WARN_LOG, "\nping ipv4 datalen:%d, timeout:%d, interval_time:%d\n", data_len, timeout, interval_time);

    now_in_ping = 1;

    while (!g_ping_stop && ping_num--)
    {
        icmpechohdr->seqno = net_tool_htons(seqno);
        icmpechohdr->chksum = 0;
        icmpechohdr->chksum = net_tool_htons(net_tool_standard_chksum(icmpechohdr, sizeof(struct icmp_echo_hdr) + data_len));

        is_timedout = 0;
        
        if(g_OOS_flag == 1)
        {
			osSemaphoreAcquire(g_out_OOS_sem, osWaitForever);
			if(g_ping_stop == 1) //if OOS -> cfun0->cfun1->is,but cfun1 no cgev,will break ping pro
			{
				softap_printf(USER_LOG, WARN_LOG, "report OOS and exit ping\n");
				break;
			}
        }
		
		start_time = osKernelGetTickCount();

        if ((ip_local = getlocalip()) == NULL)
        {
			softap_printf(USER_LOG, WARN_LOG, "net is null");
			break;
		}
        if (memcmp(&iphdr->src, ip_local, sizeof(ip4_addr_t)) != 0)
        {
            memcpy(&iphdr->src, ip_local, sizeof(ip4_addr_t));
            iphdr->_chksum = 0;
            iphdr->_chksum = net_tool_htons(net_tool_standard_chksum(iphdr, sizeof(struct ip_hdr)));
        }
        send_to_ps_directly(buffer, ip_packet_len, rai_val);

        ping_info.ping_send_num++;

		rcv_timeout = timeout * 1000;
        uint8_t first_recv_flag = 0;
        int reply_size = 0;
        while(1)
        {
            if (getlocalip() == NULL)
                break;
			
            recv_len = recv_from_ps_directly(recv_data, rcv_timeout, malloc_len);
			
            if (recv_len > 0)
            {
                reply_size += is_expected_ping_response(recv_data, recv_len, &ip_dest, &reply_ttl, &first_recv_flag);
                if (reply_size == data_len)
                {
                    ping_info.ping_reply_num++;
                    end_time = osKernelGetTickCount();
                    rtt = end_time - start_time;
                    if (ping_info.longest_rtt == 0)
                    {
                        ping_info.longest_rtt = rtt;
                    }
                    else if ((uint32_t)(ping_info.longest_rtt) < rtt)
                    {
                        ping_info.longest_rtt = rtt;
                    }

                    if (ping_info.shortest_rtt == 0)
                    {
                        ping_info.shortest_rtt = rtt;
                    }
                    else if ((uint32_t)(ping_info.shortest_rtt) > rtt)
                    {
                        ping_info.shortest_rtt = rtt;
                    }
                    total_rtt += rtt;
                    ping_info.time_average = total_rtt / ping_info.ping_reply_num;
                    softap_printf(USER_LOG, WARN_LOG, "reply from %s, %d bytes %d ms %d ttl\n", host_ip, data_len, rtt, reply_ttl);
                    break;
                }
            }
			else if (recv_len == -1)
            {
                softap_printf(USER_LOG, WARN_LOG, "ping timeout and break");
				is_timedout = 1;
                break;
            }
			end_time = osKernelGetTickCount();
            if (end_time - start_time < timeout * 1000)
            {
                rcv_timeout = timeout * 1000 - (end_time - start_time);
            }
        }
		
        if (ping_num == 0)
            break;

        end_time = osKernelGetTickCount();

        if (end_time - start_time < (uint32_t)(interval_time * 1000))
        {
            osDelay(interval_time * 1000 - (end_time - start_time));
        }
        
        /* 入网入库版本超时delay 60秒 */
        if (is_timedout && CHECK_SDK_TYPE(OPERATION_VER))
        {
            osDelay(60 * 1000);
        }

        seqno++;
    }
out:    
	rsp_cmd = (char*)xy_zalloc(128);
#if VER_QUECTEL
	sprintf(rsp_cmd, "\r\n+NPING:statistics: ping num:%d, reply:%d, longest_rtt:%dms, shortest_rtt:%dms, average_time:%dms\r\n", 
			ping_info.ping_send_num, ping_info.ping_reply_num, ping_info.longest_rtt, ping_info.shortest_rtt, ping_info.time_average);
#else
	sprintf(rsp_cmd, "\r\nstatistics: ping num:%d, reply:%d, longest_rtt:%dms, shortest_rtt:%dms, average_time:%dms\r\n", 
        ping_info.ping_send_num, ping_info.ping_reply_num, ping_info.longest_rtt, ping_info.shortest_rtt, ping_info.time_average);
#endif
    send_rsp_str_to_ext(rsp_cmd);
	softap_printf(USER_LOG, WARN_LOG, "end ping pro\r\n");

    if (buffer != NULL)
        xy_free(buffer);
    if (recv_data != NULL)
        xy_free(recv_data);
	if (rsp_cmd != NULL)
        xy_free(rsp_cmd);

	xy_deregist_psnetif_callback(EVENT_PSNETIF_PING, ping_netif_event_callback);

	ping_packet_t *ping_packet = NULL;

	while(osMessageQueueGet(ping_packet_q, &ping_packet, NULL, 0) == osOK)
	{
		if (ping_packet->data != NULL)
		{
			xy_free(ping_packet->data);
		}
		if (ping_packet != NULL)
		{
			xy_free(ping_packet);
		}
	}

	if(ping_packet_q != NULL)
    {
        xy_queue_Delete(ping_packet_q);
        ping_packet_q = NULL;
    }
    now_in_ping = 0;

	return ret;
}

void process_ping(ping_arguments_t *ping_arguments)
{
    uint16_t ip_type = ping_arguments->ip_type;

	if (ip_type != AF_INET && ip_type != AF_INET6) 
    {
        at_ping_thread_id = NULL;
        osThreadExit();
		return;
	}

    if (CHECK_SDK_TYPE(OPERATION_VER))
    {
        //入网入库版本需要申请worklock锁
        xy_work_lock(0);
	}

    if (ip_type == AF_INET)
        _ping_ipv4(ping_arguments->host, ping_arguments->data_len, ping_arguments->time_out, ping_arguments->ping_num, ping_arguments->interval_time,ping_arguments->rai_val);
#if LWIP_IPV6
	else if(ip_type == AF_INET6)
		_ping_ipv6(ping_arguments->host, ping_arguments->data_len, ping_arguments->time_out, ping_arguments->ping_num, ping_arguments->interval_time,ping_arguments->rai_val);
#endif
	xy_free(ping_arguments);

    if (CHECK_SDK_TYPE(OPERATION_VER))
    {
        //入网入库版本需要释放worklock锁
        xy_work_unlock(0);
	}

    at_ping_thread_id = NULL;
    osThreadExit();
    return;
}

int start_ping(int ip_type, char *host_ip, int data_len, int ping_num, int time_out, int interval_time, int rai_val)
{
    ping_arguments_t *ping_arguments = NULL;
	osThreadAttr_t thread_attr = {0};

    if (at_ping_thread_id != NULL)
    {
        if (g_ping_stop == 1)
        {
            softap_printf(USER_LOG, WARN_LOG, "ping is stopping, please wait!");
        }
        else
        {
            softap_printf(USER_LOG, WARN_LOG, "error, ping is in progress, please wait until ping finished!");
        }
        return XY_ERR;
    }

    ping_arguments = (ping_arguments_t *)xy_zalloc(sizeof(ping_arguments_t));

    strcpy(ping_arguments->host, host_ip);
    ping_arguments->data_len = data_len;
    ping_arguments->ping_num = ping_num;
    ping_arguments->time_out = time_out;
    ping_arguments->interval_time = interval_time;
	ping_arguments->ip_type = ip_type;
	ping_arguments->rai_val = rai_val;

    if (ip_type == AF_INET)
    {
		thread_attr.name	   = "ping_thd";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = PING_THREAD_STACK_SIZE;
        at_ping_thread_id = osThreadNew((osThreadFunc_t)(process_ping), ping_arguments, &thread_attr);
    }
    else if (ip_type == AF_INET6)
    {
		thread_attr.name	   = "ping6_thd";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = PING6_THREAD_STACK_SIZE;
        at_ping_thread_id = osThreadNew((osThreadFunc_t)(process_ping), ping_arguments, &thread_attr);
    }

    return XY_OK;
}

int stop_ping()
{
    if (at_ping_thread_id == NULL)
    {
        return 0;
    }
    g_ping_stop = 1;
	if(g_OOS_flag == 1) //put semaphore when stop ping
		osSemaphoreRelease(g_out_OOS_sem);
	return 1;
}

int ping_opencpu_api(int ip_type, char *host_ip, uint16_t data_len, uint8_t timeout, uint32_t ping_num, int interval_time, ping_info_t *ppinginfo)
{
	int result = XY_ERR;
	
	if (xy_wait_tcpip_ok(0) != XY_OK)
	{
		softap_printf(USER_LOG, WARN_LOG, "ps netif is not ok!");
		result = XY_ERR;
		goto END;
	}
    if (g_OOS_flag == 1)
    {
		softap_printf(USER_LOG, WARN_LOG, "ps netif is OOS!");
		result = XY_ERR;
		goto END;
	}

    if (ip_type != 0 && ip_type != 1)
    {
		softap_printf(USER_LOG, WARN_LOG, "param invalid");
		xy_assert(0);
		result = XY_ERR;
		goto END;
	}
	if(at_ping_thread_id != NULL)
	{
		softap_printf(USER_LOG, WARN_LOG, "Multithreaded call");
		result = XY_ERR;
		goto END;
	}

	if(ip_type == 0)
	{
		if(_ping_ipv4(host_ip, data_len, timeout, ping_num, interval_time,0) < 0)
		{
			result = XY_ERR;
		}
		else
		{
			result = XY_OK;
			if(ppinginfo != NULL)
				memcpy(ppinginfo,&ping_info,sizeof(ping_info_t));
		}
	}
	else
	{
		if(_ping_ipv6(host_ip, data_len, timeout, ping_num, interval_time,0) < 0)
		{
			result = XY_ERR;
			softap_printf(USER_LOG, WARN_LOG, "\r\nsendto failed and again!!!\r\n");
		}
		else
		{
			result = XY_OK;
			if(ppinginfo != NULL)
				memcpy(ppinginfo,&ping6_info,sizeof(ping_info_t));
		}
	}
	
END:
	return result;
}

#endif
