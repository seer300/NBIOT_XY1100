/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ctl.h"
#include "at_cmd.h"
#include "at_global_def.h"
#include "at_netdog.h"
#include "at_ps_cmd.h"
#include "at_ps_proxy.h"
#include "at_uart.h"
#include "at_worklock.h"
#include "csp.h"
#include "dsp_process.h"
#include "factory_nv.h"
#include "low_power.h"
#include "oss_nv.h"
#include "rtc_tmr.h"
#include "softap_nv.h"
#include "xy_lib_stub.h"
#include "xy_passthrough.h"
#include "xy_sys_hook.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "zero_copy_shm.h"
#include "at_xy_cmd.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define RETRANS_MAX_NUM 40  //底板MCU和用户同时发送冲突时采用的重传次数，以取代原先8007的报错
#define AT_CTRL_THREAD_STACKSIZE 0x800

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
osMessageQueueId_t at_farps_q = NULL;
osMutexId_t at_farps_m = NULL;
osMutexId_t g_farps_write_m = NULL;
struct at_user_inform_proc_e *g_sysapp_urc = NULL;
struct at_user_serv_proc_e *g_sysapp_req_serv = NULL;
extern int rcvd_passthr_stream_proc(char *buf, uint32_t data_len);

#if VER_QUECTEL
extern char *get_power_on_string(int reboot_num);
#endif
/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/
osMessageQueueId_t at_msg_q = NULL;
osMutexId_t at_send_2_ctl_m = NULL; //used to protect send_msg_2_at_ctl func visit
char g_atctl_req_type = AT_CMD_REQ;

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
int is_softap_DSP_cmd(char *buf)
{
	int i = 0;
	char *dsp_at_list[] =
	{
		"AT+LOGW",
		"AT+PSINFO=",
		"AT+NV",
		"AT+PINGXY",
		"AT+PINGSTOPXY",
		"AT+FLASH",
		"AT+NUESTATS=DSPMEM",
		"AT+NUESTATS=DSPSTACK",
		"AT+NUESTATS=DSPSIZE",
		"AT+ASSERTCP",
		"AT+PHY=",
		"AT+TEST=",
		"AT+RF=",
		"AT+RDNV",
		"AT+RFNV=",
		"AT+LOG=",
		"AT+QSCLK=",
	};

	for(i=0;i<(int)(sizeof(dsp_at_list)/sizeof(dsp_at_list[0]));i++)
	{
		if(!at_strncasecmp(buf,dsp_at_list[i],strlen(dsp_at_list[i])))
		{
			return 1;
		}
	}
	return 0;
}

 /*********************************************
用于修改AT+RF=XXX查询RF相关的命令，调用at_ReqAndRsp_to_ps接口发送该命令，得到的中间结果前缀和AT命令的前缀不一致，此接口用于修改一致整形为+RF:xxx形式。 
 **********************************************/
char *specail_rsp_proc(at_msg_t *msg)
{
	char *data_str = NULL;
	int i = 0;
	char *head = NULL;
	char *msg_data = msg->data;
	at_context_t *rcv_ctx = msg->ctx;
	xy_assert(rcv_ctx != NULL);

	if(strlen(rcv_ctx->at_cmd_prefix) == 0 || (at_strncasecmp(rcv_ctx->at_cmd_prefix,"+RF",strlen("+RF")) && at_strncasecmp(rcv_ctx->at_cmd_prefix,"+NUESTATS",strlen("+NUESTATS"))))
		return msg->data;
	char *specail_chr_list[] = 
	{
		"+RFVER:",
		"+IMEI:",
		"+SN:",
		"+MAXPOW:",
		"+DVGALMT:",
		"+VBAT:",
		"+Status:",
		"+Result:",
		"+RXSYNC:",
		"+GTSensor:",
		"+TSensor:",
		"+RXTEST:",
		"+GRFCAL:",
		"+CIMEI:",
		"+GRFBAK:",
		"+Signal power:",
	};
		

	while(*msg_data == '\r' || *msg_data == '\n')
	{
		msg_data++;
	}
	
	for(i=0;i<(int)(sizeof(specail_chr_list)/sizeof(specail_chr_list[0]));i++)
	{
		if(!at_strncasecmp(msg_data,specail_chr_list[i],strlen(specail_chr_list[i])))
		{
			data_str = xy_zalloc(msg->size + 14);
			if(!at_strncasecmp(msg_data,"+Signal power",strlen("+Signal power")))
			{
				memcpy(data_str,"\r\n+NUESTATS:",strlen("\r\n+NUESTATS:"));
				memcpy(data_str+strlen(data_str),msg_data,strlen(msg_data));
			}
			else
			{
				head = at_strstr2(msg->data,":");
				memcpy(data_str,"\r\n+RF:",strlen("\r\n+RF:"));
				memcpy(data_str+strlen(data_str),head,strlen(head));
			}
			return data_str;
		}
	}
	return msg->data;
	
}

