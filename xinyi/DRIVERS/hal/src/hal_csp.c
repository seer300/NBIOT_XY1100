/**
  ******************************************************************************
  * @file    hal_csp.c
  * @brief   此文件包含CSP外设的函数定义等.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hal_csp.h"
#include "hal_cortex.h"
#include "hw_types.h"
#include "hw_csp.h"
#include "csp.h"
#include "hal_def.h"
#include "xy_clk_config.h"
#include "hal_gpio.h"

#define  CUR_SYSTEM_PCLK    XY_PCLK


static void CSPMode1PinSet(unsigned long ulBase, unsigned long mode_mask, unsigned long mode)
{
	HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~mode_mask) | mode;
}

static void __HAL_CSP_UART_Init(HAL_CSP_TypeDef CSPx, uint32_t Baudrate, HAL_CSPDataBits_TypeDef DataBits, HAL_CSPStopBits_TypeDef StopBits)
{
	CSPUARTModeSet((uint32_t)CSPx, CUR_SYSTEM_PCLK, Baudrate, DataBits, StopBits);
	CSPDisable((uint32_t)CSPx);
}

static void __HAL_CSP_IRDA_Init(HAL_CSP_TypeDef CSPx, uint32_t Baudrate, HAL_CSPDataBits_TypeDef DataBits, HAL_CSPStopBits_TypeDef StopBits, unsigned long IdelLevel, unsigned long DataWidth)
{
    switch(DataWidth)
    {
        case HAL_IRDA_DATA_WIDTH_1_6:
            DataWidth = CSP_MODE2_IRDA_DATA_WIDTH_1_6;
            break;

        case HAL_IRDA_DATA_WIDTH_3_16:
            DataWidth = CSP_MODE2_IRDA_DATA_WIDTH_3_16;
            break;

        default:
            break;
    }

    switch(IdelLevel)
    {
        case HAL_IRDA_IDEL_LEVEL_H:
            IdelLevel = CSP_MODE1_IrDA_IDLE_LEVEL_HIGH;
            break;

        case HAL_IRDA_IDEL_LEVEL_L:
            IdelLevel = CSP_MODE1_IrDA_IDLE_LEVEL_LOW;
            break;

        default:
            break;
    }

    CSPIrdaMode((uint32_t)CSPx, CUR_SYSTEM_PCLK, Baudrate, DataBits, StopBits,IdelLevel,DataWidth);
    CSPDisable((uint32_t)CSPx);
}

static void __HAL_CSP_SPI_Init(HAL_CSP_TypeDef CSPx, HAL_CSPWorkMode_TypeDef WorkMode, HAL_CSPCommunicationMode_TypeDef CommunicationMode, unsigned long SCLK)
{
	// clear all pending uart interrupts
    HWREG((uint32_t)CSPx + CSP_INT_STATUS) = CSP_INT_ALL;

	CSPOperationModeSet((uint32_t)CSPx, CSP_MODE1_SYNC);

	if(WorkMode == HAL_MASTER_MODE)
	{
		CSPClockModeSet((uint32_t)CSPx, CSP_MODE1_CLOCK_MODE_Master);

		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_CLK_PIN_MODE_Msk, CSP_MODE1_CLK_PIN_MODE_CSP);
		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_PIN_MODE_Msk, CSP_MODE1_RXD_PIN_MODE_CSP);
		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_PIN_MODE_Msk, CSP_MODE1_TXD_PIN_MODE_CSP);

		CSPClockIdleModeSet((uint32_t)CSPx, CSP_MODE1_CLK_IDLE_KEEP_LEVEL);

		switch(CommunicationMode)
		{
			case HAL_MODE_0:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_RISE);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_FALL);
			CSPClockIdleLevelSet((uint32_t)CSPx, CSP_MODE1_CLK_IDLE_LOGIC0);
			break;
			case HAL_MODE_1:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_FALL);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_RISE);
			CSPClockIdleLevelSet((uint32_t)CSPx, CSP_MODE1_CLK_IDLE_LOGIC0);
			break;

			case HAL_MODE_2:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_FALL);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_RISE);
			CSPClockIdleLevelSet((uint32_t)CSPx, CSP_MODE1_CLK_IDLE_LOGIC1);
			break;

			case HAL_MODE_3:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_RISE);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_FALL);
			CSPClockIdleLevelSet((uint32_t)CSPx, CSP_MODE1_CLK_IDLE_LOGIC1);
			break;

			default: 
			break;
		}		

		HWREG((uint32_t)CSPx + CSP_MODE2) |= 0x101;
		CSPClockSet((uint32_t)CSPx, CSP_MODE_SPI, CUR_SYSTEM_PCLK, SCLK);
	}
	else
	{
		CSPClockModeSet((uint32_t)CSPx, CSP_MODE1_CLOCK_MODE_Slave);

		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_CLK_PIN_MODE_Msk, CSP_MODE1_CLK_PIN_MODE_CSP);
		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_PIN_MODE_Msk, CSP_MODE1_RXD_PIN_MODE_CSP);
		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_PIN_MODE_Msk, CSP_MODE1_TXD_PIN_MODE_CSP);
		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_SCLK_IO_MODE_Msk, CSP_MODE1_SCLK_IO_MODE_Input);
		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_IO_MODE_Msk, CSP_MODE1_RXD_IO_MODE_Input);
		CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_IO_MODE_Msk, CSP_MODE1_TXD_IO_MODE_Output);

		switch(CommunicationMode)
		{
			case HAL_MODE_0:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_RISE);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_FALL);
			break;

			case HAL_MODE_1:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_FALL);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_RISE);
			break;

			case HAL_MODE_2:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_FALL);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_RISE);
			break;

			case HAL_MODE_3:
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_RXD_ACT_EDGE_Msk, CSP_MODE1_RXD_ACT_RISE);
			CSPMode1PinSet((uint32_t)CSPx, CSP_MODE1_TXD_ACT_EDGE_Msk, CSP_MODE1_TXD_ACT_FALL);
			break;

			default: 
			break;
		}		

		HWREG((uint32_t)CSPx + CSP_MODE2) |= 0x180001;
		CSPClockSet((uint32_t)CSPx, CSP_MODE_SPI, CUR_SYSTEM_PCLK, SCLK);
	}
	
    HWREG((uint32_t)CSPx + CSP_TX_DMA_IO_CTRL) = CSP_TX_DMA_IO_CTRL_IO_MODE;
    HWREG((uint32_t)CSPx + CSP_RX_DMA_IO_CTRL) = CSP_RX_DMA_IO_CTRL_IO_MODE;
    HWREG((uint32_t)CSPx + CSP_TX_DMA_IO_LEN) = 0;
    HWREG((uint32_t)CSPx + CSP_RX_DMA_IO_LEN) = 0;
    
    // CSP FIFO Enable
    HWREG((uint32_t)CSPx + CSP_TX_RX_ENABLE) = (CSP_TX_RX_ENABLE_TX_ENA | CSP_TX_RX_ENABLE_RX_ENA);

    // Fifo Reset
    HWREG((uint32_t)CSPx + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_RESET;
    HWREG((uint32_t)CSPx + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_RESET;

    // Fifo Start
    HWREG((uint32_t)CSPx + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_START;
    HWREG((uint32_t)CSPx + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_START;

	CSPDisable((uint32_t)CSPx);
}

/**
  * @brief  获取 CSP 工作状态.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval HAL state.详情参考 @ref HAL_CSP_StateTypeDef
  */
