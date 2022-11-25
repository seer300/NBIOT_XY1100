/**
 * @file xy_passthrough.c
 * @brief 芯翼透传模块，用于通过at uart口进行透传数据传输，目前支持ppp,固定长度，socket tcp及短信模式透传
 * @version 1.0
 * @date 2021-04-29
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 * 
 */

#include "xy_utils.h"
#include "xy_passthrough.h"
#include "xy_system.h"
#include "xy_at_api.h"
#include "at_uart.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "at_passthrough.h"
#include "inter_core_msg.h"
#if AT_SOCKET
#include "at_socket_context.h"
#endif
#if TENCENT_VER
#include "xy_tencent_mqtt_client.h"
#endif
/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define PASSTHR_MAX_LEN  128
#define PASSTHR_CLOUD_MAX_LEN  1024
#define PPP_QUIT_SYMBOL "+++"
#define PASSTHR_UNUSED(x)  (void)(x)
#define PASSTHR_CTRLZ 0x1A
#define PASSTHR_ESC 0x1B
#define PASSTHR_END_SYMBOL "\r\nNO CARRIER\r\n\r\nOK\r\n"

/*******************************************************************************
 *						   Global variable definitions				           *
 ******************************************************************************/
data_proc_func g_at_passthr_hook = NULL; //must set hook by specail AT req,such as ATD/ONENETSENDRAW/SOCSENDRAW
uint8_t g_passthr_mode = PASSTHR_INVALID;
app_passthr_info_t g_app_passthr = {0};
#if AT_SOCKET
extern void restore_passthr_hook();
extern void add_uplink_data_to_list(char *data_buff, int data_len);
#endif //AT_SOCKET

/*******************************************************************************
 *						   Local variable definitions				           *
 ******************************************************************************/
char *passthr_rcv_buff = NULL;        //用于透传的AT命令的数据缓存
char ppp_tail_buff[4] = {0};          //缓存接收到的码流最后3个字节，以供匹配是否为+++结束标识
uint32_t ppp_rcvd_tail_len = 0;       //缓存接收到的+字符大小
uint32_t passthr_rcvd_len = 0;        //指示当前已接收到多少字节的透传数据
uint32_t passthr_fixed_buff_len = 0;  //指示需要接收的透传数据总长度，仅针对指定透传长度的AT命令有效，PPP模式该全局始终为0
uint32_t passthr_dynmic_buff_len = 0; //动态分配的buffer数据总长度
osTimerId_t passthr_timer = NULL;     //透传超时定时器
osMutexId_t passthr_msg_m;
osThreadId_t passthr_thread_id = NULL;
osMessageQueueId_t passthr_msg_q = NULL;

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
/**
 * @brief  判断特殊的透传结束字符串
 * @note   退出透传模式时，如果透传结束字符串不包含OK或者ERR，farps侧需要重置AT上下文
 * @warning 如果透传结束字符串不包含OK或者ERR, 缺少此判断条件，可能会导致8007错误
 */
bool is_spec_passthr_end_symbol(char* buf)
{
    PASSTHR_UNUSED(buf);
#if 0
	char *passthr_end_symbols[] = {
		"NO CARRIER",
	};
	uint32_t i = 0;
	for (i = 0; i < sizeof(passthr_end_symbols) / sizeof(passthr_end_symbols[0]); i++)
	{
		if (g_at_passthr_hook != NULL && at_strstr(buf, passthr_end_symbols[i]) != NULL)
			return true;
	}
#endif
	return false;
}

/**
 * @brief  对于不定长的透传模式，通过识别透传数据尾部的特殊字符串来退出透传模式，如“PPP”
 * @note   该接口内部实现不建议客户做任何修改
 */
