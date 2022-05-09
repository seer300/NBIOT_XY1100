
/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "xy_utils.h"
#include "xy_system.h"
#include "rtc_adapt.h"
#include "rtc_tmr.h"
#include "oss_nv.h"
#include "softap_nv.h"
#include "inter_core_msg.h"
#include "sema.h"
#include "xy_rtc_api.h"

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
struct rtc_event_info *ap_rtc_event_arry = NULL;  //指向retention内存中的一段
struct rtc_event_info *cp_rtc_event_arry = NULL;  //指向retention内存中的一段
osMutexId_t g_rtc_mutex = NULL;
osSemaphoreId_t g_rtc_sem = NULL;

//0,NULL;1,wakeup by PS;2,wakeup by other UTC
int g_RTC_wakeup_type = 0;

/*******************************************************************************
 *                       function implementations	                           *
 ******************************************************************************/
void Set_AP_ALARM_RAM(uint64_t rtcmsc)
{
	(*((uint64_t *)AP_ALARM_VALUE)) = rtcmsc;
}
uint64_t Get_AP_ALARM_RAM()
{
	//xy_cache_invalid((void *)AP_ALARM_VALUE, sizeof(uint64_t));
	return (*((uint64_t *)AP_ALARM_VALUE));
}

void Set_CP_ALARM_RAM(uint64_t rtcmsc)
{
	(*((uint64_t *)CP_ALARM_VALUE)) = rtcmsc;
}
uint64_t Get_CP_ALARM_RAM()
{
	return (*((uint64_t *)CP_ALARM_VALUE));
}

//该接口目前只用在一些关中断处理rtc事件的代码中，在关中断前调用
void rtc_mutex_acquire(void)
{
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexAcquire(g_rtc_mutex, osWaitForever);
}

void rtc_mutex_release(void)
{
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexRelease(g_rtc_mutex);
}


int is_rtc_event_expired(struct rtc_event_info * event)
{
   struct rtc_time cur_rtctime;
   uint64_t cur_rtc_msec = 0;
   
   rtc_timer_read(&cur_rtctime);
   cur_rtc_msec = xy_mktime(&cur_rtctime);
   //此处+1，保证当前时间后至少10ms以内的事件能到得到处理；否则，该事件设置到alarm可能已经过期值
   if(event->rtc_msec/10 <= (cur_rtc_msec/10+1))
	   return 1;
   else
	   return 0;

}
//get the latest node
struct rtc_event_info* rtc_get_next_event()
{
	int index =0;
	struct rtc_event_info * rtc_info = NULL; 
	
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexAcquire(g_rtc_mutex, osWaitForever);
	
	for(index = RTC_TIMER_AP_BASE; index <= RTC_TIMER_AP_END; index++)
	{
		if(ap_rtc_event_arry[index].rtc_msec==0)
			continue;
		
		if(!rtc_info)
			rtc_info = &ap_rtc_event_arry[index];
		else if(ap_rtc_event_arry[index].rtc_msec < rtc_info->rtc_msec)
			rtc_info = &ap_rtc_event_arry[index];
	}
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexRelease(g_rtc_mutex);
	return rtc_info;
}

struct rtc_event_info* rtc_get_next_event_nomutex()
{
	int index =0;
	struct rtc_event_info * rtc_info = NULL; 
	

	
	for(index = RTC_TIMER_AP_BASE; index <= RTC_TIMER_AP_END; index++)
	{
		if(ap_rtc_event_arry[index].rtc_msec==0)
			continue;
		
		if(!rtc_info)
			rtc_info = &ap_rtc_event_arry[index];
		else if(ap_rtc_event_arry[index].rtc_msec < rtc_info->rtc_msec)
			rtc_info = &ap_rtc_event_arry[index];
	}

	return rtc_info;
}


//cannot call this function when cp core is working
struct rtc_event_info* rtc_get_next_event_CP_nomutex()
{
	int index =0;
	struct rtc_event_info * rtc_info = NULL;	
	
	if(RTC_TIMER_CP_MAX == 0)
		return rtc_info;
	
