/**
  ******************************************************************************
  * @file    hal_spi.c
  * @brief   HAL库SPI.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hal_spi.h"
#include "hal_gpio.h"
#include "hw_spi.h"
#include "hw_prcm.h"
#include "replace.h"
#include "hw_memmap.h"


/**
  * @brief  获取SPIHandleStruct结构体中State状态参数.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval SPI结构体中State状态参数.详见HAL_SPI_StateTypeDef
  */
HAL_SPI_StateTypeDef HAL_SPI_GetState(HAL_SPI_HandleTypeDef *SPIHandleStruct)
{
  return SPIHandleStruct->State;
}

/**
  * @brief  获取SPIHandleStruct结构体中ErrorCode错误码参数.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval SPI结构体中ErrorCode错误码参数.详见HAL_SPI_Error_CodeTypeDef
  */
uint32_t HAL_SPI_GetError(HAL_SPI_HandleTypeDef *SPIHandleStruct)
{
  return SPIHandleStruct->ErrorCode;
}

/**
  * @brief  清空SPI发送FIFO.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval 无
  */
void HAL_SPI_Clean_TxFIFO(HAL_SPI_HandleTypeDef *SPIHandleStruct)
{
	HAL_SPI_TXFIFO_FLUSH(SPIHandleStruct);
	HAL_SPI_TXFIFO_NORMAL(SPIHandleStruct);
	HAL_SPI_TXFIFO_START(SPIHandleStruct);
}

/**
  * @brief  清空SPI接收FIFO.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval 无
  */
void HAL_SPI_Clean_RxFIFO(HAL_SPI_HandleTypeDef *SPIHandleStruct)
{
	HAL_SPI_RXFIFO_FLUSH(SPIHandleStruct);
	HAL_SPI_RXFIFO_NORMAL(SPIHandleStruct);
	HAL_SPI_RXFIFO_START(SPIHandleStruct);
}

/** @brief  检查SPI发送FIFO参数状态.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @param  TxFIFO_Flag：SPI发送FIFO参数状态类型.详见HAL_SPI_TXFIFO_FlagTypeDef.
  *            @arg HAL_SPI_TXFIFO_DATA_LEN: 发送FIFO剩余数据长度，当发送FIFO满标志为1时，该长度为0。
  *            @arg HAL_SPI_TXFIFO_TX_FULL: 发送FIFO满标志。
  *            @arg HAL_SPI_TXFIFO_TX_EMPTY: 发送FIFO空标志。
  * @retval SPI发送FIFO参数状态(HAL_SET or HAL_RESET).
  */
uint8_t HAL_SPI_Get_TxFIFO_Flag(HAL_SPI_HandleTypeDef* SPIHandleStruct,HAL_SPI_TXFIFO_FlagTypeDef TxFIFO_Flag)
{
	if(TxFIFO_Flag == HAL_SPI_TXFIFO_DATA_LEN)
		return (SPIHandleStruct->Instance->TXFIFO_STATUS & TxFIFO_Flag);
	else
		return ((SPIHandleStruct->Instance->TXFIFO_STATUS & TxFIFO_Flag) ? HAL_SET : HAL_RESET);
}

/** @brief  检查SPI接收FIFO参数状态.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @param  RxFIFO_Flag：SPI发送FIFO参数状态类型.详见HAL_SPI_RXFIFO_FlagTypeDef。
  *            @arg HAL_SPI_RXFIFO_DATA_LEN: 接收FIFO剩余数据长度，当接收FIFO满标志为1时，该长度为0。
  *            @arg HAL_SPI_RXFIFO_RX_FULL: 接收FIFO满标志。
  *            @arg HAL_SPI_RXFIFO_RX_EMPTY: 接收FIFO空标志。
  * @retval SPI接收FIFO参数状态(HAL_SET or HAL_RESET).
  */
uint8_t HAL_SPI_Get_RxFIFO_Flag(HAL_SPI_HandleTypeDef* SPIHandleStruct,HAL_SPI_RXFIFO_FlagTypeDef RxFIFO_Flag)
{
	if(RxFIFO_Flag == HAL_SPI_RXFIFO_DATA_LEN)
		return (SPIHandleStruct->Instance->RXFIFO_STATUS & RxFIFO_Flag);
	else
		return ((SPIHandleStruct->Instance->RXFIFO_STATUS & RxFIFO_Flag) ? HAL_SET : HAL_RESET);
}

