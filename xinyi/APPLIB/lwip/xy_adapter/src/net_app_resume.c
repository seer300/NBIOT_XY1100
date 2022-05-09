#include "net_app_resume.h"
#include "oss_nv.h"
#include "xy_proxy.h"
#include "lwip/ip4.h"
#include "lwip/ip6.h"
#include "lwip/udp.h"

//#define  RESUME_ALL_CLOUD          0   //#0#下行包恢复指定云业务，匹配五元组,#1#下行包恢复所有云业务，仅匹配下行包的目的IP和本地IP
osSemaphoreId_t cdp_recovery_sem = NULL;
osMutexId_t g_cloud_resume_mutex= NULL;
osMutexId_t g_cdp_keepalive_update_mutex= NULL;
osMutexId_t g_cis_keepalive_update_mutex= NULL;

//该接口内部禁止调用softap_printf，可调用send_debug_str_to_at_uart
void save_net_app_infos(void)
{
    int ret = -1;
    app_deepsleep_infos_t *net_info = NULL;

    if(!g_softap_fac_nv->save_cloud || !sizeof(app_deepsleep_infos_t))
        return ;

    net_info = xy_zalloc(sizeof(app_deepsleep_infos_t));
    xy_ftl_read(NV_FLASH_NET_BASE, (unsigned char*)net_info, sizeof(app_deepsleep_infos_t));
   
#if AT_SOCKET            
    ret = save_udp_context_deepsleep(&(net_info->udp_context));
    SET_NET_RECOVERY_FLAG(SOCKET_TASK);
#endif

#if MOBILE_VER
    ret = onenet_write_regInfo(&(net_info->onenet_regInfo));
    SET_NET_RECOVERY_FLAG(ONENET_TASK);
#endif

#if TELECOM_VER
    ret = cdp_write_regInfo(&(net_info->cdp_regInfo));
    SET_NET_RECOVERY_FLAG(CDP_TASK);
#endif
//防止保存数据时候内存泄漏
    if(g_softap_var_nv->app_recovery_flag == 0)
    {
        if(net_info)
            xy_free(net_info);
        return ;
    }
    xy_ftl_write(NV_FLASH_NET_BASE,0,(unsigned char *)net_info, sizeof(app_deepsleep_infos_t));

    if(net_info)
        xy_free(net_info);
    return ;
}

int is_IP_changed(net_app_type_t type)
{
    ip_addr_t pre_local_ip = {0};
    ip_addr_t new_local_ip = {0};
    unsigned int offset = 0;

    if(type == ONENET_TASK)
    {
#if MOBILE_VER
    	offset = OFFSET_NETINFO_PARAM(onenet_regInfo.net_info.local_ip);
#endif
    }
    else if(type == CDP_TASK)
    {
#if TELECOM_VER
    	offset = OFFSET_NETINFO_PARAM(cdp_regInfo.net_info.local_ip);
#endif
    }
    else
        return IP_RECEIVE_ERROR;

    if(xy_ftl_read(NV_FLASH_NET_BASE+offset, (unsigned char*)&pre_local_ip, sizeof(ip_addr_t)) != XY_OK
            || (pre_local_ip.type !=IPADDR_TYPE_V4 && pre_local_ip.type !=IPADDR_TYPE_V6 ))
        return IP_RECEIVE_ERROR;

    if(xy_get_ipaddr(pre_local_ip.type,&new_local_ip) == XY_ERR)
        return IP_RECEIVE_ERROR;

    if(ip_addr_cmp(&pre_local_ip,&new_local_ip))
        return IP_NO_CHANGED;
    else
        return IP_IS_CHANGED;
}

