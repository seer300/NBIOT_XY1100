#pragma once

#include <stdint.h>

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef enum passthrogh_app_type
{
	APP_INVILAD = 0,
	APP_SOCKET,
	APP_COAP,
	APP_MQTT,
	APP_TENCENT,
}passthrogh_app_type_t;

/**
 * @brief 业务透传数据处理接口定义
 * @param buf 透传数据地址
 * @param buf 透传数据长度
 */
typedef int (*app_passthrough_proc)(char* buf, uint32_t len, void *param);

typedef struct app_passthr_info
{
	uint8_t app_type;
    uint8_t reserved;
	uint16_t recv_len;
    uint32_t max_len;
    app_passthrough_proc proc;
	int callback_ret;
	void *param;
}app_passthr_info_t;

typedef enum xy_passthrough_type
{
    PASSTHR_INVALID = 0,
    PASSTHR_NORMAL_PPP,				//ppp模式，通常以+++作为退出标志符
    PASSTHR_FIXED_LENGTH,			//固定长度模式，收到指定长度数据后退出
    PASSTHR_SOCKET_PPP,				//socket数据透传模式，类似于ppp，以+++作为退出符
    PASSTHR_SMS,					//短信模式
    PASSTHR_CLOUD_FIXED_LENGTH,		//云业务的定长透传模式，收到指定长度数据后退出
	PASSTHR_CLOUD_UNFIXED_LEN,		//云业务的不定常透传模式，以ctrlz或esc作为退出标识符
#if TENCENT_VER
	PASSTHR_TENCENT, 
#endif
    PASSTHR_MAX,
} xy_passthrough_type_t;

typedef struct xy_passthr_msg
{
    uint8_t msg_id;
    uint32_t len;
    char data[0];
} xy_passthr_msg_t;

/**
  * @brief  AT通道传输透传数据的回调函数声明
  */
typedef int (*data_proc_func)(char *data, unsigned int data_len);

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern data_proc_func g_at_passthr_hook;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief  用户关注！收到切换为透传模式AT指令后，调用该接口切换为透传模式接收状态
 * @param  mode [IN] 指定透传模式
 * @param  rcv_len [IN] 待接收的透传数据长度，PPP模式下该值必须填0，以指示长度不确定
 * @param  func [IN] 接收到的透传数据的回调接口，用户实现该接口内容
 * @note   该接口内部实现不建议客户做任何修改，客户仅需关注func回调接口和超时时长及超时处理即可
 */
void xy_enterPassthroughMode(xy_passthrough_type_t mode, uint32_t rcv_len, data_proc_func func);

/**
 * @brief 退出透传模式，切换到AT命令模式
 * @note
 */
void xy_exitPassthroughMode();

/**
 * @brief 业务进入透传模式的接口
 * @note
 */
int into_Passthr_Mode(app_passthr_info_t app_passthr);

/**
 * @brief 退出Socket模式透传回调函数
 * @note  netif去激活时，回调该接口
 */
void xy_exitSocketPassthroughMode(uint32_t eventId);

/** 
* @brief  用户关注！供用户处理PPP模式接收到的透传数据，需要客户自行拼接和缓存
*/
int passthr_normal_ppp_hook(char *buf, uint32_t len);

/** 
* @brief  用户关注！供用户自行处理指定长度的透传数据
*/
int passthr_fixed_len_hook(char *buf, uint32_t len);
