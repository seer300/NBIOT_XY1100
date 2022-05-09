/*
 * @file      hal_wakeup_demo.c
 * @ingroup   peripheral
 * @brief     此demo使用GPI唤醒standby，在正常模式下，GPIO13为SPI的片选脚，进入standby模式时，GPI使用GPIO13作为唤醒引脚。
 * @warning   其中gpio10/11/12不可作为外部唤醒源，不使用的唤醒源需将其gpio配置成GPIO_WAKEUP_UNUSE
 * 			  !!!唤醒触发方式只能为低电平唤醒，低电平时间不得小于100us，最好不要超过1ms,用户需要自行通过硬件来保证!!!
 */
#if DEMO_TEST

#include <string.h>
#include "hal_gpio.h"
#include "hal_gpi.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "low_power.h"
#include "hal_spi.h"

#define GPIO_WAKEUP_PIN         HAL_GPIO_PIN_NUM_13           //唤醒源的gpio
#define GPIO_WAKEUP_PIN_UNUSE   GPIO_WAKEUP_UNUSE            //不用的唤醒源需将gpio配置成GPIO_WAKEUP_UNUSE

#define SPI_MOSI_PIN        HAL_GPIO_PIN_NUM_6
#define SPI_MISO_PIN        HAL_GPIO_PIN_NUM_9
#define SPI_SCLK_PIN        HAL_GPIO_PIN_NUM_8
#define SPI_NSS1_PIN        HAL_GPIO_PIN_NUM_13

HAL_GPI_InitTypeDef gpi_init;
osMessageQueueId_t gpi_demo_msg_q = NULL;//消息句柄
HAL_SPI_HandleTypeDef spi_master;
osThreadId_t  gpi_wakeup_demo_handle = NULL;

/**
 * @brief	SPI初始化函数，这个函数描述了SPI主模式初始化需要的相关步骤。\n
 * 			在初始化函数内部需要设置SPI主设备的MISO、MOSI、SCLK、NSS引脚与GPIO引脚的对应关系、上下拉状态 \n
 * 			以及SPI的主从模式、工作模式、时钟分频、数据宽度与传输方向以及片选管理等\n
 */

void spi_init(void)
{
	HAL_GPIO_InitTypeDef spi_master_gpio;

	//SPI主设备参数配置
	spi_master.Instance = HAL_SPI;
	//配置为SPI主设备
	spi_master.Init.Mode = HAL_SPI_MODE_MASTER;
	//设配置为工作模式0，CPOL==0&&CPHA==0
	spi_master.Init.WorkMode = HAL_SPI_WORKMODE_0;
	//设置SPI的传输长度为8位
	spi_master.Init.DataSize = HAL_SPI_DATASIZE_8BIT;
	//设置SPI时钟为 pclk32分频
	spi_master.Init.SPI_CLK_DIV = HAL_SPI_CLKDIV_32;
	//设置SPI软件片选管理模式
	spi_master.Init.NSS = HAL_SPI_NSS_SOFT;

	//映射SPI的MOSI引脚
	spi_master_gpio.Pin = SPI_MOSI_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_master_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_master_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的MOSI引脚
	spi_master_gpio.Alternate = HAL_REMAP_SPI_MOSI;
	//使能之前的配置
	HAL_GPIO_Init(&spi_master_gpio);

	//映射SPI的MISO引脚
	spi_master_gpio.Pin = SPI_MISO_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_master_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_master_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的MISO引脚
	spi_master_gpio.Alternate = HAL_REMAP_SPI_MISO;
	//使能之前的配置
	HAL_GPIO_Init(&spi_master_gpio);

	//映射SPI的SCLK引脚
	spi_master_gpio.Pin = SPI_SCLK_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_master_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_master_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的SCLK引脚
	spi_master_gpio.Alternate = HAL_REMAP_SPI_SCLK;
	//使能之前的配置
	HAL_GPIO_Init(&spi_master_gpio);

	//映射SPI的NSS1引脚
	spi_master_gpio.Pin = SPI_NSS1_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_master_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_master_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的NSS1引脚
	spi_master_gpio.Alternate = HAL_REMAP_SPI_NSS1;
	//使能之前的配置
	HAL_GPIO_Init(&spi_master_gpio);

	//初始化SPI
	if(HAL_SPI_Init(&spi_master) != HAL_OK)
	{
		xy_assert(0);
	}
}

