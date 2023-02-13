/**
  ******************************************************************************
  * @file   hal_adc.c
  * @brief  此文件包含ADC外设的函数定义等.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hal_adc.h"
#include "adc.h"
#include "stdlib.h"

/**
  * @brief  初始化ADC外设.       
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval HAL status.详情参考 @ref HAL_StatusTypeDef.
  * @note   如用户使用ADC时需要修改参数，再次初始化前，请先去初始化
  */
HAL_StatusTypeDef HAL_ADC_Init(HAL_ADC_HandleTypeDef *hadc)
{
    HAL_StatusTypeDef tmp_hal_status = HAL_OK;

    /* Check ADC handle */
    if (hadc == NULL)
    {
        return HAL_ERROR;
    }


    if (hadc->State == HAL_ADC_STATE_RESET)
    {
      /* Allocate lock resource and initialize it */
      hadc->Lock = HAL_UNLOCKED;
    }
    else
    {
      return HAL_ERROR;
    }

    /* Update status */
    hadc->State = HAL_ADC_STATE_BUSY;

    osCoreEnterCritical();
    WAIT_DSP_ADC_IDLE();	
    SET_ARM_ADC_BUSY();
    for(volatile uint8_t i = 0; i < 200; i++);	//delay
    WAIT_DSP_ADC_IDLE();
    osCoreExitCritical();  
    ADC_Open(hadc->Init.Mode, hadc->Init.Speed, hadc->Init.ADCRange);


    if(hadc->Init.Mode == HAL_ADC_MODE_TYPE_VBAT)
    {
      HWREGB(0xA0110048)|= 0X51;
    }
    else
    {
      HWREGB(0xA0110048)|= 0X41;
    }


    hadc->ErrorCode = HAL_ADC_ERROR_NONE;
    hadc->State = HAL_ADC_STATE_READY;

    /* Return function status */
    return tmp_hal_status;
}

/**
  * @brief  去初始化ADC外设.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval HAL status.详情参考 @ref HAL_StatusTypeDef.
  */
HAL_StatusTypeDef HAL_ADC_DeInit(HAL_ADC_HandleTypeDef *hadc)
{
    HAL_StatusTypeDef tmp_hal_status = HAL_OK;

    /* Check ADC handle */
    if (hadc == NULL)
    {
        return HAL_ERROR;
    }
	//未初始化则返回
	if (hadc->State == HAL_ADC_STATE_RESET)
	{
		return HAL_ERROR;
	}
    /* Update status */
    hadc->State = HAL_ADC_STATE_BUSY;

    ADC_Close();

	hadc->ErrorCode = HAL_ADC_ERROR_NONE;
	hadc->State = HAL_ADC_STATE_RESET;

	/* Release Lock */
	__HAL_UNLOCK(hadc);

    /* Return function status */
    return tmp_hal_status;
}

/**待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  获取数据寄存器值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的数据寄存器值.
  */
int32_t HAL_ADC_GetValue(HAL_ADC_HandleTypeDef *hadc)
{
    int32_t ret_val;
    /* Check ADC handle */
    if (hadc == NULL)
	{
		return HAL_ERROR;
	}
    /* Return the selected ADC converted value */
    ret_val = ADC_Get_Real_Value();
    return ret_val;
}

/**此接口待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  将数据寄存器值转换为中间值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的中间值.
  */
int32_t HAL_ADC_GetConverteValue(HAL_ADC_HandleTypeDef *hadc, int32_t realvalue)
{
    int32_t ret_val;
    /* Return the selected ADC converted value */
    ret_val = ADC_Get_Converte_Value(hadc->Init.Mode, realvalue);
    return ret_val;
}

/**此接口待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  将中间值转换为未经过校验的电压值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的未经过校验的电压值.
  */
int32_t HAL_ADC_GetVoltageValue(HAL_ADC_HandleTypeDef *hadc, int32_t conv_val)
{
    int32_t ret_val;
    ret_val = ADC_Get_Voltage_Without_Cal(hadc->Init.Mode, hadc->Init.ADCRange, conv_val);
    return ret_val;
}

/**此接口待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  将中间值转换为校验后的电压值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的校验后的电压值.
  */
int32_t HAL_ADC_GetVoltageValueWithCal(HAL_ADC_HandleTypeDef *hadc, int32_t conv_val)
{
    int32_t ret_val;
    ret_val = ADC_Get_Voltage_With_Cal(hadc->Init.Mode, hadc->Init.ADCRange, conv_val);
    return ret_val;
}


/**
  * @brief  获取ADC的转换值
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval ADC转换结果
  */
int32_t HAL_ADC_GetValue_2(HAL_ADC_HandleTypeDef *hadc)
{
	short real_val;       //从寄存器获取的真实值
	short convert_val;    //转换值
//	short no_cal_vol;     //未校准的电压值
	short cal_vol;        //校准后的电压值

	int average = 0;

    /* Check ADC handle */
  if (hadc == NULL)
	{
		return HAL_ERROR;
	}

	//从寄存器获取真实值10次取平均值
	for(uint8_t i = 0; i < 10; i++)
	{
		real_val = ADC_Get_Real_Value();//HAL_ADC_GetValue(&ADC_HandleStruct);
		average += real_val;
	}

	if(average >= 0)
		real_val = (average + 5) / 10;
	else
		real_val = (average - 5) / 10;

	//获取转换值
	convert_val =  ADC_Get_Converte_Value(hadc->Init.Mode, real_val);

	if(otp_valid_flag == 1)
	    cal_vol = ADC_Get_Voltage_With_Cal(hadc->Init.Mode, hadc->Init.ADCRange, convert_val);
	else
        cal_vol = ADC_Get_Voltage_Without_Cal(hadc->Init.Mode, hadc->Init.ADCRange, convert_val);

	return abs(cal_vol);
}

