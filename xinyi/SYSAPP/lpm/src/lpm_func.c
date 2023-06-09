#include "flash_vendor.h"
#include "low_power.h"
#include "prcm.h"
#include "rtc_tmr.h"
#include "prcm.h"
#include "hw_csp.h"
#include "hw_timer.h"
#include "hw_dma.h"
#include "hw_utc.h"
#include "gpio.h"
#include "lpm_config.h"
#include "lpm_adapt.h"
#include "at_global_def.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "softap_nv.h"
#include "ipcmsg.h"
#include "sys_init.h"
#include "at_com.h"
#include "osal.h"
#include "mcnt.h"
#include "rf_drv.h"
#include "utc.h"
#include "lwip/tcpip.h"
#include "xy_sys_hook.h"
#include "watchdog.h"
#include "csp.h"
#include "xy_flash.h"
#include "at_uart.h"



#ifndef MIN
#define MIN(a,b)  ((a)<(b)?(a):(b))
#endif



volatile unsigned long long g_tick_us = 0;
volatile unsigned long g_deepsleep_dsp_ready = 0;  //DSP have into deepsleep
volatile unsigned long g_standby_dsp_ready = 0;  //only linxu can vary;DSP have into standby
volatile unsigned long long g_deepsleep_time = 0;  //DSP deepsleep sleep time
volatile unsigned long long g_standby_time = 0;  //DSP standby sleep time
volatile unsigned long g_32k_to_hclk = 0;
volatile uint8_t g_gpi_wakeupsrc = 0;
static unsigned char s_systick_pending_in_lpmode = 0;

//volatile unsigned char  pin_wakeup_standby = 0; //0,UTC wakeup;1,GPI wakeup;2,WAKEUP pin

#if (STANDBY_LOG == 1)

static struct {
	unsigned char *timername;
	uint32_t tmr_expired_time;
	unsigned char *taskname;
	uint32_t tsk_expired_time;
	uint32_t rtc_id;
	uint8_t core;				//0:ap 1:cp
	uint8_t wkup_source;				//0:utc 1:rtc 2:extpin
}standby_wkup_debug_info = {0};
/*
static uint32_t standby_wkup_tmr_id = -1;
static uint32_t standby_wkup_tmr_expired_time = -1;

static TCB_t *standby_wkup_tcb = NULL;
static uint32_t standby_wkup_tcb_expired_time = -1;

static uint8_t standby_wkup_rtc_id;
*/
void StandbyWkupInfoInit()
{
	standby_wkup_debug_info.timername = NULL;
	standby_wkup_debug_info.tmr_expired_time = -1;
	standby_wkup_debug_info.taskname = NULL;
	standby_wkup_debug_info.tsk_expired_time = -1;
	standby_wkup_debug_info.rtc_id = -1;
	standby_wkup_debug_info.core = 0;
	standby_wkup_debug_info.wkup_source = -1;
}

void StandbyWkupGetWkupSource(uint8_t wkup_source)
{
	standby_wkup_debug_info.wkup_source = wkup_source;
}

void StandbyWkupGetWkupCore(uint8_t core)
{
	standby_wkup_debug_info.core = core;
}

void StandbyWkupGetTmrInfo(unsigned char *timername,uint32_t expired_time)
{
	standby_wkup_debug_info.timername = timername;
	standby_wkup_debug_info.tmr_expired_time = expired_time;
}

void StandbyWkupGetTcbInfo(unsigned char *taskname,uint32_t expired_time)
{
	standby_wkup_debug_info.taskname = taskname;
	standby_wkup_debug_info.tsk_expired_time = expired_time;
}

void StandbyWkupGetRtcID()
{
	struct rtc_event_info * rtc_info = NULL;
	if(standby_wkup_debug_info.wkup_source == 0)
	{
		//utc wkup situation
		rtc_info = rtc_get_next_event();
		if(rtc_info != NULL && is_rtc_event_expired(rtc_info))
		{
			standby_wkup_debug_info.rtc_id = rtc_info-ap_rtc_event_arry;
		}
	}
	
}

void StandbyExitInfoProcess()
{
	//lpmrtc 已经删除，此时获取的是业务或者用户rtc,必须在睡眠流程的临界区被获取
	StandbyWkupGetRtcID();
}
void StandbyExitLogPrint()
{
	
	if(standby_wkup_debug_info.wkup_source == 0 )
	{
		//utc wakeup,not lpm event
		if(standby_wkup_debug_info.rtc_id != (uint32_t)-1) 
		{
			xy_printf("Standby wakeup by rtcevent:%d",standby_wkup_debug_info.rtc_id);
		}
		else
		{
			//lpm event wakeup
			if(standby_wkup_debug_info.core == 0)
			{
				if(standby_wkup_debug_info.tsk_expired_time < standby_wkup_debug_info.tmr_expired_time)
				{
					if(standby_wkup_debug_info.taskname)
						xy_printf("Standby wakeup by AP task:%s",standby_wkup_debug_info.taskname);
					else
						xy_printf("Standby wakeup taskname error");
				}
				else
				{
					if(standby_wkup_debug_info.timername)
						xy_printf("Standby wakeup by AP timer:%s",standby_wkup_debug_info.timername);
					else
						xy_printf("Standby wakeup timername error");
				}
			}
			else
			{
				xy_printf("Standby wakeup by CP");
			}
		}
		
	}
	else if(standby_wkup_debug_info.wkup_source == 1)
	{
		xy_printf("Standby wakeup by gpi");
	}
	else if(standby_wkup_debug_info.wkup_source == 2)
	{
		xy_printf("Standby wakeup by extpin");
	}
	else
	{
		xy_printf("Standby enter fail");
	}
}

#endif

unsigned int sys_upstate_get()
{
	volatile unsigned char up_stat;
	volatile unsigned char sleep_mode;
	volatile unsigned char wakeup_info;
	
	up_stat = HWREGB(0xA0058004);
	sleep_mode = (HWREGB(0xA0058004)&0x10);//deepsleep mode
	wakeup_info = (HWREGB(0xA0058001)&0x3F);
	HWREGB(0xA0058001) = 0;
	//due to the UP_STAT register not clear after wdt_rst,read wdtrst_up_stat bit first
	if(up_stat&0x40)
	{
		if(HWREG(BACKUP_MEM_RESET_FLAG) >= RB_MAX)
		{
			return WDT_RESET;
		}
		else
		{
			return SOFT_RESET;
		}
	}
		

	if(sleep_mode != 0)
	{
		if(wakeup_info&0x01)
			return UTC_WAKEUP;
		else if(wakeup_info&0x02)
			return EXTPIN_WAKEUP;
	}
	
	
	
	if(up_stat & 0x01)
		return POWER_ON;
	else if(up_stat & 0x02)
		return PIN_RESET;
	else if(up_stat & 0x04)
		return SOFT_RESET;
	else if(up_stat & 0x40)
		return WDT_RESET;		

	
	
	return UNKNOWN_ON;

}

void lpm_string_output(void *buf, int size)
{
	int i;
	 //bug5325
	if(size <= 0 || g_autobaud_state == 0 || ((HWREG(AT_UART_CSP+CSP_MODE1) & 0x20) == 0) || g_softap_fac_nv->close_at_uart == 1)
		return;

	if(drop_unused_urc((char *)buf)==XY_OK)
		return;

	csp_out_flag |= 0x01;

	HWREG(AT_UART_CSP+CSP_INT_STATUS) = CSP_INT_CSP_TX_ALLOUT;
	 
	for(i = 0 ; i < size ; i++ )
	{
		CSPCharPut( AT_UART_CSP, *((char *)buf + i));
	}
}

void utc_timer_get(LPM_TIME_T* p_lpmtime)
{
	uint64_t utc_cnt = utc_cnt_get();
	p_lpmtime->ullpmtime_s = utc_cnt/32000;
	p_lpmtime->ullpmtime_us = (utc_cnt%32000)*1000/32;
}
//return MS,this val will near 0XFFFFFFFF,so can not * some val,onlu -
uint64_t global_timepoint_get(uint64_t ull_offset_ms)
{
	LPM_TIME_T utctime;
	utc_timer_get(&utctime);

	return (ull_offset_ms*XY_UTC_CLK/32000 + (uint64_t)utctime.ullpmtime_s*1000 + utctime.ullpmtime_us/1000);
}


