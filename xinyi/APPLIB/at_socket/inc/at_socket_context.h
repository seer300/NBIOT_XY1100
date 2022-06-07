#pragma once

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "cloud_utils.h"
#include "lwip/ip_addr.h"
#include "softap_macro.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define SEQUENCE_MAX                255
#define SOCK_CREATE_TIMEOUT         5   //seconds
#if VER_QUECTEL
#define SOCK_NUM                    7
#define AT_SOCKET_MAX_DATA_LEN      1358
#elif VER_QUCTL260
#define SOCK_NUM                    5
#define AT_SOCKET_MAX_DATA_LEN      1024
#else
#define SOCK_NUM                    2
#define AT_SOCKET_MAX_DATA_LEN      1400
#endif //VER_QUECTEL

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct rcv_data_nod
{
	void *next;
	int len;
	char *data;
	struct sockaddr_in *sockaddr_info;
} recv_data_node_t;

typedef struct seuqence_nod
{
    int socket_ctx_id; //socket context id
    int socket_id;     //socket fd
} sequence_node_t;

struct udp_context_fd
{ 
    net_infos_t net_info;
    char socket_idx;
    unsigned char af_type;
    unsigned char recv_ctl;
};

typedef struct _udp_context_t
{
	struct udp_context_fd udp_socket[SOCK_NUM];
	unsigned int data_mode;
} udp_context_t;

typedef struct sock_context
{
	unsigned char sock_id;		//用户自定义socket id
	unsigned char af_type;		//ai family类型，1表示AF_INET6，0表示AF_INET
	unsigned char is_deact;		//pdp去激活标识
	unsigned char quit;			//socket数据接收线程退出标识
	unsigned char fd;			//socket fd
	unsigned char firt_recv;	//首次收到下行数据标识
	unsigned char recv_ctl;		//1:指定socket_id接收传入的下行消息，默认值; 0:指定socket_id忽略传入的下行消息
	unsigned char net_type;		//1:UDP,DGRAM; 0:TCP,STREAM
	unsigned short remote_port; //远端端口号
	unsigned short local_port;	//本地端口号
	unsigned int sended_size;	//已发送的数据大小
	char *remote_ip;			//远端IP,以字符串表示
	struct rcv_data_nod *data_list;
#if VER_QUCTL260
    uint32_t acked;
    uint32_t unacked;
    uint16_t local_port_ori;
    uint16_t zero_flag;
    uint8_t accessmode;
    uint8_t cid;
    uint8_t sock_state;
#endif
	int8_t sequence_state[255];	//记录对应序列数据的发送状态，1:发送成功，0:发送失败
} socket_context_t;

struct sock_sn_node
{
	struct sock_sn_node *next;
	uint32_t per_sn;
	int data_len;
	unsigned char socket_id;
	unsigned char sequence;
	unsigned char net_type;
};

/*
RAI_DATA_EXCEPTION: Exception Message-send message with high priority
RAI_DATA_REL_UP: Release Indicator-indicate release after next message
RAI_DATA_REL_DOWN:Release Indicator-indicate release after next message has been replied
*/
typedef enum SOCKET_RAI_TYPE_T
{
	RAI_DATA_EXCEPTION = 0,
	RAI_DATA_REL_UP,
	RAI_DATA_REL_DOWN,
} SOCKET_RAI_TYPE;

typedef enum ipdatamode
{
	ASCII_STRING = 0,  
	HEX_ASCII_STRING,   //such as: "1234"=>1852(2 bytes)
	BIT_STREAM,
} ipdata_mode_e;

/* bc95 socket下行数据上报模式 */
typedef enum at_sck_report_mode
{
	BUFFER_NO_HINT,			//下行数据无提示，只存储4000个字节，多余的丢弃
	BUFFER_WITH_HINT,		//下行数据提示，第一个下行数据来临时，会主动上报，内容为“+NSONMI:<socketid>,<length>”，最多存储4000个字节，多余的丢弃
	HINT_WITH_REMOTE_INFO,	//下行数据提示，内容为“+NSONMI:<socket>,<remote_addr>,<remote_port>,<length>,<data>”，不储存数据
	HINT_NO_REMOTE_INFO, 	//下行数据提示，内容为+NSONMI: <socket>,<length>,<data>，不储存数据
} at_sck_report_mode_e;

/*数据包发送状态*/
typedef enum sequence_send_status
{
	SEND_STATUS_UNUSED = -1,
	SEND_STATUS_FAILED,
	SEND_STATUS_SUCCESS,
	SEND_STATUS_SENDING,
} sequence_send_status_e;

typedef enum socket_status
{
	SOCKET_STATE_AVAIL,	   //可用的
	SOCKET_STATE_UNAVAIL,  //不存在
	SOCKET_STATE_FLOW_CRL, //流量控制，unused
	SOCKET_STATE_BACK_OFF, //退避，unused
} socket_status_e;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern int g_ipdata_mode;		 //ipdata数据类型，参考ipdata_mode_e定义
extern uint8_t g_data_recv_mode;
extern uint8_t g_data_send_mode;
extern int g_at_sck_report_mode; //at socket下行数据上报模式,参考bc95_report_mode_e定义
extern struct sock_context *sock_ctx[];
extern osThreadId_t at_rcv_thread_id;
extern osMutexId_t g_socket_mux;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
void at_sock_recv_thread(void);
int del_socket_ctx_by_index(int idx, bool report);
int at_socket_create(struct sock_context *sock_param, int *fd);
int find_maxfd_in_use();
int add_new_rcv_data_node(int skt_id, int rcv_len, char *buf, struct sockaddr_in *sockaddr_info);
int find_sock_ctx_id_by_sock_id(int sock_id);
int find_sock_ctx_id_by_sock_fd(int sock_fd);
void set_socket_sequence_default_state(unsigned char sock_id);
void add_sninfo_node(unsigned char socket_id, int len, unsigned char sequence, uint32_t pre_sn);
void del_sninfo_node(unsigned char socket_id, unsigned char sequence);
void del_allsnnode_by_socketid(int socket_id);
int find_match_tcp_node(unsigned char socket_id, uint32_t ack, unsigned char *psequence);
int check_same_sequence(int sockid, unsigned char sequence_num);
int find_match_udp_node(unsigned char socket_id, unsigned char sequence);
int check_socket_valid(unsigned char net_type, unsigned short remote_port, char *remote_ip, unsigned short local_port, int flag);
void nonblock(int fd);
int get_free_sock_id();
int is_all_socket_exit();
void get_local_port_by_fd(int fd, unsigned short *local_port);
int connect_network(struct sock_context *sock_param);
int at_report_socket_state(int id, int net_type);
int at_SEQUENCE_req(char *at_buf, char **prsp_cmd);
int at_TCPACK_info(char *at_buf);
void netif_down_close_socket(uint32_t eventId);
void socket_init();