HAL_CSP_StateTypeDef HAL_CSP_GetState(HAL_CSP_HandleTypeDef *hcsp)
{
	uint32_t temp1 = 0x00U;
	uint32_t temp2 = 0x00U;
	temp1 = hcsp->gState;
	temp2 = hcsp->RxState;

	return (HAL_CSP_StateTypeDef)(temp1 | temp2);
}

/**
  * @brief  获取 CSP 错误状态.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval CSP 错误码
  */
uint32_t HAL_CSP_GetError(HAL_CSP_HandleTypeDef *hcsp)
{
	return hcsp->ErrorCode;
}

/**
  * @brief  CSP的SPI模式下，使能NSS的手动控制模式.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  NewState
  * 	@arg	HAL_ENABLE	使能NSS为手动控制模式
  * 	@arg	HAL_DISABLE	失能NSS为手动控制模式，即NSS由硬件自动控制，无需用户通过API拉高拉低NSS
  * @retval 无
  */
void HAL_CSP_SPI_SetNSSManual(HAL_CSP_HandleTypeDef *hcsp, HAL_FunctionalState NewState)
{
	if(NewState == HAL_ENABLE)
	{
		HWREG((uint32_t)hcsp->Instance + CSP_MODE1) |= 0x4000;
	}
	else
	{
		HWREG((uint32_t)hcsp->Instance + CSP_MODE1) &= ~0x4000;
	}
}

/**
  * @brief  设置CSP的SPI模式下，发送帧的配置参数.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  tx_data_length		一帧的数据长度.一般默认为 8 即可.
  * @param  tx_delay	接收延时.一般默认为 1 即可.
  * @param  idle		空闲延时.一般默认为 1 即可.
  * @retval 无
  */
void HAL_CSP_SPI_SetTxFrame(HAL_CSP_HandleTypeDef *hcsp, uint8_t tx_data_length, uint8_t tx_delay, uint8_t idle)
{
	HWREG((uint32_t)hcsp->Instance + CSP_TX_FRAME_CTRL) = ((HWREG((uint32_t)hcsp->Instance + CSP_TX_FRAME_CTRL) & ~(CSP_TX_FRAME_CTRL_TX_DATA_LEN_Msk)) | (tx_data_length - 1)) \
		| ((HWREG((uint32_t)hcsp->Instance + CSP_TX_FRAME_CTRL) & ~(CSP_TX_FRAME_CTRL_TX_SYNC_LEN_Msk)) | ((tx_data_length + tx_delay -1) << CSP_TX_FRAME_CTRL_TX_SYNC_LEN_Pos)) \
		| ((HWREG((uint32_t)hcsp->Instance + CSP_TX_FRAME_CTRL) & ~(CSP_TX_FRAME_CTRL_TX_FRAME_LEN_Msk)) | ((tx_data_length + tx_delay + idle - 1) << CSP_TX_FRAME_CTRL_TX_FRAME_LEN_Pos)) \
		| ((HWREG((uint32_t)hcsp->Instance + CSP_TX_FRAME_CTRL) & ~(CSP_TX_FRAME_CTRL_TX_SHIFTER_LEN_Msk)) | ((tx_data_length - 1) << CSP_TX_FRAME_CTRL_TX_SHIFTER_LEN_Pos));
}

