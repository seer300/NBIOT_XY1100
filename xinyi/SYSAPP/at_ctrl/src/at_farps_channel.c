/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "at_netdog.h"
#include "at_ps_cmd.h"
#include "at_uart.h"
#include "at_worklock.h"
#include "csp.h"
#include "factory_nv.h"
#include "ipcmsg.h"
#include "low_power.h"
#include "oss_nv.h"
#include "softap_nv.h"
#include "xy_at_api.h"
#include "xy_lib_stub.h"
#include "xy_passthrough.h"
#include "xy_sys_hook.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "zero_copy_shm.h"

#if VER_QUECTEL
#include "at_hardware_cmd.h"
#include "osal.h"
extern void natspeed_succ_hook(void);
extern void natspeed_change_hook(void);
#endif //VER_QUECTEL
extern void ipr_change_hook(void);
/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
int realloc_farps_rcv_mem(int len,at_context_t *ctx)
{
	char *new_mem;

	xy_assert(ctx->g_farps_rcv_mem != NULL);

	if (len > 5000 + AT_CMD_MAX_LEN)
	{
		 unsigned int check_zero = 0;
		 int i = 0;
		 for (; i < 1250; i++)
		 {
			check_zero += *((unsigned int *)(ctx->g_farps_rcv_mem+i*4));
		 }
		 if(check_zero < 255)
		 {
			  ctx->g_have_rcved_len = 0;
			  len = AT_CMD_MAX_LEN;
			  softap_printf(USER_LOG, WARN_LOG, "AT rx pin pull down,and recv all zero!!!ERROR!!!");
			  send_debug_str_to_at_peripheral("+DBGINFO:AT RX PIN pull down ERROR\r\n", ctx->fd);
		 }
	}

	new_mem = xy_zalloc(len);
	if(ctx->g_have_rcved_len != 0)
		memcpy(new_mem,ctx->g_farps_rcv_mem,ctx->g_have_rcved_len);
	xy_free(ctx->g_farps_rcv_mem);
	ctx->g_farps_rcv_mem = new_mem;
	ctx->g_rcv_buf_len = len;
	return 0;
}

