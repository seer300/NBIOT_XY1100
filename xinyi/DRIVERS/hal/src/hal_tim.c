/**
  ******************************************************************************
  * @file    hal_tim.c
  * @brief   此文件包含定时器外设的函数定义等.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hal_tim.h"
#include "hw_timer.h"

/**
  * @brief  初始化定时器.
  * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @retval HAL status.详情参考 @ref HAL_StatusTypeDef.
  */
HAL_StatusTypeDef HAL_TIM_Init(HAL_TIM_HandleTypeDef *htim)
{
	/* Check the CSP handle allocation */
	if(htim == NULL)
		return HAL_ERROR;

	if (htim->State == HAL_TIM_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		htim->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	htim->State = HAL_TIM_STATE_BUSY;

	/* In case of mode selection */
	switch(htim->Init.Mode)
	{
		case HAL_TIM_MODE_ONE_SHOT:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			break;
		case HAL_TIM_MODE_CONTINUOUS:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			break;
		case HAL_TIM_MODE_COUNTER:
			//Set TPOL
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, htim->Init.TIMPolarity);
			break;
		case HAL_TIM_MODE_PWM_SINGLE:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			//Set TPOL
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, htim->Init.TIMPolarity);
			//Write PWMReg
			WRITE_REG(htim->Instance->TMR_PWM, htim->Init.PWMReg);
			break;
		case HAL_TIM_MODE_CAPTURE:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			//Set TPOL
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, htim->Init.TIMPolarity);
			break;
		case HAL_TIM_MODE_COMPARE:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			//Set TPOL
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, htim->Init.TIMPolarity);
			break;
		case HAL_TIM_MODE_GATED:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			//Set TPOL
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, htim->Init.TIMPolarity);
			break;
		case HAL_TIM_MODE_CAP_AND_CMP:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			//Set TPOL
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, htim->Init.TIMPolarity);
			break;
		case HAL_TIM_MODE_PWM_DUAL:
			//Set ClockDivision
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, htim->Init.ClockDivision);
			//Set TPOL
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, htim->Init.TIMPolarity);
			//Write PWMReg
			WRITE_REG(htim->Instance->TMR_PWM, htim->Init.PWMReg);
			//Set PWMD
			MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PWMD_Msk, htim->Init.PWMDualDelay);
			break;
		default:
			return HAL_ERROR;
	}
	//Write Reload
	WRITE_REG(htim->Instance->TMR_RLD, htim->Init.Reload);
	//Set Mode
	MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TMODE_Msk, htim->Init.Mode);
	//Set COUNT
	WRITE_REG(htim->Instance->TMR_COUNT, 0x0);//for bug 7141

	htim->ErrorCode = HAL_TIM_ERROR_NONE;
	htim->State = HAL_TIM_STATE_READY;

	return HAL_OK;
}


/**
  * @brief  去初始化定时器.
  * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @retval 无
  */
HAL_StatusTypeDef HAL_TIM_DeInit(HAL_TIM_HandleTypeDef *htim)
{
	/* Check the CSP handle allocation */
	if(htim == NULL)
		return HAL_ERROR;

	htim->State = HAL_TIM_STATE_BUSY;

	//Set COUNT
	WRITE_REG(htim->Instance->TMR_COUNT, 0x0);
	//Set ClockDivision
	MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PRES_Msk, HAL_TIM_CLK_DIV_1);
	//Set TPOL
	MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk, HAL_TIM_Polarity_Reset);
	//Write PWMReg
	WRITE_REG(htim->Instance->TMR_PWM, 0x0);
	//Set PWMD
	MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_PWMD_Msk, HAL_TIM_DELAY_0_CYCLES);
	//Write Reload
	WRITE_REG(htim->Instance->TMR_RLD, 0xFFFFFFFF);
	//Set Mode
	MODIFY_REG(htim->Instance->TMR_CTL, TIMER_CTL_TMODE_Msk, HAL_TIM_MODE_ONE_SHOT);

	htim->ErrorCode = HAL_TIM_ERROR_NONE;
	htim->State = HAL_TIM_STATE_RESET;

	/* 释放程序锁 */
	__HAL_UNLOCK(htim);

	return HAL_OK;
}


/**
  * @brief  开启定时器.
  * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @retval 无
  */
HAL_StatusTypeDef HAL_TIM_Start(HAL_TIM_HandleTypeDef *htim)
{
	if(htim->State == HAL_TIM_STATE_RESET)
	{
		return HAL_ERROR;
	}
	
	if(htim->State != HAL_TIM_STATE_READY)
	{
		return HAL_BUSY;
	}

	htim->State = HAL_TIM_STATE_BUSY;

	SET_BIT(htim->Instance->TMR_CTL, TIMER_CTL_TEN_Msk);

	htim->State = HAL_TIM_STATE_READY;

	return HAL_OK;
}


/**
  * @brief  停止定时器.
  * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @retval 无
  */
