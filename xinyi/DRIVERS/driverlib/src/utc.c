#include "utc.h"

#if !USE_ROM_UTC
void UTCTimerStop(void)
{
    HWREG(UTC_BASE + UTC_CTRL) |= UTC_CTRL_TIMER_STOP;
}

void UTCTimerRun(void)
{
    HWREG(UTC_BASE + UTC_CTRL) &= (~UTC_CTRL_TIMER_STOP);
}

void UTCCalStop(void)
{
    HWREG(UTC_BASE + UTC_CTRL) |= UTC_CTRL_CAL_STOP;
}

void UTCCalRun(void)
{
    HWREG(UTC_BASE + UTC_CTRL) &= (~UTC_CTRL_CAL_STOP);
}

void UTCDivStop(void)
{
    HWREG(UTC_BASE + UTC_CTRL) |= UTC_CTRL_DIVCNT;
}

void UTCDivEn(void)
{
    HWREG(UTC_BASE + UTC_CTRL) &= (~UTC_CTRL_DIVCNT);
}


void UTCHourModeSet(unsigned long ulMode)
{
    HWREG(UTC_BASE + UTC_HOUR_MODE) = ulMode;
}

unsigned long UTCHourModeGet(void)
{
    return (HWREG(UTC_BASE + UTC_HOUR_MODE));
}

void UTCTimerSet(unsigned long ulAMPM, unsigned long ulHour, unsigned long ulMin, unsigned long ulSec, unsigned long ulMinSec)
{
    unsigned char ucHourHigh;
    unsigned char ucHourLow;
    unsigned char ucMinHigh;
    unsigned char ucMinLow;
    unsigned char ucSecHigh;
    unsigned char ucSecLow;
    unsigned char ucMinSecHigh;
    unsigned char ucMinSecLow;
    
//    ulAMPM &= 0x01;
//    ulHour &= 0x17;
//    ulMin  &= 0x3B;
//    ulSec  &= 0x3B;
//    ulMinSec &= 0x63;
    
    ucHourHigh = ulHour / 10;
    ucHourLow  = ulHour % 10;
    
    ucMinHigh  = ulMin / 10;
    ucMinLow   = ulMin % 10;
    
    ucSecHigh  = ulSec / 10;
    ucSecLow   = ulSec % 10;
    
    ucMinSecHigh = ulMinSec / 10;
    ucMinSecLow  = ulMinSec % 10;
    
    HWREG(UTC_BASE + UTC_TIMER) = (HWREG(UTC_BASE + UTC_TIMER) & 0x80000000) | (ulAMPM << UTC_TIMER_PM_Pos) | 
                                  (ucHourHigh   << UTC_TIMER_MRT_Pos) | (ucHourLow   << UTC_TIMER_MRU_Pos) | 
                                  (ucMinHigh    << UTC_TIMER_MT_Pos)  | (ucMinLow    << UTC_TIMER_MU_Pos) | 
                                  (ucSecHigh    << UTC_TIMER_ST_Pos)  | (ucSecLow    << UTC_TIMER_SU_Pos) | 
                                  (ucMinSecHigh << UTC_TIMER_HT_Pos)  | (ucMinSecLow << UTC_TIMER_HU_Pos);
}

unsigned long UTCTimerChangeGet(void)
{
    return ((HWREG(UTC_BASE + UTC_TIMER) & UTC_TIMER_CH_Msk) >> UTC_TIMER_CH_Pos);
}
#endif

void UTCTimerGet(unsigned char *ulAMPM, unsigned char *ulHour, unsigned char *ulMin, unsigned char *ulSec, unsigned char *ulMinSec,unsigned long ulRegData)
{
    unsigned long ulReg;
    if(ulRegData)
		ulReg = ulRegData;
	else
    	ulReg = HWREG(UTC_BASE + UTC_TIMER);
    
    *ulAMPM = (ulReg & UTC_TIMER_PM_Msk) >> UTC_TIMER_PM_Pos;
    
    *ulHour = ((ulReg & UTC_TIMER_MRT_Msk) >> UTC_TIMER_MRT_Pos) * 10 + ((ulReg & UTC_TIMER_MRU_Msk) >> UTC_TIMER_MRU_Pos);
    
    *ulMin = ((ulReg & UTC_TIMER_MT_Msk) >> UTC_TIMER_MT_Pos) * 10 + ((ulReg & UTC_TIMER_MU_Msk) >> UTC_TIMER_MU_Pos);
    
    *ulSec = ((ulReg & UTC_TIMER_ST_Msk) >> UTC_TIMER_ST_Pos) * 10 + ((ulReg & UTC_TIMER_SU_Msk) >> UTC_TIMER_SU_Pos);
    
    *ulMinSec = ((ulReg & UTC_TIMER_HT_Msk) >> UTC_TIMER_HT_Pos) * 10 + ((ulReg & UTC_TIMER_HU_Msk) >> UTC_TIMER_HU_Pos);
}

