/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_worklock.h"
#include "at_xy_cmd.h"
#include "diag_format.h"
#include "diag_mem_dump.h"
#include "factory_nv.h"
#include "flash_adapt.h"
#include "flash_vendor.h"
#include "hw_memmap.h"
#include "inter_core_msg.h"
#include "ipcmsg.h"
#include "osal_statistics.h"
#include "oss_nv.h"
#include "rtc_tmr.h"
#include "sys_init.h"
#include "uart.h"
#include "xy_at_api.h"
#include "xy_flash.h"
#include "xy_log_send.h"
#include "xy_net_api.h"
#include "xy_ps_api.h"
#include "xy_rtc_api.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "xy_watchdog.h"
#include "rtc_adapt.h"
#include "softap_nv.h"
#include "ps_netif_api.h"


/* 按照如下规则写flash并读出来对比，直到写满需要测试的数据量，偏移地址是相对于addr讲的
	偏移地址	  写入字节数(单次)
	0		  1
	1		  2
	3(1+2)	  3
	6(3+3)	  4
	.		  .
	.		  .
	.		  .
注意事项：测试的flash范围内不能有版本的有效数据，否则版本将被破坏
*/
int xy_flash_test(unsigned int addr, unsigned int len)
{
	uint32_t i = 0;
	uint8_t *pbuf_wr = NULL;
	uint8_t *pbuf_rd = NULL;
	uint32_t test_size;
	uint32_t test_addr;
	uint32_t test_len;
	uint32_t test_num = 0;
	char *str = NULL;

	if ((addr < FLASH_BASE) || ((addr + len) > (FLASH_BASE + FLASH_LENGTH)))
	{
		return 1;
	}

	test_addr = addr;
	test_len = len;

	pbuf_wr = xy_malloc(test_len);
	pbuf_rd = xy_malloc(test_len);
	for (i = 0; i < test_len; i++)
	{
		pbuf_wr[i] = (uint8_t)xy_rand();
	}
	/* test xy_flash_write and flash_read */
	test_num = 0;
	test_size = 0;
	for (i = 1;; i++)
	{
		if (test_size + i > test_len)
		{
			i = test_len - test_size;
		}
		xy_flash_write(test_addr + test_size, pbuf_wr + test_size, i);
		test_size += i;
		xy_flash_read(test_addr, pbuf_rd, test_size);
		test_num++;
		if (memcmp(pbuf_rd, pbuf_wr, test_size))
		{
			xy_assert(0);
			break;
		}

		if (test_size == test_len)
		{
			str = xy_zalloc(128);
			snprintf(str, 128, "\r\nwrite test succcess, total write size: %ld, total write num: %ld, last write size %ld\r\n", test_size, test_num, i);
			send_urc_to_ext(str);
			xy_free(str);

			break;
		}
	}

	for (i = 0; i < test_len; i++)
	{
		pbuf_wr[i] = (uint8_t)xy_rand();
	}
	/* test xy_flash_write_no_erase and flash_read */
	xy_flash_erase(test_addr & (~(EFTL_PAGE_SIZE - 1)), (test_len + (test_addr & (EFTL_PAGE_SIZE - 1)) + EFTL_PAGE_SIZE - 1) & (~(EFTL_PAGE_SIZE - 1)));
	test_num = 0;
	test_size = 0;
	for (i = 1;; i++)
	{
		if (test_size + i > test_len)
		{
			i = test_len - test_size;
		}
		xy_flash_write_no_erase(test_addr + test_size, pbuf_wr + test_size, i);
		test_size += i;
		xy_flash_read(test_addr, pbuf_rd, test_size);
		test_num++;
		if (memcmp(pbuf_rd, pbuf_wr, test_size))
		{
			xy_assert(0);
			break;
		}
		if (test_size == test_len)
		{
			str = xy_zalloc(128);
			snprintf(str, 128, "\r\nwrite_no_erase test succcess, total write size: %ld, total write num: %ld, last write size %ld\r\n", test_size, test_num, i);
			send_urc_to_ext(str);
			xy_free(str);

			break;
		}
	}

	xy_free(pbuf_wr);
	xy_free(pbuf_rd);
	
	/* 测试整个sector的读写 */
	pbuf_wr = xy_malloc(EFTL_PAGE_SIZE);
	pbuf_rd = xy_malloc(EFTL_PAGE_SIZE);
	for (i = 0; i < EFTL_PAGE_SIZE; i++)
	{
		pbuf_wr[i] = (uint8_t)xy_rand();
	}
	xy_flash_write(test_addr & (~(EFTL_PAGE_SIZE - 1)), pbuf_wr, EFTL_PAGE_SIZE);
	xy_flash_read(test_addr & (~(EFTL_PAGE_SIZE - 1)), pbuf_rd, EFTL_PAGE_SIZE);
	if (memcmp(pbuf_rd, pbuf_wr, EFTL_PAGE_SIZE))
	{
		xy_assert(0);
	}
	for (i = 0; i < EFTL_PAGE_SIZE; i++)
	{
		pbuf_wr[i] = (uint8_t)xy_rand();
	}
	xy_flash_erase(test_addr & (~(EFTL_PAGE_SIZE - 1)), EFTL_PAGE_SIZE);
	xy_flash_write_no_erase(test_addr & (~(EFTL_PAGE_SIZE - 1)), pbuf_wr, EFTL_PAGE_SIZE);
	xy_flash_read(test_addr & (~(EFTL_PAGE_SIZE - 1)), pbuf_rd, EFTL_PAGE_SIZE);
	if (memcmp(pbuf_rd, pbuf_wr, EFTL_PAGE_SIZE))
	{
		xy_assert(0);
	}
	xy_free(pbuf_wr);
	xy_free(pbuf_rd);
	
	return 0;
}


