/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_context.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "factory_nv.h"
#include "rtc_tmr.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "xy_utils.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
char g_req_type = AT_CMD_REQ;

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
osTimerId_t at_wait_farps_rsp_timer = NULL;

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
#if 0
static void wait_farps_rsp_timeout_proc(osTimerId_t timerId)
{
	(void) timerId;

    char *data = AT_ERR_BUILD(ATERR_WAIT_RSP_TIMEOUT);
    unsigned int size = strlen(data);
    struct at_fifo_msg *msg = xy_zalloc(sizeof(struct at_fifo_msg) + size + 1);

    memcpy(msg->data, data, size);
    osMessageQueuePut(at_farps_q, &msg, 0, osWaitForever);
    xy_free(data);
    return;
}
#endif

static void wait_nearps_rsp_timeout_proc(osTimerId_t timerId)
{
    xy_assert(timerId != NULL);
	softap_printf(USER_LOG, WARN_LOG, "at_ReqAndRsp_to_ps with timer: [%p] timeout!!!", timerId);

    osMessageQueueId_t queue_Id = at_related_queue_4_user(timerId);

    xy_assert(queue_Id != NULL);

    //超时重置forward context,避免超时定时器生效同时协议栈也上报返回，at_ctl处理协议栈上报时因为fwd_ctx指向异常而踩内存
    at_context_t *fwd_ctx = search_at_context(NEARPS_DSP_FD);
    if(fwd_ctx != NULL && fwd_ctx->fwd_ctx != NULL)
    {
        reset_ctx(fwd_ctx);
    }

    char *data = AT_ERR_BUILD(ATERR_WAIT_RSP_TIMEOUT);
    unsigned int size = strlen(data);
    struct at_fifo_msg *msg = xy_zalloc(sizeof(struct at_fifo_msg) + size + 1);
    memcpy(msg->data, data, size);
    osMessageQueuePut(queue_Id, &msg, 0, osWaitForever);
    xy_free(data);
    return;
}

//parse_rst:param parase errno; return int is ERROR: value
#if 0
static int get_handle_rst(osMessageQueueId_t rcv_fifo, char *prefix, char *info_fmt, void **pval)
{
    int ret = -1;
    char *end_tail;
    char *str_param;
	char *maohao=NULL;
    do
    {
        struct at_fifo_msg *rcv_msg = NULL;
        osMessageQueueGet(rcv_fifo, &rcv_msg, NULL, osWaitForever);
        char *rsp_at = rcv_msg->data;

        softap_printf(USER_LOG, WARN_LOG, "rsp_at:%s\r\n", rsp_at);
        if (rsp_at == NULL)
            xy_assert(0);

		if (info_fmt != NULL && strcmp(info_fmt, "%a") != 0 && strcmp(info_fmt, "%A") != 0 && pval != NULL && prefix != NULL)
		{
			if (((str_param = at_strstr2(rsp_at, prefix)) != NULL))
			{
				xy_assert(*str_param == ':' || *str_param == '=' || *str_param == '^');
				str_param++;
				if ((end_tail = at_strstr(str_param, "\r")) != NULL)
					*end_tail = 0;
				else
					xy_assert(0);
				ret = at_parse_param_2(info_fmt, str_param, pval);
				*end_tail = '\r';
				//break;
			}
			//中间结果没有前缀时，检查到没冒号或逗号前没冒号，可以直接解析参数,如3,"do:it"
            else if ((str_param = find_first_print_char(rsp_at, strlen(rsp_at))) != NULL \
                && !is_result_at(rsp_at) \
                && ((maohao = strchr(str_param, ':')) == NULL || (maohao > strchr(str_param, ','))))
			{
				if ((end_tail = at_strstr(str_param, "\r")) != NULL)
					*end_tail = 0;
				else
					xy_assert(0);
				ret = at_parse_param_2(info_fmt, str_param, pval);
				*end_tail = '\r';
				//break;
			}
		}
		
		//没有中间结果
        if (ret == -1)
        {
            if (at_strstr(rsp_at, "OK\r"))
                ret = AT_OK;
            else if (at_is_rsp_err(rsp_at, &ret) && ret == -1)
                ret = ATERR_NOT_ALLOWED;
        }
        else if (ret == AT_OK)
        {
            if (!is_result_at(rsp_at))
                ret = -1;
        }

        if (ret >= 0 && info_fmt != NULL &&
            (strcmp(info_fmt, "%a") == 0 || strcmp(info_fmt, "%A") == 0))
        {
            strcpy((char *)pval[0], rsp_at);
        }

        xy_free(rcv_msg);
    } while (ret < 0);

    return ret;
}
#endif