#if !USE_ROM_UTC
unsigned long UTCTimerGetBy10ms(void)
{
    unsigned long ulReg;
    unsigned long ulMinSec;
//    unsigned char ucAMPM;
    unsigned char ucHour;
    unsigned char ucMin;
    unsigned char ucSec;
    unsigned char ucMinSec;
    
    ulReg = HWREG(UTC_BASE + UTC_TIMER);
    
//    ucAMPM = (ulReg & UTC_TIMER_PM_Msk) >> UTC_TIMER_PM_Pos;
    
    ucHour = ((ulReg & UTC_TIMER_MRT_Msk) >> UTC_TIMER_MRT_Pos) * 10 + ((ulReg & UTC_TIMER_MRU_Msk) >> UTC_TIMER_MRU_Pos);
    
    ucMin = ((ulReg & UTC_TIMER_MT_Msk) >> UTC_TIMER_MT_Pos) * 10 + ((ulReg & UTC_TIMER_MU_Msk) >> UTC_TIMER_MU_Pos);
    
    ucSec = ((ulReg & UTC_TIMER_ST_Msk) >> UTC_TIMER_ST_Pos) * 10 + ((ulReg & UTC_TIMER_SU_Msk) >> UTC_TIMER_SU_Pos);
    
    ucMinSec = ((ulReg & UTC_TIMER_HT_Msk) >> UTC_TIMER_HT_Pos) * 10 + ((ulReg & UTC_TIMER_HU_Msk) >> UTC_TIMER_HU_Pos);
    
    ulMinSec  = ucHour * 60 * 60 * 100;
    ulMinSec += ucMin * 60 * 100;
    ulMinSec += ucSec * 100;
    ulMinSec += ucMinSec;
    
    return ulMinSec;
}
#endif

void UTCTimerAlarmSet(unsigned long ulAMPM, unsigned long ulHour, unsigned long ulMin, unsigned long ulSec, unsigned long ulMS)
{
    unsigned char ucHourHigh;
    unsigned char ucHourLow;
    unsigned char ucMinHigh;
    unsigned char ucMinLow;
    unsigned char ucSecHigh;
    unsigned char ucSecLow;
    unsigned char ucMinSecHigh;
    unsigned char ucMinSecLow;
#if 0
    unsigned long ulClkCnt;
#endif
    ucHourHigh = ulHour / 10;
    ucHourLow  = ulHour % 10;
    
    ucMinHigh  = ulMin / 10;
    ucMinLow   = ulMin % 10;
    
    ucSecHigh  = ulSec / 10;
    ucSecLow   = ulSec % 10;
    
    ucMinSecHigh = (ulMS/10) / 10;
    ucMinSecLow  = (ulMS/10) % 10;
    
    HWREG(UTC_BASE + UTC_ALARM_TIMER) = (ulAMPM       << UTC_ALARM_TIMER_PM_Pos)  | 
                                        (ucHourHigh   << UTC_ALARM_TIMER_MRT_Pos) | (ucHourLow   << UTC_ALARM_TIMER_MRU_Pos) | 
                                        (ucMinHigh    << UTC_ALARM_TIMER_MT_Pos)  | (ucMinLow    << UTC_ALARM_TIMER_MU_Pos) | 
                                        (ucSecHigh    << UTC_ALARM_TIMER_ST_Pos)  | (ucSecLow    << UTC_ALARM_TIMER_SU_Pos) | 
                                        (ucMinSecHigh << UTC_ALARM_TIMER_HT_Pos)  | (ucMinSecLow << UTC_ALARM_TIMER_HU_Pos);
	
#if 0
	ulClkCnt = (ulMS%10)*32;
	if(ulClkCnt < 160)
		HWREG(UTC_BASE + UTCALARMCLKCNT_CFG) = ((1<<9)|(ulClkCnt + 256));
	else
		HWREG(UTC_BASE + UTCALARMCLKCNT_CFG) = ((1<<9)|(ulClkCnt - 160));
#endif
}