/**
  * @brief  设置CSP的SPI模式下，接收帧的配置参数.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  rx_data_length	一帧的数据长度.一般默认为 8 即可.
  * @param  rx_delay	接收延时.一般默认为 1 即可.
  * @param  idle		空闲延时.一般默认为 1 即可.
  * @retval 无
  */
void HAL_CSP_SPI_SetRxFrame(HAL_CSP_HandleTypeDef *hcsp, uint8_t rx_data_length, uint8_t rx_delay, uint8_t idle)
{
	HWREG((uint32_t)hcsp->Instance + CSP_RX_FRAME_CTRL) = ((HWREG((uint32_t)hcsp->Instance + CSP_RX_FRAME_CTRL) & ~(CSP_RX_FRAME_CTRL_RX_DATA_LEN_Msk)) | (rx_data_length - 1)) \
		| ((HWREG((uint32_t)hcsp->Instance + CSP_RX_FRAME_CTRL) & ~(CSP_RX_FRAME_CTRL_RX_FRAME_LEN_Msk)) | ((rx_data_length + rx_delay + idle - 1) << CSP_RX_FRAME_CTRL_RX_FRAME_LEN_Pos)) \
		| ((HWREG((uint32_t)hcsp->Instance + CSP_RX_FRAME_CTRL) & ~(CSP_RX_FRAME_CTRL_RX_SHIFTER_LEN_Msk)) | ((rx_data_length - 1) << CSP_RX_FRAME_CTRL_RX_SHIFTER_LEN_Pos));
}

/**
  * @brief  设置CSP的UART模式下，Timeout中断的超时时间.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  timeout	超时时间
  * @retval 无
  */
void HAL_CSP_UART_SetTimeout(HAL_CSP_HandleTypeDef *hcsp, uint16_t timeout)
{
	uint32_t reg;
	reg = HWREG((uint32_t)hcsp->Instance + CSP_AYSNC_PARAM_REG);
	HWREG((uint32_t)hcsp->Instance + CSP_AYSNC_PARAM_REG) = (reg & 0xFFFF0000) | timeout;
}

/**
  * @brief  设置CSP发送和接收的FIFO深度.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  TxLevel	发送深度
  * @param  RxLevel	接收深度
  * @retval 无
  */
void HAL_CSP_SetFIFOLevel(HAL_CSP_HandleTypeDef *hcsp, uint32_t TxLevel, uint32_t RxLevel)
{
	if((TxLevel == 0) || (RxLevel == 0))
		return;
	if((TxLevel > 128) || (RxLevel > 128))
		return;
	CSPFIFOLevelSet((uint32_t)hcsp->Instance, (TxLevel-1)<<2, (RxLevel-1)<<2);
}

/**
  * @brief  获取CSP FIFO状态.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param	FIFO_State	FIFO状态类型.详情参考 @ref CSPFIFO_TypeDef
  * @retval FIFO_State 为CSP_TX_FIFO_LEVEL和CSP_RX_FIFO_LEVEL时返回FIFO内数据长度，其余则返回FIFO_State真假状态
  */
uint8_t HAL_CSP_GetFIFOState(HAL_CSP_HandleTypeDef *hcsp, HAL_CSPFIFO_TypeDef FIFO_State)
{
	uint32_t reg;

	if(FIFO_State & 0x1000)
	{
		reg = HWREG((uint32_t)hcsp->Instance + CSP_RX_FIFO_STATUS);
	}
	else
	{
		reg = HWREG((uint32_t)hcsp->Instance + CSP_TX_FIFO_STATUS);
	}
	FIFO_State &= 0x01FF;

	if(FIFO_State == 0)
	{
		return (reg & 0x7F);
	}
	else
	{
		return (reg & FIFO_State) ? 1 : 0;
	}
}

/**
  * @brief  CSP发送和接收使能控制.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  TxRxEn	TX和RX使能控制
  * 	@arg	CSP_DIS_RX_TX，失能TX和RX
  *		@arg	CSP_EN_TX_ONLY，只使能TX
  *		@arg	CSP_EN_RX_ONLY，只使能RX
  *		@arg	CSP_EN_RX_TX，使能TX和RX
  * @retval 无
  */
void HAL_CSP_CtrlTxRx(HAL_CSP_HandleTypeDef *hcsp, HAL_CSPRxTx_TypeDef TxRxEn)
{
	HWREG((uint32_t)hcsp->Instance + CSP_TX_RX_ENABLE) = TxRxEn;
}

/**
  * @brief  CSP使能控制.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  NewState
  * 	@arg	HAL_ENABLE	使能CSP
  * 	@arg	HAL_DISABLE	失能CSP
  * @retval 无
  */
void HAL_CSP_Ctrl(HAL_CSP_HandleTypeDef *hcsp, HAL_FunctionalState NewState)
{
	if(NewState != HAL_DISABLE)
		CSPEnable((uint32_t)hcsp->Instance);
	else
		CSPDisable((uint32_t)hcsp->Instance);
}

/**
  * @brief  CSP中断使能控制.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param	IntType		中断类型.详情参考 @ref HAL_CSPInt_TypeDef
  * @param  NewState	使能状态
  * 	@arg	HAL_ENABLE	使能中断
  * 	@arg	HAL_DISABLE	失能中断
  * @retval 无
  */
