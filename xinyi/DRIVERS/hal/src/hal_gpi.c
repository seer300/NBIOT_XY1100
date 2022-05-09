/*
 * @file      hal_wakeup.c
 * @ingroup   peripheral
 * @brief     用于唤醒standby的唤醒源共有三个，分别是wakeupsrc0/wakeupsrc1/wakeupsrc2，对应每个唤醒都可以配置一个gpio作为外部唤醒源
 * @warning   其中gpio10/11/12不可作为外部唤醒源，不使用的唤醒源需将其gpio配置成GPIO_WAKEUP_UNUSE
 *            !!!唤醒触发方式只能为低电平唤醒，低电平时间不得小于100us，最好不要超过1ms,用户需要自行通过硬件来保证!!!
 */
#include "hal_gpio.h"
#include "hal_gpi.h"
#include "hal_def.h"
#include "hw_gpio.h"
#include "gpio.h"
#include "low_power.h"
#include <stdint.h>


#define GPI_INT_REG               0xA0058001


HAL_StatusTypeDef HAL_GPI_Init(HAL_GPI_InitTypeDef* GPIHandleStruct)
{

	if (GPIHandleStruct == NULL)
	    return HAL_ERROR;

	for(int i = 0; i < 3; i++)
	{
		if(GPIHandleStruct->WakePin[i] == HAL_GPIO_PIN_NUM_10 || GPIHandleStruct->WakePin[i] == HAL_GPIO_PIN_NUM_11 || GPIHandleStruct->WakePin[i] == HAL_GPIO_PIN_NUM_12)
		{
			return HAL_ERROR;
		}
	}

	if (GPIHandleStruct->State == HAL_GPI_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		GPIHandleStruct->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	GPIHandleStruct->State = HAL_GPI_STATE_BUSY;

	uint8_t tempvalue = HWREGB(0xA0110002);

	HWREGB(0xA0110002) = 0;

	for(int i = 0; i < 3; i++)
	{
		uint8_t tempmask = 0x08;

		if(GPIHandleStruct->WakePin[i] <= HAL_GPIO_PIN_NUM_63)
		{
			GPIOConflictStatusClear(GPIHandleStruct->WakePin[i]);
			GPIOPadModeSet(GPIHandleStruct->WakePin[i], GPIO_MODE_HW, GPIO_CTL_REGISTER);
			GPIOPullupEn(GPIHandleStruct->WakePin[i]);
			GPIOPulldownDis(GPIHandleStruct->WakePin[i]);
			GPIODirectionSet(GPIHandleStruct->WakePin[i], GPIO_DIR_MODE_IN);
			GPIOPeripheralPad(GPIO_GPI1 + i, GPIHandleStruct->WakePin[i]);


			while(!(HWREGB(GPI_INT_REG) & (tempmask << i)));

			HWREGB(GPI_INT_REG) &= ~(tempmask << i);

			while(HWREGB(GPI_INT_REG) & (tempmask << i));
		}
	}

	HWREGB(0xA0110002) = tempvalue;

	GPIHandleStruct->ErrorCode = HAL_GPI_ERROR_NONE;
	GPIHandleStruct->State = HAL_GPI_STATE_READY;

	return HAL_OK;
}


HAL_StatusTypeDef HAL_GPI_Deinit(HAL_GPI_InitTypeDef* GPIHandleStruct)
{
	uint8_t tempmask = 0x08;

	/* Check the GPI handle allocation */
	if (GPIHandleStruct == NULL)
		return HAL_ERROR;

	/* Update GPI status */
	GPIHandleStruct->State = HAL_GPI_STATE_BUSY;

	for(int i = 0; i < 3; i++)
	{
		if(GPIHandleStruct->WakePin[i] <= HAL_GPIO_PIN_NUM_63)
		{
			GPIOModeSet(GPIHandleStruct->WakePin[i], GPIO_MODE_GPIO);
		}

		if(HWREGB(GPI_INT_REG) & (tempmask << i))
		{
			HWREGB(GPI_INT_REG) &= ~(tempmask << i);
		}
	}

	GPIHandleStruct->ErrorCode = HAL_GPI_ERROR_NONE;
	GPIHandleStruct->State = HAL_GPI_STATE_RESET;

	/* Release Lock */
	__HAL_UNLOCK(GPIHandleStruct);

	return HAL_OK;
}


__weak void HAL_GPI_CallBack(uint8_t SrcValue)
{
	/* 防止编译器报错 */
	UNUSED_ARG(SrcValue);
}