bool find_ppp_quit_symbol(char *buf, uint32_t data_len)
{
    //先找开头，判断是否满足+++，++，+
	if (strlen(PPP_QUIT_SYMBOL) - ppp_rcvd_tail_len >= data_len)
	{
		memcpy(ppp_tail_buff + ppp_rcvd_tail_len, buf, data_len);
	}
	else
	{
		memcpy(ppp_tail_buff + ppp_rcvd_tail_len, buf, strlen(PPP_QUIT_SYMBOL) - ppp_rcvd_tail_len);
	}

	if (!strcmp(ppp_tail_buff, PPP_QUIT_SYMBOL))
	{
		memset(ppp_tail_buff, 0, sizeof(ppp_tail_buff));
		ppp_rcvd_tail_len = 0;
		return true;
	}
	else if (strlen(ppp_tail_buff) == 1 && *ppp_tail_buff == '+')
	{
		//ppp_tail_buff起始一个+
		ppp_rcvd_tail_len = 1;
		return false;
	}
	else if (strlen(ppp_tail_buff) == 2 && !strcmp(ppp_tail_buff, "++"))
	{
		//ppp_tail_buff起始两个+
		ppp_rcvd_tail_len = 2;
		return false;
	}
	else
	{
		//再找结尾，判断是否存在+++，++，+
		memset(ppp_tail_buff, 0, sizeof(ppp_tail_buff));
		if (data_len >= strlen(PPP_QUIT_SYMBOL))
		{
			if (memcmp(buf + data_len - 3, PPP_QUIT_SYMBOL, 3) == 0)
			{
				//数据末尾三字节为+++
				ppp_rcvd_tail_len = 0;
				return true;
			}
			else if (strstr(buf, PPP_QUIT_SYMBOL) != NULL)
			{
				//eg:abc+++def
				ppp_rcvd_tail_len = 0;
				return true;
			}
			else if (memcmp(buf + data_len - 2, "++", 2) == 0)
			{
				memcpy(ppp_tail_buff, "++", 2);
				ppp_rcvd_tail_len = 2;
			}
			else if (*(buf + data_len - 1) == '+')
			{
				*ppp_tail_buff = '+';
				ppp_rcvd_tail_len = 1;
			}
		}
		else
		{
			memcpy(ppp_tail_buff, buf, data_len);
			ppp_rcvd_tail_len = data_len;
		}	
		return false;
	}
}

int passthr_normal_ppp_hook(char* buf, uint32_t len)
{
    PASSTHR_UNUSED(buf);
    PASSTHR_UNUSED(len);
    return XY_OK;
}

int passthr_fixed_buff_len_hook(char *buf, uint32_t len)
{
    PASSTHR_UNUSED(buf);
    PASSTHR_UNUSED(len);
    return XY_OK;
}

/**
 * @brief 处理固定长度的透传数据
 * @note 如果接收到指定长度的数据，退出透传模式
 */
void process_fixed_len_data(char* buf, uint32_t data_len)
{
	softap_printf(USER_LOG, WARN_LOG, "fixed rcv len:%d", data_len);
	if (passthr_fixed_buff_len != 0)
        xy_assert((passthr_rcvd_len + data_len) <= passthr_fixed_buff_len);

    memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len);
    passthr_rcvd_len = passthr_rcvd_len + data_len;

    if (passthr_timer != NULL)
        osTimerStart(passthr_timer, 1000);

    //have finish passthrough data transfer
    if (passthr_fixed_buff_len != 0 && passthr_rcvd_len == passthr_fixed_buff_len)
    {
        passthr_fixed_buff_len_hook(passthr_rcv_buff, passthr_rcvd_len);
        xy_exitPassthroughMode();
    }
}

void cloud_fixed_len_data_proc(char* buf, uint32_t data_len)
{

	softap_printf(USER_LOG, WARN_LOG, "app passthr %d rcv len:%d", g_app_passthr.app_type, data_len);
	if((passthr_rcvd_len + data_len) < passthr_fixed_buff_len)
	{
		memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len);
		passthr_rcvd_len = passthr_rcvd_len + data_len;
	}
	else
	{
		if (passthr_rcvd_len < passthr_fixed_buff_len)
		{ 
			memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, passthr_fixed_buff_len - passthr_rcvd_len);
			passthr_rcvd_len = passthr_fixed_buff_len;
		}

		if (check_channel_fifo() == 0)
			return;
		g_app_passthr.callback_ret = g_app_passthr.proc(passthr_rcv_buff, passthr_fixed_buff_len, g_app_passthr.param);
        if (g_app_passthr.app_type == APP_SOCKET)
            return;
        xy_exitPassthroughMode();
	}
}

#if TENCENT_VER
/**
 * @brief 处理腾讯云相关的透传数据
 * @note 如果接收到指定长度或指定长度的数据
 */