void UTCTimerAlarmSetAccurate(unsigned long ulAMPM, unsigned long ulHour, unsigned long ulMin, unsigned long ulSec, unsigned long ulMS)
{
    unsigned char ucHourHigh;
    unsigned char ucHourLow;
    unsigned char ucMinHigh;
    unsigned char ucMinLow;
    unsigned char ucSecHigh;
    unsigned char ucSecLow;
    unsigned char ucMinSecHigh;
    unsigned char ucMinSecLow;
#if 1
    unsigned long ulClkCnt;
#endif
    ucHourHigh = ulHour / 10;
    ucHourLow  = ulHour % 10;
    
    ucMinHigh  = ulMin / 10;
    ucMinLow   = ulMin % 10;
    
    ucSecHigh  = ulSec / 10;
    ucSecLow   = ulSec % 10;
    
    ucMinSecHigh = (ulMS/10) / 10;
    ucMinSecLow  = (ulMS/10) % 10;
    
    HWREG(UTC_BASE + UTC_ALARM_TIMER) = (ulAMPM       << UTC_ALARM_TIMER_PM_Pos)  | 
                                        (ucHourHigh   << UTC_ALARM_TIMER_MRT_Pos) | (ucHourLow   << UTC_ALARM_TIMER_MRU_Pos) | 
                                        (ucMinHigh    << UTC_ALARM_TIMER_MT_Pos)  | (ucMinLow    << UTC_ALARM_TIMER_MU_Pos) | 
                                        (ucSecHigh    << UTC_ALARM_TIMER_ST_Pos)  | (ucSecLow    << UTC_ALARM_TIMER_SU_Pos) | 
                                        (ucMinSecHigh << UTC_ALARM_TIMER_HT_Pos)  | (ucMinSecLow << UTC_ALARM_TIMER_HU_Pos);
	
#if 1
	ulClkCnt = (ulMS%10)*32;
	if(ulClkCnt < 160)
		HWREG(UTC_BASE + UTCALARMCLKCNT_CFG) = ((1<<9)|(ulClkCnt + 256));
	else
		HWREG(UTC_BASE + UTCALARMCLKCNT_CFG) = ((1<<9)|(ulClkCnt - 160));
#endif
}

void UTCTimerAlarmGet(unsigned long *ulAMPM, unsigned long *ulHour, unsigned long *ulMin, unsigned long *ulSec, unsigned long *ulMS)
{
	unsigned long ulReg;
	unsigned long ulClkCnt;

	(void) ulAMPM;
	ulReg = HWREG(UTC_BASE + UTC_ALARM_TIMER);
    
    //*ulAMPM = (ulReg & UTC_ALARM_TIMER_PM_Msk) >> UTC_ALARM_TIMER_PM_Pos);
    *ulHour = ((ulReg & UTC_ALARM_TIMER_MRT_Msk) >> UTC_ALARM_TIMER_MRT_Pos) * 10 + ((ulReg & UTC_ALARM_TIMER_MRU_Msk) >> UTC_ALARM_TIMER_MRU_Pos);
    *ulMin = ((ulReg & UTC_ALARM_TIMER_MT_Msk) >> UTC_ALARM_TIMER_MT_Pos) * 10 + ((ulReg & UTC_ALARM_TIMER_MU_Msk) >> UTC_ALARM_TIMER_MU_Pos);
    *ulSec = ((ulReg & UTC_ALARM_TIMER_ST_Msk) >> UTC_ALARM_TIMER_ST_Pos) * 10 + ((ulReg & UTC_ALARM_TIMER_SU_Msk) >> UTC_ALARM_TIMER_SU_Pos);	
#if 1
	if(HWREG(UTC_BASE + UTCALARMCLKCNT_CFG)&(1<<9))
    {
		ulClkCnt = HWREG(UTC_BASE + UTC_CLK_CNT);
		ulClkCnt &= 0x1FF;
		if(ulClkCnt & 0x100)
			*ulMS = ((ulClkCnt - 256)/32) + 10*(((ulReg & UTC_ALARM_TIMER_HT_Msk) >> UTC_ALARM_TIMER_HT_Pos) * 10 + ((ulReg & UTC_ALARM_TIMER_HU_Msk) >> UTC_ALARM_TIMER_HU_Pos));
		else
			*ulMS = ((ulClkCnt + 160)/32) + 10*(((ulReg & UTC_ALARM_TIMER_HT_Msk) >> UTC_ALARM_TIMER_HT_Pos) * 10 + ((ulReg & UTC_ALARM_TIMER_HU_Msk) >> UTC_ALARM_TIMER_HU_Pos));

	}
	else
#endif
		*ulMS = 10*(((ulReg & UTC_ALARM_TIMER_HT_Msk) >> UTC_ALARM_TIMER_HT_Pos) * 10 + ((ulReg & UTC_ALARM_TIMER_HU_Msk) >> UTC_ALARM_TIMER_HU_Pos));
    
}

