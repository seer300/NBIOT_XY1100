#include "osal.h"

#include "print.h"
#include "sys_debug.h"

#include "gpio.h"
#include "factory_nv.h"

#if (configSUPPORT_STATIC_ALLOCATION == 1)

__WEAK void vApplicationGetIdleTaskMemory (StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
	/* Idle task control block and stack */
	static StaticTask_t Idle_TCB;
	static StackType_t  Idle_Stack[configMINIMAL_STACK_SIZE];

	*ppxIdleTaskTCBBuffer   = &Idle_TCB;
	*ppxIdleTaskStackBuffer = &Idle_Stack[0];
	*pulIdleTaskStackSize   = (uint32_t)configMINIMAL_STACK_SIZE;
}

#endif


#if( configCHECK_FOR_STACK_OVERFLOW > 0 )

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	(void) xTask;

	PRINT_BUF("stack overflow in task ");
	PRINT_BUF(pcTaskName);

	Sys_Assert(0);
}

#endif

#if( configUSE_TICK_HOOK == 1 )

void vApplicationTickHook( void )
{
	static uint32_t tickcount=0;

	if((tickcount++) % 500 == 0)
	{
		//toggle led_pin
		if((g_softap_fac_nv != NULL) && (g_softap_fac_nv->peri_remap_enable & 0x0100) && (g_softap_fac_nv->led_pin <= GPIO_PAD_NUM_63))
		{
			HWREG(0xA0140008) ^= (1<<g_softap_fac_nv->led_pin);
		}
	}
}

#endif

#if( configUSE_MALLOC_FAILED_HOOK == 1 )

void vApplicationMallocFailedHook( void )
{

}

#endif

#if( configUSE_IDLE_HOOK == 1 )

//void vApplicationIdleHook( void )
//{
//	uint32_t time1, time2;
//	char *mem;
//
//	time1 = prvTaskGetExpectedIdleTime( eAttentionLowPowerFlag );
//
//	time2 = prvTimerGetExpectedUnblockTime( eAttentionLowPowerFlag );
//
//	mem = osMemoryAllocAlign(500);
//
//	sprintf(mem, "\r\nTask is %lu\r\nTimer is %lu\r\n", time1, time2);
//
//	PRINT_BUF(mem);
//
//	osMemoryFree(mem);
//}

#endif
