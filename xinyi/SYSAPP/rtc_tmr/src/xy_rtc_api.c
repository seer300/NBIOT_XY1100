#include "xy_utils.h"
#include "xy_rtc_api.h"
#include "xy_at_api.h"
#include "xy_net_api.h"
#include "softap_nv.h"
#include "lwip/apps/sntp.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"
#include "osal.h"

//for to reduce BSS stress,and improve communication success rate,user task must set rand UTC,see  xy_rtc_set_by_day
int xy_rtc_timer_create(char timer_id, int sec_offset, rtc_timeout_cb_t callback, void *data)
{
	uint64_t msec = ((uint64_t)sec_offset) * 1000;
	rtc_event_add_by_offset(timer_id, msec, callback, data);
	return XY_OK;
}

//return seconds for any ID next wakeup time offset
uint32_t xy_rtc_next_offset_by_ID(char timer_id)
{
	return rtc_get_next_offset_by_ID(timer_id);
}

//return seconds,for get last work time offset to do some decide,such as give up current work
uint32_t xy_get_last_work_offset()
{
	uint64_t rtc_msecond;
	uint64_t offset = 0;
	rtc_msecond = rtc_get_global_byoffset(0);

	if(g_softap_var_nv->rtc_snap_msec != 0 && g_softap_var_nv->rtc_snap_msec < rtc_msecond)
		offset = (rtc_msecond/1000 - g_softap_var_nv->rtc_snap_msec) * 32 / XY_UTC_CLK;
	
	return (uint32_t)offset;
}

//return seconds,indicate have working time
uint32_t xy_get_working_time()
{
	return (osKernelGetTickCount() / 1000);
}

int xy_rtc_timer_delete(char timer_id)
{
	rtc_event_delete(timer_id);
	return XY_OK;
}

int xy_rtc_set_time(struct xy_wall_clock *wall_clock,int zone_sec)
{
	struct rtc_time rtctime={0};
	
	rtctime.tm_year=wall_clock->tm_year;
	rtctime.tm_mon=wall_clock->tm_mon;
	rtctime.tm_mday=wall_clock->tm_mday;
	rtctime.tm_hour=wall_clock->tm_hour;
	rtctime.tm_min=wall_clock->tm_min;
	rtctime.tm_sec=wall_clock->tm_sec;

	reset_universal_timer(&rtctime,zone_sec);
	return XY_OK;
}

//get beijing year/mon/day/hour,and not care zone;if not attached,return 0
int xy_rtc_get_time(struct xy_wall_clock *wall_clock)
{
	struct rtc_time rtctime={0};

	//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
	if (get_universal_timer(&rtctime, 0) == 0)
		return XY_ERR;

	wall_clock->tm_year = rtctime.tm_year;
	wall_clock->tm_mon = rtctime.tm_mon;
	wall_clock->tm_mday = rtctime.tm_mday;
	wall_clock->tm_hour = rtctime.tm_hour;
	wall_clock->tm_min = rtctime.tm_min;
	wall_clock->tm_sec = rtctime.tm_sec;
	return XY_OK;
}

//return year/mon/day/hour...;offset_sec is sec offset,maybe negative value
int xy_rtc_get_UT_offset(struct xy_wall_clock *wall_clock,int offset_sec)
{
	//int ret = 0;
	struct rtc_time rtctime={0};

	//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
	if(get_universal_timer(&rtctime,(0-offset_sec)) == 0)
		return XY_ERR;

	wall_clock->tm_year = rtctime.tm_year;
	wall_clock->tm_mon = rtctime.tm_mon;
	wall_clock->tm_mday = rtctime.tm_mday;
	wall_clock->tm_hour = rtctime.tm_hour;
	wall_clock->tm_min = rtctime.tm_min;
	wall_clock->tm_sec = rtctime.tm_sec;
	return XY_OK;
}

/*if get UT time,must have attached,else return 0*/
uint32_t xy_rtc_get_sec(int need_UT_time)
{
	uint32_t now_sec = get_system_sec();
	if(need_UT_time == 1)
	{
		if(g_softap_var_nv->cclk_netl_sec != 0)
		{
			//abnormal process
			if(now_sec < g_softap_var_nv->cclk_local_sec || g_softap_var_nv->cclk_local_sec == 0)
			{
				g_softap_var_nv->cclk_local_sec = 0;
				g_softap_var_nv->cclk_netl_sec = 0;
				softap_printf(USER_LOG, WARN_LOG, "UT time ERROR!!!");
				return 0;
			}
			
			return (g_softap_var_nv->cclk_netl_sec + (now_sec - g_softap_var_nv->cclk_local_sec - ((int)g_softap_var_nv->g_zone * 15 * 60)));
		}
		else
		{
			softap_printf(USER_LOG, WARN_LOG, "error!NB not attach!!!");
			return 0;
		}
	}
	else
		return now_sec;
}

