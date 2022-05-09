/**
* @file        gpio_int_demo.c
* @ingroup     peripheral
* @brief       GPIO中断demo
* @note    	           在这个DEMO中，先将GPIO引脚6配置成下降沿中断触发模式，  将GPIO引脚7配置为双边沿中断，在死循环中等待
* 			           外部中断的到来，在中断服务函数中，读取中断状态并清中断标志位，然后累计每个引脚的中断次数。*
*
* @attention   1、GPIO支持中断触发方式有：下降沿触发，双边沿触发，高电平触发，不支持上升沿触发。\n
* 			   2、如果外部输入信号并不平稳，需要外部增加消抖电路。\n
*
***********************************************************************************/

#if DEMO_TEST


#include "xy_api.h"

//任务参数配置
#define HAL_GPIO_INT_TASK_PRIORITY    10
#define HAL_GPIO_INT_STACK_SIZE       500

//任务全局变量
osThreadId_t g_hal_gpio_int_TskHandle = NULL;
osSemaphoreId_t g_hal_gpio_int_sem = NULL;

static uint8_t hal_gpio_interrupt_times = 0;


/**
 * @brief 	GPIO引脚外部中断的中断服务函数，在此中断服务函数中，先取GPIO的中断状态，若有中断发生，则将相应引脚的中断次数累加，\n
 *
 * @note 	1、中断服务函数，用户在使用时可以将__weak 删除，或者自行添加中断处理函数，但是不需要__weak ！！！注意：用户需保证中断里调用的函数都在ram上！！！\n
 * 			2、HAL_GPIO_ReadAndClearIntFlag（...）函数用于获取GPIO中断状态寄存器的值判断具体触发中断的GPIO，
 * 			！！！注意此函数的内部设定是可以一次读所有的GPIO的中断状态 ，即如果多个GPIO设置了中断，则填其中任意一个GPIO即可.API的详细介绍请参考.h文件或电子书
 *
 */
void HAL_GPIO_IRQHandler(void) __RAM_FUNC;
__weak void HAL_GPIO_IRQHandler(void)
{
	//读清中断，并返回中断状态寄存器的值
	uint32_t sta = HAL_GPIO_ReadAndClearIntFlag(HAL_GPIO_PIN_NUM_6);
	//关闭GPIO中断
//	HAL_GPIO_IntCtl(HAL_GPIO_PIN_NUM_6, DISABLE);
	//判断中断是否来自当前GPIO
	if(sta & (1 << HAL_GPIO_PIN_NUM_6))
	{
		//引脚6的中断次数累加，这里只是为了演示，用户可自行在中断服务函数内部进行相关业务的开发。
		hal_gpio_interrupt_times++;
		osSemaphoreRelease(g_hal_gpio_int_sem);
	}
//	HAL_GPIO_IntCtl(HAL_GPIO_PIN_NUM_6, ENABLE);

}


/**
 * @brief   GPIO引脚外部中断的初始化，这个函数描述了GPIO引脚外部中断初始化需要的相关步骤。\n
 * 			此函数将引脚6配置为下降沿中断的触发，将引脚7配置为双边沿触发

 */
void hal_gpio_int_init(void)
{
	//注册GPIO中断
	HAL_GPIO_IT_REGISTER();

	//创建信号量
	g_hal_gpio_int_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	HAL_GPIO_InitTypeDef gpio_init;

	//选择引脚6
	gpio_init.Pin		= HAL_GPIO_PIN_NUM_6;
	//设置引脚的外部中断为下降沿触发
	gpio_init.Mode		= GPIO_MODE_IT_FALLING;
	//设置引脚为内部上拉
	gpio_init.Pull		= GPIO_PULLUP;
	//使能之前的配置
	HAL_GPIO_Init(&gpio_init);

	//选择引脚7
	gpio_init.Pin		= HAL_GPIO_PIN_NUM_7;
	//设置引脚的外部中断为双边沿触发
	gpio_init.Mode		= GPIO_MODE_IT_RISING_FALLING;
	//设置引脚为内部上拉
	gpio_init.Pull		= GPIO_PULLUP;
	//使能之前的配置
	HAL_GPIO_Init(&gpio_init);
}


/**
 * @brief 等待引脚外部中断的到来，如果有中断到来，则跳转中断服务函数执行中断次数累加
 */
void hal_gpio_int_work_task(void)
{
	hal_gpio_int_init();

	while(1)
	{
		//等待获取信号量，释放线程控制权
		if(osOK == osSemaphoreAcquire(g_hal_gpio_int_sem, osWaitForever))
		{
			xy_standby_lock();
			//打印信息，用于调试，用户可在此做实际业务
			xy_printf("hal_gpio_interrupt times:%d\n", hal_gpio_interrupt_times);
			xy_standby_unlock();
		}
	}
}


/**
 * @brief 任务创建
 */
void hal_gpio_int_work_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "hal_gpio_int_work_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = HAL_GPIO_INT_STACK_SIZE;
	g_hal_gpio_int_TskHandle = osThreadNew ((osThreadFunc_t)hal_gpio_int_work_task,NULL,&thread_attr);
}
#endif