void HAL_CSP_CtrlInt(HAL_CSP_HandleTypeDef *hcsp, HAL_CSPInt_TypeDef IntType, HAL_FunctionalState NewState)
{
	if(NewState != HAL_DISABLE)
		CSPIntEnable((uint32_t)hcsp->Instance, IntType);
	else
		CSPIntDisable((uint32_t)hcsp->Instance, IntType);
}

/**
  * @brief  清除CSP中断状态.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param	IntType		中断类型.详情参考 @ref HAL_CSPInt_TypeDef
  * @retval 无
  */
void HAL_CSP_ClearIntState(HAL_CSP_HandleTypeDef *hcsp, HAL_CSPInt_TypeDef IntType)
{
	CSPIntClear((uint32_t)hcsp->Instance, IntType);
}

/**
  * @brief  初始化CSP为IRDA.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_CSP_IRDA_Init(HAL_CSP_HandleTypeDef *hcsp)
{
    /* Check the CSP handle allocation */
    if(hcsp == NULL)
    {
    	return HAL_ERROR;
    }

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		hcsp->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}


    hcsp->gState = HAL_CSP_STATE_BUSY;

    /* Enable the CSP Peripheral Clock */
    if(hcsp->Instance == HAL_CSP1)
        PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP1_EN);
    else if(hcsp->Instance == HAL_CSP2)
        PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP2_EN);
    else if(hcsp->Instance == HAL_CSP3)
        PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP3_EN);
    else if(hcsp->Instance == HAL_CSP4)
        PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP4_EN);
    __HAL_CSP_IRDA_Init(hcsp->Instance, hcsp->Init.UART_BaudRate, hcsp->Init.UART_DataBits,
                        hcsp->Init.UART_StopBits, hcsp->Init.IRDA_Idellevel, hcsp->Init.IRDA_Datawidth);

    hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
    hcsp->gState = HAL_CSP_STATE_READY;
    hcsp->RxState = HAL_CSP_STATE_READY;

    return HAL_OK;
}

/**
  * @brief  初始化CSP为UART.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_CSP_UART_Init(HAL_CSP_HandleTypeDef *hcsp)
{
	/* Check the CSP handle allocation */
	if(hcsp == NULL)
		return HAL_ERROR;

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		hcsp->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	hcsp->gState = HAL_CSP_STATE_BUSY;

	/* Enable the CSP Peripheral Clock */
	if(hcsp->Instance == HAL_CSP1)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP1_EN);
	else if(hcsp->Instance == HAL_CSP2)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP2_EN);
	else if(hcsp->Instance == HAL_CSP3)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP3_EN);
	else if(hcsp->Instance == HAL_CSP4)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP4_EN);
	__HAL_CSP_UART_Init(hcsp->Instance, hcsp->Init.UART_BaudRate, hcsp->Init.UART_DataBits, hcsp->Init.UART_StopBits);

	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_READY;
	hcsp->RxState = HAL_CSP_STATE_READY;

	return HAL_OK;
}

/**
  * @brief  初始化CSP为SPI.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_CSP_SPI_Init(HAL_CSP_HandleTypeDef *hcsp)
{
	/* Check the CSP handle allocation */
	if(hcsp == NULL)
		return HAL_ERROR;

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		hcsp->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	hcsp->gState = HAL_CSP_STATE_BUSY;

	/* Enable the CSP Peripheral Clock */
	if(hcsp->Instance == HAL_CSP1)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP1_EN);
	else if(hcsp->Instance == HAL_CSP2)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP2_EN);
	else if(hcsp->Instance == HAL_CSP3)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP3_EN);
	else if(hcsp->Instance == HAL_CSP4)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_CSP4_EN);

	__HAL_CSP_SPI_Init(hcsp->Instance, hcsp->Init.SPI_Mode, hcsp->Init.SPI_WorkMode, hcsp->Init.SPI_Clock);

	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_READY;
	hcsp->RxState = HAL_CSP_STATE_READY;

	return HAL_OK;
}