/*if set 15:00--18:00 everyday to do user work,then call xy_rtc_set_by_day(RTC_TIMER_USER1,15,3*60*60,0,0)*/
int xy_rtc_set_by_day(char timer_id, rtc_timeout_cb_t callback, void *data, int hour_start, int sec_span, int min, int sec)
{
	int sec_offset;
	struct xy_wall_clock now_wall_time={0};

	//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
	if (xy_rtc_get_time(&now_wall_time) == XY_ERR)
		return XY_ERR;

	sec_offset = (hour_start-now_wall_time.tm_hour)*3600+(min-now_wall_time.tm_min)*60+(sec-now_wall_time.tm_sec);

	if(sec_offset <= 0)
		sec_offset += 3600*24;

	//set rand work time  to reduce BSS stress
	xy_rtc_timer_create(timer_id,sec_offset+(rand()%sec_span),callback,data);
	return XY_OK;
}

/*if set sunday 15:00--20:00 every week to do user work,then call xy_rtc_set_by_week(RTC_TIMER_USER1,7,15,5,0,0)*/
/*day_week  is 1-7*/
int xy_rtc_set_by_week(char timer_id, rtc_timeout_cb_t callback, void *data, int day_week, int hour_start, int sec_span, int min, int sec)
{
	int sec_offset;
	struct rtc_time rtctime={0};

	//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
	if(get_universal_timer(&rtctime,0) == 0)
		return XY_ERR;

	sec_offset = (day_week-rtctime.tm_wday)*3600*24+(hour_start-rtctime.tm_hour)*3600+(min-rtctime.tm_min)*60+(sec-rtctime.tm_sec);
	if(sec_offset <= 0)
		sec_offset += 7*3600*24;

	//set rand work time  to reduce BSS stress
	xy_rtc_timer_create(timer_id,sec_offset+(rand()%sec_span),callback,data);
	return XY_OK;
}

//return year/mon/day/hour...;if not attached,return -1
int xy_get_ntp_time(const char *ser_name, int timeout, struct xy_wall_clock *wall_clock)
{
	struct sntp_time *ptimemap = NULL;
	int zone_sec = 0;
	struct rtc_time rtctime = {0};

	if (timeout < 10)
	{
		timeout = 20;
	}

	if (xy_wait_tcpip_ok(1) == XY_ERR)
	{
		softap_printf(USER_LOG, WARN_LOG, "error!set rtc by ntp,but netif is not ok");
		return XY_ERR;
	}

	if (ser_name == NULL)
		ptimemap = gettimebysntp("ntp1.aliyun.com", 0, timeout-1);
	else
		ptimemap = gettimebysntp(ser_name, 0, timeout-1);

	if (ptimemap == NULL)
	{
		softap_printf(USER_LOG, WARN_LOG, "error!set rtc by ntp,fail get time!");
		return (errno == SNTP_ERR_TIMEOUT) ? XY_Err_Timeout : XY_ERR;
	}
	if(g_softap_var_nv->g_zone != 0)
		zone_sec = (int)g_softap_var_nv->g_zone*15*60;
	else
		zone_sec = 8*60*60;
	
	xy_gmtime_r((((uint64_t)ptimemap->sec)+zone_sec)*1000,&rtctime);
	
	reset_universal_timer(&rtctime,0);
	
	wall_clock->tm_year = rtctime.tm_year;
	wall_clock->tm_mon = rtctime.tm_mon;
	wall_clock->tm_mday = rtctime.tm_mday;
	wall_clock->tm_hour = rtctime.tm_hour;
	wall_clock->tm_min = rtctime.tm_min;
	wall_clock->tm_sec = rtctime.tm_sec;
	return XY_OK;
}

int xy_set_RTC_by_NTP(const char *ser_name, int timeout, int zone_sec)
{
	struct sntp_time *ptimemap = NULL;
	struct rtc_time rtctime = {0};

	if (timeout < 10)
	{
		timeout = 20;
	}

	if (xy_wait_tcpip_ok(1) == XY_ERR)
	{
		softap_printf(USER_LOG, WARN_LOG, "error!set rtc by ntp,but netif is not ok");
		return XY_ERR;
	}

	if (ser_name == NULL)
		ptimemap = gettimebysntp("ntp1.aliyun.com", 0, timeout - 1);
	else
		ptimemap = gettimebysntp(ser_name, 0, timeout - 1);

	if (ptimemap == NULL)
	{
		softap_printf(USER_LOG, WARN_LOG, "error!set rtc by ntp,fail get time!");
		return (errno == SNTP_ERR_TIMEOUT) ? XY_Err_Timeout : XY_ERR;
	}

	xy_gmtime_r((((uint64_t)ptimemap->sec) + zone_sec) * 1000, &rtctime);

	reset_universal_timer(&rtctime,0);

	return XY_OK;
}