/**
  ******************************************************************************
  * @file    hal_uart.c
  * @brief   This file contains HAL uart function prototype.
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "hal_uart.h"
#include "hal_cortex.h"
#include "xy_clk_config.h"
#include "hal_gpio.h"

/* Private macros ------------------------------------------------------------*/

#define IS_UART_WORD_LENGTH(LENGTH) (((LENGTH) == UART_WORDLENGTH_6B) || \
                                     ((LENGTH) == UART_WORDLENGTH_7B) || \
                                     ((LENGTH) == UART_WORDLENGTH_8B))

#define IS_UART_STOPBITS(STOPBITS) (((STOPBITS) == UART_STOPBITS_1) ||   \
                                    ((STOPBITS) == UART_STOPBITS_1_5) || \
                                    ((STOPBITS) == UART_STOPBITS_2))

#define IS_UART_PARITY(PARITY) (((PARITY) == UART_PARITY_EVEN) || \
                                ((PARITY) == UART_PARITY_ODD) ||  \
                                ((PARITY) == UART_PARITY_ZERO) || \
                                ((PARITY) == UART_PARITY_ONE) ||  \
                                ((PARITY) == UART_PARITY_NONE))

#define IS_UART_BAUDRATE(BAUDRATE) ((BAUDRATE) <= 921600U)


/**
  * @brief  This function handles UART Communication Timeout.
  * @param  huart  Pointer to a HAL_UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  Flag specifies the UART flag to check.
  * @param  Status The new Flag status (SET or RESET).
  * @param  Tickstart Tick start value
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_WaitOnFlagUntilTimeout(HAL_UART_HandleTypeDef *huart, uint32_t StatusFlag, HAL_FlagStatus Status, uint64_t Tickstart, uint32_t Timeout)
{
	/* Wait until flag is set */
	while ((HAL_UART_GET_FLAG(huart, StatusFlag) ? HAL_SET : HAL_RESET) == Status)
	{
		/* Check for the Timeout */
		if (Timeout != HAL_MAX_DELAY)
		{
			if ((Timeout == 0U) || ((HAL_GetTick() - Tickstart) > Timeout))
			{
				/* Disable PARE, TEMPTY and FRAME (Frame error, noise error, overrun error) interrupts for the interrupt process */
				SET_BIT(huart->Instance->HAL_UART_INT_DIS, (UART_CHNSTA_PARE | UART_CHNSTA_TEMPTY | UART_CHNSTA_FRAME));

				huart->gState = HAL_UART_STATE_READY;
				huart->RxState = HAL_UART_STATE_READY;

				/* Process Unlocked */
				__HAL_UNLOCK(huart);

				return HAL_TIMEOUT;
			}
		}
	}
	return HAL_OK;
}

/**
  * @brief  Configures the UART peripheral.
  * @param  huart  Pointer to a HAL_UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
static void UART_SetConfig(HAL_UART_HandleTypeDef *huart)
{
	unsigned long BDIV;
	unsigned long CD;
	unsigned long diff;
	unsigned long minBDIV = 0;
	unsigned long minDiff = 0x0FFFFFFF;
	unsigned long ulPclkCalculate;

	for (BDIV = 254; BDIV > 3; BDIV--)
	{
		CD = XY_PCLK / (huart->Init.BaudRate * (BDIV + 1));

		if (CD == 0 || CD >= 0x10000 || (BDIV + 1) * CD < 16)
		{
			continue;
		}
		ulPclkCalculate = huart->Init.BaudRate * (BDIV + 1) * CD;
		if (XY_PCLK >= ulPclkCalculate)
		{
			diff = XY_PCLK - ulPclkCalculate;
		}
		else
		{
			diff = ulPclkCalculate - XY_PCLK;
		}
		if (diff < minDiff)
		{
			minDiff = diff;
			minBDIV = BDIV;
		}
		if (diff == 0)
		{
			break;
		}
	}

	if (minDiff == 0x0FFFFFFF)
	{
	}

	HAL_UART_DISABLE(huart);

	CD = XY_PCLK / (huart->Init.BaudRate * (minBDIV + 1));
	huart->Instance->HAL_UART_BAUD_RATE = (CD & UART_Baudrate_CD_Msk);
	huart->Instance->HAL_UART_BAUD_DIV  = (minBDIV & UART_Baudrate_BDIV_Msk);

	// Set parity, data length, and number of stop bits.
	huart->Instance->HAL_UART_MODE = ((huart->Instance->HAL_UART_MODE & ~(UART_MODE_CHANNEL_MODE_BITS_Msk | UART_MODE_DATA_BITS_Msk | UART_MODE_PARITY_Msk | UART_MODE_STOP_BITS_Msk)) | huart->Init.WordLength | huart->Init.StopBits | huart->Init.Parity);

	huart->Instance->HAL_UART_TX_TRIGGER = 1;

	HAL_UART_ENABLE(huart);
}

/**
  * @brief  获取 UART 工作状态.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @retval HAL state.详情参考 @ref HAL_UART_StateTypeDef.
  */
