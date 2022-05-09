/**
 * @file user_netif_demo.c
 * @brief 用户处理netif事件Demo,用户需调用芯翼提供的xy_regist_psnetif_callback接口将需要监听的PS事件及回调的函数注册到芯翼内部
 * 当所监听的事件发生变化时，芯翼内部会调用用户提供的回调函数体
 * 监听的事件可以是单个也可以是多个事件组合，事件类型可以参考PsNetifStateChangeEvent定义
 * 如果在回调中有阻塞的操作，用户需创建额外的线程进行处理，在回调中将netif事件传递给该线程处理
 * 如果在回调中无阻塞操作，可以不用创建线程，直接在回调中完成。
 * @version 1.0
 * @date 2021-04-14
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 * 
 */
#include "xy_api.h"
#include "xy_net_api.h"

#if DEMO_TEST

//用户自定义ps消息结构体，用于将数据从PS回调函数中传递给用户线程
typedef struct user_psnetinfo
{
    uint32_t eventId;
} user_psnetinfo_t;

osMessageQueueId_t user_psnetif_q = NULL;


//发送消息到用户线程
void send_psnetif_event_to_user(user_psnetinfo_t * msg)
{
    xy_assert(msg != NULL);
    if (user_psnetif_q != NULL)
        osMessageQueuePut(user_psnetif_q, &msg, 0, osWaitForever);
}

/**
 * 用户线程函数体，用于处理阻塞操作
 */
void user_psnetif_process(void)
{
    void *rcv_msg = NULL;
    user_psnetinfo_t *msg = NULL;
    while (1)
	{
		osMessageQueueGet(user_psnetif_q, &rcv_msg, NULL, osWaitForever);
		if (rcv_msg == NULL)
		{
			xy_assert(0);
		}

        msg = ((user_psnetinfo_t *)rcv_msg);
        switch (msg->eventId)
        {
            case EVENT_PSNETIF_ENTER_OOS:
            {
                xy_printf("[user psnetif demo] enter oos event get");
                send_debug_str_to_at_uart("[user psnetif demo] enter oos event get\r\n");
            }
                break;
            case EVENT_PSNETIF_ENTER_IS:
            {
                xy_printf("[user psnetif demo] enter is event get");
                send_debug_str_to_at_uart("[user psnetif demo] enter is event get\r\n");
            }
                break;
            case EVENT_PSNETIF_INVALID:
            {
                xy_printf("[user psnetif demo] netif deact event get");
                send_debug_str_to_at_uart("[user psnetif demo] netif deact event get\r\n");
            }
                break;
            case EVENT_PSNETIF_IPV4_VALID:
            {
                xy_printf("[user psnetif demo] netif act event get");
                PsNetifInfo *netifInfo = (PsNetifInfo *) xy_zalloc(sizeof(PsNetifInfo));
                if (xy_get_netifInfo(netifInfo) == XY_OK)
                {
                    char *debug_info = xy_zalloc(200);
                    char *ip_addr = xy_zalloc(64);
                    char *dns_pri = xy_zalloc(64);
                    char *dns_sec = xy_zalloc(64);
                    inet_ntop(AF_INET, &netifInfo->ipv4Info.ip, ip_addr, 64);
                    inet_ntop(AF_INET, &netifInfo->ipv4Info.pri_dns, dns_pri, 64);
                    inet_ntop(AF_INET, &netifInfo->ipv4Info.sec_dns, dns_sec, 64);

                    snprintf(debug_info, 200, "cid:%d,ip_addr:%s,dns_pri:%s,dns_sec:%s\r\n",
                             netifInfo->workingCid, ip_addr, dns_pri, dns_sec);
                    send_debug_str_to_at_uart(debug_info);

                    xy_free(ip_addr);
                    xy_free(dns_pri);
                    xy_free(dns_sec);
                    xy_free(debug_info);
                }
                xy_free(netifInfo);
            }
                break;
            case EVENT_PSNETIF_DNS_CHNAGED:
            {
                xy_printf("[user psnetif demo] netif info changed event get");
                send_debug_str_to_at_uart("[user psnetif demo] netif info changed event get\r\n");
            }
                break;
            case EVENT_PSNETIF_IPV6_VALID:
            case EVENT_PSNETIF_IPV4V6_VALID:
            {
                xy_printf("[user psnetif demo] ipv6 addr prefered event get");
                send_debug_str_to_at_uart("[user psnetif demo] ipv6 addr prefered event get\r\n");
                unsigned int ipv6[4] = {0};
                char *ipv6_str = xy_zalloc(40);
                char *debug = xy_zalloc(100);
                xy_get_ip6addr(ipv6);
                inet_ntop(AF_INET6, ipv6, ipv6_str, 40);
                snprintf(debug, 100,"user get ipv6:%s\r\n",ipv6_str);
                send_debug_str_to_at_uart(debug);
                xy_free(ipv6_str);
                xy_free(debug);
            }
                break;
            default:
                break;     
        }
        xy_free(msg);
    }
}

void user_psnetif_callback(uint32_t event)
{
    user_psnetinfo_t *msg = (user_psnetinfo_t *)xy_malloc(sizeof(user_psnetinfo_t));
    msg->eventId = event;
    send_psnetif_event_to_user(msg);
}

void xy_netif_demo_init()
{
	osThreadAttr_t thread_attr = {0};

    xy_regist_psnetif_callback(EVENT_PSNETIF_ALL_MASK, user_psnetif_callback);
    //如果在回调中有阻塞的操作，用户创建额外的线程进行处理;如果在回调中无阻塞操作，可以不用创建线程，直接在回调中完成。
    user_psnetif_q = osMessageQueueNew(10, sizeof(void *), NULL);

	thread_attr.name	   = "psnetif_demo";
	thread_attr.priority   = XY_OS_PRIO_NORMAL;
	thread_attr.stack_size = 0x800;
    osThreadNew((osThreadFunc_t)(user_psnetif_process), NULL, &thread_attr);
}

#endif //DEMO_TEST