void process_tencent_mqtt_data(char* buf, uint32_t data_len)
{
    softap_printf(USER_LOG, WARN_LOG, "fixed rcv len:%d", data_len);

	if (NULL == passthr_rcv_buff)
	{
		passthr_rcv_buff = xy_malloc(passthr_fixed_buff_len + 1);
		memset(passthr_rcv_buff, 0, passthr_fixed_buff_len + 1);
	}
	
    if (passthr_rcvd_len + data_len > passthr_fixed_buff_len)
    {
    	softap_printf(USER_LOG, WARN_LOG, "exceed the maximum length of data");
		xy_assert((passthr_rcvd_len + data_len) <= passthr_fixed_buff_len);
		goto END;
    }

	if (buf[data_len - 1] == PASSTHR_CTRLZ)
	{	
		memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len - 1);
		passthr_tencent_data_hook(passthr_rcv_buff, passthr_rcvd_len);
		goto END;
	}
	
    memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len);
    passthr_rcvd_len = passthr_rcvd_len + data_len;
	
    //have finish passthrough data transfer
    if (passthr_fixed_buff_len != 0 && passthr_rcvd_len == passthr_fixed_buff_len)
    {
        passthr_tencent_data_hook(passthr_rcv_buff, passthr_rcvd_len);
		goto END;
    }
	else
	{
		return;
	}
END:
	xy_exitPassthroughMode(); 
}
#endif

/**
 * @brief 处理ppp透传数据
 * @note 如果接收到ppp退出标识符+++，退出透传模式
 */
void process_normal_ppp_data(char *buf, uint32_t data_len)
{
    softap_printf(USER_LOG, WARN_LOG, "ppp_rcv len:%d", data_len);
    passthr_rcvd_len += data_len;
	if (find_ppp_quit_symbol(buf, data_len))
    {
		xy_exitPassthroughMode();
    }
    else
    {
		passthr_normal_ppp_hook(buf, data_len);
    }
}

void cloud_unfixed_len_data_proc(char *buf, uint32_t data_len)
{
    softap_printf(USER_LOG, WARN_LOG, "app passthr %d rcv len:%d", g_app_passthr.app_type, data_len);
    if (g_app_passthr.max_len == 0)
    {
        softap_printf(USER_LOG,WARN_LOG,"cloud unfixed passthr but max len is 0!!");
        goto END;
    }
	if (passthr_rcv_buff == NULL)
	{
		passthr_rcv_buff = xy_zalloc(PASSTHR_MAX_LEN);
		passthr_dynmic_buff_len = PASSTHR_MAX_LEN;
	}

	if(passthr_dynmic_buff_len < passthr_rcvd_len + data_len)
	{
		char *new_mem = xy_zalloc(passthr_dynmic_buff_len + PASSTHR_MAX_LEN);
		if(passthr_rcvd_len != 0)
			memcpy(new_mem, passthr_rcv_buff, passthr_rcvd_len);
		xy_free(passthr_rcv_buff);
		passthr_rcv_buff = new_mem;
		passthr_dynmic_buff_len += PASSTHR_MAX_LEN;
	}

	memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len);
	passthr_rcvd_len += data_len;

	if(passthr_rcvd_len > AT_DATA_MAX_LEN)
		goto END;

	//串口数据没有读完，返回继续读取
	if (check_channel_fifo() == 0)
		return;
	if(passthr_rcvd_len > g_app_passthr.max_len)
	{
		g_app_passthr.callback_ret = g_app_passthr.proc(passthr_rcv_buff, g_app_passthr.max_len, g_app_passthr.param);
        if (g_app_passthr.app_type == APP_SOCKET)
            return;
    }
	else if(buf[data_len - 1] == PASSTHR_CTRLZ)
	{
		g_app_passthr.callback_ret = g_app_passthr.proc(passthr_rcv_buff, passthr_rcvd_len - 1, g_app_passthr.param);
        if (g_app_passthr.app_type == APP_SOCKET)
            return;
	}
	else if(passthr_rcvd_len == g_app_passthr.max_len)
	{
		g_app_passthr.callback_ret = g_app_passthr.proc(passthr_rcv_buff, g_app_passthr.max_len, g_app_passthr.param);
        if (g_app_passthr.app_type == APP_SOCKET)
            return;
	}
  	else if(buf[data_len - 1] == PASSTHR_ESC)
		goto END;
	else
	{
		//未收到ctrl+z or esc,继续接收
		return;
	}

END:
	xy_exitPassthroughMode();
}

extern void xy_atc_data_req(unsigned short usDataLen, unsigned char*pucData);
/**
 * @brief 处理短信sms透传数据
 * @note 如果接收到短信结束标识符Ctrl+Z或者ESC，退出透传模式
 */
