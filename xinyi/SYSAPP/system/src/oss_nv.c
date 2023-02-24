#include "flash_adapt.h"
#include "flash_vendor.h"
#include "at_context.h"
#include "osal_statistics.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "rtc_tmr.h"
#include "xy_flash.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "at_global_def.h"
#include "low_power.h"
#include "xy_sys_hook.h"
#include "net_app_resume.h"
#include "at_com.h"
#include "at_uart.h"
#include "dsp_process.h"
#include "sys_init.h"
#include "adc.h"
#include "xy_ps_api.h"
#include "ps_netif_api.h"
#if CTWING_VER
#include "xy_ctwing_adapt.h"
#endif

#if VER_QUCTL260 && TELECOM_VER
#include "agent_tiny_demo.h"
#endif

//softap nv include RTC info,can not erase
softap_var_nv_t *g_softap_var_nv = NULL;

softap_fac_nv_t *g_softap_fac_nv = NULL;
void *g_invar_nv=NULL;        //DSP notify to M3,DSP core invar nv RAM address
int   g_invar_len=0;
int g_dsp_mode_dbg = 0; //see print_dsp_dbg
volatile unsigned int g_current_baudrate = 0;
volatile unsigned char g_BKMEMCTL_reg = 0x11;
/*若OFFTIME模式，必须等到输入AT+OFFTIME后才能正常处理其它AT命令*/
char g_need_offtime = 0;

#define  varnv_is_need_wrtieback()    (g_softap_fac_nv->keep_retension_mem==0 || g_softap_fac_nv->offtime==1 || \
		(g_softap_var_nv->next_PS_sec!=0&&g_softap_fac_nv->backup_threshold!=0&&g_softap_var_nv->next_PS_sec-(global_timepoint_get(0)/1000)>g_softap_fac_nv->backup_threshold))


extern  int  g_first_poweron;

//only save  DSP factory NV,and softap NV save by SAVE_FAC_PARAM
void save_factory_nv()
{
	//仅DSP核的出厂NV在深睡或重启时保存，以解决实时写可能存在的脏数据问题
	//xy_ftl_write内部会将头部魔数偏移掉，这里的长度也需要减去魔数
	if(xy_ftl_write(NV_FLASH_FACTORY_BASE,0,(unsigned char *)RAM_FACTORY_NV_BASE,DSP_FAC_LEN-4) != XY_OK)
	{
		xy_flash_erase(NV_FLASH_FACTORY_BASE,NV_FLASH_FACTORY_LEN);
		xy_assert(0);
	}

	//若没有工作态出厂NV，则写入工作态出厂NV，以满足后台修改出厂NV参数的需求
	if(g_first_poweron == 1)
	{
		if(xy_ftl_write(NV_FLASH_FACTORY_BASE,DSP_FAC_LEN-4,(unsigned char *)g_softap_fac_nv,sizeof(softap_fac_nv_t)) != XY_OK) 
		{  
			xy_flash_erase(NV_FLASH_FACTORY_BASE,NV_FLASH_FACTORY_LEN);  
			xy_assert(0); 
		} 
	}
}


//do before reboot,save nv by param
void reboot_nv()
{
	//if DSP have sleep,or need AT+OFFTIME,or BUSY,not shark with DSP
	if (*(uint32_t*)(BACKUP_MEM_RF_MODE) == 0 && !DSP_IS_NOT_LOADED() && !DSP_IS_DEEPSLEEP() && !(nearps_ctx.state & SEND_REQ_NOW))
	{
		xy_atc_interface_call("AT+CFUN=5\r\n", NULL, (void*)NULL);
	}
	osCoreEnterCritical();

	save_factory_nv();

	g_softap_var_nv->next_PS_sec = 0;

	memset((void *)&ap_rtc_event_arry[RTC_TIMER_LPM],0,sizeof(struct rtc_event_info));

	save_volatile_nv(1);
}

