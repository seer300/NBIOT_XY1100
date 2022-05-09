#include "lpm_adapt.h"
#include "rtc_adapt.h"
#include "rtc_tmr.h"
#include "utc.h"

uint32_t lpmSavePrimask = 0;
uint32_t lpmCriticalNesting = 0;

void lpm_EnterCritical(void)
{
	uint32_t save_primask;

	__asm volatile
	(
		"	mrs %0, primask											\n"
		"	cpsid i													\n"
		"	isb														\n"
		"	dsb														\n"
		: "=r" (save_primask)
		: : "memory"
	);

	if(lpmCriticalNesting == 0)
	{
		lpmSavePrimask = save_primask;
	}

	lpmCriticalNesting++;
}

void lpm_ExitCritical(void)
{
	uint32_t save_primask;

	if(lpmCriticalNesting > 0)
	{
		lpmCriticalNesting--;
	}
	else
	{
		return;
	}

	if(lpmCriticalNesting == 0)
	{
		save_primask = lpmSavePrimask;

		__asm volatile
		(
			"	msr primask, %0											\n"
			"	isb														\n"
			"	dsb														\n"
			: : "r" (save_primask)
			: "memory"
		);
	}
}

int lpm_rtc_event_add_by_global_critical(char timer_id, uint64_t rtc_msec, rtc_timeout_cb_t callback, void *data)
{		
	rtc_event_add_by_global(timer_id,rtc_msec,callback,data);

	return 0;
}

int lpm_rtc_event_add_by_offset_critical(char timer_id, uint64_t msec_offset, rtc_timeout_cb_t callback, void *data)
{
		
	rtc_event_add_by_offset(timer_id,msec_offset,callback,data);

	return 0;
}

int lpm_rtc_event_delete_critical(char timer_id)
{
   rtc_event_delete(timer_id);

   return 0;
}


int lpm_rtc_event_add_by_offset_singlecore_wfi(char timer_id, uint64_t msec_offset, rtc_timeout_cb_t callback, void *data)
{
		
	rtc_event_add_by_offset_accurate(timer_id,msec_offset,callback,data);

	return 0;
}

int lpm_rtc_event_delete_singlecore_wfi(char timer_id)
{
	HWREG(UTC_BASE + UTCALARMCLKCNT_CFG) = 0;
   	rtc_event_delete(timer_id);

   	return 0;
}


#if (LPM_LITEOS == 1)

uint32_t xy_lpm_GetExpectedIdleTick(uint8_t sleep_mode)
{
	uint32_t uwSwtmrSortLinkTicks,uwTskSortLinkTicks,uwSleepTicks;

	uwTskSortLinkTicks  = xy_osTaskNextSwitchTimeGet(sleep_mode);
	if(sleep_mode == LPM_STANDBY)
		uwSwtmrSortLinkTicks = osSwTmrGetNextTimeout();
	else if(sleep_mode == LPM_WFI)
		uwSwtmrSortLinkTicks = osSwTmrGetNextTimeoutAll();

	uwSleepTicks = (uwTskSortLinkTicks < uwSwtmrSortLinkTicks) ? uwTskSortLinkTicks : uwSwtmrSortLinkTicks;
	return uwSleepTicks;
}

extern uint64_t      g_ullTickCount;//LITEOS define
void lpm_osTickHandlerLoop(uint32_t uwElapseTicks)
{
	g_ullTickCount+=uwElapseTicks;
	xy_osTaskScan(uwElapseTicks);
	xy_osSwtmrScan(uwElapseTicks);
}

extern volatile BOOL bWfiNeedSchedule;//xinyi define
void lpm_ScheduleAfterSleep(void)
{
	if(bWfiNeedSchedule)
    {
        LOS_Schedule();
    }
}

#elif (LPM_FREERTOS == 1)

//extern TickType_t prvGetExpectedIdleTime_xy( void );
//extern TickType_t prvGetNextExpireTime_xy();
//extern TickType_t prvGetExpectedIdleTime( void );
//extern TickType_t prvGetNextExpireTime( BaseType_t * const pxListWasEmpty );
//extern void vTaskStepTick_xy( const TickType_t xTicksToJump );

uint32_t lpm_GetExpectedIdleTick(uint8_t sleep_mode)
{
	uint32_t uwTaskSleepTicks, uwSwtmrSleepTicks, uwSleepTicks=0;
	//BaseType_t pxListWasEmpty;
	if(sleep_mode == LPM_STANDBY)
	{
		uwTaskSleepTicks = osThreadGetLowPowerTime(osAttentionLowPowerFlag);
		uwSwtmrSleepTicks = osTimerGetLowPowerTime(osAttentionLowPowerFlag);
		uwSleepTicks = (uwTaskSleepTicks < uwSwtmrSleepTicks) ? uwTaskSleepTicks : uwSwtmrSleepTicks;
	}
	else if(sleep_mode == LPM_WFI)
	{
		uwSleepTicks = osThreadGetLowPowerTime(osIgnorLowPowerFlag);
	}
	
	return uwSleepTicks;
}


void lpm_osTickHandlerLoop(uint32_t uwElapseTicks)
{
	
	vTaskSleepStepTick(uwElapseTicks);
}


void lpm_ScheduleAfterSleep(void)
{
	//taskYIELD();
}

#endif