static void at_info_proc(char *at_prefix, char *at_param)
{
	int i;
	struct at_inform_proc_e *at_inform;
	struct at_user_inform_proc_e *sysapp_at_inform;
	for (i = 0; g_at_basic_info != NULL && *(unsigned long *)(g_at_basic_info + i) != 0; i++)
	{
		at_inform = g_at_basic_info + i;

		if (!at_strcasecmp(at_inform->at_prefix, at_prefix))
		{
			at_inform->proc(at_param);
		}
	}

	sysapp_at_inform = g_sysapp_urc;
	while (sysapp_at_inform != NULL)
	{
		if (!at_strcasecmp(sysapp_at_inform->at_prefix, at_prefix))
			sysapp_at_inform->proc(at_param);
		sysapp_at_inform = sysapp_at_inform->next;
	}
}

static void proc_from_nearps(at_msg_t *msg)
{
	xy_assert(msg->ctx != NULL);
	char *at_prefix = xy_zalloc(AT_CMD_PREFIX);
	char *at_param = NULL;
	char *data_temp = NULL;
	int size_buf = msg->size;
	int is_rlt = is_result_at(msg->data,NULL);

	data_temp = specail_rsp_proc(msg);

	if ((at_param = at_get_prefix_and_param(data_temp, at_prefix, 0)) != NULL) //at_prefix:+XXX:
		at_info_proc(at_prefix, at_param);

#ifndef TASK_UNUSED
	//处理短信信息
	if (strlen(msg->ctx->at_cmd_prefix) && at_check_sms_fun(msg->ctx->at_cmd_prefix) && msg->ctx->fwd_ctx != NULL)
	{
		at_write_by_ctx(msg->ctx->fwd_ctx, msg->data, size_buf);
		if (at_strstr(msg->data, "> "))
		{	
			xy_enterPassthroughMode(PASSTHR_SMS, 0, (data_proc_func)rcvd_passthr_stream_proc);
		}
		else if (is_result_at(msg->data,NULL) == 1)
		{
			xy_exitPassthroughMode();
		}
		goto end_proc;
	}
#endif

	//only info
	if (is_rlt == 0 && strlen(at_prefix) != 0 &&
		(strlen(msg->ctx->at_cmd_prefix) == 0 || at_strncasecmp(msg->ctx->at_cmd_prefix, at_prefix, strlen(msg->ctx->at_cmd_prefix))))
	{
		//broadcast for any farps that can receive broadcast info
		softap_printf(USER_LOG, WARN_LOG, "[proc_from_nearps] ps urc info [%s]", at_prefix);
		at_rsp_info_broadcast(msg->data, size_buf);
	}
	else
	{
		//Step1:处理at_ReqAndRsp_to_ps接口上报的信息
		if (msg->ctx->fwd_ctx != NULL && msg->ctx->fwd_ctx->fd >= FARPS_USER_MIN && msg->ctx->fwd_ctx->fd <= FARPS_USER_MAX)
		{
			softap_printf(USER_LOG, WARN_LOG, "[proc_from_nearps] response info for user:[%d]", msg->ctx->fwd_ctx->fd);
			at_write_by_ctx(msg->ctx->fwd_ctx, data_temp, strlen(data_temp));
		}
		else if (msg->ctx->fwd_ctx != NULL)
		{
			//Step2: send response to original context
			softap_printf(USER_LOG, WARN_LOG, "[proc_from_nearps] response info for sended cmd");
			at_write_by_ctx(msg->ctx->fwd_ctx, msg->data, size_buf);
		}
		else
		{
			if (is_rlt == 1)
            {
				if (msg->ctx->fd == FARPS_UART_FD)
					at_write_by_ctx(msg->ctx, msg->data, size_buf);
				else
					softap_printf(USER_LOG, WARN_LOG, "ERROR!!!!rcv undefined rsp AT!");
            }
			else
			{
				//broadcast for farps entity which can receive broadcast info
				at_rsp_info_broadcast(msg->data, size_buf);
			}
		}
	}

end_proc:
	if (is_rlt == 1)
	{
		//clear nearps context
		if (msg->msg_id == AT_MSG_RCV_STR_FROM_NEARPS)
		{
			if (msg->ctx->fd == NEARPS_DSP_FD)
			{
				int err = 0;
				at_is_rsp_err(msg->data, &err);

				if (err != ATERR_CHANNEL_BUSY)
				{
					reset_ctx(msg->ctx);
				}
			}
			else if (msg->ctx->fd == NEARPS_USER_FD)
			{
				at_write_by_ctx(msg->ctx,msg->data, size_buf);
			}
		}
	}
	if (data_temp != msg->data)
		xy_free(data_temp);
	xy_free(at_prefix);
}