int single_atcmd_proc(at_context_t *ctx, char *recv_atcmd, int data_len)
{
	char *at_str = find_first_print_char(recv_atcmd,data_len);
	if(at_str == NULL)
	{
		softap_printf(USER_LOG, WARN_LOG, "recv only /CR/LF");
		return XY_ERR;
	}

	/** 中移socket灌包命令对处理时间有要求，xdsend命令需尽快交由at_ctl处理*/
	if (!at_strncasecmp(at_str, "AT+XDSEND=", strlen("AT+XDSEND=")))
	{
		*(recv_atcmd + data_len - 1) = '\r';
		data_len = data_len - (at_str - recv_atcmd);
		send_msg_2_atctl(AT_MSG_RCV_STR_FROM_FARPS, at_str, data_len, ctx->fd);
		return XY_OK;
	}

	*(recv_atcmd + data_len - 1) = '\0';
	char *print_str = xy_zalloc(76);
	memcpy(print_str, at_str, (unsigned int)(data_len - (at_str - recv_atcmd)) > 75 ? 75 : (unsigned int)(data_len - (at_str - recv_atcmd)));
	softap_printf(USER_LOG, WARN_LOG, "uart_recv:%s,len:%d", print_str, data_len);
	xy_free(print_str);
	
	if (!at_strcasecmp(at_str, "AT+ASSERT"))
		xy_assert(0);

	if (!at_strncasecmp(at_str, "AT+FASTOFF", strlen("AT+FASTOFF")) || !at_strncasecmp(at_str, "AT+CPOF", strlen("AT+CPOF")))
	{
		goto CONV_CHECK;
	}

	if (g_FOTAing_flag)
	{
		AT_ERR_TO_PERIPHERAL(ATERR_DOING_FOTA, ctx);
		softap_printf(USER_LOG, WARN_LOG, "FOTAing, drop all at command!\r\n");
		*(recv_atcmd + data_len - 1) = '\r';
		return XY_ERR;
	}

	if (xy_check_low_vBat(0) == XY_ERR)
	{
		char *s_low_vBat = xy_zalloc(30);
		snprintf(s_low_vBat, 30, "\r\n+LOWVBAT:%d\r\n", xy_getVbat());
		send_urc_to_ext(s_low_vBat);
		xy_free(s_low_vBat);
	}

	//judge at request is offtime request or not
	if (g_need_offtime == 1 && at_strncasecmp(at_str, "AT+OFFTIME", strlen("AT+OFFTIME")))
	{
		AT_ERR_TO_PERIPHERAL(ATERR_NEED_OFFTIME, ctx);
		return XY_ERR;
	}

	if ((ctx->state & RCV_REQ_NOW) && (is_critical_req(at_str) == 0))
	{
		AT_ERR_TO_PERIPHERAL(ATERR_CHANNEL_BUSY, ctx);
		char *atstr = xy_zalloc(80);
		char new_prefix[10] = {0};
		memcpy(new_prefix, recv_atcmd, 9);
		if (strlen(ctx->at_cmd_prefix) != 0)
			snprintf(atstr, 80, "\r\n+DBGINFO:FARPS BUSY:%s,new:%s,fd:%d\r\n", ctx->at_cmd_prefix, new_prefix, ctx->fd);
		else
			snprintf(atstr, 80, "\r\n+DBGINFO:at_ctl block,new:%s,fd:%d\r\n", new_prefix, ctx->fd);

		send_debug_str_to_at_peripheral(atstr, ctx->fd);
		xy_free(atstr);
		*(recv_atcmd + data_len - 1) = '\r';
		return XY_ERR;
	}

CONV_CHECK:
	if (vary_to_uppercase(at_str) == 0)
	{
		AT_ERR_TO_PERIPHERAL(ATERR_NOT_ALLOWED, ctx);
		return XY_ERR;
	}

	ctx->state |= RCV_REQ_NOW;

	//close standby until wait reponse AT
	*(recv_atcmd + data_len - 1) = '\r';
	data_len = data_len - (at_str - recv_atcmd);

	ctx->process_state = AT_PROCESSING;

	send_msg_2_atctl(AT_MSG_RCV_STR_FROM_FARPS, at_str, data_len, ctx->fd);

	return XY_OK;
}

int atstr_is_dirty(at_context_t *ctx, char **ppchr)
{
	char *read;
	char *check_str = ctx->g_farps_rcv_mem;
	char *temp_chr = NULL;

	while((check_str-ctx->g_farps_rcv_mem) < ctx->g_have_rcved_len && (*check_str == '\r' || *check_str == '\n' || *check_str == '\0'))
		check_str++;

	if((check_str-ctx->g_farps_rcv_mem) == ctx->g_have_rcved_len) 
		return 0;
	
	if((*check_str!='A' && *check_str!='a') || (*(check_str+1)!='T' && *(check_str+1)!='t')) //!check_atcmd(check_str)
	{
		softap_printf(USER_LOG, WARN_LOG, "len=%d,UART DIRTY!!!ERROR!", ctx->g_have_rcved_len);
		if(check_str == NULL)
			softap_printf(USER_LOG, WARN_LOG, "check_str is null!");
		return 1;
	}

	read = check_str;

	for (; read < ctx->g_farps_rcv_mem+ctx->g_have_rcved_len; read++) 
	{
		//"AT+W***LOCK=0",for ***
		if (!is_at_char(*read)) 
		{				
			if(*read == '\0' && (int)(read - ctx->g_farps_rcv_mem) == ctx->g_have_rcved_len-1)
				continue;
			char *dirty_str=xy_zalloc((ctx->g_farps_rcv_mem+strlen(ctx->g_farps_rcv_mem)-read)*2+1);
			bytes2hexstr((unsigned char *)read, (signed long)(ctx->g_farps_rcv_mem+strlen(ctx->g_farps_rcv_mem)-read), dirty_str, (signed long)((ctx->g_farps_rcv_mem+strlen(ctx->g_farps_rcv_mem)-read)*2+1));
			softap_printf(USER_LOG, WARN_LOG, "dirty AT:%s\r\n",dirty_str);
			xy_free(dirty_str);

			at_netdog.tail_dirty_sum++;
			return 1;
		}
	}

	//0XFFFFFF	maybe exist
	if(check_str != ctx->g_farps_rcv_mem)
	{
		temp_chr = ctx->g_farps_rcv_mem;
		while(*check_str != '\0')
		{
			*temp_chr = *check_str;
			check_str++;
			temp_chr++;
		}
		*temp_chr = '\0';
		ctx->g_have_rcved_len = temp_chr-ctx->g_farps_rcv_mem;
		*ppchr = memchr(ctx->g_farps_rcv_mem, '\r', ctx->g_have_rcved_len);
		return 0;		
	}	
	return 0;
}