/**
  * @brief  去初始化CSP.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示去初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_CSP_DeInit(HAL_CSP_HandleTypeDef* hcsp)
{
	/* Check the CSP handle allocation */
	if(hcsp == NULL)
		return HAL_ERROR;

	//未初始化则返回
	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hcsp->gState = HAL_CSP_STATE_BUSY;

	//重置发送与接收FIFO
	HAL_CSP_TXFIFO_RESET(hcsp);
	HAL_CSP_TXFIFO_NORMAL(hcsp);
	HAL_CSP_TXFIFO_START(hcsp);

	HAL_CSP_RXFIFO_RESET(hcsp);
	HAL_CSP_RXFIFO_NORMAL(hcsp);
	HAL_CSP_RXFIFO_START(hcsp);

	//向寄存器写入默认值
	(HWREG((uint32_t)hcsp->Instance + CSP_MODE1)) = 0x0;			
	(HWREG((uint32_t)hcsp->Instance + CSP_MODE2)) = 0x0;            
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_FRAME_CTRL)) = 0x0;    
	(HWREG((uint32_t)hcsp->Instance + CSP_RX_FRAME_CTRL)) = 0x0;    
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_RX_ENABLE)) = 0x0;	 
	(HWREG((uint32_t)hcsp->Instance + CSP_INT_ENABLE)) = 0x0;       
	(HWREG((uint32_t)hcsp->Instance + CSP_INT_STATUS)) = 0x0;       
	(HWREG((uint32_t)hcsp->Instance + CSP_PIN_IO_DATA)) = 0x0;      
	(HWREG((uint32_t)hcsp->Instance + CSP_AYSNC_PARAM_REG)) = 0xFFFF;  
	(HWREG((uint32_t)hcsp->Instance + CSP_IRDA_X_MODE_DIV)) = 0xAA;  
	(HWREG((uint32_t)hcsp->Instance + CSP_SM_CFG)) = 0x1C18;           
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_DMA_IO_CTRL)) = 0x0;   
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_DMA_IO_LEN)) = 0xFFFFFFFF;    
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_FIFO_CTRL)) = 0x0;     
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_FIFO_LEVEL_CHK)) = 0x0;
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_FIFO_OP)) = 0x0;       
	(HWREG((uint32_t)hcsp->Instance + CSP_TX_FIFO_DATA)) = 0x0;     
	(HWREG((uint32_t)hcsp->Instance + CSP_RX_DMA_IO_CTRL)) = 0x0;   
	(HWREG((uint32_t)hcsp->Instance + CSP_RX_DMA_IO_LEN)) = 0xFFFFFFFF;    
	(HWREG((uint32_t)hcsp->Instance + CSP_RX_FIFO_CTRL)) = 0x0;     
	(HWREG((uint32_t)hcsp->Instance + CSP_RX_FIFO_LEVEL_CHK)) = 0x0;
	(HWREG((uint32_t)hcsp->Instance + CSP_RX_FIFO_OP)) = 0x0;       

	//释放外设上映射的PAD
	switch(hcsp->Instance)
	{
		case HAL_CSP1:
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP1_SCLK);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP1_TXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP1_TFS);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP1_RXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP1_RFS);
			break;
		case HAL_CSP2:
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP2_SCLK);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP2_TXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP2_TFS);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP2_RXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP2_RFS);
			break;
		case HAL_CSP3:
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP3_SCLK);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP3_TXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP3_TFS);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP3_RXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP3_RFS);
			break;
		case HAL_CSP4:
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP4_SCLK);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP4_TXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP4_TFS);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP4_RXD);
			PeriPadAllocationStatusRemove(HAL_REMAP_CSP4_RFS);
			break;

		default:
			return HAL_ERROR;
		break;
	}

	//失能外设时钟
	if(hcsp->Instance == HAL_CSP1)
		PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_CSP1_EN);
	else if(hcsp->Instance == HAL_CSP2)
		PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_CSP2_EN);
	else if(hcsp->Instance == HAL_CSP3)
		PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_CSP3_EN);
	else if(hcsp->Instance == HAL_CSP4)
		PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_CSP4_EN);

	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_RESET;
	hcsp->RxState = HAL_CSP_STATE_RESET;

	/* Release Lock */
	__HAL_UNLOCK(hcsp);

	return HAL_OK;
}

/**
  * @brief  发送数据.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  pData 发送数据缓冲区指针.
  * @param  Size  发送数据字节长度.
  * @param  Timeout 超时时间（单位ms）; 如果Time_out取值为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出；.\n
  * 		如果Timeout设置为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数，hcsp结构体中的TxXferCount表示实际发送的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
  *       @retval  HAL_ERROR    ：表示入参错误
  *       @retval  HAL_BUSY     ：表示外设正在使用中
  *       @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
  */
HAL_StatusTypeDef HAL_CSP_Transmit(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if(hcsp->gState == HAL_CSP_STATE_RESET)
    {
    	return HAL_ERROR;
    }
    
    if(hcsp->gState != HAL_CSP_STATE_READY)
    {
    	return HAL_BUSY;
    }

    if((pData == NULL) || (Size == 0U) || (Timeout == 0))
    {
    	return HAL_ERROR;
    }

	/* Process Locked */
    __HAL_LOCK(hcsp);

    /* Init tickstart for timeout management*/
    tickstart = HAL_GetTick();

	/* Set the transaction information */
	hcsp->gState 		= HAL_CSP_STATE_BUSY_TX;
    hcsp->ErrorCode		= HAL_CSP_ERROR_NONE;
    hcsp->pTxBuffPtr	= (uint8_t *)pData;
    hcsp->TxXferSize	= Size;
    hcsp->TxXferCount	= 0;
	
	/* Process Unlocked */
	__HAL_UNLOCK(hcsp);
	//清除HAL_CSP_INTTYPE_TX_ALLOUT标志位，后面通过检查该标志位是否置1判断FIFO中的数据是否发送出去（包含移位寄存器中的数据）
	HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_TX_ALLOUT);

	while(hcsp->TxXferSize > 0)
	{
		
		if(CSPCharPutNonBlocking((uint32_t)hcsp->Instance, *(hcsp->pTxBuffPtr)))
		{
			hcsp->pTxBuffPtr++;
			hcsp->TxXferSize--;
			hcsp->TxXferCount++;
		}
		else
		{
			if((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick() - tickstart) >= Timeout))  
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
		}
	}

	error:
	//等待FIFO中的数据全部发送出去（包含移位寄存器中的数据）
	while(HAL_RESET == (HAL_CSP_GetITState(hcsp) & HAL_CSP_INTTYPE_TX_ALLOUT));
        HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_TX_ALLOUT);
	hcsp->gState = HAL_CSP_STATE_READY;

	return errorcode;
}

