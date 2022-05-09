/**
* @file		hal_i2c_master_demo.c
* @ingroup	peripheral
* @brief	I2C主机工作模式Demo
 * 在这个Demo中,I2C被配置为主机，先向从机写（发送）8个字节的数据，再从从机读（接收）8个字节的数据，每次发送完后将\n
 *   	                                 发送数据的值统一加8，从而更改发送的数据.\n
* @warning	用户使用外设时需注意：1.使用前关闭standby，使用完开启standby 2.确保没有在NV中配置外设所使用的GPIO，否则芯片进出standby时会根据NV重配GPIO，导致外设无法正常使用
***********************************************************************************/

#if DEMO_TEST


#include "xy_api.h"

//任务参数配置
#define I2C_SCL_PIN        HAL_GPIO_PIN_NUM_6
#define I2C_SDA_PIN        HAL_GPIO_PIN_NUM_7

//任务全局变量
osThreadId_t  g_i2c_master_test_TskHandle = NULL;
HAL_I2C_HandleTypeDef i2c_master;

/**
*
*   @brief	 I2C主机初始化函数，这个函数描述了将I2C1初始化为主机需要的相关步骤。 \n
*         	  在初始化函数内部需要设置I2C的SCL、SDA引脚与GPIO引脚的对应关系、上下拉状态，I2C从机地址，传输速度等 \n
*
*/

void hal_i2c_master_demo_init(void)
{
	HAL_GPIO_InitTypeDef i2c_master_gpio;

	//I2C参数配置
	i2c_master.Instance = HAL_I2C1;
	//设置从机地址宽度为7位
	i2c_master.Init.AddressingMode = HAL_I2C_ADDRESS_7BITS;
	//设置I2C传输速度为100Kbit/s
	i2c_master.Init.ClockSpeed = HAL_I2C_CLK_SPEED_100K;
	//设置I2C为主模式
	i2c_master.Init.Mode = HAL_I2C_MODE_MASTER;

	//映射I2C的SCL引脚
	i2c_master_gpio.Pin = I2C_SCL_PIN;
	i2c_master_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上拉
	i2c_master_gpio.Pull = GPIO_NOPULL;
	//设置GPIO6引脚为I2C的同步时钟引脚SCLK
	i2c_master_gpio.Alternate = HAL_REMAP_I2C1_SCLK;
	//使能之前的引脚配置
	HAL_GPIO_Init(&i2c_master_gpio);

	//映射I2C的SDA引脚
	i2c_master_gpio.Pin = I2C_SDA_PIN;
	//设置引脚为自动模式，此时芯片会根据引脚的复用状态自动选择模式
	i2c_master_gpio.Mode = GPIO_MODE_AF_PER;
	//设置引脚内部无上拉
	i2c_master_gpio.Pull = GPIO_NOPULL;
	//设置GPIO7引脚为I2C的数据引脚SDA
	i2c_master_gpio.Alternate = HAL_REMAP_I2C1_SDA;
	//使能之前的引脚配置
	HAL_GPIO_Init(&i2c_master_gpio);

	//初始化I2C
	if(HAL_I2C_Init(&i2c_master) != HAL_OK)
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
 在这个Demo中,I2C作为通讯的主机，先向从机写8个字节的数据，再从从机读8个字节的数据，每次发送完后将\n
 *   	     发送数据的值统一加8，从而更改发送的数据.\n
 *  
  */
void i2c_master_test_task(void)
{
	uint8_t txdata[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
	uint8_t rxdata[9] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0};
	uint16_t devaddress = 0x24;

	hal_i2c_master_demo_init();

	while(1)
	{
		xy_standby_lock();
		//测试1
		//发送8个字节，超时时间设置为HAL_MAX_DELAY，即一直等待到8个字节发送完成
		HAL_I2C_Master_Transmit(&i2c_master, devaddress, txdata, 8, HAL_MAX_DELAY);
		//接收8个字节，超时时间设置为HAL_MAX_DELAY，即一直等待到8个字节接收完成
		HAL_I2C_Master_Receive(&i2c_master, devaddress, rxdata, 8, HAL_MAX_DELAY);
/*
		//测试2
		//发送8个字节，超时时间设置为500ms
		HAL_I2C_Master_Transmit(&i2c_master, devaddress, txdata, 8, 500);
		//接收8个字节，超时时间设置为500ms
		HAL_I2C_Master_Receive(&i2c_master, devaddress, rxdata, 8, 500);
*/
		//打印接收到的字符，用于调试
		send_debug_str_to_at_uart((char *)rxdata);
		//改变发送的字符，以示区分
		for(uint8_t i = 0; i < sizeof(txdata); i++)
		{
			txdata[i] += 8;
		}
		xy_standby_unlock();
		//通过osDelay释放线程控制权
		osDelay(1000);
	}
}

/**
 * @brief 任务创建
 */
void i2c_master_test_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "i2c_master_test_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 512;
    g_i2c_master_test_TskHandle = osThreadNew((osThreadFunc_t)i2c_master_test_task, NULL, &thread_attr);
}


#endif