void measure_rc32k()
{
	// NV:rc32k_freq 初始值为0，开机作校准，校准后更新NV
	// 若程序需要进行内外部RC切换时，会重新将此值置0。
	uint64_t freq_32k = 0;
	if(g_softap_fac_nv->rc32k_freq == 0)
	{
		//内部RC直接测量，外部RC需要等待起振
		if( g_softap_fac_nv->xtal_32k == 0)
		{
			Switch_32k_Xtal();
		}

		// bug 6803
		//PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_MCNT_EN);
		
		NVIC_DisableIRQ(MEASURECNT_IRQn);
		NVIC_ClearIntPending(MEASURECNT_IRQn);
		
		MCNTStop();
		MCNTSetCNT32K(6400);//about 200ms
		MCNTStart();
		
		while(RESET == NVIC_GetIntPending(MEASURECNT_IRQn));
		// NV:rc32k_freq 中存放的是rc频率的十倍，表示晶振误差为1ppm。
		freq_32k = (uint64_t)XY_HCLK*6400*10/(MCNTGetMCNT()-1);
		g_softap_fac_nv->rc32k_freq = freq_32k;
		SAVE_FAC_PARAM(rc32k_freq);
		
		//出于兼容性考虑，四舍五入，保证误差在10ppm。
		g_XY_RC32K_CLK = (freq_32k + 5) / 10;

	}
}

void lpm_msg_process(LPM_MSG_T * pMsg)
{
	static int only_one=0;
	static int only_once=0;
	unsigned long long msec = pMsg->msec;
	if(pMsg->lpm_mode == LPM_DEEPSLEEP)
	{	
		g_standby_dsp_ready = 0;
		g_deepsleep_dsp_ready = 1;
		rtc_event_delete(RTC_TIMER_LPM);
		//if(pMsg->p_tcmcnt_table)
		//{
		//	memcpy(&g_softap_fac_nv->nv_tcmcnt_table,(void *)((unsigned int)pMsg->p_tcmcnt_table-0X30000000),sizeof(struct tcmcnt_table_t));
		//}
		if(g_fast_off != -1)
		{
			g_softap_var_nv->next_PS_sec = 0;	
		}
		else
		{
			
			if((msec>>32) == 0XFFFFFFFF)
			{
				//TAU Infinite-Length,always not occur
				if(pMsg->ps_state == 1)
				{
					g_softap_var_nv->next_PS_sec = 0XFFFFFFFF;
					
					if(g_softap_fac_nv->off_debug == 0)
					{
						if(only_one == 0)
						{
							only_one = 1;
							send_debug_str_to_at_uart("+DBGINFO:PS TAU FF\r\n");	
						}
					}
				}
				//PS state abnormal,such as not campon.So TAU is invalid,next wakeup PS do attach
				else
				{
					g_softap_var_nv->next_PS_sec = 0;
					
					if(g_softap_fac_nv->off_debug == 0)
					{
						if(only_once == 0)
						{
							only_once = 1;
							send_debug_str_to_at_uart("+DBGINFO:PS FASTOFF\r\n");	
						}
					}
				}
			}
			else
			{		
				xy_assert(msec!=0);
				//TAU RTC maybe not set,but DRX rtc must set
				if(g_softap_fac_nv->xtal_32k == 0 && g_softap_fac_nv->work_mode==0 && ((g_softap_fac_nv->set_tau_rtc==1 && pMsg->ps_state==1) || pMsg->ps_state!=1))
					rtc_event_add_by_global(RTC_TIMER_LPM,msec,NULL,NULL);

				//Record the next TAU time to decide whether to attach or not.
				g_softap_var_nv->next_PS_sec = msec/1000;	
			}	
			g_softap_var_nv->ps_deepsleep_state = pMsg->ps_state;
		}
		g_deepsleep_time = pMsg->msec;
		g_invar_nv=(void *)((int)pMsg->invar_nv);
		g_invar_len=pMsg->invar_len;

		//save only once

			
	}
	else if(pMsg->lpm_mode == LPM_STANDBY)
	{
		//此处主要是避免此前deepsleep状态未能深睡而超时，协议栈睡眠状态转变，需将DSP_STATE_OFFCLOCK转为DSP_STATE_WORKING
		
		g_standby_time = pMsg->msec;
		rtc_event_add_by_global(RTC_TIMER_LPM,g_standby_time,psm_standby_cb,NULL);				
		g_32k_to_hclk =  (MCNTGetMCNT()-1)/3200;
		if(g_32k_to_hclk>(XY_HCLK/XY_UTC_CLK+10)||g_32k_to_hclk<(XY_HCLK/XY_UTC_CLK-10))
			g_32k_to_hclk = XY_HCLK/XY_UTC_CLK;
		g_standby_dsp_ready = 1;
		g_deepsleep_dsp_ready = 0;
	}	
	
}

extern T_IpcMsg_ChInfo g_IpcMsg_ChInfo[IpcMsg_Channel_MAX];
unsigned int icm_buf_check()
{
	T_RingBuf* ringbuf_rcv = g_IpcMsg_ChInfo[IpcMsg_Normal].RingBuf_rcv;
	T_RingBuf* ringbuf_send = g_IpcMsg_ChInfo[IpcMsg_Normal].RingBuf_send;
	T_RingBuf* flash_ringbuf_send = g_IpcMsg_ChInfo[IpcMsg_Flash].RingBuf_send;

	if(Is_Ringbuf_Empty(ringbuf_send)!=true || Is_Ringbuf_Empty(flash_ringbuf_send)!=true)
	{	
		if(!DSP_IS_ALIVE())
			HWREG(PRCM_BASE + 0x1c)|= 0x02;
		return 1;
	}
	if(Is_Ringbuf_Empty(ringbuf_rcv)==true)
		return 0;

	if(Is_Ringbuf_Empty(ringbuf_rcv)!=true)
	{
		IpcMsg_Semaphore_Give(&(g_IpcMsg_ChInfo[IpcMsg_Normal].read_sema));
	}
	
	return 1;
}

void NVIC_CLREN_CLRPEN_ALL()
{
	HWREG(0xE000E180)=0xFFFFFFFF;
	HWREG(0xE000E184)=0xFFFFFFFF;
	__DSB();
	__ISB();
	
	HWREGB(0xA0058001) = 0;
	HWREGB(PRCM_BASE + 0x1C)=0;
	
	HWREG(0xE000E280)=0xFFFFFFFF;
	HWREG(0xE000E284)=0xFFFFFFFF;
	__DSB();
	__ISB();
}


//yus
void  force_into_deep_sleep()
{
	while(csp_write_allout_state() == 0);

	HWREGB(0xA0110018) = 0x00;
	osCoreEnterCritical();

	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	HWREG(0xE000ED04) =((1<<27)|(1<<25));
	while(1)
	{
		NVIC_CLREN_CLRPEN_ALL();
		//rtc_event_delete(RTC_TIMER_ARM_WFI);
		//rtc_event_delete(RTC_TIMER_PS_STANDBY);
		
		if(HWREG(0xe000e200)&(1<<UTC_IRQn))	
		{			
			volatile unsigned long reg_val = HWREG(UTC_BASE + UTC_INT_STAT);//read and clear int status
			(void) reg_val;
			HWREG(0xe000e280) = (1<<UTC_IRQn);//CLRPEND
		}

		
		if(g_softap_fac_nv->keep_retension_mem==0)
		{
			//HWREGB(0xA0058005) = 0x02;//iso bak mem
			HWREGB(0xA0058008) &= 0xFE; //power off bkmen retldo
		}
		else
		{					
			HWREGB(0xA0058005) = 0x00;//not iso bak mem
			HWREGB(0xA0058008) = 0x41; //power on bkmen retldo,and retldo supply power
		}

		HWREGB(0xA0110002) = 0x03;
		HWREGB(0xA0058000) = 0x31;//enable extpin utc wakeup,deepsleep mode
			
		HWREGB(0xE000ED10) |= 0x4;


		
		HWREGB(0xA0058001) = 0;	//clr WAKPUP_INT0
		
		__asm ("WFI") ;
	}
	
}

void get_powerdown_urc(char *at_str)
{
    //if(g_softap_var_nv->ps_deepsleep_state != 2 || get_sys_up_stat() == EXTPIN_WAKEUP || get_sys_up_stat() == POWER_ON )/*20230310 MG*/
	//if(g_softap_var_nv->ps_deepsleep_state != 2 || get_sys_up_stat() != UTC_WAKEUP)
	//printf("\r\n[ENTER]ps_deepsleep_state=%d, g_RTC_wakeup_type=%d\r\n", g_softap_var_nv->ps_deepsleep_state, g_RTC_wakeup_type);
	//20230319 MG
	//factoryNV parameter ucEdrxEnableFlag set 0(now edrx defalut closed) to reduce ps/edrx rtc timer effect
	//under psm, the normal urc and platform lifetime update urc, under edrx, also consider platform lt update urc
	if(g_softap_fac_nv->mgedrxrpt_ind == 1 && g_softap_fac_nv->deepsleep_urc == 1 && g_softap_var_nv->sleep_mode == 1)
		sprintf(at_str,"\r\n+QNBIOTEVENT: \"ENTER DEEPSLEEP\"\r\n");
	
	else if((g_softap_fac_nv->mgedrxrpt_ind == 0) && (g_softap_var_nv->ps_deepsleep_state != 2 || get_sys_up_stat() != UTC_WAKEUP || (g_softap_var_nv->ps_deepsleep_state == 2 && g_RTC_wakeup_type == 2)))
	{
	    if(g_softap_fac_nv->deepsleep_urc == 1 && g_softap_var_nv->sleep_mode == 1)
	    {
		    sprintf(at_str,"\r\n+QNBIOTEVENT: \"ENTER DEEPSLEEP\"\r\n");
	    }
    }
	//MG END
}