HAL_UART_StateTypeDef HAL_UART_GetState(HAL_UART_HandleTypeDef *huart)
{
	uint32_t temp1 = 0x00U, temp2 = 0x00U;
	temp1 = huart->gState;
	temp2 = huart->RxState;

	return (HAL_UART_StateTypeDef)(temp1 | temp2);
}

/**
  * @brief 获取 UART 错误状态.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @retval UART 错误码.
  */
uint32_t HAL_UART_GetError(HAL_UART_HandleTypeDef *huart)
{
	return huart->ErrorCode;
}

/**
  * @brief  设置 UART 硬件Timeout中断的超时时间.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @param  timeout 超时时间值.
  * @retval none
  */
void HAL_UART_SetTimeout(HAL_UART_HandleTypeDef *huart, uint16_t timeout)
{
	uint32_t actual_reg_set;

	actual_reg_set = timeout / 4;
	actual_reg_set &= 0x000000FF;
	
	huart->Instance->HAL_UART_RX_TIMEOUT = actual_reg_set;
}

/**
  * @brief  设置 UART 发送FIFO阈值
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @param  TxLevel 发送FIFO阈值
  * @retval none
  */
void HAL_UART_SetTxFIFOLevel(HAL_UART_HandleTypeDef *huart, uint32_t TxLevel)
{
	if((TxLevel == 0) || (TxLevel > 63))
		return;
	huart->Instance->HAL_UART_TX_TRIGGER = TxLevel;
}

/**
  * @brief  设置 UART 接收FIFO阈值
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @param  RxLevel 接收FIFO阈值
  * @retval none
  */
void HAL_UART_SetRxFIFOLevel(HAL_UART_HandleTypeDef *huart, uint32_t RxLevel)
{
	if((RxLevel == 0) || (RxLevel > 63))
		return;
	huart->Instance->HAL_UART_RX_TRIGGER = RxLevel;
}

/**
  * @brief  初始化 UART.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_UART_Init(HAL_UART_HandleTypeDef *huart)
{
	/* Check the UART handle allocation */
	if (huart == NULL)
	{
		return HAL_ERROR;
	}

	if (huart->gState == HAL_UART_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		huart->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	if (IS_UART_BAUDRATE(huart->Init.BaudRate) && IS_UART_WORD_LENGTH(huart->Init.WordLength) && IS_UART_STOPBITS(huart->Init.StopBits) && IS_UART_PARITY(huart->Init.Parity) == 0)
	{
		return HAL_ERROR;
	}

	PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_UART1_EN);

	huart->gState = HAL_UART_STATE_BUSY;

	/* Set the UART Communication parameters */
	UART_SetConfig(huart);

	/* Enable the peripheral */
	HAL_UART_ENABLE(huart);

	/* Initialize the UART state */
	huart->ErrorCode = HAL_UART_ERROR_NONE;
	huart->gState = HAL_UART_STATE_READY;
	huart->RxState = HAL_UART_STATE_READY;

	return HAL_OK;
}

