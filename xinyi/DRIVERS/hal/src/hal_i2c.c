/**
  ******************************************************************************
  * @file    hal_i2c.c
  * @brief   HAL库I2C
  ******************************************************************************
  */

/* 头文件 ------------------------------------------------------------------*/
#include "hal_i2c.h"

#include "hw_i2c.h"
#include "hw_prcm.h"
#include "replace.h"
#include "stddef.h"
#include "xy_clk_config.h"
#include "hal_gpio.h"

/**
  * @brief   获取I2C工作状态.
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval 返回HAL状态. 详见HAL_I2C_StateTypeDef.
  */
HAL_I2C_StateTypeDef HAL_I2C_GetState(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
	return I2CHandleStruct->State;
}

/**
  * @brief  获取I2C错误状态
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval 返回I2C错误码，详见HAL_I2C_ErrorCodeTypeDef.
  */
uint32_t HAL_I2C_GetError(HAL_I2C_HandleTypeDef *I2CHandleStruct)
{
	return I2CHandleStruct->ErrorCode;
}

/**
  * @brief  获取I2C的传输模式，.即I2C的模式是主模式还是从模式还是无。
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval I2C工作模式，详见HAL_I2CModeTypeDef。
  */
HAL_I2CModeTypeDef HAL_I2C_GetMode(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
	return I2CHandleStruct->Mode;
}

/** @brief  使能I2C外设时钟。
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval 无。
  */
void HAL_I2C_ENABLE(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
	if(I2CHandleStruct->Instance == HAL_I2C1)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_I2C1_EN);

	if(I2CHandleStruct->Instance == HAL_I2C2)
		PRCM_Clock_Enable(PRCM_BASE, PRCM_CKG_CTL_I2C2_EN);
}

/** @brief  失能I2C外设时钟。
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval 无。
  */
void HAL_I2C_DISABLE(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
	if(I2CHandleStruct->Instance == HAL_I2C1)
		PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_I2C1_EN);

	if(I2CHandleStruct->Instance == HAL_I2C2)
		PRCM_Clock_Disable(PRCM_BASE, PRCM_CKG_CTL_I2C2_EN);
}

/**
  * @brief  初始化I2C外设函数，详情可见内部函数。
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval HAL状态.详见HAL_StatusTypeDef。
  */
