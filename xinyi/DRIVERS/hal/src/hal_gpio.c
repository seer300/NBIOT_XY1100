/**
  ******************************************************************************
  * @file    hal_gpio.c
  * @brief   此文件包含GPIO外设的函数定义等.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hal_gpio.h"
#include "hal_def.h"
#include "gpio.h"
#include "hw_prcm.h"





#define EXTI_MODE             0x10000000U
#define RISING_EDGE           0x00100000U
#define FALLING_EDGE          0x00200000U
#define RISING_FALLING        0x00400000U
#define HIGH_LEVEL            0x00800000U
#define GPIO_AF_MODE          0x00002000U
#define GPIO_OUTPUT_TYPE      0x00000010U
#define GPIO_IO_MODE          0x00000001U



/**
  * @brief  初始化GPIO.
  * @param  GPIO_Init 指向一个包含GPIO具体配置信息的 HAL_GPIO_InitTypeDef 结构体的指针.详情参考 @ref HAL_GPIO_InitTypeDef.
  * @retval 无
  */
void HAL_GPIO_Init(HAL_GPIO_InitTypeDef *GPIO_Init)
{
	PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_GPIO_EN);

	/*--------------------- GPIO Mode Configuration ------------------------*/
	/* In case of Alternate Peripheral function mode selection */
	if(GPIO_Init->Mode == GPIO_MODE_AF_PER)
	{
		for(uint8_t ucTemp = 0; ucTemp < 64; ucTemp++)
		{
			uint32_t ulValue = HWREG(GPIO_BASE + GPIO_PAD_SEL0 + ucTemp * 4);
			if((ulValue == GPIO_Init->Alternate) && (GPIO_Init->Pin != ucTemp))
			{
				if (!GPIOAllocationStatusGet(ucTemp))
				{
					GPIOConflictStatusClear(ucTemp);
				}
				GPIOPeripheralPad(0XFF, GPIO_Init->Pin);
			}
		}
		GPIOPeripheralPad(GPIO_Init->Alternate, GPIO_Init->Pin);
		GPIOPadModeSet(GPIO_Init->Pin, GPIO_MODE_HW, GPIO_CTL_PERIPHERAL);
		return ;
	}

	/* Activate the Pull-up or Pull down resistor for the current IO */
	if(GPIO_Init->Pull == GPIO_PULLUP)
	{
		GPIOPulldownDis(GPIO_Init->Pin);
		GPIOPullupEn(GPIO_Init->Pin);
	}
	else if(GPIO_Init->Pull == GPIO_PULLDOWN)
	{
		GPIOPullupDis(GPIO_Init->Pin);
		GPIOPulldownEn(GPIO_Init->Pin);
	}
	else
	{
		GPIOPullupDis(GPIO_Init->Pin);
		GPIOPulldownDis(GPIO_Init->Pin);
	}

	/* In case of Alternate or GPIO function mode selection */
	if((GPIO_Init->Mode & GPIO_AF_MODE) == GPIO_AF_MODE)
	{
		GPIOPadModeSet(GPIO_Init->Pin, GPIO_MODE_HW, GPIO_CTL_REGISTER);
		GPIOPeripheralPad(GPIO_Init->Alternate, GPIO_Init->Pin);
	}
	else
	{
		GPIOPadModeSet(GPIO_Init->Pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);
	}

	/* In case of Output or Input function mode selection */
	if(GPIO_Init->Mode == GPIO_MODE_INOUT)
	{
		GPIODirectionSet(GPIO_Init->Pin, GPIO_DIR_MODE_INOUT);
	}
	else if((GPIO_Init->Mode & GPIO_IO_MODE) == GPIO_IO_MODE)
	{
		GPIODirectionSet(GPIO_Init->Pin, GPIO_DIR_MODE_OUT);
		if((GPIO_Init->Mode & GPIO_OUTPUT_TYPE) == GPIO_OUTPUT_TYPE)
		{
			/*开漏输出*/
		}
		else
		{
			/*推挽输出*/
		}
	}
	else
	{
		GPIODirectionSet(GPIO_Init->Pin, GPIO_DIR_MODE_IN);
	}

	/*--------------------- EXTI Mode Configuration ------------------------*/
	/* Configure the External Interrupt for the current IO */
	if((GPIO_Init->Mode & EXTI_MODE) == EXTI_MODE)
	{
		if((GPIO_Init->Mode & HIGH_LEVEL) == HIGH_LEVEL)
		{
			GPIOIntTypeSet(GPIO_Init->Pin, GPIO_INT_LEVEL);
		}
		else if((GPIO_Init->Mode & RISING_FALLING) == RISING_FALLING)
		{
			GPIOIntTypeSet(GPIO_Init->Pin, GPIO_INT_EDGE);
			GPIOIntEdgeSet(GPIO_Init->Pin, GPIO_INT_EDGE_BOTH);
		}
		else if((GPIO_Init->Mode & FALLING_EDGE) == FALLING_EDGE)
		{
			GPIOIntTypeSet(GPIO_Init->Pin, GPIO_INT_EDGE);
			GPIOIntEdgeSet(GPIO_Init->Pin, GPIO_INT_EDGE_SINGLE);
			GPIOIntSingleEdgeSet(GPIO_Init->Pin, GPIO_INT_EDGE_FALL);
		}
		GPIOIntEnable(GPIO_Init->Pin);
	}
}