void lpm_add_ticks(uint64_t utc_cnt_sleep)
{
	unsigned long uwCyclesPerTick = OS_SYS_CLOCK / configTICK_RATE_HZ;  
	unsigned long long ticks_sleep;
	unsigned int ultickleft;

	ticks_sleep = utc_cnt_sleep*g_32k_to_hclk/uwCyclesPerTick;
	if(s_systick_pending_in_lpmode)
		ticks_sleep++;
	g_tick_us+=(utc_cnt_sleep*g_32k_to_hclk%uwCyclesPerTick)*1000/uwCyclesPerTick;
	ultickleft = g_tick_us/1000;
	if(ticks_sleep||ultickleft)
	{
		lpm_osTickHandlerLoop(ticks_sleep+ultickleft);
		if(ultickleft)
			g_tick_us -= (ultickleft*1000);
	}
}





uint64_t  utc_cnt_get()
{
	struct rtc_time rtctime = {0};
	unsigned char ulCentury;
	unsigned char ulYear;
	unsigned char ulMonth;
	unsigned char ulData;
	unsigned char ulDay;
	unsigned char ulAMPM;
	unsigned char ulHour;
	unsigned char ulMin;
	unsigned char ulSec;
	unsigned char ulMinSec;
	unsigned int  ulClkCnt;
	volatile unsigned long 	ulcalReg ;
	volatile unsigned long  ultmrReg ;
	volatile unsigned long  /*ulpsintlevel,*/cnttmp;
	volatile unsigned long utc_delay;
	volatile unsigned char 	carryflag = 0 ;
	lpm_EnterCritical();

	cnttmp = HWREG(UTC_BASE + UTC_CLK_CNT);
	
	while(cnttmp==159||cnttmp==415)
	{
		cnttmp = HWREG(UTC_BASE + UTC_CLK_CNT);
		carryflag=1;
	}	
	if(carryflag)             //bug3356
		for(utc_delay=0;utc_delay<10;utc_delay++);
	ulcalReg = HWREG(UTC_BASE + UTC_CAL);
	ultmrReg = HWREG(UTC_BASE + UTC_TIMER);	
	ulClkCnt = UTCClkCntConvert(cnttmp);
	
	lpm_ExitCritical();
	
	UTCCalGet(&ulCentury,&ulYear, &ulMonth,&ulData,&ulDay,ulcalReg);
	UTCTimerGet(&ulAMPM, &ulHour, &ulMin,&ulSec, &ulMinSec,ultmrReg);
	rtctime.tm_hour = ulAMPM*12+ulHour;
	rtctime.tm_min = ulMin;
	rtctime.tm_sec = ulSec;
	rtctime.tm_mday = ulData;
	rtctime.tm_mon = ulMonth;
	rtctime.tm_year = ulCentury*100 + ulYear;
	rtctime.tm_wday = ulDay;
	rtctime.tm_msec = ulMinSec*10;

	return (xy_mktime(&rtctime)*32 + ulClkCnt);
}


uint64_t  utc_ms_get()
{
	return (utc_cnt_get()/32);
}

#if  (XY_WFI==1)

void LPM_ARM_Entry_Wfi()
{	
	HWREGB(AP_LPM_STATE)=LPM_WFI;
	HWREGB(0xA0058000) = 0x00;
    
    HWREGB(0xE000ED10) &= ~0x4;
	
	__asm ("WFI") ;
	HWREGB(AP_LPM_STATE)=LPM_ACTIVE;
	
}

#endif

#if (XY_DEEPSLEEP==1)

volatile unsigned long g_nvic_pend_l = 0;
volatile unsigned long g_nvic_pend_h = 0;
volatile unsigned char g_wakeup_info = 0;
extern volatile unsigned char g_BKMEMCTL_reg;
volatile unsigned long g_nvic_enable_l = 0;
volatile unsigned long g_nvic_enable_h = 0;
volatile unsigned long g_wakeupint_source = 0;

void LPM_ARM_Entry_Deep_Sleep()
{
	volatile unsigned long delay;

	HWREGB(0xA0058008) = g_BKMEMCTL_reg; 
	for(delay=10;delay>0;delay--);//bug5166,try
	HWREGB(AP_LPM_STATE)=LPM_DEEPSLEEP;
	HWREGB(0xA011002C)&=(~(1<<4)); //pll_lock from  analog	
  //while(while1)
	{	
	
		HWREGB(0xA0058000) = 0x31;//enable extpin utc wakeup,deepsleep mode
		HWREGB(0xE000ED10) |= 0x4;
		//HWREGB(0xA0110018) = 0;		//DSPCLKRSTCTL:reset dsp core
		

		HWREGB(0xA0058001) = 0;	//clr WAKPUP_INT0
		//HWREGB(0xA0058008) &= 0xEF;//retldo supply power
		
		//HWREGB(0xA0110008) &= 0xDF;	//disable utc_clk_en	
		
		__asm ("WFI") ;
		HWREGB(AP_LPM_STATE)=LPM_ACTIVE;
		HWREGB(0xA0058008) = 0x11; 
		HWREGB(0xA0058000) = 0;
		g_nvic_pend_l = HWREG(0xe000e200);
		g_nvic_pend_h = HWREG(0xe000e204);
		g_nvic_enable_l= HWREG(0xe000e100);
		g_nvic_enable_h= HWREG(0xe000e104);
		g_wakeupint_source = HWREG(0xa0058001);

#if 0				
		delay=100;
		HWREGB(0xa0110008)|=0x20;
		HWREGB(0xA0110018) = 0x21;
		while(delay--);
		if(g_wakeup_info==0)
			g_wakeup_info = HWREGB(0xA0058001);
		//LOS_IntUnLock();
		HWREGB(0xA0058001) = 0;
		HWREGB(PRCM_BASE + 0x1C)=0;
		HWREG(0xE000E280)=0xFFFFFFFF;
		HWREG(0xE000E284)=0xFFFFFFFF;
		__DSB();
		__ISB();
		delay=100;
		while(delay--);
#endif
  }   
  HWREGB(0xA011002C)|=(1<<4); //force pll_lock to digital

}

void LPM_ARM_Entry_Deep_Sleep_fast_off()
{
	volatile unsigned long delay;
	HWREGB(AP_LPM_STATE)=LPM_DEEPSLEEP;
	HWREGB(0xA011002C)&=(~(1<<4)); //pll_lock from  analog	

  //while(while1)
	{	
				
		HWREGB(0xA0058000) = 0x31;//enable extpin utc wakeup,deepsleep mode
			
		HWREGB(0xE000ED10) |= 0x4;

		//HWREGB(0xA0110018) = 0;		//DSPCLKRSTCTL:reset dsp core
		

		
		HWREGB(0xA0058001) = 0;	//clr WAKPUP_INT0
		
		//HWREGB(0xA0058008) &= 0xEF;//retldo supply power
		
		//HWREGB(0xA0110008) &= 0xDF;	//disable utc_clk_en	
		
		__asm ("WFI") ;
		HWREGB(AP_LPM_STATE)=LPM_ACTIVE;
		HWREGB(0xA0058000) = 0;
		if(g_nvic_pend_l==0)
			g_nvic_pend_l = HWREG(0xe000e200);
		if(g_nvic_pend_h==0)
			g_nvic_pend_h = HWREG(0xe000e204);
			
		delay=100;
		//XY_DSPCLK_DIV
		HWREGB(0xa0110008)|=0x20;
		HWREGB(0xA0110018) = (HWREGB(0xA0110018)&0x1E)|0x21;

		while(delay--);
		if(g_wakeup_info==0)
			g_wakeup_info = HWREGB(0xA0058001);
		//LOS_IntUnLock();
		HWREGB(0xA0058001) = 0;
		HWREGB(PRCM_BASE + 0x1C)=0;
		HWREG(0xE000E280)=0xFFFFFFFF;
		HWREG(0xE000E284)=0xFFFFFFFF;
		__DSB();
		__ISB();
		delay=100;
		while(delay--);

  }  
  HWREGB(0xA011002C)|=(1<<4); //force pll_lock to digital

}

#endif

void ioldo2_off() {
	//Set bypass mode and clear pu and put it at lp mode
	HWD_REG_WRITE08(0xa0058013,(HWD_REG_READ08(0xa0058013)&0xf)|(0x5<<4));	
}

#if (XY_STANDBY==1)