//begin of rtc demo
#define USER_RTC_OFFSET (40 + osKernelGetTickCount() % 60)

osThreadId_t g_rtc_demo_Handle = NULL;
osSemaphoreId_t rtc_demo_sem = NULL;

/**
 * @brief周期性RTC超时回调接口
 */
static void rtc_timeout_cb(void *para)
{
	(void)para;

	if (rtc_demo_sem == NULL)
	{
		rtc_demo_sem = osSemaphoreNew(0xFFFF, 0, NULL);
	}
	xy_printf("rtc demo callback\n");

	osSemaphoreRelease(rtc_demo_sem);
}

static void day_rtc_callback(void *para)
{
	(void)para;

	xy_rtc_timer_delete(RTC_TIMER_USER2);
	xy_printf("day task timeout, delete day rtc\n");
}

static void week_rtc_callback(void *para)
{
	(void)para;
	xy_rtc_timer_delete(RTC_TIMER_USER3);
	xy_printf("week task timeout, delete week rtc\n");
}

/**
 * @brief rtc 测试用例
 */
static void xy_rtc_test()
{
	static int api_id = 0;
	static int dw_task = 0;
	struct xy_wall_clock usertime = {0};
	struct xy_wall_clock curtime = {0};
	if (dw_task == 1)
	{
		xy_printf("day task will done in %d second, week task will be done in %d second\n",
				  xy_rtc_next_offset_by_ID(RTC_TIMER_USER2), xy_rtc_next_offset_by_ID(RTC_TIMER_USER3));
		api_id = 0;
	}
	xy_printf("user work start:%2d\n", api_id);

	switch (api_id)
	{
	case 0:
		xy_printf("local rtc sec:%lu, net rtc sec: %lu\r\n", xy_rtc_get_sec(0), xy_rtc_get_sec(1));
		xy_printf("%lu seconds passed since last deepsleep\r\n", xy_get_last_work_offset());
		if (xy_rtc_get_time(&curtime) == XY_OK)
			xy_printf("current time:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\n",
					  curtime.tm_year, curtime.tm_mon, curtime.tm_mday, curtime.tm_hour, curtime.tm_min, curtime.tm_sec);
		break;

	case 1:
		usertime.tm_year = 2020;
		usertime.tm_mon = 10;
		usertime.tm_mday = 1;
		usertime.tm_hour = 12;
		usertime.tm_min = 0;
		usertime.tm_sec = 0;
		if (xy_rtc_set_time(&usertime, 0) == XY_OK)
			xy_printf("user set time:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\n",
					  usertime.tm_year, usertime.tm_mon, usertime.tm_mday, usertime.tm_hour, usertime.tm_min, usertime.tm_sec);
		if (xy_rtc_get_time(&curtime) == XY_OK)
			xy_printf("current time:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\n",
					  curtime.tm_year, curtime.tm_mon, curtime.tm_mday, curtime.tm_hour, curtime.tm_min, curtime.tm_sec);
		break;

	case 2:
		if (xy_get_ntp_time("ntp1.aliyun.com", 5, &usertime) == XY_OK)
			xy_printf("user set time:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\n",
					  usertime.tm_year, usertime.tm_mon, usertime.tm_mday, usertime.tm_hour, usertime.tm_min, usertime.tm_sec);
		else
			xy_printf("update NTP time fail\n");
		break;

	case 3:
		if (xy_set_RTC_by_NTP("ntp1.aliyun.com", 5, 8 * 60 * 60) != XY_OK)
			xy_printf("update NTP time fail\n");

		break;

	case 4:
		if (xy_rtc_get_UT_offset(&usertime, 0 - 8 * 60 * 60) == XY_OK)
			xy_printf("user set time:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\n",
					  usertime.tm_year, usertime.tm_mon, usertime.tm_mday, usertime.tm_hour, usertime.tm_min, usertime.tm_sec);
		break;

	case 5:
		xy_printf("%lu seconds passed since power on\r\n", xy_get_working_time());
		break;

	case 6:
		if (xy_rtc_set_by_day(RTC_TIMER_USER2, day_rtc_callback, NULL, 16, 60, 30, 0) == XY_OK)
			xy_printf("set day task rtc:%d\n", xy_rtc_next_offset_by_ID(RTC_TIMER_USER2));
		break;

	case 7:
		if (xy_rtc_set_by_week(RTC_TIMER_USER3, week_rtc_callback, NULL, 3, 16, 60, 35, 0) == XY_OK)
			xy_printf("set week task rtc:%d\n", xy_rtc_next_offset_by_ID(RTC_TIMER_USER3));
		break;

	default:
		break;
	}

	api_id++;
	if (api_id > 7)
	{
		dw_task = 1;
	}

	osDelay(2000);
}