/**
  * @brief  去初始化GPIO.
  * @param  GPIO_Init 指向一个包含GPIO具体配置信息的 HAL_GPIO_InitTypeDef 结构体的指针.详情参考 @ref HAL_GPIO_InitTypeDef.
  * @retval 无
  */
void HAL_GPIO_DeInit(HAL_GPIOPin_TypeDef GPIO_Pin)
{ 
	/*------------------------- EXTI Mode Configuration --------------------*/
	GPIOIntDisable(GPIO_Pin);
	GPIOIntTypeSet(GPIO_Pin, GPIO_INT_EDGE);
	GPIOIntEdgeSet(GPIO_Pin, GPIO_INT_EDGE_SINGLE);
	GPIOIntSingleEdgeSet(GPIO_Pin, GPIO_INT_EDGE_FALL);

	/* Configure IO in GPIO Register mode */
	GPIOPadModeSet(GPIO_Pin, GPIO_MODE_GPIO, GPIO_CTL_REGISTER);

	/* Deactivate the Pull-up and Pull-down resistor for the current IO */
	GPIOPullupDis(GPIO_Pin);
	GPIOPulldownDis(GPIO_Pin);

	/* Configure IO Direction in Input Mode */
	GPIODirectionSet(GPIO_Pin, GPIO_DIR_MODE_IN);
}



/**
  * @brief  读取GPIO 引脚的电平状态.
  * @param  GPIO_Pin 具体的GPIO引脚.详情参考 @ref HAL_GPIOPin_TypeDef.
  * @retval 引脚状态.详情参考 @ref HAL_GPIO_PinState.
  */
HAL_GPIO_PinState HAL_GPIO_ReadPin(HAL_GPIOPin_TypeDef GPIO_Pin)
{
	uint32_t ulValue;
	uint32_t ulOffsetReg;
	uint32_t ulOffsetBit;

	if(GPIO_Pin < 32)
	{
		ulOffsetReg = 0;
		ulOffsetBit = 1 << GPIO_Pin;
	}
	else
	{
		ulOffsetReg = 4;
		ulOffsetBit = 1 << (GPIO_Pin & 0x1F);
	}

	if(((HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) & ulOffsetBit) > 0) && ((HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) & ulOffsetBit) > 0))
		ulValue = HWREG(GPIO_BASE + GPIO_DIN0 + ulOffsetReg); //GPIO_INDIRECTION;
	else
		ulValue = HWREG(GPIO_BASE + GPIO_DOUT0 + ulOffsetReg); //GPIO_OUTDIRECTION;

	return (ulValue & ulOffsetBit) ? 1 : 0;
}


/**
  * @brief  设置GPIO输出状态.
  * @param  GPIO_Pin 具体的GPIO引脚.详情参考 @ref HAL_GPIOPin_TypeDef.
  * @param  PinState 设置的输出状态.详情参考 @ref HAL_GPIO_PinState.
  *         参数可以是以下枚举值:
  *				@arg HAL_GPIO_PIN_RESET:	GPIO输出低电平
  *				@arg HAL_GPIO_PIN_SET:		GPIO输出高电平
  * @retval 无
  */
void HAL_GPIO_WritePin(HAL_GPIOPin_TypeDef GPIO_Pin, HAL_GPIO_PinState PinState)
{
	if(PinState != HAL_GPIO_PIN_RESET)
	{
		GPIOPinSet(GPIO_Pin);
	}
	else
	{
		GPIOPinClear(GPIO_Pin);
	}
}


/**
  * @brief  翻转GPIO引脚输出状态.
  * @param  GPIO_Pin 具体的GPIO引脚.详情参考 @ref HAL_GPIOPin_TypeDef.
  * @note	此函数只能用于GPIO模式设置为输出模式时
  * @retval 无
  */