//0,AT+RESET;1,assert or watchdog,2,PIN  reset
void  erase_flash_by_abnormal(int state)
{
	//always can not erase factory working NV
	if(state == 0)
		xy_flash_erase(NV_FLASH_FACTORY_BASE,NV_FLASH_FACTORY_LEN);

	xy_flash_erase(NV_FLASH_DSP_VOLATILE_BASE,NV_FLASH_DSP_VOLATILE_LEN);
	xy_flash_erase(NV_FLASH_DSP_NON_VOLATILE_BASE,NV_FLASH_DSP_NON_VOLATILE_LEN);
	xy_flash_erase(NV_FLASH_NET_BASE,NV_FLASH_NET_LEN);
	g_softap_var_nv->app_recovery_flag = 0;//清除云恢复标志位
#if VER_QUCTL260 && TELECOM_VER
	//清除下行缓存
	xy_flash_erase(FOTA_BACKUP_BASE, DOWNSTREAM_LEN_MAX*8);
#endif
}

void  init_DSP_external_POOL()
{
	uint32_t* dsp_ext_heap_addr=NULL;
	uint32_t* dsp_log_addr=NULL;

	if(g_softap_fac_nv->dsp_ext_pool != 0)
	{
		dsp_ext_heap_addr = xy_malloc_Align(g_softap_fac_nv->dsp_ext_pool * 1024);

		*((uint32_t *)DSP_EXT_POOL) = get_Dsp_Addr_from_ARM((unsigned int)(dsp_ext_heap_addr));
		*((uint32_t *)(DSP_EXT_POOL+4)) = g_softap_fac_nv->dsp_ext_pool * 1024;
	}
	else
	{
		*((uint32_t *)DSP_EXT_POOL) = 0;
		*((uint32_t *)(DSP_EXT_POOL+4)) = 0;

	}
	
	if(g_softap_fac_nv->open_log != 0)
	{
		if(g_softap_fac_nv->log_size != 0)
		{
			dsp_log_addr =  xy_malloc_Align(g_softap_fac_nv->log_size * 1024);

			*((uint32_t *)(DSP_EXT_POOL+8)) = get_Dsp_Addr_from_ARM((unsigned int)(dsp_log_addr));
			*((uint32_t *)(DSP_EXT_POOL+12)) = g_softap_fac_nv->log_size * 1024;
		}
		else
		{
			dsp_log_addr =  xy_malloc_Align(20 * 1024);

			*((uint32_t *)(DSP_EXT_POOL+8)) = get_Dsp_Addr_from_ARM((unsigned int)(dsp_log_addr));
			*((uint32_t *)(DSP_EXT_POOL+12)) = 20480;
		}
	}
	else
	{
		*((uint32_t *)(DSP_EXT_POOL+8)) = 0;
		*((uint32_t *)(DSP_EXT_POOL+12)) = 0;
	}	
}

int restore_var_nv()
{
	if(g_softap_fac_nv->keep_retension_mem==1 && get_sys_up_stat()!=POWER_ON)
	{
		//retension mem is valid
		if(*((unsigned int*)(BACKUP_MEM_CHECKSUM)) == xy_chksum((void*)RAM_NV_VOLATILE_SOFTAP_START,RAM_NV_VOLATILE_SOFTAP_LEN))
		{
			//only M3 core work,PS/PHY/LPM retension mem use to save user data,when dynamic_load_up_dsp,will restore DSP var nv 
			if(*((unsigned long*)(BACKUP_MEM_FLAG)) == BK_FLAG_INUSE_2)
			{
				g_dsp_mode_dbg|=1<<14;
				memcpy(g_user_vol_data,(const void *)(BACKUP_MEM_BASE),USER_VOL_DATA_LEN);
			}
			//retension memory save PS/PHY/LPM/SOFTAP volatile NV
			else if(*((unsigned long*)(BACKUP_MEM_FLAG)) != BK_FLAG_INUSE)
				goto ABNORMAL_PROC;
		}
		//dirty or first on
		else 
		{
ABNORMAL_PROC:
			//maybe write back to flash
			if(g_softap_fac_nv->backup_threshold!=0 || g_softap_fac_nv->high_freq_data==1)
				goto READ_FLASH;
			else
			{
				g_dsp_mode_dbg|=1<<6;
				memset((void *)BACKUP_MEM_BASE,0,FLASH_SECTOR_LENGTH);
				return XY_ERR;
			}
		}
	}
	else
	{
READ_FLASH:
		if(xy_ftl_read(NV_FLASH_DSP_VOLATILE_BASE,(unsigned char *)BACKUP_MEM_BASE,XY_FTL_AVAILABLE_SIZE)!= XY_OK)
        {
        	g_dsp_mode_dbg|=1<<11;
			memset((void *)BACKUP_MEM_BASE,0,FLASH_SECTOR_LENGTH);
			return XY_ERR;
        }
	}
	return XY_OK;
}