#if !USE_ROM_UTC
void UTCTimerAlarmSetBy10ms(unsigned long ulMinSec)
{
    unsigned long ulcurMinSec;
    unsigned char ucAMPM;
    unsigned char ucHour;
    unsigned char ucMin;
    unsigned char ucSec;
    unsigned char ucMinSec;
    
    unsigned char ucHourHigh;
    unsigned char ucHourLow;
    unsigned char ucMinHigh;
    unsigned char ucMinLow;
    unsigned char ucSecHigh;
    unsigned char ucSecLow;
    unsigned char ucMinSecHigh;
    unsigned char ucMinSecLow;
    
    ulcurMinSec = UTCTimerGetBy10ms();
    
    ulMinSec += ulcurMinSec;
    
    ucAMPM = 0;
    
    ucHour     =   ulMinSec / 360000;
    ucMin      =  (ulMinSec % 360000) / 6000;
    ucSec      = ((ulMinSec % 360000) % 6000) / 100;
    ucMinSec   = ((ulMinSec % 360000) % 6000) % 100;
    
    ucHourHigh = ucHour / 10;
    ucHourLow  = ucHour % 10;
    
    ucMinHigh  = ucMin / 10;
    ucMinLow   = ucMin % 10;
    
    ucSecHigh  = ucSec / 10;
    ucSecLow   = ucSec % 10;
    
    ucMinSecHigh = ucMinSec / 10;
    ucMinSecLow  = ucMinSec % 10;
    
    HWREG(UTC_BASE + UTC_ALARM_TIMER) = (ucAMPM       << UTC_ALARM_TIMER_PM_Pos)  | 
                                        (ucHourHigh   << UTC_ALARM_TIMER_MRT_Pos) | (ucHourLow   << UTC_ALARM_TIMER_MRU_Pos) | 
                                        (ucMinHigh    << UTC_ALARM_TIMER_MT_Pos)  | (ucMinLow    << UTC_ALARM_TIMER_MU_Pos) | 
                                        (ucSecHigh    << UTC_ALARM_TIMER_ST_Pos)  | (ucSecLow    << UTC_ALARM_TIMER_SU_Pos) | 
                                        (ucMinSecHigh << UTC_ALARM_TIMER_HT_Pos)  | (ucMinSecLow << UTC_ALARM_TIMER_HU_Pos);
}
#endif

void UTCTimerAlarmSetSecond(unsigned long ulSec)
{
    unsigned char ucAMPM;
    unsigned char ucHour;
    unsigned char ucMin;
    unsigned char ucSec;
    unsigned char ucMinSec;
    
    (void) ulSec;

    UTCTimerGet(&ucAMPM, &ucHour, &ucMin, &ucSec, &ucMinSec,0);
    
    
}

/*****************************************************************************
 Function    : UTCCalSet
 Description : Set UTC calendar.
 Input       : ulCentury    --- Century
               ulYear       --- Year
               ulMonth      --- Month
               ulDate       --- Date
               ulDay        --- Day of the week
 Output      : None
 Return      : None
 *****************************************************************************/