static int get_handle_rst_vp(osMessageQueueId_t rcv_fifo, char *prefix, char *info_fmt, const va_list* vp)
{
    int ret = -1;
    char *end_tail;
    char *str_param;
	char *maohao=NULL;
    char* all_format = NULL;
    do
    {
        struct at_fifo_msg *rcv_msg = NULL;
        osMessageQueueGet(rcv_fifo, &rcv_msg, NULL, osWaitForever);
        char *rsp_at = rcv_msg->data;

        softap_printf(USER_LOG, WARN_LOG, "user rsp:%s\r\n", rsp_at);
        if (rsp_at == NULL)
            xy_assert(0);

		if (info_fmt != NULL && strcmp(info_fmt, "%a") != 0 && strcmp(info_fmt, "%A") != 0 && prefix != NULL)
		{
	        if (((str_param = at_strstr2(rsp_at, prefix)) != NULL))
	        {
	            xy_assert(*str_param == ':' || *str_param == '=' || *str_param == '^');
	            str_param++;
	            if ((end_tail = at_strstr(str_param, "\r")) != NULL)
	                *end_tail = '\0';
	            else
	                xy_assert(0);
	            ret = parse_param_vp(info_fmt, str_param, 0, (va_list*)(vp));
	            *end_tail = '\r';
	        }
			//中间结果没有前缀时，检查到没冒号或逗号前没冒号，可以直接解析参数,如3,"do:it"
            else if ((str_param = find_first_print_char(rsp_at, strlen(rsp_at))) != NULL \
                && ((maohao = strchr(str_param, ':')) == NULL || (maohao > strchr(str_param, ','))))
            {
            	char *ok_head = at_strstr(str_param, "OK\r\n");
            	end_tail = at_strstr(str_param, "\r");
	            if (ok_head != str_param && end_tail != NULL)
	            {
	                *end_tail = 0;
					ret = parse_param_vp(info_fmt, str_param, 0, (va_list*)(vp));
	            	*end_tail = '\r';
	            }
	            else if (end_tail == NULL)
	                xy_assert(0);
	        }
		}
		//没有中间结果或者"%a"格式输出的结果
        if (ret == -1)
        {
            if (at_strstr(rsp_at, "OK\r"))
                ret = AT_OK;
            else if (at_is_rsp_err(rsp_at, &ret) && ret == -1)
                ret = ATERR_NOT_ALLOWED;
        }
		//中间结果解析正确，但可能还未收到最终结果码，需要继续接收
        else if (ret == AT_OK && !is_result_at(rsp_at,NULL))
        {
            ret = -1;
        }

        if (info_fmt != NULL && (strcmp(info_fmt, "%a") == 0 || strcmp(info_fmt, "%A") == 0))
        {
            if (ret != -1)
            {
                if (all_format != NULL)
                {
                    //收到了OK或者ERR并且之前上报过中间结果
                    strcpy((char *)va_arg(*vp, int), all_format);
                    xy_free(all_format);
                    all_format = NULL;
                }
            }
            else
            {
                if (all_format == NULL)
                {
                    //第一次上报中间结果
                    int all_format_len = strlen(rsp_at) + 1;
                    all_format = (char *)xy_zalloc(all_format_len);
                    memcpy(all_format, rsp_at, strlen(rsp_at) + 1);
                }
                else
                {
                    //多次收到中间结果需要拼接
                    int all_format_len = strlen(all_format) + strlen(rsp_at) + 1;
                    char *all_format_tmp = (char *)xy_zalloc(all_format_len);
                    memcpy(all_format_tmp, all_format, strlen(all_format));
                    memcpy(all_format_tmp + strlen(all_format), rsp_at, strlen(rsp_at) + 1);
                    xy_free(all_format);
                    all_format = all_format_tmp;
                }
            }
        }

        xy_free(rcv_msg);
    
    } while (ret == -1);

    return ret;
}

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
//用于解析AT命令参数中含ASCII字符串的参数，由于ASCII字符串可能包含双引号和逗号特殊字符，影响通过at_parse_param解析后续的参数，进而在将ASCII字符串拷贝出来之后，对原ASCII字符串中的特殊字符进行转义
int get_ascii_data(char *fmt_parm, char *at_str, int ascii_len, char *param_data)
{
	int i = 0;
	char *end_temp;
	char *fmt_temp;
	int douhao_num = 0;
	int str_len = 0;

	end_temp = strstr(fmt_parm, "%s");
	xy_assert(end_temp != NULL);
	xy_assert(strstr(end_temp + 2, "%s") == NULL);

	//跳过前面的参数个数
	fmt_temp = fmt_parm;
	while (fmt_temp < end_temp)
	{
		if (*fmt_temp == ',')
			douhao_num++;
		fmt_temp++;
	}

	//找到字符串参数首地址
	while (douhao_num != 0)
	{
		at_str = strchr(at_str, ',');
		if (at_str == NULL)
			return ATERR_PARAM_INVALID;
		at_str++;
		douhao_num--;
	}

	//字符串用双引号括起来情况
	if (*at_str == '"')
	{
		at_str++;
		if (*(at_str + ascii_len) != '"')
			return ATERR_PARAM_INVALID;
	}
	//无引号，下一个逗号或结束符为字符串结尾
	else
	{
		end_temp = strchr(at_str, ',');
		if(end_temp != NULL)
		{
			str_len = end_temp - at_str;
		}
		else {
			end_temp = strchr(at_str, '\0');
			if(end_temp == NULL)
				return ATERR_PARAM_INVALID;

			str_len = end_temp - at_str;
		}

		if(str_len != ascii_len)
			 return ATERR_PARAM_INVALID;
	}

	//将引号里面的内容拷出来作为有效参数内容
	memcpy(param_data, at_str, ascii_len);
	*(param_data + ascii_len) = '\0';

	
	//由于后续还有参数需要使用at_parse_param接口进行解析，进而需要将源字符串参数中的双引号和逗号这两个特殊匹配字符转义，以避免后续参数解析异常
	while (i < ascii_len)
	{
		if (*(at_str + i) == '"')
			*(at_str + i) = '\'';
		if (*(at_str + i) == ',')
			*(at_str + i) = ';';
		i++;
	}
	return AT_OK;

}