/**
  * @brief  设置SPI片选引脚.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @param  CS_Pin：可以选择的SPI片选引脚，详见HAL_SPI_ChipSelTypeDef
  *            @arg SPI_NSS_PIN_1:设置SPI片选引脚为SPI_NSS_PIN_1.
  *            @arg SPI_NSS_PIN_2:设置SPI片选引脚为SPI_NSS_PIN_2.
  *            @arg SPI_NSS_NONE:设置SPI片选引脚为SPI_NSS_NONE.
  * @retval 无
  */
void HAL_SPI_ChipSelect(HAL_SPI_HandleTypeDef *SPIHandleStruct, HAL_SPI_ChipSelTypeDef CS_Pin)
{
	MODIFY_REG(SPIHandleStruct->Instance->CONFIG,0x3C00,CS_Pin);
}

/**
  * @brief  初始化SPI函数
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval HAL状态，详见HAL_StatusTypeDef
  */
HAL_StatusTypeDef HAL_SPI_Init(HAL_SPI_HandleTypeDef* SPIHandleStruct)
{
	/* 检查SPIHandleStruct是否合法 */
	if(SPIHandleStruct == NULL)
		return HAL_ERROR;

	if (SPIHandleStruct->State == HAL_SPI_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		SPIHandleStruct->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}


	SPIHandleStruct->State = HAL_SPI_STATE_BUSY;

	/* 使能SPI外设时钟 */
	PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_SPI_EN);

	/* 失能SPI外设功能 */
	HAL_SPI_DISABLE(SPIHandleStruct);

	/*----------------------- SPIx 寄存器配置 ---------------------*/
	if(SPIHandleStruct->Init.Mode == HAL_SPI_MODE_MASTER)
		HAL_SPI_CONFIG_EXP_CLK_MASTER(SPIHandleStruct);

	if(SPIHandleStruct->Init.Mode == HAL_SPI_MODE_SLAVE)
		HAL_SPI_CONFIG_EXP_CLK_SLAVE(SPIHandleStruct);

	HAL_SPI_Clean_TxFIFO(SPIHandleStruct);

	HAL_SPI_Clean_RxFIFO(SPIHandleStruct);

	if(SPIHandleStruct->Init.Mode == HAL_SPI_MODE_SLAVE)
	{
		HAL_SPI_SET_TXFIFO_THD(SPIHandleStruct,1);
		HAL_SPI_SET_RXFIFO_THD(SPIHandleStruct,1);
	}

	if((SPIHandleStruct->Init.NSS == HAL_SPI_NSS_SOFT) && (SPIHandleStruct->Init.Mode == HAL_SPI_MODE_MASTER))
		HAL_SPI_MANUAL_CS_ENABLE(SPIHandleStruct);
	else
		HAL_SPI_MANUAL_CS_DISABLE(SPIHandleStruct);

	if(SPIHandleStruct->Init.Mode == HAL_SPI_MODE_MASTER)
		HAL_SPI_CS_NOHARDWARE(SPIHandleStruct);

	/* 使能SPI外设功能 */
	HAL_SPI_ENABLE(SPIHandleStruct);

	SPIHandleStruct->ErrorCode = HAL_SPI_ERROR_NONE;
	SPIHandleStruct->State = HAL_SPI_STATE_READY;

	return HAL_OK;
}

/**
  * @brief  去初始化SPI外设.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval HAL状态，详见HAL_StatusTypeDef。
  */