/**
 * @brief 主要用于xdsend灌包命令的连发，xdsend命令连续下发不判定8007
 * @param  ctx  at_context上下文
 * @param  p_current_chr  当前'\r'的位置
 * @example 其他类似于at+cgsn=1\r\nat+cimi\r\n 这样的命令也可以处理
 */
void refresh_store_atcmd(at_context_t *ctx, char *p_current_chr)
{
	char *pchr = ctx->g_farps_rcv_mem;
	while((p_current_chr-ctx->g_farps_rcv_mem) < ctx->g_have_rcved_len && (*p_current_chr == '\r' || *p_current_chr == '\n' || *p_current_chr == '\0'))
		p_current_chr++;
	while (*p_current_chr != '\0' || (p_current_chr - ctx->g_farps_rcv_mem) < ctx->g_have_rcved_len)
	{
		*pchr = *p_current_chr;
		p_current_chr++;
		pchr++;
	}
	*pchr = '\0';
	ctx->g_have_rcved_len = (int)(pchr-ctx->g_farps_rcv_mem);
}

extern void soft_sample_print_8003_log(char *rcvd_mem, int rcvd_len);
extern void soft_sample_print_utc_data(void);
void at_recv_from_uart(char *buf, unsigned long data_len, int fd)
{
	at_context_t *ctx = search_at_context(fd);
	xy_assert(ctx != NULL);

	char *pchr = NULL;
	int datalen_offset = ctx->g_have_rcved_len;

	//透传模式
	if (g_at_passthr_hook != NULL && g_at_passthr_hook(buf, data_len) == XY_OK)
		return;

	//接收AT命令过程中，有sleep动作，需要关闭standby
	at_standby_set();

	if (ctx->g_farps_rcv_mem == NULL)
	{
		ctx->g_farps_rcv_mem = xy_zalloc(AT_CMD_MAX_LEN);
		ctx->g_rcv_buf_len = AT_CMD_MAX_LEN;
	}
	if ((unsigned long)(ctx->g_rcv_buf_len) < (unsigned long)(ctx->g_have_rcved_len) + data_len + 1)
		realloc_farps_rcv_mem(ctx->g_rcv_buf_len + AT_CMD_MAX_LEN, ctx);
	memcpy(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len, buf, data_len);
	ctx->g_have_rcved_len += data_len;
	*(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len) = '\0';

PROC_AGAIN:
	if ((pchr = memchr(ctx->g_farps_rcv_mem + datalen_offset, '\r', data_len)) != NULL)
	{
		// 8003 utc sample success, print log
		soft_sample_print_utc_data();

		if (!find_xdsend_cmd(ctx->g_farps_rcv_mem) && atstr_is_dirty(ctx, &pchr))
		{
			at_netdog.dirty_sum++;
			AT_ERR_TO_PERIPHERAL(ATERR_DROP_MORE, ctx);
            start_at_standby_timer();
            soft_sample_print_8003_log(ctx->g_farps_rcv_mem, ctx->g_have_rcved_len); // standby wakeup 8003 debug
			goto END;
		}
		else if (ctx->g_have_rcved_len > AT_DATA_MAX_LEN)
		{
			AT_ERR_TO_PERIPHERAL(ATERR_PARAM_INVALID, ctx);
			goto END;
		}
		else
		{
			if (pchr == NULL)
				goto PROC_AGAIN;

			int atcmd_len = (int)(pchr - ctx->g_farps_rcv_mem) + 1;

            //用于模组类客户输入AT命令后默认持有一把锁，待用户发送AT+WORKLOCK=0后再释放锁
			set_lock_by_MCU_at();
			
            //回显模式
            if (g_Echo_mode == 1)
            {
                osMutexAcquire(g_farps_write_m, osWaitForever);
#if VER_QUECTEL
                /* BC95 去掉AT命令回显的\r或者\r\n */
                ctx->farps_write(ctx->g_farps_rcv_mem, atcmd_len - 1);
#else
                ctx->farps_write(ctx->g_farps_rcv_mem, ctx->g_have_rcved_len);
#endif //VER_QUECTEL               
                osMutexRelease(g_farps_write_m);
            }

#if VER_QUECTEL
			natspeed_succ_hook();
#endif
			single_atcmd_proc(ctx, ctx->g_farps_rcv_mem, atcmd_len);
			refresh_store_atcmd(ctx, pchr);

			if (ctx->g_have_rcved_len == 0)
			{
				goto END;
			}
			else
			{
				softap_printf(USER_LOG, WARN_LOG, "goto proc_again");
				datalen_offset = 0;
				data_len = ctx->g_have_rcved_len;
				goto PROC_AGAIN;
			}
		}
	}
	else
	{
#if !WAIT_UNTIL_ATEND
		//if rx fifo is NULL after wait for 2 ms,represent that current transmission has been complete from external MCU
		if (check_channel_fifo() == -1)
		{
			//\n  \0  can not send ERROR
			if (strcmp(ctx->g_farps_rcv_mem, "\n") && strcmp(ctx->g_farps_rcv_mem, "\0"))
			{
				//if((g_farps_rcv_mem[0] != 'A') && (g_farps_rcv_mem[1] == 'T') && (g_farps_rcv_mem[2] == '+'))
				//	xy_assert(0);
				softap_printf(USER_LOG, WARN_LOG, "recv lenth:%d,NO \r:%s", ctx->g_have_rcved_len, ctx->g_farps_rcv_mem);
				at_netdog.all_dirty_sum++;
				AT_ERR_TO_PERIPHERAL(ATERR_DROP_MORE, ctx);
                start_at_standby_timer();
				soft_sample_print_8003_log(ctx->g_farps_rcv_mem, ctx->g_have_rcved_len); // standby wakeup 8003 debug
			}
			else
			{
				softap_printf(USER_LOG, WARN_LOG, "recv :%02x", *ctx->g_farps_rcv_mem);
			}
			goto END;
		}
		else
			return;
#else
		if (!strcmp(ctx->g_farps_rcv_mem, "\n") || !strcmp(ctx->g_farps_rcv_mem, "\0"))
		{
			softap_printf(USER_LOG, WARN_LOG, "cvte rcv :%02x", *ctx->g_farps_rcv_mem);
			goto END;
		}
		else
			return;
#endif //WAIT_UNTIL_ATEND
	}
END:
	xy_free(ctx->g_farps_rcv_mem);
	ctx->g_farps_rcv_mem = NULL;
	ctx->g_have_rcved_len = 0;
	ctx->g_rcv_buf_len = 0;
}


