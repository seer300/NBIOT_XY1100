/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_xy_cmd.h"
#include "xy_at_api.h"
#include "at_worklock.h"
#include "flash_vendor.h"
#include "flash_adapt.h"
#include "hw_memmap.h"
#include "ipcmsg.h"
#include "oss_nv.h"
#include "rtc_tmr.h"
#include "uart.h"
#include "xy_flash.h"
#include "osal_statistics.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "xy_rtc_api.h"
#include "at_com.h"
#include "diag_format.h"
#include "sys_init.h"
#include "xy_log_send.h"
#include "factory_nv.h"
#include "diag_mem_dump.h"
#include "inter_core_msg.h"
#include "xy_watchdog.h"
#include "at_global_def.h"
#include "at_uart.h"
#include "cloud_utils.h"
#include "xy_atc_interface.h" //kt

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define COMPARE_MAX(a , b)  (((a) > (b)) ? (a) : (b))

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
extern int at_ReqAndRsp_to_ps(char *req_at, char *info_fmt, int timeout, ...);
uint8_t* getNewBuildInfo(void);

static void get_req_to_dsp(char *at_buf)
{
	int ret = -1;
	char *at_str = xy_zalloc(50);
	snprintf(at_str, 50, "AT+NV=%s\r\n", at_buf);

	if (DSP_IS_NOT_LOADED())
	{
		goto free_out;
	}
	if ((ret = at_ReqAndRsp_to_ps(at_str, NULL, 5)) != 0)
	{
		xy_assert(ret == ATERR_DSP_NOT_RUN);
	}
free_out:
	xy_free(at_str);
}


/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
int at_POWER_test_req(int val)
{
	if (g_req_type == AT_CMD_REQ || g_req_type == AT_CMD_ACTIVE)
	{
		//入库功能测试版本
		if (val == 0)
		{
			g_softap_fac_nv->wfi_enable = 0;
			g_softap_fac_nv->lpm_standby_enable = 0;
			g_softap_fac_nv->deepsleep_enable = 1;
			g_softap_fac_nv->need_start_dm = 1;
			g_softap_fac_nv->ver_type = 1;
			g_softap_fac_nv->keep_retension_mem = 0;
			g_softap_fac_nv->backup_threshold = 0;
			g_softap_fac_nv->off_debug = 1;
			g_softap_fac_nv->down_data = 1;
		}
		//仪表功耗测试版本
		else if (val == 1)
		{
            xy_atc_interface_call("AT+NSET=\"LOWPWR\",1\r\n", NULL, (void*)NULL);
			xy_atc_interface_call("AT+CEDRXS=1\r\n", NULL, (void*)NULL);

			HWREGB(0xA0058019) |= (1 << 4);
			g_softap_fac_nv->aon_uvlo_en = 1;

			g_softap_fac_nv->wfi_enable = 1;
			g_softap_fac_nv->lpm_standby_enable = 1;
			g_softap_fac_nv->deepsleep_enable = 1;
			g_softap_fac_nv->need_start_dm = 0;
			g_softap_fac_nv->ver_type = 1;
			g_softap_fac_nv->keep_retension_mem = 0;
			g_softap_fac_nv->backup_threshold = 0;
			g_softap_fac_nv->off_debug = 1;
			g_softap_fac_nv->close_urc = 1;
		}

		//SDK商用现网功耗测试版本
		else if (val == 2)
		{
			xy_atc_interface_call("AT+CEDRXS=1\r\n", NULL,(void*)NULL);

			g_softap_fac_nv->wfi_enable = 1;
			g_softap_fac_nv->lpm_standby_enable = 1;
			g_softap_fac_nv->deepsleep_enable = 1;
			g_softap_fac_nv->need_start_dm = 0;
			g_softap_fac_nv->ver_type = 2;
			g_softap_fac_nv->keep_retension_mem = 1;
			g_softap_fac_nv->backup_threshold = 60;
			g_softap_fac_nv->off_debug = 1;
			g_softap_fac_nv->close_urc = 1;
		}
		//eDRX	xianwang
		else if (val == 3)
		{
			xy_atc_interface_call("AT+CGDCONT=0,\"IPV4V6\",\"CMNBIOT5\"\r\n", NULL, (void*)NULL);
			xy_atc_interface_call("AT+CEDRXS=1,5,\"0011\",\"0011\"\r\n", NULL, (void*)NULL);
			xy_atc_interface_call("AT+CPSMS=0\r\n", NULL, (void*)NULL);

			g_softap_fac_nv->wfi_enable = 1;
			g_softap_fac_nv->lpm_standby_enable = 1;
			g_softap_fac_nv->deepsleep_enable = 1;
			g_softap_fac_nv->need_start_dm = 0;
			g_softap_fac_nv->ver_type = 2;
			g_softap_fac_nv->keep_retension_mem = 1;
			g_softap_fac_nv->backup_threshold = 0;
			g_softap_fac_nv->off_debug = 1;
		}
		//single core power test
		else if (val == 4)
		{
#if 0
			g_softap_fac_nv->wfi_enable = 1;
			g_softap_fac_nv->lpm_standby_enable = 1;
			g_softap_fac_nv->deepsleep_enable = 1;
			g_softap_fac_nv->need_start_dm = 0;
			g_softap_fac_nv->ver_type = 2;
			g_softap_fac_nv->keep_retension_mem = 1;
			g_softap_fac_nv->backup_threshold = 60;
			//g_softap_fac_nv->off_debug = 1;
			g_softap_fac_nv->work_mode = 1;
			extern void user_rtc_cb(void *para);
			xy_rtc_timer_create(RTC_TIMER_USER_BASE, 30, user_rtc_cb, (void *)30);
#endif
			return AT_END;
		}
		SAVE_SOFTAP_FAC();
		send_urc_to_ext("\r\nREBOOTING\r\n");

		reboot_nv();
		HWREG(PS_START_SYNC) = 0;

		soft_reset_by_wdt(RB_BY_POWERTEST);
	}
	return AT_END;
}


/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/