void nv_restore()
{
	otp_valid_flag = FLASH_OTP_CAL_Read_and_Check();

	xy_assert((sizeof(T_PsNvInfo)%CACHE_LINE) == 0);
	xy_assert((sizeof(rf_dciq_t)%CACHE_LINE) == 0);
	xy_assert(sizeof(factory_nv_t) <= PAGE_AVAILABLE_SIZE);
	xy_assert(RAM_NV_VOLATILE_PS_LEN+RAM_NV_VOLATILE_PHY_LEN+RAM_NV_VOLATILE_LPM_LEN+RAM_NV_VOLATILE_SOFTAP_LEN+RAM_NV_VOLATILE_RTCLIST_LEN <= PAGE_AVAILABLE_SIZE);
	xy_assert(sizeof(softap_var_nv_t) <= RAM_NV_VOLATILE_SOFTAP_LEN);

	g_softap_fac_nv = (softap_fac_nv_t *)xy_malloc(sizeof(softap_fac_nv_t));	
	g_softap_var_nv = (softap_var_nv_t *)RAM_NV_VOLATILE_SOFTAP_START;

	ap_rtc_event_arry = ((rtclist_var_nv_t *)RAM_NV_VOLATILE_RTCLIST_START)->ap_rtc_list;
	cp_rtc_event_arry = ((rtclist_var_nv_t *)RAM_NV_VOLATILE_RTCLIST_START)->cp_rtc_list;
	
	xy_nv_ftl_init();

	//if working factory have erased,will copy from main factory NV
	xy_ftl_read(NV_FLASH_FACTORY_BASE,(unsigned char *)RAM_FACTORY_NV_BASE,XY_FTL_AVAILABLE_SIZE);
	
	//factory nv include DSP and ARM
	memcpy(g_softap_fac_nv,(void *)(SOFTAP_FAC_RAM_BASE),sizeof(softap_fac_nv_t));

	xy_reset_fac_nv_hook();
	
	//utc_init();

	init_DSP_external_POOL();

	AutoBaudStart();
	
	if((g_softap_fac_nv->uart_rate & 0x1ff) != 0)
	{
		g_current_baudrate = 2400 * (g_softap_fac_nv->uart_rate & 0x1ff);
	}
	else
	{
		g_current_baudrate = g_autobaud_rate;
	}
	
	HWREG(AT_UART_BAUD) = g_current_baudrate;
	
	measure_rc32k();

	if(g_softap_fac_nv->deepsleep_enable == 0)
	{
		HAVE_FREE_LOCK = 0;
	}
	//if PS rtc timeout, reset HAVE_FREE_LOCK  in psm_TAU_timeout_cb
	else if(!CHECK_SDK_TYPE(OPERATION_VER))
	{
		if (g_softap_fac_nv->work_mode!=0 || g_softap_fac_nv->deepsleep_delay==0)
			HAVE_FREE_LOCK = 0;
		else
			HAVE_FREE_LOCK = 1;
	}
	else
	{
		if(get_sys_up_stat()==EXTPIN_WAKEUP)
			HAVE_FREE_LOCK = 0;
		else
			HAVE_FREE_LOCK = 1;
	}

	//restore user volatile data from retension memory,but g_softap_fac_nv(RTC_list) still valid,can't write back to flash 
	if(g_softap_fac_nv->high_freq_data==1)
	{
		xy_assert(g_softap_fac_nv->keep_retension_mem == 1);
		
		g_user_vol_data = xy_zalloc(USER_VOL_DATA_LEN);
	}

	if(get_sys_up_stat()==WDT_RESET || get_sys_up_stat()==SOFT_RESET || get_sys_up_stat()==PIN_RESET)
	{
		g_dsp_mode_dbg|=1<<9;

		xy_flash_erase(NV_FLASH_NET_BASE,NV_FLASH_NET_LEN);
		g_softap_var_nv->app_recovery_flag = 0;//清除云恢复标志位
#if VER_QUCTL260 && TELECOM_VER
		//清除下行缓存
		xy_flash_erase(FOTA_BACKUP_BASE, DOWNSTREAM_LEN_MAX*8);
#endif
		
		if(get_sys_up_stat()!=SOFT_RESET || get_softreset_type()==RB_BY_ASSERT)
		{
			xy_flash_erase(NV_FLASH_DSP_VOLATILE_BASE,NV_FLASH_DSP_VOLATILE_LEN);
			xy_flash_erase(NV_FLASH_DSP_NON_VOLATILE_BASE,NV_FLASH_DSP_NON_VOLATILE_LEN);
		}
		
		//软重启时需要保证尾部4字节的rf_mode不能被清除
		if(get_sys_up_stat()==SOFT_RESET && get_softreset_type()==RB_BY_NRB)
		{
			//软重启时，dm_cfg仍有效
			void *dm_ctl_cpy = xy_malloc(sizeof(dm_cfg_t));
			memcpy(dm_ctl_cpy,&g_softap_var_nv->dm_cfg,sizeof(dm_cfg_t));
			memset((void *)BACKUP_MEM_BASE,0,FLASH_SECTOR_LENGTH - 4);
			memcpy(&g_softap_var_nv->dm_cfg,dm_ctl_cpy,sizeof(dm_cfg_t));
			xy_free(dm_ctl_cpy);
		}
		else
		{
			memset((void *)BACKUP_MEM_BASE,0,FLASH_SECTOR_LENGTH);
		}
	}
	//POWER_ON  EXTPIN_WAKEUP  UTC_WAKEUP
	else
	{
		restore_var_nv();
		if(get_sys_up_stat() == POWER_ON)
		{
			xy_flash_erase(NV_FLASH_NET_BASE,NV_FLASH_NET_LEN);
			g_softap_var_nv->app_recovery_flag = 0;//清除云恢复标志位
#if VER_QUCTL260 && TELECOM_VER
			//清除下行缓存
			xy_flash_erase(FOTA_BACKUP_BASE, DOWNSTREAM_LEN_MAX*8);
#endif
			if(g_softap_fac_nv->offtime == 1)
			{
				g_need_offtime = 1;
				memset((void *)RAM_NV_VOLATILE_RTCLIST_START,0,RAM_NV_VOLATILE_RTCLIST_LEN);
			}
			else
			{
				//DM要求断电再上电，dm_cfg仍有效，此处为恢复临时备份
				void *dm_ctl_cpy = xy_malloc(sizeof(dm_cfg_t));
				memcpy(dm_ctl_cpy,&g_softap_var_nv->dm_cfg,sizeof(dm_cfg_t));
				
				memset((void *)BACKUP_MEM_BASE,0,FLASH_SECTOR_LENGTH);
				
				memcpy(&g_softap_var_nv->dm_cfg,dm_ctl_cpy,sizeof(dm_cfg_t));
				xy_free(dm_ctl_cpy);
			}
		}
		else
		{
			restore_RTC_list();
		}
	}

	if(get_sys_up_stat() != UTC_WAKEUP && get_sys_up_stat() != EXTPIN_WAKEUP)
	{
		g_softap_var_nv->user_lock_state = 1;
		g_softap_var_nv->sleep_mode = 1;
	}

	if(g_softap_fac_nv->work_mode!=0 || g_need_offtime == 1)
	{
		set_dsp_no_work();
	}
	else
	{
		set_DSP_work_mode();
	}
}

