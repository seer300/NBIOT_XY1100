/**
 ******************************************************************************
 * @file	hal_i2c.h
 * @ingroup	外设
 * @brief  	此文件包含12C外设的变量，枚举，结构体定义，函数声明等.
 *
 * 		I2C（Inter Integrated Circuit）总线是 PHILIPS 公司开发的一种半双工、双向二线制同步串行总线。\n
 * 		I2C 总线传输数据时只需两根信号线，一根是双向数据线 SDA（serial data），另一根是双向时钟线 SCL（serial\n
 * 		clock）。由于I2C引脚少，硬件实现简单，可扩展性强，现在被广泛地使用在系统内多个集成电路(IC)间的通讯。\n
 * 		本产品有两个I2C模块，I2C1与I2C2。\n
 *
 * 		I2C 以主从的方式工作，允许同时有多个主机存在，每个连接到总线上的器件都有唯一的地址，主机启动数据传输并产生\n
 * 		时钟信号，从机设备被主机寻址，I2C总线的仲裁机制保证了同一时刻只有一个主机。当总线空闲时，SDA 和 SCL 都处\n
 * 		于高电平状态，当主机要和某个从机通讯时，主机将 SDA 拉低，表示数据传输即将开始，然后发送从机地址和读写控制位，\n
 * 		接下来传输数据（主机发送或者接收数据）。传输的每个字节为8位，高位在前，低位在后。每传输完成一个字节的数据，\n
 * 		接收方就需要回复一个 ACK（acknowledge）。主机写数据时由从机发送 ACK，读数据时由主机发送 ACK。当读到\n
 * 		最后一个字节数据时，主机发送 NACK（Not acknowledge）然后在 SDA 为低电平时，主机将 SCL 拉高并保持\n
 * 		高电平，然后在将 SDA 拉高，表示传输结束。\n
 *
 * 		I2C的主要特性
 * 		- 两线制I2C串行接口–由串行数据线（SDA）和串行时钟（SCL）组成\n
 * 		- 两种速度：\n
 * 			- 标准模式（0到100 Kb / s）\n
 * 			- 快速模式（<= 400 Kb / s）\n 			
 * 		- 时钟同步和总线仲裁\n
 * 		- 主或从I2C通讯模式\n
 * 		- 8个字节的发送FIFO和8个字节的接收FIFO \n
 * 		- 7位或10位寻址\n
 * 		- 可编程从站的响应地址\n 		
 * 		- 支持组合格式传输\n * 		
 * 		- 中断或轮询模式通讯 * 		
 * 		- 5个错误检测标志
 * 			- I2C超时错误
 * 			- I2C接收溢出错误
 * 			- I2C发送溢出错误
 * 			- I2C接收下溢错误
 * 			-    仲裁丢失错误
 * 		- 9个带标志的中断源
 * 			- I2C传输完成中断
 * 			- I2C接收到数据中断
 * 			- I2C无应答中断
 * 			- I2C传输超时中断
 * 			- I2C监测从机就绪中断
 * 			- I2C接收溢出中断
 * 			- I2C发送溢出中断
 * 			- I2C接收下溢中断
 * 			- I2C仲裁丢失中断
 *******************************************************************************
 */

/* 防止递归包含 -----------------------------------------------------------------*/
#pragma once

/* 引用头文件 ------------------------------------------------------------------*/
#include "hal_cortex.h"
#include "hw_memmap.h"


/** 
  * @brief  I2C工作状态枚举
  */