HAL_StatusTypeDef HAL_SPI_DeInit(HAL_SPI_HandleTypeDef* SPIHandleStruct)
{
	/* 检查SPIHandleStruct是否合法 */
	if(SPIHandleStruct == NULL)
		return HAL_ERROR;

	//未初始化则返回
	if (SPIHandleStruct->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	SPIHandleStruct->State = HAL_SPI_STATE_BUSY;

	HAL_SPI_Clean_TxFIFO(SPIHandleStruct);

	HAL_SPI_Clean_RxFIFO(SPIHandleStruct);

	HAL_SPI_DISABLE_IT(SPIHandleStruct, 0xFFFFFFFF);

	(SPIHandleStruct->Instance->CONFIG) = 0x20000;                     	/*!< SPI控制寄存器                */
	(SPIHandleStruct->Instance->DELAY) = 0x0;                      		/*!< SPI时钟同步延时             */
	(SPIHandleStruct->Instance->TXD) = 0x0;                      		/*!< SPI发送缓冲数据寄存器,32位 */
	(SPIHandleStruct->Instance->SIC) = 0xFF;                        	/*!< SPI从机时钟空闲计数寄存器,主机提供时钟时,从机检测到设置的空闲周期值后开始处理数据 */
	(SPIHandleStruct->Instance->TX_THRESH) = 0x00000001;                /*!< SPI发送阈值设置寄存器                */
	(SPIHandleStruct->Instance->RX_THRESH) = 0x00000001;                /*!< SPI接收阈值寄存器                         */
	(SPIHandleStruct->Instance->TX_FIFO_OP) = 0x2;                 		/*!< SPI发送FIFO选择寄存器           */
	(SPIHandleStruct->Instance->RX_FIFO_OP) = 0x2;                		/*!< SPI接收FIFO选择寄存器           */

	PeriPadAllocationStatusRemove(HAL_REMAP_SPI_MOSI);
	PeriPadAllocationStatusRemove(HAL_REMAP_SPI_MISO);
	PeriPadAllocationStatusRemove(HAL_REMAP_SPI_SCLK);
	PeriPadAllocationStatusRemove(HAL_REMAP_SPI_NSS1);
	PeriPadAllocationStatusRemove(HAL_REMAP_SPI_NSS2);
	PeriPadAllocationStatusRemove(HAL_REMAP_SPI_CS);
	
	/* 失能SPI外设 */
	HAL_SPI_DISABLE(SPIHandleStruct);

	/* 失能SPI外设时钟 */
	PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_SPI_EN);

	SPIHandleStruct->ErrorCode = HAL_SPI_ERROR_NONE;
	SPIHandleStruct->State = HAL_SPI_STATE_RESET;

	/* 释放程序锁 */
	__HAL_UNLOCK(SPIHandleStruct);

	return HAL_OK;
}

/**
  * @brief  SPI发送阻塞函数,timeout为阻塞时间.
  * @param  SPIHandleStruct. 详见结构体定义  @ref HAL_SPI_HandleTypeDef。
  * @param  pData:发送数据的存储地址。
  * @param  Size:发送数据的大小。
  * @param  Timeout（单位ms）; 如果Time_out取值为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出；.\n
  * 		如果Timeout设置为某一数值时，表示函数等待数据发送完成的最长时间，超时则退出函数，SPIHandleStruct结构体中的TxXferCount表示实际发送的字节数。
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
  *       @retval  HAL_ERROR    ：表示入参错误
  *       @retval  HAL_BUSY     ：表示外设正在使用中
  *       @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
  */