	for(index = RTC_TIMER_CP_BASE; index <= RTC_TIMER_CP_END; index++)
	{
		if(cp_rtc_event_arry[index].rtc_msec==0)
			continue;
		
		if(!rtc_info)
			rtc_info = &cp_rtc_event_arry[index];
		else if(cp_rtc_event_arry[index].rtc_msec < rtc_info->rtc_msec)
			rtc_info = &cp_rtc_event_arry[index];
	}
	 

	return rtc_info;
}


// call this function when restore volatile nv in initialization
void restore_RTC_list(void)
{
	static uint8_t s_called_once = 0;//只调用一次
	struct rtc_event_info* next_rtcinfo = NULL;
	struct rtc_event_info* ap_rtcinfo = rtc_get_next_event();
	struct rtc_event_info* cp_rtcinfo = rtc_get_next_event_CP_nomutex();

	if(s_called_once)
		return;
	else
		s_called_once++;
	
	if(ap_rtcinfo != NULL)
	{
		Set_AP_ALARM_RAM(ap_rtcinfo->rtc_msec);
	}

	if(cp_rtcinfo != NULL)
	{
		Set_CP_ALARM_RAM(cp_rtcinfo->rtc_msec);
	}

	if(ap_rtcinfo != NULL && cp_rtcinfo != NULL)
	{
		if(ap_rtcinfo->rtc_msec <= cp_rtcinfo->rtc_msec)
			next_rtcinfo = ap_rtcinfo;
		else
			next_rtcinfo = cp_rtcinfo;
	}
	else if(ap_rtcinfo != NULL)
	{
		next_rtcinfo = ap_rtcinfo;
	}
	else if(cp_rtcinfo != NULL)
	{
		next_rtcinfo = cp_rtcinfo;
	}
	
	//若是UTC或者WAKEUP PIN唤醒，UTC寄存器内容未丢失
	
	if(get_sys_up_stat() != UTC_WAKEUP && get_sys_up_stat() != EXTPIN_WAKEUP)
		rtc_event_refresh();

	if(next_rtcinfo == NULL)
		return;
	
	//if power on for UTC wakeup
	if(get_sys_up_stat() == UTC_WAKEUP)
	{
		//任意RTC事件唤醒都默认深睡,TAU/DRX/eDRX唤醒由3GPP决定何时进深睡;用户事件或FOTA/DM/ONENET唤醒,需在做业务前自行申请锁,业务做完后释放锁,否则随时可能进深睡.
		HAVE_FREE_LOCK = 1;
		if(next_rtcinfo == &ap_rtc_event_arry[RTC_TIMER_LPM])
			g_RTC_wakeup_type = 1;
		else 
			g_RTC_wakeup_type = 2;
	}

}


/**
  * @brief	获取本地毫秒偏移时间
  * @param	msec_offset 偏移的毫秒数
  * @return	若mesc_offset为0则返回当前的本地毫秒时间，
  			非0则返回msec_offset毫秒之后的本地时间毫秒数，可在设置rtc定时器时调用。
  */
uint64_t rtc_get_global_byoffset(uint64_t msec_offset)
 {
	 struct rtc_time rtc_time;
	 uint64_t rtc_msec = 0;
 
 
	 rtc_timer_read(&rtc_time);
	 rtc_msec = msec_offset*XY_UTC_CLK/32000 + xy_mktime(&rtc_time);
	 

	 return rtc_msec;
 }

/**
  * @brief	根据毫秒偏移设置本地时间（写寄存器）
  * @param	ullms 偏移的毫秒数
  */