int match_net_app_by_touple(net_app_type_t type, void *data)
{
#if RESUME_ALL_CLOUD
    return 1;
#endif
    net_infos_t  net_info;
    ip_addr_t pre_server_ip={0};
    ip_addr_t server_ip={0};
    uint16_t server_port;
    uint16_t local_port;
    uint16_t offset;
    uint8_t  protocol;
    uint8_t  ip_version;
    uint8_t  i ;
    
    ip_version = IP_HDR_GET_VERSION(data);
    if(ip_version == 0x4)
    {
        protocol = ((struct ip_hdr *)data)->_proto;
        ip_addr_copy_from_ip4(server_ip, ((struct ip_hdr *)data)->src);
        server_port = lwip_ntohs(((struct udp_hdr *)(data+IPH_HL_BYTES((struct ip_hdr *)data)))->src);
        local_port  = lwip_ntohs(((struct udp_hdr *)(data+IPH_HL_BYTES((struct ip_hdr *)data)))->dest);
    }
    else
    {
        protocol = ((struct ip6_hdr *)data)->_nexth;
        ip_addr_copy_from_ip6_packed(server_ip, ((struct ip6_hdr *)data)->src);
        server_port = lwip_ntohs(((struct udp_hdr *)(data+IP6_HLEN))->src);
        local_port  = lwip_ntohs(((struct udp_hdr *)(data+IP6_HLEN))->dest);
    }

    if(type == SOCKET_TASK)
    {
#if AT_SOCKET
        for(i = 0 ;i < SOCK_NUM ;i++)
        {
            offset = OFFSET_NETINFO_PARAM(udp_context.udp_socket[i].net_info);
            xy_ftl_read(NV_FLASH_NET_BASE+ offset,(unsigned char *)&net_info, sizeof(net_infos_t));
            if((net_info.local_port == local_port) && (protocol == IPPROTO_UDP))
                return 1;
        }
#endif
    }
    else if (type == ONENET_TASK)
    {
#if MOBILE_VER
    	offset = OFFSET_NETINFO_PARAM(onenet_regInfo.net_info);
        xy_ftl_read(NV_FLASH_NET_BASE+ offset,(unsigned char *)&net_info, sizeof(net_infos_t));
        ipaddr_aton(net_info.remote_ip, &pre_server_ip);
        if(net_info.is_dm == FALSE && net_info.remote_port == server_port && ip_addr_cmp(&pre_server_ip,&server_ip))
            return 1;
#endif
    }
    else if((type == CDP_TASK))
    {
#if TELECOM_VER
        offset = OFFSET_NETINFO_PARAM(cdp_regInfo.net_info);
        xy_ftl_read(NV_FLASH_NET_BASE+ offset,(unsigned char *)&net_info, sizeof(net_infos_t));
        ipaddr_aton(net_info.remote_ip, &pre_server_ip);
        if(net_info.remote_port == server_port && ip_addr_cmp(&pre_server_ip,&server_ip))
            return 1;
#endif
    }

    return 0;
}

#if VER_QUCTL260
int check_onenet_reginfo()
{
	int curtime = utils_gettime_s();
	if(g_onenet_regInfo->life_time > 0 && g_onenet_regInfo->last_update_time > 0)
	{
		if(g_onenet_regInfo->last_update_time + g_onenet_regInfo->life_time <= curtime)
		{
			softap_printf(USER_LOG, WARN_LOG,"[cis]resume err, last_update_time: %d, lifetime: %d, currenttime: %d\r\n",
					g_onenet_regInfo->last_update_time , g_onenet_regInfo->life_time, curtime);
			return RESUME_LIFETIME_TIMEOUT;
		}

	}
	else
		return RESUME_REG_CONFIG_ERROR;

	return XY_OK;
}
#endif