void at_uart_gpio_recovery()
{
	GPIOConflictStatusClear(g_softap_fac_nv->csp2_rxd_pin);
	HWREG(GPIO_BASE + GPIO_CTRL0)|=(1U<<g_softap_fac_nv->csp2_rxd_pin);
	HWREG(0xa0140040)&=(~(1U<<(g_softap_fac_nv->csp2_rxd_pin)));
	HWREG(0xa0140018)&=(~(1U<<(g_softap_fac_nv->csp2_rxd_pin)));
	HWREG(0xa0140020)|=(1U<<(g_softap_fac_nv->csp2_rxd_pin));
	HWREG(0xa0140028)|=(1U<<(g_softap_fac_nv->csp2_rxd_pin));
	HWREG(0xa0140038)|=(1U<<(g_softap_fac_nv->csp2_rxd_pin));
	GPIOPeripheralPad(GPIO_CSP2_RXD,g_softap_fac_nv->csp2_rxd_pin);
	
	//CSPIntEnable(CSP2_BASE,CSP_INT_RX_DONE);
}

extern QSPI_FLASH_Def xinyi_flash;
volatile unsigned char g_stby_gpio_config=0;
volatile unsigned long standby_delay;
//extern volatile unsigned int intsave;
extern volatile unsigned long long g_standby_time;
//extern volatile unsigned char  pin_wakeup_standby;
extern volatile unsigned long g_plllock_forcextal_num;
//extern uint32_t g_csp_uart_work_TskHandle;
#if  XTAL_TEST
// use for get force crystal time
extern volatile uint32_t at_utc_alarm;
extern volatile uint32_t at_utc_time_xtal;
extern volatile uint32_t at_utc_cnt_xtal;
extern volatile uint32_t at_utc_time_pll;
extern volatile uint32_t at_utc_cnt_pll;
extern volatile uint8_t  at_utc_wakeup_flag;
#endif



void GPIO_lpmode_before_standby()
{
	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_SWD)) != 0)
	{
		if(g_softap_fac_nv->swd_swclk_pin != 0xff)
		{//default GPIO pin:2
			GPIOConflictStatusClear(g_softap_fac_nv->swd_swclk_pin);
			GPIOPadModeSet(g_softap_fac_nv->swd_swclk_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->swd_swclk_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->swd_swclk_pin);
		}
		if(g_softap_fac_nv->swd_swdio_pin != 0xff)
		{//default GPIO pin:3
			GPIOConflictStatusClear(g_softap_fac_nv->swd_swdio_pin);
			GPIOPadModeSet(g_softap_fac_nv->swd_swdio_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->swd_swdio_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->swd_swdio_pin);
		}
	}

	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_JTAG)) != 0)
	{
		
		if(g_softap_fac_nv->jtag_jtck_pin != 0xff)
		{//default GPIO pin:21
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtck_pin);
			GPIOPadModeSet(g_softap_fac_nv->jtag_jtck_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->jtag_jtck_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->jtag_jtck_pin);
		}
		if(g_softap_fac_nv->jtag_jtdi_pin != 0xff)
		{//default GPIO pin:18
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtdi_pin);
			GPIOPadModeSet(g_softap_fac_nv->jtag_jtdi_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->jtag_jtdi_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->jtag_jtdi_pin);
		}
		if(g_softap_fac_nv->jtag_jtms_pin != 0xff)
		{//default GPIO pin:19
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtms_pin);
			GPIOPadModeSet(g_softap_fac_nv->jtag_jtms_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->jtag_jtms_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->jtag_jtms_pin);
		}
		if(g_softap_fac_nv->jtag_jtdo_pin != 0xff)
		{//default GPIO pin:20
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtdo_pin);
			GPIOPadModeSet(g_softap_fac_nv->jtag_jtdo_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->jtag_jtdo_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->jtag_jtdo_pin);
		}
	}

	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_CSP2)) != 0)
	{
		if(g_softap_fac_nv->csp2_txd_pin != 0xff)
		{//default GPIO pin:15
			GPIOConflictStatusClear(g_softap_fac_nv->csp2_txd_pin);
			GPIOPadModeSet(g_softap_fac_nv->csp2_txd_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->csp2_txd_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->csp2_txd_pin);
		}
	}

	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_CSP3)) != 0)
	{
		if(g_softap_fac_nv->csp3_txd_pin != 0xff)
		{//default GPIO pin:14
			GPIOConflictStatusClear(g_softap_fac_nv->csp3_txd_pin);
			GPIOPadModeSet(g_softap_fac_nv->csp3_txd_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->csp3_txd_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->csp3_txd_pin);
		}
		/*
		if(g_softap_fac_nv->csp3_rxd_pin != 0xff)
		{//default GPIO pin:17
			GPIOConflictStatusClear(g_softap_fac_nv->csp3_rxd_pin);
			GPIOPadModeSet(g_softap_fac_nv->csp3_rxd_pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
			GPIODirectionSet(g_softap_fac_nv->csp3_rxd_pin, GPIO_DIR_MODE_IN);
			GPIOPullupEn(g_softap_fac_nv->csp3_rxd_pin);
		}*/
	}

}
void GPIO_lpmode_after_standby()
{
	

	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_SWD)) != 0)
	{
		if(g_softap_fac_nv->swd_swclk_pin != 0xff)
		{//default GPIO pin:2
			GPIOConflictStatusClear(g_softap_fac_nv->swd_swclk_pin);
			GPIOPeripheralPad(GPIO_AP_SWCLKTCK,g_softap_fac_nv->swd_swclk_pin);
			GPIODirModeSet(g_softap_fac_nv->swd_swclk_pin, GPIO_DIR_MODE_HW);
		}
		if(g_softap_fac_nv->swd_swdio_pin != 0xff)
		{//default GPIO pin:3
			GPIOConflictStatusClear(g_softap_fac_nv->swd_swdio_pin);
			GPIOPeripheralPad(GPIO_AP_SWDIOTMS,g_softap_fac_nv->swd_swdio_pin);
			GPIODirModeSet(g_softap_fac_nv->swd_swdio_pin, GPIO_DIR_MODE_HW);
		}
	}

	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_JTAG)) != 0)
	{
		if(g_softap_fac_nv->jtag_jtck_pin != 0xff)
		{//default GPIO pin:21
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtck_pin);
			GPIOPeripheralPad(GPIO_DSP_JTCK,g_softap_fac_nv->jtag_jtck_pin);
			GPIODirModeSet(g_softap_fac_nv->jtag_jtck_pin, GPIO_DIR_MODE_HW);
		}
		if(g_softap_fac_nv->jtag_jtdi_pin != 0xff)
		{//default GPIO pin:18
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtdi_pin);
			GPIOPeripheralPad(GPIO_DSP_JTDI,g_softap_fac_nv->jtag_jtdi_pin);
			GPIODirModeSet(g_softap_fac_nv->jtag_jtdi_pin, GPIO_DIR_MODE_HW);
		}
		if(g_softap_fac_nv->jtag_jtms_pin != 0xff)
		{//default GPIO pin:19
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtms_pin);
			GPIOPeripheralPad(GPIO_DSP_JTMS,g_softap_fac_nv->jtag_jtms_pin);
			GPIODirModeSet(g_softap_fac_nv->jtag_jtms_pin, GPIO_DIR_MODE_HW);
		}
		if(g_softap_fac_nv->jtag_jtdo_pin != 0xff)
		{//default GPIO pin:20
			GPIOConflictStatusClear(g_softap_fac_nv->jtag_jtdo_pin);
			GPIOPeripheralPad(GPIO_DSP_JTDO,g_softap_fac_nv->jtag_jtdo_pin);
			GPIODirModeSet(g_softap_fac_nv->jtag_jtdo_pin, GPIO_DIR_MODE_HW);
		}
	}

	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_CSP2)) != 0)
	{
		if(g_softap_fac_nv->csp2_txd_pin != 0xff)
		{//default GPIO pin:15
			GPIOConflictStatusClear(g_softap_fac_nv->csp2_txd_pin);
			GPIOPeripheralPad(GPIO_CSP2_TXD,g_softap_fac_nv->csp2_txd_pin);
			GPIODirModeSet(g_softap_fac_nv->csp2_txd_pin, GPIO_DIR_MODE_HW);
		}
	}

	if((g_softap_fac_nv->peri_remap_enable & (1<<REMAP_CSP3)) != 0)
	{
		if(g_softap_fac_nv->csp3_txd_pin != 0xff)
		{//default GPIO pin:14
			GPIOConflictStatusClear(g_softap_fac_nv->csp3_txd_pin);
			GPIOPeripheralPad(GPIO_CSP3_TXD,g_softap_fac_nv->csp3_txd_pin);
			GPIODirModeSet(g_softap_fac_nv->csp3_txd_pin, GPIO_DIR_MODE_HW);
		}
/*		
		if(g_softap_fac_nv->csp3_rxd_pin != 0xff)
		{//default GPIO pin:17
			GPIOConflictStatusClear(g_softap_fac_nv->csp3_rxd_pin);
			HWREG(GPIO_BASE + GPIO_CTRL0)|=(1U<<(g_softap_fac_nv->csp3_rxd_pin));
			HWREG(0xa0140040)&=(~(1U<<(g_softap_fac_nv->csp3_rxd_pin)));
			HWREG(0xa0140018)&=(~(1U<<(g_softap_fac_nv->csp3_rxd_pin)));
			HWREG(0xa0140020)|=(1U<<(g_softap_fac_nv->csp3_rxd_pin));
			HWREG(0xa0140028)|=(1U<<(g_softap_fac_nv->csp3_rxd_pin));
			HWREG(0xa0140038)|=(1U<<(g_softap_fac_nv->csp3_rxd_pin));
			GPIOPeripheralPad(GPIO_CSP3_RXD,g_softap_fac_nv->csp3_rxd_pin);
		}
*/
	}
}