int rtc_timer_reset(uint64_t ullms)
 {
	 struct rtc_time rtc_time;
	 uint64_t rtc_msec = ullms;
	 
	 osCoreEnterCritical();
	 xy_gmtime_r(rtc_msec, &rtc_time);

	 SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

	 rtc_timer_stop();
	 rtc_timer_set(&rtc_time);
	 rtc_timer_run();

	 SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

	 osCoreExitCritical();
	 return 0;

 }

 int rtc_time_compare(struct rtc_time *rtc_time1, struct rtc_time *rtc_time2)
 {

	 uint64_t t1 = (uint64_t)rtc_time1->tm_msec + ((uint64_t)(rtc_time1->tm_sec) << 16) + ((uint64_t)(rtc_time1->tm_min) << 22) + ((uint64_t)(rtc_time1->tm_hour) << 28) + ((uint64_t)(rtc_time1->tm_mday) << 34) + ((uint64_t)(rtc_time1->tm_mon) << 40) + ((uint64_t)(rtc_time1->tm_year) << 46);
	 uint64_t t2 = (uint64_t)rtc_time2->tm_msec + ((uint64_t)(rtc_time2->tm_sec) << 16) + ((uint64_t)(rtc_time2->tm_min) << 22) + ((uint64_t)(rtc_time2->tm_hour) << 28) + ((uint64_t)(rtc_time2->tm_mday) << 34) + ((uint64_t)(rtc_time2->tm_mon) << 40) + ((uint64_t)(rtc_time2->tm_year) << 46);

	 if (t1 < t2)
	 {
		 return 0;
	 }
	 else
	 {
		 return 1;
	 }
 }

  static int rtc_time_compare_offset(struct rtc_time *rtc_time1, struct rtc_time *rtc_time2,uint32_t offset1_ms,uint32_t offset2_ms)
 {

	 uint64_t t1 = (uint64_t)rtc_time1->tm_msec + ((uint64_t)(rtc_time1->tm_sec) << 16) + ((uint64_t)(rtc_time1->tm_min) << 22) + ((uint64_t)(rtc_time1->tm_hour) << 28) + ((uint64_t)(rtc_time1->tm_mday) << 34) + ((uint64_t)(rtc_time1->tm_mon) << 40) + ((uint64_t)(rtc_time1->tm_year) << 46);
	 uint64_t t2 = (uint64_t)rtc_time2->tm_msec + ((uint64_t)(rtc_time2->tm_sec) << 16) + ((uint64_t)(rtc_time2->tm_min) << 22) + ((uint64_t)(rtc_time2->tm_hour) << 28) + ((uint64_t)(rtc_time2->tm_mday) << 34) + ((uint64_t)(rtc_time2->tm_mon) << 40) + ((uint64_t)(rtc_time2->tm_year) << 46);

	 if (t1+(uint64_t)offset1_ms < t2+(uint64_t)offset2_ms)
	 {
		 return 0;
	 }
	 else
	 {
		 return 1;
	 }
 }

int rtc_alarm_reset(uint64_t ullms)
{
	struct rtc_time rtc_time={0};
	struct rtc_time cur_rtctime={0};
	//struct rtc_time cur_time = {0};
	//struct rtc_time alarm_time = {0};
	uint64_t rtc_msec = ullms;
	//uint32_t clkcnt_reg;
	uint64_t cp_rtc_msec;
	
	osCoreEnterCritical();
	
	if(rtc_msec ==  RTC_ALARM_INVALID)
	{
		//ap has no alarm to set,maybe AP have deleted alarm,so set cp alarm

		SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

		Set_AP_ALARM_RAM(rtc_msec);
		cp_rtc_msec = Get_CP_ALARM_RAM();
		if(cp_rtc_msec != RTC_ALARM_INVALID)
		{
			xy_gmtime_r(cp_rtc_msec, &rtc_time);
			rtc_alarm_set(&rtc_time);
		}
		else
		{
			rtc_alarm_disable();
		}

		SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);
	}
	else
	{
		xy_gmtime_r(rtc_msec, &rtc_time);

		SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

		//可能rtc任务流程处理时事件还没到期，但由于一些诸如高优先级任务打断等原因，rtc_alarm_reset时该事件已过期
		rtc_timer_read(&cur_rtctime);
		Set_AP_ALARM_RAM(rtc_msec);
		if(rtc_time_compare_offset(&rtc_time, &cur_rtctime,0,11) == 0)
		{
			/*rtc_time < cur_rtctime + 11ms,若满足此条件，则之后必定满足is_rtc_event_expired()中的条件，将得到处理
				+11原因：若+10，rtc_time = 29，cur_rtctime = 19，29 == 19+10，alarm设置为20，出错
			*/
			osSemaphoreRelease(g_rtc_sem);
		}
		else
		{
			cp_rtc_msec = Get_CP_ALARM_RAM();
			if(rtc_msec < cp_rtc_msec)
			{
				rtc_alarm_set(&rtc_time);
			}
		}
		
		SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);
	}
	
	osCoreExitCritical();
	return 0;
}

