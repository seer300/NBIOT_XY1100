/** 
* @file        main.c
* @ingroup     main
* @brief       主入口，用户在对应的接口中进行外设和私有应用任务的初始化，但不得长时间阻塞！
*/
/**
 * @defgroup main main
 */




#include "osal.h"
#include "xinyi_hardware.h"
#include "sys_init.h"
#include "factory_nv.h"
#include "utc.h"
#include "rtc_tmr.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "xy_mem.h"
#include "xy_utils.h"
#if ABUP_FOTA
#include "abup_fota.h"
#endif

#include "xy_demo.h"



extern void user_led_demo_init(void);

extern void hal_gpio_out_work_task_init();



/** 
* @brief   用户外设总线等初始化入口，内部不能执行flash的擦写接口
*/
void user_peripheral_init()
{

	if(*(uint32_t*)(BACKUP_MEM_RF_MODE) != 0)
		return;
	
#if 1//DEMO_TEST	
	if((g_softap_fac_nv != NULL) && (!(g_softap_fac_nv->peri_remap_enable & 0x0100)) && (g_softap_fac_nv->led_pin <= HAL_GPIO_PIN_NUM_63))
		{
				
			user_led_demo_init();
		}
#endif
	
}

/** 
* @brief   用户任务线程初始化入口，内部不能执行flash的擦写接口
*/
void user_task_init()
{
    int i ;

	if(*(uint32_t*)(BACKUP_MEM_RF_MODE) != 0)
		return;

	hal_gpio_out_work_task_init();
	
#if DEMO_TEST
 	if(g_softap_fac_nv->demotest > 0)
 	{
 	    for (i = 0;i <g_demo_proc_list_len; i++)
 	    {
 	        if((g_demo_proc_list + i)->demo_num == g_softap_fac_nv->demotest)
 	        {
               (g_demo_proc_list + i)->proc();
               return;
 	        }
 	        else
 	            continue;
 	    }
 	    send_debug_str_to_at_uart("+DBGINFO:[CONFIG ERROR] Error demotest num\r\n");
 	}

#endif


}


/** 
* @brief   用户不得在该接口中修改任何代码
*/

int main(void)
{
	HWREG(PS_START_SYNC) = 0;
	HWREG(SYS_UP_REASON) = 0;
	HWREG(DSP_LPM_STATE) = DSP_NOT_LOADED;
	HWREG(ARM_STANDBY_ENTER_XIP) = 0;
	HWREG(FLASH_DYN_SYNC) = 0x01; 
	HWREG(FLASH_NOTICE_FLAG) = 0x01;
	HWREG(AP_ADC_FLAG) = 0;
	HWREG(CP_ADC_FLAG) = 0;
	Set_AP_ALARM_RAM(RTC_ALARM_INVALID);
	Set_CP_ALARM_RAM(RTC_ALARM_INVALID);
	NVIC_SetVectorTable(g_pfnVectors);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);

	//MPU_config();

	osKernelInitialize();
#ifdef TRACEALYZER_ENABLE
	vTraceEnable(TRC_START);
#endif
#if EXT_MEM_TRACE
	extMemTraceInit();
#endif

	system_init();

	user_peripheral_init();

	sys_app_init();

	user_task_init();
	
	xy_srand((uint32_t)(HWREG(UTC_BASE + UTC_CLK_CNT)+HWREG(SYS_UP_REASON+32-4)));
	
	osKernelStart();

	while(1);

	return 0;
}