static void rtc_demo_task(void *args)
{
	int ret;

	(void)args;

	xy_assert(g_softap_fac_nv->deepsleep_enable == 1 && g_softap_fac_nv->work_mode == 0);

	while (1)
	{
		//wait user RTC timeout,maybe happen after PS RTC wakeup
		if (rtc_demo_sem != NULL)
		{
			ret = osSemaphoreAcquire(rtc_demo_sem, osWaitForever);

			//user RTC timeout
			if (ret == osOK)
			{
				xy_printf("user_task_demo RTC timeout\n");

				xy_work_lock(0);

				xy_rtc_test();

				xy_printf("set next user rtc:%d\n", USER_RTC_OFFSET);

				//for to reduce BSS stress,and improve communication success rate,user task must set rand UTC,see  xy_rtc_set_by_day
				xy_rtc_timer_create(RTC_TIMER_USER1, USER_RTC_OFFSET, rtc_timeout_cb, NULL);

				xy_printf("user task end,goto deep sleep\n");

				xy_work_unlock(0);
			}
			else
				xy_assert(0);
		}
		//first POWENON,only set next user RTC,not use NB PS
		else
		{
			//wakeup by other cause,such as PS RTC/PIN
			if (xy_rtc_next_offset_by_ID(RTC_TIMER_USER1))
			{
				xy_printf("RTC have setted\n");
				rtc_demo_sem = osSemaphoreNew(0xFFFF, 0, NULL);
				continue;
			}

			xy_printf("first set user RTC:%d\n", USER_RTC_OFFSET);

			//not need load DSP core
			xy_work_lock(0);
			xy_rtc_timer_create(RTC_TIMER_USER1, USER_RTC_OFFSET, rtc_timeout_cb, NULL);
			rtc_demo_sem = osSemaphoreNew(0xFFFF, 0, NULL);

			xy_work_unlock(0);
		}
	}
}

/**
 * @brief 用户任务初始化函数，在user_task_init中添加
 * @attention   
 */
void user_task_rtc_demo_init()
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "rtc_demo_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 0x800;
	g_rtc_demo_Handle = osThreadNew((osThreadFunc_t)(rtc_demo_task), NULL, &thread_attr);
}
//end of rtc demo