/**
  * @brief  去初始化 UART.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示去初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_UART_DeInit(HAL_UART_HandleTypeDef *huart)
{
	/* Check the UART handle allocation */
	if (huart == NULL)
	{
		return HAL_ERROR;
	}

	//未初始化则返回
	if (huart->gState == HAL_UART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	huart->gState = HAL_UART_STATE_BUSY;

	//清FIFO
	SET_BIT(huart->Instance->HAL_UART_CONTROL,UART_CTRL_RXRES);
	SET_BIT(huart->Instance->HAL_UART_CONTROL,UART_CTRL_TXRES);

	//向寄存器写入默认值
	(huart->Instance->HAL_UART_CONTROL) = 0x00000128;			/*!< UART 控制寄存器        */
	(huart->Instance->HAL_UART_MODE) = 0x0;					/*!< UART 模式寄存器        */
	(huart->Instance->HAL_UART_INT_ENA) = 0x0; 				/*!< UART 中断使能控制寄存器 */
	(huart->Instance->HAL_UART_INT_DIS) = 0xFFFFFFF;			/*!< UART 中断失能控制寄存器 */
	(huart->Instance->HAL_UART_INT_STAT) = 0x0;				/*!< UART 中断状态寄存器     */
	(huart->Instance->HAL_UART_BAUD_RATE) = 0x0;				/*!< UART 波特率控制寄存器   */
	(huart->Instance->HAL_UART_RX_TIMEOUT) = 0x0;				/*!< UART 接收超时控制寄存器 */
	(huart->Instance->HAL_UART_RX_TRIGGER) = 0x00000020;		/*!< UART 接收触发控制寄存器 */
	(huart->Instance->HAL_UART_MODEM_CTRL) = 0x0;				/*!< UART 模式控制寄存器     */
	(huart->Instance->HAL_UART_FIFO_DATA) = 0x0;				/*!< UART FIFO数据寄存器     */
	(huart->Instance->HAL_UART_BAUD_DIV) = 0x0000000F;			/*!< UART 波特率分频寄存器    */
	(huart->Instance->HAL_UART_FLOW_DELAY) = 0x0;				/*!< UART 寄存器              */
	(huart->Instance->HAL_UART_IRX_PW) = 0x0;					/*!< UART IR接收寄存器        */
	(huart->Instance->HAL_UART_ITX_PW) = 0x0;					/*!< UART IR发送寄存器        */
	(huart->Instance->HAL_UART_TX_TRIGGER) = 0x00000020;		/*!< UART 发送FIFO触发寄存器   */


	//释放外设上映射的PAD
	PeriPadAllocationStatusRemove(HAL_REMAP_UART_TXD);
	PeriPadAllocationStatusRemove(HAL_REMAP_UART_RTS);
	PeriPadAllocationStatusRemove(HAL_REMAP_UART_RXD);
	PeriPadAllocationStatusRemove(HAL_REMAP_UART_CTS);

	/* 失能外设 */
	HAL_UART_DISABLE(huart);

	/* 失能外设时钟 */
	PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_UART1_EN);

	huart->ErrorCode = HAL_UART_ERROR_NONE;
	huart->gState = HAL_UART_STATE_RESET;
	huart->RxState = HAL_UART_STATE_RESET;

	/* Process Unlock */
	__HAL_UNLOCK(huart);

	return HAL_OK;
}

/**
  * @brief  发送数据.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @param  pData 发送数据缓冲区指针.
  * @param  Size  发送数据字节长度.
  * @param  Timeout 超时时间（单位ms）; 如果Time_out取值为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出；.\n
  * 		如果Timeout设置为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数，huart结构体中的TxXferCount表示实际发送的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
  *       @retval  HAL_ERROR    ：表示入参错误
  *       @retval  HAL_BUSY     ：表示外设正在使用中
  *       @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
  */
