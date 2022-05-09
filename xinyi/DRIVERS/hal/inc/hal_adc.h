/**
  ******************************************************************************
  * @file   hal_adc.h
  * @brief  此文件包含ADC外设的变量，枚举，结构体定义，函数声明等.
  * @attention	ADC精度12bit, 采样频率有240K和480K可选。
  *				测试外接信号源时，量程为1V。分为差分模式与单端模式。
  *         	内部电压模式，量程为0-5V。无需接线，测量结果为芯片内部VBAT接口处的电压。
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include "hal_cortex.h"

/** 
  * @brief  ADC硬件连接模式枚举
  *
**/
typedef enum {
  HAL_ADC_MODE_TYPE_DOUBLE_P  = 0, /*!< 双端P模式, P 端连接信号端, N 端接地，待废弃   */
  HAL_ADC_MODE_TYPE_DOUBLE_N,      /*!< 双端N模式, N 端连接信号端, P 端接地，待废弃   */
  HAL_ADC_MODE_TYPE_DOUBLE_PN,     /*!< 双端PN模式, P端 N 端分别连接信号源的正负极     */
  HAL_ADC_MODE_TYPE_SINGLE_P,      /*!< 单端P模式, P 端连接信号端，芯片内部接地          */
  HAL_ADC_MODE_TYPE_SINGLE_N,      /*!< 单端N模式, N 端连接信号端，芯片内部接地             */
  HAL_ADC_MODE_TYPE_VBAT           /*!< 内部电压模式, 无需接线                */
} HAL_ADCMode_TypeDef;

/** 
  * @brief  ADC工作采样频率枚举 
**/
typedef enum {
	HAL_ADC_SPEED_TYPE_480K = 0x1F,  /*!< 480K工作采样频率  */ 
	HAL_ADC_SPEED_TYPE_240K = 0x3F   /*!< 240K工作采样频率  */
} HAL_ADCSpeed_TypeDef;

/** 
  * @brief  ADC电压测量范围枚举 
**/
typedef enum {
	HAL_ADC_RANGE_TYPE_1V = 0x00,    /*!< 1V电压测量范围  */
} HAL_ADCRange_TypeDef;

/** 
  * @brief  ADC错误类型枚举 
**/
typedef enum {
  HAL_ADC_ERROR_MODE_ERROR      = -32768, /*!< 工作模式错误     */
  HAL_ADC_ERROR_NO_CALIBRATION  = -32767, /*!< 未校验错误       */
  HAL_ADC_ERROR_RANGE_ERROR     = -32766  /*!< 电压测量范围错误 */
} HAL_ADCError_TypeDef;

/** 
  * @brief  ADC初始化结构体 
**/
typedef struct
{
	HAL_ADCMode_TypeDef Mode;           /*!< ADC工作模式      */
	HAL_ADCSpeed_TypeDef Speed;         /*!< ADC工作采样频率  */
	HAL_ADCRange_TypeDef ADCRange;      /*!< ADC电压测量范围  */
} HAL_ADC_InitTypeDef;

/** 
  * @brief  ADC工作状态枚举
  */
typedef enum {
	HAL_ADC_STATE_RESET = 0x00U,				/*!< GPI外设在初始化状态*/
	HAL_ADC_STATE_READY,						/*!< GPI外设已初始化并就绪*/
	HAL_ADC_STATE_BUSY,							/*!< GPI外设处于忙状态*/
} HAL_ADC_StateTypeDef;

/** 
  * @brief  ADC错误码类型枚举
  */
typedef enum
{
	HAL_ADC_ERROR_NONE = 0x00000000U,			/*!< 无错误*/
}HAL_ADC_ErrorCodeTypeDef;

/** 
  * @brief  ADC 控制结构体 
**/
typedef struct
{
	HAL_ADC_InitTypeDef Init;							/*!< ADC初始化结构体*/

	HAL_LockTypeDef Lock;								/*!< ADC设备锁*/

	volatile HAL_ADC_StateTypeDef  State;				/*!< ADC外设状态*/

	volatile uint32_t ErrorCode;						/*!< ADC错误码*/
  
} HAL_ADC_HandleTypeDef;


/**
  * @brief  初始化ADC外设.       
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval HAL status.详情参考 @ref HAL_StatusTypeDef.
  * @note   调用初始化函数后，使用结束后请调用去初始化
  */
HAL_StatusTypeDef HAL_ADC_Init(HAL_ADC_HandleTypeDef *hadc);

/**
  * @brief  去初始化ADC外设.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval HAL status.详情参考 @ref HAL_StatusTypeDef.
  */
HAL_StatusTypeDef HAL_ADC_DeInit(HAL_ADC_HandleTypeDef *hadc);

/**待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  获取数据寄存器值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的数据寄存器值.
  */
int32_t HAL_ADC_GetValue(HAL_ADC_HandleTypeDef *hadc);

/**此接口待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  将数据寄存器值转换为中间值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的中间值.
  */
int32_t HAL_ADC_GetConverteValue(HAL_ADC_HandleTypeDef *hadc, int32_t realvalue);

/**此接口待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  将中间值转换为未经过校验的电压值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的未经过校验的电压值.
  */
int32_t HAL_ADC_GetVoltageValue(HAL_ADC_HandleTypeDef *hadc, int32_t conv_val);

/**此接口待废弃  ADC转换过程集中至接口HAL_ADC_GetValue_2中，请客户参考Demo使用
  * @brief  将中间值转换为校验后的电压值.
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval 有符号的校验后的电压值.
  */
int32_t HAL_ADC_GetVoltageValueWithCal(HAL_ADC_HandleTypeDef *hadc, int32_t conv_val);

/**
  * @brief  获取ADC的转换值
  * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.
  *         详情参考 @ref HAL_ADC_HandleTypeDef.
  * @retval ADC转换结果
  */
int32_t HAL_ADC_GetValue_2(HAL_ADC_HandleTypeDef *hadc);


