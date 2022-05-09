/** 
* @file        xy_at_api.h
* @brief       用户开发扩展AT命令所需要的常用接口，通常用于定制用户自己的AT请求命令
* @warning     该头文件仅适用于非3GPP的扩展AT命令；3GPP相关的AT命令，请参阅xy_ps_api.h
*/
#pragma once
#include <stdint.h>
#include "xy_ps_api.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
/**
* @brief  AT错误码构建宏定义，内部组装为“+CME ERROR: XXX”
* @param  a 错误码，参见@ref AT_XY_ERR
* @return  返回字符串堆指针，由芯翼平台内部负责释放空间
*/

#define AT_ERR_BUILD(a) at_err_build_info(a, __FILE__, __LINE__)

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
* @brief AT process result macro
* @param AT_OK			         at_parse_param_2 success
* @param ATERR_XY_ERR			 fatal error need reset
* @param ATERR_PARAM_INVALID     param  invalid
* @param ATERR_NOT_ALLOWED       operate not allow
* @param ATERR_DROP_MORE		 because sleep,at string head drop too more
* @param ATERR_DOING_FOTA		 For the duration of FOTA,cannot proc any AT req
* @param ATERR_MORE_PARAM        too more param,can not parase this param,only used for strict check
* @param ATERR_WAIT_RSP_TIMEOUT  wait AT response result timeout
* @param ATERR_CHANNEL_BUSY  	 now have one req at working,receive new  req at 
* @param ATERR_DSP_NOT_RUN       dsp not run
* @param ATERR_NOT_NET_CONNECT   NB PS can not PDP activate within the agreed time
* @param ATERR_NEED_OFFTIME      extern MCU power off XY Soc,and not send time bias extensional AT CMD when power up again
* @param ATERR_CONN_NOT_CONNECTED  connecting socket be resetted by server for unknown case,user must close current socket
* @param ATERR_INVALID_PREFIX 	 invalid prefix
* @param ATERR_LOW_VOL           voltage  too low,and SoC can not work normal
* @param USER_EXTAND_ERR_BASE    user can add self  err number from  here!
*/
enum AT_XY_ERR
{
	AT_OK = 0,

	ATERR_XY_ERR = 8000,
	ATERR_PARAM_INVALID,         //8001
	ATERR_NOT_ALLOWED,           //8002
	ATERR_DROP_MORE,             //8003
	ATERR_DOING_FOTA,            //8004
	ATERR_MORE_PARAM,            //8005
	ATERR_WAIT_RSP_TIMEOUT,      //8006
	ATERR_CHANNEL_BUSY,          //8007
	ATERR_DSP_NOT_RUN,           //8008
	ATERR_NOT_NET_CONNECT,       //8009
	ATERR_NEED_OFFTIME,          //8010
	ATERR_CONN_NOT_CONNECTED,    //8011
	ATERR_INVALID_PREFIX,	     //8012
	ATERR_LOW_VOL,               //8013

	USER_EXTAND_ERR_BASE = 9000,
};

/**
* @brief AT STR process result
* @param AT_END			reply rsp cmd  by at_ctl dirextly
* @param AT_FORWARD		forward  at req to  external processor
*/
enum at_cmd_route
{
	AT_END = 0,
	AT_FORWARD,
	AT_ASYN,
	AT_ROUTE_MAX,
};

/**
* @brief AT动作的种类，用全局g_req_type来记录
*/
enum AT_REQ_TYPE
{
	AT_CMD_REQ = 0, //AT+xxx=param
	AT_CMD_ACTIVE,	//AT+xxx,not include param
	AT_CMD_QUERY,	//AT+XXX?
	AT_CMD_TEST,	//AT+XXX=?
};

/**
* @brief  请求类AT命令的注册回调函数声明
* @param  at_paras [IN] at cmd param head,such as "1,5,CMNET"
* @param  rsp_cmd  [OUT] double pointer,response result string,such as "+ERROR:8009"
* @return always is AT_END,,see  @ref  at_cmd_route
* @note    if have response result string ,must malloc memory for rsp_cmd,and set value
*/
typedef int (*ser_req_func)(char *at_paras, char **rsp_cmd);

/**
* @brief  主动上报类AT命令的注册回调函数声明
* @param  at_paras  at cmd param head,such as "1,5,CMNET"
* @return see  @ref at_context_state,always is AT_END
* @note   always is AT_END,see  @ref  at_cmd_route
*/
typedef int (*inform_act_func)(char *at_paras);


/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern char g_req_type;		//record request cmd type

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 仅用于调试，通过AT口发送调试类URC主动上报，such as "+DBGINFO:"
 * @param data [IN]is URC cmd,such as "\r\n+DBGINFO:NV ERROR\r\n"
 * @warning  该接口在idle线程或锁临界区情况下会直接返回，造成丢数据，所以该接口仅用于发送调试信息，不得发送正常功能性AT命令
 */
void send_debug_str_to_at_uart(char *buf);