HAL_StatusTypeDef HAL_SPI_Transmit(HAL_SPI_HandleTypeDef* SPIHandleStruct, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if(SPIHandleStruct->State == HAL_SPI_STATE_RESET)
    {
    	return HAL_ERROR;
    }

    if(SPIHandleStruct->State != HAL_SPI_STATE_READY)
    {
    	return HAL_BUSY;
    }

    if((pData == NULL) || (Size == 0U) || (Timeout == 0))
    {
    	return HAL_ERROR;
    }

	if(SPIHandleStruct->Init.DataSize == HAL_SPI_DATASIZE_16BIT)
	{
		if(Size % 2 != 0)
    		return HAL_ERROR;
	}
	else if(SPIHandleStruct->Init.DataSize == HAL_SPI_DATASIZE_32BIT)
	{
		if(Size % 4 != 0)
    		return HAL_ERROR;
	}

	/* 程序锁 */
    __HAL_LOCK(SPIHandleStruct);

	/* 初始化时间管理，给tickstart赋值*/
    tickstart = HAL_GetTick();

    /* 设置SPI传输参数 */
    SPIHandleStruct->State        = HAL_SPI_STATE_BUSY_TX;
    SPIHandleStruct->ErrorCode    = HAL_SPI_ERROR_NONE;
    SPIHandleStruct->pTxBuffPtr   = (uint8_t *)pData;
    SPIHandleStruct->TxXferSize   = Size;
    SPIHandleStruct->TxXferCount  = 0;

    /* 初始化未使用参数为0或空 */
    SPIHandleStruct->pRxBuffPtr   = (uint8_t *)NULL;
    SPIHandleStruct->RxXferSize   = 0U;
    SPIHandleStruct->RxXferCount  = 0U;

    /* 使能SPI外设 */
    HAL_SPI_ENABLE(SPIHandleStruct);

	while(SPIHandleStruct->TxXferSize > 0U)
	{
		/* 判断发送FIFO是否满 */
		if (!HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_TX_FULL))
		{
			if((SPIHandleStruct->TxXferSize >= 4) && (HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_DATA_LEN)) <= 124)
			{
				SPIHandleStruct->Instance->TXD = *((uint32_t *)SPIHandleStruct->pTxBuffPtr);
				SPIHandleStruct->pTxBuffPtr += sizeof(uint32_t);
				SPIHandleStruct->TxXferSize -= 4;
				SPIHandleStruct->TxXferCount += 4;
			}
			else if((SPIHandleStruct->TxXferSize >= 2) && (HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_DATA_LEN)) <= 126)
			{
				SPIHandleStruct->Instance->TXD_16 = *((uint16_t *)SPIHandleStruct->pTxBuffPtr);
				SPIHandleStruct->pTxBuffPtr += sizeof(uint16_t);
				SPIHandleStruct->TxXferSize -= 2;
				SPIHandleStruct->TxXferCount += 2;
				//解决bug：发送3个字节实际只发送2个字节
				while(!HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_TX_EMPTY));
			}
			else if((SPIHandleStruct->TxXferSize >= 1) && (HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_DATA_LEN)) <= 127)
			{
				SPIHandleStruct->Instance->TXD_8 = *((uint8_t *)SPIHandleStruct->pTxBuffPtr);
				SPIHandleStruct->pTxBuffPtr += sizeof(uint8_t);
				SPIHandleStruct->TxXferSize -= 1;
				SPIHandleStruct->TxXferCount += 1;
			}
		}
		else
		{

			/* 超时判断 */
			if ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick() - tickstart) >= Timeout))
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
		}
	}
    
    error:
	//解决bug：必须保证fifo空之后再调用此api
	while(!HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_TX_EMPTY));
	SPIHandleStruct->State = HAL_SPI_STATE_READY;

	/* 程序解锁 */
	__HAL_UNLOCK(SPIHandleStruct);

	return errorcode;
}

/**
  * @brief  SPI接收函数,timeout为阻塞时间.
  * @param  SPIHandleStruct. 详见结构体定义  @ref HAL_SPI_HandleTypeDef。
  * @param  pData:接收数据的存储地址。
  * @param  Size:接收数据的大小。
  * @param  Timeout（单位ms）:如果Timeout（超时）为0，则是不阻塞，检测到接收FIFO空后，则程序立刻退出;HAL_MAX_DELAY 是一直阻塞，直到接收到指定数量的字节后才退出.\n
  * 		非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数，SPIHandleStruct结构体中的RxXferCount表示实际接收的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过SPIHandleStruct结构体中的RxXferCount来确定
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  *       @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
  */