//云业务恢复接口 使用场景：1 下行恢复 2 AT命令发送上行数据 3 PDP激活
int resume_net_app(net_app_type_t type)
{
    int check_result = XY_OK;

    if(!g_softap_fac_nv->save_cloud || g_softap_fac_nv->work_mode != 0)
        return RESUME_SWITCH_INACTIVE;

    softap_printf(USER_LOG, WARN_LOG,"[resume_net_app] resume_net_app %d!\n",type);
    osMutexAcquire(g_cloud_resume_mutex, osWaitForever);

    if(!NET_NEED_RECOVERY(type))
    {
        osMutexRelease(g_cloud_resume_mutex);
        return RESUME_FLAG_ERROR;
    }

#if AT_SOCKET 
    if(type == SOCKET_TASK)
    {   
        resume_socket_app(OFFSET_NETINFO_PARAM(udp_context));
        g_softap_var_nv->app_recovery_flag&=(~(1<<SOCKET_TASK));
        osMutexRelease(g_cloud_resume_mutex);
        return RESUME_SUCCEED;
    }   
#endif

#if MOBILE_VER
    if(type == ONENET_TASK)
    {
		onenet_read_regInfo(OFFSET_NETINFO_PARAM(onenet_regInfo));

#if CIS_ENABLE_DM
		if(g_onenet_regInfo->net_info.is_dm) {
			osMutexRelease(g_cloud_resume_mutex);
			return RESUME_FLAG_ERROR;
		}
#endif

#if VER_QUCTL260
		// BC260可关闭自动update，导致存在深睡时lifetime超期的情况
		check_result = check_onenet_reginfo();
		if(check_result == XY_OK)
			onet_resume_task();
		else if(check_result == RESUME_LIFETIME_TIMEOUT)
			softap_printf(USER_LOG, WARN_LOG,"[CIS]resume lifetime timeout",type);
		else
			softap_printf(USER_LOG, WARN_LOG,"[CIS]resume reginfo err\r\n",type);
#else
		onet_resume_task();
#endif
        g_softap_var_nv->app_recovery_flag&=(~(1<<ONENET_TASK));
        osMutexRelease(g_cloud_resume_mutex);
        return check_result;
    }   
#endif

#if TELECOM_VER
    if(type == CDP_TASK)
    {
        cdp_read_regInfo(OFFSET_NETINFO_PARAM(cdp_regInfo));
		cdp_resume_task();
        g_softap_var_nv->app_recovery_flag&=(~(1<<CDP_TASK));
        osMutexRelease(g_cloud_resume_mutex);
        return check_result;
    }   
#endif               

    osMutexRelease(g_cloud_resume_mutex);
    return RESUME_OTHER_ERROR;
}

//when DRX/eDRX period,user may start other new cloud connect,so must care conflict
int  resume_net_app_by_dl_pkt(void *data, unsigned short len)
{
    unsigned char ip_version;
	unsigned char protocol;
    
    if(!g_softap_fac_nv->save_cloud || g_softap_fac_nv->work_mode != 0 || g_softap_var_nv->app_recovery_flag == 0)
        return NO_TASK;

    //从数据包中获取ip_type
    ip_version = IP_HDR_GET_VERSION(data);
	
    if(ip_version == 0x4)
    {
        protocol = ((struct ip_hdr *)data)->_proto;
    }
    else if(ip_version == 0x6)
    {
        protocol = ((struct ip6_hdr *)data)->_nexth;
    }

	if(ip_version != 0x4 && ip_version != 0x6 && protocol != IPPROTO_UDP)
    {
        return UNKNOWN_IP;
    }

    softap_printf(USER_LOG, WARN_LOG,"eDRX recv downlink ip packet, resume start!\n");

#if AT_SOCKET
    if(NET_NEED_RECOVERY(SOCKET_TASK))
    {
        if(match_net_app_by_touple(SOCKET_TASK, data))
        {   
            resume_net_app(SOCKET_TASK);
            send_debug_str_to_at_uart("+DBGINFO:[SOCKET] RECOVERY\r\n");
            return SOCKET_TASK;
        } 
    }
#endif

#if MOBILE_VER
    if(NET_NEED_RECOVERY(ONENET_TASK))
    {
        if(match_net_app_by_touple(ONENET_TASK, data))
        {   
            resume_net_app(ONENET_TASK);
            send_debug_str_to_at_uart("+DBGINFO:[CIS] RECOVERY\r\n");
            return ONENET_TASK;
        } 
    }
#endif

#if TELECOM_VER
    if(NET_NEED_RECOVERY(CDP_TASK))
    {
        if(match_net_app_by_touple(CDP_TASK, data))
        {   
            resume_net_app(CDP_TASK);      
            send_debug_str_to_at_uart("+DBGINFO:[CDP] RECOVERY\r\n");
            return CDP_TASK;
        }  
    }
#endif

    return NO_TASK;
}


void cloud_resume_init(void)
{
	osMutexAttr_t mutex_attr = {0};

    if(g_cloud_resume_mutex == NULL)
    {
    	mutex_attr.attr_bits = osMutexRecursive;
        g_cloud_resume_mutex = osMutexNew(&mutex_attr);
    }
#if TELECOM_VER
    if(g_cdp_keepalive_update_mutex == NULL)
        g_cdp_keepalive_update_mutex = osMutexNew(NULL);
    if(cdp_recovery_sem == NULL)
        cdp_recovery_sem = osSemaphoreNew(0xFFFF, 0, NULL);
#endif
#if MOBILE_VER
    if(g_cis_keepalive_update_mutex == NULL )
        g_cis_keepalive_update_mutex = osMutexNew(NULL);
#endif
}