/**
  * @brief  接收数据.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  pData 接收数据缓冲区指针.
  * @param  Size  接收数据字节长度.
  * @param  Timeout 超时时间. 0 是不阻塞，接收FIFO空后立刻退出; HAL_MAX_DELAY 是一直阻塞，直到接收到指定数量的字节后才退出.\n
  * 		非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数，hcsp结构体中的RxXferCount表示实际接收的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过hcsp结构体中的RxXferCount来确定
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  *       @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
  */
HAL_StatusTypeDef HAL_CSP_Receive(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;
    
	if(hcsp->RxState == HAL_CSP_STATE_RESET)
    {
    	return HAL_ERROR;
    }

    if(hcsp->RxState != HAL_CSP_STATE_READY)
    {
    	return HAL_BUSY;
    }

    if((pData == NULL) || (Size == 0U))
    {
    	return HAL_ERROR;
    }

	/* Process Locked */
    __HAL_LOCK(hcsp);

    /* Init tickstart for timeout management*/
    tickstart = HAL_GetTick();

	/* Set the transaction information */
	hcsp->RxState 		= HAL_CSP_STATE_BUSY_RX;
    hcsp->ErrorCode		= HAL_CSP_ERROR_NONE;
    hcsp->pRxBuffPtr	= (uint8_t *)pData;
    hcsp->RxXferSize	= Size;
    hcsp->RxXferCount	= 0;

	/* Process Unlocked */
	__HAL_UNLOCK(hcsp);
	
	while(hcsp->RxXferSize > 0)
	{
		int32_t rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
		if(rcv_dat != -1)
		{
			*(hcsp->pRxBuffPtr) = (char)rcv_dat;
			hcsp->pRxBuffPtr++;
			hcsp->RxXferSize--;
			hcsp->RxXferCount++;
		}
		else
		{
			if ((Timeout != 0U) && ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick() - tickstart) >= Timeout)))  
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
			else if (Timeout == 0U)
			{
				errorcode = HAL_OK;
				goto error;
			}
		}
	}

	error:
	hcsp->RxState = HAL_CSP_STATE_READY;

	return errorcode;
}

/**
  * @brief  发送和接收数据.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  pTxData 发送缓冲区指针
  * @param  pRxData 接收缓冲区指针
  * @param  Size 发送和接收数据字节长度
  * @param  Timeout 超时时间.  HAL_MAX_DELAY 是一直阻塞，直到发送和接收到指定数量的字节后才退出.\n
  * 		非0非HAL_MAX_DELAY时，表示函数等待数据发送接收完成的最长时间，超时则退出函数，hcsp结构体中的TxXferCount表示实际发送的字节数，RxXferCount表示实际接收的字节数.
  *
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：发送和接收数据成功，表示指定时间内成功发送和接收指定数量的数据
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  *       @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送和接收指定数量的数据
  */
HAL_StatusTypeDef HAL_CSP_SPI_TransmitReceive(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;
	uint32_t txallowed = 1U;
    
	if(hcsp->gState == HAL_CSP_STATE_RESET)
    {
    	return HAL_ERROR;
    }

    if(hcsp->gState != HAL_CSP_STATE_READY)
    {
    	return HAL_BUSY;
    }

    if((pTxData == NULL) || (pRxData == NULL) || (Size == 0U) || (Timeout == 0))
    {
    	return HAL_ERROR;
    }

	/* Process Locked */
    __HAL_LOCK(hcsp);

    /* Init tickstart for timeout management*/
    tickstart = HAL_GetTick();

	/* Set the transaction information */
	hcsp->RxState 		= HAL_CSP_STATE_BUSY_RX;
    hcsp->ErrorCode		= HAL_CSP_ERROR_NONE;
    hcsp->pTxBuffPtr	= (uint8_t *)pTxData;
    hcsp->TxXferSize	= Size;
    hcsp->TxXferCount	= 0;
    hcsp->pRxBuffPtr	= (uint8_t *)pRxData;
    hcsp->RxXferSize	= Size;
    hcsp->RxXferCount	= 0;
	
	while((hcsp->TxXferSize > 0) || (hcsp->RxXferSize > 0))
	{
		if(1 == txallowed)
		{
			if(CSPCharPutNonBlocking((uint32_t)hcsp->Instance,  *(hcsp->pTxBuffPtr)))
			{
				hcsp->pTxBuffPtr++;
				hcsp->TxXferSize--;
				hcsp->TxXferCount++;
				txallowed = 0;
			}
			else
			{
				if ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick() - tickstart) >= Timeout)) 
				{
					errorcode = HAL_TIMEOUT;
					goto error;
				}

			}
		}
		
		if(0 == txallowed)
		{
			int32_t rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
			if(rcv_dat != -1)
			{
				*(hcsp->pRxBuffPtr) = (char)rcv_dat;
				hcsp->pRxBuffPtr++;
				hcsp->RxXferSize--;
				hcsp->RxXferCount++;
				/* Next Data is a Transmission (Tx). Tx is allowed */
				txallowed = 1U;
			}
			else
			{
				if ((Timeout != 0U) && ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick() - tickstart) >= Timeout)))  
				{
					errorcode = HAL_TIMEOUT;
					goto error;
				}

			}
		}	
	}

	error:
	/* Process Unlocked */
	__HAL_UNLOCK(hcsp);
	hcsp->RxState = HAL_CSP_STATE_READY;

	return errorcode;
}