void UTCCalSet(unsigned long ulCentury, unsigned long ulYear, unsigned long ulMonth, unsigned long ulDate, unsigned long ulDay)
{
    unsigned char ucCenturyHigh;
    unsigned char ucCenturyLow;
    unsigned char ucYearHigh;
    unsigned char ucYearLow;
    unsigned char ucMonthHigh;
    unsigned char ucMonthLow;
    unsigned char ucDateHigh;
    unsigned char ucDateLow;
    
    ucCenturyHigh = ulCentury / 10;
    ucCenturyLow  = ulCentury % 10;
    
    ucYearHigh = ulYear / 10;
    ucYearLow  = ulYear % 10;
    
    ucMonthHigh = ulMonth / 10;
    ucMonthLow  = ulMonth % 10;
    
    ucDateHigh = ulDate / 10;
    ucDateLow  = ulDate % 10;

    HWREG(UTC_BASE + UTC_CAL) = (HWREG(UTC_BASE + UTC_CAL) & 0x80000000) | 
                                (ucCenturyHigh << UTC_CAL_CT_Pos) | (ucCenturyLow << UTC_CAL_CU_Pos) | 
                                (ucYearHigh    << UTC_CAL_YT_Pos) | (ucYearLow    << UTC_CAL_YU_Pos) | 
                                (ucMonthHigh   << UTC_CAL_MT_Pos) | (ucMonthLow   << UTC_CAL_MU_Pos) | 
                                (ucDateHigh    << UTC_CAL_DT_Pos) | (ucDateLow    << UTC_CAL_DU_Pos) |
                                (ulDay         << UTC_CAL_DAY_Pos);
	//HWREG(UTC_BASE + UTC_STATUS) |= UTC_STATUS_VALID_CAL_Msk;
}

#if !USE_ROM_UTC
unsigned long UTCCalChangeGet(void)
{
    return ((HWREG(UTC_BASE + UTC_CAL) & UTC_CAL_CH_Msk) >> UTC_CAL_CH_Pos);
}
#endif

void UTCCalGet(unsigned char *ulCentury, unsigned char *ulYear, unsigned char *ulMonth, unsigned char *ulDate, unsigned char *ulDay,unsigned long ulRegData)
{
    unsigned long ulReg;

	if(ulRegData)
    	ulReg = ulRegData;
	else
    	ulReg = HWREG(UTC_BASE + UTC_CAL);
    
    *ulCentury = ((ulReg & UTC_CAL_CT_Msk) >> UTC_CAL_CT_Pos) * 10 + ((ulReg & UTC_CAL_CU_Msk) >> UTC_CAL_CU_Pos);
    
    *ulYear = ((ulReg & UTC_CAL_YT_Msk) >> UTC_CAL_YT_Pos) * 10 + ((ulReg & UTC_CAL_YU_Msk) >> UTC_CAL_YU_Pos);
    
    *ulMonth = ((ulReg & UTC_CAL_MT_Msk) >> UTC_CAL_MT_Pos) * 10 + ((ulReg & UTC_CAL_MU_Msk) >> UTC_CAL_MU_Pos);
    
    *ulDate = ((ulReg & UTC_CAL_DT_Msk) >> UTC_CAL_DT_Pos) * 10 + ((ulReg & UTC_CAL_DU_Msk) >> UTC_CAL_DU_Pos);
    
    *ulDay = (ulReg & UTC_CAL_DAY_Msk) >> UTC_CAL_DAY_Pos;
}

#if !USE_ROM_UTC
void UTCCalAlarmSet(unsigned long ulMonth, unsigned long ulDate)
{
    unsigned char ucMonthHigh;
    unsigned char ucMonthLow;
    unsigned char ucDateHigh;
    unsigned char ucDateLow;
    
    ucMonthHigh = ulMonth / 10;
    ucMonthLow  = ulMonth % 10;
    
    ucDateHigh = ulDate / 10;
    ucDateLow  = ulDate % 10;
    
    HWREG(UTC_BASE + UTC_ALARM_CAL) = (ucMonthHigh << UTC_ALARM_CAL_MT_Pos) | (ucMonthLow << UTC_ALARM_CAL_MU_Pos) | 
                                      (ucDateHigh  << UTC_ALARM_CAL_DT_Pos) | (ucDateLow  << UTC_ALARM_CAL_DU_Pos);
}
#endif

