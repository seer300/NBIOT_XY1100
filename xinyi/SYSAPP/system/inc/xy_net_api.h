/** 
* @file        xy_net_api.h
* @brief       tcpip api,dns/socket/ping...
*
* @attention  User must use these APIs in the right process,and prevent state inconsistency caused by instantaneous change

* @warning     Socket related, please see lwip  POSIX Socket API "sockets.h"
*
*/
#pragma once
#include <stdint.h>
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/dns.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define XY_IP4ADDR_STRLEN       INET_ADDRSTRLEN     /* IPv4地址字符串最大长度 */
#define XY_IP6ADDR_STRLEN       INET6_ADDRSTRLEN    /* IPv6地址字符串最大长度 */
#define XY_IPADDR_STRLEN_MAX    INET6_ADDRSTRLEN    /* IP地址字符串最大长度 */
#define XY_BC26_SERVER_MAX      150

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
 * @brief ipv4信息
 */
typedef struct PsNetifIpv4Info_T
{
	ip4_addr_t ip;
	ip4_addr_t gateway;
	ip4_addr_t pri_dns;
	ip4_addr_t sec_dns;
} PsNetifIpv4Info;

/**
 * @brief ipv6信息
 */
typedef struct PsNetifIpv6Info_T
{
	unsigned char addr_type;
	unsigned char addr_stats;
	unsigned short mtu;
	unsigned short hop_limit;
	unsigned int base_reachable_time;
	unsigned int reachable_time;
	unsigned int retrans_timer;
	struct in6_addr ipv6_addr;
} PsNetifIpv6Info;

/**
 * @brief ip类型定义
 */
typedef enum PsNetifIpType_T
{
    NON_IP = -1,
    IPV4_TYPE = IPADDR_TYPE_V4,
    IPV6_TYPE = IPADDR_TYPE_V6,
    IPV46_TYPE= IPADDR_TYPE_ANY,
} PsNetifIpType;

/**
 * @brief Netif状态事件定义
 */
typedef enum PsNetifStateChangeEvent_T
{
    EVENT_PSNETIF_ENTER_OOS				= 1 << 0, //OOS状态，小区驻留失败，比如在隧道中
    EVENT_PSNETIF_ENTER_IS				= 1 << 1, //从OOS进入IS
    EVENT_PSNETIF_INVALID				= 1 << 2, //NEITF未激活
    EVENT_PSNETIF_IPV4_VALID			= 1 << 3, //IPV4地址可用
    EVENT_PSNETIF_IPV6_VALID			= 1 << 4, //IPV6地址可用
    EVENT_PSNETIF_IPV4V6_VALID			= 1 << 5, //IPV4V6双栈激活,V4和V6地址均可用
    EVENT_PSNETIF_DNS_CHNAGED			= 1 << 6, //NEITF已激活状态下,网卡信息发生改变，如dns server变化
    EVENT_PSNETIF_VALID                 = (EVENT_PSNETIF_IPV4_VALID | EVENT_PSNETIF_IPV6_VALID | EVENT_PSNETIF_IPV4V6_VALID),
    EVENT_PSNETIF_ALL_MASK				= (EVENT_PSNETIF_ENTER_OOS | EVENT_PSNETIF_ENTER_IS |
										   EVENT_PSNETIF_INVALID | EVENT_PSNETIF_IPV4_VALID | 
										   EVENT_PSNETIF_IPV6_VALID | EVENT_PSNETIF_IPV4V6_VALID | EVENT_PSNETIF_DNS_CHNAGED),
} PsNetifStateChangeEvent;

/**
 * @brief netif info数据
 */
typedef struct PsNetifInfo_T
{
    uint8_t           ipType;               //see @ref PsNetifIpType
    uint8_t           workingCid;           //pdp channel id
    uint16_t          reserved;
    PsNetifIpv4Info   ipv4Info;
    PsNetifIpv6Info   ipv6Info;
} PsNetifInfo;

/**
 * @brief psnetif事件回调接口类型定义
 * @param eventId  用户需要监听的psnetif事件 @see @ref PsNetifStateChangeEvent
 * @warning 注册psnetif回调函数必须在系统初始化中执行，否则可能出现事件遗漏问题 
 */
typedef void (*psNetifEventCallback_t)(uint32_t eventId);