typedef enum
{
  HAL_I2C_STATE_RESET             = 0x00U,   /*!< I2C外设在初始化状态			   */
  HAL_I2C_STATE_READY             = 0x01U,   /*!< I2C外设已初始化并就绪                                    */
  HAL_I2C_STATE_BUSY              = 0x02U,   /*!< I2C外设处于忙状态                                             */
  HAL_I2C_STATE_RXRW              = 0x08U,   /*!< I2C外设处于读写状态                                         */
  HAL_I2C_STATE_RXDV              = 0x20U,   /*!< I2C外设中有有效数据处于接收状态              */
  HAL_I2C_STATE_TXDV              = 0x40U,   /*!< I2C外设中有有效数据处于发送状态               */
  HAL_I2C_STATE_RXOVF             = 0x80U,   /*!< I2C外设接收溢出        			   */
  HAL_I2C_STATE_BA                = 0x100U,  /*!< I2C外设总线正常			   */
  HAL_I2C_STATE_FIFO_OVF          = 0x101U,  /*!< I2C外设FIFO溢出                                             */
  HAL_I2C_STATE_TIMEOUT           = 0x102U,  /*!< I2C外设接收或发送超时                                    */
  HAL_I2C_STATE_ERROR             = 0x104U   /*!< I2C外设状态错误                                                  */
} HAL_I2C_StateTypeDef;

/** 
  * @brief  I2C传输速度枚举
  */
typedef enum {
  HAL_I2C_CLK_SPEED_100K = 100000U,           /*!< I2C传输速度100K   */
  HAL_I2C_CLK_SPEED_400K = 400000U            /*!< I2C传输速度400K   */
} HAL_I2C_CLK_SpeedTypeDef;

/** 
  * @brief  I2C传输模式枚举
  */
typedef enum
{
  HAL_I2C_MODE_SLAVE     = 0x00U,             /*!< I2C从模式   */
  HAL_I2C_MODE_MASTER    = 0x02U,             /*!< I2C主模式   */
  HAL_I2C_MODE_NONE      = 0x04U              /*!< I2C模式空   */
}HAL_I2CModeTypeDef;

/** 
  * @brief  I2C传输地址模式枚举，7位或10位地址，当地址为最后一位为读写位时，需要用户将该地址右移一位
  */
typedef enum
{
  HAL_I2C_ADDRESS_10BITS = 0x00U,             /*!< I2C从机地址为10位   */
  HAL_I2C_ADDRESS_7BITS  = 0x04U              /*!< I2C从机地址为7位      */
}HAL_I2C_AddressModeTypeDef;


/**
  * @brief  I2C中断类型枚举
  */
typedef enum {
  HAL_I2C_INTTYPE_COMP                      = 0x01U,/*!< I2C传输完成中断              */
  HAL_I2C_INTTYPE_DATA                      = 0x02U,/*!< I2C无应答中断                  */
  HAL_I2C_INTTYPE_NACK                      = 0x04U,/*!< I2C接收到数据中断         */
  HAL_I2C_INTTYPE_TO                        = 0x08U,/*!< I2C传输超时中断             */
  HAL_I2C_INTTYPE_SLV_RDY                   = 0x10U,/*!< I2C监测从机就绪中断    */
  HAL_I2C_INTTYPE_RX_OVF                    = 0x20U,/*!< I2C接收溢出中断             */
  HAL_I2C_INTTYPE_TX_OVF                    = 0x40U,/*!< I2C发送溢出中断             */
  HAL_I2C_INTTYPE_RX_UNF                    = 0x80U,/*!< I2C接收下溢中断             */
  HAL_I2C_INTTYPE_ARB_LOST                  = 0x200U/*!< I2C仲裁丢失中断             */
} HAL_I2C_Int_TypeDef;

/** 
  * @brief  I2C错误码类型枚举
  */
typedef enum
{
	HAL_I2C_ERROR_NONE		= 0x00U,	/*!<  I2C无错误			*/
	HAL_I2C_ERROR_TIMEOUT,				/*!<  I2C超时错误			*/
	HAL_I2C_ERROR_RX_OVF	= 0x20U,	/*!<  I2C接收溢出错误		*/
	HAL_I2C_ERROR_TX_OVF	= 0x40U,	/*!<  I2C发送溢出错误	    */
	HAL_I2C_ERROR_RX_UNF	= 0x80U,	/*!<  I2C接收下溢错误	    */
	HAL_I2C_ERROR_ARB_LOST	= 0x200U	/*!<  I2C仲裁丢失错误		*/
}HAL_I2C_ErrorCodeTypeDef;

/**
  * @brief I2C寄存器结构体
  */