/**
  * @brief  中断接收数据.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @param  pData 接收数据缓冲区指针.
  * @param  Size  接收数据字节长度.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：中断接收函数执行成功，等待在中断中接收指定数量的的数据
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  */
HAL_StatusTypeDef HAL_CSP_Receive_IT(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size)
{
	if(hcsp->RxState == HAL_CSP_STATE_RESET)
    {
    	return HAL_ERROR;
    }

    if(hcsp->RxState != HAL_CSP_STATE_READY)
    {
    	return HAL_BUSY;
    }

    if((pData == NULL) || (Size == 0U))
    {
    	return HAL_ERROR;
    }

	/* Process Locked */
    __HAL_LOCK(hcsp);

	/* Set the transaction information */
	hcsp->RxState 		= HAL_CSP_STATE_BUSY_RX;
    hcsp->ErrorCode		= HAL_CSP_ERROR_NONE;
    hcsp->pRxBuffPtr	= (uint8_t *)pData;
    hcsp->RxXferSize	= Size;
    hcsp->RxXferCount	= 0;

	if(hcsp->Instance == HAL_CSP1)
    	HAL_NVIC_IntRegister(HAL_CSP1_IRQn, HAL_CSP1_IRQHandler, 1);
    else if(hcsp->Instance == HAL_CSP2)
    	HAL_NVIC_IntRegister(HAL_CSP2_IRQn, HAL_CSP2_IRQHandler, 1);
	else if(hcsp->Instance == HAL_CSP3)
    	HAL_NVIC_IntRegister(HAL_CSP3_IRQn, HAL_CSP3_IRQHandler, 1);
	else if(hcsp->Instance == HAL_CSP4)
    	HAL_NVIC_IntRegister(HAL_CSP4_IRQn, HAL_CSP4_IRQHandler, 1);

	//设置接收阈值为120个字节，发送阈值32（默认值，暂无用处）。
	if(Size < 120)
	{
		HAL_CSP_SetFIFOLevel(hcsp, 32, Size);
	}
	else
	{
		HAL_CSP_SetFIFOLevel(hcsp, 32, 120);
	}
	//1个字节10个bit，此处设置timeout为50个bit的时间
	HAL_CSP_UART_SetTimeout(hcsp, 50);

	int32_t rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
	while(rcv_dat != -1)
	{
		*(hcsp->pRxBuffPtr) = (char)rcv_dat;
		hcsp->pRxBuffPtr++;
		hcsp->RxXferSize--;
		hcsp->RxXferCount++;
		if(hcsp->RxXferSize == 0)
		{
			hcsp->RxState = HAL_CSP_STATE_READY;
			HAL_CSP_RxCpltCallback(hcsp);
			break;
		}
		else
		{
			rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
		}
	}

	//FIFO中有遗留数据，遗留数据长度小于要接收的数据长度
	if((hcsp->RxXferSize > 0) && (hcsp->RxXferCount > 0))
	{
		hcsp->RxState = HAL_CSP_STATE_READY;
		HAL_CSP_RxCpltCallback(hcsp);
	}
	//FIFO中没有遗留数据，打开UART_INTTYPE_RTRIG即可
	if(hcsp->RxXferCount == 0)
	{
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_DONE);
		HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RX_DONE, HAL_ENABLE);
	}

	/* Process Unlocked */
	__HAL_UNLOCK(hcsp);

//	HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RX_OFLOW, HAL_ENABLE);
//	HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_TX_UFLOW, HAL_ENABLE);
//	HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_UART_FRM_ERR, HAL_ENABLE);

	return HAL_OK;
}

/**
  * @brief  获取CSP中断使能状态.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval CSP中断使能状态
  */
uint32_t HAL_CSP_GetITEnable(HAL_CSP_HandleTypeDef *hcsp)
{
	return (HWREG((uint32_t)hcsp->Instance + CSP_INT_ENABLE));
}

/**
  * @brief  获取CSP中断状态.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval CSP中断状态
  */
uint32_t HAL_CSP_GetITState(HAL_CSP_HandleTypeDef *hcsp)
{
	return CSPIntStatus((uint32_t)hcsp->Instance);
}

/**
  * @brief  CSP错误中断回调函数.在对应的CSP中断处理函数中调用
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
__weak void HAL_CSP_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED_ARG(hcsp);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_CSP_ErrorCallback could be implemented in the user file
   */
}

/**
  * @brief  CSP接收完成回调函数.在对应的CSP中断处理函数中调用.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
__weak void HAL_CSP_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED_ARG(hcsp);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_UART_RxCpltCallback could be implemented in the user file
   */
}