extern uint32_t soft_sample_get_special_id(void);
extern void soft_sample_from_standby(uint32_t type, uint32_t wakeup_status);
//yus
void LPM_ARM_Entry_Standby()
{
	volatile unsigned char reg_lpldo = HWREGB(0xa005801c);
	volatile unsigned char reg_xtalcur = HWREGB(0xA011003E);
#if 0
	volatile unsigned char reg_coreldo = HWREGB(0xa0058009);	
	volatile unsigned char reg_utc = HWREGB(0xa005801f);
	volatile unsigned char reg_STBLDO = HWREGB(0xa005802c);
	volatile unsigned char reg_STBLDO1 = HWREGB(0xa005802d);
#endif
	volatile unsigned long gpio_mode_ctl=HWD_REG_READ32(0xa0140010);
	volatile unsigned long gpio_pullup_nx=HWD_REG_READ32(0xa0140040);
	volatile unsigned long gpio_pullup_n0=HWD_REG_READ32(0xa0140018);
	volatile unsigned long gpio_pulldown_n0=HWD_REG_READ32(0xa0140020);
	volatile unsigned long gpio_oe_n0=HWD_REG_READ32(0xa0140038);
	volatile unsigned long gpio_output=HWD_REG_READ32(0xa0140008);

	volatile unsigned long at_uart_csp_timeout=HWD_REG_READ32(0xa0100024);

	//volatile unsigned long gpio_AT_LOG_pin_mask = 0;
	//volatile unsigned long gpio_AT_LOG_rxd_mask = 0;
	volatile uint8_t reg_sta;

	volatile unsigned char xipflag = 0;

	(void) reg_xtalcur;
	(void) gpio_mode_ctl;
	(void) gpio_pullup_nx;
	(void) gpio_pullup_n0;
	(void) gpio_pulldown_n0;
	(void) gpio_oe_n0;
	(void) gpio_output;

	HWREGB(AP_LPM_STATE)=LPM_STANDBY;

	//进standby前将nv用到的引脚配置成输入上拉
	GPIO_lpmode_before_standby();

	HWREG(0xa0140008) &= ~((1U<<g_softap_fac_nv->rf_switch_v0_pin)|(1U<<g_softap_fac_nv->rf_switch_v1_pin)|(1U<<g_softap_fac_nv->rf_switch_v2_pin));//gpio 0 1 4 output low
	//LED output 0 to reduce leakage,gpio5 default
	if(g_softap_fac_nv->peri_remap_enable & (1<<REMAP_LED))
	{
		HWREG(0xa0140008) &= ~(1U<<g_softap_fac_nv->led_pin);
	}

	//关AT口时不走此流程，此流程用2bit的timeout检测AT口是否还有接收数据，有则跳转至standby_exit，否则直到timeout到来，继续下面流程
	if(g_softap_fac_nv->close_at_uart == 0)
	{
		/* 8003相关*/
		if(soft_sample_get_special_id() != 0)
		{
			if(g_softap_fac_nv->off_debug == 0)
				xy_assert(0);
		}

		// CSP_AYSNC_PARAM_REG： 此参数指定接收操作的超时位数
		// 此处指定2bit的超时位：当接收到最后一个数据后，2bit时间内没有接收到更多数据，则触发一次超时中断
		HWREG(AT_UART_CSP + CSP_AYSNC_PARAM_REG) = ((HWREG(AT_UART_CSP + CSP_AYSNC_PARAM_REG)&0xffff0000)|0x02);
		HWREG(AT_UART_CSP + CSP_INT_STATUS) = CSP_INT_CSP_RX_TIMEOUT;
		while(1)
		{
			//（1）CSP_INT_RX_DONE： RXFIFO中断中接收到了有效数据。
			//（2）或者csp2_rxd_pin引脚上读到了数据
			//	即AT口接收到了数据，则退出睡眠流程。
			if((HWREG(AT_UART_CSP + CSP_INT_STATUS)&CSP_INT_RX_DONE)||GPIOPinRead(g_softap_fac_nv->csp2_rxd_pin) == 0)	
				goto standby_exit;
			if(HWREG(AT_UART_CSP + CSP_INT_STATUS)&CSP_INT_CSP_RX_TIMEOUT)
			{
				HWREG(AT_UART_CSP + CSP_INT_STATUS)=CSP_INT_CSP_RX_TIMEOUT;
				break;
			}
		}
	}

	// 判断DSP状态： 当DSP已经进入standby状态，或者DSP未启动时才继续M3侧睡眠流程，否则M3退出睡眠流程
	if( DSP_IS_STANDBY()  || DSP_IS_NOT_LOADED() )
	{
		if(g_softap_fac_nv->close_at_uart == 0)
		{
			//此时接收到数据则跳转至standby_exit，否则映射AT口接收引脚为gpi0
			if(GPIOPinRead(g_softap_fac_nv->csp2_rxd_pin) == 0)
				goto standby_exit;

			// 关闭wakeup中断，防止以下操作中意外唤醒DSP核。
			HWREGB(0xA0110002) = 0;

			//CSPIntDisable(CSP2_BASE,CSP_INT_ALL);
			//CSPRxDisable(CSP2_BASE);

			// 关闭AT口
			CSPDisable(AT_UART_CSP);

			// 关闭AT口FIFO，并重置RX FIFO的状态（包含中断状态）
			HWREG(AT_UART_CSP + 0x130) = 0x1;
			// Start RX FIFO
			HWREG(AT_UART_CSP + 0x130) = 0x2;

			// 以下为配置RXD为gpi外设功能（低电平触发唤醒中断）
			GPIOConflictStatusClear(g_softap_fac_nv->csp2_rxd_pin);
			HWREG(GPIO_BASE + GPIO_CTRL0)|=(1U<<g_softap_fac_nv->csp2_rxd_pin);
			HWREG(0xa0140040)&=(~(1U<<(g_softap_fac_nv->csp2_rxd_pin)));
			HWREG(0xa0140018)&=(~(1U<<(g_softap_fac_nv->csp2_rxd_pin)));
			HWREG(0xa0140020)|=(1U<<(g_softap_fac_nv->csp2_rxd_pin));
			HWREG(0xa0140028)|=(1U<<(g_softap_fac_nv->csp2_rxd_pin));
			HWREG(0xa0140038)|=(1U<<(g_softap_fac_nv->csp2_rxd_pin));
			//HWREG(GPIO_BASE + GPIO_CTRL0)|=(1U<<g_softap_fac_nv->csp2_rxd_pin);
			//HWREG(0xa0140040)|=((1U<<(g_softap_fac_nv->csp2_rxd_pin)));
			GPIOPeripheralPad(GPIO_GPI0,g_softap_fac_nv->csp2_rxd_pin);

			// 硬件BUG：gpi配置后会产生中断，此处为软件规避。
			/*配置完gpi后会立刻来一个gpi的中断并且将该位置1，需要100us检测到该位为1 */
			while(!(HWREGB(0xA0058001)&0x04))
			{
				/*每20us检测AT的接收引脚，如果有数据则恢复AT口的引脚配置并跳转至uart_sample_pll*/
				// 等待内部gpi中断时，可以被外部pin唤醒打断。
				if(GPIOPinRead(g_softap_fac_nv->csp2_rxd_pin) == 0)
				{	
					at_uart_gpio_recovery();
					reg_sta = 0x80;
					goto uart_sample_pll;
				}

				for(standby_delay =0;standby_delay <40;standby_delay ++);
			}

			//检测到gpi0位为1后，清除该位，清除需要60us
			HWREGB(0xA0058001) = 0;
			/*等待gpi0位被清除为0，大约需要60us*/
			while(HWREGB(0xA0058001)&0x04)
			{
				/*检测AT的接收引脚，如果有数据则恢复AT口的引脚配置并跳转至uart_sample_pll*/
				if(GPIOPinRead(g_softap_fac_nv->csp2_rxd_pin) == 0)
				{	
					at_uart_gpio_recovery();
					reg_sta = 0x80;
					goto uart_sample_pll;
				}
				for(standby_delay =0;standby_delay <40;standby_delay ++);
			} 
		}
		
		//HWREGB(0xA011001c) = 0;

		// 重新使能唤醒中断，此后允许唤醒中断产生，DSP/M3可被唤醒。
		if(DSP_IS_NOT_LOADED())
			HWREGB(0xA0110002) = 0x2;
		else
			HWREGB(0xA0110002) = 0x3;
		//HWREGB(0xA011003E) = 0x7e;

		if( DSP_IS_STANDBY() || DSP_IS_NOT_LOADED() )
		{
			// ARM_STANDBY_ENTER_XIP：M3与DSP同步，此后M3将退出XIP模式以节省功耗。
			HWREG(ARM_STANDBY_ENTER_XIP) = 0xA5;
			FLASH_ExitXIPMode(&xinyi_flash);
			FLASH_WaitIdle(&xinyi_flash);
			xipflag = 1;

			if(g_softap_fac_nv->close_at_uart == 0)
			{
				/*检测AT的接收引脚，如果有数据则清除gpi中断状态标志并跳转至standby_Cal_On*/
				if(GPIOPinRead(g_softap_fac_nv->csp2_rxd_pin) == 0)
				{
					HWREGB(0xA0058001) = 0;
					goto standby_Cal_On;
				}
			}

			// bug4505
			if((HWREGB(0xA0110030) & 0x02) && (HWREGB(0xA011003B) & 0x2) && (g_softap_fac_nv->pmu_ioldo_sel == 0))
			{
				// detect low voltage, enable bypass ioldo
				HWREGB(0xA011003B) |= 0x04;
			}
			else
			{
				// detect normal voltage, disable bypass ioldo
				HWREGB(0xA011003B) &= 0xFB;
			}

//			HWREGB(0xA011003A) = (HWREGB(0xA011003A) & 0x1F) | (0x60); //WideBat reduce LPBG setting to 0x5
//			HWREGB(0xA0110070) = (HWREGB(0xA0110070) & 0x87) | (0x58); //WideBat reduce LPBG setting to 0x5
//			HWREGB(0xA0110031) = (HWREGB(0xA0110031) & 0xF8) | (0x7);	   //WideBat reduce LPBG setting to 0x5

			HWREGB(0xA011003A) = (HWREGB(0xA011003A) & 0x1F) | (0x0);  //A011003A [7:5] 改为 111            (调整IOLDO1)
			HWREGB(0xA0110070) = (HWREGB(0xA0110070) & 0x87) | (0x40);  //A0110070 [6:3] 改为 1111          (调整IOLDO2)
			HWREGB(0xA0110031) = (HWREGB(0xA0110031) & 0xF8) | (0x04);	//A0110031 [2:0] 改为 111            (调整LDO2P5)

			//PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_UTC_EN);
			// 配置进入standby睡眠模式
			HWREGB(0xA0058000) = 0x33;
			HWREGB(0xE000ED10) |= 0x4;
			g_plllock_check_enable = 0;

			/*bug 3611*/
			//if(g_softap_fac_nv->xtal_type)
				//HWREGB(0xA011003E) = (reg_xtalcur&0xcf)|0x10;

			// bug3857
			if(g_softap_fac_nv->xtal_type)	// XO
			{
				HWREGB(0xA011003E) = 0x4E;
			}

			if(!(g_softap_fac_nv->work_mode == 2 && DSP_IS_NOT_LOADED()))
			{
				PRCM_Clock_Mode_Force_XTAL();
				for(standby_delay =0;standby_delay <2;standby_delay ++);
				RF_BBPLL_Cal_Off();
				//for(standby_delay =0;standby_delay <2;standby_delay ++);
			}
			/*bug 3794*/
			//PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_UTC_EN);
			//for(standby_delay =0;standby_delay <400;standby_delay ++);// delay 130 us
			if(g_softap_fac_nv->close_at_uart == 0)
			{
				/*检测AT的接收引脚，如果有数据则清除gpi的状态标志并跳转至standby_wakeup*/
				if(GPIOPinRead(g_softap_fac_nv->csp2_rxd_pin) == 0)
				{
					// 清除gpi唤醒中断标志位
					HWREGB(0xA0058001) = 0;
					goto standby_wakeup;
				}
			}

			//内核进入WFI状态
			__asm ("WFI") ;
			//xtal 700 - 1700 us
standby_wakeup:
			/*bug 3794*/
			//for(standby_delay =0;standby_delay <2;standby_delay ++);
			//PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_UTC_EN);
			//for(standby_delay =0;standby_delay <2;standby_delay ++);
			/*after this 3-4 utc clock delay is necessary before utc register opetation*/
			if(!(g_softap_fac_nv->work_mode == 2 && DSP_IS_NOT_LOADED()))
				RF_BBPLL_Cal_On();

			//if(g_softap_fac_nv->xtal_type)
				//HWREGB(0xA011003E) = (reg_xtalcur&0xcf);

			//if(!RF_BBPLL_Lock_Status_Get())
			{
				g_plllock_forcextal_num++;
			}
		}

standby_Cal_On:
		//读取gpi各位状态，以供软采样判断使用
		reg_sta = HWREGB(0xA0058001);

		//读取gpi各位状态，以供gpi的demo使用
		g_gpi_wakeupsrc = HWREGB(0xA0058001);

		// 退出睡眠状态
		HWREGB(0xA0058000) = 0;

		//HWREGB(0xA0058001) = 0;
		if(g_softap_fac_nv->close_at_uart == 0)
		{
			//恢复AT口的配置
			at_uart_gpio_recovery();
			// use for dirty data processing, after force crystal
			soft_sample_from_standby(0, reg_sta);
		}
		else
		{
			// force pll
			if(!(g_softap_fac_nv->work_mode == 2 && DSP_IS_NOT_LOADED()))
			{
				PRCM_Clock_Mode_Auto();
				while((!(HWREGB(0xa011000D)&0x04))||(!(HWREGB(0xa011000D)&0x02)));
			}
		}

		//双核模式下清除gpi状态位； 共核模式下，会在dsp的wakeup中断中清除该状态
		HWREGB(0xA0058001) = 0;

		#if (STANDBY_LOG == 1)
		if(reg_sta & 0x01)
		{
			StandbyWkupGetWkupSource(0);//utc
		}
		else if(reg_sta & 0x02)
		{
			StandbyWkupGetWkupSource(2);//extpin
		}
		else
		{
			StandbyWkupGetWkupSource(1);//gpi
		}
		#endif
		//HWREGB(0xA0058001) = 0;
		// 等待PLL时钟稳定。
		// work_mode == 2 代表外部晶振驱动的单核模式。此模式下不需要PLL时钟。但即便其中模式下，若需要启动DSP，仍需要PLL时钟。
		if(!(g_softap_fac_nv->work_mode == 2 && DSP_IS_NOT_LOADED()))
			while((!(HWREGB(0xa011000D)&0x04))||(!(HWREGB(0xa011000D)&0x02)));

		// 恢复XIP模式，并置共享xip全局
		if(xipflag)
			FLASH_EnterXIPMode(&xinyi_flash,0xa0);

		HWREG(ARM_STANDBY_ENTER_XIP) = 0;

uart_sample_pll:
		if(g_softap_fac_nv->close_at_uart == 0)
		{
			// use for dirty data processing, after force pll_lock to digital
			soft_sample_from_standby(1, reg_sta);
		}
	}

standby_exit:
	if(DSP_IS_NOT_LOADED())
		HWREGB(0xA0110002) = 0x2;
	else
		HWREGB(0xA0110002) = 0x3;

	HWREGB(AP_LPM_STATE)=LPM_ACTIVE;

	HWD_REG_WRITE08(0xa005801c,reg_lpldo);
	HWREGB(0xA011003A) = (HWREGB(0xA011003A) & 0x1F) | (0x0); //WideBat reduce LPBG setting to 0x5(IOLDO1)
	HWREGB(0xA0110070) = (HWREGB(0xA0110070) & 0x87) | (0x40); //WideBat reduce LPBG setting to 0x5(IOLDO2)
	HWREGB(0xA0110031) = (HWREGB(0xA0110031) & 0xF8) | (0x4);	   //WideBat reduce LPBG setting to 0x5(LDO2P5)
	if(g_softap_fac_nv->close_at_uart == 0)
	{
		//恢复原AT口接收timeout的配置，因为在此之前有改动过此值，所以需要恢复
		HWREG(0xA0100024) = at_uart_csp_timeout;
	}

	// bug3857
	if(g_softap_fac_nv->xtal_type)	// XO
	{
		HWREGB(0xA011003E) = 0x8A;
	}

	//退standby前恢复原nv中使用的引脚，将其恢复成对应的外设功能
	GPIO_lpmode_after_standby();
}