typedef struct
{
  volatile uint32_t CONTROL;                         /*!< I2C控制寄存器         */
  volatile uint32_t STATUS;                          /*!< I2C状态寄存器         */
  volatile uint32_t ADDRESS;                         /*!< I2C地址寄存器         */
  volatile uint32_t DATA;                            /*!< I2C传输数据寄存器 */
  volatile uint32_t INT_STATUS;                      /*!< I2C中断状态寄存器 */
  volatile uint32_t TRAN_SIZE;                       /*!< I2C传输长度寄存器 */
  volatile uint32_t SLAVE_MONITOR_PAUSE;             /*!< I2C检测停止寄存器 */
  volatile uint32_t TIMEOUT;                         /*!< I2C超时寄存器          */
  volatile uint32_t INT_MASK;                        /*!< I2C中断掩码寄存器 */
  volatile uint32_t INT_ENABLE;                      /*!< I2C中断使能寄存器 */
  volatile uint32_t INT_DISABLE;                     /*!< I2C中断失能寄存器 */
} HAL_I2C_TypeDef;

/**
  * @brief  I2C初始化结构体
  */
typedef struct
{
  HAL_I2CModeTypeDef Mode;                   /*!< I2C传输模式，详见    @brf  HAL_I2CModeTypeDef           */

  HAL_I2C_CLK_SpeedTypeDef ClockSpeed;       /*!< I2C传输速度，详见    @brf  HAL_I2C_CLK_SpeedTypeDef     */

  uint32_t OwnAddress;                       /*!< I2C传输地址                                                                                                                                           */

  HAL_I2C_AddressModeTypeDef AddressingMode; /*!< I2C传输地址模式，详见    @brf HAL_I2C_AddressModeTypeDef */
} HAL_I2C_InitTypeDef;

/**
  * @brief  I2C控制结构体
  */
typedef struct __I2C_HandleTypeDef
{
  HAL_I2C_TypeDef             *Instance;      /*!< I2C外设寄存器基地址                  */

  HAL_I2C_InitTypeDef         Init;           /*!< I2C初始化配置参数                     */

  uint8_t                     *pBuffPtr;      /*!< I2C数据传输地址                          */

  volatile uint16_t           XferSize;       /*!< I2C数据传输大小                          */

  volatile uint16_t           XferCount;      /*!< I2C数据传输大小计数                */

  HAL_LockTypeDef             Lock;           /*!< I2C设备锁                                      */

  volatile HAL_I2C_StateTypeDef  State;       /*!< I2C传输状态                                 */

  volatile HAL_I2CModeTypeDef    Mode;        /*!< I2C传输模式                                 */

  volatile uint32_t           ErrorCode;      /*!< I2C错误码                                     */

  volatile uint32_t           Devaddress;     /*!< I2C目标设备地址                        */

} HAL_I2C_HandleTypeDef;


/**
* @brief  I2C 外设.
*/
#define HAL_I2C1               ((HAL_I2C_TypeDef *)I2C1_BASE)  /*!< I2C1外设基地址   */
#define HAL_I2C2               ((HAL_I2C_TypeDef *)I2C2_BASE)  /*!< I2C2外设基地址   */


/** @brief  使能I2C中断.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  __INTERRUPT__：I2C中断类型，详见枚举类型  @ref HAL_I2C_Int_TypeDef.
  * @retval 无
  */
#define HAL_I2C_ENABLE_IT(__HANDLE__,__INTERRUPT__) SET_BIT((__HANDLE__)->Instance->INT_ENABLE,(__INTERRUPT__))

/** @brief  失能I2C中断.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  __INTERRUPT__：I2C中断类型，详见枚举类型  @ref HAL_I2C_Int_TypeDef.
  * @retval 无
  */
#define HAL_I2C_DISABLE_IT(__HANDLE__,__INTERRUPT__) SET_BIT((__HANDLE__)->Instance->INT_DISABLE,(__INTERRUPT__))

/** @brief  清除I2C中断标志位.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  __INTERRUPT__：I2C中断类型，详见枚举类型  @ref HAL_I2C_Int_TypeDef.
  * @retval 无
  */