typedef enum BC26_TCPIP_RESULT_CODE
{
    NET_OP_OK                   = 0,
    NET_UNKNOWN_ERR             = 550,
    NET_PARAM_ERR               = 552,
    NET_SOCK_CREATE_ERR         = 554,
    NET_OP_NOT_SUPPORT          = 555,
    NET_SOCK_BIND_ERR           = 556,
    NET_SOCK_WRITE_ERR          = 558,
    NET_SOCK_READ_ERR           = 559,
    NET_SOCK_ID_HAS_USED        = 563,
    NET_DNS_BUSY                = 564,
    NET_DNS_PARSE_FAIL          = 565,
    NET_SOCK_CONNECT_ERR        = 566,
    NET_OP_BUSY                 = 568,
    NET_OP_TIMEOUT              = 569,
    NET_OP_NOT_ALLOW            = 572,
    NET_PORT_BUSY               = 574,
} BC26_TCPIP_RESULT_CODE_T;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief  设置ipv4 dns地址，不支持ipv6 dns设置，保存nv
 * @ingroup tcpipapi
 * @param pridns[IN]  主dns服务器，ip4_addr_t类型
 * @param secdns[IN]  辅dns服务器，ip4_addr_t类型
 * @return XY_ERR设置失败, XY_OK设置成功
 * @note  输入的IP地址为二进制网络字节序
 */
int xy_dns_set(ip4_addr_t *pridns,ip4_addr_t *secdns);

/**
 * @brief 设置ipv4 dns地址，不支持ipv6 dns设置,保存nv
 * @ingroup tcpipapi
 * @param pri_dns[IN]  主dns服务器，字符串类型，例如"114.114.114.114"
 * @param sec_dns[IN]  辅dns服务器，字符串类型
 * @return XY_ERR设置失败, XY_OK设置成功
 * @note   输入的IP地址为点分文本的IP地址，内部通过inet_pton接口转换为二进制网络字节序的IP地址
 */
int xy_dns_set2(char *pri_dns,char *sec_dns);

/**
 * @brief  用户须在收到“+CGEV: ME PDN ACT 0”主动上报后调用，pdp激活时，tcpip协议栈的网路尚未配置完成,需等待TCPIP网路成功，
 *         IPV4单栈激活场景下等待获取ipv4地址成功，V6单栈及双栈激活场景下等待获取有效ipv6地址成功。
 * @ingroup tcpipapi
 * @param timeout [IN] 超时时间 0:非阻塞态，立即返回 非0：阻塞态
 * @return  XY_OK:tcpip可用 XY_ERR:tcpip不可用
 * @attention  用户需要特别注意超时时间.
 *  若用户在开机后直接调用该接口，建议timeout设置大于30秒，以给3GPP足够的时长进行小区驻留。一旦超时，表明3GPP无线环境异常，建议客户软重启后再尝试，若多次失败，建议睡眠一段时间后再尝试。
 *  v6单栈或者双栈激活情况下，由于ipv6需要花费额外的时间做RS/RA流程才能获取真实的ip地址。此时若在pdp激活后使用该接口，建议timeout设置大于3秒。
 * @warning 该接口内部有delay，因此at命令内部需谨慎使用该接口，如需使用timeout参数建议设置为0，否则会断言。
 */
int xy_wait_tcpip_ok(int timeout);

/**
 * @brief 获取ipv4字符串型地址
 * @param ipAddr [OUT]由调用者申请内存，接口内部赋值
 * @param addrLen [IN]用户申请保存ipv4地址的内存大小，不能小于16字节
 * @return 	XY_ERR获取失败, XY_OK获取成功
 * @warning 注意入参内存申请长度，否则会造成内存越界
 *          若尚未PDP激活成功，ipAddr不会被赋值
 * @note  该接口得到的是字符串类型地址，如需要转成int型，需要自行调用inet_aton接口
 */
int xy_getIP4Addr(char *ipAddr, int addrLen);

/**
 * @brief 	获取ipv4的整型地址
 * @param 	ip4addr[OUT] 指针类型，指向用户申请的存储ipv4地址
 * @return	XY_ERR获取失败, XY_OK获取成功
 * @warning 若尚未PDP激活成功，返回XY_ERR
 * @note    用户申请保存ipv4的内存大小至少需要4个字节，否则存在内存越界风险
 */
int xy_get_ip4addr(unsigned int *ip4addr);

/**
 * @brief 获取网络信息，如ip地址，地址类型等
 * @param netifInfo [OUT] 返回给用户的netfinfo数据，内存由用户申请
 * @return XY_ERR: 网卡尚未激活或者netifInfo参数为NUL, XY_OK:执行成功
 * @note
 */
int xy_get_netifInfo(PsNetifInfo *netifInfo);

/**
 * @brief  注册netif事件回调接口
 * @param  eventGroup  用户需要监听的事件组，可以是多个事件组合，也可以是单个事件
 * @param  callback 回调接口，类型@see @ref psNetifEventCallback_t
 * @warning 注册用户回调必须在初始化过程中完成！
 */