static void proc_from_farps(at_msg_t *msg)
{
	if (do_xdsend_cmd(msg->data))
		return;
	xy_assert(msg->ctx != NULL && msg->msg_id == AT_MSG_RCV_STR_FROM_FARPS);

	//Step1: get at-prefix,params and progress func
	msg->ctx->at_param = at_get_prefix_and_param(msg->data, msg->ctx->at_cmd_prefix, 1);

	//Step2: judge the parsed prefix is valid or not
	char *at_prefix = msg->ctx->at_cmd_prefix;
	if (at_prefix == NULL || !strlen(at_prefix))
	{
		softap_printf(USER_LOG, WARN_LOG, "at prefix from farps invalid");
		msg->ctx->at_param = NULL;
		AT_ERR_BY_CONTEXT(ATERR_INVALID_PREFIX, msg->ctx);
		return;
	}

	//Step3: the cmd from proxy task should not pass to proxy again!!! just pass the cmd into dsp
	if(msg->ctx->fd == FARPS_USER_PROXY)
	{
		softap_printf(USER_LOG, WARN_LOG, "at ctl deal with msg from proxy");
		goto FWD_PROC;
	}

	//Step4:deal with at-cmd belongs to xy_basic table
	int i = 0;
	struct at_serv_proc_e *at_basic;
	for (i = 0; g_at_basic_req != NULL && (g_at_basic_req + i)->at_prefix != 0; i++)
	{
		at_basic = g_at_basic_req + i;
		if (at_strcasecmp(at_prefix, at_basic->at_prefix) == 0)
		{
			//proxy命令交由主控线程处理
			xy_assert(at_basic->proc != NULL);
			msg->ctx->at_proc = at_basic->proc;

			int at_proxy_data_len = sizeof(at_proxy_data_t) + strlen(msg->data) + 1;
			at_proxy_data_t *at_proxy_data = xy_zalloc(at_proxy_data_len);

			/**由于at_context对应的上下文是全局变量，全局context随时可能被at_ctrl线程修改
			 * 因此交由主控线程处理时，必须malloc新的at_context,将当前的context拷贝一份
			 * 主控线程中的at_context不受at_ctl线程影响，但需在主控线程中主动释放新malloc的at_context
			*/
			at_proxy_data->msg_id = msg->msg_id;
			at_proxy_data->proxy_ctx = xy_zalloc(sizeof(at_context_t));
			memcpy(at_proxy_data->proxy_ctx, msg->ctx, sizeof(at_context_t));
			at_proxy_data->at_req_type = g_atctl_req_type;

			if(msg->ctx->at_param != NULL)
			{
				/**at_param同at_context一样，也需要额外malloc一块拷贝给主控线程，避免数据被at_ctrl线程修改*/
				int param_len = strlen(msg->ctx->at_param) + 1;
				at_proxy_data->proxy_ctx->at_param = xy_zalloc(param_len);
				memcpy(at_proxy_data->proxy_ctx->at_param, msg->ctx->at_param, param_len);
			}
			else
			{
				at_proxy_data->proxy_ctx->at_param = NULL;
			}

			//将at原始数据拷贝到at_proxy_data->data字段中
			if (msg->data != NULL)
				memcpy(at_proxy_data->data, msg->data, strlen(msg->data) + 1);

			send_msg_2_proxy(PROXY_MSG_AT_RECV, at_proxy_data, at_proxy_data_len);
			softap_printf(USER_LOG, WARN_LOG, "[xy_basic] msg [%s] send to proxy task", at_prefix);
			xy_free(at_proxy_data);
			return;
		}
	}

	//Step5:deal with at_cmd belongs to proxy or not
	struct at_user_serv_proc_e *at_user_custom = g_sysapp_req_serv;
	while (at_user_custom != NULL)
	{
		if (at_strcasecmp(at_prefix, at_user_custom->at_prefix) == 0)
		{
			//proxy命令交由主控线程处理
			xy_assert(at_user_custom->proc != NULL);
			msg->ctx->at_proc = at_user_custom->proc;

			int at_proxy_data_len = sizeof(at_proxy_data_t) + strlen(msg->data) + 1;
			at_proxy_data_t *at_proxy_data = xy_zalloc(at_proxy_data_len);

			/**由于at_context对应的上下文是全局变量，全局context随时可能被at_ctrl线程修改
			 * 因此交由主控线程处理时，必须malloc新的at_context,将当前的context拷贝一份
			 * 主控线程中的at_context不受at_ctl线程影响，但需在主控线程中主动释放新malloc的at_context
			*/
			at_proxy_data->msg_id = msg->msg_id;
			at_proxy_data->proxy_ctx = xy_zalloc(sizeof(at_context_t));
			memcpy(at_proxy_data->proxy_ctx, msg->ctx, sizeof(at_context_t));
			at_proxy_data->at_req_type = g_atctl_req_type;

			if(msg->ctx->at_param != NULL)
			{
				/**at_param同at_context一样，也需要额外malloc一块拷贝给主控线程，避免数据被at_ctrl线程修改*/
				int param_len = strlen(msg->ctx->at_param) + 1;
				at_proxy_data->proxy_ctx->at_param = xy_zalloc(param_len);
				memcpy(at_proxy_data->proxy_ctx->at_param, msg->ctx->at_param, param_len);
			}
			else
			{
				at_proxy_data->proxy_ctx->at_param = NULL;
			}

			//将at原始数据拷贝到at_proxy_data->data字段中
			if (msg->data != NULL)
				memcpy(at_proxy_data->data, msg->data, strlen(msg->data) + 1);

			send_msg_2_proxy(PROXY_MSG_AT_RECV, at_proxy_data, at_proxy_data_len);
			softap_printf(USER_LOG, WARN_LOG, "[user_custom] msg [%s] send to proxy task", at_prefix);
			xy_free(at_proxy_data);
			return;
		}
		at_user_custom = at_user_custom->next;
	}
	
FWD_PROC:
	//Step6: transfer at cmd to dsp
	forward_req_at_proc(msg);
}

