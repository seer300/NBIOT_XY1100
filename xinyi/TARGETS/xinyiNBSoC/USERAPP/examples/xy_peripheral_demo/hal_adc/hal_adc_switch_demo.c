/** 此Demo待废弃
* @file			hal_adc_switch_demo.c
* @ingroup		peripheral
* @brief		ADC外设Demo
* @attention	当量程设置为1V时（目前仅支持1V量程）：
*         		ADC参考电压是1V，精度12bit, 采样频率有240K和480K可选。
*         		此demo为ADC两个通道之间的切换，轮流进行ADC转换。每路都是单端模式下，ADC量程为0-1V；
***********************************************************************************/

#if DEMO_TEST

#include "xy_api.h"


//任务全局变量
osThreadId_t  hal_adc_switch_TskHandle = NULL;

HAL_ADC_HandleTypeDef ADC_HandleStructP;
HAL_ADC_HandleTypeDef ADC_HandleStructN;

/**
* @brief	P通道初始化函数
*
*/
void Adc_ChannelP_Init(void)
{
	ADC_HandleStructP.Init.Mode = HAL_ADC_MODE_TYPE_SINGLE_P;
	ADC_HandleStructP.Init.Speed = HAL_ADC_SPEED_TYPE_240K;
	ADC_HandleStructP.Init.ADCRange = HAL_ADC_RANGE_TYPE_1V;

	HAL_ADC_Init(&ADC_HandleStructP);
}


/**
* @brief	N通道初始化函数
*
*/
void Adc_ChannelN_Init(void)
{
	ADC_HandleStructN.Init.Mode = HAL_ADC_MODE_TYPE_SINGLE_N;
	ADC_HandleStructN.Init.Speed = HAL_ADC_SPEED_TYPE_240K;
	ADC_HandleStructN.Init.ADCRange = HAL_ADC_RANGE_TYPE_1V;

	HAL_ADC_Init(&ADC_HandleStructN);
}




/**
 * @brief 任务线程
 *
 */

void hal_adc_switch_task()
{
	uint16_t PCh_cal_vol=0;
	uint16_t NCh_cal_vol=0;
	char *str_out = NULL;

	while(1)
	{
		xy_standby_lock();


		//P通道转换
		Adc_ChannelP_Init();
		PCh_cal_vol = HAL_ADC_GetValue_2(&ADC_HandleStructP);	//转换
		HAL_ADC_DeInit(&ADC_HandleStructP);

		//通过AT口打印信息，用于调试
		//if (no_cal_vol > 400)
		{
			str_out = xy_zalloc(200);
			snprintf(str_out, 200, "\r\n""PCh_cal_vol  = %d\r\n", PCh_cal_vol);
			send_debug_str_to_at_uart(str_out);
			xy_free(str_out);
		}
		osDelay(1000);



		//N通道转换
		Adc_ChannelN_Init();
		NCh_cal_vol    = HAL_ADC_GetValue_2(&ADC_HandleStructN);	//转换 
		HAL_ADC_DeInit(&ADC_HandleStructN);

		//通过AT口打印信息，用于调试
		//if (no_cal_vol > 400)
		{
			str_out = xy_zalloc(200);
			snprintf(str_out, 200, "\r\n""NCh_cal_vol  = %d\r\n", NCh_cal_vol);
			send_debug_str_to_at_uart(str_out);
			xy_free(str_out);
		}
		osDelay(1000);


		xy_standby_unlock();

	}
}


/**
 * @brief 任务创建
 */
void hal_adc_switch_task_init(void)
{
	osThreadAttr_t thread_attr = {0};

	thread_attr.name	   = "hal_adc_test_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 512;
	hal_adc_switch_TskHandle = osThreadNew((osThreadFunc_t)(hal_adc_switch_task), NULL, &thread_attr);
}

#endif
