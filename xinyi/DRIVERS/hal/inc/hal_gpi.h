/*
 * @file      hal_wakeup.h
 * @ingroup   peripheral
 * @brief     用于唤醒standby的唤醒源共有三个，分别是wakeupsrc0/wakeupsrc1/wakeupsrc2，对应每个唤醒都可以配置一个gpio作为外部唤醒源
 * @warning   其中gpio10/11/12不可作为外部唤醒源，不使用的唤醒源需将其gpio配置成GPIO_WAKEUP_UNUSE
 *			  !!!唤醒触发方式只能为低电平唤醒，低电平时间不得小于100us，最好不要超过1ms,用户需要自行通过硬件来保证!!!
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "hal_def.h"                                                                     

#define GPIO_WAKEUP_UNUSE     0xff      /*!< 不需要唤醒时引脚的配置*/

/** 
  * @brief  GPI工作状态枚举
  */
typedef enum {
  HAL_GPI_STATE_RESET             = 0x00U,   /*!< GPI外设在初始化状态			*/
  HAL_GPI_STATE_READY,                       /*!< GPI外设已初始化并就绪		*/
  HAL_GPI_STATE_BUSY,                        /*!< GPI外设处于忙状态			*/
} HAL_GPI_StateTypeDef;

/** 
  * @brief  GPI错误码类型枚举
  */
typedef enum
{
	HAL_GPI_ERROR_NONE = 0x00000000U,			/*!< 无错误*/
}HAL_GPI_ErrorCodeTypeDef;

/**
 * @brief GPI外设配置结构体
 * @param WkePin 唤醒源对应的GPIO引脚，其中WakePin[0]对应GPI唤醒源1，其中WakePin[2]对应GPI唤醒源2，其中WakePin[2]对应GPI唤醒源3
 * 
 */
 typedef struct 
 {
	uint8_t                         WakePin[3];					      /*!< GPI映射脚            */

	HAL_LockTypeDef                 Lock;					            /*!< GPI设备锁            */

	volatile HAL_GPI_StateTypeDef	State;                    /*!< GPI外设状态          */

  volatile uint32_t ErrorCode;			                        /*!< GPI错误码				*/

 } HAL_GPI_InitTypeDef;


/*****************************************************************************************/
/**
  * @func   HAL_GPI_Init(HAL_GPI_HandleTypeDef GPIHandleStruct).
  * @brief  用户配置唤醒源的gpio。
  * @param  GPIHandleStruct:GPI配置结构体,详见    @ref HAL_GPI_HandleTypeDef。
  * @retval HAL_StatusTypeDef:状态结果码,详见    @ref HAL_StatusTypeDef。
  */
/*****************************************************************************************/
HAL_StatusTypeDef HAL_GPI_Init(HAL_GPI_InitTypeDef* GPIHandleStruct);


/*****************************************************************************************/
/**
  * @func   HAL_GPI_Deinit(HAL_GPI_HandleTypeDef* GPIHandleStruct).
  * @brief  用户清除唤醒源的gpio配置，在调用该函数前，需要先将Deinit的引脚置为0xff，然后再调用HAL_GPI_Init函数.
  * @param  GPIHandleStruct:GPI配置结构体,详见    @ref HAL_GPI_HandleTypeDef。
  * @retval HAL_StatusTypeDef:状态结果码,详见    @ref HAL_StatusTypeDef。
  */
/*****************************************************************************************/
HAL_StatusTypeDef HAL_GPI_Deinit(HAL_GPI_InitTypeDef* GPIHandleStruct);


/*****************************************************************************************/
/**
   * @func   HAL_GPI_CallBack(HAL_GPI_HandleTypeDef* GPIHandleStruct).
   * @brief  GPI唤醒中断处理回调函数.
   * @param  GPIHandleStruct:GPI配置结构体详见    @ref HAL_GPI_HandleTypeDef。
   * @warning 这个函数在中断服务函数中被调用，不允许阻塞，不允许执行耗时很长的代码
   * @retval 无
   */
/*****************************************************************************************/
void HAL_GPI_CallBack(uint8_t SrcValue) __RAM_FUNC;

