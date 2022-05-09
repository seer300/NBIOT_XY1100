#pragma once

/*******************************************************************************
 *						      Include header files							   *
 ******************************************************************************/
#include "lwip/priv/sockets_priv.h"
#include "xy_net_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define ADD_NONIP_PROC_FUNC(uplink_func) (g_nonip_func = uplink_func)
#define RMV_NONIP_PROC_FUNC(uplink_func) \
	do                                   \
	{                                    \
		if (g_nonip_func == uplink_func) \
			g_nonip_func = NULL;         \
	} while (0)

#define INVALID_CID 0xFF

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
 * @brief activation information specific to the PS network, which cid used to identify different PS network port
 */
struct pdp_active_info
{
	unsigned char       cid;
	unsigned char       ip46flag;
	ip4_addr_t          ip4_addr;
#if LWIP_IPV6
	ip6_addr_t          ip6_addr;
#endif
};

/**
 * @brief 
 */
struct ps_netif
{
	unsigned char cid;  
	struct netif *ps_eth;
};

/**
 * @brief
 */
struct ipdata_info
{
	unsigned char cid;
	unsigned char rai;
	unsigned char data_type;
	unsigned char padding;
	unsigned short sequence;
	unsigned short data_len;
	void *data;
};

typedef struct psNetifEventCallbackList_T
{
	uint32_t eventGroup;
	psNetifEventCallback_t callback;
	struct psNetifEventCallbackList_T *next;
} psNetifEventCallbackList;

/**
 * @brief 
 */
typedef int (*nonip_uplink_func)(void *data, int len);

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern nonip_uplink_func g_nonip_func;
extern int now_in_ping;
extern unsigned short g_udp_send_seq[NUM_SOCKETS];
extern unsigned char g_udp_send_rai[NUM_SOCKETS];
extern osMutexId_t g_udp_send_m;
extern psNetifEventCallbackList *g_netif_callback_list;
extern char g_OOS_flag;
extern osMutexId_t g_netif_callbacklist_mutex;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
void send_packet_to_user(unsigned char cid, int len, char *data);

int send_nonip_packet_to_ps(char *data, int data_len, int rai);

/**
 * @brief 查找已经激活的网口
 * @return 返回已激活的网口，netif结构为lwip标准定义
 * @note
 */
struct netif* find_active_netif();

/**
 * @brief PDP激活处理
 * @param pdp_info 参考pdp_active_info
 * @note
 */
void ps_netif_activate(struct pdp_active_info *pdp_info);

/**
 * @brief PDP去激活处理
 * @param cid pdp链路ID
 * @param ip46flag IP类型，可以参考IP_ADDR_TYPE定义
 * @note
 */
int ps_netif_deactivate(unsigned char cid, unsigned short ip46flag);

/**
 * @brief 查询netif是否启动完成，返回1表示网口已启动，返回0表示未启动
 * @note
 */
int ps_netif_is_ok();

void acquire_udp_send_mutex(void);

void release_udp_send_mutex(void);

/**
 * @brief 查询对应cid的netif是否激活
 */
bool is_netif_active(unsigned char cid);

/**
 * @brief 执行psNetif事件回调
 * @param Netif状态事件
 * @note
 */
void psNetifEventInd(PsNetifStateChangeEvent event);

bool ipv6_recovery_from_bakmem(ip6_addr_t *ip6_addr);

int save_ipv6_to_bakmem();

/**
 * @brief  仅内部，不推荐用户使用。以地址字符串方式设置DNS地址,支持ipv4 ipv6 dns设置
 * @ingroup tcpipapi
 * @param dns_addr [IN] dns服务器地址,例如"114.114.114.114"
 * @param index [IN] dns索引，索引范围由lwip DNS_MAX_SERVERS宏配置决定，目前取值范围是[0,3]
 * @return XY_OK:表示操作成功，XY_ERR:表示操作失败 
 * @note   输入的IP地址为点分文本的IP地址，内部通过ipaddr_aton接口转换为二进制网络字节序的IP地址
 */
int xy_set_dns_server(char *dns_addr, int index);

/**
 * @brief  仅内部，不推荐用户使用，以ip_addr_t类型设置DNS地址,支持ipv4 ipv6 dns设置
 * @ingroup tcpipapi
 * @param dns_addr [IN] dns服务器地址
 * @param index [IN] dns索引，索引范围由lwip DNS_MAX_SERVERS宏配置决定，目前取值范围是[0,3]
 * @return XY_OK:表示操作成功，XY_ERR:表示操作失败 
 */
int xy_set_dns_server_by_ipaddr(ip_addr_t *dns_addr, int index);

/**
 * @brief 根据索引获取dns服务器的字符串型地址
 * @ingroup tcpipapi
 * @param  index [IN] dns索引，索引范围由lwip配置决定，参考DNS_MAX_SERVERS宏，目前取值范围是[0,3]
 * @param  dns_addr [OUT] 保存字符串地址数据的内存地址，由用户外部申请
 * @return XY_OK:表示操作成功，XY_ERR:表示操作失败
 * @note 内部通过ipaddr_ntoa_r接口转换为字符串数据
 */
int xy_get_dns_server(int index, char* dns_addr);

/**
 * @brief 根据索引获取dns服务器的ip_addr_t类型地址
 * @ingroup tcpipapi
 * @param  index [IN] dns索引，索引范围由lwip配置决定，参考DNS_MAX_SERVERS宏，目前取值范围是[0,3]
 * @param  dns_addr [OUT] 保存ip_addr_t类型地址数据的内存地址，由用户传入
 * @return XY_OK:表示操作成功，XY_ERR:表示操作失败
 */
int xy_get_dns_server_by_ipaddr(int index, ip_addr_t* dns_addr);

/**
 * @brief  仅内部，不推荐用户使用，以ip_addr_t类型设置默认DNS地址,保存到出厂nv中，支持ipv4 ipv6 dns设置
 * @ingroup tcpipapi
 * @param ipaddr [IN] dns服务器地址
 * @param isSec [IN] 是否是辅助dns，0:主dns，1辅dns
 * @return XY_OK:表示操作成功，XY_ERR:表示操作失败 
 */
int xy_set_default_dns_server(ip_addr_t *ipaddr, uint8_t isSec);

/**
 * @brief  仅内部，不推荐用户使用，以字符串类型设置默认DNS地址,保存到出厂nv中，支持ipv4 ipv6 dns设置
 * @ingroup tcpipapi
 * @param ipaddr [IN] dns服务器地址
 * @param isSec [IN] 是否是辅助dns，0:主dns，1辅dns
 * @return XY_OK:表示操作成功，XY_ERR:表示操作失败 
 */
int xy_set_default_dns_server2(char *ipaddr, uint8_t isSec);

/**
 * @brief 根据索引获取默认dns服务器，以ip_addr_t类型地址返回
 * @ingroup tcpipapi
 * @param  ipaddr [OUT] 保存ip_addr_t类型地址数据的内存地址，由用户传入
 * @param  iptype [IN] 获取的dns ip类型
 * @param  isSec [IN] 是否是辅助dns，0:主dns，1辅dns
 * @return XY_OK:表示操作成功，XY_ERR:表示操作失败
 */
int xy_get_default_dns_server(ip_addr_t *ipaddr, uint8_t iptype, uint8_t isSec);