//write user ram data to retension memory,and if PS var nv is valid,must write back to flash
void save_user_ram_to_retension_mem()
{
	//write NB val NV back flash,and will restore when wakeup from deepsleep
	if (!DSP_IS_NOT_LOADED())
	{
		if(xy_ftl_write(NV_FLASH_DSP_VOLATILE_BASE,0,((unsigned char *)BACKUP_MEM_BASE),XY_FTL_AVAILABLE_SIZE)!=XY_OK)
		{
			xy_assert(0);
		}
	}
	//user data cover PS var NV in retension memory
	if(g_user_vol_data!=NULL)
	{
		memcpy((void *)(BACKUP_MEM_BASE),g_user_vol_data,USER_VOL_DATA_LEN);
	
		*((unsigned int*)(BACKUP_MEM_CHECKSUM)) = xy_chksum((void*)RAM_NV_VOLATILE_SOFTAP_START,(uint32_t)RAM_NV_VOLATILE_SOFTAP_LEN);
		*((unsigned long*)(BACKUP_MEM_FLAG)) = BK_FLAG_INUSE_2;
	}
	else
	{		
		*((unsigned int*)(BACKUP_MEM_CHECKSUM)) = xy_chksum((void*)RAM_NV_VOLATILE_SOFTAP_START,(uint32_t)RAM_NV_VOLATILE_SOFTAP_LEN);
		*((unsigned long*)(BACKUP_MEM_FLAG)) = BK_FLAG_INUSE;
	}
}

