/**
* @file		hal_csp_irda_demo.c
* @ingroup	peripheral
* @brief	CSP_IRDA模式Demo
*
* @note		CSP1默认用于SIMCARD，CSP2默认用于AT，CSP3默认用于LOG，CSP4可以自由配置
* 			此处配置为IRDA模式，可以通过配置IRDA_Datawidth来配置IRDA的输出电平的脉冲宽度
*
* @warning	用户使用外设时需注意：1.使用前关闭standby，使用完开启standby 
                                  2.确保没有在NV中配置外设所使用的GPIO，否则芯片进出standby时会根据NV重配GPIO，导致外设无法正常使用
***********************************************************************************/
#if DEMO_TEST

#include "xy_api.h"

#define CSP_IRDA_REMAP_TX       HAL_REMAP_CSP4_TXD
#define CSP_IRDA_REMAP_RX       HAL_REMAP_CSP4_RXD
#define CSP_IRDA_TX_PIN         HAL_GPIO_PIN_NUM_6
#define CSP_IRDA_RX_PIN         HAL_GPIO_PIN_NUM_7

osThreadId_t  g_hal_csp_irda_demo_TskHandle = NULL;

HAL_CSP_HandleTypeDef hal_csp_irda_demo_handle;

/**
* @brief	csp_IRDA初始化函数，这个函数描述了进行串口初始化需要的相关步骤，
* 			在初始化内部需要设置串口的RX TX引脚与GPIO引脚的对应关系、上下拉状态，
* 			以及串口的波特率、数据长度、停止位、脉冲宽度等 
*/
void hal_csp_irda_demo_peri_init(void)
{
    HAL_GPIO_InitTypeDef hal_csp_irda_demo_gpio_init;

    hal_csp_irda_demo_gpio_init.Pin		    = CSP_IRDA_TX_PIN;
    hal_csp_irda_demo_gpio_init.Mode		= GPIO_MODE_AF_PER;
    hal_csp_irda_demo_gpio_init.Pull		= GPIO_NOPULL;
    hal_csp_irda_demo_gpio_init.Alternate	= CSP_IRDA_REMAP_TX;
    HAL_GPIO_Init(&hal_csp_irda_demo_gpio_init);

    hal_csp_irda_demo_gpio_init.Pin		    = CSP_IRDA_RX_PIN;
    hal_csp_irda_demo_gpio_init.Mode 		= GPIO_MODE_AF_INPUT;
    hal_csp_irda_demo_gpio_init.Pull		= GPIO_PULLUP;
    hal_csp_irda_demo_gpio_init.Alternate 	= CSP_IRDA_REMAP_RX;
    HAL_GPIO_Init(&hal_csp_irda_demo_gpio_init);				

    hal_csp_irda_demo_handle.Instance			 = HAL_CSP4;
    hal_csp_irda_demo_handle.Init.UART_BaudRate	 = 9600;
    hal_csp_irda_demo_handle.Init.UART_DataBits	 = HAL_CSP_DATA_BITS_8;
    hal_csp_irda_demo_handle.Init.UART_StopBits	 = HAL_CSP_STOP_BITS_1;
    hal_csp_irda_demo_handle.Init.IRDA_Datawidth = HAL_IRDA_DATA_WIDTH_1_6;
    hal_csp_irda_demo_handle.Init.IRDA_Idellevel = HAL_IRDA_IDEL_LEVEL_H;

    HAL_CSP_IRDA_Init(&hal_csp_irda_demo_handle);

    //使能CSP
    HAL_CSP_Ctrl(&hal_csp_irda_demo_handle, HAL_ENABLE);			
}

/**
 * @brief	在这个Demo中，用户可以通过示波器看到TX的输出电平脉冲是否和初始化时的配置一致，
 * 			HAL_IRDA_DATA_WIDTH_1_6:1.6us，HAL_IRDA_DATA_WIDTH_3_16:(1000000/9600)*(3/16)us
 */
void hal_csp_irda_demo_task(void)
{
    uint8_t txdata[4] = {'U', 'U', 'U', 'U'};  //0X55

    hal_csp_irda_demo_peri_init();

    while(1)
    {
    	xy_standby_lock();

        for(uint8_t i = 0; i < sizeof(txdata); i++)
        {
            //发送1个字节，设置超时时间为500ms
            HAL_CSP_Transmit(&hal_csp_irda_demo_handle, &txdata[i], 1, 500);	
        }

        xy_standby_unlock();

        //通过osDelay释放线程控制权
        osDelay(1000);															
    }
}

/**
 * @brief	任务创建
 */
void hal_csp_irda_demo_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "hal_csp_irda_demo_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 512;
    g_hal_csp_irda_demo_TskHandle = osThreadNew((osThreadFunc_t)hal_csp_irda_demo_task, NULL, &thread_attr);
}

#endif