#define HAL_I2C_CLEAR_IT(__HANDLE__,__INTERRUPT__) SET_BIT((__HANDLE__)->Instance->INT_STATUS,(__INTERRUPT__))

/** @brief  获取I2C状态标志.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  __FLAG__：I2C状态， 详见  @ref HAL_I2C_StateTypeDef.
  * @retval 无.
  */
#define HAL_I2C_GET_FLAG(__HANDLE__,__FLAG__) (((((__HANDLE__)->Instance->STATUS) & (__FLAG__)) == (__FLAG__)) ? HAL_SET : HAL_RESET)

/** @brief  清除I2C状态标志.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  __FLAG__：I2C状态， 详见  @ref HAL_I2C_StateTypeDef.
  * @retval 无
  */
#define HAL_I2C_CLEAR_FLAG(__HANDLE__,__FLAG__) (((__HANDLE__)->Instance->STATUS) &= (~(__FLAG__)))

/** @brief  设置I2C地址宽度，7位还是10位，详见  @ref HAL_I2C_AddressModeTypeDef.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_SET_ADDR_MODE(__HANDLE__) (MODIFY_REG((__HANDLE__)->Instance->CONTROL,I2C_CTL_NEA_Msk,((__HANDLE__)->Init.AddressingMode)))

/** @brief  设置I2C作为从机时的地址.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_SET_SLAVE_OWN_ADDR(__HANDLE__) (MODIFY_REG((__HANDLE__)->Instance->ADDRESS,0x3FF,((__HANDLE__)->Init.OwnAddress)))

/** @brief  设置I2C作为主机时目标从机的地址，若地址中最低位表示读写位，则需将该地址右移一位.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  __ADDR__：需要设置的地址.
  * @retval 无
  */
#define HAL_I2C_SET_MASTER_DIR_ADDR(__HANDLE__,__ADDR__) (WRITE_REG((__HANDLE__)->Instance->ADDRESS,(__ADDR__)))

/** @brief  清空I2C的FIFO.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_CLEAN_FIFO(__HANDLE__) (MODIFY_REG((__HANDLE__)->Instance->CONTROL,I2C_CTL_CLR_FIFO_Msk,I2C_CTL_CLR_FIFO_EN))

/** @brief  设置I2C时钟分频系数.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  __VAL__：I2C分频系数.
  * @retval 无
  */
#define HAL_I2C_SET_EXP_CLK(__HANDLE__,__VAL__) (MODIFY_REG((__HANDLE__)->Instance->CONTROL,I2C_CTL_DIVISOR_Msk,(__VAL__)))

/** @brief  设置I2C主从模式.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_SET_MODE(__HANDLE__) (MODIFY_REG((__HANDLE__)->Instance->CONTROL,I2C_CTL_MS_Msk,(__HANDLE__)->Init.Mode))

/** @brief  使能主模式下的传输应答模式.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_SET_ACKMODE(__HANDLE__) (MODIFY_REG((__HANDLE__)->Instance->CONTROL,I2C_CTL_ACKEN_Msk,I2C_CTL_ACKEN_EN))

/** @brief  清除I2C控制寄存器.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_CLEAR_CONFIG(__HANDLE__) (MODIFY_REG((__HANDLE__)->Instance->CONTROL,0xFFFF,0x0))

/** @brief  设置I2C为写方向。
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_SET_TRAN_DIRECTION_WRITE(__HANDLE__) (MODIFY_REG((__HANDLE__)->Instance->CONTROL,I2C_CTL_M_RW_Msk,I2C_CTL_M_WRITE))

/** @brief  设置I2C为读方向.
  * @param  __HANDLE__：I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
#define HAL_I2C_SET_TRAN_DIRECTION_READ(__HANDLE__) (SET_BIT((__HANDLE__)->Instance->CONTROL,I2C_CTL_M_READ))


/**
  * @brief  获取I2C工作状态.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 返回I2C工作状态. 详见  @ref HAL_I2C_StateTypeDef.
  */
HAL_I2C_StateTypeDef HAL_I2C_GetState(HAL_I2C_HandleTypeDef* I2CHandleStruct);