void save_volatile_nv(int reBoot)
{	
	if(g_softap_fac_nv->offtime==1) 
	{
		g_softap_var_nv->rtc_snap_msec =  rtc_get_global_byoffset(0);
	}

	//write user volatile data to retension memory,can not cover  g_softap_var_nv
	if(g_softap_fac_nv->high_freq_data==1)
	{
		save_user_ram_to_retension_mem();
		if(reBoot==0)
			g_BKMEMCTL_reg = 0x41;
			//HWREGB(0xA0058008) = 0x01; 
	}	
	//volatile NV ,must write last,because update some flag by other NV
	else if(varnv_is_need_wrtieback())
	{
		if(xy_ftl_write(NV_FLASH_DSP_VOLATILE_BASE,0,((unsigned char *)BACKUP_MEM_BASE),XY_FTL_AVAILABLE_SIZE)!=XY_OK)
		{
			xy_assert(0);
		}

		//set backup flag dirty,and must read from flash next wakeup
		*((unsigned int*)(BACKUP_MEM_CHECKSUM)) += 1;
		*((unsigned long*)(BACKUP_MEM_FLAG)) = BK_FLAG_INVALID;
		
		if(reBoot==0)
			g_BKMEMCTL_reg &= 0xFE;
			//HWREGB(0xA0058008) &= 0xFE; 	
	}
	else
	{
		//HWREGB(0xA0058005) = 0x00;		
		*((unsigned int*)(BACKUP_MEM_CHECKSUM)) = xy_chksum((void*)RAM_NV_VOLATILE_SOFTAP_START,(uint32_t)RAM_NV_VOLATILE_SOFTAP_LEN);
		*((unsigned long*)(BACKUP_MEM_FLAG)) = BK_FLAG_INUSE;
		
		if(reBoot==0)
			g_BKMEMCTL_reg = 0x41;
			//HWREGB(0xA0058008) = 0x01; 
	}
}