int at_parse_param(char *fmt, char *buf, ...)
{
    int ret = AT_OK;
    va_list vl;
    va_start(vl,buf);
    ret = parse_param_vp(fmt, buf, 0, &vl);
    va_end(vl);
    return ret;
}

int at_parse_param_in_quotation(char *fmt, char *buf, ...)
{
	int ret = AT_OK;
	va_list vl;
	va_start(vl,buf);
	
	char *p_end = NULL;
	char *quot_end = NULL;
	while (*buf == ' ')
		buf++;
	if(*buf == '"')
	{
		buf++;
		quot_end = strchr(buf,'"');
		if(quot_end != NULL)
		{
			p_end = quot_end + 1;
			while(*p_end == ' ')
				p_end++;
			if(*p_end == '\r')
			{
				*quot_end = '\r';
				*(quot_end + 1) = '\0';
				ret = parse_param_vp(fmt, buf, 0, &vl);
			}
			else
				ret = ATERR_PARAM_INVALID;
		}
		else
			ret = ATERR_PARAM_INVALID;
	}
	else
		ret = ATERR_PARAM_INVALID;
	va_end(vl);
	return ret;
}

int at_parse_param_ext(char *fmt, char *buf, int is_strict, ...)
{
    int ret = AT_OK;
    va_list vl;
    va_start(vl,is_strict);
    ret = parse_param_vp(fmt, buf, is_strict, &vl);
    va_end(vl);
    return ret;
}
#if 0
/**
 * @brief send at req to ext MCU,and parase reponse str to pval memory
 * @param req_at [IN] req at str,such as "AT+EXTATs=1,2,CMNET"
 * @param info_fmt [IN] response str format
 * @param pval [OUT] result memory
 * @param timeout [IN] wait rsp time,if timeout ,user need resend or process exception
 * @return parase result,0 is success,see  @ref  AT_XY_ERR
 * @note   暂未使用！！！
 * @par example
 * @code
	int ret = 0;
	ret = at_ReqAndRsp_to_ext("AT+EXTAT=1\r", NULL,  NULL, 0);
	if(ret == 0)
		xy_printf("success");

	char *pstr = k_malloc(50);
	memset(pstr,0,50);
	ret = at_ReqAndRsp_to_ext("AT+EXTAT\r", "%50s",  (void**)&pstr, 1);
	if(ret == 0)
		xy_printf("success");

	int n1 = 0;
	short n2 = 0;
	char n3 = 0;
	char *p2[] = {&n1,&n2,&n3};
	ret = at_ReqAndRsp_to_ext("AT+EXTAT?\r", "%d,%2d,%1d",	(void**)p2,0);
	if(ret == 0)
		xy_printf("success");
 * @endcode
 * @warning   该接口目前没有使用场景，慎用！
 */