//该接口会在idle线程调用，内部不能调用softap_printf接口，否则会有死机风险
int drop_unused_urc(char *buf)
{
	while(*buf == '\r' || *buf == '\n')
		buf++;

	if(g_softap_fac_nv->close_urc == 1 || *(uint32_t*)(BACKUP_MEM_RF_MODE) == 1)
		return urc_can_drop(buf);

	if(drop_unused_urc_hook(buf) == XY_OK)
		return  XY_OK;

/*
	//对于芯翼芯片自身的RTC唤醒，不上报非下行数据包的URC；一旦用户申请锁，后续的URC皆正常上报
	if (g_softap_fac_nv->off_debug == 1 && !CHECK_SDK_TYPE(OPERATION_VER) && (get_locknum_by_type(LOCK_EXT) + get_locknum_by_type(LOCK_DELAY)) == 0)
	{
		//eDXR/TAU唤醒，是否上报URC，open_tau_urc为0时不上报
		if (g_RTC_wakeup_type == 1 && g_softap_fac_nv->open_tau_urc == 0 && urc_can_drop(buf) == XY_OK)
			return XY_OK;

		//user UTC wakeup
		if (g_RTC_wakeup_type == 2 && g_softap_fac_nv->open_rtc_urc == 0 && urc_can_drop(buf) == XY_OK)
			return XY_OK;
	}
*/

	if (!at_strncasecmp(buf, "+TCPACK:", strlen("+TCPACK:")))
		return XY_OK;

	if (g_softap_fac_nv->off_debug == 1 && !at_strncasecmp(buf, "+DBGINFO:", strlen("+DBGINFO:")))
			return XY_OK;

#if WAKEUP_MCU_BY_URC
	wakeup_MCU_by_URC();
#endif
	return XY_ERR;
}

