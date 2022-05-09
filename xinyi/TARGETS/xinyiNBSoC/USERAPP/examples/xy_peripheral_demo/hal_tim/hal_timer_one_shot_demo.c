/**
* @file			timer_one_shot_timer_demo.c
* @ingroup		peripheral
* @brief		Timer定时器外设Demo，one_shot模式，单次定时。此模式下timer到达reload触发中断后，timer会自动关闭。
* @attention	如果对时间精度不敏感，不建议直接使用定时器外设进行定时功能的开发，建议使用软定时器替代。
				timer frequency = pclk / ClockDivision / Reload
				以timer reload值为306*12000为例，示意图如下：
				               _                                                            
				              | |                                                              
				 _____________| |____________________________________________
				|--306*12000--|

***********************************************************************************/

#if DEMO_TEST



#include "xy_api.h"

//任务参数配置
#define TIMER_TASK_PRIORITY  10
#define TIMER_STACK_SIZE     0x400

//demo宏定义
#define TimHandle          		TimOneShotHandle
HAL_TIM_HandleTypeDef TimHandle;

//任务全局变量
osThreadId_t  g_hal_one_shot_timer_TskHandle = NULL;
osSemaphoreId_t g_hal_one_shot_timer_sem = NULL;

static uint32_t counter_value;
/**
 * @brief TIMER中断处理函数
 * @note 可以在定时器中断函数中做时间敏感业务相关处理
 * @warning 部分函数和打印log xy_printf()不能在此处调用，否则会触发断言
 * 注意：1.用户在使用时需将__weak 删除！！！2.用户需保证中断及中断里调用的函数都在ram上！！！
 * 用户注意Timer中断函数与Timer的匹配。HAL_TIM1与HAL_TIM1_IRQHandler匹配，依此类推。
 */
void HAL_TIM1_IRQHandler(void) __RAM_FUNC;
__weak void HAL_TIM1_IRQHandler(void)
{
	counter_value = HAL_TIM_GetCount(&TimHandle);
	osSemaphoreRelease(g_hal_one_shot_timer_sem);
}


/**
 * @brief TIMER one_shot模式初始化函数
 *  	这个函数描述了timer初始化为one_shot模式时需要的相关步骤。\n
 * 		在初始化函数内部需要设置定时器的编号、工作模式、重载值、时钟分频、时钟极性等\n
 * 		且需要注册定时中断，然后打开定时器。\n
 */
void hal_one_shot_timer_init(void)
{
	//创建信号量
	g_hal_one_shot_timer_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	//初始化Timer
	TimHandle.Instance				=	HAL_TIM1;
	TimHandle.Init.Mode				=	HAL_TIM_MODE_ONE_SHOT;
	TimHandle.Init.Reload			=	306 * 5000;
	TimHandle.Init.ClockDivision	=	HAL_TIM_CLK_DIV_128;
	TimHandle.Init.TIMPolarity		=	HAL_TIM_Polarity_Set;
	HAL_TIM_Init(&TimHandle);

	//注册Timer中断服务函数
	HAL_TIM_IT_REGISTER(&TimHandle);

	//开启Timer
	HAL_TIM_Start(&TimHandle);
}


/**
 * @brief 任务线程
 *
 */
void hal_one_shot_timer_work_task(void)
{
	hal_one_shot_timer_init();

	while(1)
	{
		//等待获取信号量，释放线程控制权
		if( osOK == osSemaphoreAcquire(g_hal_one_shot_timer_sem, osWaitForever) )
		{
			xy_standby_lock();
			//打印调试信息，用户可在此做实际业务
			xy_printf("hal_one_shot_timer_work_task counter_value:%d\n", counter_value);

			xy_standby_unlock();

		}
	}
}


/**
 * @brief 任务创建
 *
 */
void hal_one_shot_timer_work_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "hal_one_shot_timer_work_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = TIMER_STACK_SIZE;
	g_hal_one_shot_timer_TskHandle = osThreadNew((osThreadFunc_t)hal_one_shot_timer_work_task,NULL,&thread_attr);
}
#endif