void process_sms_data(char *buf, uint32_t data_len)
{
	char *ret_rsp = NULL;
    if (passthr_rcv_buff == NULL)
	{
		passthr_rcv_buff = xy_zalloc(PASSTHR_MAX_LEN);
		passthr_dynmic_buff_len = PASSTHR_MAX_LEN;
	}

    if (passthr_dynmic_buff_len < passthr_rcvd_len + data_len)
    {
        char *new_mem = xy_zalloc(passthr_dynmic_buff_len + PASSTHR_MAX_LEN);
        if (passthr_rcvd_len != 0)
            memcpy(new_mem, passthr_rcv_buff, passthr_rcvd_len);
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = new_mem;
        passthr_dynmic_buff_len += PASSTHR_MAX_LEN;
    }

    memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len);
    passthr_rcvd_len += data_len;

	if (DSP_IS_NOT_LOADED())
	{
		ret_rsp = AT_ERR_BUILD(ATERR_DSP_NOT_RUN);
		send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, ret_rsp, strlen(ret_rsp), NEARPS_DSP_FD);
		xy_free(ret_rsp);
		goto END;
	}


	if (buf[data_len - 1] == PASSTHR_CTRLZ || buf[data_len - 1] == PASSTHR_ESC)
	{
		void *sms_data = NULL;
		sms_data = (void *)xy_zalloc_Align(passthr_rcvd_len);
		memcpy((char *)sms_data, passthr_rcv_buff, passthr_rcvd_len);

        xy_atc_data_req(passthr_rcvd_len, sms_data);
        xy_free(sms_data);
		goto END;
	}
	else
    {
        //未收到ctrl+z or esc
        return;
    }

END:
    xy_exitPassthroughMode();
}

/**
 * @brief 处理socket tcp透传数据
 * @note 如果接收到退出标识符+++，退出透传模式
 */
void process_socket_ppp_data(char *buf, uint32_t data_len)
{
#if AT_SOCKET
    if (find_ppp_quit_symbol(buf, data_len))
    {
        xy_exitPassthroughMode();
        return;
    }
    if (passthr_rcv_buff == NULL)
    {
        passthr_rcv_buff = xy_zalloc(PASSTHR_MAX_LEN);
        passthr_dynmic_buff_len = PASSTHR_MAX_LEN;
    }
    if (passthr_dynmic_buff_len < passthr_rcvd_len + data_len)
    {
        char *new_mem = xy_zalloc(passthr_dynmic_buff_len + PASSTHR_MAX_LEN);
        if (passthr_rcvd_len != 0)
            memcpy(new_mem, passthr_rcv_buff, passthr_rcvd_len);
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = new_mem;
        passthr_dynmic_buff_len += PASSTHR_MAX_LEN;
    }
    memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len);
    passthr_rcvd_len += data_len;
    
	//if rx fifo is NULL after wait for 2 ms,represent that current transmission has been complete from external MCU
	if (check_channel_fifo() == -1)
	{
        //挂链表，等待socket接收发送线程接收数据处理
		add_uplink_data_to_list(passthr_rcv_buff, passthr_rcvd_len);
		passthr_rcv_buff = NULL;
		passthr_rcvd_len = 0;
		passthr_dynmic_buff_len = 0;
	}
#else
    PASSTHR_UNUSED(buf);
    PASSTHR_UNUSED(data_len);
#endif //AT_SOCKET
}

/** 
* @brief  用户关注！超时未接收完透传数据的容错处理，建议客户切换到AT命令模式，并报错给外部MCU
*/
void passthr_timeout_callback(uint16_t *timer)
{
	PASSTHR_UNUSED(timer);
#if VER_QUCTL260  //add by cjh for bc260y 20221125
	send_urc_to_ext("\r\nTimeout\r\n");
	xy_exitPassthroughMode();
#endif
}

void passthr_send_msg(uint8_t type, uint32_t len, void* data)
{
    xy_assert(passthr_msg_q != NULL);
    osMutexAcquire(passthr_msg_m, osWaitForever);
    xy_passthr_msg_t *msg = xy_malloc(sizeof(xy_passthr_msg_t) + len);
    msg->msg_id = type;
    msg->len = len;
    if(data != NULL)
        memcpy(msg->data, data, len);
    osMessageQueuePut(passthr_msg_q, &msg, 0, osWaitForever);
    osMutexRelease(passthr_msg_m);
}