int rtc_alarm_reset_accurate(uint64_t ullms)
{
	struct rtc_time rtc_time={0};
	//struct rtc_time cur_rtctime={0};
	//struct rtc_time cur_time = {0};
	//struct rtc_time alarm_time = {0};
	uint64_t rtc_msec = ullms;
	//uint32_t clkcnt_reg;
	uint64_t cp_rtc_msec;
	
	osCoreEnterCritical();
	/*
	if(rtc_msec ==  RTC_ALARM_INVALID)
	{
		//ap has no alarm to set,maybe AP have deleted alarm,so set cp alarm

		SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

		Set_AP_ALARM_RAM(rtc_msec);
		cp_rtc_msec = Get_CP_ALARM_RAM();
		if(cp_rtc_msec != RTC_ALARM_INVALID)
		{
			xy_gmtime_r(cp_rtc_msec, &rtc_time);
			rtc_alarm_set(&rtc_time);
		}
		else
		{
			rtc_alarm_disable();
		}

		SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);
	}
	else
	*/
	{
		xy_gmtime_r(rtc_msec, &rtc_time);

		SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

		//可能rtc任务流程处理时事件还没到期，但由于一些诸如高优先级任务打断等原因，rtc_alarm_reset时该事件已过期
		//rtc_timer_read(&cur_rtctime);
		Set_AP_ALARM_RAM(rtc_msec);
		//if(rtc_time_compare_offset(&rtc_time, &cur_rtctime,0,11) == 0)
		{
			/*rtc_time < cur_rtctime + 11ms,若满足此条件，则之后必定满足is_rtc_event_expired()中的条件，将得到处理
				+11原因：若+10，rtc_time = 29，cur_rtctime = 19，29 == 19+10，alarm设置为20，出错
			*/
			//osSemaphoreRelease(g_rtc_sem);
		}
		//else
		{
			cp_rtc_msec = Get_CP_ALARM_RAM();
			if(rtc_msec < cp_rtc_msec)
			{
				rtc_alarm_set_accurate(&rtc_time);
			}
		}
		
		SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);
	}
	
	osCoreExitCritical();
	return 0;
}

static int  insert_new_rtc_event(char timer_id, uint64_t rtc_msec,rtc_timeout_cb_t callback, void *data)
{
	//int index =0;
	struct rtc_event_info * rtc_info = NULL; 
	
	if(timer_id<RTC_TIMER_AP_BASE || timer_id > RTC_TIMER_AP_END)
		xy_assert(0);
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexAcquire(g_rtc_mutex, osWaitForever);
	ap_rtc_event_arry[(int)(timer_id)].timer_id = timer_id;
	ap_rtc_event_arry[(int)(timer_id)].rtc_msec = rtc_msec;
	ap_rtc_event_arry[(int)(timer_id)].callback = callback;
	ap_rtc_event_arry[(int)(timer_id)].data = data;
	
	rtc_info = rtc_get_next_event();
	//new rtc event is the latest,reset RTC register
	if(rtc_info == &ap_rtc_event_arry[(int)(timer_id)])
		rtc_alarm_reset(rtc_info->rtc_msec);
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexRelease(g_rtc_mutex);
	return 0;
}


//单核wfi使用该接口，其他事件禁用
static int  insert_new_rtc_event_accurate(char timer_id, uint64_t rtc_msec,rtc_timeout_cb_t callback, void *data)
{
	//int index =0;
	struct rtc_event_info * rtc_info = NULL; 
	
	if(timer_id<RTC_TIMER_AP_BASE || timer_id > RTC_TIMER_AP_END)
		xy_assert(0);
	//if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		//osMutexAcquire(g_rtc_mutex, osWaitForever);
	ap_rtc_event_arry[(int)(timer_id)].timer_id = timer_id;
	ap_rtc_event_arry[(int)(timer_id)].rtc_msec = rtc_msec;
	ap_rtc_event_arry[(int)(timer_id)].callback = callback;
	ap_rtc_event_arry[(int)(timer_id)].data = data;
	
	rtc_info = rtc_get_next_event_nomutex();
	//new rtc event is the latest,reset RTC register
	if(rtc_info == &ap_rtc_event_arry[(int)(timer_id)])
		rtc_alarm_reset_accurate(rtc_info->rtc_msec);
	//if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		//osMutexRelease(g_rtc_mutex);
	return 0;
}