/**
  * @brief  CSP中断处理函数.在对应的中断服务函数中调用.
  * @param  hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
void CSP_IRQHandler(HAL_CSP_HandleTypeDef *hcsp)
{
	uint32_t ITState = HAL_CSP_GetITState(hcsp);
	uint32_t ITEnable = HAL_CSP_GetITEnable(hcsp);

	//发生错误中断
	if((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RX_OFLOW)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_OFLOW)))
	{
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_OFLOW);
		hcsp->ErrorCode |= HAL_CSP_ERROR_RX_OFLOW;
	}
	if((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_TX_UFLOW)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_TX_UFLOW)))
	{
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_TX_UFLOW);
		hcsp->ErrorCode |= HAL_CSP_ERROR_TX_UFLOW;
	}
	if((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_UART_FRM_ERR)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_UART_FRM_ERR)))
	{
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_UART_FRM_ERR);
		hcsp->ErrorCode |= HAL_CSP_ERROR_UART_FRM_ERR;
	}

	if (hcsp->ErrorCode != HAL_CSP_ERROR_NONE)
	{
		HAL_CSP_ErrorCallback(hcsp);
		return;
	}
	
	//没有发生错误中断,HAL_CSP_INTTYPE_RX_DONE中断，开启HAL_CSP_INTTYPE_RXFIFO_THD_REACH中断、HAL_CSP_INTTYPE_RX_TIMEOUT中断
	if((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RX_DONE)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_DONE)))
	{
		HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RX_DONE, HAL_DISABLE);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_DONE);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RXFIFO_THD_REACH);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT);
		HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RXFIFO_THD_REACH, HAL_ENABLE);
		HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT, HAL_ENABLE);
		return;
	}
	//HAL_CSP_INTTYPE_RXFIFO_THD_REACH中断，接收数据
	if((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RXFIFO_THD_REACH)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RXFIFO_THD_REACH)))
	{
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RXFIFO_THD_REACH);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_DONE);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT);

		int32_t rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
		while(rcv_dat != -1)
		{
			*(hcsp->pRxBuffPtr) = (char)rcv_dat;
			hcsp->pRxBuffPtr++;
			hcsp->RxXferSize--;
			hcsp->RxXferCount++;
			if(hcsp->RxXferSize == 0)
			{
				HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RXFIFO_THD_REACH, HAL_DISABLE);
				HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT, HAL_DISABLE);
				HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RXFIFO_THD_REACH);
				HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT);
				hcsp->RxState = HAL_CSP_STATE_READY;
				HAL_CSP_RxCpltCallback(hcsp);
				break;
			}
			else
			{
				rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
			}
		}
		return;
	}
	//HAL_CSP_INTTYPE_RX_TIMEOUT中断，接收数据
	if((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RX_TIMEOUT)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_TIMEOUT)))
	{
		HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RXFIFO_THD_REACH, HAL_DISABLE);
		HAL_CSP_CtrlInt(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT, HAL_DISABLE);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RX_DONE);
		HAL_CSP_ClearIntState(hcsp, HAL_CSP_INTTYPE_RXFIFO_THD_REACH);

		int32_t rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
		while(rcv_dat != -1)
		{
			*(hcsp->pRxBuffPtr) = (char)rcv_dat;
			hcsp->pRxBuffPtr++;
			hcsp->RxXferSize--;
			hcsp->RxXferCount++;
			if(hcsp->RxXferSize == 0)
			{
				break;
			}
			else
			{
				rcv_dat = CSPCharGetNonBlocking((uint32_t)hcsp->Instance);
			}
		}
		HAL_CSP_RxCpltCallback(hcsp);
		hcsp->RxState = HAL_CSP_STATE_READY;
		return;
	}

}

/**
  * @brief  CSP1 中断服务函数.CSP1 中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_CSP1_IRQHandler(void)
{

}

/**
  * @brief  CSP2 中断服务函数.CSP2 中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_CSP2_IRQHandler(void)
{

}

/**
  * @brief  CSP3 中断服务函数.CSP3 中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_CSP3_IRQHandler(void)
{

}

/**
  * @brief  CSP4 中断服务函数.CSP4 中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_CSP4_IRQHandler(void)
{

}


/** @brief  开始CSP发送FIFO.
  * @param	hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
void HAL_CSP_TXFIFO_START(HAL_CSP_HandleTypeDef *hcsp)
{

	SET_BIT(*((uint32_t*)(hcsp->Instance + CSP_TX_FIFO_OP)),CSP_TX_FIFO_OP_START);
}

/** @brief  重置CSP发送FIFO.
  * @param	hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
void HAL_CSP_TXFIFO_RESET(HAL_CSP_HandleTypeDef *hcsp)
{
	SET_BIT(*((uint32_t*)(hcsp->Instance + CSP_TX_FIFO_OP)),CSP_TX_FIFO_OP_RESET);
}

/** @brief  初始化重置CSP发送FIFO.
  * @param	hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
void HAL_CSP_TXFIFO_NORMAL(HAL_CSP_HandleTypeDef *hcsp)
{
	CLEAR_BIT(*((uint32_t*)(hcsp->Instance + CSP_TX_FIFO_OP)),CSP_TX_FIFO_OP_RESET);
}


/** @brief  开始CSP接收FIFO.
  * @param	hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
void HAL_CSP_RXFIFO_START(HAL_CSP_HandleTypeDef *hcsp)
{

	SET_BIT(*((uint32_t*)(hcsp->Instance + CSP_RX_FIFO_OP)),CSP_RX_FIFO_OP_START);
}

/** @brief  重置CSP接收FIFO.
  * @param	hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
void HAL_CSP_RXFIFO_RESET(HAL_CSP_HandleTypeDef *hcsp)
{
	SET_BIT(*((uint32_t*)(hcsp->Instance + CSP_RX_FIFO_OP)),CSP_RX_FIFO_OP_RESET);
}

/** @brief  初始化重置CSP接收FIFO.
  * @param	hcsp 指向一个包含CSP具体配置信息的HAL_CSP_HandleTypeDef 结构体的指针.详情参考 @ref HAL_CSP_HandleTypeDef.
  * @retval 无
  */
void HAL_CSP_RXFIFO_NORMAL(HAL_CSP_HandleTypeDef *hcsp)
{
	CLEAR_BIT(*((uint32_t*)(hcsp->Instance + CSP_RX_FIFO_OP)),CSP_RX_FIFO_OP_RESET);
}