//save all NV to flash by LPM machine
void nv_write_to_flash()
{
	save_factory_nv();
	if(g_invar_nv!=NULL && g_invar_len!=0)
	{
		if(xy_ftl_write(NV_FLASH_DSP_NON_VOLATILE_BASE,0,(uint8_t *)RAM_INVAR_NV_BASE,g_invar_len) != XY_OK)
		{
			xy_assert(0);
		}
	}
	
	if(g_softap_fac_nv->offtime == 0)
	{
		//must called before save_volatile_nv
	    if(g_fast_off != -1)
	    {
	        //clear  DSP core info
	        memset((void *)(BACKUP_MEM_BASE), 0, DSP_VOLATILE_ALL_LEN);

	        //擦除云NV数据
            xy_flash_erase(NV_FLASH_NET_BASE, NV_FLASH_NET_LEN);
            g_softap_var_nv->app_recovery_flag = 0;//清除云恢复标志位
#if VER_QUCTL260 && TELECOM_VER
			//清除下行缓存
			xy_flash_erase(FOTA_BACKUP_BASE, DOWNSTREAM_LEN_MAX*8);
#endif
	    }
	    else
	    {
	        save_net_app_infos();
			save_ipv6_to_bakmem();

#if VER_QUCTL260 && TELECOM_VER
			cdp_downbuffered_save();
#endif

#if CTWING_VER
	        xy_ctlw_save_date();
#endif
	    }
	}
	
	save_volatile_nv(0);
}

//check softap nv,if conflict,will asssert!
void check_conflict_nv()
{
	static int print_num = 0;
	char *at_str = xy_zalloc(360);

	if (print_num == 0)
		print_num = 1;
	else
	{
		xy_free(at_str);
		return;
	}

	if (g_softap_fac_nv->off_debug == 1)
	{
		if (g_softap_fac_nv->open_log == 1)
			sprintf(at_str + strlen(at_str), "+DBGINFO:NV_ERR:off_debug=%d;open_log=%d;deepsleep_enable=%d\r\n", g_softap_fac_nv->off_debug, g_softap_fac_nv->open_log,g_softap_fac_nv->deepsleep_enable);
	}
	else
	{
		if (g_softap_fac_nv->high_freq_data == 1)
		{
			if (g_softap_fac_nv->work_mode == 0 || g_softap_fac_nv->keep_retension_mem != 1)
				sprintf(at_str + strlen(at_str), "+DBGINFO:NV_ERR:high_freq=%d;workmode=%d;keep_backup=%d\r\n", g_softap_fac_nv->high_freq_data, g_softap_fac_nv->work_mode, g_softap_fac_nv->keep_retension_mem);
		}


		if (g_softap_fac_nv->offtime == 1 && g_softap_fac_nv->keep_retension_mem == 1)
			sprintf(at_str + strlen(at_str), "+DBGINFO:NV_ERR:offtime=%d;keep_backup=%d\r\n", g_softap_fac_nv->offtime, g_softap_fac_nv->keep_retension_mem);

		if (g_softap_fac_nv->deepsleep_enable == 0 && (g_softap_fac_nv->high_freq_data != 0 || g_softap_fac_nv->keep_retension_mem != 1))
			sprintf(at_str + strlen(at_str), "+DBGINFO:NV_ERR:deepsleep_enable=%d;high_freq_data=%d;keep_retension_mem=%d\r\n", g_softap_fac_nv->deepsleep_enable, g_softap_fac_nv->high_freq_data, g_softap_fac_nv->keep_retension_mem);

		if (g_softap_fac_nv->deepsleep_enable == 1 && g_softap_fac_nv->g_NPSMR_enable == 1 && g_softap_fac_nv->offtime != 0)
			sprintf(at_str + strlen(at_str), "+DBGINFO:NV_ERR:deepsleep_enable=%d;g_NPSMR_enable=%d,offtime=%d\r\n", g_softap_fac_nv->deepsleep_enable, g_softap_fac_nv->g_NPSMR_enable, g_softap_fac_nv->offtime);
	}
	if (strlen(at_str) > 0 && g_at_ctrl_cbk.at_broadcast_cbk != NULL)
		g_at_ctrl_cbk.at_broadcast_cbk(at_str,strlen(at_str));
	xy_free(at_str);
}