HAL_StatusTypeDef HAL_UART_Transmit(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;

	if (huart->gState == HAL_UART_STATE_RESET)
  	{
		return HAL_ERROR;
	}

	/* Check that a Tx process is not already ongoing */
	if (huart->gState != HAL_UART_STATE_READY)
  	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	/* Process Locked */
	__HAL_LOCK(huart);

	/* Init tickstart for timeout managment */
	tickstart = HAL_GetTick();

	huart->ErrorCode = HAL_UART_ERROR_NONE;
	huart->gState = HAL_UART_STATE_BUSY_TX;
	huart->TxXferSize = Size;
	huart->TxXferCount = 0;

	/* Process Unlocked */
	__HAL_UNLOCK(huart);

	while (huart->TxXferSize > 0U)
	{
		huart->TxXferSize--;
		huart->TxXferCount++;
		if (UART_WaitOnFlagUntilTimeout(huart, UART_INTTYPE_TFUL, HAL_SET, tickstart, Timeout) != HAL_OK)
		{
			return HAL_TIMEOUT;
		}
		huart->Instance->HAL_UART_FIFO_DATA = (*pData++ & (uint8_t)0xFF);
	}

	if (UART_WaitOnFlagUntilTimeout(huart, UART_INTTYPE_TEMPTY, HAL_RESET, tickstart, Timeout) != HAL_OK)
	{
		return HAL_TIMEOUT;
	}

	//确保移位寄存器的数据也发送完成
	while(HAL_UART_GET_FLAG(huart, UART_CHNSTA_TACTIVE));

	/* At end of Tx process, restore huart->gState to Ready */
	huart->gState = HAL_UART_STATE_READY;

	return HAL_OK;
}

/**
  * @brief  接收数据.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @param  pData 接收数据缓冲区指针.
  * @param  Size  接收数据字节长度.
  * @param  Timeout 超时时间. 0 是不阻塞，接收FIFO空后立刻退出; HAL_MAX_DELAY 是一直阻塞，直到接收到指定数量的字节后才退出.\n
  * 		非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数，huart结构体中的RxXferCount表示实际接收的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过huart结构体中的RxXferCount来确定
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  *       @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
  */