/**
 * @brief 用于在芯翼平台框架中发送AT应答结果给外部MCU，通常在register_app_at_req注册的func回调接口中调用该接口.
 * @param data [IN] is response result cmd,such as "\r\nOK\r\n"  "+URC:ZZZZ\r\n\r\nOK\r\n"
 * @warning  该接口效率偏低，不得用来发送URC主动上报，详见send_urc_to_ext接口。   \n
 ***         该接口只能运行在芯翼的at框架线程中，不得运行在onenet、ctwing、user等线程中。
 */
void send_rsp_str_to_ext(void *data);

/**
 * @brief 用于在用户线程中异步发送AT应答结果给外部MCU，常见的使用策略是在register_app_at_req注册的func回调接口中，将AT请求命令发送给用户线程，用户线程处理完毕后调用该接口回复AT应答结果。
 * @param data [IN] is response result cmd,such as "\r\nOK\r\n"   "+URC:ZZZZ\r\n\r\nOK\r\n"
 * @warning  该接口效率偏低，不得用来发送URC主动上报，详见send_urc_to_ext接口。\n
 ***         该接口只能运行在onenet、ctwing、user等线程中，不得运行在芯翼的at框架线程中。
 */
void send_asyn_rsp_to_ext(void *data);

/**
 * @brief 仅用于供业务层发送主动上报URC给AT口，不得发送结果码
 * @param data [IN] is URC cmd,such as "\r\n+XXX:YYYY\r\n"
 * @warning  该接口不能发送携带“OK”“ERROR”结果码的字符串，只能发送主动上报URC，如"+URC:ZZZZ\r\n"
 */
void send_urc_to_ext(void *data);

/**
 * @brief  用于注册3gpp相关的主动上报处理函数，待废除
 * @param  at_prefix  [IN] URC prefix,such as "+CGEV:"
 * @param  该接口待废除；如果用户想截获3GPP相关的主动上报，请使用xy_atc_registerPSEventCallback
 */
void register_app_at_urc(char *at_prefix, inform_act_func func);

/**
 * @brief  用于注册非3GPP的扩展AT请求命令的处理函数
 * @param  at_prefix [IN] AT请求前缀
 * @param  func [IN] 相应的AT请求处理函数
 * @note   该接口不适用于3GPP相关的AT请求命令
 */
void register_app_at_req(char *at_prefix, ser_req_func func);


/**
 * @brief  按照fmt格式解析每个参数值，类似scanf格式
 * @param fmt  [IN] format,such as "%1d,%d,%s"
 * @param buf  [IN] AT string param head,such as "2,5,cmnet"
 * @param va_args ... [IN/OUT] va_args assigned by user,should be pointer
 * @return parase result,0 is success,see  @ref  AT_XY_ERR
 * @par example:
 * @code
	char buf[]="1,2,\"test\"";
	char n1 = 0;
	int n2 = 0;
	char *n3 = xy_zalloc(strlen(buf));
	ret = at_parse_param("%1d,%d,%s", buf, &n1,&n2,n3);
	//result : n1==1 n2==2 n3=="test"
	ret = at_parse_param("%1d,%d,%2s", buf, &n1,&n2,n3);
	//result : n1==1 n2==2 n3=="te"
   @endcode
 * @warning   该接口仅用于客户二次扩展的非3GPP命令的参数解析
 */
int at_parse_param(char *fmt, char *buf, ...);

/**
 * @brief  仅用于特殊AT命令格式的参数解析，即多个参数被放在一组冒号内的AT命令，例如AT+CCLK="19/12/10,11:12:24+32"
 * @param fmt  [IN] format,such as "%s,%s"
 * @param buf  [IN] AT string param head with quotation,such as "\"19/12/10,11:12:24+32\""
 * @param va_args ... [IN/OUT] va_args assigned by user,should be pointer
 * @return parase result,0 is success,see  @ref  AT_XY_ERR
 * @par example:
 * @code
	char buf[]="\"19/12/10,11:12:24+32\"";
	char date[16] = {0};
	char time[16] = {0};
	ret = at_parse_param_in_quotation("%1s,%s", buf, n1,n2);
	//result : n1=="19/12/10",n2=="11:12:24+32"
   @endcode
 * @warning   该接口用于解析多个参数被放在一组冒号内的AT命令，例如AT+CCLK="19/12/10,11:12:24+32"
 */
int at_parse_param_in_quotation(char *fmt, char *buf, ...);


/**
 * @brief AT错误码构建函数
 * @param err_no [IN] 错误码，参见AT_XY_ERR
 * @param file [IN] 调用函数所在文件名
 * @param line [IN] 调用函数所在行
 * @return 返回字符串堆指针
 * @attention  慎用！建议客户直接使用@ref AT_ERR_BUILD
 */
char *at_err_build_info(int err_no, char *file, int line);

//该接口只能做生产测试使用,业务开发严禁使用该接口，如需触发3GPP相关AT命令，请使用xy_atc_interface_call接口，细节参阅at_3GPP_cmd_demo.c
int at_ReqAndRsp_to_ps(char *req_at, char *info_fmt, int timeout, ...);