HAL_StatusTypeDef HAL_SPI_Receive(HAL_SPI_HandleTypeDef *SPIHandleStruct, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if(SPIHandleStruct->State == HAL_SPI_STATE_RESET)
    {
    	return HAL_ERROR;
    }

	if(SPIHandleStruct->State != HAL_SPI_STATE_READY)
	{
		return HAL_BUSY;
	}

	if((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	if(SPIHandleStruct->Init.DataSize == HAL_SPI_DATASIZE_16BIT)
	{
		if(Size % 2 != 0)
    		return HAL_ERROR;
	}
	else if(SPIHandleStruct->Init.DataSize == HAL_SPI_DATASIZE_32BIT)
	{
		if(Size % 4 != 0)
    		return HAL_ERROR;
	}

	/* 程序锁 */
	__HAL_LOCK(SPIHandleStruct);

	/* 初始化时间管理，给tickstart赋值*/
	tickstart = HAL_GetTick();

	/* 设置SPI传输参数 */
	SPIHandleStruct->State       = HAL_SPI_STATE_BUSY_RX;
	SPIHandleStruct->ErrorCode   = HAL_SPI_ERROR_NONE;
	SPIHandleStruct->pRxBuffPtr  = (uint8_t *)pData;
	SPIHandleStruct->RxXferSize  = Size;
	SPIHandleStruct->RxXferCount = 0;

	/*初始化未使用参数为0或空 */
	SPIHandleStruct->pTxBuffPtr  = (uint8_t *)NULL;
	SPIHandleStruct->TxXferSize  = 0U;
	SPIHandleStruct->TxXferCount = 0U;

	/* 使能SPI外设功能 */
	HAL_SPI_ENABLE(SPIHandleStruct);

	while(SPIHandleStruct->RxXferSize)
	{
		/* 判断接收FIFO是否空 */
		if(!HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_EMPTY))
		{
			/* 接收数据 */
			if((SPIHandleStruct->RxXferSize >= 4) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 4)))
			{
				(*(uint32_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint32_t);
				SPIHandleStruct->RxXferSize -= 4;
				SPIHandleStruct->RxXferCount += 4;
			}
			else if((SPIHandleStruct->RxXferSize >= 2) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 2)))
			{
				(*(uint16_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD_16;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint16_t);
				SPIHandleStruct->RxXferSize -= 2;
				SPIHandleStruct->RxXferCount += 2;
			}
			else if((SPIHandleStruct->RxXferSize >= 1) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 1)))
			{
				(*(uint8_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD_8;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint8_t);
				SPIHandleStruct->RxXferSize -= 1;
				SPIHandleStruct->RxXferCount += 1;
			}
		}
		else
		{
			/* 超时判断 */
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
	
	error :
	SPIHandleStruct->State = HAL_SPI_STATE_READY;
	__HAL_UNLOCK(SPIHandleStruct);

	return errorcode;
}

/**
  * @brief  SPI收发函数.
  * @param  SPIHandleStruct. 详见结构体定义  @ref HAL_SPI_HandleTypeDef。
  * @param  pTxData:发送数据的存储地址。
  * @param  pRxData:接收数据的存储地址。
  * @param  Size:收发数据的大小。
  * @param  Timeout（单位ms）:HAL_MAX_DELAY 是一直阻塞，直到发送和接收到指定数量的字节后才退出.\n
  * 		非0非HAL_MAX_DELAY时，表示函数等待数据发送接收完成的最长时间，超时则退出函数，SPIHandleStruct结构体中的TxXferCount表示实际发送的字节数，RxXferCount表示实际接收的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
    *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：发送和接收数据成功，表示指定时间内成功发送和接收指定数量的数据
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  *       @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送和接收指定数量的数据
  *	@note 此函数中有控制传输方向的变量，用于切换发送接收状态。
  */
