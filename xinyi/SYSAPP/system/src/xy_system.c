/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_system.h"
#include "at_ps_cmd.h"
#include "at_worklock.h"
#include "xy_at_api.h"
#include "low_power.h"
#include "oss_nv.h"
#include "hal_adc.h"
#include "xy_utils.h"
#include "xy_hwi.h"
#include "utc.h"
#include "ps_netif_api.h"
#include "sys_init.h"
#include "hw_prcm.h"
#include "flash_adapt.h"
#include "osal.h"
#include "ipcmsg.h"
#include "csp.h"
#include "xy_sys_hook.h"
#include "shm_msg_api.h"
#include "at_ctl.h"
#include "dsp_process.h"
#include "xy_watchdog.h"
#include "at_uart.h"
#include "rtc_tmr.h"
#include "xy_ps_api.h"
#include "softap_nv.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define HARDVER_LEN 	20
#define VERSIONEXT_LEN 	28
#define MODULVER_LEN 	20

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
//++ or --,default into standby
int g_user_off_standby = 0;

//0,save fac/var/invar NV,and into deepsleep directly,can wakeup by WAKEUP_PIN;1,save fac/var/invar NV,and not into DEEPSLEEP,wait MCU to OFF power for slow discharge of capacitor
int  g_fast_off = -1; 

osTimerId_t g_xy_user_dog_timer = NULL;

#if VER_QUECTEL
static reboot_reason_t poweron_reason_list[] = 
{
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_RESET_PIN),
	POWER_ON_IT(REBOOT_CAUSE_APPLICATION_AT),
	POWER_ON_IT(REBOOT_CAUSE_APPLICATION_RTC),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_EXTERNAL_PIN),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_WATCHDOG),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_RESET_UNKNOWN),
};
#endif

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
#if VER_QUECTEL
char *get_power_on_string(int reboot_num)
{
	int i;
	for (i = 0; i < (int)(sizeof(poweron_reason_list) / sizeof(poweron_reason_list[0])); ++i)
	{
		if (poweron_reason_list[i].reboot_num == reboot_num)
			return poweron_reason_list[i].reboot_info;
	}
	return NULL;
}
#endif

uint32_t get_sys_up_stat()
{
	
	static uint32_t g_sys_up_stat=0xffffffff;  

	if(g_sys_up_stat==0xffffffff)
	{
		g_sys_up_stat = sys_upstate_get();
		xy_assert(g_sys_up_stat != UNKNOWN_ON);
		HWREG(SYS_UP_REASON) = g_sys_up_stat;

		if(g_sys_up_stat == SOFT_RESET)
		{
			HWREG(SOFT_RESET_REASON) = HWREG(BACKUP_MEM_RESET_FLAG);
		}
		HWREG(BACKUP_MEM_RESET_FLAG) = 0xffffffff;// clear this flag 
	}

	return g_sys_up_stat;
}

uint32_t get_softreset_type()
{
    return HWREG(SOFT_RESET_REASON);
}

void xy_work_lock(int lock_type)
{
	if(1 != increase_worklock(lock_type))
	{
		xy_assert(0);
	}
}

void xy_work_unlock(int lock_type)
{
	//stop to use NB protocol stack
	if (decrease_worklock(lock_type) != 1)
	{
		xy_assert(0);
	}	
}

extern void do_unlock_all(int fastoff);
//0，仅保存NV后，正常进入DEEPSLEEP；1，保存NV后，lpm_func内通过判断g_fast_off进行锁中断，等待客户断电，解决电容放电问题
void xy_fast_power_off(int abnormal_off)
{
    int i = RTC_TIMER_AP_BASE;

    g_fast_off = abnormal_off;
	do_unlock_all(1);

    if (!DSP_IS_NOT_LOADED())
    {
        //执行fastoff情况下，删除芯翼自己的RTC事件，否则powerdown仍有RTC相关打印
        for (; i < RTC_TIMER_AP_USER_BASE; i++)
            rtc_event_delete(i);

    	int lock_msg = MSG_FASTOFF;
		g_softap_var_nv->user_lock_state = 0;

		xy_atc_interface_call("AT+CFUN=5\r\n", NULL, (void*)NULL);
		//such as AT+WORKLOCK=0,but not send RAI to NB base,and not care lock num++
        softap_printf(USER_LOG, WARN_LOG, "do AT+FASTOFF");
		send_shm_msg(SHM_MSG_PLAT_WORKLOCK,&lock_msg,sizeof(int));
    }
}