#endif

void psm_standby_cb(void *para)
{
	(void) para;

	HWREGB(PRCM_BASE + 0x1c) |= 0x02;
}

void Switch_32k_Xtal()
{
	static unsigned char switch_32k_xtal = 0;
	if (g_softap_fac_nv->xtal_32k == 0)
	{
		if(switch_32k_xtal == 0)
		{
			Switch_to_Outer_UTC_XTAL();
			switch_32k_xtal++;
		}
	}
}
void Check_ioldo_Bypass()
{
	// bug4505
	if(!(HWREGB(0xA0110030) & 0x02))
	{
		// detect normal voltage and bypass ioldo enabled, should disable bypass ioldo
		if(HWREGB(0xA011003B) & 0x04)
		{
			HWREGB(0xA011003B) &= 0xFB;
		}
	}
}

//used for AT+UARTSET/AT+STANDBY
volatile unsigned char g_standby_lock=0;
void xy_standby_close()
{
	g_standby_lock=1;
}

void xy_standby_open()
{
	g_standby_lock=0;
}
/*
unsigned long long xy_StandbyUtcTicksGet(unsigned long long ulldsputcticks)
{
 
    unsigned long ulSwtmrSortLinkTicks = 0;
    unsigned long long ullfinalutcTicks,ullcurutcTicks;
	struct rtc_time rtctime_cur;
	unsigned int intsave;
	
	intsave = LOS_IntLock();
	ulSwtmrSortLinkTicks = osSwTmrGetNextTimeout();
	rtc_timer_read(&rtctime_cur);
	ullcurutcTicks = xy_mktime(&rtctime_cur);	

	if(ulldsputcticks<(ullcurutcTicks+ulSwtmrSortLinkTicks+50))
	{
		ullfinalutcTicks = ulldsputcticks;		
	}
	else
	{		
		ullfinalutcTicks = (ullcurutcTicks+ulSwtmrSortLinkTicks);
	}

	LOS_IntRestore(intsave);
	return ullfinalutcTicks;
	
}
*/
int xy_StandbyCheckAtUart(void)
{
	if(g_softap_fac_nv->close_at_uart == 1)
		return 0;
	if((HWREG(AT_UART_CSP + CSP_INT_ENABLE) & CSP_INT_CSP_RX_TIMEOUT))
		return 1;
	else
		return 0;
}

