/**
* @file		hal_spi_master_demo.c
* @ingroup	peripheral
* @brief	SPI主设备工作模式Demo
* @note 	在这个Demo中，MCU作为主设备，选中了CS_PIN_1的从设备，由MCU发起通信，主从设备每次阻塞收发一个字节的数据，\n
* 			共进行8个字节的数据交换。\n
* @warning	用户使用外设时需注意：1.使用前关闭standby，使用完开启standby 2.确保没有在NV中配置外设所使用的GPIO，否则芯片进出standby时会根据NV重配GPIO，导致外设无法正常使用
***********************************************************************************/

#if DEMO_TEST


#include "xy_api.h"

//任务参数配置
#define SPI_MOSI_PIN        HAL_GPIO_PIN_NUM_6
#define SPI_MISO_PIN        HAL_GPIO_PIN_NUM_7
#define SPI_SCLK_PIN        HAL_GPIO_PIN_NUM_8
#define SPI_NSS1_PIN        HAL_GPIO_PIN_NUM_9
#define SPI_NSS2_PIN        HAL_GPIO_PIN_NUM_20

//任务全局变量
osThreadId_t  g_spi_master_test_TskHandle = NULL;
HAL_SPI_HandleTypeDef spi_master;

/**
 * @brief	SPI初始化函数，这个函数描述了SPI主模式初始化需要的相关步骤。\n
 * 			在初始化函数内部需要设置SPI主设备的MISO、MOSI、SCLK、NSS引脚与GPIO引脚的对应关系、上下拉状态 \n
 * 			以及SPI的主从模式、工作模式、时钟分频、数据宽度与传输方向以及片选管理等\n
 *

 */

void spi_master_test_init(void)
{
	HAL_GPIO_InitTypeDef spi_master_gpio;

	//SPI参数配置
	spi_master.Instance = HAL_SPI;
	//设置为SPI主设备
	spi_master.Init.Mode = HAL_SPI_MODE_MASTER;
	//设置为工作模式0，CPOL==0&&CPHA==0
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

	//映射SPI的NSS2引脚
	spi_master_gpio.Pin = SPI_NSS2_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_master_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_master_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的NSS2引脚
	spi_master_gpio.Alternate = HAL_REMAP_SPI_NSS2;
	//使能之前的配置
	HAL_GPIO_Init(&spi_master_gpio);

	//初始化SPI
	if(HAL_SPI_Init(&spi_master) != HAL_OK)
	{
		send_debug_str_to_at_uart("\r\nerror\r\n");
	}
	else
	{
		send_debug_str_to_at_uart("\r\nnormal\r\n");
	}
}

/**
 * @brief 任务线程
 * 在这个Demo中，MCU作为主设备，由MCU发起通信，主从设备每次阻塞收发一个字节的数据，\n
 * 			共进行8个字节的数据交换。\n
 *
 */
void spi_master_test_task(void)
{
	uint8_t txdata[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
	uint8_t rxdata[9] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0};

	spi_master_test_init();

	while(1)
	{
		xy_standby_lock();
/*		//测试1，HAL_SPI_Transmit、HAL_SPI_Receive
		for(uint8_t i = 0; i < sizeof(txdata); i++)
		{
			HAL_SPI_ChipSelect(&spi_master, HAL_SPI_CS_PIN_1);
			HAL_SPI_Transmit(&spi_master, &txdata[i], 1, HAL_MAX_DELAY);
			HAL_SPI_Receive(&spi_master, &rxdata[i], 1, HAL_MAX_DELAY);
			HAL_SPI_ChipSelect(&spi_master, HAL_SPI_CS_NONE);
			txdata[i] += 5;
		}
*/
		//测试2，HAL_SPI_TransmitReceive
		for(uint8_t i = 0; i < sizeof(txdata); i++)
		{
			//拉低片选引脚
			HAL_SPI_ChipSelect(&spi_master, HAL_SPI_CS_PIN_1);
			//发送1个字节，接收1个字节
			HAL_SPI_TransmitReceive(&spi_master, &txdata[i], &rxdata[i], 1, HAL_MAX_DELAY);
			//拉高片选引脚
			HAL_SPI_ChipSelect(&spi_master, HAL_SPI_CS_NONE);
			//改变发送的字符，以示区分
			txdata[i] += 8;
		}
		//打印接收到的字符，用于调试
		send_debug_str_to_at_uart((char *)rxdata);
		xy_standby_unlock();
		//通过osDelay释放线程控制权
		osDelay(1000);
	}
}

/**
 * @brief 任务创建
 *
 */
void spi_master_test_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "spi_master_test_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 512;
	g_spi_master_test_TskHandle = osThreadNew((osThreadFunc_t)spi_master_test_task, NULL, &thread_attr);
}


#endif