static void init_resource(void)
{
	osMutexAttr_t mutex_attr = {0};

	at_msg_q = osMessageQueueNew(50, sizeof(void *), NULL);
	at_farps_q = osMessageQueueNew(10, sizeof(void *), NULL);

	g_farps_write_m = osMutexNew(NULL);
	at_farps_m = osMutexNew(NULL);
	at_send_2_ctl_m = osMutexNew(NULL);
	shm_mem_alloc_m = osMutexNew(NULL);
	mutex_attr.attr_bits = osMutexRecursive;
	at_ctx_dict_m = osMutexNew(&mutex_attr);
}

static void init_at_context(void)
{
	//initial nearps context
	nearps_ctx.fd = NEARPS_DSP_FD;
	nearps_ctx.position = NEAR_PS;
	register_at_context(&nearps_ctx);

	//initial uart context
	uart_ctx.fd = FARPS_UART_FD;
	uart_ctx.position = FAR_PS;
	uart_ctx.farps_write = at_write_to_uart;
	register_at_context(&uart_ctx);

	//initial user rsp context
	user_rsp_ctx.fd = NEARPS_USER_FD;
	user_rsp_ctx.position = NEAR_PS;
	register_at_context(&user_rsp_ctx);
}

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
void at_ctl(void)
{
	void *rcv_msg = NULL;
	at_msg_t *msg = NULL;

	//zhongyi request send "POWERON" followed by "^SIMST:"
	if (g_softap_fac_nv->work_mode != 0 || g_softap_fac_nv->offtime == 1 || g_softap_var_nv->ps_deepsleep_state == 2)
	{
		sys_up_urc();
	}
	
	//除了UTC唤醒，其他上电或唤醒方式都需要设置延迟锁
	if(get_sys_up_stat() != UTC_WAKEUP && g_softap_fac_nv->work_mode == 0)
	{
		xy_delaylock_active();
	}

	print_dsp_dbg();

	while (1)
	{
		osMessageQueueGet(at_msg_q, &rcv_msg, NULL, osWaitForever);

		if (rcv_msg == NULL)
		{
			xy_assert(0);
			continue;
		}
		msg = ((at_msg_t *)rcv_msg);
		switch (msg->msg_id)
		{
		case AT_MSG_RCV_STR_FROM_FARPS:
			proc_from_farps(msg);
			break;
		case AT_MSG_RCV_STR_FROM_NEARPS:
			proc_from_nearps(msg);
			break;

		default:
			break;
		}
		xy_free(msg);
	}
}