/**
 * @brief 这个函数需放入user_standby_before_hook中，功能为配置GPI唤醒源
 * 
 */
void init_gpi(void)
{
	HAL_SPI_DeInit(&spi_master);
	
	//将GPIO口映射至GPI
	gpi_init.WakePin[0] = GPIO_WAKEUP_PIN;
	gpi_init.WakePin[1] = GPIO_WAKEUP_UNUSE;
	gpi_init.WakePin[2] = GPIO_WAKEUP_UNUSE;

	//初始化GPI
	HAL_StatusTypeDef ret = HAL_GPI_Init(&gpi_init);

	if(ret != HAL_OK)
	{
		xy_assert(0);
	}

}

/**
 * @brief 这个函数需放入user_standby_after_hook中，用于解除GPI的映射关系，并将GPIO13配置为SPI的片选脚
 * 
 */
void deinit_gpi(void)
{
	HAL_StatusTypeDef ret = HAL_GPI_Deinit(&gpi_init);

	if(ret != HAL_OK)
	{
		xy_assert(0);
	}

	spi_init();
}


/**
 * @brief 这里使用messageQ将中断源标志位传递给处理线程
 * 
 * @param SrcValue 为GPI唤醒源，以BIT的方式表述此次唤醒是由哪个GPI源导致的，其中 LSB开始BIT 0表示GPI唤醒源1; BIT 1表示GPI唤醒源2; BIT 2表示GPI唤醒源3
 */
void HAL_GPI_CallBack(uint8_t SrcValue)
{
	if(gpi_demo_msg_q != NULL)
	{
		osMessageQueuePut(gpi_demo_msg_q, (void*)(&SrcValue), 0, osNoWait);
	}
}

/**
 * @brief gpi demo线程入口函数
 */	
void gpi_demo_entry()
{
	uint8_t gpi_wakeup_src = 0;

	if(gpi_demo_msg_q == NULL)
	{
		gpi_demo_msg_q = osMessageQueueNew(10, sizeof(uint8_t), NULL);
	}

	while(1)
	{
		osMessageQueueGet(gpi_demo_msg_q, (void *)(&gpi_wakeup_src), NULL, osWaitForever);

		//以BIT为的方式表述此次唤醒是由哪个GPI源导致的，其中 LSB开始BIT 0表示GPI唤醒源1; BIT 1表示GPI唤醒源2; BIT 2表示GPI唤醒源3
		if(gpi_wakeup_src & 0x01)
		{
			xy_standby_lock();

			lpm_string_output("GPI wakeup resource is 1\r\n", strlen("GPI wakeup resource is 1\r\n"));

			uint8_t txdata[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
			uint8_t rxdata[8] = {0};
	
			for(uint8_t i = 0; i < sizeof(txdata); i++)
			{
				//拉低片选引脚
				HAL_SPI_ChipSelect(&spi_master, HAL_SPI_CS_PIN_1);
				//发送1个字节，接收1个字节
				HAL_SPI_TransmitReceive(&spi_master, &txdata[i], &rxdata[i], 1, HAL_MAX_DELAY);
				//拉高片选引脚
				HAL_SPI_ChipSelect(&spi_master, HAL_SPI_CS_NONE);				
			}	

			xy_standby_unlock();
		}

	}
}

/**
 * @brief 用户任务初始化函数，在user_task_init中添加
 * @attention   
 */	
void gpi_demo_task_init()
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "gpi_wakeup_demo";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 0x400;
	gpi_wakeup_demo_handle = osThreadNew ((osThreadFunc_t)(gpi_demo_entry), NULL, &thread_attr); 
}

#endif