HAL_StatusTypeDef HAL_SPI_TransmitReceive(HAL_SPI_HandleTypeDef *SPIHandleStruct, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size,uint32_t Timeout)
{
    uint32_t             tmp_mode;
    HAL_SPI_StateTypeDef     tmp_state;
    uint32_t             tickstart = 0;

    /* 控制传输方向，使之在发送和接收间交替 */
    uint32_t             txallowed = 1U;
    HAL_StatusTypeDef    errorcode = HAL_OK;

    /* 初始化SPI状态和模式 */
    tmp_state           = SPIHandleStruct->State;
    tmp_mode            = SPIHandleStruct->Init.Mode;

	if(SPIHandleStruct->State == HAL_SPI_STATE_RESET)
    {
    	return HAL_ERROR;
    }

    if (!((tmp_state == HAL_SPI_STATE_READY) || ((tmp_mode == HAL_SPI_MODE_MASTER) && (tmp_state == HAL_SPI_STATE_BUSY_RX))))
    {
        return HAL_BUSY;
    }

    if ((pTxData == NULL) || (pRxData == NULL) || (Size == 0U) || (Timeout == 0))
    {
        return HAL_ERROR;
    }

	if(SPIHandleStruct->Init.DataSize == HAL_SPI_DATASIZE_16BIT)
	{
		if(Size % 2 != 0)
    		return HAL_ERROR;
	}
	else if(SPIHandleStruct->Init.DataSize == HAL_SPI_DATASIZE_32BIT)
	{
		if(Size % 4 != 0)
    		return HAL_ERROR;
	}

	/* 程序锁 */
    __HAL_LOCK(SPIHandleStruct);

    /* 初始化时间管理，给tickstart赋值*/
    tickstart = HAL_GetTick();

    /* 不要覆盖HAL_SPI_STATE_BUSY_RX状态 */
    if (SPIHandleStruct->State != HAL_SPI_STATE_BUSY_RX)
    {
    	SPIHandleStruct->State = HAL_SPI_STATE_BUSY_TX_RX;
    }

    /* 设置传输参数 */
    SPIHandleStruct->ErrorCode   = HAL_SPI_ERROR_NONE;
    SPIHandleStruct->pTxBuffPtr  = (uint8_t *)pTxData;
    SPIHandleStruct->TxXferSize  = Size;
    SPIHandleStruct->TxXferCount = 0;
    SPIHandleStruct->pRxBuffPtr  = (uint8_t *)pRxData;
    SPIHandleStruct->RxXferSize  = Size;
    SPIHandleStruct->RxXferCount = 0;

    /* 使能SPI外设 */
    HAL_SPI_ENABLE(SPIHandleStruct);

	while ((SPIHandleStruct->TxXferSize > 0U) || (SPIHandleStruct->RxXferSize > 0U))
	{
		/* 检查发送FIFO是否满 */
		if ((!HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_TX_FULL)) && (SPIHandleStruct->TxXferSize > 0U) && (txallowed == 1U))
		{
			//待发送的长度与FIFO剩余长度的总和不大于128
			if((SPIHandleStruct->TxXferSize >= 4) && (HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_DATA_LEN) <= 124))
			{
				SPIHandleStruct->Instance->TXD = *((uint32_t *)SPIHandleStruct->pTxBuffPtr);
				SPIHandleStruct->pTxBuffPtr += sizeof(uint32_t);
				SPIHandleStruct->TxXferSize -= 4;
				SPIHandleStruct->TxXferCount += 4;
			}
			else if((SPIHandleStruct->TxXferSize >= 2) && (HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_DATA_LEN) <= 126))
			{
				SPIHandleStruct->Instance->TXD_16 = *((uint16_t *)SPIHandleStruct->pTxBuffPtr);
				SPIHandleStruct->pTxBuffPtr += sizeof(uint16_t);
				SPIHandleStruct->TxXferSize -= 2;
				SPIHandleStruct->TxXferCount += 2;
				while(!HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_TX_EMPTY));
			}
			else if((SPIHandleStruct->TxXferSize >= 1) && (HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_DATA_LEN) <= 127))
			{
				SPIHandleStruct->Instance->TXD_8 = *((uint8_t *)SPIHandleStruct->pTxBuffPtr);
				SPIHandleStruct->pTxBuffPtr += sizeof(uint8_t);
				SPIHandleStruct->TxXferSize -= 1;
				SPIHandleStruct->TxXferCount += 1;
			}
			//解决bug
			while(!HAL_SPI_Get_TxFIFO_Flag(SPIHandleStruct, HAL_SPI_TXFIFO_TX_EMPTY));
			/* 下一个是接收数据，发送将不被允许 */
			txallowed = 0U;
		}
		else if ((Timeout != 0U) && ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick() - tickstart) >= Timeout)))  
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}


		/* 检查接收FIFO是否空 */
		if (!HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_EMPTY) && (SPIHandleStruct->RxXferSize > 0U))
		{
			/* 接收数据 */
			if((SPIHandleStruct->RxXferSize >= 4) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 4)))
			{
				(*(uint32_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint32_t);
				SPIHandleStruct->RxXferSize -= 4;
				SPIHandleStruct->RxXferCount += 4;
			}
			else if((SPIHandleStruct->RxXferSize >= 2) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 2)))
			{
				(*(uint16_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD_16;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint16_t);
				SPIHandleStruct->RxXferSize -= 2;
				SPIHandleStruct->RxXferCount += 2;
			}
			else if((SPIHandleStruct->RxXferSize >= 1) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 1)))
			{
				(*(uint8_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD_8;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint8_t);
				SPIHandleStruct->RxXferSize -= 1;
				SPIHandleStruct->RxXferCount += 1;
			}
			/* 下一个是发送数据，发送将被允许 */
			txallowed = 1U;
		}
		else if ((Timeout != 0U) && ((Timeout != HAL_MAX_DELAY) && ((HAL_GetTick() - tickstart) >= Timeout)))  
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}

	}
    
    error :
	SPIHandleStruct->State = HAL_SPI_STATE_READY;
    __HAL_UNLOCK(SPIHandleStruct);

    return errorcode;
}