void at_init(void)
{
	osThreadAttr_t thread_attr = {0};

	init_resource();
	init_at_context();
	//ps回调注册必须在内核主线程中执行！
	ps_urc_register_callback_init();

	//initial at_ctrl callback and global variable
	g_at_ctrl_cbk.recv_from_nearps_cbk = at_rcv_from_nearps;
	g_at_ctrl_cbk.send_at_msg_cbk = send_msg_2_atctl;
	g_at_ctrl_cbk.at_broadcast_cbk = at_rsp_info_broadcast;
	
	g_NITZ_mode = g_softap_fac_nv->g_NITZ;
#if WAKEUP_MCU_BY_URC
	g_ring_enable = g_softap_fac_nv->ring_enable;
#endif

	//create at ctl thread
	thread_attr.name       = AT_CTRL_THREAD_NAME;
	thread_attr.priority   = osPriorityNormal2;
	thread_attr.stack_size = AT_CTRL_THREAD_STACKSIZE;
	osThreadNew((osThreadFunc_t)(at_ctl), NULL, &thread_attr);
}

int send_msg_2_atctl(int msg_id, void *buf, int size, int fd)
{
    osMutexAcquire(at_send_2_ctl_m, osWaitForever);

    int ret = AT_OK;
    at_msg_t *msg = xy_zalloc(sizeof(at_msg_t) + size + 1);
    msg->msg_id = msg_id;
    msg->size = size;

    softap_printf(USER_LOG, WARN_LOG, "send_msg_2_atctl msg_id: %d, fd: %d", msg_id, fd);

    if (msg_id == AT_MSG_RCV_STR_FROM_NEARPS ||
        msg_id == AT_MSG_RCV_STR_FROM_FARPS)
        msg->ctx = search_at_context(fd);
	else
    {
        ret = ATERR_NOT_ALLOWED;
		xy_free(msg);
        goto END_SND;
    }

    if (buf != NULL)
    {
        memcpy(msg->data, buf, size);
    }

	osMessageQueuePut(at_msg_q, &msg, 0, osWaitForever);

END_SND:
    osMutexRelease(at_send_2_ctl_m);
    return ret;
}

