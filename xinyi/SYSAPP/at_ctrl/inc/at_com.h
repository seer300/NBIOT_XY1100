#pragma once

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
 #include "xy_at_api.h"
 #include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define AT_CMD_MAX_LEN 128   //uart receive  max size once
#define AT_WAKEUP_DROP_LEN 4 //approximate string matching,to solve the problem of losing front strings by standby

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
struct at_serv_proc_e
{
	char *at_prefix;
	ser_req_func proc;
};

struct at_user_serv_proc_e
{
	struct at_user_serv_proc_e *next;
	ser_req_func proc;
	char at_prefix[0];
};

struct at_inform_proc_e
{
	char *at_prefix;
	inform_act_func proc;
};

struct at_user_inform_proc_e
{
	struct at_user_inform_proc_e *next;
	inform_act_func proc;
	char at_prefix[0];
};

struct at_fifo_msg
{
	char data[0];
};

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
//用于解析AT命令参数中含ASCII字符串的参数，由于ASCII字符串可能包含双引号和逗号特殊字符，影响通过at_parse_param解析后续的参数，进而在将ASCII字符串拷贝出来之后，对原ASCII字符串中的特殊字符进行转义
int get_ascii_data(char *fmt_parm, char *at_str, int ascii_len, char *param_data);

/**
 * @brief 获取AT请求命令的前缀和参数首地址，其中is_all用来指示前缀是否携带“AT+”
 * @param at_cmd [IN] at cmd data
 * @param at_prefix [OUT]record the at_prefix after parse, if prefix format invalid or parse error,return NULL
 * @param is_all [IN] value[0,1], 0:get prefix such as +WORKLOCK,1:get prefix such as AT+WORKLOCK
 * @note Notice the function may change g_req_type!!!
 * @return return params first char address
 */
char *at_get_prefix_and_param(char *at_cmd, char *at_prefix, char is_all);

/**
 * @brief  待废弃，请使用at_parse_param接口
 * @param fmt  [IN] format,such as "%1d,%d,%s"
 * @param buf  [IN] AT string param head,such as "2,5,cmnet"
 * @param pval [OUT] result memory
 * @return parase result,0 is success,see  AT_XY_ERR
 * @par example:
 * @code
	char buf[]="1,2,\"test\"";
	char n1 = 0;
	int n2 = 0;
	char *n3 = xy_zalloc(strlen(buf));
	char *p[] = {&n1,&n2,n3};
	ret = at_parse_param_2("%1d,%d,%s", buf, (void**)p);
	//result : n1==1 n2==2 n3=="test"
	ret = at_parse_param_2("%1d,%d,%2s", buf, (void**)p);
	//result : n1==1 n2==2 n3=="te"
   @endcode
 * @warning  不建议使用该接口，统一使用at_parse_param接口
 */
int at_parse_param_2(char *fmt, char *buf, void **pval);

/**
 * @brief  仅用于含转义字符串参数的解析，通常用于http等特殊命令中
 * @brief  由于支持字符串中转义字符解析，输入的字符串必须用""包含！！
 * @param fmt  [IN] 格式化字符串,例如 "%1d,%d,%s"
 * @param buf  [IN] 待解析的字符串头地址,例如 "2,5,cmnet"
 * @param parse_num  [OUT] 实际解析的参数个数，例如AT+HTTPHEADER=1,"" parse_num=2，而AT+HTTPHEADER=1  parse_num=1
 * @return 解析结果 参考@AT_XY_ERR
 * @par 示例:
 * @code
	char buf[]="1,2,\"test\"";
	int  parse_num = 0;
	char param1[16] = {0};
	char param2 = 0;
	int  param3 = 0;
	ret = at_parse_param_3("%1d,%d", buf, parse_num, &param2, &param3);
	//result : param2=1 param3=2 parse_num=2
	ret = at_parse_param_3("%1d,%d,%s", buf, parse_num, &param2, &param3, param1);
	//result : param1="test" param2=1 param3=2 parse_num=3
	ret = at_parse_param_3(",,%s", buf, parse_num, param1);
	//result : param1="test" parse_num=1
	char buf[]="1,2,\101test\102"";
	ret = at_parse_param_3("%1d,%d,%s", buf, parse_num, &param2, &param3, param1);
	//result : param1="AtestB" param2=1 param3=2 parse_num=3
 * @note 字符串转义字符解析示例: "\101xinyin\x42" => "AxinyiB"
 * @note 字符串转义字符解析示例: "\r\nxinyi\?" = "'\r''\n'xinyi?" 原先字符串中\r占用两个字节，解析成功后转为'\r' ACSII字符占用一个字节
 * @warning  该接口目前仅用于芯翼内部，解决特殊的字符转译问题
 */
int at_parse_param_3(char *fmt, char *buf, int* parse_num,...);

/*待废除 */
int is_critical_req(char *at_str);

int at_check_sms_fun(char *at_cmd_prefix);

/*该接口仅内部函数调用，参数解析以可变入参方式提供，类似scanf*/
int parse_param_vp(char *fmt_param, char *buf, int is_strict, va_list *ap);

/**
 * @brief close_urc置1时，识别待输出给外部MCU的buf是否可以为可丢弃的URC；对于一些异步AT命令的中间结果上报和特别重要的URC是不可以丢弃的
 * @param buf [IN] 待输出的主动上报信息
 * @return XY_OK表示URC可丢弃, XY_ERR表示URC不可丢弃
 * @note   该接口仅当底板MCU不想获取URC时才会被调到，即close_urc置1时
 */
int urc_can_drop(char *buf);

/**
 * @brief 待输出给外部MCU的buf是否为可丢弃的没有意义的URC，如调试URC、+XYIPDNS内部URC等
 * @param buf [IN] at urc信息
 * @return XY_OK表示可丢弃, XY_ERR表示不可丢弃
 */
int drop_unused_urc(char *buf);

void start_at_standby_timer();

void at_standby_set();

void at_standby_unset();

void at_standby_clear();

int at_deepsleep_Check();