extern unsigned char g_autobaud_stop_standby;
int xy_StandbyStateCheck()
{
	if(g_standby_lock || g_at_off_standby || g_user_off_standby || g_softap_fac_nv->lpm_standby_enable == 0 || g_autobaud_stop_standby )
		return 0;
	else
		return 1;
}

unsigned int LPM_Deepsleep_Allow()
{
	if(g_softap_fac_nv->deepsleep_enable == 1 && HAVE_FREE_LOCK == 1 &&  g_softap_fac_nv->mgsleep_enable == 0 && g_softap_var_nv->sleep_mode != 0 && (DSP_IS_NOT_LOADED() || g_deepsleep_dsp_ready) && user_allow_deepsleep_hook()==XY_OK)
	{
		return 1;
	}	
	else
	{
		return 0;
	}
		
}

unsigned int LPM_Deepsleep_Allow_Critical()
{
	if( !(g_softap_fac_nv->deepsleep_enable == 1 && HAVE_FREE_LOCK == 1 && g_softap_fac_nv->mgsleep_enable == 0 && g_softap_var_nv->sleep_mode != 0 && (DSP_IS_NOT_LOADED() || g_deepsleep_dsp_ready) && at_deepsleep_Check() && user_allow_deepsleep_hook()==XY_OK) \
		|| icm_buf_check())
	{
		return 0;
	}	
	else
	{
		return 1;
	}
		
}

unsigned int LPM_Standby_Allow()
{
	if(g_standby_dsp_ready &&  xy_StandbyStateCheck() && user_allow_standby_hook()==XY_OK)
	{
		return 1;
	}	
	else
	{
		return 0;
	}
		
}

unsigned int LPM_Standby_Allow_Critical()
{
	if( icm_buf_check() || g_standby_dsp_ready==0 || xy_StandbyStateCheck()==0 || xy_StandbyCheckAtUart() || g_fast_off==1 || csp_write_allout_state() == 0 || user_allow_standby_hook()!=XY_OK)
	{
		return 0;
	}	
	else
	{
		tcpip_standby_wakeup_config();
		return 1;
	}
		
}

unsigned int LPM_Standby_Allow_SingleCore()
{
	if(DSP_IS_NOT_LOADED() &&  xy_StandbyStateCheck() && user_allow_standby_hook()==XY_OK)
	{
		return 1;
	}	
	else
	{
		return 0;
	}
		
}

unsigned int LPM_Standby_Allow_Critical_SingleCore()
{
	if( !DSP_IS_NOT_LOADED() || xy_StandbyStateCheck()==0 || xy_StandbyCheckAtUart()  || csp_write_allout_state() == 0 || user_allow_standby_hook()!=XY_OK)
	{
		return 0;
	}	
	else
	{
		return 1;
	}
		
}

unsigned int LPM_Wfi_Allow()
{
	if(g_softap_fac_nv->wfi_enable==1)
	{
		return 1;
	}	
	else
	{
		return 0;
	}
		
}

unsigned int LPM_Wfi_Allow_Critical()
{
	if( g_softap_fac_nv->wfi_enable==0 || icm_buf_check() )
	{
		return 0;
	}	
	else
	{
		return 1;
	}
		
}


//extern volatile int bWfiNeedSchedule;

volatile  long g_standby_wkup_taskid;
volatile  long g_standby_wkup_tmrid;
volatile unsigned long long g_m3_standby_time = 0;  //M3 standby sleep time

int LPM_Fastoff_Powerdown_Check()
{
	return (g_fast_off == 1);
}

void LPM_Fastoff_Powerdown_Process()
{
	lpm_EnterCritical();
	WatchdogDisable(WDT_BASE);
	nv_write_to_flash();
	HWREGB(0xA0110018)=0;
	if(g_softap_fac_nv->g_NPSMR_enable == 1)
		lpm_string_output("\r\n+NPSMR:1\r\n",strlen("\r\n+NPSMR:1\r\n"));
	else
		lpm_string_output("\r\n+POWERDOWN:0,-1\r\n",strlen("\r\n+POWERDOWN:0,-1\r\n"));
	while(1);
}

int save_nv_in_deepsleep(void)
{
	lpm_EnterCritical();

	if( DSP_IS_DEEPSLEEP() || DSP_IS_NOT_LOADED())
	{
		HWREGB(0xA0110002) &= 0x02;//not wakeup dsp
		if(DSP_IS_DEEPSLEEP() || DSP_IS_NOT_LOADED())
		{
			g_have_suspend_dsp = 1;
			user_deepsleep_before_hook();
			nv_write_to_flash();
			while(HWREGB(DMAC_CH2_TC)!=0);
			
			if(DSP_IS_DEEPSLEEP() || DSP_IS_NOT_LOADED())
				HWREGB(0xA0110002) = 0x03;
			else
				xy_assert(0);

			g_have_suspend_dsp = 0;
			lpm_ExitCritical();
			return 1;
		}
		//HWREGB(0xA0110002) = 0x03;
	}
	lpm_ExitCritical();
	return 0;	
}

void LPM_Deepsleep_Process()
{
	char  *powerdown_at_str=xy_zalloc(40);
	char  *deepsleep_debug_info_str = (char *)xy_malloc(250); 
	char ps_deepsleep_time_enough_flg = 0;
	volatile unsigned long long utc_cnt_before;

	//sprintf may alloc block,so must call before LOS_IntLock
	get_powerdown_urc(powerdown_at_str);

	lpm_EnterCritical();

	if (!LPM_Deepsleep_Allow_Critical())
	{
		lpm_ExitCritical();
		xy_free(deepsleep_debug_info_str);
		xy_free(powerdown_at_str);
		return;
	}

	if(DSP_IS_DEEPSLEEP())
	{
		utc_cnt_before = utc_cnt_get();
		if((utc_cnt_before/32+AP_DEEPSLEEP_MIN)<g_deepsleep_time)
		{
			ps_deepsleep_time_enough_flg = 1;
		}
	}

	if( DSP_IS_NOT_LOADED() || (ps_deepsleep_time_enough_flg && DSP_IS_DEEPSLEEP()))
	{
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		//NVIC_ClearPendingIRQ(SysTick_IRQn);
		HWREG(0xE000ED04) =((1<<27)|(1<<25));
		//NVIC_CLREN_CLRPEN_ALL();

		if(!save_nv_in_deepsleep() || (HWREG(0xe000e200)&(1<<UTC_IRQn)))	
		{
			SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
			lpm_ExitCritical();
			xy_free(powerdown_at_str);
			xy_free(deepsleep_debug_info_str);
			return;
		}
		//used  for AT+FASTOFF,dsp force to OFF,so DMA maybe conflict

		lpm_string_output(powerdown_at_str,strlen(powerdown_at_str));
		while(csp_write_allout_state() == 0);

		//断电补偿模式，死循环解决大电容放电问题
		if(g_softap_fac_nv->offtime == 1)
			while(1);

		LPM_ARM_Entry_Deep_Sleep();
#if 0	// not use
		sprintf(deepsleep_debug_info_str, "g_nvic_pend_l:%lx\r\n",g_nvic_pend_l );
		sprintf(deepsleep_debug_info_str + strlen(deepsleep_debug_info_str), "g_nvic_pend_h:%lx\r\n",g_nvic_pend_h );
		sprintf(deepsleep_debug_info_str + strlen(deepsleep_debug_info_str), "g_nvic_enable_l:%lx\r\n",g_nvic_enable_l );
		sprintf(deepsleep_debug_info_str + strlen(deepsleep_debug_info_str), "g_nvic_enable_h:%lx\r\n",g_nvic_enable_h );
		sprintf(deepsleep_debug_info_str + strlen(deepsleep_debug_info_str), "g_wakeupint_source:%lx\r\n",g_wakeupint_source );
		lpm_string_output(deepsleep_debug_info_str,strlen(deepsleep_debug_info_str));			
#endif		
		while(csp_write_allout_state() == 0);
		//while(while1);
	}
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	//wakeup dsp core
	if(!DSP_IS_NOT_LOADED())
	{
		HWREG(PRCM_BASE + 0x1c)|= 0x02;
		g_deepsleep_dsp_ready = 0;
	}

	lpm_ExitCritical();
	xy_free(powerdown_at_str);
	xy_free(deepsleep_debug_info_str);
}