void passthrough_process()
{
    xy_passthr_msg_t *msg = NULL;
    while(1) 
	{
		osMessageQueueGet(passthr_msg_q, (void *)(&msg), NULL, osWaitForever);
        switch (msg->msg_id)
        {
        case PASSTHR_NORMAL_PPP:
            {
                process_normal_ppp_data(msg->data, msg->len);
            }
        break;

        case PASSTHR_FIXED_LENGTH:
            {
                process_fixed_len_data(msg->data, msg->len);
            }
        break;

        case PASSTHR_SOCKET_PPP:
            {
                process_socket_ppp_data(msg->data, msg->len);
            }
        break;

        case PASSTHR_SMS:
            {
                process_sms_data(msg->data, msg->len);
            }
        break;

#if	TENCENT_VER
		case PASSTHR_TENCENT:
            {
                process_tencent_mqtt_data(msg->data, msg->len);
            }
        break;
#endif
        default:
            break;
        }

        xy_free(msg);
    }
}

void xy_enterPassthroughMode(xy_passthrough_type_t mode, uint32_t rcv_len, data_proc_func func)
{
	osThreadAttr_t thread_attr = {0};
	osTimerAttr_t timer_attr = {0};
    xy_assert(func != NULL && passthr_rcv_buff == NULL && mode > PASSTHR_INVALID && mode < PASSTHR_MAX);

    g_passthr_mode = mode;
    passthr_rcv_buff = NULL;

	if (mode == PASSTHR_CLOUD_FIXED_LENGTH)
	{
		passthr_rcv_buff = xy_zalloc(rcv_len + 1);
	}
	else if (mode == PASSTHR_FIXED_LENGTH)
    {
        passthr_rcv_buff = xy_zalloc(rcv_len + 1);
        if (passthr_timer == NULL)
        {
        	timer_attr.name = "passthr_timer";
            passthr_timer = osTimerNew((osTimerFunc_t)(passthr_timeout_callback), osTimerOnce, NULL, &timer_attr);
        }
    }

	if (g_app_passthr.app_type == APP_INVILAD)
	{
		//初始化透传消息发送线程
		if (passthr_msg_q == NULL)
			passthr_msg_q = osMessageQueueNew(10, sizeof(void *), NULL);
		if (passthr_msg_m == NULL)
			passthr_msg_m = osMutexNew(NULL);
		if (passthr_thread_id == NULL)
		{
			thread_attr.name = "xy_passthr";
			thread_attr.priority = osPriorityNormal;
			thread_attr.stack_size = 0x800;
			passthr_thread_id = osThreadNew((osThreadId_t)passthrough_process, NULL, &thread_attr);
		}
	}
	passthr_fixed_buff_len = rcv_len;
	passthr_rcvd_len = 0;
	g_at_passthr_hook = func;
	
	//锁standby
	xy_standby_lock();
}

/**
 * @brief  切换回AT命令模式，该接口调用点为完成透传数据的传递后，或者接收超时后。
 * @note   该接口内部实现不建议客户做任何修改
 */
void xy_exitPassthroughMode()
{
    if (g_passthr_mode == PASSTHR_INVALID)
        return;

    if (passthr_rcv_buff != NULL)
        xy_free(passthr_rcv_buff);
	passthr_rcv_buff = NULL;
	passthr_rcvd_len = 0;
	passthr_fixed_buff_len = 0;
    passthr_dynmic_buff_len = 0;

#if AT_SOCKET
    //socket ppp模式缓存hook函数，以便ATO恢复
    if (g_passthr_mode == PASSTHR_SOCKET_PPP)
        restore_passthr_hook();
#endif //AT_SOCKET
    g_at_passthr_hook = NULL;

    if(g_passthr_mode == PASSTHR_SOCKET_PPP || g_passthr_mode == PASSTHR_NORMAL_PPP)
    {
        memset(ppp_tail_buff, 0, sizeof(ppp_tail_buff));
	    ppp_rcvd_tail_len = 0;
    }

	if (passthr_timer != NULL)
	{
        //删除透传定时器
		osTimerDelete(passthr_timer);
        passthr_timer = NULL;
    }

    //standby解锁
    xy_standby_unlock();
	if (g_app_passthr.app_type != APP_INVILAD)
	{
        if (g_app_passthr.app_type == APP_SOCKET && g_app_passthr.callback_ret == XY_OK)
        {   
#if AT_SOCKET && VER_QUCTL260
            socket_context_t *ctx = (socket_context_t *)g_app_passthr.param;
            if (ctx->zero_flag == 1)
            {
                ctx->zero_flag = 0;
                send_asyn_rsp_to_ext("\r\nOK\r\n\r\nSEND OK\r\n");
            }
            else
#endif 
            {
                send_asyn_rsp_to_ext("\r\nOK\r\n");
            }
        
        }
		else if(g_app_passthr.callback_ret == XY_OK)
			send_asyn_rsp_to_ext("\r\nOK\r\n");
		else
		{
			char *passthr_cmd = NULL;
			passthr_cmd = BC26_AT_ERR_BUILD();
			send_asyn_rsp_to_ext(passthr_cmd);
			if(passthr_cmd != NULL)
				xy_free(passthr_cmd);
		}
	}
	else
	{
#if TENCENT_VER
	    if ((g_passthr_mode != PASSTHR_SMS) && (g_passthr_mode != PASSTHR_TENCENT))
#else
		if (g_passthr_mode != PASSTHR_SMS)
#endif
		{
	        char *passthr_end = xy_zalloc(50);
	        sprintf(passthr_end, PASSTHR_END_SYMBOL);
	        send_urc_to_ext(passthr_end);
	        xy_free(passthr_end);
	    }
	}
    //重置透传模式
    g_app_passthr.app_type = APP_INVILAD;
	g_app_passthr.proc = NULL;
    g_passthr_mode = PASSTHR_INVALID;
}