void UTCCalAlarmGet(unsigned char *ulMonth, unsigned long *ulDate)
{
    unsigned long ulReg;

	ulReg = HWREG(UTC_BASE + UTC_ALARM_CAL);
    
    *ulMonth = ((ulReg & UTC_ALARM_CAL_MT_Msk) >> UTC_ALARM_CAL_MT_Pos) * 10 + ((ulReg & UTC_ALARM_CAL_MU_Msk) >> UTC_ALARM_CAL_MU_Pos);
    
    *ulDate = ((ulReg & UTC_ALARM_CAL_DT_Msk) >> UTC_ALARM_CAL_DT_Pos) * 10 + ((ulReg & UTC_ALARM_CAL_DU_Msk) >> UTC_ALARM_CAL_DU_Pos);
    
}

#if !USE_ROM_UTC
void UTCAlarmEnable(unsigned long ulAlarmFlags)
{
    HWREG(UTC_BASE + UTC_ALARM_EN) |= ulAlarmFlags;
}

void UTCAlarmDisable(unsigned long ulAlarmFlags)
{
    HWREG(UTC_BASE + UTC_ALARM_EN) &= (~ulAlarmFlags);
}

void UTCIntEnable(unsigned long ulIntFlags)
{
    HWREG(UTC_BASE + UTC_INT_EN) |= ulIntFlags;
}

void UTCIntDisable(unsigned long ulIntFlags)
{
    HWREG(UTC_BASE + UTC_INT_DIS) |= ulIntFlags;
}

void UTCIntMaskSet(unsigned long ulIntMask)
{
    HWREG(UTC_BASE + UTC_INT_MSK) |= ulIntMask;
}

unsigned long UTCIntMaskGet(void)
{
    return (HWREG(UTC_BASE + UTC_INT_MSK));
}

void UTCIntStatusSet(unsigned long ulIntFlags)
{
    HWREG(UTC_BASE + UTC_INT_STAT) |= ulIntFlags;
}

unsigned long UTCIntStatusGet(void)
{
    return (HWREG(UTC_BASE + UTC_INT_STAT));
}

unsigned long UTCValidStatusGet(void)
{
    return (HWREG(UTC_BASE + UTC_STATUS));
}

void UTCKeepRun(unsigned long ulKeepUTC)
{
    HWREG(UTC_BASE + UTC_KEEP_UTC) = ulKeepUTC;
}

void UTCClkCntSet(unsigned long ulClkCnt)
{
	if(ulClkCnt < 160)
		HWREG(UTC_BASE + UTC_CLK_CNT) = ulClkCnt + 256;
	else
		HWREG(UTC_BASE + UTC_CLK_CNT) = ulClkCnt - 160;
}
#endif


unsigned long UTCClkCntConvert(unsigned long ulRegData)
{
	unsigned long reg_value;
	
	reg_value = ulRegData & 0x1FF;

	if(reg_value & 0x100)
		return (reg_value - 256);
	else
		return (reg_value + 160);

}


#if !USE_ROM_UTC
void UTCIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    //
    // Register the interrupt handler.
    //
    IntRegister(INT_UTC, g_pRAMVectors, pfnHandler);

    //
    // Enable the UART interrupt.
    //
    IntEnable(INT_UTC);
}

void UTCIntUnregister(unsigned long *g_pRAMVectors)
{
	//
	// Disable the interrupt.
	//
	IntDisable(INT_UTC);

	//
	// Unregister the interrupt handler.
	//
	IntUnregister(INT_UTC, g_pRAMVectors);
}
#endif

void Switch_to_Outer_UTC_XTAL(void)
{
	volatile unsigned long i;

    if(!(HWREGB(0xA0058019) & 0x20))
    {

        while(!(HWREGB(0xA0058018) & 0x04));

        HWREGB(0xA0058019) |= 0x20;

        i = 5000;

        while(i--);

        HWREGB(0xA0058019) |= 0x04;
    }
}