int at_ReqAndRsp_to_ext(char *req_at, char *info_fmt, void **pval, int timeout)
{
    int again_num = 0;
    int at_errno = -1;
    char *prefix = xy_zalloc(AT_CMD_PREFIX);
	osTimerAttr_t timer_attr = {0};

    if ((info_fmt == NULL && pval != NULL) || (info_fmt != NULL && pval == NULL))
    {
        xy_assert(!" in param err");
		xy_free(prefix);
    }

    osMutexAcquire(at_farps_m, osWaitForever);
    if (timeout > 0)
    {
        if (at_wait_farps_rsp_timer == NULL)
        {
			timer_attr.name = "wait_near";
            at_wait_farps_rsp_timer = osTimerNew ((osTimerFunc_t)(wait_farps_rsp_timeout_proc), osTimerOnce, NULL, &timer_attr);
        }
        xy_assert(at_wait_farps_rsp_timer != NULL);
    }
    at_get_prefix_and_param(req_at, prefix, 0);
    if (strcmp(prefix, "") == 0)
    {
        at_errno = ATERR_INVALID_PREFIX;
        goto END_PROC;
    }

AGAIN:
    at_errno = -1;
    if (timeout > 0)
    {
        osTimerStop(at_wait_farps_rsp_timer);
        osTimerStart(at_wait_farps_rsp_timer, timeout * 1000);
    }

    xy_queue_clear(at_farps_q);
    at_errno = send_msg_2_atctl(AT_MSG_REQ_TO_FARPS, req_at, strlen(req_at), PS_FD_NOT_AVAIL);
    xy_assert(at_errno == AT_OK);
    at_errno = get_handle_rst(at_farps_q, prefix, info_fmt, pval);
    xy_assert(at_errno != -1);

    if (at_errno == ATERR_CHANNEL_BUSY)
    {
        again_num++;
        if (again_num > 100)
            xy_assert(0);
        osTimerStop(at_wait_farps_rsp_timer);
        osDelay(500 * again_num);
        goto AGAIN;
    }
END_PROC:
    if (timeout > 0)
    {
        osTimerDelete(at_wait_farps_rsp_timer);
        at_wait_farps_rsp_timer = NULL;
    }

    osMutexRelease(at_farps_m);
    if (at_errno != 0)
    {
		char *at_str = xy_zalloc(32);
        sprintf(at_str, "\r\n+DBGINFO:AT_ERROR:%d\r\n", at_errno);
        send_debug_str_to_at_uart(at_str);
		xy_free(at_str);
    }
	xy_free(prefix);
    return at_errno;
}
#endif
//该接口被废弃，若需要，请使用xy_atc_interface_call接口
int at_ReqAndRsp_to_ps(char *req_at, char *info_fmt, int timeout, ...)
{
    int at_errno = -1;
    int from_proxy = 0;
    int userFd = PS_FD_NOT_AVAIL;
    char *prefix = xy_zalloc(AT_CMD_PREFIX);
    at_context_t *ctx = NULL;
    osTimerId_t timer_Id = NULL;
    osMessageQueueId_t queue_Id = NULL;
	osTimerAttr_t timer_attr = {0};
    va_list vp;
    va_start(vp, timeout);

    //Step1: deal with abnormal situation
    softap_printf(USER_LOG, WARN_LOG, "user req:%s", req_at);

    //发送AT命令后该接口处于阻塞状态，不可在软定时器和RTC的回调中使用该接口！
    char *curTaskName = (char *)(osThreadGetName(osThreadGetId()));
    softap_printf(USER_LOG, WARN_LOG, "current task:%s", curTaskName);
    if (strstr(curTaskName, "Tmr Svc") || strstr(curTaskName, RTC_TMR_THREAD_NAME) ||
        strstr(curTaskName, AT_CTRL_THREAD_NAME))
    {
        xy_assert(0);
    }

    if (g_FOTAing_flag && (strncmp("AT+CFUN", req_at, strlen("AT+CFUN")) && strncmp("AT+CSQ", req_at, strlen("AT+CSQ"))))
    {
        va_end(vp);
		xy_free(prefix);
        return ATERR_DOING_FOTA;
    }

    at_get_prefix_and_param(req_at, prefix, 0);

    if (strcmp(prefix, "") == 0)
    {
        at_errno = ATERR_INVALID_PREFIX;
        goto END_PROC;
    }

    //Step2: get available at context fd for user
    if (strstr(curTaskName, XY_PROXY_THREAD_NAME))
    {
        from_proxy = 1;
    }

	timer_attr.name = "wait_nearps";
    timer_Id = osTimerNew((osTimerFunc_t)(wait_nearps_rsp_timeout_proc), osTimerOnce, NULL, &timer_attr);
    xy_assert(timer_Id != NULL);
    softap_printf(USER_LOG, WARN_LOG, "create timer: [%p] for user:%s", timer_Id, curTaskName);

    queue_Id = osMessageQueueNew(10, sizeof(void *), NULL);
    xy_assert(queue_Id != NULL);
    softap_printf(USER_LOG, WARN_LOG, "create queue: [%p] for user:%s", queue_Id, curTaskName);

    //Step3: create a context associated with current task
    ctx = get_avail_atctx_4_user(from_proxy);
    xy_assert(ctx != NULL);
    userFd = ctx->fd;
    softap_printf(USER_LOG, WARN_LOG, "get available at fd: [%d] for user", userFd);
    ctx->user_queue_Id = queue_Id;
    ctx->user_ctx_tmr = timer_Id;

    //Step5: start timer here
    at_errno = -1;
    if (timeout < 30)
    {
        timeout = 30;
    }
    osTimerStart(timer_Id, timeout * 1000);
    
    //Step6: clear queue and send msg to at ctl
    xy_queue_clear(queue_Id);
    at_errno = send_msg_2_atctl(AT_MSG_RCV_STR_FROM_FARPS, req_at, strlen(req_at), userFd);
    xy_assert(at_errno == AT_OK);
    at_errno = get_handle_rst_vp(queue_Id, prefix, info_fmt, &vp);
    softap_printf(USER_LOG, WARN_LOG, "USER_RSP:%d\r\n", at_errno);
    xy_assert(at_errno != -1);

END_PROC:
    //Step1: delete the related queue
    if (queue_Id != NULL)
    {
        softap_printf(USER_LOG, WARN_LOG, "delete queue fd: [%p]", queue_Id);
        xy_queue_Delete(queue_Id);
        queue_Id = NULL;
    }
    //Step2: delete the related timer
    if (timeout > 0 && timer_Id != NULL)
    {
        osTimerDelete(timer_Id);
        timer_Id = NULL;
    }
    //Step3: delete the user context which register in the global context dictionary
    if (ctx != NULL && userFd >= FARPS_USER_MIN && userFd <= FARPS_USER_MAX)
    {
        deregister_at_context(userFd);
        xy_free(ctx);
        ctx = NULL;
    }
    //Step4: if err occured,notify ext,such as uart or m3 core
    if (at_errno != AT_OK)
    {
		char *at_str = xy_zalloc(32);
        sprintf(at_str, "\r\n+DBGINFO:AT_ERROR:%d\r\n", at_errno);
        send_debug_str_to_at_uart(at_str);
		xy_free(at_str);
    }
    va_end(vp);
	xy_free(prefix);
    return at_errno;
}