void forward_req_at_proc(at_msg_t *msg)
{
	//Step1: get nearps context firstly
	at_context_t *fwd_ctx = search_at_context(NEARPS_DSP_FD);
	xy_assert(fwd_ctx != NULL);
	char *curTaskName = (char *)(osThreadGetName(osThreadGetId()));

	//Step2: judge nearps context is on working or not
	//when MCU and M3 user send at req at the same time
	if ((fwd_ctx->state & SEND_REQ_NOW) && (is_critical_req(msg->data) == 0))
	{
		if (msg->ctx->retrans_count >= RETRANS_MAX_NUM)
		{
			AT_ERR_TO_PERIPHERAL(ATERR_CHANNEL_BUSY, msg->ctx);
			char *at_str = xy_zalloc(52);
			char new_prefix[10] = {0};
			memcpy(new_prefix, msg->data, 9);
			sprintf(at_str, "\r\n+DBGINFO:NEARPS BUSY:%s,new:%s\n", fwd_ctx->at_cmd_prefix, new_prefix);
			send_debug_str_to_at_peripheral(at_str, msg->ctx->fd);
			xy_free(at_str);
			reset_ctx(msg->ctx);
		}
		else
		{
			osDelay(50);
			msg->ctx->retrans_count++;
			softap_printf(USER_LOG, WARN_LOG, "atforward conflict, re-send fd:%d,count:%d", msg->ctx->fd, msg->ctx->retrans_count);
			//重发至at_ctrl线程
			send_msg_2_atctl(msg->msg_id, msg->data, msg->size, msg->ctx->fd);
		}
		return;
	}

	//Step3: get expected response prefix,for example: send cmd AT+CGSN=1,its expected preson prefix is +CGSN
	at_get_prefix_and_param(msg->data, fwd_ctx->at_cmd_prefix, 0);
	if (!strlen(fwd_ctx->at_cmd_prefix))
	{
		AT_ERR_BY_CONTEXT(ATERR_INVALID_PREFIX, msg->ctx);
		return;
	}

	//Step4: mark nearps context state,and link the nearsps context with current context
	fwd_ctx->state |= SEND_REQ_NOW;
	msg->ctx->fwd_ctx = fwd_ctx;
	if (strstr(curTaskName, XY_PROXY_THREAD_NAME))
	{
		//in xy proxy task,should use global context instead,because msg->ctx will free afterwards
		softap_printf(USER_LOG, WARN_LOG, "at forward for proxy ctl task");
		at_context_t *tmp_context = search_at_context(msg->ctx->fd);
		xy_assert(tmp_context != NULL);
		fwd_ctx->fwd_ctx = tmp_context;
		tmp_context->fwd_ctx = fwd_ctx;
	}
	else
		fwd_ctx->fwd_ctx = msg->ctx;

	//Step5: send msg to shm
	softap_printf(USER_LOG, WARN_LOG, "send msg [%s] to dsp core", msg->ctx->at_cmd_prefix);
	at_write_by_ctx(fwd_ctx, msg->data, strlen(msg->data));
}

void send_at_err_to_context(int errno, at_context_t *ctx, char *file, int line)
{
	char *at_errno = at_err_build_info(errno, file, line);
	at_write_by_ctx(ctx, at_errno, strlen(at_errno));
	xy_free(at_errno);
}

int send_rsp_str_2_app(osMessageQueueId_t queue_fd, char *data, int size)
{
	(void) size;

    xy_assert(queue_fd != NULL);
    struct at_fifo_msg *msg = xy_zalloc(strlen(data) + sizeof(struct at_fifo_msg) + 1);
    strcpy(msg->data, data);
	osMessageQueuePut(queue_fd, &msg, 0, osWaitForever);
    return 0;
}

