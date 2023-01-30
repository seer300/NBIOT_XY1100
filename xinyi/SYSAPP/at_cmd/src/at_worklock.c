/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_worklock.h"
#include "xy_at_api.h"
#include "at_global_def.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "shm_msg_api.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "at_com.h"
#include "xy_ps_api.h"
#include "at_ps_cmd.h"
#include "softap_nv.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
osTimerId_t deepsleep_delay_timer = NULL;
osThreadId_t g_send_rai_proxy_handle = NULL;
osMutexId_t send_rai_proxy_handle_mux = NULL;
worklock_t *lock_head = NULL;
osMutexId_t lock_mux = NULL;

/*******************************************************************************
 *						Local function implementations 					       *
 ******************************************************************************/
void send_rai_proxy(void)
{
    send_debug_str_to_at_uart("+DBGINFO:[RAI] send_rai_proxy begin \r\n");
    uint8_t delay_num  = 0;
    while(HAVE_FREE_LOCK == 1)
    {
    	//释放锁时没有待发送上行数据
        if (g_udp_latest_seq == 0)
        {
            send_debug_str_to_at_uart("+DBGINFO:[RAI] send AT+RAI=1 \r\n");
            xy_send_rai();
            goto end;
        }
		//释放锁后仍然有待发送数据，等待发送完成后再触发RAI
        else
        { 
            send_debug_str_to_at_uart("+DBGINFO:[RAI] wait seq \r\n");
            delay_num++;
            if(delay_num > 10)
            {
                send_debug_str_to_at_uart("+DBGINFO:[RAI] send AT+RAI=1 \r\n");
				xy_send_rai();
                goto end;
            }
            osDelay(1000);
        }
    }
end:
    g_send_rai_proxy_handle = NULL;
    osThreadExit();
}

static void worklock_debug(int increase, int type, int at_spec)
{
	if (increase == 1)
	{
		if (at_spec == 1)
			softap_printf(USER_LOG, WARN_LOG, "increase lock %d by first AT cmd !\r\n", type);
		else
			softap_printf(USER_LOG, WARN_LOG, "increase lock %d !\r\n", type);
	}
	else if (increase == 0)
	{
		if (type == LOCK_MAX)
			softap_printf(USER_LOG, WARN_LOG, "decrease all locks\r\n");
		else
			softap_printf(USER_LOG, WARN_LOG, "decrease lock %d !\r\n", type);
	}

	if (g_softap_fac_nv->off_debug == 1)
		return;

	char *debug_info = xy_zalloc(64);
	if (increase == 1)
	{
		if (at_spec == 1)
		{
			snprintf(debug_info, 60, "\r\n+DBGINFO:[WORKLOCK]increase lock %d by first AT cmd !\r\n", type);
			send_debug_str_to_at_uart(debug_info);
		}
		else
		{
			snprintf(debug_info, 60, "\r\n+DBGINFO:[WORKLOCK]increase lock %d !\r\n", type);
			send_debug_str_to_at_uart(debug_info);
		}
	}
	else if (increase == 0)
	{
		if (type == LOCK_MAX)
			snprintf(debug_info, 60, "\r\n+DBGINFO:[WORKLOCK]decrease all locks !\r\n");
		else
			snprintf(debug_info, 60, "\r\n+DBGINFO:[WORKLOCK]decrease lock %d !\r\n", type);
		send_debug_str_to_at_uart(debug_info);
	}

	xy_free(debug_info);
	return;
}

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
void create_send_rai_task()
{
	osThreadAttr_t thread_attr = {0};

	if (g_softap_fac_nv->close_rai == 1)
		return;
	send_debug_str_to_at_uart("+DBGINFO:[RAI] create_send_rai_task \r\n");
	osMutexAcquire(send_rai_proxy_handle_mux, osWaitForever);
	if (g_send_rai_proxy_handle == NULL)
	{
		thread_attr.name = "send_rai_process";
		thread_attr.stack_size = 0x300;
		thread_attr.priority = osPriorityLow;

		g_send_rai_proxy_handle = osThreadNew((osThreadFunc_t)(send_rai_proxy), NULL, &thread_attr);
	}
	osMutexRelease(send_rai_proxy_handle_mux);
}