/**
  * @brief  获取I2C错误状态.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 返回I2C错误码.详见  @ref HAL_I2C_ErrorCodeTypeDef。
  */
uint32_t HAL_I2C_GetError(HAL_I2C_HandleTypeDef *I2CHandleStruct);

/**
  * @brief  获取I2C的传输模式，即I2C的模式是主模式还是从模式还是无。
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval I2C工作模式。详见  @ref HAL_I2CModeTypeDef。
  */
HAL_I2CModeTypeDef HAL_I2C_GetMode(HAL_I2C_HandleTypeDef* I2CHandleStruct);

/** @brief  使能I2C外设时钟。
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无。
  */
void HAL_I2C_ENABLE(HAL_I2C_HandleTypeDef* I2CHandleStruct);

/** @brief  失能I2C外设时钟。
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无。
  */
void HAL_I2C_DISABLE(HAL_I2C_HandleTypeDef* I2CHandleStruct);

/**
  * @brief  初始化I2C.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_I2C_Init(HAL_I2C_HandleTypeDef* I2CHandleStruct);

/**
  * @brief  去初始化I2C.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
  *   返回值可能是以下类型：
  *       @retval  HAL_OK       ：表示去初始化外设成功
  *       @retval  HAL_ERROR    ：表示入参错误
  */
HAL_StatusTypeDef HAL_I2C_DeInit(HAL_I2C_HandleTypeDef* I2CHandleStruct);

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

HAL_StatusTypeDef HAL_I2C_Master_Transmit(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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

HAL_StatusTypeDef HAL_I2C_Master_Receive(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/**
  * @brief  I2C从机模式下发送数据阻塞函数，Timeout为阻塞时间。
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
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
HAL_StatusTypeDef HAL_I2C_Slave_Transmit(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_I2C_Slave_Receive(HAL_I2C_HandleTypeDef* I2CHandleStruct, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/**
  * @brief  I2C从机模式下接收数据中断模式。
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @param  pData：要接收的数据的存储地址。
  * @param  Size：要接收的数据的长度。
  * @retval HAL状态.详见  @ref HAL_StatusTypeDef。
  */
HAL_StatusTypeDef HAL_I2C_Slave_Receive_IT(HAL_I2C_HandleTypeDef *I2CHandleStruct, uint8_t *pData, uint16_t Size);

/** @brief  获取I2C中断使能状态.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval I2C中断使能结果.详见  @ref HAL_I2C_Int_TypeDef.
  */
uint32_t HAL_I2C_GetITEnable(HAL_I2C_HandleTypeDef* I2CHandleStruct);

/** @brief  获取I2C中断状态.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval I2C当前中断状态.详见  @ref HAL_I2C_Int_TypeDef.
  */
uint32_t HAL_I2C_GetITState(HAL_I2C_HandleTypeDef* I2CHandleStruct);

/**
  * @brief  I2C错误中断回调函数.在对应的CSP中断处理函数中调用.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
void HAL_I2C_ErrorCallback(HAL_I2C_HandleTypeDef *I2CHandleStruct) __RAM_FUNC;

/**
  * @brief  I2C接收完成回调函数.在对应的I2C中断处理函数中调用.
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
void HAL_I2C_RxCpltCallback(HAL_I2C_HandleTypeDef *I2CHandleStruct) __RAM_FUNC;

/**
  * @brief  I2C中断处理函数.在对应的中断服务函数中调用..
  * @param  I2CHandleStruct. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
  * @retval 无
  */
void I2C_IRQHandler(HAL_I2C_HandleTypeDef *I2CHandleStruct) __RAM_FUNC;

/**
  * @brief  I2C1中断服务函数.I2C1中断触发时调用此函数.
  * @param  无
  * @retval 无
  */
void HAL_I2C1_IRQHandler(void) __RAM_FUNC;

/**
  * @brief  I2C2中断服务函数.I2C2中断触发时调用此函数.
  * @param  None
  * @retval None
  */
void HAL_I2C2_IRQHandler(void) __RAM_FUNC;