int at_write_by_ctx(at_context_t *ctx, void *buf, int size)
{
	int ret = -1;

	xy_assert(size != 0 && ctx != NULL);

	if (ctx->position == NEAR_PS)
	{
		if(ctx->fd == NEARPS_DSP_FD) //just dsp fd transfer to dsp
		{

			if (DSP_IS_NOT_LOADED())
			{
				char *err_info = AT_ERR_BUILD(ATERR_DSP_NOT_RUN);
				send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, err_info, strlen(err_info), NEARPS_DSP_FD);
				xy_free(err_info);
				return ret;
			}

			if (is_softap_DSP_cmd(buf))
				ret = at_nearps_fd_write(ctx->fd, buf, size);
			else
				SendAt2AtcAp(buf, size);
			ret = 0;
		}
	}
	else if (ctx->position == FAR_PS)
	{
		if(ctx->fd >= FARPS_USER_MIN && ctx->fd <= FARPS_USER_MAX)
		{
			//notify the blocked queue which related to the fd
			softap_printf(USER_LOG, WARN_LOG, "receive rsp and notify queue: %p", ctx->user_queue_Id);
			ret = send_rsp_str_2_app(ctx->user_queue_Id, buf, size);
			return ret;
		}

		ret = at_farps_fd_write(ctx->fd, buf, size);
	}

	return ret;
}

int at_rsp_info_broadcast(void *buf, int size)
{
	at_write_by_ctx(&uart_ctx, buf, size);
	return XY_OK;
}

extern user_nv_data_t *g_user_nv;
extern void read_user_nv_demo();
extern void init_user_flash();
#if VER_QUCTL260 //MG 20221121 add by LGF
extern char* cis_cfg_tool(char* ip,unsigned int port,char is_bs,char* authcode,char is_dtls,char* psk,int *cfg_out_len);
#endif
void sys_up_urc()
{
    static int have_send_poweron = 0;
    char *at_str = NULL;

    if (have_send_poweron == 1)
        return;

    have_send_poweron = 1;
	
	at_str = xy_zalloc(64);
	
	init_user_flash();
	read_user_nv_demo();
	
	if(get_sys_up_stat() == UTC_WAKEUP || get_sys_up_stat() == EXTPIN_WAKEUP)
	{
		if(g_softap_fac_nv->deepsleep_urc == 1 && g_softap_var_nv->sleep_mode == 1)
		{
			sprintf(at_str,"\r\n+QNBIOTEVENT: \"EXIT DEEPSLEEP\"\r\n");			

			if(g_user_nv->atWakeup==1)
			{
				strcat(at_str,"\r\n+QATWAKEUP\r\n");
			}
		}
	}
	else
	{
		sprintf(at_str,"\r\nRDY\r\n");
#if VER_QUCTL260 //MG 20221116 add by LGF
		g_softap_var_nv->g_Echo_mode = 1;		
		//MG 20221121 added by LGF:for AT+MIPLCONFIG,sleepPreservation,resetNotPreservation
		g_softap_fac_nv->onenet_config_len = 69;
		g_softap_fac_nv->keep_cloud_alive = 1;
		g_softap_fac_nv->write_format = 0;	
		memset(g_softap_fac_nv->onenet_config_hex, 0, sizeof(g_softap_fac_nv->onenet_config_hex));
		memcpy(g_softap_fac_nv->onenet_config_hex, (char*)cis_cfg_tool("183.230.40.39", 5683, 1, NULL, 0, NULL, NULL), 69);

#endif
}

	if(strlen(at_str) != 0)
		at_rsp_info_broadcast(at_str, strlen(at_str));
	xy_free(at_str);

	if(xy_check_low_vBat(0) == XY_ERR)
	{
		char *power_str = xy_zalloc(32);;
		sprintf(power_str, "\r\n+LOWVBAT:%d\r\n", xy_getVbat());
		at_rsp_info_broadcast(power_str, strlen(power_str));
		xy_free(power_str);
	}
}