void deepsleep_delay_callback()
{
	xy_work_unlock(LOCK_DELAY);
}

//当存在外部锁时，不执行延迟锁；释放锁后立即停该延迟锁
void xy_delaylock_active()
{
	osTimerAttr_t timer_attr = {0};

	osMutexAcquire(lock_mux, osWaitForever);
	//外部已经持有worklock锁情况下不做处理，如已经下发了at+worklock=1
	if (g_softap_fac_nv->deepsleep_enable == 0 || g_softap_fac_nv->deepsleep_delay == 0 || get_locknum_by_type(LOCK_EXT) >= 1)
	{
		osMutexRelease(lock_mux);
		return;
	}

	if (deepsleep_delay_timer == NULL)
    {
        xy_work_lock(LOCK_DELAY);
		timer_attr.name = "deepsleep_delay";
		deepsleep_delay_timer = osTimerNew((osTimerFunc_t)deepsleep_delay_callback, \
            osTimerOnce, NULL, &timer_attr);
        osTimerStart(deepsleep_delay_timer, g_softap_fac_nv->deepsleep_delay * 1000);
    }
    else
    {
		if (osTimerIsRunning(deepsleep_delay_timer) == 0)
		{
			xy_work_lock(LOCK_DELAY);
		}	
		osTimerStart(deepsleep_delay_timer, g_softap_fac_nv->deepsleep_delay * 1000);
	}
	osMutexRelease(lock_mux);
}

void xy_delaylock_suspend()
{
	osMutexAcquire(lock_mux, osWaitForever);
    if (g_softap_fac_nv->deepsleep_enable == 0 || g_softap_fac_nv->deepsleep_delay == 0 || lock_head[LOCK_DELAY].num == 0)
    {
		osMutexRelease(lock_mux);
        return;
    }
	if (deepsleep_delay_timer != NULL && osTimerIsRunning(deepsleep_delay_timer) == 1)
	{
		osTimerStop(deepsleep_delay_timer);
	}
	lock_head[LOCK_DELAY].num = 0;
	worklock_debug(0, LOCK_DELAY, 0);
	osMutexRelease(lock_mux);
}

static int have_lock_ext = 0;
int increase_worklock(int type)
{
	if (g_softap_fac_nv->deepsleep_enable == 0)
		return 1;

	osMutexAcquire(lock_mux, osWaitForever);

	HAVE_FREE_LOCK = 0;

	//收到外部MCU发送的AT命令
	if (type == LOCK_EXT)
	{
		//外部MCU第一次输入AT+WORKLOCK=1时，不累加，因为第一条AT命令会调用set_lock_by_MCU_at接口申请一把锁
		if (lock_head[type].num == 1 && have_lock_ext == 0 && g_softap_fac_nv->ext_lock_disable == 0)
		{
			have_lock_ext = 1;
			softap_printf(USER_LOG, WARN_LOG, "first AT+WORKLOCK=1, lock num don't add !\r\n");
			//外部MCU第一次输入AT命令时会默认申请一把锁，此时不需要发送核间信息给DSP
			osMutexRelease(lock_mux);
			return 1;
		}
		else
		{
			lock_head[type].num++;
			//外部MCU已介入，删除延迟锁
			xy_delaylock_suspend();
		}
	}
	else
		lock_head[type].num++;

	worklock_debug(1, type, 0);

	//只要有需要使用DSP的锁存在，就必须通知DSP核持有锁
	if ((type == LOCK_DEFAULT || type == LOCK_EXT) && (lock_head[LOCK_DEFAULT].num + lock_head[LOCK_EXT].num) == 1 && !DSP_IS_NOT_LOADED())
	{
		int lock_msg = MSG_LOCK;
		g_softap_var_nv->user_lock_state = 1;
		send_shm_msg(SHM_MSG_PLAT_WORKLOCK, &lock_msg, sizeof(int)); //SHM_MSG_PLAT_WORKLOCK
	}
	
	osMutexRelease(lock_mux);
	return 1;
}