/**
  * @brief  SPI中断方式接收数据.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @param  接收数据的存储地址。
  * @param  接收数据的大小。
  * @retval HAL状态，详见HAL_StatusTypeDef。
  */
HAL_StatusTypeDef HAL_SPI_Receive_IT(HAL_SPI_HandleTypeDef *SPIHandleStruct, uint8_t *pData, uint16_t Size)
{
	if(SPIHandleStruct->State == HAL_SPI_STATE_RESET)
    {
    	return HAL_ERROR;
    }

  if (SPIHandleStruct->State != HAL_SPI_STATE_READY)
  {
    return HAL_BUSY;
  }

  if ((pData == NULL) || (Size == 0U))
  {
	return HAL_ERROR;
  }

  /* 程序锁 */
  __HAL_LOCK(SPIHandleStruct);

  /* 设置传输参数 */
  SPIHandleStruct->State       = HAL_SPI_STATE_BUSY_RX;
  SPIHandleStruct->ErrorCode   = HAL_SPI_ERROR_NONE;
  SPIHandleStruct->pRxBuffPtr  = (uint8_t *)pData;
  SPIHandleStruct->RxXferSize  = Size;
  SPIHandleStruct->RxXferCount = 0;

  /* 初始化未使用参数为0或空 */
  SPIHandleStruct->pTxBuffPtr  = (uint8_t *)NULL;
  SPIHandleStruct->TxXferSize  = 0U;
  SPIHandleStruct->TxXferCount = 0U;

  /* 程序解锁 */
  __HAL_UNLOCK(SPIHandleStruct);

  HAL_NVIC_IntRegister(HAL_SPI1_IRQn,HAL_SPI_IRQHandler, 1);

  /* 使能SPI中断 */
  HAL_SPI_ENABLE_IT(SPIHandleStruct, HAL_SPI_IT_RX_NOT_EMPTY);

//  HAL_SPI_ENABLE_IT(SPIHandleStruct, HAL_SPI_IT_REV_OVERFLOW);
//  HAL_SPI_ENABLE_IT(SPIHandleStruct, HAL_SPI_IT_MODE_FAIL);
//  HAL_SPI_ENABLE_IT(SPIHandleStruct, HAL_SPI_IT_TX_UNDERFLOW);

  /* Note : The SPI must be enabled after unlocking current process
            to avoid the risk of SPI interrupt handle execution before current
            process unlock */

  /* 检查SPI外设是否使能 */
  if ((SPIHandleStruct->Instance->ENABLE & 0x01) != 0x01)
  {
    /* 没有则是能SPI外设 */
    HAL_SPI_ENABLE(SPIHandleStruct);
  }

  return HAL_OK;
}

/** @brief  获取SPI中断使能状态.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval SPI中断使能状态.
  */
uint32_t HAL_SPI_GetITEnable(HAL_SPI_HandleTypeDef* SPIHandleStruct)
{
    return(SPIHandleStruct->Instance->IMASK);
}

/** @brief  获取SPI中断状态.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval SPI中断状态.
  */
uint32_t HAL_SPI_GetITState(HAL_SPI_HandleTypeDef* SPIHandleStruct)
{
    return(SPIHandleStruct->Instance->INT_STATUS);
}

/**
  * @brief  SPI错误回调函数。
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval 无
  */
__weak void HAL_SPI_ErrorCallback(HAL_SPI_HandleTypeDef *SPIHandleStruct)
{
  /* 防止编译器报错 */
  UNUSED_ARG(SPIHandleStruct);
}

