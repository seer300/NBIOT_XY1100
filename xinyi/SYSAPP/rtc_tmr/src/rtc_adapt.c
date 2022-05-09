#include "rtc_adapt.h"
#include "hw_utc.h"
#include "sema.h"
#include "xy_utils.h"
#include "utc.h"

/**
 * Judge if the year is leapyear.
 *
 * @param[in]   year      The year to judge.
 * @return      1 or 0.
 */
static inline int clock_is_leapyear(int year)
{
	return (year % 400) ? ((year % 100) ? ((year % 4) ? 0 : 1) : 0) : 1;
}

static const uint16_t g_daysbeforemonth[13] =
{
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};


/**
 * Claculate the days before a specified month,month 0-based.
 *
 * @param[in]   month      The specified month.
 * @param[in]   year      Leapyear check.
 * @return      Sum days.
 */
int clock_days_before_month(int month, bool leapyear)
{
	int retval = g_daysbeforemonth[month];
	if (month >= 2 && leapyear) {
	  retval++;
	}
	return retval;
}

/**
 * Convert calendar to utc time.
 *
 * @param[in]   year      The specified year.
 * @param[in]   month      The specified month.
 * @param[in]   day      The specified day.
 * @return      Sum days.
 */
uint32_t clock_calendar_to_utc(int year, int month, int day)
{
	uint32_t days;
	/* Years since epoch in units of days (ignoring leap years). */
	days = (year - 1970) * 365;
	/* Add in the extra days for the leap years prior to the current year. */
	days += (year - 1969) >> 2;
	/* Add in the days up to the beginning of this month. */
	days += (uint32_t)clock_days_before_month(month, clock_is_leapyear(year));
	/* Add in the days since the beginning of this month (days are 1-based). */
	days += day - 1;
	/* Then convert the seconds and add in hours, minutes, and seconds */
	return days;
}


static int clock_dayoftheweek(int mday, int month, int year)
{
  	if((month==1)||(month==2)) {  
		month+=12;  
		year--;  
	}  
	
	return (mday + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400)%7 + 1;  
}

static void clock_utc_to_calendar(time_t64 days, int *year, int *month, int *day)
{
  int  value=0;
  int  min=0;
  int  max=0;
  int  tmp=0;
  bool leapyear=0;

  /* There is one leap year every four years, so we can get close with the
   * following:
   */
  value   = days  / (4 * 365 + 1); /* Number of 4-years periods since the epoch */
  days   -= (time_t64)(value) * (4 * 365 + 1); /* Remaining days */
  value <<= 2;                     /* Years since the epoch */

  /* Then we will brute force the next 0-3 years */

  for (; ; ) {
      /* Is this year a leap year (we'll need this later too) */
      leapyear = clock_is_leapyear(value + 1970);

      /* Get the number of days in the year */
      tmp = (leapyear ? 366 : 365);

      /* Do we have that many days? */
      if (days >= (time_t64)(tmp)) {
           /* Yes.. bump up the year */
           value++;
           days -= tmp;
        }
      else {
           /* Nope... then go handle months */
           break;
        }
    }

  /* At this point, value has the year and days has number days into this year */
  *year = 1970 + value;

  /* Handle the month (zero based) */
  min = 0;
  max = 11;

  do {
      /* Get the midpoint */
      value = (min + max) >> 1;

      /* Get the number of days that occurred before the beginning of the month
       * following the midpoint.
       */
      tmp = clock_days_before_month(value + 1, leapyear);

      /* Does the number of days before this month that equal or exceed the
       * number of days we have remaining?
       */
      if ((time_t64)(tmp) > days) {
          /* Yes.. then the month we want is somewhere from 'min' and to the
           * midpoint, 'value'.  Could it be the midpoint?
           */
          tmp = clock_days_before_month(value, leapyear);
          if ((time_t64)(tmp) > days) {
              /* No... The one we want is somewhere between min and value-1 */
              max = value - 1;
            }
          else {
              /* Yes.. 'value' contains the month that we want */
              break;
            }
        }
      else {
           /* No... The one we want is somwhere between value+1 and max */
           min = value + 1;
        }

      /* If we break out of the loop because min == max, then we want value
       * to be equal to min == max.
       */
      value = min;
    }
  while (min < max);

  /* The selected month number is in value. Subtract the number of days in the
   * selected month
   */
  days -= clock_days_before_month(value, leapyear);

  /* At this point, value has the month into this year (zero based) and days has
   * number of days into this month (zero based)
   */
  *month = value + 1; /* 1-based */
  *day   = days + 1;  /* 1-based */
}