//extern void soft_sample_print_wakeup_reason(void);

void LPM_Standby_Process()
{
	volatile unsigned long long utc_cnt_before,utc_cnt_after;
	//volatile unsigned long long ticks_standby,ticks_wfi;	
	unsigned long /*uwSwtmrSortLinkTicks,uwTskSortLinkTicks,*/uwSleepTicks;
	//unsigned int intsave=0;

	
	lpm_EnterCritical();

	if(!LPM_Standby_Allow_Critical() || LPM_Deepsleep_Allow())
	{
		lpm_ExitCritical();
		return;
	}						
	
	utc_cnt_before = utc_cnt_get();
	//utc_cnt_before_dbg = utc_cnt_before;
	
	#if (STANDBY_LOG == 1)
		StandbyWkupInfoInit();
	#endif
	
	if(((utc_cnt_before/32+AP_STANDBY_MIN)<g_standby_time) && DSP_IS_STANDBY())
	{			
		
		
		//if(g_standby_time-utc_cnt_before/32>100)
		{

			uwSleepTicks = lpm_GetExpectedIdleTick(LPM_STANDBY);

			if(uwSleepTicks<AP_STANDBY_MIN)
			{
				lpm_ExitCritical();
				return;
			}
			g_m3_standby_time = utc_cnt_before/32+uwSleepTicks;
						
			//lpm_rtc_event_add_by_global_critical(RTC_TIMER_LPM,MIN(g_m3_standby_time,g_standby_time),psm_standby_cb,NULL);
			lpm_rtc_event_add_by_global_critical(RTC_TIMER_LPM,MIN(g_m3_standby_time,g_standby_time),NULL,NULL);

			
			#if (STANDBY_LOG == 1)
			if(g_m3_standby_time < g_standby_time)
				StandbyWkupGetWkupCore(0);
			else
				StandbyWkupGetWkupCore(1);
			#endif

		}
		//else
		//{
		//	rtc_event_add_by_global(RTC_TIMER_PS_STANDBY,g_standby_time,psm_standby_cb,NULL);
		//}
		
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

		s_systick_pending_in_lpmode = 0;
		if(HWREG(0xE000ED04) & (1<<26))
		{
			HWREG(0xE000ED04) = (1<<25);
			s_systick_pending_in_lpmode = 1;
		}
		//lpm_string_output("+DBGINFO:into standby\r\n",strlen("+DBGINFO:into standby\r\n"));

		user_standby_before_hook();
		LPM_ARM_Entry_Standby();
		user_standby_after_hook();

		
		lpm_rtc_event_delete_critical(RTC_TIMER_LPM);
		
		utc_cnt_after = utc_cnt_get();
		//utc_cnt_after_dbg = utc_cnt_after;
		lpm_add_ticks(utc_cnt_after - utc_cnt_before);
		
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
			
	}

	if(!DSP_IS_STANDBY())
		g_standby_dsp_ready = 0;

	#if (STANDBY_LOG == 1)
	StandbyExitInfoProcess();
	#endif
	
	lpm_ExitCritical();
	
	#if (STANDBY_LOG == 1)
	StandbyExitLogPrint();
	#endif
	// soft_sample_print_wakeup_reason();	// 8003 debug use, print current wakeup reason
	
	lpm_ScheduleAfterSleep();
}

void LPM_Standby_Process_SingleCore()
{
	volatile unsigned long long utc_cnt_before,utc_cnt_after;
	//volatile unsigned long long ticks_standby,ticks_wfi;	
	unsigned long uwSleepTicks;


	lpm_EnterCritical();
			
	if(!LPM_Standby_Allow_Critical_SingleCore() || LPM_Deepsleep_Allow())
	{
		lpm_ExitCritical();
		return;
	}	
	
	uwSleepTicks = lpm_GetExpectedIdleTick(LPM_STANDBY);

	if(uwSleepTicks > AP_TICKLESS_MIN)
	{

		utc_cnt_before = utc_cnt_get();
		
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		s_systick_pending_in_lpmode = 0;
		if(HWREG(0xE000ED04) & (1<<26))
		{
			HWREG(0xE000ED04) = (1<<25);
			s_systick_pending_in_lpmode = 1;
		}

		//uwSleepTicks= MIN(30000,uwSleepTicks);// limit max standby time
		
		lpm_rtc_event_add_by_offset_critical(RTC_TIMER_LPM,uwSleepTicks,NULL,NULL);
	
		user_standby_before_hook();
		LPM_ARM_Entry_Standby();
		user_standby_after_hook();

		lpm_rtc_event_delete_critical(RTC_TIMER_LPM);

		utc_cnt_after = utc_cnt_get();
		
		if (utc_cnt_after < utc_cnt_before)
		{
			utc_cnt_after = utc_cnt_before;
		}

		if(g_32k_to_hclk == 0)
			g_32k_to_hclk=XY_HCLK/XY_UTC_CLK;
		
		lpm_add_ticks(utc_cnt_after - utc_cnt_before);
		
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

	}

	lpm_ExitCritical();

	// soft_sample_print_wakeup_reason();	// 8003 debug use, print current wakeup reason

	lpm_ScheduleAfterSleep();
}

void LPM_Wfi_Process()
{
	volatile unsigned long long utc_cnt_before,utc_cnt_after;
	unsigned long uwSleepTicks;


	lpm_EnterCritical();

	if (!LPM_Wfi_Allow_Critical() || LPM_Deepsleep_Allow() || LPM_Standby_Allow() || LPM_Standby_Allow_SingleCore())
	{
		lpm_ExitCritical();
		return;
	}						

	uwSleepTicks = lpm_GetExpectedIdleTick(LPM_WFI);
		
	if(uwSleepTicks>AP_TICKLESS_MIN || (DSP_IS_NOT_LOADED() && (uwSleepTicks>AP_TICKLESS_MIN_SIGNDLECORE)) )
	{
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		s_systick_pending_in_lpmode = 0;
		if(HWREG(0xE000ED04) & (1<<26))
		{
			HWREG(0xE000ED04) = (1<<25);
			s_systick_pending_in_lpmode = 1;
		}

		//LED SetOFF
		/*if(g_softap_fac_nv->peri_remap_enable & (1<<REMAP_LED))
		{
			HWREG(0xa0140008) &= ~(1U<<g_softap_fac_nv->led_pin);
		}*/

		utc_cnt_before = utc_cnt_get();			
		
		uwSleepTicks= MIN(AP_TICKLESS_MAX,uwSleepTicks);
		
		if(DSP_IS_NOT_LOADED())
		{
			lpm_rtc_event_add_by_offset_singlecore_wfi(RTC_TIMER_LPM,uwSleepTicks,NULL,NULL);
			
			FLASH_ExitXIPMode(&xinyi_flash);
			FLASH_WaitIdle(&xinyi_flash);
			
		}
		else
			lpm_rtc_event_add_by_offset_critical(RTC_TIMER_LPM,uwSleepTicks,NULL,NULL);

			
		LPM_ARM_Entry_Wfi();
		
		if(DSP_IS_NOT_LOADED())
		{
			FLASH_EnterXIPMode(&xinyi_flash,0xa0);
			lpm_rtc_event_delete_singlecore_wfi(RTC_TIMER_LPM);
		}
		else
			lpm_rtc_event_delete_critical(RTC_TIMER_LPM);
		
		utc_cnt_after = utc_cnt_get();
		
		if (utc_cnt_after < utc_cnt_before)
		{
			utc_cnt_after = utc_cnt_before;
		}

		if(g_32k_to_hclk == 0)
			g_32k_to_hclk=XY_HCLK/XY_UTC_CLK;
		
		lpm_add_ticks(utc_cnt_after - utc_cnt_before);
		
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

	}
	lpm_ExitCritical();
	lpm_ScheduleAfterSleep();
}