void send_rsp_str_to_ext(void *data)
{
    send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, data, strlen(data), NEARPS_USER_FD);
}

void send_asyn_rsp_to_ext(void *data)
{
    send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, data, strlen(data), FARPS_UART_FD);
}

void send_urc_to_ext(void *data)
{
    at_rsp_info_broadcast(data, strlen(data));
}

void register_app_at_req(char *at_prefix, ser_req_func func)
{
    struct at_user_serv_proc_e *new_proc = (struct at_user_serv_proc_e *)xy_zalloc(sizeof(struct at_user_serv_proc_e) + strlen(at_prefix) + 1);
    strcpy(new_proc->at_prefix, at_prefix);
    new_proc->proc = func;
    new_proc->next = NULL;
    struct at_user_serv_proc_e *temp, *pre;
    if (g_sysapp_req_serv == NULL)
    {
        g_sysapp_req_serv = new_proc;
        return;
    }
    pre = g_sysapp_req_serv;
    temp = g_sysapp_req_serv->next;
    while (temp != NULL)
    {
        pre = temp;
        temp = temp->next;
    }
    pre->next = new_proc;
}

void register_app_at_urc(char *at_prefix, inform_act_func func)
{
    struct at_user_inform_proc_e *new_proc = (struct at_user_inform_proc_e *)xy_zalloc(sizeof(struct at_user_inform_proc_e) + strlen(at_prefix) + 1);
    strcpy(new_proc->at_prefix, at_prefix);
    new_proc->proc = func;
    new_proc->next = NULL;
    struct at_user_inform_proc_e *temp, *pre;
    if (g_sysapp_urc == NULL)
    {
        g_sysapp_urc = new_proc;
        return;
    }
    pre = g_sysapp_urc;
    temp = g_sysapp_urc->next;
    while (temp != NULL)
    {
        pre = temp;
        temp = temp->next;
    }
    pre->next = new_proc;
}