/*from 1970/1/1, msec*/
uint64_t xy_mktime(struct rtc_time *tp)   
{   
   uint32_t msec,sec,min,hour,mday,mon,year;
   
   uint64_t ret;
   msec = tp->tm_msec,
   sec = tp->tm_sec;
   min = tp->tm_min;
   hour = tp->tm_hour;
   mday=tp->tm_mday;
   mon=tp->tm_mon;
   year=tp->tm_year;
   if(sec==0&&min==0&&hour==0&&mday==0&&mon==0&&year==0)
   		return 0;
    /* 1..12 -> 11,12,1..10 */   
    if (0 >= (int) (mon -= 2)) {   
        mon += 12;  /* Puts Feb last since it has leap day */   
        year -= 1;   
    }   
	ret = (((((uint64_t)   \
          (year/4 - year/100 + year/400 + (uint64_t)367*mon/12 + mday) +   \
          (uint64_t)year*365 - 719499   \
        )*24 + (uint64_t)hour \
      )*60 + (uint64_t)min \
    )*60 + (uint64_t)sec)*1000 + (uint64_t)msec;

	
    return ret;
} 

//get year/mon/day/hour/min/sec by msec,and msec is 1970/1/1 offset
void xy_gmtime_r(const uint64_t msec, struct rtc_time *result)
{
  time_t64 epoch=0;
  time_t64 jdn=0;
  uint32_t    year=0;
  uint32_t    month=0;
  uint32_t    day=0;
  uint32_t    hour=0;
  uint32_t    min=0;
  uint32_t    sec=0;

  /* Get the seconds since the EPOCH */
  epoch = (msec)/1000;

  /* Convert to days, hours, minutes, and seconds since the EPOCH */
  jdn    = epoch / SEC_PER_DAY;
  epoch -= SEC_PER_DAY * jdn;

  hour   = epoch / SEC_PER_HOUR;
  epoch -= SEC_PER_HOUR * hour;

  min    = (uint32_t)epoch / SEC_PER_MIN;
  epoch -= SEC_PER_MIN * min;

  sec    = epoch;

  /* Convert the days since the EPOCH to calendar day */
  clock_utc_to_calendar(jdn, (int *)&year, (int *)&month, (int *)&day);

  /* Then return the struct tm contents */
  result->tm_year  = (uint32_t)year; /* Relative to 1900 */
  result->tm_mon   = (uint32_t)month;   /* one-based */
  result->tm_mday  = (uint32_t)day;         /* one-based */
  result->tm_hour  = (uint32_t)hour;
  result->tm_min   = (uint32_t)min;
  result->tm_sec   = (uint32_t)sec;
  result->tm_msec  = (uint32_t)((msec)%1000);

  result->tm_wday  = clock_dayoftheweek(day, month, year);
  result->tm_yday  = day + clock_days_before_month(result->tm_mon-1, clock_is_leapyear(year));
  result->tm_isdst = 0;
}




/**
 * @brief Read the rtc time.
 *
 * This routine read the rtc time.
 *
 * @param	dev		Pointer to the device structure for the driver instance.
 * @param   rtctime	The rtc time.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
int rtc_timer_read(struct rtc_time *rtctime)
{
	uint8_t ulCentury=0;
	uint8_t ulYear=0;
	uint8_t ulMonth=0;
	uint8_t ulData;
	uint8_t ulDay;	

	uint8_t ulAMPM;
	uint8_t ulHour;
	uint8_t ulMin;
	uint8_t ulSec;
	uint8_t ulMinSec;
	uint32_t ulclkcnt;
	//uint32_t intsave;

	volatile uint32_t cnttmp;
	volatile uint32_t utc_delay;
	volatile uint8_t 	carryflag = 0 ;
	
	osCoreEnterCritical();
	cnttmp = HWREG(UTC_BASE + UTC_CLK_CNT);
	//UTCTimerGet(&ulAMPM, &ulHour, &ulMin,&ulSec, &ulMinSec,0);
	//UTCCalGet(&ulCentury,&ulYear, &ulMonth,&ulData,&ulDay,0);
	while(cnttmp==159||cnttmp==415)
	{
		cnttmp = HWREG(UTC_BASE + UTC_CLK_CNT);
		carryflag=1;
	}	
	if(carryflag)             //bug3356
		for(utc_delay=0;utc_delay<10;utc_delay++);
	UTCTimerGet(&ulAMPM, &ulHour, &ulMin,&ulSec, &ulMinSec,0);
	UTCCalGet(&ulCentury,&ulYear, &ulMonth,&ulData,&ulDay,0);
	ulclkcnt = UTCClkCntConvert(cnttmp);

	osCoreExitCritical();

	
	rtctime->tm_hour = ulAMPM*12+ulHour;//默认为24小时制，ulAMPM为0；此处需要防止被修改为AM/PM模式
	rtctime->tm_min = ulMin;
	rtctime->tm_sec = ulSec;
	rtctime->tm_msec = ulMinSec*10+ulclkcnt/32;
	rtctime->tm_mday = ulData;
	rtctime->tm_mon = ulMonth;
	rtctime->tm_year = ulCentury*100 + ulYear;
	rtctime->tm_wday = ulDay;
	rtctime->tm_yday  = ulData + clock_days_before_month(rtctime->tm_mon-1, clock_is_leapyear(rtctime->tm_year));
	rtctime->tm_isdst = 0;
	return 0;
}

/**
  * @brief	设置本地时间（写寄存器）
  * @param	要设置的时间，年/月/日，时：分：秒
  */