HAL_StatusTypeDef HAL_TIM_Stop(HAL_TIM_HandleTypeDef *htim)
{
	if(htim->State == HAL_TIM_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if(htim->State != HAL_TIM_STATE_READY)
	{
		return HAL_BUSY;
	}

	htim->State = HAL_TIM_STATE_BUSY;

	CLEAR_BIT(htim->Instance->TMR_CTL, TIMER_CTL_TEN_Msk);
	WRITE_REG(htim->Instance->TMR_COUNT, 0);
	
	htim->State = HAL_TIM_STATE_READY;

	return HAL_OK;
}


/**
  * @brief  获取定时器Count寄存器中的值.
  * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @retval Count寄存器中的值.
  */
uint32_t HAL_TIM_GetCount(HAL_TIM_HandleTypeDef *htim)
{
	return READ_REG(htim->Instance->TMR_COUNT);
}


/**
  * @brief  获取定时器CaptureCount寄存器中的值.
  * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @retval CaptureCount寄存器中的值.
  */
uint32_t HAL_TIM_GetCaptureCount(HAL_TIM_HandleTypeDef *htim)
{
	return READ_REG(htim->Instance->TMR_PWM);
}


/**
  * @brief  设置定时器Count寄存器中的值.
  * @param  htim	指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @param	count	设置的定时器Count寄存器中的值.
  * @retval 无
  */
void HAL_TIM_SetCount(HAL_TIM_HandleTypeDef *htim, uint32_t count)
{
	WRITE_REG(htim->Instance->TMR_COUNT, count);
}


/**
  * @brief  设置定时器PWM寄存器值
  * @param  htim	指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @param	count	设置的定时器PWM寄存器值中的值.
  * @retval 无
  */
void HAL_TIM_SetPWMReg(HAL_TIM_HandleTypeDef *htim, uint32_t count)
{
	WRITE_REG(htim->Instance->TMR_PWM, count);
}

/**
  * @brief  定时器中断控制.
  * @param  htim 	指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @param	option	中断控制类型.详情参考 @ref HAL_TIMInt_TypeDef.
  *		参数可以是以下枚举值:
  *		@arg HAL_TIM_ALL_EVENTS:				所有定义的事件都会触发中断，包含重载，比较和输入事件
  *		@arg HAL_TIM_CAP_DEASSERTION_EVENTS:	仅输入事件触发中断
  *		@arg HAL_TIM_RELOAD_COMP_EVENTS:		仅重载，比较事件触发中断
  * @retval 无
  */
void HAL_TIM_IntControl(HAL_TIM_HandleTypeDef *htim, HAL_TIMInt_TypeDef option)
{
	MODIFY_REG(htim->Instance->TMR_CTL, 0x0060, option);
}


/**
  * @brief  Timer1中断服务函数.Timer1中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_TIM1_IRQHandler(void)
{

}


/**
  * @brief  Timer2中断服务函数.Timer2中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_TIM2_IRQHandler(void)
{

}


/**
  * @brief  Timer3中断服务函数.Timer3中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_TIM3_IRQHandler(void)
{

}


/**
  * @brief  Timer4中断服务函数.Timer4中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_TIM4_IRQHandler(void)
{

}


/**
  * @brief  注册定时器中断.
  * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
  * @retval 无
  */
void HAL_TIM_IT_REGISTER(HAL_TIM_HandleTypeDef *htim)
{
	if(HAL_TIM1 == htim->Instance)
	{
		HAL_NVIC_IntRegister(HAL_TIM1_IRQn, HAL_TIM1_IRQHandler, 1);
	}
	else if(HAL_TIM2 == htim->Instance)
	{
		HAL_NVIC_IntRegister(HAL_TIM2_IRQn, HAL_TIM2_IRQHandler, 1);
	}
	else if(HAL_TIM3 == htim->Instance)
	{
		HAL_NVIC_IntRegister(HAL_TIM3_IRQn, HAL_TIM3_IRQHandler, 1);
	}
	else if(HAL_TIM4 == htim->Instance)
	{
		HAL_NVIC_IntRegister(HAL_TIM4_IRQn, HAL_TIM4_IRQHandler, 1);
	}
	else
	{
		;
	}
}


/*
void HAL_TIM_SetReload(HAL_TIM_HandleTypeDef *htim, uint32_t Reload)
{
	CLEAR_BIT(htim->Instance->TMR_CTL, TIMER_CTL_TEN_Msk);
	WRITE_REG(htim->Instance->TMR_COUNT, 0);
	WRITE_REG(htim->Instance->TMR_RLD, Reload);
}

void HAL_TIM_SetPWMCritical(HAL_TIM_HandleTypeDef *htim, uint32_t pwm_val)
{
	WRITE_REG(htim->Instance->TMR_PWM, pwm_val);
}

void HAL_TIM_SetTPOL(HAL_TIM_HandleTypeDef *htim, uint8_t polarity)
{
	if(polarity)
	{
		SET_BIT(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk);
	}
	else
	{
		CLEAR_BIT(htim->Instance->TMR_CTL, TIMER_CTL_TPOL_Msk);
	}
}

void HAL_TIM_Complement_SetDelay(HAL_TIM_HandleTypeDef *htim, HAL_TIMDelay_TypeDef delay)
{
	htim->Instance->TMR_CTL = (htim->Instance->TMR_CTL & 0xfff1) | (delay << 1);
}

void HAL_TIM_Int_Control(HAL_TIM_HandleTypeDef *htim, HAL_TIMInt_TypeDef option)
{
	htim->Instance->TMR_CTL = (htim->Instance->TMR_CTL & 0xff9f) | (option << 5);
}

void HAL_TIM_SetCaptureCount(HAL_TIM_HandleTypeDef *htim, uint32_t count)
{
	WRITE_REG(htim->Instance->TMR_PWM, count);
}

uint8_t HAL_TIM_IsCaptureEVT(HAL_TIM_HandleTypeDef *htim)
{
	return READ_BIT(htim->Instance->TMR_CTL, TIMER_CTL_INPCAP_Msk);
}
*/