void xy_Dynamic_load_DSP()
{
    dynamic_load_up_dsp(0);
}

//user reboot Soc,param specify which NV to save,can not call by time/RTC hook
//RTC timer still valid
void xy_reboot()
{
    send_debug_str_to_at_uart("+DBGINFO:USER call reboot\n");
    reboot_nv();

    HWREG(PS_START_SYNC) = 0;

    soft_reset_by_wdt(RB_BY_NRB);
}

void xy_standby_lock()
{
	osCoreEnterCritical();
    g_user_off_standby++;
	osCoreExitCritical();
}

void xy_standby_unlock()
{
	osCoreEnterCritical();
    if(g_user_off_standby != 0)
    {
    	g_user_off_standby--;
    }
	osCoreExitCritical();
}

void xy_standby_clear()
{
	osCoreEnterCritical();
   	g_user_off_standby = 0;
	osCoreExitCritical();
}

//TAU not timeout,and IP addr still valid
int is_powenon_from_deepsleep()
{
	if(g_normal_wakeup == 1)
    	return  XY_OK;
	else
		return  XY_ERR;
}


extern unsigned char otp_valid_flag;
extern short  get_ADC_val(HAL_ADCMode_TypeDef Mode);
unsigned int xy_getVbat()
{
    short cal_vol;

	cal_vol = get_ADC_val(HAL_ADC_MODE_TYPE_VBAT);
	
	return cal_vol;
}

void xy_kill_user_dog()
{
	if(g_xy_user_dog_timer != NULL)
		osTimerDelete(g_xy_user_dog_timer);
	g_xy_user_dog_timer = NULL;
}

void xy_user_dog_set(int sec)
{
	osTimerAttr_t timer_attr = {0};

	if(g_softap_fac_nv->deepsleep_enable==0 || sec==0) 
		return;

	if(g_xy_user_dog_timer != NULL)
		xy_kill_user_dog();
	timer_attr.name = "dog";
	g_xy_user_dog_timer = osTimerNew((osTimerFunc_t)user_dog_timeout_hook, osTimerOnce, NULL,&timer_attr);
	osTimerStart(g_xy_user_dog_timer,sec * 1000);
}

int xy_get_HARDVER(char *hardver, int len)
{
	if (len < HARDVER_LEN)
		return XY_ERR;
	snprintf(hardver, HARDVER_LEN, "%s", g_softap_fac_nv->hardver);
	return XY_OK;
}

int xy_get_VERSIONEXT(char *versionExt, int len)
{
	if (len < VERSIONEXT_LEN)
		return XY_ERR;
	snprintf(versionExt, VERSIONEXT_LEN, "%s", g_softap_fac_nv->versionExt);
	return XY_OK;
}

int xy_get_MODULVER(char *modul_ver, int len)
{
	if (len < MODULVER_LEN)
		return XY_ERR;
	snprintf(modul_ver, MODULVER_LEN, "%s", g_softap_fac_nv->modul_ver);
	return XY_OK;
}

short  get_ADC_val(HAL_ADCMode_TypeDef Mode)
{
	short real_val;       //从寄存器获取的真实值
	short convert_val;    //转换值
	short cal_vol;        //电压值,mv
	int average = 0;

	HAL_ADC_HandleTypeDef ADC_HandleStruct={0};

	ADC_HandleStruct.Init.Mode = Mode;
	ADC_HandleStruct.Init.Speed = HAL_ADC_SPEED_TYPE_240K;
	ADC_HandleStruct.Init.ADCRange = HAL_ADC_RANGE_TYPE_1V;

	HAL_ADC_Init(&ADC_HandleStruct);

	//从寄存器获取真实值10次取平均值
	for(uint8_t i = 0; i < 10; i++)
	{
		real_val = HAL_ADC_GetValue(&ADC_HandleStruct);//HAL_ADC_GetValue(&ADC_HandleStruct);
		average += real_val;
	}

	if(average >= 0)
		real_val = (average + 5) / 10;
	else
		real_val = (average - 5) / 10;

	
	//获取转换值
	convert_val =  HAL_ADC_GetConverteValue(&ADC_HandleStruct, real_val);
	
	if(otp_valid_flag == 1)   
		cal_vol     = HAL_ADC_GetVoltageValueWithCal(&ADC_HandleStruct, convert_val);//校准的后的电压值
	else 
		cal_vol  = HAL_ADC_GetVoltageValue(&ADC_HandleStruct, convert_val);//未校准的电压值

	HAL_ADC_DeInit(&ADC_HandleStruct);

	return  abs(cal_vol);
}