/**
  * @brief  SPI发送完成回调函数.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval 无
  */
__weak void HAL_SPI_RxCpltCallback(HAL_SPI_HandleTypeDef *SPIHandleStruct)
{
  /* 防止编译器报错 */
  UNUSED_ARG(SPIHandleStruct);
}

/**
  * @brief  SPI中断处理函数.
  * @param  SPIHandleStruct. 详见结构体定义 HAL_SPI_HandleTypeDef。
  * @retval 无
  */
void SPI_IRQHandler(HAL_SPI_HandleTypeDef *SPIHandleStruct)
{
	uint32_t ITState = HAL_SPI_GetITState(SPIHandleStruct);
	uint32_t ITEnable = HAL_SPI_GetITEnable(SPIHandleStruct);

	//发生错误中断
	if((HAL_RESET != (ITEnable & HAL_SPI_IT_REV_OVERFLOW)) && (HAL_RESET != (ITState & HAL_SPI_IT_REV_OVERFLOW)))
	{
		SPIHandleStruct->ErrorCode |= HAL_SPI_ERROR_REV_OVERFLOW;
	}
	if((HAL_RESET != (ITEnable & HAL_SPI_IT_MODE_FAIL)) && (HAL_RESET != (ITState & HAL_SPI_IT_MODE_FAIL)))
	{
		SPIHandleStruct->ErrorCode |= HAL_SPI_ERROR_MODE_FAIL;
	}
	if((HAL_RESET != (ITEnable & HAL_SPI_IT_TX_UNDERFLOW)) && (HAL_RESET != (ITState & HAL_SPI_IT_TX_UNDERFLOW)))
	{
		SPIHandleStruct->ErrorCode |= HAL_SPI_ERROR_TX_UNDERFLOW;
	}

	if (SPIHandleStruct->ErrorCode != HAL_SPI_ERROR_NONE)
	{
		HAL_SPI_ErrorCallback(SPIHandleStruct);
		return;
	}

	//没有发生错误中断
	if((HAL_RESET != (ITEnable & HAL_SPI_IT_RX_NOT_EMPTY)) && (HAL_RESET != (ITState & HAL_SPI_IT_RX_NOT_EMPTY)))
	{
		HAL_SPI_DISABLE_IT(SPIHandleStruct, HAL_SPI_IT_RX_NOT_EMPTY);

		while(!HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_EMPTY) && (SPIHandleStruct->RxXferSize > 0))
		{
			/* 接收数据 */
			if((SPIHandleStruct->RxXferSize >= 4) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 4)))
			{
				(*(uint32_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint32_t);
				SPIHandleStruct->RxXferSize -= 4;
				SPIHandleStruct->RxXferCount += 4;
			}
			else if((SPIHandleStruct->RxXferSize >= 2) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 2)))
			{
				(*(uint16_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD_16;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint16_t);
				SPIHandleStruct->RxXferSize -= 2;
				SPIHandleStruct->RxXferCount += 2;
			}
			else if((SPIHandleStruct->RxXferSize >= 1) && ((HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_RX_FULL) == HAL_SET) || (HAL_SPI_Get_RxFIFO_Flag(SPIHandleStruct, HAL_SPI_RXFIFO_DATA_LEN) >= 1)))
			{
				(*(uint8_t *)SPIHandleStruct->pRxBuffPtr) = SPIHandleStruct->Instance->RXD_8;
				SPIHandleStruct->pRxBuffPtr += sizeof(uint8_t);
				SPIHandleStruct->RxXferSize -= 1;
				SPIHandleStruct->RxXferCount += 1;
			}
		}

		if (SPIHandleStruct->RxXferSize == 0U)
		{
			SPIHandleStruct->State = HAL_SPI_STATE_READY;
			HAL_SPI_RxCpltCallback(SPIHandleStruct);
		}
		else
		{
			HAL_SPI_ENABLE_IT(SPIHandleStruct, HAL_SPI_IT_RX_NOT_EMPTY);
		}
	}
	
}

/**
  * @brief  SPI中断处理函数.
  * @param  无
  * @retval 无
  */
__weak void HAL_SPI_IRQHandler(void)
{

}