/**
  * @brief	新增一个mesc_offset毫秒之后的定时器
  * @param	timer_ID: 定时器ID
  * @param	msec_offset 基于本地时间的毫秒偏移
  * @param	callback: 超时后的回调函数
  * @param	data：callback的输入参数
  */
int rtc_event_add_by_offset(char timer_id, uint64_t msec_offset, rtc_timeout_cb_t callback, void *data)
{
		
	uint64_t  rtc_msecond ;

	rtc_msecond = rtc_get_global_byoffset(msec_offset);
	
	insert_new_rtc_event(timer_id,rtc_msecond,callback,data);

	return 0;
}

//单核wfi专用
int rtc_event_add_by_offset_accurate(char timer_id, uint64_t msec_offset, rtc_timeout_cb_t callback, void *data)
{
		
	uint64_t  rtc_msecond ;

	rtc_msecond = rtc_get_global_byoffset(msec_offset);
	
	insert_new_rtc_event_accurate(timer_id,rtc_msecond,callback,data);

	return 0;
}


//return seconds for any ID next wakeup time offset
uint32_t rtc_get_next_offset_by_ID(char timer_id)
{
	uint64_t  rtc_msecond ;
	uint64_t offset=0;
	rtc_msecond = rtc_get_global_byoffset(0);

	if(ap_rtc_event_arry[(int)(timer_id)].rtc_msec!=0 && ap_rtc_event_arry[(int)(timer_id)].rtc_msec>=rtc_msecond)
		offset = (ap_rtc_event_arry[(int)(timer_id)].rtc_msec - rtc_msecond) * 32 / XY_UTC_CLK;
	
	//if reset,rtc phy will reset to 2018/10/1
	return (uint32_t)offset;
}

int rtc_event_add_by_global(char timer_id, uint64_t rtc_msec, rtc_timeout_cb_t callback, void *data)
{		
	insert_new_rtc_event(timer_id,rtc_msec,callback,data);

	return 0;
}

#if 0
int  rtc_event_change(char timer_id, uint64_t rtc_msec, rtc_timeout_cb_t callback, void *data)
{
	struct rtc_event_info * rtc_event = NULL;
	struct rtc_event_info * rtc_event_old = NULL;
	
	osMutexAcquire(g_rtc_mutex, osWaitForever);
	if(timer_id<RTC_TIMER_AP_BASE || timer_id > RTC_TIMER_AP_END)
		xy_assert(0);
	rtc_event_old = rtc_get_next_event();
	
	ap_rtc_event_arry[(int)(timer_id)].timer_id = timer_id;
	ap_rtc_event_arry[(int)(timer_id)].rtc_msec = rtc_msec;
	ap_rtc_event_arry[(int)(timer_id)].callback = callback;
	ap_rtc_event_arry[(int)(timer_id)].data = data;
	
	rtc_event = rtc_get_next_event();
	xy_assert(rtc_event!=NULL);
	if(rtc_event==&ap_rtc_event_arry[(int)(timer_id)] || rtc_event_old==&ap_rtc_event_arry[(int)(timer_id)])
		rtc_alarm_reset(ap_rtc_event_arry[(int)(timer_id)].rtc_msec);
	osMutexRelease(g_rtc_mutex);
	return 0;
	
}
#endif

 int rtc_event_delete(char timer_id)
 {
	struct rtc_event_info * rtc_info = NULL;

	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexAcquire(g_rtc_mutex, osWaitForever);    
	rtc_info = ap_rtc_event_arry+timer_id;
	if(rtc_info == rtc_get_next_event())
	{
	   memset(rtc_info, 0, sizeof(struct rtc_event_info));
	   rtc_info = rtc_get_next_event();
	   if(rtc_info!=NULL)
	   		rtc_alarm_reset(rtc_info->rtc_msec);
	   else
	   		rtc_alarm_reset(RTC_ALARM_INVALID);
	}
	else
	{
	   memset(rtc_info, 0, sizeof(struct rtc_event_info));
	}
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexRelease(g_rtc_mutex);

	return 0;
 }
 
 int rtc_event_delete_nomutex(char timer_id)
 {
	struct rtc_event_info * rtc_info = NULL;

	rtc_info = ap_rtc_event_arry+timer_id;
	if(rtc_info == rtc_get_next_event_nomutex())
	{
	   memset(rtc_info, 0, sizeof(struct rtc_event_info));
	   rtc_info = rtc_get_next_event_nomutex();
	   if(rtc_info!=NULL)
	   		rtc_alarm_reset(rtc_info->rtc_msec);
	   else
	   		rtc_alarm_reset(RTC_ALARM_INVALID);
	}
	else
	{
	   memset(rtc_info, 0, sizeof(struct rtc_event_info));
	}

	return 0;
 }