void print_dsp_dbg()
{
	static int print_num = 0;
	char *at_str = xy_zalloc(250);

	softap_printf(USER_LOG, WARN_LOG, "PoweronReason:%d,SoftRstReason:%d",get_sys_up_stat(), get_softreset_type());
	if (g_softap_fac_nv->off_debug == 0)
	{

		if (print_num == 0)
			sprintf(at_str, "\r\n+DBGINFO:FACTORY_NV:VERTYPE:%x,DOWNDATA:%d,BACKUPKEEP:%d,DEEPSLEEP:%d,OFFTIME:%d,WORKMODE:%d\r\n", g_softap_fac_nv->ver_type,
					g_softap_fac_nv->down_data, g_softap_fac_nv->keep_retension_mem, g_softap_fac_nv->deepsleep_enable, g_softap_fac_nv->offtime, g_softap_fac_nv->work_mode);
		else
			sprintf(at_str, "\r\n+DBGINFO:");

		if (g_dsp_mode_dbg & (1 << 0))
			strcat(at_str, "INIT\r\n");

		if (g_dsp_mode_dbg & (1 << 1))
			strcat(at_str, "CAMPON\r\n");

		if (g_dsp_mode_dbg & (1 << 2))
		{
			if (g_softap_var_nv->ps_deepsleep_state == 1)
				strcat(at_str, "TAU\r\n");
			else if (g_softap_var_nv->ps_deepsleep_state == 2)
				strcat(at_str, "eDRX\r\n");
		}

		if (g_dsp_mode_dbg & (1 << 3))
			strcat(at_str, "ATTACH\r\n");

		if (g_dsp_mode_dbg & (1 << 4))
		{
			;
		}

		if (g_dsp_mode_dbg & (1 << 5))
		{
			;
		}

		if (g_dsp_mode_dbg & (1 << 6))
			strcat(at_str, "BACKUP CHECKSUM ERR\r\n");

		if (g_dsp_mode_dbg & (1 << 7))
			strcat(at_str, "FAC NV INIT\r\n");

		if (g_dsp_mode_dbg & (1 << 8))
		{
			;
		}

		if (g_dsp_mode_dbg & (1 << 9))
		{
			if (get_sys_up_stat() == WDT_RESET)
				sprintf(at_str + strlen(at_str), "WATCHDOG RESET\r\n");
			else if (get_sys_up_stat() == PIN_RESET)
				sprintf(at_str + strlen(at_str), "PIN RESET\r\n");
			else if (get_sys_up_stat() == POWER_ON)
				sprintf(at_str + strlen(at_str), "ABNORMAL POWER OFF\r\n");
			else if (get_sys_up_stat() == SOFT_RESET)
				sprintf(at_str + strlen(at_str), "SOFTRESET by %ld\r\n",get_softreset_type());
		}

		if (g_dsp_mode_dbg & (1 << 10))
		{
			;
		}

		if (g_dsp_mode_dbg & (1 << 11))
			strcat(at_str, "variable NV NULL\r\n");

		if (g_dsp_mode_dbg & (1 << 12)) //
		{
			;
		}

		if (g_dsp_mode_dbg & (1 << 13))
			strcat(at_str, "ATTACH DSP ON FIRST\r\n");

		if (g_dsp_mode_dbg & (1 << 14))
			strcat(at_str, "USER DATA IN BACKUP MEM\r\n");

		if (print_num == 0)
		{
			print_num = 1;
			if ((g_softap_fac_nv->work_mode == 1 || g_softap_fac_nv->work_mode == 2) && DSP_IS_NOT_LOADED())
				strcat(at_str, "only M3 work\r\n");

			if (get_sys_up_stat() == UTC_WAKEUP)
				strcat(at_str, "WAKEUP by RTC\r\n");
			else if (get_sys_up_stat() == EXTPIN_WAKEUP)
				strcat(at_str, "WAKEUP by WAKEUP PIN\r\n");
		}

		if(g_at_ctrl_cbk.at_broadcast_cbk != NULL)
			g_at_ctrl_cbk.at_broadcast_cbk(at_str,strlen(at_str));

		g_dsp_mode_dbg = 0;

		check_conflict_nv();
	}

	xy_free(at_str);
}