//AT+TEST=
int at_TEST_req(char *at_buf, char **prsp_cmd)
{
	char cmd[10] = {0};
	char *str_tmp = NULL;
	char sub_cmd[10] = {0};
	uint32_t str_len;
	int ret;
	uint32_t param1 = 0;
	uint32_t param2 = 0;

	if (at_parse_param("%s,", at_buf, cmd) != AT_OK)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	//AT+TEST=?
	if (strstr(cmd, "?"))
	{
		*prsp_cmd = xy_malloc(64);
		str_len = 0;
		str_len += sprintf(*prsp_cmd + str_len, "\r\nWDT+\r\n");
		str_len += sprintf(*prsp_cmd + str_len, "\r\nFLASH+\r\n");
		str_len += sprintf(*prsp_cmd + str_len, "\r\nSEC+\r\n");
		str_len += sprintf(*prsp_cmd + str_len, "\r\nOK\r\n");
		return AT_END;
	}
	else if (at_strstr2(cmd, "SEC"))
	{
		int type;
		if (at_parse_param(",%d", at_buf, &type) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		*prsp_cmd = xy_zalloc(60);
		unsigned int current_sec = xy_rtc_get_sec(type);

		sprintf(*prsp_cmd, "\r\n+SEC:%u\r\n\r\nOK\r\n", current_sec);
		return AT_END;
	}
	else if (at_strstr2(cmd, "WDT"))
	{
		if (at_parse_param(",%10s", at_buf, sub_cmd) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (at_strstr2(sub_cmd, "ARM"))
		{
			//wait until wdt reset
			taskENTER_CRITICAL();
			while (1)
				;
		}
		else if (at_strstr2(sub_cmd, "DSP"))
		{
			return AT_FORWARD;
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
	}
	else if (at_strstr2(cmd, "RTC"))
	{
		user_task_rtc_demo_init();
		return AT_END;
	}
	else if (at_strstr2(cmd, "IPCHK"))
	{
        char *ip = xy_zalloc(100);
        uint32_t type = 0;
        if (at_parse_param(",%d,%s", at_buf, &type, ip) != AT_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            xy_free(ip);
            return AT_END;
        }

        if (type == 0)
        {
            if (xy_ipaddr_check(ip, IPV4_TYPE) != XY_OK)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            }
        }
        else if (type == 1)
        {
            if (xy_ipaddr_check(ip, IPV6_TYPE) != XY_OK)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            }
        }
        xy_free(ip);
        return AT_END;
	}
	else if (at_strstr2(cmd, "CCLK"))
	{
		//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
		if(g_softap_var_nv->cclk_netl_sec == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
		}

		*prsp_cmd = xy_zalloc(100);

		int zone_sec = 0;
		struct rtc_time rtctime = {0};
		char zone[4] = {0};
		char positive_zone;

		if(g_softap_var_nv->g_zone >= 0)
		{
			positive_zone = g_softap_var_nv->g_zone;
			if(g_softap_var_nv->g_zone >= 10)
				sprintf(zone,"+%2d",positive_zone);
			else
				sprintf(zone,"+%1d",positive_zone);
		}
		else
		{
			positive_zone = 0 - g_softap_var_nv->g_zone;
			if(g_softap_var_nv->g_zone <= -10)
				sprintf(zone,"-%2d",positive_zone);
			else
				sprintf(zone,"-%1d",positive_zone);
			
		}	
		if(g_softap_var_nv->g_zone != 0)
		{
			zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;
			get_universal_timer(&rtctime, zone_sec);
		}
		else
		{
			get_universal_timer(&rtctime, 0);
		}
		struct rtc_time rtc_time;
		rtc_timer_read(&rtc_time);
		
		int tm_year_diff = rtctime.tm_year-2000;
		if(g_softap_var_nv->g_zone == 0)
		{
			if(tm_year_diff >= 0)
				sprintf(*prsp_cmd,"\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+0\r\n+UTC:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\nOK\r\n",tm_year_diff,rtctime.tm_mon,rtctime.tm_mday,  \
					rtctime.tm_hour,rtctime.tm_min,rtctime.tm_sec,rtc_time.tm_year,rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
			else
				sprintf(*prsp_cmd,"\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+0\r\n+UTC:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\nOK\r\n",100+tm_year_diff,rtctime.tm_mon,rtctime.tm_mday,  \
					rtctime.tm_hour,rtctime.tm_min,rtctime.tm_sec,rtc_time.tm_year,rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
		}
		else
		{
			if(tm_year_diff >= 0)
				sprintf(*prsp_cmd,"\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s\r\n+UTC:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\nOK\r\n",tm_year_diff,rtctime.tm_mon,rtctime.tm_mday,  \
					rtctime.tm_hour,rtctime.tm_min,rtctime.tm_sec,zone,rtc_time.tm_year,rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
			else
				sprintf(*prsp_cmd,"\r\n+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s\r\n+UTC:%04lu/%02lu/%02lu,%02lu:%02lu:%02lu\r\nOK\r\n",100+tm_year_diff,rtctime.tm_mon,rtctime.tm_mday,  \
					rtctime.tm_hour,rtctime.tm_min,rtctime.tm_sec,zone,rtc_time.tm_year,rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
		}
		return AT_END;
	}
	else if (at_strstr2(cmd, "PSAPI"))
	{
		if (at_parse_param(",%12s", at_buf, sub_cmd) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "CFUN0"))
		{
			if(xy_cfun_excute(0) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "CFUN1"))
		{
			if(xy_cfun_excute(1) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETCFUN"))
		{
			*prsp_cmd = xy_zalloc(32);
			int cfun = -1;
			if(xy_cfun_read(&cfun) == XY_OK)
				sprintf(*prsp_cmd, "\r\n+CFUN:%d\r\n\r\nOK\r\n", cfun);
			else *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETCGACT"))
		{
			*prsp_cmd = xy_zalloc(32);
			int cgact = -1;
			if(xy_get_CGACT(&cgact) == XY_OK)
				sprintf(*prsp_cmd, "\r\n+CGACT:%d\r\n\r\nOK\r\n", cgact);
			else *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETCEREG"))
		{
			*prsp_cmd = xy_zalloc(32);
			int cereg = -1;
			if(xy_cereg_read(&cereg) == XY_OK)
				sprintf(*prsp_cmd, "\r\n+CEREG:%d\r\n\r\nOK\r\n", cereg);
			else *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETSN"))
		{
			*prsp_cmd = xy_zalloc(100);
			char *sn=xy_zalloc(SN_LEN + 1);
			if(xy_get_SN(sn, SN_LEN) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+SN:%s\r\n\r\nOK\r\n", sn);
			if(sn != NULL)
				xy_free(sn);
		}
		else if (at_strstr2(sub_cmd, "GETRSSI"))
		{
			*prsp_cmd = xy_zalloc(32);
			int rssi = 0;
			if(xy_get_RSSI(&rssi) == XY_OK)
				sprintf(*prsp_cmd, "\r\n+RSSI:%u\r\n\r\nOK\r\n", rssi);
			else *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETIMEI"))
		{
			*prsp_cmd = xy_zalloc(64);
			char *imei=xy_zalloc(IMEI_LEN);
			if(xy_get_IMEI(imei, IMEI_LEN) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+IMEI:%s\r\n\r\nOK\r\n", imei);
			if(imei != NULL)
				xy_free(imei);
		}
		else if (at_strstr2(sub_cmd, "GETIMSI"))
		{
			*prsp_cmd = xy_zalloc(64);
			char *imsi=xy_zalloc(IMSI_LEN);
			if(xy_get_IMSI(imsi, IMSI_LEN) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+IMSI:%s\r\n\r\nOK\r\n", imsi);
			if(imsi != NULL)
				xy_free(imsi);
		}
		else if (at_strstr2(sub_cmd, "GETCELLID"))
		{
			*prsp_cmd = xy_zalloc(32);
			int cellid = -1;
			if(xy_get_CELLID(&cellid) == XY_OK)
				sprintf(*prsp_cmd, "\r\n+CELLID:%d\r\n\r\nOK\r\n", cellid);
			else *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETCCID"))
		{
			*prsp_cmd = xy_zalloc(64);
			char *ccid=xy_zalloc(UCCID_LEN);
			if(xy_get_NCCID(ccid, UCCID_LEN) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+CCID:%s\r\n\r\nOK\r\n", ccid);
			if(ccid != NULL)
				xy_free(ccid);
		}
		else if (at_strstr2(sub_cmd, "GETAPN"))
		{
			*prsp_cmd = xy_zalloc(150);
			char *apn = xy_zalloc(APN_LEN);
			if(xy_get_PDP_APN(apn, APN_LEN, -1) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+APN:%s\r\n\r\nOK\r\n", apn);
			if(apn != NULL)
				xy_free(apn);
		}
		else if (at_strstr2(sub_cmd, "GETCID"))
		{
			*prsp_cmd = xy_zalloc(40);
			unsigned char cid = xy_get_working_cid();
			sprintf(*prsp_cmd, "\r\n+WORKING CID:%u\r\n\r\nOK\r\n", cid);
		}
		else if (at_strstr2(sub_cmd, "GETIP4S"))
		{
			*prsp_cmd = xy_zalloc(48);
			char *ip4s = xy_zalloc(XY_IP4ADDR_STRLEN);
			if(xy_getIP4Addr(ip4s, XY_IP4ADDR_STRLEN) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\nIP4ADDR:%s\r\n\r\nOK\r\n", ip4s);
			if(ip4s != NULL)
				xy_free(ip4s);
		}
		else if (at_strstr2(sub_cmd, "GETIP4D"))
		{
			*prsp_cmd = xy_zalloc(32);
			unsigned int ip4addr = 0;
			if(xy_get_ip4addr(&ip4addr) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
			{
				sprintf(*prsp_cmd, "\r\n+IP4ADDR:%u\r\n\r\nOK\r\n", ip4addr);
			}
		}
		else if (at_strstr2(sub_cmd, "GETTAU"))
		{
			*prsp_cmd = xy_zalloc(32);
			int tau = -1;
			if(xy_get_T_TAU(&tau) == XY_OK)
				sprintf(*prsp_cmd, "\r\n+TAU:%d\r\n\r\nOK\r\n", tau);
			else *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETACT"))
		{
			*prsp_cmd = xy_zalloc(32);
			int t3324 = -1;
			if(xy_get_T_ACT(&t3324) == XY_OK)
				sprintf(*prsp_cmd, "\r\n+T3324:%d\r\n\r\nOK\r\n", t3324);
			else *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "RAI"))
		{
			if(xy_send_rai() != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		else if (at_strstr2(sub_cmd, "GETMODVER"))
		{
			*prsp_cmd = xy_zalloc(64);
			char *modver = xy_zalloc(20);
			if(xy_get_MODULVER(modver, 20) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+MODULEVER:%s\r\n\r\nOK\r\n", modver);
			if(modver != NULL)
				xy_free(modver);
		}
		else if (at_strstr2(sub_cmd, "GETVEREXT"))
		{
			*prsp_cmd = xy_zalloc(64);
			char *verext = xy_zalloc(28);
			if(xy_get_VERSIONEXT(verext, 28) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+VEREXT:%s\r\n\r\nOK\r\n", verext);
			if(verext != NULL)
				xy_free(verext);
		}
		else if (at_strstr2(sub_cmd, "GETHARDVER"))
		{
			*prsp_cmd = xy_zalloc(64);
			char *hardver = xy_zalloc(20);
			if(xy_get_HARDVER(hardver, 20) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+HARDVER:%s\r\n\r\nOK\r\n", hardver);
			if(hardver != NULL)
				xy_free(hardver);
		}
		else if (at_strstr2(sub_cmd, "GETUICCTYPE"))
		{
			*prsp_cmd = xy_zalloc(48);
			int uicc_type = -1;
			if(xy_get_UICC_TYPE(&uicc_type) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\n+UICCTYPE:%d\r\n\r\nOK\r\n", uicc_type);
		}
		else if (at_strstr2(sub_cmd, "GETEDRX"))
		{
			*prsp_cmd = xy_zalloc(64);
			float eDRX_value;
			float ptw_value;
			if(xy_get_eDRX_value(&eDRX_value, &ptw_value) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
				sprintf(*prsp_cmd, "\r\neDRX:%.2f ,PTW:%.2f\r\n\r\nOK\r\n", eDRX_value, ptw_value);
		}
		else if (at_strstr2(sub_cmd, "GETCELLINFO"))
		{
			ril_serving_cell_info_t *rcv_servingcell_info = xy_zalloc(sizeof(ril_serving_cell_info_t));

			*prsp_cmd = (char*)xy_zalloc(360);

			if(xy_get_servingcell_info(rcv_servingcell_info) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
			{
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n Signalpower:%d", rcv_servingcell_info->Signalpower);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n Totalpower:%d", rcv_servingcell_info->Totalpower);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n TXpower:%d", rcv_servingcell_info->TXpower);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n CellID:%d", rcv_servingcell_info->CellID);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n ECL:%d", rcv_servingcell_info->ECL);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n SNR:%d", rcv_servingcell_info->SNR);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n EARFCN:%d", rcv_servingcell_info->EARFCN);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n PCI:%d", rcv_servingcell_info->PCI);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n RSRQ:%d", rcv_servingcell_info->RSRQ);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n TAC:%s", rcv_servingcell_info->tac);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n SBAND:%d", rcv_servingcell_info->sband);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
			}
			xy_free(rcv_servingcell_info);

		}
		else if (at_strstr2(sub_cmd, "GETNEBCELL"))
		{
			ril_neighbor_cell_info_t *rcv_neighborcell_info = xy_zalloc(sizeof(ril_neighbor_cell_info_t));

			*prsp_cmd = (char*)xy_zalloc(360);

			if(xy_get_neighborcell_info(rcv_neighborcell_info) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
			{
				while(rcv_neighborcell_info->nc_num > 0)
				{

					sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n Neighbor cell %d: %d, %d, %d", rcv_neighborcell_info->nc_num,
							rcv_neighborcell_info->neighbor_cell_info[rcv_neighborcell_info->nc_num-1].nc_earfcn,
							rcv_neighborcell_info->neighbor_cell_info[rcv_neighborcell_info->nc_num-1].nc_pci,
							rcv_neighborcell_info->neighbor_cell_info[rcv_neighborcell_info->nc_num-1].nc_rsrp);
					rcv_neighborcell_info->nc_num--;
				}
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
			}
			xy_free(rcv_neighborcell_info);
			
		}
		else if (at_strstr2(sub_cmd, "GETPHY"))
		{
			ril_phy_info_t *phy_info = xy_zalloc(sizeof(ril_phy_info_t));

			*prsp_cmd = (char*)xy_zalloc(400);

			if(xy_get_phy_info(phy_info) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
			{
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n RLC_UL_BLER:%d", phy_info->RLC_UL_BLER);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n RLC_DL_BLER:%d", phy_info->RLC_DL_BLER);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_UL_BLER:%d", phy_info->MAC_UL_BLER);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_DL_BLER:%d", phy_info->MAC_DL_BLER);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_UL_total_bytes:%d", phy_info->MAC_UL_total_bytes);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_DL_total_bytes:%d", phy_info->MAC_DL_total_bytes);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_UL_total_HARQ_TX:%d", phy_info->MAC_UL_total_HARQ_TX);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_DL_total_HARQ_TX:%d", phy_info->MAC_DL_total_HARQ_TX);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_UL_HARQ_re_TX:%d", phy_info->MAC_UL_HARQ_re_TX);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_DL_HARQ_re_TX:%d", phy_info->MAC_DL_HARQ_re_TX);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n RLC_UL_tput:%d", phy_info->RLC_UL_tput);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n RLC_DL_tput:%d", phy_info->RLC_DL_tput);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_UL_tput:%d", phy_info->MAC_UL_tput);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n MAC_DL_tput:%d", phy_info->MAC_DL_tput);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
			}
			xy_free(phy_info);
		}


		else if (at_strstr2(sub_cmd, "GETRADIO"))
		{
			softap_printf(USER_LOG, WARN_LOG, "GET ALL RAIDO INFO\r\n");
			ril_radio_info_t *radio_info = (ril_radio_info_t *)xy_zalloc(sizeof(ril_radio_info_t));
			if (xy_get_radio_info(radio_info) != XY_OK)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			else
			{
				//打印输出，部分测试
				*prsp_cmd = (char*)xy_zalloc(400);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "[NUEST-RADIO]RLC_UL_BLER:%d,RLC_DL_BLER:%d\r\n", radio_info->phy_info.RLC_UL_BLER, radio_info->phy_info.RLC_DL_BLER);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "[NUEST-RADIO]MAC_UL_tput:%d,MAC_DL_tput:%d\r\n", radio_info->phy_info.MAC_UL_tput, radio_info->phy_info.MAC_DL_tput);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "[NUEST-RADIO]EARFCN:%d,PCI:%d\r\n", radio_info->serving_cell_info.EARFCN, radio_info->serving_cell_info.PCI);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "[NUEST-RADIO]RSRQ:%d,sband:%d\r\n", radio_info->serving_cell_info.RSRQ, radio_info->serving_cell_info.sband);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "[NUEST-RADIO]neighbor count:%d\r\n", radio_info->neighbor_cell_info.nc_num);
				sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
			}

			xy_free(radio_info);

		}
		else
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		
		return AT_END;
	}
	
	else if (!strcmp(cmd, "FLASH"))
	{
		if (at_parse_param(",%10s", at_buf, sub_cmd) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (!strcmp(sub_cmd, "ARM"))
		{
			if (at_parse_param(",,%10s", at_buf, sub_cmd) != AT_OK)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				return AT_END;
			}

			if (!strcmp(sub_cmd, "TESTALL"))
			{
				if (at_parse_param(",,,%d,%d", at_buf, &param1, &param2) != AT_OK)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					return AT_END;
				}

				str_tmp = xy_malloc(64);
				snprintf(str_tmp, 64, "\r\ntest_addr 0x%lx, test_len 0x%lx\r\n", param1, param2);
				send_urc_to_ext(str_tmp);
				xy_free(str_tmp);

				ret = xy_flash_test(param1, param2);
				if (1 == ret)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					return AT_END;
				}
				else
				{
					*prsp_cmd = xy_malloc(16);
					sprintf(*prsp_cmd, "\r\nOK\r\n");
				}
			}
#if FLASH_PROTECT
			else if (!strcmp(sub_cmd, "PROTECT"))
			{
				if (at_parse_param(",,,%d,%d", at_buf, &param1, &param2) != AT_OK)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					return AT_END;
				}

				xy_flash_protect((FLASH_PROTECT_MODE)param1, (uint8_t)param2);

				*prsp_cmd = xy_malloc(16);
				sprintf(*prsp_cmd, "\r\nOK\r\n");
			}
#endif
			else
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				return AT_END;
			}
		}
		else if (!strcmp(sub_cmd, "DSP"))
		{
			return AT_FORWARD;
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
	}
#if DEMO_TEST
	else if (at_strstr2(cmd, "SOCKETDEMO"))
	{
		extern void xy_socket_demo_init();
		xy_socket_demo_init();
		return AT_END;
	}
#endif

	else
	{
		return AT_FORWARD;
	}

	return AT_END;
}

int at_FLASH_req(char *at_buf, char **prsp_cmd)
{
	unsigned char cmd[16] = {0};
	unsigned int destAddr = 0;
	unsigned int srcAddr = 0;
	unsigned int size = 0;
	unsigned int i = 0;

	void *p[] = {cmd, &srcAddr, &destAddr, &size};
	if (at_parse_param_2("%15s,%d,%d,%d", at_buf, p) != AT_OK)
		goto ERR;

	//AT+FLASH=TEST_WE,0,destAddr     //test write erase flash
	if (!strcmp((const char *)(cmd), "TEST_WE"))
	{
		unsigned int j = 0;
		//unsigned char *temp = xy_zalloc(EFTL_PAGE_SIZE+64);
		//unsigned char *m3data = (unsigned char *)((unsigned int)(temp+63) & 0xffffffc0);
		unsigned char *m3data = xy_zalloc(EFTL_PAGE_SIZE);

		for (j = 0; j < EFTL_PAGE_SIZE; j++)
		{
			m3data[j] = j;
		}

		xy_flash_write(destAddr, (void *)m3data, EFTL_PAGE_SIZE);

		//unsigned char *temp2 = xy_zalloc(EFTL_PAGE_SIZE+64);
		//unsigned char *readbackdata = (unsigned char *)((unsigned int)(temp2+63) & 0xffffffc0);
		unsigned char *readbackdata = xy_zalloc(EFTL_PAGE_SIZE);

		DMAChannelControlSet((unsigned long)FLASH_DMA_CHANNEL, DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelTransfer_withMutex((unsigned long)readbackdata, destAddr, EFTL_PAGE_SIZE, (unsigned long)FLASH_DMA_CHANNEL);

		xy_assert(memcmp(m3data, readbackdata, EFTL_PAGE_SIZE) == 0);
		xy_free(readbackdata);

		xy_free(m3data);

		*prsp_cmd = xy_zalloc(30);
		sprintf(*prsp_cmd, "TEST_WE success!!\r\n\r\nOK\r\n");
	}
	//AT+FLASH=WE,srcAddr,destAddr,size    //flash write with erase
	else if (!strcmp((const char *)(cmd), "WE"))
	{
		xy_flash_write(destAddr, (void *)srcAddr, size);
	}
	//AT+FLASH=WNE,srcAddr,destAddr,size   //flash write with no erase
	else if (!strcmp((const char *)(cmd), "WNE"))
	{
		xy_flash_write_no_erase(destAddr, (void *)(srcAddr), (int)(size));
	}
	//AT+FLASH=WSTREAM,destAddr,data...    //flash write with data stream
	else if (!strcmp((const char *)(cmd), "WSTREAM"))
	{
		unsigned char *m3data = xy_zalloc(BURSTSIZE);

		unsigned int data[4];
		void *p5[] = {cmd, &destAddr, &data[0], &data[1], &data[2], &data[3]};

		if (at_parse_param_2("%10s,%d,%d,%d,%d,%d", at_buf, p5) != AT_OK)
			goto ERR;
		for (i = 0; i < 4; i++)
		{
			m3data[i * 4 + 0] = (data[i] & 0xff);
			m3data[i * 4 + 1] = (data[i] & 0xff00) >> 8;
			m3data[i * 4 + 2] = (data[i] & 0xff0000) >> 16;
			m3data[i * 4 + 3] = (data[i] & 0xff000000) >> 24;
		}

		xy_flash_write_no_erase(destAddr, m3data, BURSTSIZE);
		xy_free(m3data);
	}
	//AT+FLASH=TWSTREAM,eraseflag,destAddr,data...    //flash write with data stream
	else if (!strcmp((const char *)(cmd), "TWSTREAM"))
	{
		unsigned char *m3data = xy_zalloc(BURSTSIZE);

		unsigned int data[4];
		void *p6[] = {cmd, &destAddr, &data[0], &data[1], &data[2], &data[3]};

		if (at_parse_param_2("%10s,%d,%d,%d,%d,%d,%d", at_buf, p6) != AT_OK)
			goto ERR;
		for (i = 0; i < 4; i++)
		{
			m3data[i * 4 + 0] = (data[i] & 0xff);
			m3data[i * 4 + 1] = (data[i] & 0xff00) >> 8;
			m3data[i * 4 + 2] = (data[i] & 0xff0000) >> 16;
			m3data[i * 4 + 3] = (data[i] & 0xff000000) >> 24;
		}

		xy_flash_write(destAddr, (void *)m3data, 16);

		xy_assert(memcmp(m3data, (void *)destAddr, 16) == 0);
		xy_free(m3data);
	}
	//AT+FLASH=ERASE,addr,size
	else if (!strcmp((const char *)(cmd), "ERASE"))
	{
		void *p2[] = {cmd, &destAddr, &size};
		if (at_parse_param_2("%10s,%d,%d", at_buf, p2) != AT_OK)
			goto ERR;

		xy_flash_erase(destAddr, size);
	}
	//AT+FLASH=READ,srcAddr,size
	else if (!strcmp((const char *)(cmd), "READ"))
	{
		void *p2[] = {cmd, &srcAddr, &size};
		if (at_parse_param_2("%10s,%d,%d", at_buf, p2) != AT_OK)
			goto ERR;

		//unsigned int *srcdata = (unsigned int *)srcAddr;
		unsigned char *destdata = (unsigned char *)xy_zalloc(16);
		flash_read(srcAddr, (unsigned char *)destdata, size);

		*prsp_cmd = xy_zalloc(100);
		if (size > 16)
			size = 16;

		sprintf(*prsp_cmd, "\r\n");
		for (i = 0; i < size; i++)
		{
			unsigned char printdata[3] = {0};
#define GETCHAR(d) (((d) < 10) ? ((d) + '0') : ((d)-10 + 'A'))
			printdata[0] = GETCHAR(*(destdata + i) >> 4);
			printdata[1] = GETCHAR(*(destdata + i) & 0x0F);

			sprintf(*prsp_cmd + strlen(*prsp_cmd), "%s,", printdata);
		}

		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n\r\nOK\r\n");
		xy_free(destdata);
	}
	else if (!strcmp((const char *)(cmd), "TESTALL"))
	{
		(void)xy_flash_test(FOTA_BACKUP_BASE + 5, EFTL_PAGE_SIZE + 100);
		*prsp_cmd = xy_zalloc(16);
		sprintf(*prsp_cmd, "\r\nOK\r\n");
	}
	else
	{
		return AT_FORWARD;
	}

	return AT_END;

ERR:
	*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}