void rtc_event_refresh()
{
	struct rtc_event_info * rtc_info = NULL;

	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexAcquire(g_rtc_mutex, osWaitForever);    
	rtc_info = rtc_get_next_event();
	
	if(rtc_info!=NULL)
	{
		rtc_alarm_reset(rtc_info->rtc_msec);
	}	
	else
	{
		rtc_alarm_reset(RTC_ALARM_INVALID);
	}
		
	if(osKernelGetState() != osKernelInactive && osKernelIsRunningIdle() != osOK)
		osMutexRelease(g_rtc_mutex);

}


#if 0
 //for entering deepsleep when user rtc alarm happens soon,should reset user rtc alarm with adding 1s
 void reset_user_alarm_deepsleep()
 {
 
	struct rtc_time rtc_time_current;
	struct rtc_event_info * rtc_info = NULL;
	uint64_t rtc_msec,cp_rtc_tmr_msec;
	uint8_t	rtc_id;

	//osMutexAcquire(g_rtc_mutex, osWaitForever);	  

	rtc_info = rtc_get_next_event();
	
	SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

	cp_rtc_tmr_msec = Get_CP_ALARM_RAM();

	SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

	rtc_timer_read(&rtc_time_current);
	rtc_msec = 1000*XY_UTC_CLK/32000 + xy_mktime(&rtc_time_current);//add 1000ms for comparison
	 	
	//user rtc is in effect
	if( (rtc_info != NULL) && (rtc_info >= ap_rtc_event_arry+RTC_TIMER_AP_USER_BASE))
	{
	 	for(rtc_id = RTC_TIMER_AP_USER_BASE;rtc_id <= RTC_TIMER_AP_USER_END;rtc_id++ )
	 	{
		 	if(ap_rtc_event_arry[rtc_id].rtc_msec < rtc_msec && ap_rtc_event_arry[rtc_id].rtc_msec != 0)
		 	{
			 	ap_rtc_event_arry[rtc_id].rtc_msec = rtc_msec;
		 	}
	 	}
	}
	if(cp_rtc_tmr_msec < rtc_msec)
	{
		SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

		Set_CP_ALARM_RAM(rtc_msec);

		SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);
	}

	rtc_event_refresh();
	 
	 //osMutexRelease(g_rtc_mutex);
 }
#endif

void rtc_event_proc()
{
	struct rtc_event_info * rtc_info = NULL;
	struct rtc_event_info  temp = {0};

	osMutexAcquire(g_rtc_mutex, osWaitForever);
	
	deal_event:
		rtc_info = rtc_get_next_event();

	if(rtc_info&&is_rtc_event_expired(rtc_info))
	{		 
		//because temp.callback maybe reset current RTC,so must memset here
		memcpy(&temp,rtc_info,sizeof(struct rtc_event_info));
		memset(rtc_info, 0, sizeof(struct rtc_event_info));
	
		if(temp.callback)
		{
			temp.callback(temp.data);
		}
		goto deal_event;		 	 	 
	}
	else if(rtc_info!=NULL)
		rtc_alarm_reset(rtc_info->rtc_msec);
	else
		rtc_alarm_reset(RTC_ALARM_INVALID);

	osMutexRelease(g_rtc_mutex);
}