void send_at_err_to_periperal(int err_no, at_context_t* ctx, char *file, int line)
{
	if (ctx->fd != FARPS_UART_FD && ctx->fd != FARPS_BLE_FD)
		return;
	xy_assert(ctx->farps_write != NULL);
	char *err_str = at_err_build_info(err_no, file, line);
	ctx->error_no = err_no;

	if (ctx->fd == FARPS_UART_FD)
	{
		osMutexAcquire(g_farps_write_m, osWaitForever);
		ctx->farps_write(err_str, strlen(err_str));
		osMutexRelease(g_farps_write_m);
	}
	xy_free(err_str);
}

int proc_at_proxy_req(xy_proxy_msg_t *msg)
{
	char *at_prefix = xy_zalloc(AT_CMD_PREFIX);
	char *at_param = NULL;
	char *rsp_cmd = NULL;
	ser_req_func proc = NULL;
	at_proxy_data_t *at_proxy_data = (at_proxy_data_t *)msg->data;
	g_req_type = at_proxy_data->at_req_type;
	at_context_t *proxy_ctx = at_proxy_data->proxy_ctx;

	/* Step1: get proxy cmd info from msg context
	 * 首先判断全局context是否被重置了，后台命令并发时可能出现全局context被重置的情况
	 * 如果全局context被重置，上报8002错误，放弃本条AT命令处理
	 */
	if(proxy_ctx->at_proc == NULL)
	{
		softap_printf(USER_LOG, FATAL_LOG, "at context has been reset,drop!");
		AT_ERR_TO_PERIPHERAL(ATERR_NOT_ALLOWED, proxy_ctx);
		goto END_PROC;
	}
	at_param = proxy_ctx->at_param;
	strncpy(at_prefix, proxy_ctx->at_cmd_prefix, strlen(proxy_ctx->at_cmd_prefix));
	proc = proxy_ctx->at_proc;

	//Step2: execute at_proc function
	int res = proc(at_param, &rsp_cmd);
	softap_printf(USER_LOG, WARN_LOG, "proxy task deal with [%s] done", at_prefix);

	//Step3: deal with forward direction first
	if(AT_FORWARD == res)
	{
		/**发送数据到dsp核，由于forward_req_at_proc接口也会被at_ctrl调用，
		 * 此处需要重新封装一个at_msg_t变量作为入参，处理完毕主动释放
		 * */
		at_msg_t *at_msg_tmp = xy_zalloc(sizeof(at_msg_t) + strlen(at_proxy_data->data) + 1);
		at_msg_tmp->ctx = proxy_ctx;
		at_msg_tmp->size = strlen(at_proxy_data->data);
		at_msg_tmp->msg_id = at_proxy_data->msg_id;
		memcpy(at_msg_tmp->data, at_proxy_data->data, strlen(at_proxy_data->data) + 1);
		forward_req_at_proc(at_msg_tmp);
		xy_free(at_msg_tmp);
		at_msg_tmp = NULL;
		goto END_PROC;
	}
	//异步命令的处理结果由用户线程写到串口
	else if(AT_ASYN == res)
	{
		goto END_PROC;
	}
	else
	{
		//Step4: deal with some special response here
		if(rsp_cmd == NULL)
			rsp_cmd = at_ok_build();

		//Step5: notify original context to receive and deal with rsp
		at_write_by_ctx(proxy_ctx, rsp_cmd, strlen(rsp_cmd));
	}

END_PROC:
	if (rsp_cmd != NULL)
		xy_free(rsp_cmd);
	if (at_prefix != NULL)
		xy_free(at_prefix);

	/** 在at_ctl主线程中，proxy命令会额外malloc一个新的at_context和at_param空间
	 * 	主要是避免at_context对应的全局context被at_ctl主线程修改造成异常
	 * 	(异常的主要原因是由于后门命令不受控制导致，相关修改都是针对后门命令进行的)
	 * 	这里需要主动释放at_ctrl中额外申请的内存
	 */
	if (proxy_ctx != NULL)
	{
		if (proxy_ctx->at_param != NULL)
		{
			xy_free(proxy_ctx->at_param);
		}
		xy_free(proxy_ctx);
	}

	return 1;
}