void xy_regist_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback);

/**
 * @brief 删除netif事件回调接口
 * @param  eventGroup  用户需要删除的事件组，可以是多个事件组合，也可以是单个事件
 * @param  callback 回调接口，类型@see @ref psNetifEventCallback_t
 * @warning 如果注册的回调函数中有向其他线程发消息队列之类的操作，应在接收线程退出前删除该事件回调，否则有死机的风险！
 */
int xy_deregist_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback);

/**
 * @brief  获取可用的ipv6的整型地址，非本地链路地址（以FE80开头的v6地址，例如FE80::1）
 * @param  ip6addr [OUT] 指针类型，指向用户申请的保存ipv6整型地址的内存
 * @return XY_ERR:netif网卡尚未激活/参数错误/未拿到有效的Ipv6单播地址
 * @note  用户申请保存ipv6的内存大小至少需要16个字节，否则存在内存越界风险
 */
int xy_get_ip6addr(unsigned int *ip6addr);

/**
 * @brief 获取可用的ipv6字符串型地址，非本地链路地址（以FE80开头的v6地址，例如FE80::1）
 * @param ip6Addr [OUT]用户申请保存ipv6字符串地址的内存
 * @param addrLen [IN]用户申请保存ipv6地址的内存大小，不能小于46字节
 * @return XY_ERR获取失败, XY_OK获取成功
 * @warning 注意入参内存申请长度，否则会造成内存越界。
 *          若尚未PDP激活成功，ip6Addr不会被赋值,返回XY_ERR
 * @note   该接口得到的是字符串类型地址，如需要转成整型，需自行调用inet_pton接口
 */
int xy_getIP6Addr(char *ip6Addr, int addrLen);

/**
 * @brief  根据ip类型获取ip_addr_t类型的ip地址
 * @param  ipType [IN]pdp激活的ip类型 @see @ref PsNetifIpType
 * @param  ipaddr [OUT]指针类型，指向用户申请的保存ip整型地址的内存
 * @return XY_ERR:netif网卡尚未激活/参数错误/未拿到有效的ip地址
 * @note 用户申请保存ipv4的内存大小至少需要4个字节，否则存在内存越界风险
 *       用户申请保存ipv6的内存大小至少需要16个字节，否则存在内存越界风险
 */
int xy_get_ipaddr(uint8_t ipType, ip_addr_t *ipaddr);

/**
 * @brief 获取PDP激活的IP类型
 * @return 返回ip类型 @see @ref PsNetifIpType
 * @note 该接口需要在pdp激活后调用
 */
int8_t xy_get_netifIpType(void);

/**
 * @brief  检测字符串类型的ip地址有效性
 * @param  ipaddr ip地址，如"192.168.0.1", "ABCD:EF01:2345:6789:ABCD:EF01:2314:1111",
 *        "ABCD:EF01:2345:6789:ABCD:EF01:111.111.111.111"
 * @param  iptype pdp激活的ip类型 @see @ref PsNetifIpType
 * @return XY_OK:表示地址有效，XY_ERR:地址无效 
 * @attention 支持V4/V6地址检测
 * @warning 需要检测的ip地址与iptype类型要匹配
 */
int xy_ipaddr_check(char *ipaddr, uint8_t iptype);

/**
 * @brief 根据域名和端口号创建socket
 * @param  hostname[IN] 字符串型域名
 * @param  port[IN] 端口号
 * @param  proto[IN] 协议类型，必须为IPPROTO_UDP或者IPPROTO_TCP,否则报错
 * @param  remote_addr[OUT] 返回远程服务端ip地址，如不需关注，入参可设置为NULL
 * @return -1:表示创建失败, 成功返回创建的socket fd
 * @note
 */
int32_t xy_socket_by_host(char *hostname, uint16_t port, int32_t proto, ip_addr_t* remote_addr);

/**
 * @brief  根据socket fd获取local ipaddr和local port
 * @param  fd [IN] socket fd
 * @param  local_ipaddr [OUT] local ipaddr，用户传参
 * @param  local_port [OUT] local port，用户传参
 * @return XY_ERR获取失败, XY_OK获取成功
 * @warning 如果不需要获取ipaddr,可以将入参local_ipaddr置NULL, 如果不需要获取port,可以将入参local_port置NULL.入参local_ipaddr和local_port不可同时为NULL
 */
int xy_get_socket_local_info(int32_t fd, ip_addr_t* local_ipaddr, uint16_t* local_port);

int get_bc26_tcpip_errcode(int lwip_err);
