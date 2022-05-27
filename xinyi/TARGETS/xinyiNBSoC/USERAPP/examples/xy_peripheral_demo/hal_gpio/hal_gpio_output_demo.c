/**
* @file        gpio_out_demo.c
* @ingroup     peripheral
* @brief       GPIO输出Demo，配置GPIO为推挽输出模式，并定时切换GPIO输出状态*
*
* @attention   XY1100不支持open drain模式  。

***********************************************************************************/

#if 1//DEMO_TEST

#include "xy_api.h"
#include "at_xy_cmd.h"

//任务参数配置
#define GPIO_OUT_TASK_PRIORITY    10
#define GPIO_OUT_STACK_SIZE       500

//任务全局变量
osThreadId_t g_hal_gpio_out_TskHandle =NULL;


/**
 * @brief 将GPIO引脚7配置为 输出模式，这个函数描述了将GPIO初始化为输出端口的相关步骤。\n
 */
void hal_gpio_out_init(void)
{
	HAL_GPIO_InitTypeDef gpio_init;

	//初始化GPIO引脚	
	gpio_init.Pin		= SI_NUM;//HAL_GPIO_PIN_NUM_7;
	gpio_init.Mode		= GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull		= GPIO_NOPULL;
	HAL_GPIO_Init(&gpio_init);
}

void Vol_gpio_out_init(void)
{
	HAL_GPIO_InitTypeDef gpio_init;

	//初始化GPIO引脚	
	gpio_init.Pin		= HAL_GPIO_PIN_NUM_20;
	gpio_init.Mode		= GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull		= GPIO_PULLDOWN;
	HAL_GPIO_Init(&gpio_init);
	HAL_GPIO_WritePin(HAL_GPIO_PIN_NUM_20, HAL_GPIO_PIN_RESET);
}


/**
 * @brief 设置引脚7的输出状态，每隔3s高低电平翻转一次。\n
 */
void hal_gpio_out_work_task(void)
{
	hal_gpio_out_init();
#if MODULE_130
	Vol_gpio_out_init();
#endif
	while(1)
	{
		//隔3s切换GPIO引脚的输出状态，用于调试
		xy_standby_lock();
		//HAL_GPIO_WritePin(HAL_GPIO_PIN_NUM_7, HAL_GPIO_PIN_SET);
		HAL_GPIO_WritePin(SI_NUM, HAL_GPIO_PIN_SET);	
		//通过osDelay释放线程控制权
		osDelay(3000);
		xy_standby_unlock();
#if 0
		HAL_GPIO_WritePin(HAL_GPIO_PIN_NUM_7, HAL_GPIO_PIN_RESET);
		xy_printf("\nHAL_GPIO_PIN_NUM_7  = 0\n");
		xy_standby_unlock();
		//通过osDelay释放线程控制权
		osDelay(3000);
#endif		
	}
}


/**
 * @brief 任务创建
 */
void hal_gpio_out_work_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "hal_gpio_out_work_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = GPIO_OUT_STACK_SIZE;
	g_hal_gpio_out_TskHandle = osThreadNew((osThreadFunc_t)hal_gpio_out_work_task,NULL,&thread_attr);
	
}



#endif
