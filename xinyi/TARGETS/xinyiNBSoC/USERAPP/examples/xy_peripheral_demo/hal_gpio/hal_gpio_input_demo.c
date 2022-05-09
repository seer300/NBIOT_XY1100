/**
* @file        hal_gpio_input_demo.c
* @ingroup     peripheral
* @brief       GPIO输入Demo，配置GPIO为上拉输入模式，并读取输入状态
*
* @attention  1、对于输入模式，每个GPIO并不同时支持上拉和下拉，仅支持上拉或下拉中的其中一种状态。在XY1100芯片中，GPIO10/11/12仅支持下拉，其余的GPIO仅支持上拉；\n
*			  2、当Pad配置为input模式，需要根据不同的外部连接状态调整引脚的配置模式；\n
*           	（1）	如果外部引脚悬浮，则需配置上拉；\n
*           	（2）	如果外部引脚连接上拉，则需配置上拉或者禁用上下拉；\n
*           	（3）	如果外部引脚存在下拉，则需配置下拉或不配置；\n
*           	（4）	如果外部引脚连接外设比如(UART)分一下两种情况：\n
*                	a)	外设不掉电，外设与1100互相已经确保了PAD上不会悬浮，不需要处理；\n
					b)	外设掉电，但与外设的连接关系仍在。 则需将该PAD配置为输出0模式或者输入下拉模式。\n
			  3、XY1100目前尚未支持Analog模式。\n

***********************************************************************************/

#if DEMO_TEST


#include "xy_api.h"

//任务参数配置
#define GPIO_IN_TASK_PRIORITY    10
#define GPIO_IN_STACK_SIZE       500

//任务全局变量
osThreadId_t g_hal_gpio_in_TskHandle = NULL;


/**
 * @brief  初始化GPIO.这个函数描述了将GPIO初始化为输入端口的相关步骤
 */
void hal_gpio_in_init(void)
{
	HAL_GPIO_InitTypeDef gpio_init;

	//选择引脚6
	gpio_init.Pin		= HAL_GPIO_PIN_NUM_6;
	//设置引脚为输入模式
	gpio_init.Mode		= GPIO_MODE_INPUT;
	//设置引脚为内部上拉
	gpio_init.Pull		= GPIO_PULLUP;
	//使能之前的配置
	HAL_GPIO_Init(&gpio_init);
}


/**
 * @brief 读取引脚的输入状态，并打印出来
 */
void hal_gpio_in_work_task(void)
{
	hal_gpio_in_init();

	while(1)
	{
		xy_standby_lock();

		uint8_t gpio_in_value = 0xff;

		gpio_in_value = HAL_GPIO_ReadPin(HAL_GPIO_PIN_NUM_6);
		xy_printf("HAL_GPIO_PIN_NUM_6 Value:%d", gpio_in_value);

		xy_standby_unlock();
		//通过osDelay释放线程控制权
		osDelay(1000);	
	}
}


/**
 * @brief 任务创建
 */
void hal_gpio_in_work_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "hal_gpio_in_work_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = GPIO_IN_STACK_SIZE;
    g_hal_gpio_in_TskHandle = osThreadNew ((osThreadFunc_t)hal_gpio_in_work_task,NULL,&thread_attr);
}



#endif