char *at_err_build(int err_no)
{
    char *at_str = NULL;

    xy_assert(err_no != 0);
    at_str = xy_malloc(50);
    snprintf(at_str, 50, "\r\n+CME ERROR:%d\r\n", err_no);

    return at_str;
}

char *at_err_build_info(int err_no, char *file, int line)
{
    (void)line;
	char *at_str = NULL;

	xy_assert(err_no != 0);
	at_str = xy_zalloc(64);

	if (g_softap_fac_nv->cmee_mode == 0)
		sprintf(at_str, "\r\nERROR\r\n");
	else if (g_softap_fac_nv->cmee_mode == 1)
#if VER_QUCTL260
		sprintf(at_str, "\r\n+CME ERROR:%d\r\n", get_at_err_num(err_no));
#else
		sprintf(at_str, "\r\n+CME ERROR:%d\r\n", err_no);
#endif
	else
		sprintf(at_str, "\r\n+CME ERROR:%s\r\n", get_at_err_string(err_no));

	if (g_softap_fac_nv->off_debug == 0)
	{
		char xy_file[20] = {0};
		if (strlen(file) >= 20)
			memcpy(xy_file, file + strlen(file) - 19, 19);
		else
			memcpy(xy_file, file, strlen(file));
		softap_printf(USER_LOG, WARN_LOG,"+DBGINFO:%s,line:%d,file:%s\r\n", get_at_err_string(err_no), line, xy_file);
	}
	return at_str;
}

char *bc26_at_err_build_info(char *file, int line)
{
	char *at_str = xy_zalloc(64);
	sprintf(at_str, "\r\nERROR\r\n");

	if (g_softap_fac_nv->off_debug == 0)
	{
		char xy_file[20] = {0};
		if (strlen(file) >= 20)
			memcpy(xy_file, file + strlen(file) - 19, 19);
		else
			memcpy(xy_file, file, strlen(file));
		softap_printf(USER_LOG, WARN_LOG,"+DBGINFO:line:%d,file:%s\r\n", line, xy_file);
	}
	return at_str;
}