int decrease_worklock(int type)
{
	if (g_softap_fac_nv->deepsleep_enable == 0)
		return 1;

	osMutexAcquire(lock_mux, osWaitForever);

	if (g_softap_fac_nv->deepsleep_delay != 0 && lock_head[type].num == 0)
	{ 
		softap_printf(USER_LOG, WARN_LOG, "worklock %d not matched!ERROR!!!\r\n",type);
	}

	if (lock_head[type].num > 0)
	{
		lock_head[type].num--;

		/*此处是为了解决bug，用户不发AT+WORKLOCK=1申请锁，而是在用完芯片后直接发AT+WORKLOCK=0释放锁，此时have_lock_ext为初始值0，
		如果在进入深睡前又发两次A+WORKLOCK=1，只会申请一次LOCK_EXT,造成锁不匹配。*/
		if (type == LOCK_EXT && lock_head[LOCK_EXT].num == 0)
			have_lock_ext = 1;
		worklock_debug(0, type, 0);
	}

	//用户释放锁后，及时杀延迟锁,并通知DSP核释放锁
	if ((type == LOCK_EXT || type == LOCK_DEFAULT) && (lock_head[LOCK_EXT].num + lock_head[LOCK_DEFAULT].num) == 0)
	{
		xy_delaylock_suspend();
		if (!DSP_IS_NOT_LOADED())
		{
			int lock_msg = MSG_UNLOCK;
			g_softap_var_nv->user_lock_state = 0;
			send_shm_msg(SHM_MSG_PLAT_WORKLOCK, &lock_msg, sizeof(int)); //SHM_MSG_PLAT_WORKLOCK
		}
	}
	
	//当业务相关的锁皆已释放，通知DSP执行快速深睡流程
	if (type != LOCK_DELAY && (lock_head[LOCK_XY_LOCAL].num + lock_head[LOCK_DEFAULT].num + lock_head[LOCK_EXT].num) == 0 \
		&& !DSP_IS_NOT_LOADED())
	{
		softap_printf(USER_LOG, WARN_LOG, "all lock have released!\r\n");
		//产线测试时不插卡,设置NOSIMST为127时协议栈无法进入深睡,此时发AT+WORKLOCK=0释放锁时触发CFUN5可以强制PS软关机，从而进深睡。
		if(g_check_valid_sim == 0)
			//xy_atc_interface_call("AT+CFUN=5\r\n", NULL, (void*)NULL);
			//20230113 wjb add: cause +CFUN:(0,1,5),(0) --> +CFUN: (0,1,4),(0)
			xy_atc_interface_call("AT+CFUN=4\r\n", NULL, (void*)NULL);
		if (type != LOCK_XY_LOCAL)
			create_send_rai_task();
	}

	//若当前所有锁都已经释放，需要通过核间中断捅醒DSP核，以防止DSP在standby浅睡，常见于TAU/eDRX唤醒工作流程
	if ((lock_head[LOCK_XY_LOCAL].num + lock_head[LOCK_DEFAULT].num + lock_head[LOCK_EXT].num + lock_head[LOCK_DELAY].num) == 0)
	{
		HAVE_FREE_LOCK = 1;

		//核间中断，如果DSP已经在standby流程内，会使得后续芯片进入standby而不是deepsleep，需让DSP退出睡眠流程并更新深睡状态信息
		if (!DSP_IS_NOT_LOADED())
		{
			HWREG(PRCM_BASE + 0x1c) |= 0x02;
		}
	
		//没有业务锁，仅申请了延迟锁，常见于异常唤醒SoC
		if (type == LOCK_DELAY)
		{
			softap_printf(USER_LOG, WARN_LOG,"user not alloc lock,and delay into deepsleep!!!\r\n");
		}
	}
	osMutexRelease(lock_mux);
	return 1;
}

int get_locknum_by_type(int type)
{
	int locknum;
	if(osCoreGetState() != osCoreInCritical)
		osMutexAcquire(lock_mux, osWaitForever);
	
	locknum = lock_head[type].num;
	if(osCoreGetState() != osCoreInCritical)
		osMutexRelease(lock_mux);
	return locknum;
}

