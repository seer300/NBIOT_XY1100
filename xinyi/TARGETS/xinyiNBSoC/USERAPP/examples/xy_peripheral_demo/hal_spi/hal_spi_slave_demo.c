/**
* @file		hal_spi_slave_demo.c
* @ingroup	peripheral
* @brief	SPI从设备工作模式Demo
			这个Demo为MCU作为SPI从设备通讯的演示，在这个Demo中，使用PC端的SPI通讯调试工具作为主设备，由主设备发起通信，主从设备每次进行8个字节的数据交换。
* @warning	用户使用外设时需注意：1.使用前关闭standby，使用完开启standby 2.确保没有在NV中配置外设所使用的GPIO，否则芯片进出standby时会根据NV重配GPIO，导致外设无法正常使用
***********************************************************************************/

#if DEMO_TEST


#include "xy_api.h"

//任务参数配置
#define SPI_MOSI_PIN        HAL_GPIO_PIN_NUM_6
#define SPI_MISO_PIN        HAL_GPIO_PIN_NUM_7
#define SPI_SCLK_PIN        HAL_GPIO_PIN_NUM_8
#define SPI_CS_PIN          HAL_GPIO_PIN_NUM_9

//任务全局变量
osThreadId_t  g_spi_slave_test_TskHandle = NULL;
HAL_SPI_HandleTypeDef spi_slave;

/**
 * @brief	SPI初始化函数，这个函数描述了SPI从模式初始化需要的相关步骤。\n
 * 			在初始化函数内部需要设置SPI从设备的MISO、MOSI、SCLK、NSS引脚与GPIO引脚的对应关系、上下拉状态 \n
 * 			以及SPI的主从模式、工作模式、时钟分频、数据宽度等\n
 *
 *
 */
void spi_slave_test_init(void)
{
	HAL_GPIO_InitTypeDef spi_slave_gpio;

	//SPI参数配置
	spi_slave.Instance = HAL_SPI;
	//配置为SPI从设备
	spi_slave.Init.Mode = HAL_SPI_MODE_SLAVE;
	//配置为工作模式0，CPOL==0&&CPHA==0
	spi_slave.Init.WorkMode = HAL_SPI_WORKMODE_0;
	//设置SPI的传输长度为8位
	spi_slave.Init.DataSize = HAL_SPI_DATASIZE_8BIT;

	//映射SPI的MOSI引脚
	spi_slave_gpio.Pin = SPI_MOSI_PIN;
	//设置引脚为外设复用模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_slave_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_slave_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的MOSI引脚
	spi_slave_gpio.Alternate = HAL_REMAP_SPI_MOSI;
	//使能之前的配置
	HAL_GPIO_Init(&spi_slave_gpio);

	//映射SPI的MISO引脚
	spi_slave_gpio.Pin = SPI_MISO_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_slave_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_slave_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的MISO引脚
	spi_slave_gpio.Alternate = HAL_REMAP_SPI_MISO;
	//使能之前的配置
	HAL_GPIO_Init(&spi_slave_gpio);

	//映射SPI的SCLK引脚
	spi_slave_gpio.Pin = SPI_SCLK_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_slave_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_slave_gpio.Pull = GPIO_NOPULL;
	//复用为SPI的SCLK引脚
	spi_slave_gpio.Alternate = HAL_REMAP_SPI_SCLK;
	//使能之前的配置
	HAL_GPIO_Init(&spi_slave_gpio);

	//映射SPI的CS引脚
	spi_slave_gpio.Pin = SPI_CS_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	spi_slave_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上下拉
	spi_slave_gpio.Pull = GPIO_NOPULL;
	//复用为SPI从设备的CS引脚
	spi_slave_gpio.Alternate = HAL_REMAP_SPI_CS;
	//使能之前的配置
	HAL_GPIO_Init(&spi_slave_gpio);

	//初始化SPI
	if(HAL_SPI_Init(&spi_slave) != HAL_OK)
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
 * 这个Demo为MCU作为SPI从设备通讯的演示，在这个Demo中，使用PC端的SPI通讯调试工具作为主设备，由主设备发起通信，主从设备每次进行8个字节的数据交换。
 *
 */
void spi_slave_test_task(void)
{
	uint8_t data[9] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0};

	spi_slave_test_init();

	while(1)
	{
		xy_standby_lock();
		//接收8个字节，超时时间设置为HAL_MAX_DELAY，即一直等待到8个字节接收完成
		HAL_SPI_Receive(&spi_slave, data, 8, HAL_MAX_DELAY);
		//发送8个字节，超时时间设置为HAL_MAX_DELAY，即一直等待到8个字节发送完成
		HAL_SPI_Transmit(&spi_slave, data, 8, HAL_MAX_DELAY);
		//打印接收到的字符，用于调试
		send_debug_str_to_at_uart((char *)data);
		xy_standby_unlock();
		//通过osDelay释放线程控制权
		osDelay(1000);	
	}
}

/**
 * @brief 任务创建
 *
 */
void spi_slave_test_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "spi_slave_test_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 512;
	g_spi_slave_test_TskHandle = osThreadNew((osThreadFunc_t)spi_slave_test_task, NULL, &thread_attr);
}


#endif
