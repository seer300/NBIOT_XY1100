/**
* @file		hal_i2c_slave_demo.c
* @ingroup	peripheral
* @brief	I2C从机工作模式Demo
*			在这个Demo中,I2C被配置为从机，先阻塞接收主机发送（写）过来的数据，再将主机需要读的数据发出去。\n
*			
*
* @warning	用户使用外设时需注意：1.使用前关闭standby，使用完开启standby 2.确保没有在NV中配置外设所使用的GPIO，否则芯片进出standby时会根据NV重配GPIO，导致外设无法正常使用
***********************************************************************************/

#if DEMO_TEST


#include "xy_api.h"

//任务参数配置
#define I2C_SCL_PIN        HAL_GPIO_PIN_NUM_6
#define I2C_SDA_PIN        HAL_GPIO_PIN_NUM_7

//任务全局变量
osThreadId_t  g_i2c_slave_test_TskHandle = NULL;
HAL_I2C_HandleTypeDef i2c_slave;

/**
 *  @brief	I2C从机初始化函数，这个函数描述了将I2C1初始化为从机需要的相关步骤。 \n
 *			在初始化函数内部需要设置I2C的SCL、SDA引脚与GPIO引脚的对应关系、上下拉状态，I2C从机地址，传输速度等 \n
 *
 */
void i2c_slave_test_init(void)
{
	HAL_GPIO_InitTypeDef i2c_slave_gpio;

	//I2C参数配置
	i2c_slave.Instance = HAL_I2C1;
	//设置从机地址宽度为7位
	i2c_slave.Init.AddressingMode = HAL_I2C_ADDRESS_7BITS;
	//设置I2C传输速度为100Kbit/s
	i2c_slave.Init.ClockSpeed = HAL_I2C_CLK_SPEED_100K;
	//设置I2C为从模式
	i2c_slave.Init.Mode = HAL_I2C_MODE_SLAVE;
	//设置从机地址为0x24
	i2c_slave.Init.OwnAddress = 0x24;

	//映射I2C的SCL引脚
	i2c_slave_gpio.Pin = I2C_SCL_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	i2c_slave_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上拉
	i2c_slave_gpio.Pull = GPIO_NOPULL;
	//设置GPIO6引脚为I2C的同步时钟引脚SCLK
	i2c_slave_gpio.Alternate = HAL_REMAP_I2C1_SCLK;
	//使能之前的引脚配置
	HAL_GPIO_Init(&i2c_slave_gpio);

	//映射I2C的SDA引脚
	i2c_slave_gpio.Pin = I2C_SDA_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	i2c_slave_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上拉
	i2c_slave_gpio.Pull = GPIO_NOPULL;
	//设置GPIO7引脚为I2C的数据引脚SDA
	i2c_slave_gpio.Alternate = HAL_REMAP_I2C1_SDA;
	//使能之前的引脚配置
	HAL_GPIO_Init(&i2c_slave_gpio);

	//初始化I2C
	if(HAL_I2C_Init(&i2c_slave) != HAL_OK)
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
 * I2C作为通讯从机，先阻塞接收主机发送（写）过来的数据，再将主机需要读的数据发出去。\n
 *  	                   
 *
 */
void i2c_slave_test_task(void)
{
	uint8_t data[9] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0};

	i2c_slave_test_init();

	while(1)
	{
		xy_standby_lock();
		//接收8个字节，超时时间设置为HAL_MAX_DELAY，即一直等待到8个字节接收完成
		HAL_I2C_Slave_Receive(&i2c_slave, data, 8, HAL_MAX_DELAY);
		//打印接收到的字符，用于调试
		send_debug_str_to_at_uart((char *)data);
		//发送8个字节，超时时间设置为HAL_MAX_DELAY，即一直等待到8个字节发送完成
		HAL_I2C_Slave_Transmit(&i2c_slave, data, 8, HAL_MAX_DELAY);

		//接收8个字节，超时时间设置为500ms
//		HAL_I2C_Slave_Receive(&i2c_slave, data, 8, 500);
		//打印接收到的字符，用于调试
//		send_debug_str_to_at_uart((char *)data);
		//发送8个字节，超时时间设置为500ms
//		HAL_I2C_Slave_Transmit(&i2c_slave, data, 8, 500);

		xy_standby_unlock();
		//通过osDelay释放线程控制权		
		osDelay(1000);															
	}
}

/**
 * @brief 任务创建
 *
 */
void i2c_slave_test_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "i2c_slave_test_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 512;
	g_i2c_slave_test_TskHandle = osThreadNew((osThreadFunc_t)i2c_slave_test_task, NULL, &thread_attr);
}


#endif