void rtc_tmr_entry()
{
	osSemaphoreRelease(g_rtc_sem);	
	while(1)
	{
		if (osSemaphoreAcquire(g_rtc_sem, osWaitForever) != osOK)
			continue;
		
		rtc_event_proc();
	}
	
}

void rtc_task_init()
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = RTC_TMR_THREAD_NAME;
	thread_attr.priority   = osPriorityNormal3;
	thread_attr.stack_size = 0x400;
	osThreadNew ((osThreadFunc_t)(rtc_tmr_entry),NULL,&thread_attr);
}

extern int xy_srand(uint32_t key);

/**
  * @brief	重置世界时间秒偏移cclk_netl_sec和本地时间秒偏移cclk_local_sec
  * @param	rtctime：需重置的世界时间变量，年/月/日，时：分：秒（格林威治时间）
  * @param	zone_sec: 时区偏移秒数
  */
int reset_universal_timer(struct rtc_time *rtctime,int zone_sec)
{
	uint64_t  old_time = rtc_get_global_byoffset(0);
	uint64_t  new_time = xy_mktime(rtctime);

	if(rtctime->tm_year < 2020)
	{
		softap_printf(USER_LOG, WARN_LOG, "USER set universal time ERROR!year=%d",rtctime->tm_year);
	}
	
	g_softap_var_nv->cclk_netl_sec = (new_time+500)/1000+zone_sec;

	if(g_softap_fac_nv->xtal_32k == 0)
	{
		//晶振频率修正
		g_softap_var_nv->cclk_local_sec = (old_time+500)/1000 * 32000 * 10 / (g_softap_fac_nv->rc32k_freq);
	}
	else
	{		
		g_softap_var_nv->cclk_local_sec = osKernelGetTickCount()/1000;
	}
	
	xy_srand(new_time/1000+(uint32_t)HWREG(SYS_UP_REASON+32-4));

	return 0;
}


/**
  * @brief	根据世界时间快照和本地时间偏移获取世界时间
  * @param	rtctime：获取到的世界时间变量，年/月/日，时：分：秒（格林威治时间）
  * @param	zone_sec: 时区偏移秒数
  * @return	0：获取异常	1：正常
  * @note    用户使用该接口时，需要关注是否能够获取到有效值.当未attach成功时，返回异常。
  * @attention
  */
int get_universal_timer(struct rtc_time *rtctime,int  zone_sec)
{
	if(g_softap_var_nv->cclk_netl_sec != 0)
	{
		uint64_t  real_msec;
		uint64_t  now_sec;

		if(g_softap_fac_nv->xtal_32k == 0)
		{
			//晶振频率修正
		now_sec = (rtc_get_global_byoffset(0)+500)/1000 * 32000 * 10 / (g_softap_fac_nv->rc32k_freq);
		}
		else
		{
			now_sec = osKernelGetTickCount()/1000;
		}
		//abnormal process
		if(now_sec < g_softap_var_nv->cclk_local_sec || g_softap_var_nv->cclk_local_sec == 0)
		{
			g_softap_var_nv->cclk_local_sec = 0;
			softap_printf(USER_LOG, WARN_LOG, "CCLK NV param abnormal!");
			return 0;
		}

		real_msec = (g_softap_var_nv->cclk_netl_sec + now_sec-g_softap_var_nv->cclk_local_sec - zone_sec) * 1000;

		xy_gmtime_r(real_msec,rtctime);
	}
	else
	{
		softap_printf(USER_LOG, WARN_LOG, "NB not attach,returl local time,such as 2018/10/1!!!");
		return 0;
	}
	return 1;
}


uint32_t get_system_sec()
{
	uint64_t  now_sec;

	if(g_softap_fac_nv->xtal_32k == 0)
	{
		//晶振频率修正
			now_sec = (rtc_get_global_byoffset(0)+500)/1000 * 32000 * 10 / (g_softap_fac_nv->rc32k_freq);
	}
	else
	{
		now_sec = osKernelGetTickCount()/1000;
	}
	return  (uint32_t)now_sec;
}