int simple_set_val(char *param,int val)
{
	if (!strcmp(param, "VERTYPE"))
	{
		g_softap_fac_nv->ver_type = val;
		SAVE_FAC_PARAM(ver_type);
	}
	
	else if(!strcmp(param,"DEEPSLEEPTHRESHOLD"))
	{
		g_softap_fac_nv->deepsleep_threshold = val;
		SAVE_FAC_PARAM(deepsleep_threshold);
	}
	else if(!strcmp(param,"FOTAPERIOD"))
	{
		g_softap_fac_nv->fota_period = val;
		SAVE_FAC_PARAM(fota_period);
	}
	else if(!strcmp(param,"RATIME"))
	{
		g_softap_fac_nv->ra_timeout = (uint16_t)val;
		SAVE_FAC_PARAM(ra_timeout);
	}
	else if (!strcmp(param, "ATSTANDBY"))
	{
		g_softap_fac_nv->at_standby_delay = (uint8_t)val;
		SAVE_FAC_PARAM(at_standby_delay);
	}
	else if(!strcmp(param,"SAVECLOUD"))
	{
		//暂不支持单核模式
	    if(g_softap_fac_nv->work_mode == 1)
	    {
	        if(g_softap_fac_nv->save_cloud)
	        {
	            g_softap_fac_nv->save_cloud = 0;
	            SAVE_FAC_PARAM(save_cloud);
	        }

            if(val == 1)
            {
                send_debug_str_to_at_uart("+DBGINFO:[CONFIG ERROR] Unsupported in single-core\r\n");
            }
	        return 1;
	    }
	    else
	    {
	        g_softap_fac_nv->save_cloud = val;
	        SAVE_FAC_PARAM(save_cloud);
	    }
	}
	else if(!strcmp(param,"CLOUDALIVE"))
	{
		//兼容之前版本，后续该变量将删除
	}
	else if(!strcmp(param,"DMRETRYTIME"))
	{
		g_softap_fac_nv->dm_retry_time = val;
		SAVE_FAC_PARAM(dm_retry_time);
		
	}
	else if(!strcmp(param,"DMREGTIME"))
	{
		g_softap_fac_nv->dm_reg_time = val;
		SAVE_FAC_PARAM(dm_reg_time);
	}
	else if(!strcmp(param,"DMRETRYNUM"))
	{
		g_softap_fac_nv->dm_retry_num = val;
		SAVE_FAC_PARAM(dm_retry_num);
	}
	else if(!strcmp(param,"TAURTC"))
	{
		g_softap_fac_nv->set_tau_rtc = val;
		SAVE_FAC_PARAM(set_tau_rtc);
	}

	else if(!strcmp(param,"TEST"))
	{
		g_softap_fac_nv->test = val;
		SAVE_FAC_PARAM(test);
	}
	else if(!strcmp(param,"XTAL"))
	{
		g_softap_fac_nv->xtal_32k = val;
		g_softap_fac_nv->rc32k_freq = 0;
		SAVE_FAC_PARAM(xtal_32k);
		SAVE_FAC_PARAM(rc32k_freq);
	}
	else if(!strcmp(param,"USERDOG"))
	{
		g_softap_fac_nv->user_dog_time = (unsigned char)val;
		SAVE_FAC_PARAM(user_dog_time);
	}
	
	else if(!strcmp(param,"DOGMODE"))
	{
		g_softap_fac_nv->hard_dog_mode = val;
		SAVE_FAC_PARAM(hard_dog_mode);
	}
	else if(!strcmp(param,"DOGTIME"))
	{
		g_softap_fac_nv->hard_dog_time = (unsigned char)val;
		SAVE_FAC_PARAM(hard_dog_time);
	}
	
	else if (!strcmp(param, "VBAT"))
	{
		g_softap_fac_nv->min_mVbat = val;
		SAVE_FAC_PARAM(min_mVbat);
	}
	else if (!strcmp(param, "PINRESET"))
	{
		g_softap_fac_nv->pin_reset = val;
		SAVE_FAC_PARAM(pin_reset);
	}
	else if (!strcmp(param, "ATUART"))
	{
		g_softap_fac_nv->close_at_uart = val;
		SAVE_FAC_PARAM(close_at_uart);
	}
	else if (!strcmp(param, "MAH"))
	{
		g_softap_fac_nv->min_mah = val;
		SAVE_FAC_PARAM(min_mah);
	}
	
	else if (!strcmp(param, "RAI"))
	{
		g_softap_fac_nv->close_rai = val;
		SAVE_FAC_PARAM(close_rai);
	}
	
	else if (!strcmp(param, "RTCURC"))
	{
		g_softap_fac_nv->open_rtc_urc = val;
		SAVE_FAC_PARAM(open_rtc_urc);
	}
	
	else if (!strcmp(param, "EXTMEM"))
	{
		g_ps_mem_used_threshod = val;
	}

	else if (!strcmp(param, "UARTSET"))
	{
		g_softap_fac_nv->uart_rate = val;
		SAVE_FAC_PARAM(uart_rate);
	}
	
	else if (!strcmp(param, "HIGHFREQ"))
	{
		g_softap_fac_nv->high_freq_data = val;
		SAVE_FAC_PARAM(high_freq_data);
	}
	else if (!strcmp(param, "DROPUP"))
	{
		g_softap_fac_nv->g_drop_up_dbg = val;
		SAVE_FAC_PARAM(g_drop_up_dbg);
	}
	else if (!strcmp(param, "DROPDOWN"))
	{
		g_softap_fac_nv->g_drop_down_dbg = val;
		SAVE_FAC_PARAM(g_drop_down_dbg);
	}
	
	else if (!strcmp(param, "DOWNDATA"))
	{
		g_softap_fac_nv->down_data = val;
		SAVE_FAC_PARAM(down_data);
	}
	
	else if (!strcmp(param, "TAUURC"))
	{
		g_softap_fac_nv->open_tau_urc = val;
		SAVE_FAC_PARAM(open_tau_urc);
	}

	else if (!strcmp(param, "EXTLOCK"))
	{
		g_softap_fac_nv->ext_lock_disable = val;
		SAVE_FAC_PARAM(ext_lock_disable);
	}
	
	else if (!strcmp(param, "CLOSEDEBUG"))
	{
		g_softap_fac_nv->off_debug = val;
		SAVE_FAC_PARAM(off_debug);
	}
	
	else if (!strcmp(param, "CLOSEURC"))
	{
		g_softap_fac_nv->close_urc = val;
		SAVE_FAC_PARAM(close_urc);
	}
	
	else if (!strcmp(param, "DEMOTEST"))
	{
		g_softap_fac_nv->demotest = val;
		SAVE_FAC_PARAM(demotest);
	}
	
	else if (!strcmp(param, "NPSMR"))
	{
		g_softap_fac_nv->g_NPSMR_enable = val;
		SAVE_FAC_PARAM(g_NPSMR_enable);
	}
	
	//else if(!strcmp(param,"SWITCH"))
	//	g_softap_fac_nv->rf_switch = val;
	
	else if (!strcmp(param, "RFMODE"))
	{
		*(uint32_t*)(BACKUP_MEM_RF_MODE) = val;
	}
	
	else if (!strcmp(param, "RFTXLPM"))
	{
		g_softap_fac_nv->tx_low_power_flag = val;
		SAVE_FAC_PARAM(tx_low_power_flag);
	}
	
	else if (!strcmp(param, "RFTXMTP"))
	{
		g_softap_fac_nv->tx_multi_tone_power_flag = val;
		SAVE_FAC_PARAM(tx_multi_tone_power_flag);
	}

	else if (!strcmp(param, "TX3TPR"))
	{
		g_softap_fac_nv->tx_3tone_power_flag = val;
		SAVE_FAC_PARAM(tx_3tone_power_flag);
	}
	
	else if (!strcmp(param, "WORKMODE"))
	{
		g_softap_fac_nv->work_mode = val;
		SAVE_FAC_PARAM(work_mode);

		//设置单核模式，关闭云保存和保活
		if(val == 1)
		{
		    if(g_softap_fac_nv->save_cloud ==1)
		    {
	            g_softap_fac_nv->save_cloud = 0;
	            SAVE_FAC_PARAM(save_cloud);
		    }
		}
	}
	
	else if (!strcmp(param, "DM"))
	{
		g_softap_fac_nv->need_start_dm = val;
		SAVE_FAC_PARAM(need_start_dm);
	}
	
	else if (!strcmp(param, "OFFTIME"))
	{
		g_softap_fac_nv->offtime = val;
		SAVE_FAC_PARAM(offtime);
	}
	
	else if (!strcmp(param, "BACKUPKEEP"))
	{
		g_softap_fac_nv->keep_retension_mem = val;
		SAVE_FAC_PARAM(keep_retension_mem);
	}
	
	else if (!strcmp(param, "BACKUPTHRESHOLD"))
	{
		g_softap_fac_nv->backup_threshold = val;
		SAVE_FAC_PARAM(backup_threshold);
	}
		
	else if (!strcmp(param, "REGTIME"))
	{
		g_softap_fac_nv->dm_reg_time = val;
		SAVE_FAC_PARAM(dm_reg_time);
	}
	
	
	else if (!strcmp(param, "DUMPFLASH"))
	{
		g_softap_fac_nv->dump_mem_into_flash = val;
		SAVE_FAC_PARAM(dump_mem_into_flash);
	}
	//else if(!strcmp(param,"TCMCNT"))
	//g_softap_fac_nv->tcmcnt_enable = val;
	else if (!strcmp(param, "LOGSIZE"))
	{
		g_softap_fac_nv->log_size = val;
		SAVE_FAC_PARAM(log_size);
	}
	else if (!strcmp(param, "LOGFIFOLIMIT"))
	{
		g_softap_fac_nv->log_fifo_limit = val;
		SAVE_FAC_PARAM(log_fifo_limit);
	}
	else if(!strcmp(param, "LOGLENGTHLIMIT")){
		g_softap_fac_nv->log_length_limit = val;
		g_softap_fac_nv->log_length_limit = g_softap_fac_nv->log_length_limit < (31 - ucPortCountLeadingZeros(LOG_MAX_SIZE_LIMIT))? \
				g_softap_fac_nv->log_length_limit : 31 - ucPortCountLeadingZeros(LOG_MAX_SIZE_LIMIT);
		g_softap_fac_nv->log_length_limit = g_softap_fac_nv->log_length_limit > (31 - ucPortCountLeadingZeros(LOG_MIN_SIZE_LIMIT))? \
				g_softap_fac_nv->log_length_limit : 31 - ucPortCountLeadingZeros(LOG_MIN_SIZE_LIMIT);
		SAVE_FAC_PARAM(log_length_limit);
	}
	else if (!strcmp(param, "EXTPOOL"))
	{
		g_softap_fac_nv->dsp_ext_pool = val;
		SAVE_FAC_PARAM(dsp_ext_pool);
	}
	else if (!strcmp(param, "XTAL32K"))
	{
		g_softap_fac_nv->xtal_32k = val;
		g_softap_fac_nv->rc32k_freq = 0;
		SAVE_FAC_PARAM(xtal_32k);
		SAVE_FAC_PARAM(rc32k_freq);
	}
	else if(!strcmp(param,"PSUTCDIS"))
	{
		g_softap_fac_nv->ps_utcwkup_dis = val;
		SAVE_FAC_PARAM(ps_utcwkup_dis);
	}
	else if (!strcmp(param, "PTIME"))
	{
		g_softap_fac_nv->rfWaittingTime = val;
		SAVE_FAC_PARAM(rfWaittingTime);
	}
	else if (!strcmp(param, "INSERTDATA"))
	{
		g_softap_fac_nv->rfInsertData = val;
		SAVE_FAC_PARAM(rfInsertData);
	}
	else if (!strcmp(param, "VDDIOSEL"))
	{
		g_softap_fac_nv->pmu_ioldo_sel = val;
		SAVE_FAC_PARAM(pmu_ioldo_sel);
	}

	else
		return  0;
	return  1;

}