void xy_exitSocketPassthroughMode(uint32_t eventId)
{
	(void)eventId;
	xy_exitPassthroughMode();
}

/**
 * @brief  处理从串口接收到的透传码流，由于码流需要多次才能接收完毕，需要进行超时监控
 * @note   内部区分了各种透传模式
 * @warning 该接口暂不对客户开放，由芯翼内部实现
 */
int rcvd_passthr_stream_proc(char *buf, uint32_t data_len)
{
	softap_printf(USER_LOG, WARN_LOG, "passthr[%d] recv from uart len:%d", g_passthr_mode, data_len);
    if (data_len == 0)
        return XY_OK;
    passthr_send_msg(g_passthr_mode, data_len, buf);
    return XY_OK;
}

/**
 * @brief  云业务处理从串口接收到的透传码流
 * @note   内部区分了定长和不定长透传模式
 * @warning 该接口暂不对客户开放，由芯翼内部实现
 */
int cloud_rcvd_passthr_proc(char *buf, uint32_t data_len)
{
    if (data_len == 0)
        return XY_OK;
	softap_printf(USER_LOG, WARN_LOG, "cloud passthr[%d] recv from uart len:%d", g_passthr_mode, data_len);
         
	switch (g_passthr_mode)
	{
		case PASSTHR_CLOUD_FIXED_LENGTH:
		{
			cloud_fixed_len_data_proc(buf, data_len);
			break;
		}
		case PASSTHR_CLOUD_UNFIXED_LEN:
		{
			cloud_unfixed_len_data_proc(buf, data_len);
			break;
		}
		default:
			break;
	}
	return XY_OK;
}

int into_Passthr_Mode(app_passthr_info_t app_passthr)
{
	g_app_passthr = app_passthr;

	if(g_app_passthr.app_type == APP_INVILAD || g_app_passthr.proc == NULL)
		return XY_ERR;

	if(g_app_passthr.recv_len == 0)
		xy_enterPassthroughMode(PASSTHR_CLOUD_UNFIXED_LEN, 0, (data_proc_func)(cloud_rcvd_passthr_proc));
	else
		xy_enterPassthroughMode(PASSTHR_CLOUD_FIXED_LENGTH, g_app_passthr.recv_len, (data_proc_func)(cloud_rcvd_passthr_proc));

	return XY_OK;
}

/**
 * @brief  透传模式的AT命令注册初始化接口
 * @note   
 */
void passthr_init(void)
{
	register_app_at_req("ATD",at_ATD_req);
	register_app_at_req("AT+TRANSPARENTDEMO",at_TRANSPARENTDEMO_req);
	register_app_at_req("AT+CLOUDPASSTHR",at_CLOUDPASSTHR_req);
#if AT_SOCKET
    register_app_at_req("AT+XTRANSSOCKOPEN",at_XTRANSSOCKOPEN_req);
	register_app_at_req("AT+XTRANSSOCKCLOSE",at_XTRANSSOCKCLOSE_req);
	register_app_at_req("ATO",at_ATO_req);
#endif //AT_SOCKET
}