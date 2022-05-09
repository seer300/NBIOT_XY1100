/**
* @file			adc_demo.c
* @ingroup		peripheral
* @brief		ADC外设Demo
* @attention	ADC精度12bit, 采样频率有240K和480K可选。
*				测试外接信号源时，量程为1V。分为差分模式与单端模式。单端模式下，可作为两路ADC分时复用。
*         		内部电压模式，量程为0-5V。无需接线，测量结果为芯片内部VBAT接口处的电压。
***********************************************************************************/

#if DEMO_TEST

#include "xy_api.h"

//任务参数配置
#define ADC_WORK_MODE     HAL_ADC_MODE_TYPE_VBAT
#define ADC_WORK_SPEED    HAL_ADC_SPEED_TYPE_240K

//任务全局变量
osThreadId_t  hal_adc_TskHandle = NULL;
HAL_ADC_HandleTypeDef ADC_HandleStruct;


/**
 * @brief 任务线程
 *
 */
void hal_adc_test_task()
{
	short cal_vol;        //API内部利用转换值进行校准得到圆整值，利用圆整值转换为校准后的电压值
	
	char *str_out = NULL;
	ADC_HandleStruct.Init.Mode = ADC_WORK_MODE;
	ADC_HandleStruct.Init.Speed = ADC_WORK_SPEED;
	ADC_HandleStruct.Init.ADCRange = HAL_ADC_RANGE_TYPE_1V;

	

	while(1)
	{
		xy_standby_lock();

		//初始化ADC
		HAL_ADC_Init(&ADC_HandleStruct);
		cal_vol = HAL_ADC_GetValue_2(&ADC_HandleStruct);
		//去初始化ADC
		HAL_ADC_DeInit(&ADC_HandleStruct);

		//通过AT口打印信息，用于调试
		str_out = xy_zalloc(200);
		snprintf(str_out, 200, "\r\n"
			"cal_vol     = %d\r\n",
			cal_vol);
			send_debug_str_to_at_uart(str_out);
		xy_free(str_out);
		

		xy_standby_unlock();
		//通过osDelay释放线程控制权
		osDelay(2000);
	}

}


/**
 * @brief 任务创建
 */
void hal_adc_test_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "hal_adc_test_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 512;
	hal_adc_TskHandle = osThreadNew((osThreadFunc_t)(hal_adc_test_task), NULL, &thread_attr);
}

#endif
