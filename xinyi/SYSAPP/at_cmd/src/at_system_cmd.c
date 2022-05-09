/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_system_cmd.h"
#include "xy_at_api.h"
#include "at_global_def.h"
#include "csp.h"
#include "hw_types.h"
#include "oss_nv.h"
#include "rtc_tmr.h"
#include "rtc_adapt.h"
#include "xy_utils.h"
#include "softap_nv.h"
#include "xinyi_hardware.h"
#include "xy_rtc_api.h"
#include "xy_system.h"
#include "at_com.h"
#include "sys_init.h"
#include "xy_flash.h"
#include "at_uart.h"
#include "dsp_process.h"
#include "xy_watchdog.h"
#include "low_power.h"
/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
int g_NITZ_mode = 0;
#if VER_GXX
int g_CTZU_mode = 0;
#endif

/*******************************************************************************
 *                       Local variable definitions                          *
 ******************************************************************************/
osTimerId_t off_standby_timer = NULL;

/*******************************************************************************
 *                       Local function implementations	                   *
 ******************************************************************************/
//19/03/30,09:28:56+32---->xy_wall_clock+zone
int convert_wall_time(char *data, char *time, struct xy_wall_clock *wall_time, int *zone_sec)
{
	char *tag;
	char *next_tag;
	uint8_t month_table[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	tag = data;
	next_tag = strchr(tag, '/');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';

	int temp_tag = (int)strtol(tag,NULL,10);
#if VER_QUCTL260
	if (temp_tag >= 2000)
		wall_time->tm_year = temp_tag;
	else
		return XY_ERR;
#else
	if (temp_tag >= 0 && temp_tag < 70)
		wall_time->tm_year = 2000 + temp_tag;
	else if (temp_tag >= 70 && temp_tag <= 99)
		wall_time->tm_year = 1900 + temp_tag;
	else
		return XY_ERR;
#endif

	//闰年
	if ((0 == wall_time->tm_year % 4 && 0 != wall_time->tm_year % 100) || (0 == wall_time->tm_year % 400))
		month_table[2] = 29;
	
	tag = next_tag + 1;
	next_tag = strchr(tag, '/');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';
	wall_time->tm_mon = (int)strtol(tag,NULL,10);

	if (wall_time->tm_mon > 12 || wall_time->tm_mon <= 0)
		return XY_ERR;

	tag = next_tag + 1;
	wall_time->tm_mday = (int)strtol(tag,NULL,10);

	if (wall_time->tm_mday <= 0 || wall_time->tm_mday > month_table[wall_time->tm_mon])
		return XY_ERR;

	tag = time;
	next_tag = strchr(tag, ':');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';
	wall_time->tm_hour = (int)strtol(tag,NULL,10);

	if (wall_time->tm_hour >= 24)
		return XY_ERR;

	tag = next_tag + 1;
	next_tag = strchr(tag, ':');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';
	wall_time->tm_min = (int)strtol(tag,NULL,10);
	if (wall_time->tm_min >= 60)
		return XY_ERR;

	tag = next_tag + 1;
	//+zone
	if (((next_tag = strchr(tag, '+')) != NULL) || ((next_tag = strchr(tag, '-')) != NULL))
	{
		char zone[4] = {0};

		memcpy(zone, next_tag, 3);

		if (next_tag[0] == '+')
		{
#if VER_QUECTEL
            if ((int)strtol(next_tag + 1, NULL, 10) > 96)
                return XY_ERR;
            *zone_sec = ((int)strtol(next_tag + 1, NULL, 10)) * 15 * 30;
#else
            if ((int)strtol(next_tag + 1, NULL, 10) > 48)
                return XY_ERR;
            *zone_sec = ((int)strtol(next_tag + 1, NULL, 10)) * 15 * 60;
#endif
        }
        else if (next_tag[0] == '-')
        {
#if VER_QUECTEL
            if ((int)strtol(next_tag + 1, NULL, 10) > 96)
                return XY_ERR;
            *zone_sec = 0 - ((int)strtol(next_tag + 1, NULL, 10)) * 15 * 30;
#else
            if ((int)strtol(next_tag + 1, NULL, 10) > 48)
                return XY_ERR;
            *zone_sec = 0 - ((int)strtol(next_tag + 1, NULL, 10)) * 15 * 60;
#endif
        }

        *next_tag = '\0';
        wall_time->tm_sec = (int)strtol(tag, NULL, 10);
        if (wall_time->tm_sec >= 60)
            return XY_ERR;

        g_softap_var_nv->g_zone = (int)strtol(zone, NULL, 10);
    }
	//not +zone
	else
	{
		return XY_ERR;
	}

	return XY_OK;
}

//2019/03/29,21:12:56---->xy_wall_clock
int convert_wall_time2(char *data, char *time, struct xy_wall_clock *wall_time)
{
	char *tag;
	char *next_tag;

	tag = data;
	next_tag = strchr(tag, '/');
	if (next_tag == NULL)
		return -1;
	*next_tag = '\0';
	wall_time->tm_year = atoi(tag);

	tag = next_tag + 1;
	next_tag = strchr(tag, '/');
	if (next_tag == NULL)
		return -1;
	*next_tag = '\0';
	wall_time->tm_mon = atoi(tag);

	tag = next_tag + 1;
	wall_time->tm_mday = atoi(tag);

	tag = time;
	next_tag = strchr(tag, ':');
	if (next_tag == NULL)
		return -1;
	*next_tag = '\0';
	wall_time->tm_hour = atoi(tag);

	tag = next_tag + 1;
	next_tag = strchr(tag, ':');
	if (next_tag == NULL)
		return -1;
	*next_tag = '\0';
	wall_time->tm_min = atoi(tag);

	tag = next_tag + 1;
	wall_time->tm_sec = atoi(tag);
	return 1;
}

//AT+STANDBY参数超时
void off_standby_timeout_proc(void *arg)
{
	(void) arg;

	xy_standby_open();
	osTimerDelete(off_standby_timer);
	off_standby_timer = NULL;
}

/*******************************************************************************
 *                       Global function implementations	                   *
 ******************************************************************************/
//0，仅保存NV后，正常进入DEEPSLEEP；1/2，保存NV后，锁中断，等待客户断电，解决电容放电问题
int at_CPOF_req(char *at_buf,char **prsp_cmd)
{
	if (g_req_type == AT_CMD_ACTIVE)
	{
		xy_fast_power_off(1);
	}
	else if (g_req_type == AT_CMD_REQ)
	{
		int abnormal_off = 0;
		if (at_parse_param("%d", at_buf, &abnormal_off) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}

		if (abnormal_off == 2)  //高新兴需求，需支持AT+FASTOFF=2，功能和AT+FASTOFF=1一致
			abnormal_off = 1;

		if (abnormal_off != 0 && abnormal_off != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else
		{
			xy_fast_power_off(abnormal_off);
		}
	}
	else
	{
		*prsp_cmd = xy_zalloc(40);
		snprintf(*prsp_cmd, 40, "\r\n+CPOF:(0,1)\r\n\r\nOK\r\n");
	}
	return AT_END;
}

int at_QPOWD_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int powd = 0;
		if (at_parse_param("%d", at_buf, &powd) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}

		if(powd!=0 && powd!=1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if(powd == 0)
		{
			xy_fast_power_off(0);
		}
		else
		{
			xy_work_unlock(LOCK_EXT);
		}
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//0，仅保存NV后，正常进入DEEPSLEEP；1，保存NV后，锁中断，等待客户断电，解决电容放电问题
int at_ABOFF_req(char *at_buf,char **prsp_cmd)
{
	(void) at_buf;

	if(g_req_type == AT_CMD_ACTIVE)
	{
		xy_fast_power_off(1);
	}
	else if(g_req_type == AT_CMD_REQ)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);	
	}
	else
	{
		*prsp_cmd = xy_zalloc(40);
		snprintf(*prsp_cmd, 40, "\r\nOK\r\n");
	}
	return AT_END;
}

//AT+LOADDSP to load DSP dynamicly
int at_LOADDSP_req(char * at_buf, char * * prsp_cmd)
{
	(void) at_buf;
	(void) prsp_cmd;
	if( g_req_type==AT_CMD_ACTIVE)
	{
		if (DSP_IS_NOT_LOADED())
			dynamic_load_up_dsp(0);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	
	return AT_END;
}

//save 3GPP NV and user NVor not
int at_RB_req(char *at_buf, char **prsp_cmd)
{
	int xy_nv_save=1;
	int user_nv_save=1;
	if(g_req_type==AT_CMD_REQ || g_req_type==AT_CMD_ACTIVE)
	{
		if (at_parse_param("%d,%d", at_buf, &xy_nv_save, &user_nv_save) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		send_urc_to_ext("\r\nREBOOTING\r\n");
		reboot_nv();

		HWREG(PS_START_SYNC) = 0;
		
		soft_reset_by_wdt(RB_BY_NRB);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int at_STANDBY_req(char *at_buf, char **prsp_cmd)
{
	int enable = 0;
	int standby_keep = 0;
	osTimerAttr_t timer_attr = {0};
	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%d", at_buf, &enable) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (enable == 0)
		{
			if (at_parse_param(",%d", at_buf, &standby_keep) != AT_OK)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				return AT_END;
			}

			xy_standby_close();
			if (standby_keep != 0)
			{
				if(off_standby_timer == NULL)
				{
					timer_attr.name = "off_standby";
					off_standby_timer = osTimerNew((osTimerFunc_t)(off_standby_timeout_proc), osTimerOnce, NULL, &timer_attr);
				}
				xy_assert(off_standby_timer != NULL);
				osTimerStart(off_standby_timer, standby_keep * 1000);
			}
		}
		else
		{
			xy_standby_open();
			if(off_standby_timer != NULL)
			{
				osTimerStop(off_standby_timer);
				osTimerDelete(off_standby_timer);
				off_standby_timer = NULL;
			}
		}
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

extern void NVIC_CLREN_CLRPEN_ALL();

int at_RESET_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;

	if (g_req_type == AT_CMD_ACTIVE)
	{
		send_urc_to_ext("\r\nRESETING\r\n");

		osCoreEnterCritical();
		//close DSP clock,and DSP will not run
		HWREG(0xA0110018) = 0x00;
		g_have_suspend_dsp = 1;

		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		HWREG(0xE000ED04) = ((1 << 27) | (1 << 25));
		NVIC_CLREN_CLRPEN_ALL();

		erase_flash_by_abnormal(0);

		HWREG(PS_START_SYNC) = 0;

		soft_reset_by_wdt(RB_BY_RESET);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NITZ=<mode>
int at_NITZ_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int temp = 0;

		//移远没有深睡唤醒，设置后上报+NPSMR:1/+NPSMR:0任然有效，直到断电上电或软重启会恢复成默认值；
		//对标移远时新的设置默认保存NV，可确保深睡唤醒上报+NPSMR:1/+NPSMR:0时设置的值仍然有效，但断电上电或软重启后无法恢复为默认值。
#if VER_QUECTEL
		int save_NITZ = 1;
#else
		int save_NITZ = 0;
#endif

		if (at_parse_param("%d,%d", at_buf, &temp, &save_NITZ) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (temp != 0 && temp != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (save_NITZ != 0 && save_NITZ != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		g_NITZ_mode = temp;

		if (save_NITZ == 1)
		{
			g_softap_fac_nv->g_NITZ = g_NITZ_mode;
			SAVE_FAC_PARAM(g_NITZ);
		}
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(20);
		sprintf(*prsp_cmd, "\r\n+NITZ:%d\r\n\r\nOK\r\n", g_NITZ_mode);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+NITZ:(0,1)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

#if VER_GXX
//AT+CTZU=<mode>
int at_CTZU_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int temp = 0;

		if (at_parse_param("%d", at_buf, &temp) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (temp != 0 && temp != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		g_CTZU_mode = temp;
	}
    else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(20);
		sprintf(*prsp_cmd, "\r\n+CTZU:%d\r\n\r\nOK\r\n", g_CTZU_mode);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+CTZU:(0-1)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}
#endif

//AT+CCLK=<yy/MM/dd,hh:mm:ss>[<±zz>]    as 19/03/30,09:28:56+32
//AT+CCLK?   +CCLK:[<yy/MM/dd,hh:mm:ss>[<±zz>]]   19/03/30,09:28:56+32
int at_CCLK_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char date[16] = {0};
		char time[16] = {0};
		int zone_sec = 0;
		struct xy_wall_clock wall_time = {0};

#if VER_QUECTEL || VER_QUCTL260
        if (at_parse_param_in_quotation("%s,%s", at_buf, date, time) == AT_OK)
#else
        if (at_parse_param("%s,%s", at_buf, date, time) == AT_OK)
#endif
        {
			if (date[0] == 0 || time[0] == 0)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				return AT_END;
			}
#if VER_GXX
            if (g_CTZU_mode == 1)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
				return AT_END;
			}

#elif (!VER_QUCTL260)
			if (g_NITZ_mode == 1)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
				return AT_END;
			}
#endif
			if (convert_wall_time(date, time, &wall_time, &zone_sec) == XY_OK)
				xy_rtc_set_time(&wall_time, zone_sec);
			else
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
        if (g_softap_var_nv->cclk_netl_sec == 0)
        {
#if VER_QUCTL260
        	*prsp_cmd = xy_zalloc(60);
        	struct rtc_time local_rtctime = {0};
        	rtc_timer_read(&local_rtctime);
			sprintf(*prsp_cmd, "\r\n+CCLK:\"%04lu/%02lu/%02lu,%02lu:%02lu:%02lu+00\"\r\n\r\nOK\r\n", local_rtctime.tm_year, local_rtctime.tm_mon, local_rtctime.tm_mday,
					local_rtctime.tm_hour, local_rtctime.tm_min, local_rtctime.tm_sec);
#elif !VER_QUECTEL
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#endif
			return AT_END;
		}

		*prsp_cmd = xy_zalloc(60);
		int zone_sec = 0;
		struct rtc_time rtctime = {0};
		char zone[4] = {0};
		char positive_zone;

        if (g_softap_var_nv->g_zone >= 0)
        {
            positive_zone = g_softap_var_nv->g_zone;
            if (g_softap_var_nv->g_zone >= 10)
            {
                sprintf(zone, "+%2d", positive_zone);
            }
            else
            {
#if VER_QUECTEL || VER_QUCTL260
                sprintf(zone, "+%02d", positive_zone);
#else
                sprintf(zone, "+%1d", positive_zone);
#endif //VER_QUECTEL
            }
        }
        else
        {
            positive_zone = 0 - g_softap_var_nv->g_zone;
            if (g_softap_var_nv->g_zone <= -10)
            {
                sprintf(zone, "-%2d", positive_zone);
            }
            else
            {
#if VER_QUECTEL || VER_QUCTL260
                sprintf(zone, "-%02d", positive_zone);
#else            
                sprintf(zone, "-%1d", positive_zone);
#endif //VER_QUECTEL      
            }
        }
        if (g_softap_var_nv->g_zone != 0)
        {
#if VER_QUECTEL
            if (g_NITZ_mode == 0)
                zone_sec = (int)g_softap_var_nv->g_zone * 15 * 30;
            else
#endif //VER_QUECTEL
            zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;

            get_universal_timer(&rtctime, zone_sec);			
		}
		else
		{
			get_universal_timer(&rtctime, 0);	
		}

#if VER_QUCTL260
        if (g_softap_var_nv->g_zone == 0)
        {

			sprintf(*prsp_cmd, "\r\n+CCLK:\"%04lu/%02lu/%02lu,%02lu:%02lu:%02lu+00\"\r\n\r\nOK\r\n", rtctime.tm_year, rtctime.tm_mon, rtctime.tm_mday,
					rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec);
        }
		else
		{
			sprintf(*prsp_cmd, "\r\n+CCLK:\"%04lu/%02lu/%02lu,%02lu:%02lu:%02lu%s\"\r\n\r\nOK\r\n", rtctime.tm_year, rtctime.tm_mon, rtctime.tm_mday,
					rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec, zone);
		}
#else
        int tm_year_diff = rtctime.tm_year - 2000;
        if (g_softap_var_nv->g_zone == 0)
        {
            if (tm_year_diff >= 0)
                sprintf(*prsp_cmd, "\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+00\r\n\r\nOK\r\n", tm_year_diff, rtctime.tm_mon, rtctime.tm_mday,
                        rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec);
            else
                sprintf(*prsp_cmd, "\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+00\r\n\r\nOK\r\n", 100 + tm_year_diff, rtctime.tm_mon, rtctime.tm_mday,
                        rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec);
        }
        else
        {
            if (tm_year_diff >= 0)
                sprintf(*prsp_cmd, "\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s\r\n\r\nOK\r\n", tm_year_diff, rtctime.tm_mon, rtctime.tm_mday,
                        rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec, zone);
            else
                sprintf(*prsp_cmd, "\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s\r\n\r\nOK\r\n", 100 + tm_year_diff, rtctime.tm_mon, rtctime.tm_mday,
                        rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec, zone);
        }
#endif
	}
    else if (g_req_type == AT_CMD_ACTIVE)
    {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
	return AT_END;
}

//32K<--->32.768K
int at_OFFTIME_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	    int offset;


	    if (at_parse_param("%d", at_buf, &offset) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		
		if(g_need_offtime==0)
		{
			softap_printf(USER_LOG, WARN_LOG, "invalid AT+OFFTIME");
			//*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}
		//user force power off,because UERS's timer timeout,need do attach,delete timer info
		if(offset==(int)(0XFFFFFFFF)||offset==0)
		{
			memset((void *)g_softap_var_nv,0,sizeof(softap_var_nv_t));
			memset((void *)BACKUP_MEM_BASE,0,XY_FTL_AVAILABLE_SIZE);
			
			send_debug_str_to_at_uart("+DBGINFO:var nv invalid!\r\n");
		}
		else
		{
			rtc_timer_reset((uint64_t)g_softap_var_nv->rtc_snap_msec + (uint64_t)offset*1024);
			//restore_RTC_list();
		}
		
		if(g_softap_fac_nv->work_mode == 0)
		{
			set_DSP_work_mode();
		}
		if(g_softap_fac_nv->off_debug == 0)
			print_dsp_dbg();

		//forge  RTC interrupt  wake up
		osSemaphoreRelease(g_rtc_sem);
		g_need_offtime = 0;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}


int at_CPOFF_req(char *at_buf, char **prsp_cmd)
{
	int xy_nv_save = 1;
	int user_nv_save = 1;

	if (g_req_type == AT_CMD_REQ || g_req_type == AT_CMD_ACTIVE)
	{
		if (at_parse_param("%d,%d", at_buf, &xy_nv_save, &user_nv_save) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		send_urc_to_ext("\r\n+CPOFF REQUEST\r\n");

		shut_down_dsp_core();
		
		return AT_END;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int at_QSCLK_req(char *at_buf, char **prsp_cmd)
{
	int8_t sleep_mode;

	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%1d(0-2)", at_buf, &sleep_mode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		g_softap_var_nv->sleep_mode = sleep_mode;

	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd, "\r\n+QSCLK:%1d\r\n\r\nOK\r\n", g_softap_var_nv->sleep_mode);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd, "\r\n+QSCLK:(0-2)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}

int at_QRST_req(char *at_buf, char **prsp_cmd)
{
	uint8_t rst_mode;

	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%1d(1-1)", at_buf, &rst_mode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		send_urc_to_ext("\r\nOK\r\n");
		save_factory_nv();

		HWREG(PS_START_SYNC) = 0;

		soft_reset_by_wdt(RB_BY_NRB);

		return AT_END;
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd, "\r\n+QRST:(1)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int at_QCFG_req(char *at_buf, char **prsp_cmd)
{
	char function[20] = {0};
	char value = -1;

	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%20s,%1d", at_buf, function, &value) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if(strcmp(function, "dsevent") == 0)
		{
			if(value == -1)
			{
				*prsp_cmd = xy_zalloc(48);
				sprintf(*prsp_cmd, "\r\n+QCFG: \"dsevent\",%d\r\n\r\nOK\r\n",g_softap_fac_nv->deepsleep_urc);
			}
			else if(value != 0 && value != 1)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			}
			else if(g_softap_fac_nv->deepsleep_urc != value)
			{
				g_softap_fac_nv->deepsleep_urc = value;
				SAVE_FAC_PARAM(deepsleep_urc);
			}
			return AT_END;
		}
	}
	return AT_FORWARD;
}