void HAL_GPIO_TogglePin(HAL_GPIOPin_TypeDef GPIO_Pin)
{
	uint32_t ulValue;
	uint32_t ulOffsetReg;
	uint32_t ulOffsetBit;
	HAL_GPIO_PinState bitstatus;

	if(GPIO_Pin < 32)
	{
		ulOffsetReg = 0;
		ulOffsetBit = 1 << GPIO_Pin;
	}
	else
	{
		ulOffsetReg = 4;
		ulOffsetBit = 1 << (GPIO_Pin & 0x1F);
	}

	ulValue = HWREG(GPIO_BASE + GPIO_DOUT0 + ulOffsetReg);    
    bitstatus = (ulValue & ulOffsetBit) ? 1 : 0;
	
	if (bitstatus)
	{
		GPIOPinClear(GPIO_Pin);
	}
	else
	{
		GPIOPinSet(GPIO_Pin);
	}
}


/**
  * @brief  GPIO中断控制.
  * @param  GPIO_Pin 具体的GPIO引脚.详情参考 @ref HAL_GPIOPin_TypeDef.
  * @param  NewState 使能或者失能GPIO中断.详情参考 @ref HAL_FunctionalState.
  * 		参数可以是以下枚举值:
  *				@arg HAL_ENABLE：	使能GPIO中断
  *				@arg HAL_DISABLE:	失能GPIO中断
  * @retval 无
  */
void HAL_GPIO_IntCtl(HAL_GPIOPin_TypeDef GPIOPin, HAL_FunctionalState NewState)
{
	if(NewState != HAL_DISABLE)
		GPIOIntEnable(GPIOPin);
	else
		GPIOIntDisable(GPIOPin);
}


/**
  * @brief  GPIO中断服务函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_GPIO_IRQHandler(void)
{

}


/**
  * @brief  读取并清除GPIO寄存器中断值.
  * @param  GPIO_Pin 具体的GPIO引脚.详情参考 @ref HAL_GPIOPin_TypeDef.
  * @note	1.多个GPIO发生中断时，通过此函数获取GPIO中断状态寄存器的值判断具体触发中断的GPIO.\n
  * 		2.仅清除中断时也通过调用此函数，参数填相应的GPIO，如果多个GPIO设置了中断，则填其中任意一个GPIO即可.
  * @retval GPIO中断状态寄存器的值
  */
uint32_t HAL_GPIO_ReadAndClearIntFlag(HAL_GPIOPin_TypeDef GPIOPin)
{
	return GPIOIntStatusGet(GPIOPin);
}


/**
  * @brief  注册GPIO中断.用户使用GPIO中断时需调用此函数注册中断.
  * @param  无
  * @retval 无
  */
void HAL_GPIO_IT_REGISTER(void)
{
	HAL_NVIC_IntRegister(HAL_GPIO_IRQn, HAL_GPIO_IRQHandler, 1);
}

/**
  * @brief	  获取GPIO引脚的输入输出方向
  * @param   PIO_Pin 具体的GPIO引脚.详情参考 @ref HAL_GPIOPin_TypeDef.
  * @retval  GPIO的输入输出状态，HAL_GPIO_INDIRECTION：输入，HAL_GPIO_OUTDIRECTION：输出；
  *   */

HAL_GPIO_DIRECTION HAL_GPIO_GetDirection(HAL_GPIOPin_TypeDef GPIOPin)
{
	uint32_t ulOffsetReg;
	uint32_t ulOffsetBit;


	if(GPIOPin < 32)
	{
		ulOffsetReg = 0;
		ulOffsetBit = 1 << GPIOPin;
	}
	else
	{
		ulOffsetReg = 4;
		ulOffsetBit = 1 << (GPIOPin & 0x1F);
	}

	if(((HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) & ulOffsetBit) > 0) && ((HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) & ulOffsetBit) > 0))
		return HAL_GPIO_INDIRECTION;
	else
		return HAL_GPIO_OUTDIRECTION;
}



/**
  * @brief  释放已分配的外设，进入未分配状态
  * @param  无
  * @retval 无
  */
void PeriPadAllocationStatusRemove(unsigned char ucPeriPad)
{
        for(uint8_t ucTemp = 0; ucTemp < 64; ucTemp++)
        {
                uint32_t ulValue = HWREG(GPIO_BASE + GPIO_PAD_SEL0 + ucTemp * 4);
                if((ulValue == ucPeriPad))
                {
                    GPIOPeripheralPad(0XFF, ucTemp);
                }
        }
}