void set_lock_by_MCU_at()
{
	static int at_lock = 0;
	if (g_softap_fac_nv->deepsleep_enable == 0)
	{
		return;
	}
	//未获取外部锁情况下，外部MCU申请延迟锁
	xy_delaylock_active();

	//Bug 6354-入网入库模式，发AT命令不能加EXT锁，否则无法进深睡
	if (CHECK_SDK_TYPE(OPERATION_VER))
		return;

	//用户锁全部释放后或者ext_lock nv为0，外部MCU不再申请外部锁
	if (at_lock == 0 && g_softap_fac_nv->ext_lock_disable == 0)
	{
		//狭义模组类客户不会匹配使用LOCK锁，进而由平台后台来申请一把锁
		increase_worklock(LOCK_EXT);
		at_lock = 1;
	}
}

/**
 * @brief 清除芯翼内部使用的锁，不清用户相关锁
 * @note 用于NB切换BLE的场景
 */
void clear_xy_worklock()
{
	osMutexAcquire(lock_mux, osWaitForever);
	xy_delaylock_suspend();
	lock_head[LOCK_XY_LOCAL].num = 0;
	//用户未申请锁的情况下，通知dsp释放锁
	if (lock_head[LOCK_DEFAULT].num + lock_head[LOCK_EXT].num == 0)
	{
		int lock_msg = MSG_UNLOCK;
		g_softap_var_nv->user_lock_state = 0;
		softap_printf(USER_LOG, WARN_LOG, "clear_xy_worklock notify dsp");
		send_shm_msg(SHM_MSG_PLAT_WORKLOCK, &lock_msg, sizeof(int));
	}
	osMutexRelease(lock_mux);
}

void do_unlock_all(int fastoff)
{
	osMutexAcquire(lock_mux, osWaitForever);
	xy_delaylock_suspend();
	lock_head[LOCK_XY_LOCAL].num = 0;
	lock_head[LOCK_DEFAULT].num = 0;
	lock_head[LOCK_EXT].num = 0;
	HAVE_FREE_LOCK = 1;

	if(fastoff == 0)
	{
		int lock_msg = MSG_UNLOCK;
		g_softap_var_nv->user_lock_state = 0;
		softap_printf(USER_LOG, WARN_LOG, "clear_all_worklock notify dsp");
		send_shm_msg(SHM_MSG_PLAT_WORKLOCK, &lock_msg, sizeof(int));
		worklock_debug(0, LOCK_MAX, 0);
		create_send_rai_task();
	}
	osMutexRelease(lock_mux);
}

void xy_unlock_all()
{
	do_unlock_all(0);
}

//AT+WORKLOCK=enable,type
int at_WORKLOCK_req(char *at_buf,char **prsp_cmd)
{
	int enable = 0;
	int type = LOCK_EXT;

	if(g_req_type == AT_CMD_REQ)
	{
		if(g_softap_fac_nv->deepsleep_enable == 0)
		{
			return AT_END;
		}

		if (at_parse_param("%d,%d", at_buf, &enable, &type) != AT_OK || type != LOCK_EXT)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if (enable == 1)
		{
			if (1 != increase_worklock(type))
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
				return AT_END;
			}
		}
		else if (enable == 0)
		{
			if (decrease_worklock(type) != 1)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			}
			return AT_END;
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(48);
		snprintf(*prsp_cmd, 48, "\r\n+WORKLOCK:(0-1)[,(0-1)]\r\n\r\nOK\r\n");
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(48);
		if (lock_head != NULL)
		{
			int index = LOCK_DEFAULT;
			sprintf(*prsp_cmd, "\r\n+WORKLOCK:%d", lock_head[index].num);
			while ((++index) < LOCK_MAX)
				sprintf(*prsp_cmd + strlen(*prsp_cmd), ",%d", lock_head[index].num);
			sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n\r\nOK\r\n");
		}
		else
			sprintf(*prsp_cmd, "\r\n+WORKLOCK:0\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}

void worklock_init()
{
	osMutexAttr_t mutex_attr = {0};
	
	mutex_attr.attr_bits = osMutexRecursive;
	lock_mux = osMutexNew(&mutex_attr);
	send_rai_proxy_handle_mux = osMutexNew(NULL);
	if (lock_head == NULL)
		lock_head = xy_zalloc(LOCK_MAX * sizeof(worklock_t));
}