HAL_StatusTypeDef HAL_I2C_Init(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
	unsigned long ulDiv;
	unsigned long ulDiff;
	unsigned long ulMinDiff = 0x0FFFFFFF;
	unsigned char ucDiva, ucDivb, ucMinDiva = 0;

	if(I2CHandleStruct == NULL)
		return HAL_ERROR;

	if (I2CHandleStruct->State == HAL_I2C_STATE_RESET)
	{
		/* Allocate lock resource and initialize it */
		I2CHandleStruct->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	I2CHandleStruct->State = HAL_I2C_STATE_BUSY;

	HAL_I2C_ENABLE(I2CHandleStruct);

	HAL_I2C_SET_MODE(I2CHandleStruct);

	HAL_I2C_SET_ADDR_MODE(I2CHandleStruct);

	if(I2CHandleStruct->Init.Mode == HAL_I2C_MODE_SLAVE)
	{
		HAL_I2C_SET_SLAVE_OWN_ADDR(I2CHandleStruct);
		SET_BIT(I2CHandleStruct->Instance->CONTROL,I2C_CTL_HOLD_Msk);
	}

	HAL_I2C_CLEAN_FIFO(I2CHandleStruct);

	ulDiv = XY_PCLK / (22 * (I2CHandleStruct->Init.ClockSpeed));

	for(ucDiva = 0; ucDiva < 4; ucDiva++)
	{
		ucDivb = ulDiv / (ucDiva + 1);

		if(ucDivb == 0 || ucDivb > 64)
		{
			continue;
		}

		ulDiff = ulDiv - ucDivb * (ucDiva + 1);

		if(ulDiff < ulMinDiff)
		{
			ulMinDiff = ulDiff;
			ucMinDiva = ucDiva;
		}
	}

	ucDivb = ulDiv / (ucMinDiva + 1);

	HAL_I2C_SET_EXP_CLK(I2CHandleStruct,((ucMinDiva << I2C_CTL_DIVISOR_A_Pos) | (ucDivb << I2C_CTL_DIVISOR_B_Pos)));

	HAL_I2C_SET_ACKMODE(I2CHandleStruct);

	//清除所有中断
	HAL_I2C_CLEAR_IT(I2CHandleStruct,0xFFFFFFFF);

	I2CHandleStruct->Mode = I2CHandleStruct->Init.Mode;
	I2CHandleStruct->ErrorCode = HAL_I2C_ERROR_NONE;
	I2CHandleStruct->State = HAL_I2C_STATE_READY;

	return HAL_OK;
}

/**
  * @brief  I2C外设去初始化函数.
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval HAL状态.详见HAL_StatusTypeDef。
  */
HAL_StatusTypeDef HAL_I2C_DeInit(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
	if(I2CHandleStruct == NULL)
		return HAL_ERROR;

	//未初始化则返回
	if (I2CHandleStruct->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	I2CHandleStruct->State = HAL_I2C_STATE_BUSY;

	//向寄存器写入默认值
	I2CHandleStruct->Instance->CONTROL &= (~0x02);
	(I2CHandleStruct->Instance->TRAN_SIZE) = 0x0;                      	/*!< I2C传输长度寄存器 */
	(I2CHandleStruct->Instance->SLAVE_MONITOR_PAUSE) = 0x0;             /*!< I2C检测停止寄存器 */
	(I2CHandleStruct->Instance->TIMEOUT) = 0x0000001F;                  /*!< I2C超时寄存器          */
	(I2CHandleStruct->Instance->INT_DISABLE) = 0xFFFFFFFF;              /*!< I2C中断失能寄存器 */

	if(I2CHandleStruct->Instance == HAL_I2C1)
	{
		PeriPadAllocationStatusRemove(HAL_REMAP_I2C1_SCLK);
		PeriPadAllocationStatusRemove(HAL_REMAP_I2C1_SDA);
	}
	else if(I2CHandleStruct->Instance == HAL_I2C2)
	{
		PeriPadAllocationStatusRemove(HAL_REMAP_I2C2_SCLK);
		PeriPadAllocationStatusRemove(HAL_REMAP_I2C2_SDA);
	}

	//失能外设时钟
	HAL_I2C_DISABLE(I2CHandleStruct);

	I2CHandleStruct->ErrorCode = HAL_I2C_ERROR_NONE;
	I2CHandleStruct->State = HAL_I2C_STATE_RESET;

	/* 释放程序锁 */
	__HAL_UNLOCK(I2CHandleStruct);

	return HAL_OK;
}

/**
  * @brief  I2C主模式下的发送阻塞函数，Timeout为阻塞时间.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  DevAddress：目标从机地址，7位地址时低7位有效，10位地址时低10位有效，若最后一位为R/W位，则需用户将该地址右移一位。
  * @param  pData：要发送的数据的存储地址。
  * @param  Size：要发送的数据的长度。
  * @param  Timeout：超时时间（单位ms）; 如果Time_out取值为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出；.\n
  * 		如果Timeout设置为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数，I2CHandleStruct结构体中的XferCount表示实际发送的字节数。
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
    *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
  *       @retval  HAL_ERROR    ：表示入参错误
  *       @retval  HAL_BUSY     ：表示外设正在使用中
  *       @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
  */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if(I2CHandleStruct->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if(I2CHandleStruct->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if((pData == NULL) || (Size == 0U) || (Timeout == 0))
    {
    	return HAL_ERROR;
    }
	/* 程序锁 */
	__HAL_LOCK(I2CHandleStruct);

	/* 初始化时间管理，给tickstart赋值 */
	tickstart = HAL_GetTick();

	/* 更新 I2C状态 */
	I2CHandleStruct->State = HAL_I2C_STATE_BUSY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_MASTER;
	I2CHandleStruct->ErrorCode = HAL_I2C_ERROR_NONE;

	/* 准备传输参数 */
	I2CHandleStruct->pBuffPtr = pData;
	I2CHandleStruct->XferSize = Size;
	I2CHandleStruct->XferCount = 0;

	/* 设置从机地址宽度，7位还是11位 */
	HAL_I2C_SET_ADDR_MODE(I2CHandleStruct);

	while(I2CHandleStruct->XferSize)
	{
		if((I2CHandleStruct->XferCount % 8 == 0))
			HAL_I2C_SET_TRAN_DIRECTION_WRITE(I2CHandleStruct);

		/* 将需要传输的数据写入I2C数据寄存器 */
		I2CHandleStruct->Instance->DATA = *I2CHandleStruct->pBuffPtr;
		/* 数据地址指针递增 */
		I2CHandleStruct->pBuffPtr++;
		/* 更新计数器 */
		I2CHandleStruct->XferSize--;
		I2CHandleStruct->XferCount++;

		if((I2CHandleStruct->XferCount % 8 == 0) || (I2CHandleStruct->XferSize == 0))
		{
			// HAL_I2C_SET_TRAN_DIRECTION_WRITE(I2CHandleStruct);
			HAL_I2C_SET_MASTER_DIR_ADDR(I2CHandleStruct, DevAddress);
			while(HAL_RESET == (HAL_I2C_GetITState(I2CHandleStruct) & HAL_I2C_INTTYPE_COMP))
			{
				if (((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY))
				{
					HAL_I2C_CLEAN_FIFO(I2CHandleStruct);

					errorcode = HAL_TIMEOUT;
					goto error;
				}
			}
			HAL_I2C_CLEAR_IT(I2CHandleStruct, HAL_I2C_INTTYPE_COMP);
			for(volatile unsigned long i = 0; i < 1000; i++);
		}
	}

error:

	I2CHandleStruct->State = HAL_I2C_STATE_READY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_NONE;

    /* 程序解锁 */
	__HAL_UNLOCK(I2CHandleStruct);

	return errorcode;
}

/**
  * @brief  I2C主模式下的接收阻塞函数，Timeout为阻塞时间.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  DevAddress：目标从机地址，7位地址时低7位有效，10位地址时低10位有效，若最后一位为R/W位，则需用户将该地址右移一位。
  * @param  pData：要接收的数据的存储地址。
  * @param  Size：要接收的数据的长度。
  * @param  Timeout（单位ms）:如果Timeout（超时）为0，则是不阻塞，检测到接收FIFO空后，则程序立刻退出;HAL_MAX_DELAY 是一直阻塞，直到接收到指定数量的字节后才退出.\n
  * 		非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数，I2CHandleStruct结构体中的XferCount表示实际接收的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
    *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过SPIHandleStruct结构体中的RxXferCount来确定
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  *       @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
  */
HAL_StatusTypeDef HAL_I2C_Master_Receive(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if(I2CHandleStruct->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if(I2CHandleStruct->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if((pData == NULL) || (Size == 0U))
    {
    	return HAL_ERROR;
    }
	/* 程序锁 */
	__HAL_LOCK(I2CHandleStruct);

	/* 初始化时间管理，给tickstart赋值*/
	tickstart = HAL_GetTick();

	/* 更新I2C状态 */
	I2CHandleStruct->State = HAL_I2C_STATE_BUSY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_MASTER;
	I2CHandleStruct->ErrorCode = HAL_I2C_ERROR_NONE;

	/* 准备传输参数 */
	I2CHandleStruct->pBuffPtr = pData;
	I2CHandleStruct->XferSize = Size;
	I2CHandleStruct->XferCount = 0;

	/* 设置传输方向 */
	HAL_I2C_SET_TRAN_DIRECTION_READ(I2CHandleStruct);

	/* 设置从机地址宽度，7位还是11位 */
	HAL_I2C_SET_ADDR_MODE(I2CHandleStruct);

	while(I2CHandleStruct->XferSize)
	{
		if((I2CHandleStruct->XferCount % 8 == 0))
		{
		/* 设置从机地址，如果从机地址最后一位为读写位，则需要用户将该地址右移一位 */
			HAL_I2C_SET_MASTER_DIR_ADDR(I2CHandleStruct,DevAddress);

			I2CHandleStruct->Instance->TRAN_SIZE = (I2CHandleStruct->XferSize >= 8)?8:I2CHandleStruct->XferSize;

			while(HAL_RESET == (HAL_I2C_GetITState(I2CHandleStruct) & HAL_I2C_INTTYPE_COMP))
			{
				/*检查超时时间*/
				if ((((HAL_GetTick() - tickstart) >= Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
				{
					HAL_I2C_CLEAN_FIFO(I2CHandleStruct);

					errorcode = HAL_TIMEOUT;
					goto error;
				}
			}
			//清除中断
			HAL_I2C_CLEAR_IT(I2CHandleStruct, HAL_I2C_INTTYPE_COMP);
			for(volatile unsigned long i = 0; i < 1000; i++);
		}

		/*将需要接收的数据写入接收数据所在地址中*/
		*I2CHandleStruct->pBuffPtr = (uint8_t)I2CHandleStruct->Instance->DATA;
		/* 数据地址指针递增 */
		I2CHandleStruct->pBuffPtr++;
		/* 更新计数器 */
		I2CHandleStruct->XferSize--;
		I2CHandleStruct->XferCount++;
	}

error:

	I2CHandleStruct->State = HAL_I2C_STATE_READY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_NONE;

	/* 程序解锁 */
	__HAL_UNLOCK(I2CHandleStruct);

	return errorcode;
}

/**
  * @brief  I2C从机模式下发送数据阻塞函数，Timeout为阻塞时间。
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  pData：要发送的数据的存储地址。
  * @param  Size：要发送的数据的长度。
  * @param  Timeout：超时时间（单位ms）; 如果Time_out取值为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出；.\n
  * 		如果Timeout设置为某一数值时，表示函数等待数据发送完成的最长时间，超时则退出函数，I2CHandleStruct结构体中的XferCount表示实际发送的字节数。
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
    *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
  *       @retval  HAL_ERROR    ：表示入参错误
  *       @retval  HAL_BUSY     ：表示外设正在使用中
  *       @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
  */
HAL_StatusTypeDef HAL_I2C_Slave_Transmit(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if(I2CHandleStruct->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if(I2CHandleStruct->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if((pData == NULL) || (Size == 0U) || (Timeout == 0))
    {
    	return HAL_ERROR;
    }
	/* 程序锁 */
	__HAL_LOCK(I2CHandleStruct);

	/* 初始化时间管理，给tickstart赋值*/
	tickstart = HAL_GetTick();

	/* 更新I2C状态 */
	I2CHandleStruct->State = HAL_I2C_STATE_BUSY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_SLAVE;
	I2CHandleStruct->ErrorCode = HAL_I2C_ERROR_NONE;

	/* 准备传输参数 */
	I2CHandleStruct->pBuffPtr = pData;
	I2CHandleStruct->XferSize = Size;
	I2CHandleStruct->XferCount = 0;

	while(I2CHandleStruct->XferSize)
	{
		//检测总线处于active状态以及主机要求读数据状态
		if((HAL_I2C_GET_FLAG(I2CHandleStruct,HAL_I2C_STATE_BA)) && (HAL_I2C_GET_FLAG(I2CHandleStruct,HAL_I2C_STATE_RXRW)) && (!HAL_I2C_GET_FLAG(I2CHandleStruct,HAL_I2C_STATE_TXDV)))
		{
			/* 将需要传输的数据写入I2C数据寄存器  */
			I2CHandleStruct->Instance->DATA = *I2CHandleStruct->pBuffPtr;

			/* 数据地址指针递增 */
			I2CHandleStruct->pBuffPtr++;

			/* 更新计数器 */
			I2CHandleStruct->XferSize--;
			I2CHandleStruct->XferCount++;
		}
		/* 检查超时时间 */
		if (((HAL_GetTick() - tickstart) >= Timeout) && (Timeout != HAL_MAX_DELAY))
		{
			HAL_I2C_CLEAN_FIFO(I2CHandleStruct);

		    errorcode = HAL_TIMEOUT;
			goto error;
		}
	}

	//必须等待总线不忙，否则在总线忙的情况下去初始化再初始化会导致下一次传输失败
	while(HAL_RESET != HAL_I2C_GET_FLAG(I2CHandleStruct,HAL_I2C_STATE_BA)){;}

error:

	I2CHandleStruct->State = HAL_I2C_STATE_READY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_NONE;

	/* 程序解锁 */
	__HAL_UNLOCK(I2CHandleStruct);

	return errorcode;
}

/**
  * @brief  I2C从机模式下接收数据阻塞函数，Timeout为阻塞时间。
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  pData：要接收的数据的存储地址。
  * @param  Size：要接收的数据的长度。
  * @param  Timeout（单位ms）:如果Timeout（超时）为0，则是不阻塞，检测到接收FIFO空后，则程序立刻退出;HAL_MAX_DELAY 是一直阻塞，直到接收到指定数量的字节后才退出.\n
  * 		非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数，I2CHandleStruct结构体中的XferCount表示实际接收的字节数.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
    *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过SPIHandleStruct结构体中的RxXferCount来确定
  *       @retval  HAL_ERROR    ：入参错误
  *       @retval  HAL_BUSY     ：外设正在使用中
  *       @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
  */
HAL_StatusTypeDef HAL_I2C_Slave_Receive(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if(I2CHandleStruct->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if(I2CHandleStruct->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if((pData == NULL) || (Size == 0U))
    {
    	return HAL_ERROR;
    }
	/* 程序锁 */
	__HAL_LOCK(I2CHandleStruct);

	/* 初始化时间管理，给tickstart赋值*/
	tickstart = HAL_GetTick();

	/* 更新I2C状态 */
	I2CHandleStruct->State = HAL_I2C_STATE_BUSY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_SLAVE;
	I2CHandleStruct->ErrorCode = HAL_I2C_ERROR_NONE;

	/* 准备传输参数 */
	I2CHandleStruct->pBuffPtr = pData;
	I2CHandleStruct->XferSize = Size;
	I2CHandleStruct->XferCount = 0;

	while(I2CHandleStruct->XferSize)
	{
		//检测总线处于active状态，以及RX fifo中有数据
		if(HAL_I2C_GET_FLAG(I2CHandleStruct,HAL_I2C_STATE_RXDV))
		{	
			/* 将需要接收的数据写入接收数据所在地址中 */
			*I2CHandleStruct->pBuffPtr = (uint8_t)I2CHandleStruct->Instance->DATA;

			/* 数据地址指针递增 */
			I2CHandleStruct->pBuffPtr++;

			/* 更新计数器 */
			I2CHandleStruct->XferSize--;
			I2CHandleStruct->XferCount++;
		}
		/*检查超时时间*/
		if ((((HAL_GetTick() - tickstart) >=  Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
		{
			HAL_I2C_CLEAN_FIFO(I2CHandleStruct);

			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}

	while(HAL_RESET != HAL_I2C_GET_FLAG(I2CHandleStruct,HAL_I2C_STATE_BA)){;}

error:

	I2CHandleStruct->State = HAL_I2C_STATE_READY;
	I2CHandleStruct->Mode = HAL_I2C_MODE_NONE;

	/* 程序解锁 */
	__HAL_UNLOCK(I2CHandleStruct);

	return errorcode;
}

/**
  * @brief  I2C从机模式下接收数据中断模式。
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @param  要接收的数据的存储地址。
  * @param  要接收的数据的长度。
  * @retval HAL状态.详见HAL_StatusTypeDef。
  */
HAL_StatusTypeDef HAL_I2C_Slave_Receive_IT(HAL_I2C_HandleTypeDef *I2CHandleStruct, uint8_t *pData, uint16_t Size)
{
	if(I2CHandleStruct->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if(I2CHandleStruct->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if((pData == NULL) || (Size == 0U))
    {
      return  HAL_ERROR;
    }

    /* 程序锁 */
    __HAL_LOCK(I2CHandleStruct);

    I2CHandleStruct->State     = HAL_I2C_STATE_BUSY;
    I2CHandleStruct->Mode      = HAL_I2C_MODE_SLAVE;
    I2CHandleStruct->ErrorCode = HAL_I2C_ERROR_NONE;

    /* 更新I2C状态 */
    I2CHandleStruct->pBuffPtr    = pData;
    I2CHandleStruct->XferSize    = Size;
    I2CHandleStruct->XferCount   = 0;

    /* 程序解锁 */
    __HAL_UNLOCK(I2CHandleStruct);

    if(I2CHandleStruct->Instance == HAL_I2C1)
    	HAL_NVIC_IntRegister(HAL_I2C1_IRQn, HAL_I2C1_IRQHandler, 1);
    else if(I2CHandleStruct->Instance == HAL_I2C2)
    	HAL_NVIC_IntRegister(HAL_I2C2_IRQn, HAL_I2C2_IRQHandler, 1);

    /* 使能I2C中断必须在I2C初始化完成之后 */
    HAL_I2C_ENABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_COMP);
    HAL_I2C_ENABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_DATA);

    return HAL_OK;
}

/** @brief  获取I2C中断使能位.
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval I2C中断使能位.
  */
uint32_t HAL_I2C_GetITEnable(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
    return (I2CHandleStruct->Instance->INT_MASK);
}

/** @brief  获取I2C当前中断状态.
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval I2C当前中断状态.
  */
uint32_t HAL_I2C_GetITState(HAL_I2C_HandleTypeDef* I2CHandleStruct)
{
    return (I2CHandleStruct->Instance->INT_STATUS);
}

/**
  * @brief  I2C错误回调函数。
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval 无
  */
__weak void HAL_I2C_ErrorCallback(HAL_I2C_HandleTypeDef *I2CHandleStruct)
{
  /* 防止编译器报错 */
  UNUSED_ARG(I2CHandleStruct);
}

/**
  * @brief  I2C传输完成回调函数。
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval 无
  */
__weak void HAL_I2C_RxCpltCallback(HAL_I2C_HandleTypeDef *I2CHandleStruct)
{
  /* 防止编译器报错  */
  UNUSED_ARG(I2CHandleStruct);
}

/**
  * @brief  I2C接收中断处理函数，数据存储在I2CHandleStruct中，用户需在HAL_I2C_RxCpltCallback回调函数中处理.
  * @param  I2CHandleStruct. 详见结构体定义 HAL_I2C_HandleTypeDef.
  * @retval 无
  */
void I2C_IRQHandler(HAL_I2C_HandleTypeDef *I2CHandleStruct)
{
	uint32_t ITState = HAL_I2C_GetITState(I2CHandleStruct);
	uint32_t ITEnable = HAL_I2C_GetITEnable(I2CHandleStruct);
	
	//发生错误中断
	if((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_RX_OVF)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_RX_OVF)))
	{
		HAL_I2C_DISABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_RX_OVF);
		I2CHandleStruct->ErrorCode |= HAL_I2C_ERROR_RX_OVF;
	}
	if((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_TX_OVF)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_TX_OVF)))
	{
		HAL_I2C_DISABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_TX_OVF);
		I2CHandleStruct->ErrorCode |= HAL_I2C_ERROR_TX_OVF;
	}
	if((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_RX_UNF)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_RX_UNF)))
	{
		HAL_I2C_DISABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_RX_UNF);
		I2CHandleStruct->ErrorCode |= HAL_I2C_ERROR_RX_UNF;
	}
	if((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_ARB_LOST)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_ARB_LOST)))
	{
		HAL_I2C_DISABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_ARB_LOST);
		I2CHandleStruct->ErrorCode |= HAL_I2C_ERROR_ARB_LOST;
	}

	if (I2CHandleStruct->ErrorCode != HAL_I2C_ERROR_NONE)
	{
		HAL_I2C_ErrorCallback(I2CHandleStruct);
		return;
	}
	
	//没有发生错误中断
	if(((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_COMP)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_COMP))) \
		|| ((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_DATA)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_DATA))))
	{	
		HAL_I2C_CLEAR_IT(I2CHandleStruct, HAL_I2C_INTTYPE_COMP);
		HAL_I2C_CLEAR_IT(I2CHandleStruct, HAL_I2C_INTTYPE_DATA);
		while(HAL_I2C_GET_FLAG(I2CHandleStruct,HAL_I2C_STATE_RXDV) && (I2CHandleStruct->XferSize > 0U))
		{
			HAL_I2C_CLEAR_FLAG(I2CHandleStruct, HAL_I2C_STATE_RXDV);

			/* 将需要接收的数据写入接收数据所在地址中 */
			*I2CHandleStruct->pBuffPtr = (uint8_t)I2CHandleStruct->Instance->DATA;

			/* 数据地址指针递增 */
			I2CHandleStruct->pBuffPtr++;

			/* 更新计数器 */
			I2CHandleStruct->XferSize--;
			I2CHandleStruct->XferCount++;
		}
		
		if(I2CHandleStruct->XferSize == 0U)
		{
			HAL_I2C_CLEAN_FIFO(I2CHandleStruct);
			HAL_I2C_DISABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_COMP);
			HAL_I2C_DISABLE_IT(I2CHandleStruct, HAL_I2C_INTTYPE_DATA);
			I2CHandleStruct->State = HAL_I2C_STATE_READY;
			I2CHandleStruct->Mode = HAL_I2C_MODE_NONE;
			HAL_I2C_RxCpltCallback(I2CHandleStruct);
		}
	}
}

/**
  * @brief  I2C1中断处理函数。
  * @param  无
  * @retval 无
  */
__weak void HAL_I2C1_IRQHandler(void)
{

}

/**
  * @brief  I2C2中断处理函数.
  * @param  None
  * @retval None
  */
__weak void HAL_I2C2_IRQHandler(void)
{

}