extern bool is_spec_passthr_end_symbol(char *buf);
//the caller need to free the memory--buf

void at_write_all_to_uart(void *buf, int size)
{
    if (g_softap_fac_nv->close_urc == 1)
        return;
    osMutexAcquire(g_farps_write_m, osWaitForever);
    at_write_to_uart(buf, size);
    osMutexRelease(g_farps_write_m);
}

int at_farps_fd_write(int farps_fd, void *buf, int size)
{
	int at_rsp_no = -1;
	at_context_t *ctx = NULL;

	//不能在锁中断状态下调用该接口！
	if (osCoreGetState() == osCoreInCritical)
		xy_assert(0);

	ctx = search_at_context(farps_fd);
	xy_assert(ctx != NULL);

	//返回OK或者错误,以及接收到透传模式退出字符的时候处理！
	if (is_result_at(buf, &at_rsp_no) || is_spec_passthr_end_symbol(buf))
	{
		ctx->process_state = AT_PROCESS_DONE;
		//update err no
		if(at_rsp_no != AT_OK)
		{
			size = strlen(buf);
			ctx->error_no = at_rsp_no;
		}
		reset_ctx(ctx);

		if (farps_fd == FARPS_UART_FD && at_rsp_no == AT_OK)
		{
			at_standby_clear();
		}
	}
	else if ((ctx->fwd_ctx == NULL || strstr((char *)buf, ctx->fwd_ctx->at_cmd_prefix) == NULL) &&
			 drop_unused_urc((char *)buf) == XY_OK)
	{
		return 1;
	}

	//Send rsp or result to original sender
	if (ctx->farps_write != NULL)
	{
		if (farps_fd == FARPS_UART_FD)
		{
			osMutexAcquire(g_farps_write_m, osWaitForever);
			softap_printf(USER_LOG, WARN_LOG, "AT farps_write:%s", buf);
			ctx->farps_write(buf, size);

#if VER_QUECTEL
			natspeed_change_hook();
#endif
			ipr_change_hook();
			osMutexRelease(g_farps_write_m);
		}
	}
	else
	{
		softap_printf(USER_LOG, WARN_LOG, "farps write for context: [%d] is null", ctx->fd);
	}

	return 1;
}

void send_debug_str_to_at_peripheral(char *buf, int fd)
{
	if (g_softap_fac_nv->off_debug == 1)
		return;

	switch (fd)
	{
	case FARPS_UART_FD:
		send_debug_str_to_at_uart(buf);
		break;
	default:
		break;
	}
}

/*only used for sending debug string to AT UART,maybe drop string when in lock int state.
such as "+DBGINFO:",normal log can not send by it*/
void send_debug_str_to_at_uart(char *buf)
{
	if(g_softap_fac_nv->off_debug == 1)
 		return;
	
	int coreState = osCoreGetState();

	if (drop_unused_urc((char *)buf) == XY_OK)
		return;
	
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() == osOK && coreState != osCoreInCritical)
	{
		return;
	}

	//if not lock int,use mutex to protect
	if (coreState != osCoreInCritical && g_farps_write_m != NULL && osKernelGetState() == osKernelRunning)
		osMutexAcquire(g_farps_write_m, osWaitForever);
	//if lock int, and other normal task is sending AT to uart, drop current string
	else if (coreState == osCoreInCritical && g_farps_write_m != NULL && osMutexGetOwner(g_farps_write_m) != NULL)
		return;

	at_write_to_uart(buf, strlen(buf));

	if (coreState != osCoreInCritical && g_farps_write_m != NULL && osKernelGetState() != osKernelInactive)
		osMutexRelease(g_farps_write_m);
}