int rtc_timer_set(struct rtc_time *rtctime)
{
	uint8_t ulCentury = (rtctime->tm_year)/100;
	uint8_t ulYear = (rtctime->tm_year)%100;
	uint8_t ulMonth = rtctime->tm_mon;
	uint8_t ulData = rtctime->tm_mday;
	uint8_t ulDay = rtctime->tm_wday;

	uint8_t ulAMPM = 0;
	uint8_t ulHour = rtctime->tm_hour;
	uint8_t ulMin = rtctime->tm_min;
	uint8_t ulSec = rtctime->tm_sec;
	uint8_t ulMinSec = rtctime->tm_msec/10;

	UTCCalSet(ulCentury,ulYear, ulMonth,ulData,ulDay);

	UTCTimerSet(ulAMPM, ulHour, ulMin,ulSec, ulMinSec);
return 0;
}



/**
 * @brief Set alarm and registe callback function.
 *
 * This routine set alarm and registe callback function.
 *
 * @param	dev		Pointer to the device structure for the driver instance.
 * @param   rtctime    The time of alarm.
 * @param   callback   Callback function for registe.
 * @param   arg        Argument of Callback.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
int rtc_alarm_set(struct rtc_time *rtctime)
{
	UTCCalAlarmSet(rtctime->tm_mon, rtctime->tm_mday);
	UTCTimerAlarmSet(0,rtctime->tm_hour,rtctime->tm_min, rtctime->tm_sec, rtctime->tm_msec);
	UTCAlarmEnable(UTC_ALARM_ALL);
	UTCIntEnable(UTC_INT_ALARM);
	return 0;
}

int rtc_alarm_set_accurate(struct rtc_time *rtctime)
{
	
	UTCAlarmDisable(UTC_ALARM_ALL);
	
	UTCCalAlarmSet(rtctime->tm_mon, rtctime->tm_mday);
	UTCTimerAlarmSetAccurate(0,rtctime->tm_hour,rtctime->tm_min, rtctime->tm_sec, rtctime->tm_msec);

	UTCAlarmEnable(UTC_ALARM_ALL);
	UTCIntEnable(UTC_INT_ALARM);
	return 0;
}


void rtc_alarm_clear()
{
	osCoreEnterCritical();

	SEMARequest(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

	UTCCalAlarmSet(0,0);
	UTCTimerAlarmSet(0,0,0,0,0);
	
	SEMARelease(SEMA_BASE,SEMA_MASTER_SELECT,SEMA_SLAVE_UTC);

	osCoreExitCritical();

}

void rtc_alarm_get(struct rtc_time *rtctime)
{
	UTCCalAlarmGet((uint8_t *)(&rtctime->tm_mon),(uint32_t *)(&rtctime->tm_mday));
	UTCTimerAlarmGet(0,(uint32_t *)(&rtctime->tm_hour),(uint32_t *)(&rtctime->tm_min), (uint32_t *)(&rtctime->tm_sec), (uint32_t *)(&rtctime->tm_msec));
	rtctime->tm_year = 0;
	
}



/**
 * @brief Disable alarm interrupt.
 *
 * This routine disable alarm interrupt.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
int rtc_alarm_disable()
{
	UTCAlarmDisable(UTC_ALARM_ALL);
	return 0;
}

int rtc_alarm_enable()
{
	UTCAlarmEnable(UTC_ALARM_ALL);
	return 0;
}


int rtc_timer_stop()
{
	HWREG(UTC_BASE+UTC_CTRL) = 0x07;
	return 0;
}

int rtc_timer_run()
{
	HWREG(UTC_BASE+UTC_CTRL) = 0;
	return 0;
}





