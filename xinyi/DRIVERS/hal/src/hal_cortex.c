/**
  ******************************************************************************
  * @file    hal_cortex.c
  * @brief   CORTEX HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities of the CORTEX:
  *           + Initialization and de-initialization functions
  *           + Peripheral Control functions 
  *
  @verbatim  
  ==============================================================================
  */

/* Includes ------------------------------------------------------------------*/
#include "hal_cortex.h"
#include "hal_def.h"
#include "xinyi_conf.h"
#include "xy_utils.h"


/**
  * @brief  This function register interrupt.
  * @param  IRQn: specifies the IRQ source.
  *         This parameter can be one of the IRQn_Type.
  * @param  IRQnHandle: specifies the IRQ handle function.
  *         This parameter can be one of the IRQnHandle_Type.
  * @param  priority: specifies the IRQ priority.
  * @warning  双核运行状态下，擦写flash接口中会锁调度，直到flash操作完成。由于仅锁调度不屏蔽中断，此时中断还会得到响应， \n
  ***  这就需要保证中断handler及其调用的所有函数中不涉及任何的flash读写，具体限制体现为：
  ***	  ​1.  不能执行flash上的代码（text）\n
  ***	  2.  不能读取任何位于flash上的常量 （rodata）\n
  ***	  3.  不能调用flash相关api  \n
  ​***  如果将锁调度替换成锁中断，则没有上面的几个限制，但有一定的负面影响，具体表现为： \n
  ​*** flash操作期间，所有中断得不到响应，无论中断触发多少次，等恢复全局中断以后都只会执行一次中断处理， \n
  ​*** 典型的如系统的1ms tick中断，在整个flash操作过程中，tick都不会增长，直到恢复全局中断以后才会增长一次， \n
  ​*** 这就导致系统维护的tick定时和真实时间产生偏离，软件定时器就会受此影响导致真实响应时间晚于预期的时间，其时间差即为flash操作耗时。 \n
  ​*** 其他依赖中断次数的场景皆有类似问题。
  */
void HAL_NVIC_IntRegister(HAL_IRQn_Type HAL_IRQn, IRQnHandle_Type IRQnHandle, uint32_t priority)
{
	NVIC_IntRegister(HAL_IRQn, IRQnHandle, priority);
}


/**
  * @brief  This function Unregister interrupt.
  * @param  IRQn: specifies the IRQ source.
  *         This parameter can be one of the IRQn_Type.
  * @retval None
  */
void HAL_NVIC_IntUnregister(HAL_IRQn_Type HAL_IRQn)
{
	NVIC_IntUnregister(HAL_IRQn);
}


/**
  * @brief  Initializes the System Timer and its interrupt, and starts the System Tick Timer.
  *         Counter is in free running mode to generate periodic interrupts.
  * @param  TicksNumb: Specifies the ticks Number of ticks between two interrupts.
  * @retval status:  - 0  Function succeeded.
  *                  - 1  Function failed.
  */
uint32_t HAL_SYSTICK_Config(uint32_t TicksNumb)
{
   return SysTick_Config(TicksNumb);
}

/**
  * @brief  Configures the SysTick clock source.
  * @param  CLKSource: specifies the SysTick clock source.
  *         This parameter can be one of the following values:
  *             @arg SYSTICK_CLKSOURCE_HCLK_DIV8: AHB clock divided by 8 selected as SysTick clock source.
  *             @arg SYSTICK_CLKSOURCE_HCLK: AHB clock selected as SysTick clock source.
  * @retval None
  */
void HAL_SYSTICK_CLKSourceConfig(uint32_t CLKSource)
{
	/* Prevent unused argument(s) compilation warning */
	  UNUSED_ARG(CLKSource);
}

/**
  * @brief  This function handles SYSTICK interrupt request.
  * @retval None
  */
void HAL_SYSTICK_IRQHandler(void)
{
  HAL_SYSTICK_Callback();
}

/**
  * @brief Provides a tick value in millisecond.
  * @note  This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @retval tick value
  */
uint32_t HAL_GetTick(void)
{
	return xTaskGetTickCountFromISR()/*g_ullTickCount*/;
}

/**
  * @brief  SYSTICK callback.
  * @retval None
  */
__weak void HAL_SYSTICK_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_SYSTICK_Callback could be implemented in the user file
   */
}