int  simple_get_val(char *param,char **prsp_cmd)
{
	if (!strcmp(param, "VERTYPE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->ver_type);
	
	else if (!strcmp(param, "PINRESET"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->pin_reset);

	else if (!strcmp(param, "FOTAPERIOD"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->fota_period);
		
	else if (!strcmp(param, "RATIME"))
		sprintf(*prsp_cmd, "\r\n%u\r\n\r\nOK\r\n", g_softap_fac_nv->ra_timeout);

	else if (!strcmp(param, "ATSTANDBY"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->at_standby_delay);
	
	else if (!strcmp(param, "TAURTC"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->set_tau_rtc);

	else if (!strcmp(param, "SAVECLOUD"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->save_cloud);
	
	else if (!strcmp(param, "CLOUDALIVE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->keep_cloud_alive);
	
	else if(!strcmp(param,"DMRETRYTIME"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->dm_retry_time);	
	
	else if(!strcmp(param,"DMREGTIME"))
		sprintf(*prsp_cmd,"\r\n%ld\r\n\r\nOK\r\n", g_softap_fac_nv->dm_reg_time);

	else if(!strcmp(param,"DMRETRYNUM"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->dm_retry_num);

	else if (!strcmp(param, "TEST"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->test);

	else if (!strcmp(param, "EXTLOCK"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->ext_lock_disable);
	
	else if (!strcmp(param, "ATUART"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->close_at_uart);
	
	else if(!strcmp(param, "DEEPSLEEPDELAY"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->deepsleep_delay);
	
	else if(!strcmp(param, "USERDOG"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->user_dog_time);

	else if(!strcmp(param, "DOGMODE"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",g_softap_fac_nv->hard_dog_mode);

	else if(!strcmp(param, "DOGTIME"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->hard_dog_time);

	else if(!strcmp(param,"XTALSOUR"))
		sprintf(*prsp_cmd,"\r\n%d\r\nOK\r\n",g_softap_fac_nv->xtal_32k);
	
	else if(!strcmp(param, "DEEPSLEEPTHRESHOLD"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",g_softap_fac_nv->deepsleep_threshold);
		
	else if(!strcmp(param, "DIV"))
		sprintf(*prsp_cmd,"\r\n%d, %d\r\n\r\nOK\r\n",g_softap_fac_nv->hclk_div, g_softap_fac_nv->pclk_div);
	
	else if (!strcmp(param, "WDT"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->watchdog_enable);
	
	else if (!strcmp(param, "SIMVCC"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->sim_vcc_ctrl);
	
	else if (!strcmp(param, "VBAT"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->min_mVbat);
	
	else if (!strcmp(param, "MAH"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->min_mah);
	
	else if (!strcmp(param, "DROPUP"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->g_drop_up_dbg);
	
	else if (!strcmp(param, "DROPDOWN"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->g_drop_down_dbg);
	
	else if (!strcmp(param, "UARTSET"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->uart_rate & 0x1ff);
	
	else if (!strcmp(param, "HIGHFREQ"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->high_freq_data);
	
	else if (!strcmp(param, "CLOSEDEBUG"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->off_debug);
	
	else if (!strcmp(param, "CLOSEURC"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->close_urc);
	
	else if (!strcmp(param, "DEMOTEST"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->demotest);
	
	else if (!strcmp(param, "DOWNDATA"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->down_data);
	
	else if (!strcmp(param, "TAUURC"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->open_tau_urc);
	
	else if (!strcmp(param, "NPSMR"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->g_NPSMR_enable);
	
	else if (!strcmp(param, "SWITCH"))
		sprintf(*prsp_cmd, "\r\n%d,%d,%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->rf_switch_mode_txl, g_softap_fac_nv->rf_switch_mode_rxl,
				g_softap_fac_nv->rf_switch_mode_txh, g_softap_fac_nv->rf_switch_mode_rxh);
	
	else if (!strcmp(param, "RAI"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->close_rai);

	else if (!strcmp(param, "RTCURC"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->open_rtc_urc);
		
	else if (!strcmp(param, "WORKMODE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->work_mode);
	
	else if (!strcmp(param, "DEEPSLEEP"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->deepsleep_enable);
	
	else if (!strcmp(param, "STANDBY"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->lpm_standby_enable);
	
	else if (!strcmp(param, "EXTVER"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->versionExt);
	
	else if (!strcmp(param, "HARDVER"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->hardver);

	else if (!strcmp(param, "PRODUCTVER"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->product_ver);

	else if (!strcmp(param, "OFFTIME"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->offtime);
	
	else if (!strcmp(param, "DM"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->need_start_dm);
	
	else if (!strcmp(param, "LOG"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->open_log);
	
	else if (!strcmp(param, "LOGSIZE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->log_size);
	
	else if (!strcmp(param, "LOGFIFOLIMIT"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->log_fifo_limit);

	else if (!strcmp(param, "LOGLENGTHLIMIT"))
			sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->log_length_limit);

	else if (!strcmp(param, "EXTPOOL"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->dsp_ext_pool);
	
	else if (!strcmp(param, "WFI"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->wfi_enable);
	
	else if (!strcmp(param, "BACKUPTHRESHOLD"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->backup_threshold);
	
	else if (!strcmp(param, "BACKUPKEEP"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->keep_retension_mem);
	
	else if (strstr(param, "FAC"))
		sprintf(*prsp_cmd, "\r\nVERTYPE:%d,LOG:%d,CLOSEDEBUG:%d,WORKMODE:%d,DOWNDATA:%d,OFFTIME:%d,KEEPRETENSION:%d\r\n\r\nOK\r\n", g_softap_fac_nv->ver_type,
				g_softap_fac_nv->open_log, g_softap_fac_nv->off_debug, g_softap_fac_nv->work_mode, g_softap_fac_nv->down_data, g_softap_fac_nv->offtime, g_softap_fac_nv->keep_retension_mem);
	
	else if (!strcmp(param, "APPKEY"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->dm_app_key);
	
	else if (!strcmp(param, "APPPWD"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->dm_app_pwd);
	
	else if (!strcmp(param, "CDPREGMODE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->cdp_register_mode);
	
	else if (!strcmp(param, "VER"))
		sprintf(*prsp_cmd, "\r\n%s,%s,%s,%s\r\n\r\nOK\r\n", g_softap_fac_nv->product_ver, g_softap_fac_nv->modul_ver, g_softap_fac_nv->hardver, g_softap_fac_nv->versionExt);
	else if (!strcmp(param, "REGTIME"))
		sprintf(*prsp_cmd, "\r\n%lu\r\n\r\nOK\r\n", g_softap_fac_nv->dm_reg_time);
	else if (!strcmp(param, "RFTXMTP"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->tx_multi_tone_power_flag);
	else if (!strcmp(param, "RFTXLPM"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->tx_low_power_flag);
	else if (!strcmp(param, "XO"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->xtal_type);
	else if (!strcmp(param, "RFMODE"))
		sprintf(*prsp_cmd, "\r\n%lu\r\n\r\nOK\r\n", *(uint32_t*)(BACKUP_MEM_RF_MODE));
	else if (!strcmp(param, "DUMPFLASH"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->dump_mem_into_flash);
	//else if(!strcmp(param,"TCMCNT"))
	//sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->tcmcnt_enable);
	else if (!strcmp(param, "BBGATE"))
		sprintf(*prsp_cmd, "\r\n%lu,%lu,%lu,%lu\r\n\r\nOK\r\n", g_softap_fac_nv->bb_threshold_1Tone, g_softap_fac_nv->bb_threshold_3Tone, g_softap_fac_nv->bb_threshold_6Tone, g_softap_fac_nv->bb_threshold_12Tone);
	else if (!strcmp(param, "AONUVLO"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->aon_uvlo_en);
	else if (!strcmp(param, "FLOWSIZE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_flow_control_below_size);
	else if (!strcmp(param, "XTAL32K"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->xtal_32k);
	else if (!strcmp(param, "PSUTCDIS"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->ps_utcwkup_dis);
	else if (!strcmp(param, "PTIME"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->rfWaittingTime);
	else if (!strcmp(param, "INSERTDATA"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->rfInsertData);
	else if (!strcmp(param, "VDDIOSEL"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->pmu_ioldo_sel);
	else if (!strcmp(param, "TX3TPR"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->tx_3tone_power_flag);

	else
		return  0;
	return  1;
}

int at_NV_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char cmd[10] = {0};
		char param[20] = {0};
		char *strval = xy_zalloc(64);
		char *verval = xy_zalloc(63);
		int val = 0;
		char * str = NULL;
		uint32_t str_len = 0;
		
		void *p[] = {cmd, param, &val};
		if (at_parse_param_2("%10s,", at_buf, p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_end;
		}
		if (!strcmp(cmd, "SAVE"))
		{
			int xy_nv_save = 1;
			int user_nv_save = 1;
			void *pp[] = {&xy_nv_save, &user_nv_save};

			if (at_parse_param_2(",%d,%d", at_buf, pp) != AT_OK)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				goto free_end;
			}
			send_urc_to_ext("\r\nREBOOTING\r\n");
			reboot_nv();

			HWREG(PS_START_SYNC) = 0;

			soft_reset_by_wdt(RB_BY_NRB);
		}
		else if (!strcmp(cmd, "FAC"))
		{
			//factory nv include DSP and ARM
			memcpy((void *)(SOFTAP_FAC_RAM_BASE), g_softap_fac_nv, sizeof(softap_fac_nv_t));
			xy_flash_write(NV_FLASH_MAIN_FACTORY_BASE, (void *)(RAM_FACTORY_NV_BASE), sizeof(factory_nv_t));
			goto free_end;
		}
		else if (!strcmp(cmd, "SET"))
		{
			p[0] = param;
			if (at_parse_param_2(",%20s,", at_buf, p) != AT_OK)
				goto ERR;

			if (!strcmp(param, "POWERTEST"))
			{
				p[0] = &val;
				if (at_parse_param_2(",,%d", at_buf, p) != AT_OK)
					goto ERR;
				at_POWER_test_req(val);
			}

			else if(!strcmp(param,"DIV"))
			{
				int hclk_div = 0;
				int pclk_div = 0;

				void* p[2] = {&hclk_div, &pclk_div};

				if (at_parse_param_2(",,%d,%d", at_buf, p) != AT_OK)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					goto free_end;
				}

				uint8_t hclk_valid_value[16] = {0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1};
				if(hclk_div <= 16 && hclk_div >= 4)
				{
					if(hclk_valid_value[hclk_div - 1] != 1)
					{
						*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
						goto free_end;
					}
					else
					{
						g_softap_fac_nv->hclk_div = (uint8_t)hclk_div;
						SAVE_FAC_PARAM(hclk_div);
					}
					
				}
				else
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					goto free_end;
				}

				if(pclk_div != 1)
    			{
        			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					goto free_end;
    			}
				else
				{
					g_softap_fac_nv->pclk_div = (uint8_t)pclk_div;
					SAVE_FAC_PARAM(pclk_div);
				}
			}

			else if (!strcmp(param, "WDT"))
			{
				int enable = 0;
				void *p[] = {&enable};

				if (at_parse_param_2(",,%d", at_buf, p) != AT_OK)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					goto free_end;
				}
				if (enable == 1)
				{
					if (g_softap_fac_nv->watchdog_enable == 0)
					{
						g_softap_fac_nv->watchdog_enable = 1;
						SAVE_FAC_PARAM(watchdog_enable);
						wdt_task_init();
					}
				}
				else if (enable == 0)
				{
					if (g_softap_fac_nv->watchdog_enable == 1)
					{
						g_softap_fac_nv->watchdog_enable = 0;
						SAVE_FAC_PARAM(watchdog_enable);
						wdt_task_deinit();
					}
				}
				else
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				}
			}
			else if (!strcmp(param, "SIMVCC"))
			{
				char vol_class = 0;
				void *p[] = {&vol_class};

				if (at_parse_param_2(",,%d", at_buf, p) != AT_OK)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					goto free_end;
				}
				if ((vol_class & 7) <= 3)
				{
					g_softap_fac_nv->sim_vcc_ctrl = vol_class;
					SAVE_FAC_PARAM(sim_vcc_ctrl);
				}
				else
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				}
			}
			else if (!strcmp(param, "APPKEY"))
			{
				p[0] = strval;
				if (at_parse_param_2(",,%64s", at_buf, p) != AT_OK)
					goto ERR;
				memset(g_softap_fac_nv->dm_app_key, 0, 64);
				memcpy(g_softap_fac_nv->dm_app_key, strval, 64);
				SAVE_FAC_PARAM(dm_app_key);
			}
			else if (!strcmp(param, "APPPWD"))
			{
				p[0] = strval;
				if (at_parse_param_2(",,%64s", at_buf, p) != AT_OK)
					goto ERR;
				memset(g_softap_fac_nv->dm_app_pwd, 0, 64);
				memcpy(g_softap_fac_nv->dm_app_pwd, strval, 64);
				SAVE_FAC_PARAM(dm_app_pwd);
			}
			else if (!strcmp(param, "PRODUCTVER"))
			{
				p[0] = verval;
				if (at_parse_param_2(",,%28s", at_buf, p))
					goto ERR;
				memset(g_softap_fac_nv->product_ver, 0, sizeof(g_softap_fac_nv->product_ver));
				memcpy(g_softap_fac_nv->product_ver, verval, strlen(verval));
				SAVE_FAC_PARAM(product_ver);
			}
			else if (!strcmp(param, "MODULVER"))
			{
				p[0] = verval;
				if (at_parse_param_2(",,%20s", at_buf, p))
					goto ERR;
				if (strrchr(verval, '-') == NULL)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				}
				else
				{
					memset(g_softap_fac_nv->modul_ver, 0, sizeof(g_softap_fac_nv->modul_ver));
					memcpy(g_softap_fac_nv->modul_ver, verval, strlen(verval));
					SAVE_FAC_PARAM(modul_ver);
				}
			}
			else if (!strcmp(param, "HARDVER"))
			{
				p[0] = verval;
				if (at_parse_param_2(",,%20s", at_buf, p))
					goto ERR;
				memset(g_softap_fac_nv->hardver, 0, sizeof(g_softap_fac_nv->hardver));
				memcpy(g_softap_fac_nv->hardver, verval, strlen(verval));
				SAVE_FAC_PARAM(hardver);
			}
			else if (!strcmp(param, "VERSIONEXT"))
			{
				p[0] = verval;
				if (at_parse_param_2(",,%28s", at_buf, p))
					goto ERR;
				memset(g_softap_fac_nv->versionExt, 0, sizeof(g_softap_fac_nv->versionExt));
				memcpy(g_softap_fac_nv->versionExt, verval, strlen(verval));
				SAVE_FAC_PARAM(versionExt);
			}
			else if (!strcmp(param, "CDPREGMODE"))
			{
				p[0] = &val;
				if (at_parse_param_2(",,%d", at_buf, p) != AT_OK)
					goto ERR;
				if (val != 0 && val != 1)
					goto ERR;

				g_softap_fac_nv->cdp_register_mode = val;
				SAVE_FAC_PARAM(cdp_register_mode);
			}
			else if (!strcmp(param, "PIN"))
			{
				int val1, val2, val3, val4;
				void *pp[] = {strval, &val1, &val2, &val3, &val4};

				if (at_parse_param_2(",,%64s,%d,%d,%d,%d", at_buf, pp) != AT_OK)
					goto ERR;
				if (!strcmp(strval, "BIT"))
				{
					g_softap_fac_nv->peri_remap_enable = val1;
					SAVE_FAC_PARAM(peri_remap_enable);
				}
				else if (!strcmp(strval, "SIM"))
				{
					g_softap_fac_nv->sim_clk_pin = val1;
					g_softap_fac_nv->sim_rst_pin = val2;
					g_softap_fac_nv->sim_data_pin = val3;
					SAVE_FAC_PARAM(sim_clk_pin);
					SAVE_FAC_PARAM(sim_rst_pin);
					SAVE_FAC_PARAM(sim_data_pin);
				}
				else if (!strcmp(strval, "SWD"))
				{
					g_softap_fac_nv->swd_swclk_pin = val1;
					g_softap_fac_nv->swd_swdio_pin = val2;
					SAVE_FAC_PARAM(swd_swclk_pin);
					SAVE_FAC_PARAM(swd_swdio_pin);
				}
				else if (!strcmp(strval, "JTAG"))
				{
					g_softap_fac_nv->jtag_jtdi_pin = val1;
					g_softap_fac_nv->jtag_jtms_pin = val2;
					g_softap_fac_nv->jtag_jtdo_pin = val3;
					g_softap_fac_nv->jtag_jtck_pin = val4;
					SAVE_FAC_PARAM(jtag_jtdi_pin);
					SAVE_FAC_PARAM(jtag_jtms_pin);
					SAVE_FAC_PARAM(jtag_jtdo_pin);
					SAVE_FAC_PARAM(jtag_jtck_pin);
				}
				else if (!strcmp(strval, "SPI"))
				{
					g_softap_fac_nv->spi_sclk_pin = val1;
					g_softap_fac_nv->spi_mosi_pin = val2;
					g_softap_fac_nv->spi_miso_pin = val3;
					g_softap_fac_nv->spi_ss_n_pin = val4;
					SAVE_FAC_PARAM(spi_sclk_pin);
					SAVE_FAC_PARAM(spi_mosi_pin);
					SAVE_FAC_PARAM(spi_miso_pin);
					SAVE_FAC_PARAM(spi_ss_n_pin);
				}
				else if (!strcmp(strval, "CSP2"))
				{
					g_softap_fac_nv->csp2_txd_pin = val1;
					g_softap_fac_nv->csp2_rxd_pin = val2;
					SAVE_FAC_PARAM(csp2_txd_pin);
					SAVE_FAC_PARAM(csp2_rxd_pin);
				}
				else if (!strcmp(strval, "CSP3"))
				{
					g_softap_fac_nv->csp3_txd_pin = val1;
					g_softap_fac_nv->csp3_rxd_pin = val2;
					SAVE_FAC_PARAM(csp3_txd_pin);
					SAVE_FAC_PARAM(csp3_rxd_pin);
				}
				else if (!strcmp(strval, "UART"))
				{
					g_softap_fac_nv->uart_txd_pin = val1;
					g_softap_fac_nv->uart_rxd_pin = val2;
					SAVE_FAC_PARAM(uart_txd_pin);
					SAVE_FAC_PARAM(uart_rxd_pin);
				}
				else if (!strcmp(strval, "I2C"))
				{
					g_softap_fac_nv->i2c1_sclk_pin = val1;
					g_softap_fac_nv->i2c1_sda_pin = val2;
					SAVE_FAC_PARAM(i2c1_sclk_pin);
					SAVE_FAC_PARAM(i2c1_sda_pin);
				}
				else if (!strcmp(strval, "PWM"))
				{
					g_softap_fac_nv->tmr1_pwm_outp_pin = val1;
					g_softap_fac_nv->tmr1_pwm_outn_pin = val2;
					SAVE_FAC_PARAM(tmr1_pwm_outp_pin);
					SAVE_FAC_PARAM(tmr1_pwm_outn_pin);
				}
				else if (!strcmp(strval, "LED"))
				{
					g_softap_fac_nv->led_pin = val1;
					SAVE_FAC_PARAM(led_pin);
				}
				else
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			}
			else if (!strcmp(param, "SWITCH"))
			{
				int switch_TXL, switch_RXL, switch_TXH, switch_RXH;
				void *pp[] = {&switch_TXL, &switch_RXL, &switch_TXH, &switch_RXH};
				if (at_parse_param_2(",,%d,%d,%d,%d", at_buf, pp) != AT_OK)
					goto ERR;
				else
				{
					g_softap_fac_nv->rf_switch_mode_txl = switch_TXL;
					g_softap_fac_nv->rf_switch_mode_rxl = switch_RXL;
					g_softap_fac_nv->rf_switch_mode_txh = switch_TXH;
					g_softap_fac_nv->rf_switch_mode_rxh = switch_RXH;
					SAVE_FAC_PARAM(rf_switch_mode_txl);
					SAVE_FAC_PARAM(rf_switch_mode_rxl);
					SAVE_FAC_PARAM(rf_switch_mode_txh);
					SAVE_FAC_PARAM(rf_switch_mode_rxh);
				}
			}
			else if (!strcmp(param, "BBGATE"))
			{
				int bb_1Tone, bb_3Tone, bb_6Tone, bb_12Tone;
				void *pp[] = {&bb_1Tone, &bb_3Tone, &bb_6Tone, &bb_12Tone};
				if (at_parse_param_2(",,%d,%d,%d,%d", at_buf, pp) != AT_OK)
					goto ERR;
				else
				{
					g_softap_fac_nv->bb_threshold_1Tone = bb_1Tone;
					g_softap_fac_nv->bb_threshold_3Tone = bb_3Tone;
					g_softap_fac_nv->bb_threshold_6Tone = bb_6Tone;
					g_softap_fac_nv->bb_threshold_12Tone = bb_12Tone;
					SAVE_FAC_PARAM(bb_threshold_1Tone);
					SAVE_FAC_PARAM(bb_threshold_3Tone);
					SAVE_FAC_PARAM(bb_threshold_6Tone);
					SAVE_FAC_PARAM(bb_threshold_12Tone);
				}
			}
			else if (!strcmp(param, "CLOUDIPTYPE"))
			{
                uint8_t i;
                char cloud_type[10];
                int ip_type = 0;
                static const char *types[] =
                {"CDP", "ONENET", "SOCKET", "CTWING",0};
                void* p[2] = {cloud_type, &ip_type};

				if (at_parse_param_2(",,%s,%d", at_buf, p) != AT_OK)
					goto ERR;
                for (i = 0; types[i] && strcasecmp(cloud_type,types[i]) != 0 ; ++i)
                    ;
				if (i >= CLOUD_IP_TYPE_MAX || (ip_type != 4 && ip_type != 6))
					goto ERR;

				if(ip_type == 6 )
				    cloud_set_ip_type(i,IPADDR_TYPE_V6);
				else
				    cloud_set_ip_type(i,IPADDR_TYPE_V4);
			}
			else
			{
				p[0] = param;
				p[1] = &val;
				if (at_parse_param_2(",%20s,%d", at_buf, p) != AT_OK)
					goto ERR;

				if(simple_set_val(param,val) == 1)
					goto free_end;
							
				else if (!strcmp(param, "DEEPSLEEP"))
				{
					g_softap_fac_nv->deepsleep_enable = val;
					SAVE_FAC_PARAM(deepsleep_enable);
					goto free_forward;
				}
				else if (!strcmp(param, "DEEPSLEEPDELAY"))
				{
					if ((unsigned char)val == 0)
						goto ERR;
					g_softap_fac_nv->deepsleep_delay = (unsigned char)val;
					SAVE_FAC_PARAM(deepsleep_delay);
				}
				else if (!strcmp(param, "STANDBY"))
				{
					g_softap_fac_nv->lpm_standby_enable = val;
					SAVE_FAC_PARAM(lpm_standby_enable);
					goto free_forward;
				}
				else if (!strcmp(param, "LOG"))
				{
					g_softap_fac_nv->open_log = val;
					SAVE_FAC_PARAM(open_log);
					get_req_to_dsp(at_buf);
					xy_log_task();
				}
				else if (!strcmp(param, "WFI"))
				{
					g_softap_fac_nv->wfi_enable = val;
					SAVE_FAC_PARAM(wfi_enable);
					goto free_forward;
				}
				else if (!strcmp(param, "RATETEST"))
				{
					goto free_forward;
				}
				else if (!strcmp(param, "AONUVLO"))
				{
					if (val == 0)
					{
						HWREGB(0xA0058019) &= ~(1 << 4);
					}
					else
					{
						HWREGB(0xA0058019) |= (1 << 4);
					}

					g_softap_fac_nv->aon_uvlo_en = val;
					SAVE_FAC_PARAM(aon_uvlo_en);
				}
				else if (!strcmp(param, "FLOWSIZE"))
				{
					g_flow_control_below_size = val;
					goto free_forward;
				}
				else
				{
					if (!DSP_IS_NOT_LOADED())
						goto free_forward;
					else
					{
						*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
						goto free_end;
					}
				}
			}
			goto free_end;
		}
		else if (!strcmp(cmd, "GET"))
		{
			char sub_param[15];

			p[0] = param;
			p[1] = sub_param;
			if (at_parse_param_2(",%20s,%s", at_buf, p) != AT_OK)
				goto ERR;
			*prsp_cmd = xy_zalloc(150);

			if(simple_get_val(param,prsp_cmd) == 1)
				goto free_end;
			else if (!strcmp(param, "EFTL"))
			{
				int addr = 0;
				hexstr2int(sub_param, &addr);
				sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n",(int)ftl_read_write_num(addr));
			}
			else if (!strcmp(param, "MEM"))
			{
				if (!DSP_IS_NOT_LOADED())
				{
					char *at_str = xy_zalloc(50);
					sprintf(at_str, "\r\n+ARMheapUsedPeak:%ld\r\n", GetHeapPeakUsed());
					send_urc_to_ext(at_str);
					xy_free(at_str);
					goto free_forward;
				}
				else
				{
					sprintf(*prsp_cmd, "\r\n+ARMheapUsedPeak:%ld\r\n\r\nOK\r\n", GetHeapPeakUsed());
				}
			}
			else if (!strcmp(param, "EXTMEM"))
			{
				if(!DSP_IS_NOT_LOADED())
				{
					str = xy_malloc(128);
					str_len = 0;
					str_len += sprintf(str + str_len, "\r\n+M3PeakUsed:%ld\r\n", g_ps_mem_used_max);
					str_len += sprintf(str + str_len, "+M3CurrentUsed:%ld", g_ps_mem_used);
					send_urc_to_ext(str);
					xy_free(str);
					goto free_forward;
				}
				else
				{
					str_len = 0;
					str_len += sprintf(*prsp_cmd + str_len, "\r\n+M3PeakUsed:%ld\r\n", g_ps_mem_used_max);
					str_len += sprintf(*prsp_cmd + str_len, "+M3CurrentUsed:%ld\r\nOK\r\n", g_ps_mem_used);
				}
			}
			else if (!strcmp(param, "PSM"))
			{
				if (!DSP_IS_NOT_LOADED())
				{
					char *at_str = xy_zalloc(40);
					sprintf(at_str, "\r\n+ARM:deep_sleep=%d\r\n", HAVE_FREE_LOCK);
					send_urc_to_ext(at_str);
					xy_free(at_str);
					goto free_forward;
				} 
				else
				{
					snprintf(*prsp_cmd, 40, "\r\n+ARM:deep_sleep=%d\r\n\r\nOK\r\n", HAVE_FREE_LOCK);
				}
				
			}
			else if (!strcmp(param, "PIN"))
			{
				if (!strcmp(sub_param, "BIT"))
					sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->peri_remap_enable);

				else if (!strcmp(sub_param, "SIM"))
					sprintf(*prsp_cmd, "\r\n%d,%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->sim_clk_pin, g_softap_fac_nv->sim_rst_pin, g_softap_fac_nv->sim_data_pin);

				else if (!strcmp(sub_param, "SWD"))
					sprintf(*prsp_cmd, "\r\n%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->swd_swclk_pin, g_softap_fac_nv->swd_swdio_pin);

				else if (!strcmp(sub_param, "JTAG"))
					sprintf(*prsp_cmd, "\r\n%d,%d,%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->jtag_jtdi_pin, g_softap_fac_nv->jtag_jtms_pin, g_softap_fac_nv->jtag_jtdo_pin, g_softap_fac_nv->jtag_jtck_pin);

				else if (!strcmp(sub_param, "SPI"))
					sprintf(*prsp_cmd, "\r\n%d,%d,%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->spi_sclk_pin, g_softap_fac_nv->spi_mosi_pin, g_softap_fac_nv->spi_miso_pin, g_softap_fac_nv->spi_ss_n_pin);

				else if (!strcmp(sub_param, "CSP2"))
					sprintf(*prsp_cmd, "\r\n%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->csp2_txd_pin, g_softap_fac_nv->csp2_rxd_pin);

				else if (!strcmp(sub_param, "CSP3"))
					sprintf(*prsp_cmd, "\r\n%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->csp3_txd_pin, g_softap_fac_nv->csp3_rxd_pin);

				else if (!strcmp(sub_param, "UART"))
					sprintf(*prsp_cmd, "\r\n%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->uart_txd_pin, g_softap_fac_nv->uart_rxd_pin);

				else if (!strcmp(sub_param, "I2C"))
					sprintf(*prsp_cmd, "\r\n%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->i2c1_sclk_pin, g_softap_fac_nv->i2c1_sda_pin);

				else if (!strcmp(sub_param, "PWM"))
					sprintf(*prsp_cmd, "\r\n%d,%d\r\n\r\nOK\r\n", g_softap_fac_nv->tmr1_pwm_outp_pin, g_softap_fac_nv->tmr1_pwm_outn_pin);

				else if (!strcmp(sub_param, "LED"))
					sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->led_pin);
				else
				{
					if (*prsp_cmd != NULL)
						xy_free(*prsp_cmd);
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				}
			}
			else if (!strcmp(param, "TICK"))
			{				
				if ( !DSP_IS_NOT_LOADED())
				{
					char *at_str = xy_zalloc(50);
					sprintf(at_str, "\r\n+M3TICK:%ld\r\n", osKernelGetTickCount());
					send_urc_to_ext(at_str);
					xy_free(at_str);
					goto free_forward;
				}
				else
				{
					sprintf(*prsp_cmd, "\r\n+M3TICK:%ld\r\n\r\nOK\r\n", osKernelGetTickCount());
				}
			}
            else if (!strcmp(param, "CLOUDIPTYPE"))
            {
                uint8_t i;
                int ip_type = 0;
                char cloud_type[10];
                p[0] = cloud_type;
                static const char *types[] =
                {"CDP", "ONENET", "SOCKET", "CTWING",0};

                if (at_parse_param_2(",,%s", at_buf, p) != AT_OK)
                    goto ERR;
                for (i=0; types[i] && strcasecmp(cloud_type,types[i]) != 0 ; ++i)
                    ;
                if (i >= CLOUD_IP_TYPE_MAX )
                    goto ERR;

                ip_type = cloud_get_ip_type(i);
                if(ip_type == IPADDR_TYPE_ANY)
                    sprintf(*prsp_cmd, "\r\n+%s_IP_TYPE:non_config\r\n\r\nOK\r\n",types[i]);
                else if (ip_type == IPADDR_TYPE_V6)
                    sprintf(*prsp_cmd, "\r\n+%s_IP_TYPE:IPv6\r\n\r\nOK\r\n",types[i]);
                else
                    sprintf(*prsp_cmd, "\r\n+%s_IP_TYPE:IPv4\r\n\r\nOK\r\n",types[i]);
            }
			else
			{
				if(*prsp_cmd != NULL)
				{
					xy_free(*prsp_cmd);
					*prsp_cmd = NULL;
				}			
free_forward:
				xy_free(strval);
				xy_free(verval);
				goto FORWARD;
			}
free_end:
			xy_free(strval);
			xy_free(verval);
			return AT_END;
		}
ERR:
		xy_free(strval);
		xy_free(verval);
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		char *str = "\r\n+NV:SIMVCC+POWERTEST+OPENTRACE+WDT+FTLIDX+CLOUDTYPE+HIGHFREQ+MEM+MEMEXT+ARMSTACK+STATICMEM+CLOSEDEBUG+DEMOTEST+NPSMR+DOWNDATA+CLOSEURC+WORKMODE+DEEPSLEEP+DROPED+DIRTY+WAKEUPDELAY+STANDBY+IPALIVE+EXTVER+HARDVER+VERTYPE+DM+OFFTIME+FACTORY+WFI+PSM+BACKUPTHRESHOLD+BACKUPKEEP+VER+POWERVER+TASKMODE+UARTSET+PIN(BIT+SWD+JTAG+CSP2+UART+CSP3+I2C+PWM+LED+SWITCH+SPI)\r\n\r\nOK\r\n";
		*prsp_cmd = xy_zalloc(strlen(str) + 1);
		strcpy(*prsp_cmd, str);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
FORWARD:
	if (!DSP_IS_NOT_LOADED())
		return AT_FORWARD;

	return AT_END;
}

int proc_FORCEDL_req(int delay)
{
	HWREG(0xA0110008) |= 0x4803C;
	HWREG(0xA0110018) = 0x0;
	HWREG(0xA0058028) = 0x81;
	HWREG(0xA0040000) = 0x21;
	HWREG(0xA0040004) = delay;

	return 0;
}

int at_FORCEDL_info(char *at_buf)
{
	int delay = 10;
	void *p[] = {&delay};

	if (at_parse_param_2("%d", at_buf, p) != AT_OK)
	{
		softap_printf(USER_LOG, WARN_LOG, "+DSPNVRAM:ERR!!!");
		return AT_END;
	}
	
	send_urc_to_ext("\r\nOK\r\n");

	osCoreEnterCritical();
	while(csp_write_allout_state() == 0);
	
	proc_FORCEDL_req(delay);
	
	while(1);
}

int at_FORCEDL_req(char *at_buf, char **prsp_cmd)
{
	int delay = 10;
	void *p[] = {&delay};

	if (at_parse_param_2("%d", at_buf, p) != AT_OK)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	send_urc_to_ext("\r\nOK\r\n");

	osCoreEnterCritical();
	while(csp_write_allout_state() == 0);
	
	proc_FORCEDL_req(delay);

	while(1);
}

extern void dsp_suspend();
#define AddrAlign(size, align)  (((unsigned int )size + align - 1) & (~(align - 1)))

/*死机时将内存保存到FOTA区域的flash中，下次开机通过该命令读flash，然后通过trace32进行查看死机*/
int at_DUMP_req(char *at_buf, char **prsp_cmd)
{

	char mem_cmd[15] = {0};
	void* p[1] = { mem_cmd };
//	UINTPTR uvIntSave, uvRet = XY_ERR;

	if (at_parse_param_2("%15s", at_buf, p) != AT_OK)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	if(strstr(mem_cmd, "DUMPFLASH"))
	{
		extern uint32_t _Flash_Used;
		extern uint32_t _Ram_Text;
		extern uint32_t _Ram_Data;
		extern uint32_t _Ram_Bss;
		extern uint32_t __Main_Stack_Size;
		extern uint32_t __vectors_start;

		dsp_suspend();
		vTaskSuspendAll();

		uint32_t DumpBase = ARM_FLASH_BASE_ADDR + AddrAlign((AddrAlign(((uint32_t)&_Flash_Used),0x1000) + (uint32_t)&_Ram_Text + (uint32_t)&_Ram_Data), 0x1000);
		uint32_t DumpBaseDSP = FOTA_BACKUP_BASE + FOTA_BACKUP_LEN_MAX - (RAM_INVAR_NV_MAXLEN + RAM_FACTORY_NV_MAXLEN + RAM_ICM_BUF_LEN + RAM_XTEND_SBUF_LEN) - DSP_DATA_DRAM_LEN - DSP_DATA_SRAM_LEN - 0x9000;
		uint32_t flash_factory_nv_xtal_addr = DumpBaseDSP + (RAM_INVAR_NV_BASE - AddrAlign(RAM_INVAR_NV_BASE - 0x1000,0x1000)) + RAM_INVAR_NV_MAXLEN + (unsigned int)&(((softap_fac_nv_t *)((unsigned int)&(((factory_nv_t *)0)->softap_fac_nv)))->xtal_freq);
		uint32_t offset = 0;
		uint32_t offsetdsp = 0;

		if(*(uint32_t *)DumpBase == *(uint32_t *)((uint32_t)&__vectors_start))
		{
			mem_dump_info_flash("Dump_Begin", DumpBase, AddrAlign(((uint32_t)&_Ram_Text + (uint32_t)&_Ram_Data + (uint32_t)&_Ram_Bss), 0x1000));

			mem_dump_info_flash("m3_base", DumpBase, AddrAlign(((uint32_t)&_Ram_Text + (uint32_t)&_Ram_Data + (uint32_t)&_Ram_Bss), 0x1000));
			offset += AddrAlign(((uint32_t)&_Ram_Text + (uint32_t)&_Ram_Data + (uint32_t)&_Ram_Bss), 0x1000);

			mem_dump_info_flash("m3_main_stack", DumpBase + offset, AddrAlign(((unsigned int)&__Main_Stack_Size), 0x1000));
			offset += AddrAlign(((unsigned int)&__Main_Stack_Size), 0x1000);

			mem_dump_info_flash("RingBuf", DumpBase + offset, AddrAlign(((RAM_ICM_BUF_LEN + RAM_XTEND_SBUF_LEN)), 0x2000));
			offset += AddrAlign((RAM_ICM_BUF_LEN + RAM_XTEND_SBUF_LEN), 0x2000);

			mem_dump_info_flash("m3_current_stack", DumpBase + offset, 0x3000);

			mem_dump_info_flash("Dump_End", DumpBase + offset, 0x3000);
		}

		if(*(uint32_t *)flash_factory_nv_xtal_addr == g_softap_fac_nv->xtal_freq)
		{
			mem_dump_info_flash("Dump_Begin", DumpBaseDSP, 0x10000);

		 	mem_dump_info_flash("NV_ringbuf", DumpBaseDSP, AddrAlign(RAM_INVAR_NV_MAXLEN + RAM_FACTORY_NV_MAXLEN + RAM_ICM_BUF_LEN + RAM_XTEND_SBUF_LEN, 0x2000));
		 	offsetdsp += AddrAlign(RAM_INVAR_NV_MAXLEN + RAM_FACTORY_NV_MAXLEN + RAM_ICM_BUF_LEN + RAM_XTEND_SBUF_LEN, 0x2000);

		 	mem_dump_info_flash("Dump_Begin", DumpBaseDSP, 0x10000);

		 	mem_dump_info_flash("dsp_dram",DumpBaseDSP + offsetdsp, AddrAlign(DSP_DATA_DRAM_LEN,0x2000));
		 	offsetdsp += AddrAlign(DSP_DATA_DRAM_LEN,0x2000);

		 	mem_dump_info_flash("dsp_sram", DumpBaseDSP + offsetdsp, AddrAlign(DSP_DATA_SRAM_LEN,0x1000));
		 	offsetdsp += AddrAlign(DSP_DATA_SRAM_LEN,0x1000);

		 	mem_dump_info_flash("dsp_current", DumpBaseDSP + offsetdsp, 0x3000);

		 	mem_dump_info_flash("Dump_End", DumpBaseDSP + offsetdsp, 0x3000);

		}
		( void ) xTaskResumeAll();
		return AT_END;
	}
	else
	{
		return AT_FORWARD;	
	}
}

extern unsigned int log_totalsize_overlimit;
extern unsigned int log_queue_overlimit;
extern volatile unsigned int log_send_num;
int at_NUESTATS_req(char* at_buf, char** prsp_cmd)
{
	uint8_t tasknum;
	uint32_t tbl_used_len;
	char mem_cmd[20] = {0};
	void* p[1] = { mem_cmd };
	HeapStats_t at_heapstats;
	memStatus_t memStatus[] = {0};
	uint32_t xPeakUsedSize;
	uint16_t str_len = 0;
	uint32_t heap_size;

	if (at_parse_param_2("%19s", at_buf, p) != AT_OK)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	if (strstr(mem_cmd, "APPSMEM")|| strstr(mem_cmd, "ARMMEM"))
	{
		*prsp_cmd = xy_malloc(400);
		osCoreEnterCritical();
		vPortGetHeapStats(&at_heapstats);
		osCoreExitCritical();
		heap_size = GetHeapSize();
		str_len = 0;
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,TotalHeapSize:%ld\r\n", heap_size);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,PeakUsedSize:%ld\r\n", GetHeapPeakUsed());
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,CurrentAllocated:%ld\r\n", heap_size - at_heapstats.xAvailableHeapSpaceInBytes);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,TotalFree:%d\r\n", at_heapstats.xAvailableHeapSpaceInBytes);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,MaxFree:%d\r\n", at_heapstats.xSizeOfLargestFreeBlockInBytes);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,MinFree:%d\r\n", at_heapstats.xSizeOfSmallestFreeBlockInBytes);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,NumberAllocs:%d\r\n", at_heapstats.xNumberOfSuccessfulAllocations);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:APPSMEM,NumberFrees:%d\r\n", at_heapstats.xNumberOfSuccessfulFrees);
		strcat(*prsp_cmd, "\r\nOK\r\n");

		return AT_END;
	}
	else if(strstr(mem_cmd, "ARMSTACK"))
	{	
		UBaseType_t ArraySize;
		uint8_t x;
		osStackStatus_t *StatusArray = NULL;
		//unsigned int pulTotalRuntime;

		osCoreEnterCritical();
		ArraySize = uxTaskGetNumberOfTasks();
		StatusArray = xy_zalloc(ArraySize * sizeof(osStackStatus_t));
		*prsp_cmd = xy_zalloc(ArraySize * 100);
		GetStackStatistics(StatusArray, ArraySize);
		osCoreExitCritical();

		str_len = 0;
		for(x = 0; x < ArraySize; x++)
		{
			str_len += sprintf(*prsp_cmd + str_len, "\r\n+%s,StackSize:%ld\r\n", StatusArray[x].name, StatusArray[x].stackSize);
			str_len += sprintf(*prsp_cmd + str_len, "\r\n+%s,CurrentUsingSize:%ld\r\n",StatusArray[x].name, StatusArray[x].currentUseSize);
			str_len += sprintf(*prsp_cmd + str_len, "\r\n+%s,PeakUsedSize:%ld\r\n", StatusArray[x].name, StatusArray[x].peakUsedSize);
		}

		strcat(*prsp_cmd, "\r\nOK\r\n");
		xy_free(StatusArray);
		return AT_END;	
	}
	else if(strstr(mem_cmd, "STACKPEAK"))
	{
		tbl_used_len = GetStackPeakTblUsed();
		*prsp_cmd = xy_malloc((tbl_used_len + 1) * 50);  /* tbl_used_len不包含主栈 */
		str_len = 0;
		for(tasknum = 0; tasknum < tbl_used_len; tasknum++)  //LOSCFG_BASE_CORE_TSK_LIMIT
		{
			xPeakUsedSize = stackPeakTbl[tasknum].peakUsed;
			str_len += sprintf(*prsp_cmd + str_len, "\r\n+%s,PeakUsedSize:%ld\r\n", stackPeakTbl[tasknum].name, xPeakUsedSize*32);
		}	
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+main_stack,PeakUsedSize:%ld\r\n", GetMainStackPeakUsed());
		strcat(*prsp_cmd, "\r\nOK\r\n");
		return AT_END;
	}
	else if(strstr(mem_cmd, "ALLMEM"))
	{	
		osCoreEnterCritical();
		GetMemStatistics(memStatus);
		osCoreExitCritical();
  
		*prsp_cmd = xy_zalloc(300);
		str_len = 0;
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+TotalFlashSPACE:%ld\r\n", memStatus->flash_total);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+FlashUsed:%ld\r\n", memStatus->flash_used);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+FlashRemaining:%ld\r\n", memStatus->flash_total - memStatus->flash_used - memStatus->data - memStatus->text);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+TotalRAMSPACE:%ld\r\n", memStatus->ram_total);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+DataUsed:%ld\r\n", memStatus->data);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+TEXTUsed:%ld\r\n", memStatus->text);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+BSSUsed:%ld\r\n", memStatus->bss);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+RAMRemaining:%ld\r\n", memStatus->ram_total - memStatus->data - memStatus->text -memStatus->bss);

		strcat(*prsp_cmd, "\r\nOK\r\n");

		return AT_END;	
	}
	else if(strstr(mem_cmd, "RESETARMHEAPPEAK"))
	{	
		*prsp_cmd = xy_malloc(16);
		ResetHeapPeakUsed();
		sprintf(*prsp_cmd, "\r\nOK\r\n");

		return AT_END;	
	}
	else if(strstr(mem_cmd, "LOGDEBUG"))
	{
		unsigned int xlog_totalsize_overlimit;
		unsigned int xlog_queue_overlimit;
		unsigned int xlog_send_num;
		osCoreEnterCritical();
		xlog_totalsize_overlimit = log_totalsize_overlimit;
		xlog_queue_overlimit = log_queue_overlimit;
		xlog_send_num = log_send_num;
		osCoreExitCritical();

		*prsp_cmd = xy_zalloc(300);
		str_len = 0;
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+log_totalsize_overlimit:%d\r\n", xlog_totalsize_overlimit);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+log_queue_overlimit:%d\r\n", xlog_queue_overlimit);
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+log_send_num:%d\r\n",xlog_send_num);
		strcat(*prsp_cmd, "\r\nOK\r\n");

		return AT_END;
	}
	else
	{
		return AT_FORWARD;	
	}
}

int at_CGMI_req(char *at_buf, char **prsp_cmd)
{
#if (!VER_QUCTL260)
	if(g_req_type == AT_CMD_REQ)
	{
		char modul_ver_temp[8] = {0};
		char* modul_ver_char = xy_zalloc(21);
		char* modul_ver_nv = xy_zalloc(21);
		char *end_str = NULL;

		if (at_parse_param("%7s,", at_buf, modul_ver_temp) != AT_OK || strlen(modul_ver_temp) == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(modul_ver_char);
			xy_free(modul_ver_nv);
			return AT_END;
		}
		memcpy(modul_ver_char,modul_ver_temp,strlen(modul_ver_temp));
		
		memcpy(modul_ver_nv,g_softap_fac_nv->modul_ver,strlen((const char*)(g_softap_fac_nv->modul_ver))>=20?19:strlen((const char*)(g_softap_fac_nv->modul_ver)));
				
		end_str = strchr(modul_ver_nv,'-');
		
		if(end_str != NULL)
		{
			sprintf(modul_ver_char+strlen(modul_ver_char),"%s",end_str);
		}	
		
		if(end_str == NULL && strlen(modul_ver_char) > 19)
		{	
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(modul_ver_char);
			xy_free(modul_ver_nv);
			return AT_END;
		}
		
		memset(g_softap_fac_nv->modul_ver,0,20);
		memcpy(g_softap_fac_nv->modul_ver,modul_ver_char,strlen(modul_ver_char)>=20?19:strlen(modul_ver_char));
			
		if(end_str == NULL && strlen(modul_ver_char) < 19)
			g_softap_fac_nv->modul_ver[strlen((const char*)(g_softap_fac_nv->modul_ver))] = '-';
		
		//if softap factory NV vary,must set 1
		if(modul_ver_char != NULL)
			xy_free(modul_ver_char);
		if(modul_ver_nv != NULL)
			xy_free(modul_ver_nv);

		SAVE_FAC_PARAM(modul_ver);
	}
	else if(g_req_type == AT_CMD_ACTIVE)
#else
	UNUSED_ARG(at_buf);
	if(g_req_type == AT_CMD_ACTIVE)
#endif
	{
		char *head;
		char *end;
		char manufa_code[25] = {0};
		
		head = (char*)g_softap_fac_nv->modul_ver;
		end = strchr(head,'-');
		*prsp_cmd = xy_zalloc(128);
		if(end == NULL)
		{
			#if MODULE_TNB100
				sprintf(*prsp_cmd, ""VENDER_NAME" \r\n"VENDER_NAME"_"MODULE"\r\nRevision: XY1100_"MODULE"\r\n\r\nOK\r\n");
			#else
				sprintf(*prsp_cmd, ""VENDER_NAME" \r\n"VENDER_NAME"_"MODULE"\r\nRevision: XY1100\r\n\r\nOK\r\n");
			#endif
			return AT_END;		
		}
		memcpy(manufa_code,head,end-head);
		//snprintf(*prsp_cmd, 30, "\r\n%s\r\n\r\nOK\r\n", manufa_code);
		#if MODULE_TNB100
			sprintf(*prsp_cmd, ""VENDER_NAME" \r\n"VENDER_NAME"_"MODULE"\r\nRevision: XY1100_"MODULE"\r\n\r\nOK\r\n");
		#else
			sprintf(*prsp_cmd, ""VENDER_NAME" \r\n"VENDER_NAME"_"MODULE"\r\nRevision: XY1100\r\n\r\nOK\r\n");
		#endif
	}
	else if (g_req_type == AT_CMD_TEST)
	{
	    return AT_END;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int at_CGMM_req(char *at_buf, char **prsp_cmd)
{
#if (!VER_QUCTL260)
	if(g_req_type == AT_CMD_REQ)
	{
		char *modul = xy_zalloc(13);
		char* modul_ver_char = xy_zalloc(21);
		char* modul_ver_nv = xy_zalloc(21);
		char* end_str = NULL;

		if (at_parse_param("%12s,", at_buf, modul) != AT_OK || strlen(modul) == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(modul);
			xy_free(modul_ver_char);
			xy_free(modul_ver_nv);
			return AT_END;
		}

		memcpy(modul_ver_nv,g_softap_fac_nv->modul_ver,strlen((const char *)(g_softap_fac_nv->modul_ver))>=20?19:strlen((const char *)(g_softap_fac_nv->modul_ver)));
		end_str = strchr(modul_ver_nv,'-');
		
		if(end_str != NULL)
			memcpy(modul_ver_char,modul_ver_nv,(int)(end_str-modul_ver_nv)+1);
		else
			memcpy(modul_ver_char,"-",1);
		
		sprintf(modul_ver_char+strlen(modul_ver_char),"%s",modul);

		if(strlen(modul_ver_char) > 19)
		{	
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(modul);
			xy_free(modul_ver_char);
			xy_free(modul_ver_nv);
			return AT_END;
		}
		
		memset(g_softap_fac_nv->modul_ver,0,sizeof(g_softap_fac_nv->modul_ver));		
		memcpy(g_softap_fac_nv->modul_ver,modul_ver_char,strlen(modul_ver_char)>=20?19:strlen(modul_ver_char));

		xy_free(modul);
		xy_free(modul_ver_char);
		xy_free(modul_ver_nv);
		
		SAVE_FAC_PARAM(modul_ver);
	}
	else if(g_req_type == AT_CMD_ACTIVE)
#else
	UNUSED_ARG(at_buf);
	if(g_req_type == AT_CMD_ACTIVE)
#endif
	{
		char *head;
		char module_type[25]={0};

		*prsp_cmd = xy_zalloc(30);
		if(!strchr((const char *)(g_softap_fac_nv->modul_ver),'-'))
		{
			//snprintf(*prsp_cmd, 30, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->modul_ver);
			sprintf(*prsp_cmd, ""VENDER_NAME"_"MODULE"\r\n\r\nOK\r\n");
			return AT_END;		
		}
		head = strchr((const char *)(g_softap_fac_nv->modul_ver),'-')+1;
		memcpy(module_type,head,strlen(head));
		//snprintf(*prsp_cmd, 30, "\r\n%s\r\n\r\nOK\r\n", module_type);
		sprintf(*prsp_cmd, "\r\n"VENDER_NAME"_"MODULE"\r\n\r\nOK\r\n");
	}
	else if (g_req_type == AT_CMD_TEST)
	{
	    return AT_END;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int at_CMVER_req(char *at_buf, char **prsp_cmd)
{

	unsigned char    compTime[9]={0};
    memcpy(compTime, getNewBuildInfo(),9);
#if (!VER_QUCTL260)	
	if(g_req_type == AT_CMD_REQ)
	{
		char verval[28] = {0};

		if(at_parse_param("%28s", at_buf, verval) != AT_OK || strlen(verval) == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		memset(g_softap_fac_nv->versionExt, 0 ,sizeof(g_softap_fac_nv->versionExt));
		memcpy(g_softap_fac_nv->versionExt, verval,strlen(verval));
		//if softap factory NV vary,must set 1
		SAVE_FAC_PARAM(versionExt);
	}
	else if(g_req_type == AT_CMD_ACTIVE)
#else
	UNUSED_ARG(at_buf);
	if(g_req_type == AT_CMD_ACTIVE)
#endif
	{
		*prsp_cmd = xy_zalloc(60);
#if VER_QUCTL260
		//snprintf(*prsp_cmd, 60, "\r\nVersion:%s\r\n\r\nOK\r\n", g_softap_fac_nv->versionExt);
		snprintf(*prsp_cmd, 60, "\r\nRevision: %s_%s%s\r\n\r\nOK\r\n", MODULE, TVERSION, SDATE);
#else
		snprintf(*prsp_cmd, 60, "\r\nSoftware Version:%s\r\n\r\nOK\r\n", g_softap_fac_nv->versionExt);
#endif
	}
	else if (g_req_type == AT_CMD_TEST)
	{
	    return AT_END;
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;	
}

int at_HVER_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		char verval[20] = {0};
			
		if(at_parse_param("%20s", at_buf, verval) != AT_OK || strlen(verval) == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		memset(g_softap_fac_nv->hardver, 0 ,sizeof(g_softap_fac_nv->hardver));
		memcpy(g_softap_fac_nv->hardver, verval,strlen(verval));
		SAVE_FAC_PARAM(hardver);
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_zalloc(55);
		snprintf(*prsp_cmd, 55, "\r\nHardware Version:%s\r\n\r\nOK\r\n", g_softap_fac_nv->hardver);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;	
}

//AT+NPSMR=<n>         AT+NPSMR? +NPSMR:<n>[,<mode>]
//when  <n>=1,and worklock is 0,SoC will send unsolicited result +NPSMR:<mode>
int at_NPSMR_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int val = 0;

		if (at_parse_param("%d", at_buf, &val) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		if (val != 0 && val != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		g_softap_fac_nv->g_NPSMR_enable = val;
		SAVE_FAC_PARAM(g_NPSMR_enable);
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(40);

		if (g_softap_fac_nv->g_NPSMR_enable == 1)
		{
			snprintf(*prsp_cmd, 40, "\r\n+NPSMR:1,0\r\n\r\nOK\r\n");
		}
		else
		{
			snprintf(*prsp_cmd, 40, "\r\n+NPSMR:0\r\n\r\nOK\r\n");
		}
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(40);
		snprintf(*prsp_cmd, 40, "\r\n+NPSMR:(0,1)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

int at_ATI_req(char *at_buf, char **prsp_cmd)
{
	(void)at_buf;

	 unsigned char   compTime[9]={0};
	 memcpy(compTime, getNewBuildInfo(),9);	
	
#if VER_QUCTL260
	if (g_req_type == AT_CMD_ACTIVE)
#else
	if (g_req_type == AT_CMD_REQ || g_req_type == AT_CMD_ACTIVE)
#endif
	{
#ifdef MODULE_VER
#ifdef PRODUCT_VER
		*prsp_cmd = xy_zalloc(128);
#if VER_QUECTEL || VER_QUCTL260
	    //snprintf(*prsp_cmd, 128, "\r\nXY1100\r\n%s\r\nRevision:%s\r\n\r\nOK\r\n", MODULE_VER, PRODUCT_VER);
		//snprintf(*prsp_cmd, 128, "\r\nMeiG \r\n"MODULE"\r\nRevision:"MODULE"_%sS%c%c%c%c\r\nOK\r\n", TVERSION, compTime[4],compTime[5],compTime[6],compTime[7]);
		snprintf(*prsp_cmd, 128, "\r\n"VENDER_NAME" \r\n"MODULE"\r\nRevision: "MODULE"_%s%s [Oct 16 2024 15:35:00]\r\n\r\nOK\r\n", TVERSION, SDATE);		
#else
		snprintf(*prsp_cmd, 128, "XY1100\r\n%s\r\nRevision:%s\r\n\r\nOK\r\n", MODULE_VER, PRODUCT_VER);
#endif //VER_QUECTEL
#endif
#endif
	}
#if VER_QUECTEL || VER_QUCTL260
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
#endif //VER_QUECTEL
	return AT_END;
}

//获取产品版本号
int at_QGMR_req(char *at_buf, char **prsp_cmd)
{
	(void)at_buf;

	if (g_req_type == AT_CMD_ACTIVE)
	{
#ifdef PRODUCT_VER
		*prsp_cmd = xy_zalloc(128);
		snprintf(*prsp_cmd, 128, "\r\n%s\r\n\r\nOK\r\n", PRODUCT_VER);
#endif
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

//获取版本编译时间
int at_QVERTIME_req(char *at_buf, char **prsp_cmd)
{
	(void)at_buf;

	if (g_req_type == AT_CMD_ACTIVE)
	{
#ifdef COMPILE_TIME
		*prsp_cmd = xy_zalloc(128);
		snprintf(*prsp_cmd, 128, "\r\n%s\r\n\r\nOK\r\n", COMPILE_TIME);
#endif
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}


int at_SGSW_req(char *at_buf, char **prsp_cmd)
{
	(void)at_buf;
	unsigned char   compTime[9]={0};
    memcpy(compTime, getNewBuildInfo(),9);
	
	if (g_req_type == AT_CMD_REQ || g_req_type == AT_CMD_ACTIVE)
	{

		*prsp_cmd = xy_zalloc(128);
       // snprintf(*prsp_cmd,128,"MeiG\r\n%s\r\n%s_%s%s",MODULE,MODULE,TVERSION,SDATE);
       //sprintf(*prsp_cmd, "\r\n%s.%s%sS%c%c%c%c_%s_XY1100\r\n\r\nOK\r\n",MODULE,SDKVERSION,TVERSION,compTime[4],compTime[5],compTime[6],compTime[7],MVERSION);
		sprintf(*prsp_cmd, "\r\n%s.%s%s%s_%s_XY1100\r\n\r\nOK\r\n",MODULE,SDKVERSION,TVERSION,SDATE,MVERSION);
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}

	return AT_END;
}

//kt
int at_QCGDEFCONT_req(char *at_buf, char **prsp_cmd)
{	
	unsigned char aPdpType[][7] = {"IP","IPV6","IPV4V6","PPP","Non-IP"};//已有类型
	unsigned char aucPdpType[][7] = {"RESERV","IP","IPV6","IPV4V6","RESERV","Non-IP"};//CGDCONT显示类型数组

	if (g_req_type == AT_CMD_REQ)
	{
		int i;
		int val;
		unsigned char PdpType[7] = {0};
		unsigned char aucApnValue[D_APN_LEN] = {0};
		unsigned char aucUserName[D_PCO_AUTH_MAX_LEN] = {0};
		unsigned char aucPassword[D_PCO_AUTH_MAX_LEN] = {0};
		
		char at_QCGDEFCONT[150]={0};
	
		if (at_parse_param("%s,%s,%s,%s", at_buf, PdpType,aucApnValue,aucUserName,aucPassword) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		for(i=0;i<5;i++)
		{
			if(strcmp((const char *)PdpType,(const char *)aPdpType[i]) == 0)
			{
				val = 1;
				break;
			}
			
		}
		if(val == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;

		}
		
		if((strlen((const char *)aucApnValue)>100) || (strlen((const char *)aucUserName)>16) || (strlen((const char *)aucPassword)>16))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		sprintf(at_QCGDEFCONT, "AT+CGDCONT=0,\"%s\",\"%s\"\r\n", PdpType,aucApnValue);
		if(XY_OK != xy_atc_interface_call(at_QCGDEFCONT, NULL, (void*)NULL))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(strlen((const char *)aucUserName) != 0 || strlen((const char *)aucPassword) != 0)
		{
			if (strlen((const char *)aucUserName) == 0 && strlen((const char *)aucPassword) != 0)
			{
				sprintf(at_QCGDEFCONT, "AT+CGAUTH=0,2,\"%s\"\r\n",aucPassword);
			}
			else if (strlen((const char *)aucUserName) != 0 && strlen((const char *)aucPassword) == 0)
			{
				sprintf(at_QCGDEFCONT, "AT+CGAUTH=0,2,\"%s\"\r\n",aucUserName);
			}
			else if (strlen((const char *)aucUserName) != 0 && strlen((const char *)aucPassword) != 0)
			{
				sprintf(at_QCGDEFCONT, "AT+CGAUTH=0,2,\"%s\",\"%s\"\r\n", aucUserName,aucPassword);
			}
			
			if(XY_OK != xy_atc_interface_call(at_QCGDEFCONT, NULL, (void*)NULL))
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				return AT_END;
			}
		}
		
		return AT_END;
		
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = (char*)xy_zalloc(200);
		ATC_MSG_CGDCONT_R_CNF_STRU* CGDCONT_R_info = (ATC_MSG_CGDCONT_R_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_CGDCONT_R_CNF_STRU));
		
		ATC_MSG_CGAUTH_R_CNF_STRU* CGAUTH_R_info = (ATC_MSG_CGAUTH_R_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_CGAUTH_R_CNF_STRU)); 
	
	    if(XY_OK != xy_atc_interface_call("AT+CGDCONT?\r\n", (func_AppInterfaceCallback)NULL, (void*)CGDCONT_R_info))
	    {
	        xy_free(CGDCONT_R_info);
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
	    }
		if(XY_OK != xy_atc_interface_call("AT+CGAUTH?\r\n", (func_AppInterfaceCallback)NULL, (void*)CGAUTH_R_info))
		{
			xy_free(CGAUTH_R_info);
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		if(strlen((const char *)CGAUTH_R_info->stCgauth[0].aucUserName) == 0 && strlen((const char *)CGAUTH_R_info->stCgauth[0].aucPassword) == 0)
		{
#if VER_QUCTL260 //MG 20221116 add by LGF			
			ATC_MSG_CGCONTRDP_CNF_STRU Disguiser = { 0 };
			xy_atc_interface_call("AT+CGCONTRDP=0\r\n", NULL, (void*)&Disguiser);
			snprintf(*prsp_cmd, 200, "\r\n+QCGDEFCONT: \"%s\",\"%s\"\r\n\r\nOK\r\n",aucPdpType[CGDCONT_R_info->stPdpContext[0].ucPdpType],Disguiser.stPara.aucPdpDynamicInfo[0].aucApn);
#else
			snprintf(*prsp_cmd, 200, "\r\n+QCGDEFCONT:\"%s\",\"%s\"\r\n\r\nOK\r\n",aucPdpType[CGDCONT_R_info->stPdpContext[0].ucPdpType],CGDCONT_R_info->stPdpContext[0].aucApnValue);
#endif

		}
		else if(strlen((const char *)CGAUTH_R_info->stCgauth[0].aucUserName) == 0 && strlen((const char *)CGAUTH_R_info->stCgauth[0].aucPassword) != 0)
		{
			snprintf(*prsp_cmd, 200, "\r\n+QCGDEFCONT: \"%s\",\"%s\",\"%s\"\r\n\r\nOK\r\n",aucPdpType[CGDCONT_R_info->stPdpContext[0].ucPdpType],CGDCONT_R_info->stPdpContext[0].aucApnValue,
																							CGAUTH_R_info->stCgauth[0].aucPassword);
		}
		else if(strlen((const char *)CGAUTH_R_info->stCgauth[0].aucUserName) != 0 && strlen((const char *)CGAUTH_R_info->stCgauth[0].aucPassword) == 0)
		{
			snprintf(*prsp_cmd, 200, "\r\n+QCGDEFCONT: \"%s\",\"%s\",\"%s\"\r\n\r\nOK\r\n",aucPdpType[CGDCONT_R_info->stPdpContext[0].ucPdpType],CGDCONT_R_info->stPdpContext[0].aucApnValue,
																							CGAUTH_R_info->stCgauth[0].aucUserName);
		}
		else if(strlen((const char *)CGAUTH_R_info->stCgauth[0].aucUserName) != 0 && strlen((const char *)CGAUTH_R_info->stCgauth[0].aucPassword) != 0)
		{
			snprintf(*prsp_cmd, 200, "\r\n+QCGDEFCONT: \"%s\",\"%s\",\"%s\",\"%s\"\r\n\r\nOK\r\n",aucPdpType[CGDCONT_R_info->stPdpContext[0].ucPdpType],CGDCONT_R_info->stPdpContext[0].aucApnValue,
																							CGAUTH_R_info->stCgauth[0].aucUserName,CGAUTH_R_info->stCgauth[0].aucPassword);
		}

	    xy_free(CGDCONT_R_info);
		xy_free(CGAUTH_R_info);	
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(60);
		snprintf(*prsp_cmd, 60, "\r\n+QCGDEFCONT: (\"IP\",\"IPV6\",\"IPV4V6\",\"PPP\",\"Non-IP\")\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}

uint8_t* getNewBuildInfo(void)
{
    static uint8_t date_origin_format_buf[9] = {0};       
    int month = 0;              
    int i = 0;            
    static const char *static_month_buf[] = 
    {
        "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
     };
     const uint8_t *cp_date =(uint8_t *) __DATE__;   // get origin format :month date year
     for (i = 0; i < 4; i++) 
     {
         date_origin_format_buf[i] = *(cp_date + 7 + i);
      }
      for(i = 0; i < 12; i++) 
      {             
        if((static_month_buf[i][0] == (cp_date[0])) &&
           (static_month_buf[i][1] == (cp_date[1])) &&
           (static_month_buf[i][2] == (cp_date[2]))) {
            month = i+1;
            break;
        }
      }        
        date_origin_format_buf[4] = month / 10 % 10 + '0';
        date_origin_format_buf[5] = month % 10 + '0';
             
        if (cp_date[4] == ' ') {
            date_origin_format_buf[6] = '0';
        } else {
            date_origin_format_buf[6] = cp_date[4];
        }
        date_origin_format_buf[7] = cp_date[5];

   // sprintf(outInfo,"%s %s",VERSION_INFO_NEW,date_origin_format_buf);
   //  sprintf(outInfo,"%s",date_origin_format_buf);
    return date_origin_format_buf;
}

extern void  write_user_nv_demo();
extern void  read_user_nv_demo();
user_nv_data_t *g_user_nv = NULL;

int at_QATWAKEUP_req(char *at_buf, char **prsp_cmd)
{
	int8_t wakeup_mode;

	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%1d", at_buf, &wakeup_mode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		g_user_nv->atWakeup = wakeup_mode;
		xy_printf("g_user_nv->atWakeup0000=%d",g_user_nv->atWakeup);
		write_user_nv_demo();

	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(32);
		read_user_nv_demo();
		sprintf(*prsp_cmd, "\r\n+QATWAKEUP:%1d\r\n\r\nOK\r\n", g_user_nv->atWakeup);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd, "\r\n+QATWAKEUP:(0,1)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}

//20230404 MG add for control EDRX mode "ENTER/EXIT DEEPSLEEP" URC
int at_MGEDRXRPT_rep(char *at_buf, char **prsp_cmd)
{
    if(AT_CMD_TEST == g_req_type)
    {
		*prsp_cmd = xy_zalloc(32);
		
		sprintf(*prsp_cmd, "\r\n+MGEDRXRPT: (0,1)\r\n\r\nOK\r\n");
    }
	
    else if(AT_CMD_QUERY == g_req_type)
    {
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd, "\r\n+MGEDRXRPT: %d\r\n\r\nOK\r\n", g_softap_fac_nv->mgedrxrpt_ind);
    }

	
    else if(AT_CMD_REQ == g_req_type)
    {
        int8_t mgedrxrpt_mode = -1;
		
		if (at_parse_param("%1d(0-1)", at_buf, &mgedrxrpt_mode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(mgedrxrpt_mode != g_softap_fac_nv->mgedrxrpt_ind)
		{
			g_softap_fac_nv->mgedrxrpt_ind = mgedrxrpt_mode;
			SAVE_FAC_PARAM(mgedrxrpt_ind);
		}
    }

	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}
//add end


unsigned int NIDD_BUFFER_ED = 0 ;
char *NIDD_STORAGE = NULL;
#define MAX_NIDD_BUFFER 1000*100

#include "xy_flash.h"
extern void xy_get_OTA_flash_info(uint32_t *addr, int *len);
void NIDD_buffer_resume()
{
	if(g_softap_fac_nv->nidd_rpt_mode != 2)
	{
		return;
	}
	int ret = -1;
	uint32_t var1;
	int var2;
	uint32_t var3 = 0;
	uint32_t var4 = 0;
	xy_get_OTA_flash_info(&var1,&var2);
	xy_printf("NIDD_ob_OTA_flash_addr[%p]size[%d]", var1, var2);
	ret = xy_flash_read(var1,&var4,4);
	if(ret != 0 || var4 == 0)
	{
		xy_printf("error:[%d]ret[%d]rec[%d]", __LINE__, ret, var4);
		return;
	}
	if(NIDD_STORAGE != NULL)
	{
		xy_printf("error:[%d]ret[%d]rec[%d]", __LINE__, ret, var4);
		return;
	}
	NIDD_BUFFER_ED = var4;
	NIDD_STORAGE = xy_zalloc(MAX_NIDD_BUFFER);
	ret = xy_flash_read(var1 + 4,NIDD_STORAGE,NIDD_BUFFER_ED);
	if(ret != 0)
	{
		xy_printf("error:[%d]ret[%d]rec[%d]", __LINE__, ret, NIDD_BUFFER_ED);
		return;
	}
	xy_flash_write(var1,&var3,4);
	if(ret != 0)
	{
		xy_printf("error:[%d]ret[%d]rec[%d]", __LINE__, ret, NIDD_BUFFER_ED);
		return;
	}
	xy_printf("[%s][%d]success\r\n", __func__, NIDD_BUFFER_ED);
}

void NIDD_buffer_flash()
{
	if(g_softap_fac_nv->nidd_rpt_mode != 2 || NIDD_BUFFER_ED == 0)
	{
		return;
	}
	int ret = -1;
	uint32_t var1;
	int var2;
	xy_get_OTA_flash_info(&var1,&var2);
	ret = xy_flash_write(var1,&NIDD_BUFFER_ED,4);
	if(ret != 0)
	{
		// xy_printf("error:[%d]ret[%d]rec[%d]", __LINE__, ret, NIDD_BUFFER_ED);
		return;
	}
	ret = xy_flash_write(var1 + 4,NIDD_STORAGE,NIDD_BUFFER_ED);
	if(ret != 0)
	{
		// xy_printf("error:[%d]ret[%d]rec[%d]", __LINE__, ret, NIDD_BUFFER_ED);
		return;
	}
	// xy_printf("[%s][%d]success\r\n", __func__, NIDD_BUFFER_ED);
}

void NIDD_buffer_write(char *nidd_data, unsigned int data_len)
{
	xy_printf("[%s][%d]data_len[%d]", __func__, __LINE__, data_len);
	if(data_len + NIDD_BUFFER_ED > MAX_NIDD_BUFFER)
	{
		// tbc : send to UART
		char *var1 = xy_zalloc(data_len * 2 + 1);
		char *var2 = xy_zalloc(data_len * 2 + 40);
		bytes2hexstr((unsigned char *)nidd_data,data_len,var1,data_len * 2 + 1);
		sprintf(var2,"\r\n+QNIDD:4,0,%d,0,%s\r\n", data_len,var1);
		send_urc_to_ext(var2);
		xy_free(var1);
		var1=NULL;
		xy_free(var2);
		var2=NULL;
	}
	else
	{
		// tbc : downlink URC notification
		char *con_mid = xy_zalloc(40);
		sprintf(con_mid,"\r\n+QNIDD:4,0,%d\r\n", data_len);
		send_urc_to_ext(con_mid);
		xy_free(con_mid);
		con_mid=NULL;

		NIDD_buffer_resume();

		if((NIDD_STORAGE == NULL) && (NIDD_BUFFER_ED == 0))
		{
			NIDD_STORAGE = xy_zalloc(MAX_NIDD_BUFFER);
			memcpy(NIDD_STORAGE,nidd_data,data_len);
			NIDD_BUFFER_ED += data_len;
		}
		else
		{
			memcpy(NIDD_STORAGE + NIDD_BUFFER_ED, nidd_data, data_len);
			NIDD_BUFFER_ED += data_len;
		}
	}
}

void NIDD_buffer_read(unsigned int data_read, char **prsp)
{
	xy_printf("[%s][%d]data_read[%d]bufferd[%d]", __func__, __LINE__, data_read, NIDD_BUFFER_ED);
	if(NIDD_BUFFER_ED == 0)
	{
		return;
	}
	data_read = (data_read > NIDD_BUFFER_ED) ? NIDD_BUFFER_ED : data_read ;
	char *var1 = xy_zalloc(data_read * 2 + 1); 
	*prsp = xy_zalloc(data_read * 2 + 40); 
	bytes2hexstr((unsigned char *)NIDD_STORAGE,data_read,var1,data_read * 2 + 1);

	// tbc : AT read response
	
	sprintf(*prsp,"\r\n+QNIDD:5,%d,%s\r\n\r\nOK\r\n", data_read,var1);
	xy_free(var1);
	var1 = NULL;
	NIDD_BUFFER_ED -= data_read;
	memmove(NIDD_STORAGE, NIDD_STORAGE + data_read, NIDD_BUFFER_ED);
}
int at_130G_NIDD_req(char *at_buf, char **prsp_cmd)
{
	if( g_req_type == AT_CMD_REQ)
	{
		int var1;
		if (at_parse_param("%d", at_buf, &var1) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		switch (var1)
		{
			case 5:
			{
				int var2 = 0;
				if (at_parse_param(",%d", at_buf, &var2) != AT_OK)
				{
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
					return AT_END;
				}
				NIDD_buffer_resume();
				NIDD_buffer_read(var2,prsp_cmd);
				break;
			}
			case 6:
			{
				NIDD_buffer_resume();
				*prsp_cmd = xy_zalloc(40);
				sprintf(*prsp_cmd, "\r\n+QNIDD:6,%d\r\n\r\nOK\r\n",NIDD_BUFFER_ED);
				break;
			}
			default:
				return AT_FORWARD;	
		}
		return AT_END;
	}
	return AT_FORWARD;
}

// AT+NIDDCTL
int at_NIDD_urc_req(char *at_buf, char **prsp_cmd)
{
	switch(g_req_type)
	{
		case AT_CMD_QUERY:
		{
			*prsp_cmd = xy_zalloc(66);
			sprintf(*prsp_cmd, "\r\n+NIDDCTL:%d\r\n\r\nOK\r\n", g_softap_fac_nv->nidd_rpt_mode);
			break;
		}
		case AT_CMD_TEST:
		{
			*prsp_cmd = xy_zalloc(66);
			sprintf(*prsp_cmd, "\r\n+NIDDCTL: (0-2)\r\n\r\nOK\r\n");
			break;
		}	
		case AT_CMD_REQ:
		{
			int var1 = -1;
			if(at_parse_param("%d(0-2)", at_buf, &var1) != AT_OK)
			 {
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				break;
			 }
			g_softap_fac_nv->nidd_rpt_mode = var1;
			SAVE_FAC_PARAM(nidd_rpt_mode);
			break;
		}
		default:
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			break;
		}
	}
	return AT_END;
}