HAL_StatusTypeDef HAL_UART_Receive(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;

	if (huart->RxState == HAL_UART_STATE_RESET)
  	{
		return HAL_ERROR;
	}

	/* Check that a Rx process is not already ongoing */
	if (huart->RxState != HAL_UART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	/* Process Locked */
	__HAL_LOCK(huart);

	/* Init tickstart for timeout managment */
	tickstart = HAL_GetTick();

	huart->ErrorCode = HAL_UART_ERROR_NONE;
	huart->RxState = HAL_UART_STATE_BUSY_RX;
	huart->RxXferSize = Size;
	huart->RxXferCount = 0;

	/* Process Unlocked */
	__HAL_UNLOCK(huart);

	/* Check the remain data to be received */
	while (huart->RxXferSize > 0U)
	{
		if (UART_WaitOnFlagUntilTimeout(huart, UART_INTTYPE_REMPTY, HAL_SET, tickstart, Timeout) != HAL_OK)
		{
			if(0 != Timeout)
			{
				return HAL_TIMEOUT;
			}
			else
			{
				return HAL_OK;
			}
		}
		huart->RxXferSize--;
		huart->RxXferCount++;
		*pData++ = (uint8_t)(huart->Instance->HAL_UART_FIFO_DATA & (uint8_t)0x00FF);
	}

	/* At end of Rx process, restore huart->RxState to Ready */
	huart->RxState = HAL_UART_STATE_READY;
	return HAL_OK;
}


/**
  * @brief  中断接收数据.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @param  pData 接收数据缓冲区指针.
  * @param  Size  接收数据字节长度.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：中断接收函数执行成功，等待在中断中接收指定数量的的数据
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  */
HAL_StatusTypeDef HAL_UART_Receive_IT(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
	if (huart->RxState == HAL_UART_STATE_RESET)
  	{
		return HAL_ERROR;
	}

	/* Check that a Rx process is not already ongoing */
	if (huart->RxState != HAL_UART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	/* Process Locked */
	__HAL_LOCK(huart);

	huart->pRxBuffPtr = pData;
	huart->RxXferSize = Size;
	huart->RxXferCount = 0;

	huart->ErrorCode = HAL_UART_ERROR_NONE;
	huart->RxState = HAL_UART_STATE_BUSY_RX;

	HAL_UART_SetTimeout(huart, 50);
	
	HAL_UART_SetRxFIFOLevel(huart, 1);

	HAL_NVIC_IntRegister(HAL_UART1_IRQn, HAL_UART_IRQHandler ,1);
	/* Enable the UART Data Register not empty Interrupt */

	int32_t rcv_dat = UARTCharGetNonBlocking((uint32_t)huart->Instance);
	while(rcv_dat != -1)
	{
		*(huart->pRxBuffPtr) = (char)rcv_dat;
		huart->pRxBuffPtr++;
		huart->RxXferSize--;
		huart->RxXferCount++;
		if(huart->RxXferSize == 0)
		{
			//FIFO中有遗留数据，遗留数据长度大于要接收的数据长度
			huart->RxState = HAL_UART_STATE_READY;
			HAL_UART_RxCpltCallback(huart);
			break;
		}
		else
		{
			rcv_dat = UARTCharGetNonBlocking((uint32_t)huart->Instance);
		}
	}
	//FIFO中有遗留数据，遗留数据长度小于要接收的数据长度
	if((huart->RxXferSize > 0) && (huart->RxXferCount > 0))
	{
		huart->RxState = HAL_UART_STATE_READY;
		HAL_UART_RxCpltCallback(huart);
	}
	//FIFO中没有遗留数据，打开UART_INTTYPE_RTRIG即可
	if(huart->RxXferCount == 0)
	{
		HAL_UART_ENABLE_IT(huart, UART_INTTYPE_RTRIG);
	}

	/* Process Unlocked */
	__HAL_UNLOCK(huart);

//	HAL_UART_ENABLE_IT(huart, UART_INTTYPE_ROVR);
//	HAL_UART_ENABLE_IT(huart, UART_INTTYPE_FRAME);
//	HAL_UART_ENABLE_IT(huart, UART_INTTYPE_PARE);
//	HAL_UART_ENABLE_IT(huart, UART_INTTYPE_TOVR);

	return HAL_OK;
}

/** 
  * @brief  获取 UART 中断使能状态.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @retval UART 中断使能状态.
  */
uint32_t HAL_UART_GetITEnable(HAL_UART_HandleTypeDef* huart)
{
    return(huart->Instance->HAL_UART_INT_MASK);
}

/** 
  * @brief  获取 UART 中断状态.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @retval UART 中断状态.
  */
uint32_t HAL_UART_GetITState(HAL_UART_HandleTypeDef* huart)
{
    return(huart->Instance->HAL_UART_INT_STAT);
}

/**
  * @brief  UART 错误中断回调函数.在对应的UART中断处理函数中调用.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @retval None
  */
__weak void HAL_UART_ErrorCallback(HAL_UART_HandleTypeDef* huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED_ARG(huart);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_UART_ErrorCallback could be implemented in the user file
   */
}

/**
  * @brief  UART 接收完成回调函数.在对应的UART中断处理函数中调用.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @retval None
  */
__weak void HAL_UART_RxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED_ARG(huart);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_UART_RxCpltCallback could be implemented in the user file
   */
}

/**
  * @brief  UART 中断处理函数.在UART中断服务中调用.
  * @param  huart 指向一个包含UART具体配置信息的 HAL_UART_HandleTypeDef 结构体的指针.详情参考 @ref HAL_UART_HandleTypeDef.
  * @retval None
  */
void UART_IRQHandler(HAL_UART_HandleTypeDef *huart)
{
	uint32_t ITState = HAL_UART_GetITState(huart);
	uint32_t ITEnable = HAL_UART_GetITEnable(huart);

	//发生错误中断
	if((HAL_RESET != (ITEnable & UART_INTTYPE_ROVR)) && (HAL_RESET != (ITState & UART_INTTYPE_ROVR)))
	{
		huart->ErrorCode |= HAL_UART_ERROR_ROVR;
	}
	if((HAL_RESET != (ITEnable & UART_INTTYPE_FRAME)) && (HAL_RESET != (ITState & UART_INTTYPE_FRAME)))
	{
		huart->ErrorCode |= HAL_UART_ERROR_FRAME;
	}
	if((HAL_RESET != (ITEnable & UART_INTTYPE_PARE)) && (HAL_RESET != (ITState & UART_INTTYPE_PARE)))
	{
		huart->ErrorCode |= HAL_UART_ERROR_PARE;
	}
	if((HAL_RESET != (ITEnable & UART_INTTYPE_TOVR)) && (HAL_RESET != (ITState & UART_INTTYPE_TOVR)))
	{
		huart->ErrorCode |= HAL_UART_ERROR_TOVR;
	}

	if (huart->ErrorCode != HAL_UART_ERROR_NONE)
	{
		HAL_UART_ErrorCallback(huart);
		return;
	}

	//没有发生错误中断,UART_INTTYPE_RTRIG中断，接收数据
	if((HAL_RESET != (ITEnable & UART_INTTYPE_RTRIG)) && (HAL_RESET != (ITState & UART_INTTYPE_RTRIG)))
	{
		if(HAL_RESET == (ITEnable & UART_INTTYPE_TIMEOUT))
		{
			HAL_UART_SetRxFIFOLevel(huart, 60);
			HAL_UART_ENABLE_IT(huart, UART_INTTYPE_TIMEOUT);
		}
		int32_t rcv_dat = UARTCharGetNonBlocking((uint32_t)huart->Instance);
		while(rcv_dat != -1)
		{
			*(huart->pRxBuffPtr) = (char)rcv_dat;
			huart->pRxBuffPtr++;
			huart->RxXferSize--;
			huart->RxXferCount++;
			if(huart->RxXferSize == 0)
			{
				HAL_UART_DISABLE_IT(huart, UART_INTTYPE_TIMEOUT);
				HAL_UART_DISABLE_IT(huart, UART_INTTYPE_RTRIG);
				huart->RxState = HAL_UART_STATE_READY;
				HAL_UART_RxCpltCallback(huart);
				break;
			}
			else
			{
				rcv_dat = UARTCharGetNonBlocking((uint32_t)huart->Instance);
			}
		}
		return;
	}
	//UART_INTTYPE_TIMEOUT中断，接收数据
	if((HAL_RESET != (ITEnable & UART_INTTYPE_TIMEOUT)) && (HAL_RESET != (ITState & UART_INTTYPE_TIMEOUT)))
	{
		HAL_UART_DISABLE_IT(huart, UART_INTTYPE_TIMEOUT);
		HAL_UART_DISABLE_IT(huart, UART_INTTYPE_RTRIG);
		int32_t rcv_dat = UARTCharGetNonBlocking((uint32_t)huart->Instance);
		while(rcv_dat != -1)
		{
			*(huart->pRxBuffPtr) = (char)rcv_dat;
			huart->pRxBuffPtr++;
			huart->RxXferSize--;
			huart->RxXferCount++;
			if(huart->RxXferSize == 0)
			{
				break;
			}
			else
			{
				rcv_dat = UARTCharGetNonBlocking((uint32_t)huart->Instance);
			}
		}
		huart->RxState = HAL_UART_STATE_READY;
		HAL_UART_RxCpltCallback(huart);
		return;
	}

}

/**
  * @brief  UART 中断服务函数. UART中断触发时调用此函数.
  * @param  None
  * @retval None
  */
__weak void HAL_UART_IRQHandler(void)
{

}
