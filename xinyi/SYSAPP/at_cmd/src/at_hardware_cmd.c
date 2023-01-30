/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "csp.h"
#include "at_hardware_cmd.h"
#include "at_global_def.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "xy_utils.h"
#include "xy_clk_config.h"
#include "hal_gpio.h"
#include "hal_adc.h"
#include "at_com.h"
#include "sys_init.h"
#include "at_uart.h"
#include "xy_watchdog.h"
#include "low_power.h"
#include "osal.h"
#include "adc.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define XYCNNT_TEST_SUM (7 * 2)

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
/*bit 0-13.If set 1,set this PIN output,and test other pin input Ok or not
UART2_TXD
UART2_RXD
SPI_MOSI
SPI_MISO
SPI_CS
SPI_CLK
UART1_RTS
UART1_CTS
SYS_STATE
WAKEUP_OUT
GPIO_0
GPIO_1
I2C_SCL
I2C_SDA
*/

/*******************************************************************************
 *						 user set GPIO PIN here by self module				   *
 ******************************************************************************/
char g_PIN_ID[XYCNNT_TEST_SUM] = {HAL_GPIO_PIN_NUM_14, HAL_GPIO_PIN_NUM_17,
								HAL_GPIO_PIN_NUM_19, HAL_GPIO_PIN_NUM_20,
								HAL_GPIO_PIN_NUM_21, HAL_GPIO_PIN_NUM_18,
								HAL_GPIO_PIN_NUM_9, HAL_GPIO_PIN_NUM_13,

								HAL_GPIO_PIN_NUM_3, HAL_GPIO_PIN_NUM_5,

								 //HAL_GPIO_PIN_NUM_15, HAL_GPIO_PIN_NUM_16,
								HAL_GPIO_PIN_NUM_2, HAL_GPIO_PIN_NUM_6,

								HAL_GPIO_PIN_NUM_7, HAL_GPIO_PIN_NUM_8};

int g_XYCNNT_bitmap = 0X3FFF;


extern unsigned char otp_valid_flag;
/**
  * @brief  测试所选引脚输入输出是否正常（数字信号），两两测试：一个引脚输出高低电平。另一个引脚读取
  * @param  gpio_in:读取管脚高低电平的所选引脚
  * @param  gpio_out:输出高低电平的所选引脚
  * @param  is_High:@param  gpio_out的输出高电平或低电平
  *                 @arg 1:高电平
  *                 @arg 0:低电平
  * @retval 所选输入引脚读取所选输出引脚电平状态是否正确
  *         @arg 1:正确
  *         @arg 0:错误
  */
int check_connectivity(char gpio_in, char gpio_out, char is_High)
{
	unsigned char read_rlt;
	volatile unsigned int soft_delay;
	HAL_GPIO_InitTypeDef test_gpio;

	test_gpio.Pin = (gpio_out);
	test_gpio.Mode = GPIO_MODE_OUTPUT_PP;
	test_gpio.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(&test_gpio);

	if (is_High)
	{
		test_gpio.Pin = (gpio_in);
		test_gpio.Mode = GPIO_MODE_INPUT;
		test_gpio.Pull = GPIO_PULLDOWN;
		HAL_GPIO_Init(&test_gpio);
		HAL_GPIO_WritePin(gpio_out,HAL_GPIO_PIN_SET);
	}
	else
	{
		test_gpio.Pin = (gpio_in);
		test_gpio.Mode = GPIO_MODE_INPUT;
		test_gpio.Pull = GPIO_PULLUP;
		HAL_GPIO_Init(&test_gpio);
		HAL_GPIO_WritePin(gpio_out,HAL_GPIO_PIN_RESET);
	}

	for (soft_delay = 0; soft_delay < 1000; soft_delay++);

	read_rlt = HAL_GPIO_ReadPin(gpio_in);

	if ((is_High == 1 && read_rlt == 1) || (is_High == 0 && read_rlt == 0))
		return 1;
	else
		return 0;
}

/**
  * @brief  循环测试所选的所有引脚，并且读取SEN_4处的电压（需有分压电阻）
  * @param  prsp_cmd:,test_cmd:是否是测试类命令，1为测试类命令，0为查询类命令
  * @retval 最终结果
  *         @arg 1:正确
  *         @arg 0:错误
  */
int do_XYCNNT_process(char **prsp_cmd,int test_cmd)
{
	int ret = 1;
	*prsp_cmd = xy_zalloc(4 * XYCNNT_TEST_SUM + 60);

	if(test_cmd == 0)
		sprintf(*prsp_cmd+strlen(*prsp_cmd),"\r\n+XYCNNT:bitmap=0x%x\r\n",g_XYCNNT_bitmap);
	else if(test_cmd == 1)
		sprintf(*prsp_cmd+strlen(*prsp_cmd),"\r\n+XYCNNT:bitmap=(0-0x3fff)\r\n");

	if (g_XYCNNT_bitmap == 0)
	{
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+XYCNNT:FAIL\r\n\r\nOK\r\n");
		return 0;
	}

	sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+XYCNNT:");

	for (int i = 0; i < XYCNNT_TEST_SUM; i++)
	{
		if (g_XYCNNT_bitmap & (1 << i))
		{
			if (i % 2 == 0)
			{
				if (check_connectivity(g_PIN_ID[i + 1], g_PIN_ID[i], 1))
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%x%c", i, 'Y');
				else
				{
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%x%c", i, 'N');
					ret = 0;
				}

				if (check_connectivity(g_PIN_ID[i + 1], g_PIN_ID[i], 0))
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%c,", 'Y');
				else
				{
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%c,", 'N');
					ret = 0;
				}
			}
			else
			{
				if (check_connectivity(g_PIN_ID[i - 1], g_PIN_ID[i], 1))
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%x%c", i, 'Y');
				else
				{
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%x%c", i, 'N');
					ret = 0;
				}

				if (check_connectivity(g_PIN_ID[i - 1], g_PIN_ID[i], 0))
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%c,", 'Y');
				else
				{
					sprintf(*prsp_cmd + strlen(*prsp_cmd), "%c,", 'N');
					ret = 0;
				}
			}
		}
	}

	short cal_vol = get_ADC_val(HAL_ADC_MODE_TYPE_DOUBLE_PN);

	
	if (cal_vol > 600 || cal_vol < 400)
	{
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "%x%c%c,", XYCNNT_TEST_SUM, 'N', 'N');
		ret = 0;
	}
	else
	{
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "%x%c%c,", XYCNNT_TEST_SUM, 'Y', 'Y');
	}

	if (ret == 1)
	{
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "SUCCESS\r\n\r\nOK\r\n");
		return 1;
	}
	else
	{
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "FAIL\r\n\r\nOK\r\n");
		return 0;
	}
}

/**
  * @brief  引脚测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+XYCNNT=<bit map>
  * @arg 查询类AT命令：AT+XYCNNT?
  * @arg 测试类AT命令：AT+XYCNNT=? 
  */
int at_XYCNNT_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		void *p[] = {&g_XYCNNT_bitmap};

		if (at_parse_param_2("%d", at_buf, p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		do_XYCNNT_process(prsp_cmd,0);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		do_XYCNNT_process(prsp_cmd,1);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}



int  g_ADC_Mode = 0;  //0,双端差分；1，单端P；2，单端N；3，单端P和N
int do_ZADC_REQ(int adc_Mode,char **prsp_cmd)
{
	*prsp_cmd = xy_zalloc(40);
	if(otp_valid_flag != 1)  //缺乏AD校验信息，则报错
	{
//		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+ZADC:%d\r\n", -1);
		return AT_END;
	}
	if(adc_Mode == 0)
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+ZADC:%d\r\n", get_ADC_val(HAL_ADC_MODE_TYPE_DOUBLE_PN));
	else if(adc_Mode == 1)
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+ZADC:%d\r\n", get_ADC_val(HAL_ADC_MODE_TYPE_SINGLE_P));
	else if(adc_Mode == 2)
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+ZADC:%d\r\n", get_ADC_val(HAL_ADC_MODE_TYPE_SINGLE_N));
	else if(adc_Mode == 3)
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+ZADC:%d,%d\r\n", get_ADC_val(HAL_ADC_MODE_TYPE_SINGLE_P),get_ADC_val(HAL_ADC_MODE_TYPE_SINGLE_N));


	sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
	return AT_END;
}



int do_ZADC_TEST(char **prsp_cmd)
{
	*prsp_cmd = xy_zalloc(30);

	sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+ZADC:(0,1000)\r\n");
	sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
	return 1;
}

/**
  * @brief  ADC测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 查询类AT命令：AT+ZADC?
  * @arg 测试类AT命令：AT+ZADC=?
  * @arg 设置类AT命令：AT+ZADC=mode
    */
int at_ZADC_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;

	if (g_req_type == AT_CMD_QUERY)
	{
		do_ZADC_REQ(g_ADC_Mode,prsp_cmd);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		do_ZADC_TEST(prsp_cmd);
	}
	else if(g_req_type==AT_CMD_REQ)
	{		
		int  adc_Mode = g_ADC_Mode;

		if(at_parse_param("%d", at_buf, &adc_Mode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}	

		do_ZADC_REQ(adc_Mode,prsp_cmd);
	}
	return AT_END;
}

/**
  * @brief  VBAT电压测试AT命令处理函数，显示结果为VBAT电压（单位mV）
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 测试AT命令：AT+VBAT=?  
  */
int at_VBAT_req(char *at_buf, char **prsp_cmd)
{
	unsigned int voltage;

	(void) at_buf;

	if (g_req_type == AT_CMD_TEST)
	{
		voltage = xy_getVbat();

		*prsp_cmd = xy_zalloc(32);

		sprintf(*prsp_cmd  + strlen(*prsp_cmd ), "\r\n+VBAT:%d\r\n", voltage);
		sprintf(*prsp_cmd  + strlen(*prsp_cmd ), "\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}


/*******************************************************************************
 *					以下AT命令已在系统级AT命令注册表中注册（at_cmd.c）					   *
 ******************************************************************************/

/**
  * @brief  AT串口配置AT命令
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+NATSPEED=<baud_rate>[,<timeout>[,<store>[,<sync_mode>[,<stopbits>]]]]
  *      @arg <baud_rate>,可配置波特率（必选）：2400,4800,9600,57600,115200,230400,460800
  *      @arg <timeout>,超时时间,不支持
  *      @arg <store>,是否存储在NV中，1存储，0不存储。默认为1存储。
  *      @arg <sync_mode>,同步模式,不支持
  *      @arg <stopbits>,停止位（可选）:1,2,默认1
  * @arg 查询类AT命令：AT+NATSPEED?
  * @arg 测试类AT命令：AT+NATSPEED=?
  */

#if VER_QUECTEL
//标识当前是否正在进行波特率切换
uint8_t g_natspeed_flag=0;

static int32_t store =0;
int32_t baud_rate_back = 0;
static int32_t nat_speed_timeout = 0;
osTimerId_t natspeed_timeout_timer = NULL;



//切换超时，回退到原波特率
void natspeed_timeout_callback()
{
	CSPClockSet(AT_UART_CSP, CSP_MODE_UART, XY_PCLK, baud_rate_back);

	g_current_baudrate = baud_rate_back;
	HWREG(AT_UART_BAUD) = baud_rate_back;

	osTimerDelete(natspeed_timeout_timer);
	natspeed_timeout_timer = NULL;
}


//AT+NATSPEED回复OK后，开始尝试切换到新波特率
void natspeed_change_hook()
{
	if(g_natspeed_flag == 1)
	{
		while((HWREG(AT_UART_CSP+CSP_INT_STATUS) & CSP_INT_CSP_TX_ALLOUT) == 0);
		CSPClockSet(AT_UART_CSP, CSP_MODE_UART, XY_PCLK, g_current_baudrate);
		g_natspeed_flag = 0;

		osTimerAttr_t timer_attr = {0};

		timer_attr.name = "natspeed_delay";
		natspeed_timeout_timer = osTimerNew((osTimerFunc_t)natspeed_timeout_callback, osTimerOnce, NULL, &timer_attr);
        osTimerStart(natspeed_timeout_timer, nat_speed_timeout * 1000);
	}
}

//在切换波特率后收到外部MCU一条正确的AT命令后，杀超时定时器
void natspeed_succ_hook()
{
	if(natspeed_timeout_timer != NULL)
	{
		osTimerStop(natspeed_timeout_timer);
		osTimerDelete(natspeed_timeout_timer);
		natspeed_timeout_timer = NULL;

		if(store == 1)
		{
			g_softap_fac_nv->uart_rate = g_current_baudrate/2400;
			SAVE_FAC_PARAM(uart_rate);
		}
	}
}



int at_NATSPEED_req(char *at_buf, char **prsp_cmd)
{
	int   default_timeout = 3;
	int   sync_mode = 0;
	int   stopbits = 1;
	int   parity = 0;
	int   xonxoff = 0;
	int   baud_rate = 0;

	if(g_softap_fac_nv->close_at_uart == 1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	if(g_req_type==AT_CMD_REQ)
	{

		if(at_parse_param("%d, %d, %d, %d, %d, %d, %d", at_buf, &baud_rate, &nat_speed_timeout, &store, &sync_mode, &stopbits, &parity, &xonxoff) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		int valid_baud_flag = 0;
		int valid_baud[8] = {2400, 4800, 9600, 57600, 115200, 230400, 460800,921600};
		for(int i = 0; i < 8; i++)
		{
			if(baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}


		
		if ((valid_baud_flag == 0) || \
		   (nat_speed_timeout > 30) || \
		   (store != 0 && store != 1) || \
		   (sync_mode != 0) || \
		   (stopbits != 1 && stopbits != 2) ||\
		   (parity != 0 ) || \
		   (xonxoff != 0) )
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		if(nat_speed_timeout == 0)
		{
			nat_speed_timeout = default_timeout;
		}
		g_natspeed_flag = 1;
		if(baud_rate != 0)
		{

			CSPTxFrameSet(AT_UART_CSP, 1, 8, 8 + 1, 8 + 1 + stopbits);
			CSPRxFrameSet(AT_UART_CSP, 1, 8, 8 + 1 + stopbits);


			baud_rate_back = g_current_baudrate;
			g_current_baudrate = baud_rate;
			HWREG(AT_UART_BAUD) = baud_rate;
		}
		if(store == 1)
		{			
//			if(baud_rate > 9600)
//			{
//				g_softap_fac_nv->lpm_standby_enable = 0;
//				SAVE_FAC_PARAM(lpm_standby_enable);
//			}

//			if(baud_rate > 57600)
//			{
//				g_softap_fac_nv->deepsleep_enable = 0;
//				SAVE_FAC_PARAM(deepsleep_enable);
//			}
		}
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(50);
		sprintf(*prsp_cmd,"\r\n+NATSPEED:%d, %d, %d, %d, %d\r\n\r\nOK\r\n",g_current_baudrate, 0, stopbits, 0, 0);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(200);
		sprintf(*prsp_cmd,"\r\n+NATSPEED:(2400,4800,9600,57600,115200,230400,460800,921600),(0-30),(0,1),(0),(1,2),(0),(0)\r\n\r\nOK\r\n");
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}

	return AT_END;
}


#else
int at_NATSPEED_req(char *at_buf, char **prsp_cmd)
{
	if(g_softap_fac_nv->close_at_uart == 1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	if(g_req_type==AT_CMD_REQ)
	{
		int   timeout=0;
		int   store=1;
		int   sync_mode=0;
		int   stopbits=1;
		int   baud_rate = -1;
		void *p[] = {&baud_rate, &timeout,&store,&sync_mode,&stopbits};
		
		if(at_parse_param_2("%d,%d,%d,%d,%d", at_buf, p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		int valid_baud_flag = 0;
		int valid_baud[8] = {2400, 4800, 9600, 57600, 115200, 230400, 460800,921600};
		for(int i = 0; i < 8; i++)
		{
			if(baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}

		if ((valid_baud_flag == 0) || \
		   (timeout != 0) || \
		   (store != 0 && store != 1) || \
		   (sync_mode != 0) || \
		   (stopbits != 1 && stopbits != 2))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		if(store == 1)
		{			
			g_softap_fac_nv->uart_rate = baud_rate/2400;
			SAVE_FAC_PARAM(uart_rate);
			send_urc_to_ext("\r\nREBOOTING\r\n");
			//see at_RB_req
			reboot_nv();

			HWREG(PS_START_SYNC) = 0;

			soft_reset_by_wdt(RB_BY_NRB);
		}
		//effect  immediately
		else
		{
			if(baud_rate != 0)
			{
				CSPClockSet(AT_UART_CSP, CSP_MODE_UART, XY_PCLK, baud_rate);
				CSPTxFrameSet(AT_UART_CSP, 1, 8, 8 + 1, 8 + 1 + stopbits);
				CSPRxFrameSet(AT_UART_CSP, 1, 8, 8 + 1 + stopbits);

				g_current_baudrate = baud_rate;
				HWREG(AT_UART_BAUD) = g_current_baudrate;
			}
		}
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(30);
		sprintf(*prsp_cmd,"\r\n+NATSPEED:%d\r\n\r\nOK\r\n",(g_softap_fac_nv->uart_rate & 0x1ff)*2400);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(75);
		sprintf(*prsp_cmd,"\r\n+NATSPEED:(2400,4800,9600,57600,115200,230400,460800,921600)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}
#endif
/**
  * @brief  wakeup引脚控制选择
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+RESETCTL=<mode>
  *      @arg <mode>:0，表示按键小于20ms唤醒，超过20ms复位，默认值
  *      @arg <mode>:1，表示按键小于6s唤醒，超过6s复位
  * @arg 查询类AT命令：AT+RESETCTL?
  */
  //yus
int at_RESETCTL_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ || g_req_type == AT_CMD_ACTIVE)
	{
		int temp = -1;
		void *p[] = {&temp};
		if (at_parse_param_2("%d", at_buf, p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		if (temp != 0 && temp != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		g_softap_fac_nv->resetctl = temp;
		SAVE_FAC_PARAM(resetctl);
		if (g_softap_fac_nv->resetctl == 0)
		{
			if (HWREG(0xA0110010) == 0x00010000) // B0p1, old chip
			{
				HWREGB(0xA005800E) |= 0x08;
			}
			else // B0p2, new chip
			{
				HWREGB(0xA005800E) &= 0xF7;
			}
		}
		else
		{
			if (HWREG(0xA0110010) == 0x00010000) // B0p1, old chip
			{

				HWREGB(0xA005800E) &= 0xF7;
			}
			else // B0p2, new chip
			{
				HWREGB(0xA005800E) |= 0x08;
			}
		}
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd, "\r\n+RESETCTL:%d\r\n\r\nOK\r\n", g_softap_fac_nv->resetctl);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		
	return AT_END;
}

/**
  * @brief  AT串口配置(波特率（必选），是否存储在NV中（可选），是否打开standby（可选）)
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+UARTSET=<baud_rate>[,<store>[,<open standby>]]
  *      @arg <baud_rate>,可配置波特率（必选）：2400,4800,9600,57600,115200,230400,460800
  *      @arg <store>,是否存储在NV中（可选），1为是，0为否
  *      @arg <open standby>,是否打开standby，1为是，0为否，不建议客户使用该参数
  * @arg 查询类AT命令：AT+UARTSET?
  * @arg 测试类AT命令：AT+UARTSET=?  
  */
int at_UARTSET_req(char *at_buf, char **prsp_cmd)
{
	if(g_softap_fac_nv->close_at_uart == 1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	if (g_req_type == AT_CMD_REQ)
	{
		int store = 0;
		int open_standby = -1;
		int baud_rate = -1;
		void *p[] = {&baud_rate, &store, &open_standby};


		if (at_parse_param_2("%d,%d,%d", at_buf, p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		int valid_baud_flag = 0;
		int valid_baud[8] = {2400, 4800, 9600, 57600, 115200, 230400, 460800,921600};
		for(int i = 0; i < 8; i++)
		{
			if(baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}

		if ((valid_baud_flag == 0) || \
			(store != 0 && store != 1) || \
			(open_standby != 0 && open_standby != 1 && open_standby != -1))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if (store == 1)
		{
			g_softap_fac_nv->uart_rate = baud_rate / 2400;
			SAVE_FAC_PARAM(uart_rate);

			send_urc_to_ext("\r\nREBOOTING\r\n");
			reboot_nv();

			HWREG(PS_START_SYNC) = 0;

			soft_reset_by_wdt(RB_BY_NRB);
		}
		else
		{
			//不建议客户使用该参数
			if (open_standby == 0)
				xy_standby_close();
			else if (open_standby == 1)
				xy_standby_open();
			//dynamic vary standby,set open if rate<=19200;else set close for big data transfer
			else if (open_standby == -1)
			{
				if (baud_rate / 9600 >= 2)
					xy_standby_close();
				else
					xy_standby_open();
			}

			if(baud_rate != 0)
			{
				CSPClockSet(AT_UART_CSP, CSP_MODE_UART, XY_PCLK, baud_rate);
				CSPTxFrameSet(AT_UART_CSP, 1, 8, 8 + 1, 8 + 1 + 1);
				CSPRxFrameSet(AT_UART_CSP, 1, 8, 8 + 1 + 1);

				g_current_baudrate = baud_rate;
				HWREG(AT_UART_BAUD) = g_current_baudrate;
			}
		}
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(30);
		sprintf(*prsp_cmd, "\r\n+UARTSET:%d\r\n\r\nOK\r\n", (g_softap_fac_nv->uart_rate & 0x1ff) * 2400);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(75);
		sprintf(*prsp_cmd, "\r\n+UARTSET:(2400,4800,9600,57600,115200,230400,460800,921600)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}

/**
  * @brief  AT串口配置(波特率)
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+IPR=<baud_rate>
  *      @arg <baud_rate>,可配置波特率：0,4800,9600,19200,38400,57600,115200,230400,460800,921600;0表示启用波特率自适应
  *		 若设置的波特率高于9600，则同时修改NV关闭standby
  * @arg 查询类AT命令：AT+IPR?
  * @arg 测试类AT命令：AT+IPR=?
  */
//标识当前是否正在进行波特率切换
uint8_t g_ipr_flag=0;
//AT+IPR回复OK后，开始尝试切换到新波特率
void ipr_change_hook()
{
	if(g_ipr_flag == 1)
	{
		while((HWREG(AT_UART_CSP+CSP_INT_STATUS) & CSP_INT_CSP_TX_ALLOUT) == 0);
		CSPClockSet(AT_UART_CSP, CSP_MODE_UART, XY_PCLK, g_current_baudrate);
		CSPTxFrameSet(AT_UART_CSP, 1, 8, 8 + 1, 8 + 1 + 1);
		CSPRxFrameSet(AT_UART_CSP, 1, 8, 8 + 1 + 1);

		HWREG(AT_UART_BAUD) = g_current_baudrate;
		g_ipr_flag = 0;

		// osTimerAttr_t timer_attr = {0};

		// timer_attr.name = "natspeed_delay";
		// natspeed_timeout_timer = osTimerNew((osTimerFunc_t)natspeed_timeout_callback, osTimerOnce, NULL, &timer_attr);
        // osTimerStart(natspeed_timeout_timer, nat_speed_timeout * 1000);
	}
}

int at_IPR_req(char *at_buf, char **prsp_cmd)
{
	if(g_softap_fac_nv->close_at_uart == 1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	if(g_req_type==AT_CMD_REQ /*|| g_req_type==AT_CMD_ACTIVE*/)
	{
		int   baud_rate = 9600;
		void *p[] = {&baud_rate};
		
		if(at_parse_param_2("%d", at_buf, p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		int valid_baud_flag = 0;
		int valid_baud[10] = {0, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};
		for(int i = 0; i < 10; i++)
		{
			if(baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}

		if(valid_baud_flag == 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
			
		g_softap_fac_nv->uart_rate = baud_rate/2400;
		
		if(baud_rate != 0)
		{
			// CSPClockSet(AT_UART_CSP, CSP_MODE_UART, XY_PCLK, baud_rate);
			// CSPTxFrameSet(AT_UART_CSP, 1, 8, 8 + 1, 8 + 1 + 1);
			// CSPRxFrameSet(AT_UART_CSP, 1, 8, 8 + 1 + 1);
			g_ipr_flag = 1;
			g_current_baudrate = baud_rate;
			// HWREG(AT_UART_BAUD) = g_current_baudrate;
		}
		if(baud_rate > 9600)
		{
			g_softap_fac_nv->lpm_standby_enable = 0;
		}
		
		SAVE_FAC_PARAM(uart_rate);		
		SAVE_FAC_PARAM(lpm_standby_enable);

	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(36);
		sprintf(*prsp_cmd,"\r\n+IPR: %d\r\n\r\nOK\r\n",(g_softap_fac_nv->uart_rate & 0x1ff) *2400);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(146);
		sprintf(*prsp_cmd,"\r\n+IPR: (2400,4800,9600,19200,38400,57600,115200,230400),(0,2400,4800,9600,19200,38400,57600,115200,230400,460800)\r\n\r\nOK\r\n");
	}
	else if( g_req_type==AT_CMD_ACTIVE)
	{		
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
	return AT_END;
}

/**
 * @brief 看门狗喂狗
 * 
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 * @note 此指令已废弃，不建议客户使用
 */
int at_WTD_req(char *at_buf, char **prsp_cmd)
{
	int sec = 0, wtd_mode = 0;

	if(at_parse_param("%d, %d", at_buf, &sec, &wtd_mode) != AT_OK)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	if(sec < 1 || sec > 131071)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	xy_watchdog_refresh(sec);
	return AT_END;
}


#define MCU_RI_OUT_PIN  HAL_GPIO_PIN_NUM_13

static uint16_t adc_conver(HAL_ADC_HandleTypeDef *ADC_HandleStruct)
{
	int32_t real_val;
	int32_t convert_val;
	int32_t cal_vol;
	int16_t average = 0;

   if (ADC_HandleStruct == NULL)
	{
		return HAL_ERROR;
	}

	for(uint8_t i = 0; i < 10; i++)
	{
		real_val = HAL_ADC_GetValue(ADC_HandleStruct);
		average += real_val;
	}

	if(average >= 0)
		real_val = (average + 5) / 10;
	else
		real_val = (average - 5) / 10;

	convert_val = HAL_ADC_GetConverteValue(ADC_HandleStruct, real_val);

	cal_vol     = HAL_ADC_GetVoltageValueWithCal(ADC_HandleStruct,convert_val);

	return cal_vol;
}

HAL_ADC_HandleTypeDef ADC_HandleStructVBAT;
static void adc_vbat_init(void)
{
	ADC_HandleStructVBAT.Init.Mode = HAL_ADC_MODE_TYPE_VBAT;
	ADC_HandleStructVBAT.Init.Speed = HAL_ADC_SPEED_TYPE_240K;

	HAL_ADC_Init(&ADC_HandleStructVBAT);
}

#define ADC_SAMPLE_NUM 4

//#ifdef UNUSED_FUNCTION
void Tempera_Delay(uint32_t uldelay)
{
    volatile uint32_t i;

    for(i = 0; i < uldelay; i++)
    {
    }
}
//#endif

void TSensor_Poweron(void)
{
	osCoreEnterCritical();
	WAIT_DSP_ADC_IDLE();//while(HWREGB(0xA0058015) & 0x80 );			// CP Use ADC

	SET_ARM_ADC_BUSY();//HWREGB(0xA0058015) |= 0x40;					// AP Use ADC
	for(volatile uint8_t i = 0; i < 200; i++);	//delay
	WAIT_DSP_ADC_IDLE();//while(HWREGB(0xA0058015) & 0x80 );			//再次判断 CP Use ADC
	osCoreExitCritical();


	HWREGB(0xA0110048) |= 0x45;    //Tsensor
	HWREGB(0xA011004B) 	= 0x01;	   //ADC内部延时,始终为0x01
	HWREGB(0xA0110049) &= 0xFC;	   //bit0:1 to disable p side of auxadc,and auxadc enter sigle-ended mode
	HWREGB(0xA0058024) |= 0x03;
	HWREGB(0xA0110046) 	= 0x3F;		//240K采样
	HWREGB(0xA0110050) 	= 0x0;		//

	HWREGB(0xA011004A) 	= 0x02;		//tsensor (channel2)
}

uint16_t Tsensor_GetData(void)
{
	while((HWD_REG_READ08(0xA0110045) & 0x80)== 0)
	{
	}

	return (HWD_REG_READ16(0xA0110044)&0x0FFF);
}

void Tsensor_Poweroff(void)
{
	//osCoreEnterCritical();
	//while(HWREGB(0xA0058015) & 0x80 );			// CP Use ADC

	//for(volatile uint8_t i = 0; i < 10; i++);	//delay
	//while(HWREGB(0xA0058015) & 0x80 );			//再次判断 CP Use ADC
	//osCoreExitCritical();

	HWREGB(0xA0110048) 	&= 0xFA;

	osCoreEnterCritical();
	SET_ARM_ADC_IDLE();//HWREGB(0xA0058015) &= 0xBF;		// Clear AP Use ADC
	osCoreExitCritical();
}

int Tempera_Update()
{
	int tempera = 0;

	uint16_t tsensor_zero;
	uint8_t  tempcnt = 0;

	int32_t adcValMax = -2047;
	int32_t adcValMin = 2047;
	int32_t tmpAdcVal[8] = {0};
	uint32_t tempADCVal = 0;

	tsensor_zero = HWREGH(OTPINFO_RAM_ADDR+0X30);

	volatile uint32_t delay;

	// osCoreEnterCritical();
	// while(HWREGB(0xA0058015) & 0x80 );			// CP Use ADC

	// for(volatile uint8_t i = 0; i < 10; i++);	//delay
	// while(HWREGB(0xA0058015) & 0x80 );			//再次判断 CP Use ADC
	// osCoreExitCritical();


	tempADCVal = Tsensor_GetData();

	//delay = 500;
	//while(delay--);

	for(tempcnt = 0; tempcnt < 4; tempcnt++)
	{
		tempADCVal = (HWD_REG_READ16(0xA0110044)&0x0FFF);

		if(tempADCVal != 0)
		{
			adcValMax = tempADCVal;
			adcValMin = tempADCVal;
			tmpAdcVal[0] = tempADCVal;
			break;
		}

		delay = 250;
		while(delay--);
	}

	for(tempcnt = 1; tempcnt < ADC_SAMPLE_NUM; tempcnt++)
	{
		delay = 50;
		while(delay--);
		tmpAdcVal[tempcnt] = (HWD_REG_READ16(0xA0110044)&0x0FFF);
		if (tmpAdcVal[tempcnt] >= adcValMax)
		{
			adcValMax = tmpAdcVal[tempcnt];
			
		}
		if (tmpAdcVal[tempcnt] <= adcValMin)
		{
			adcValMin = tmpAdcVal[tempcnt];
			
		}
		tempADCVal += tmpAdcVal[tempcnt];
	}

	Tsensor_Poweroff();

	tempADCVal -=  (adcValMax + adcValMin);

	if(tempADCVal >= (tsensor_zero * (ADC_SAMPLE_NUM - 2)))
	{
		tempera = (tempADCVal - (tsensor_zero * (ADC_SAMPLE_NUM - 2))) * 155 / (100*(ADC_SAMPLE_NUM - 2));
	}
	else
	{
		tempera = 0 - (((tsensor_zero * (ADC_SAMPLE_NUM - 2)) - tempADCVal) * 155 / (100*(ADC_SAMPLE_NUM - 2)));
	}

	return tempera;
}

int32_t TSensor_GetValue(void)
{
	TSensor_Poweron();
	Tempera_Delay(50);
	HWREGB(0xA0110048) |= 0x45;

    return Tempera_Update();
}
int at_QCHIPINFO_req(char *at_buf,char **prsp_cmd)
{
	int Ch_cal_vol = 0;
	int temp=0;
	char *cmd = NULL;

	if(g_req_type==AT_CMD_REQ)
	{
		cmd = xy_zalloc(strlen(at_buf));
		if(at_parse_param("%s", at_buf, cmd) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			xy_free(cmd);
			return AT_END;
		}

		if(at_strcasecmp(cmd, "ALL") == 0)//VBAT
		{
			adc_vbat_init();
			Ch_cal_vol = adc_conver(&ADC_HandleStructVBAT);
			HAL_ADC_DeInit(&ADC_HandleStructVBAT);

			temp=TSensor_GetValue();

			*prsp_cmd = xy_zalloc(100);
			sprintf(*prsp_cmd ,"\r\n+QCHIPINFO:%s,%d", "TEMP", temp);
			sprintf(*prsp_cmd + strlen(*prsp_cmd),"\r\n+QCHIPINFO:%s,%d\r\n\r\nOK\r\n", "VBAT", Ch_cal_vol);

		}

		else if(at_strcasecmp(cmd, "TEMP") == 0)//TEMP
		{
			temp=TSensor_GetValue();
			*prsp_cmd = xy_zalloc(50);
			sprintf(*prsp_cmd,"\r\n+QCHIPINFO:%s,%d\r\n\r\nOK\r\n", "TEMP", temp);
		}

		else if(at_strcasecmp(cmd, "VBAT") == 0)//VBAT
		{
			adc_vbat_init();
			Ch_cal_vol = adc_conver(&ADC_HandleStructVBAT);
			HAL_ADC_DeInit(&ADC_HandleStructVBAT);

			*prsp_cmd = xy_zalloc(50);
			sprintf(*prsp_cmd,"\r\n+QCHIPINFO:%s,%d\r\n\r\nOK\r\n", "VBAT", Ch_cal_vol);
		}

		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		xy_free(cmd);
	}

	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(50);
		sprintf(*prsp_cmd,"\r\n+QCHIPINFO:(%s,%s,%s)\r\n\r\nOK\r\n","ALL","TEMP","VBAT");
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
	return AT_END;
}

#if  WAKEUP_MCU_BY_URC
void wakeup_MCU_by_URC()
{
	if(g_ring_enable == 1)
	{
		HAL_GPIO_InitTypeDef MCU_RI_gpio;

		MCU_RI_gpio.Pin = MCU_RI_OUT_PIN;
		MCU_RI_gpio.Mode = GPIO_MODE_OUTPUT_PP;
		MCU_RI_gpio.Pull = GPIO_NOPULL;

		//softap_printf(USER_LOG, WARN_LOG, "Ring Indication");

		HAL_GPIO_Init(&MCU_RI_gpio);
		HAL_GPIO_WritePin(MCU_RI_OUT_PIN,HAL_GPIO_PIN_SET);
		osDelay(g_duration);

		HAL_GPIO_Init(&MCU_RI_gpio);
		HAL_GPIO_WritePin(MCU_RI_OUT_PIN,HAL_GPIO_PIN_RESET);
	}
}


//+CMSRI=<ring_en>,<duration>[,<permanent>]
int at_CMSRI_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int ring_en = -1;
		int duration = 120;
		int permanent = 0;

		void *p[] = {&ring_en, &duration, &permanent};
		if (at_parse_param_2("%d,%d,%d", at_buf,p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if((ring_en != 0 && ring_en != 1) || (permanent != 0 && permanent != 1) || (duration <= 0))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		g_ring_enable = ring_en;
		g_duration = duration;
		
		if(permanent == 1)
		{
			g_softap_fac_nv->ring_enable = g_ring_enable;
			SAVE_FAC_PARAM(ring_enable);	
		}			
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+CMSRI:%d,%d\r\n\r\nOK\r\n", g_ring_enable, g_duration);
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}
#endif

/**
  * @brief  查询模块的供电电压值，显示结果为VBAT接口的电压值（单位mV）
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 测试类AT命令：AT+CBC=?  
  * @arg 执行类AT命令：AT+CBC  
  */
int at_CBC_req(char *at_buf, char **prsp_cmd)
{
	unsigned int voltage;

	(void) at_buf;

	if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd , "\r\nOK\r\n");
	}
	else if (g_req_type == AT_CMD_ACTIVE)
	{
		voltage = xy_getVbat();
		*prsp_cmd = xy_zalloc(32);

		sprintf(*prsp_cmd  + strlen(*prsp_cmd ), "\r\n+CBC:%d\r\n", voltage);
		sprintf(*prsp_cmd  + strlen(*prsp_cmd ), "\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}


int do_QADC_REQ(int adc_Mode,char **prsp_cmd)
{
	*prsp_cmd = xy_zalloc(40);
	if(otp_valid_flag != 1)  //缺乏AD校验信息，则报错
	{
//		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+QADC:%d\r\n", -1);
		return AT_END;
	}
	if(adc_Mode == 0)
	{
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+QADC:%d,%d\r\n", 0,get_ADC_val(HAL_ADC_MODE_TYPE_DOUBLE_PN));
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
	return AT_END;
}


/**
  * @brief  ADC测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 执行类AT命令：AT+QADC  +QADC: 0,<voltage>
  * @arg 测试类AT命令：AT+QADC=? +QADC: 支持的channel列表
  * @arg 设置类AT命令：AT+QADC=<channel>    +QADC: <channel>,<voltage>
    */
int at_QADC_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;

	if (g_req_type == AT_CMD_ACTIVE)
	{
		do_QADC_REQ(0,prsp_cmd);
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(32);
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\n+QADC:(%d)\r\n",0 );
		sprintf(*prsp_cmd + strlen(*prsp_cmd), "\r\nOK\r\n");
	}
	else if(g_req_type==AT_CMD_REQ)
	{		
		int  adc_Mode = 0;

		if(at_parse_param("%d(0-0)", at_buf, &adc_Mode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}	

		do_QADC_REQ(adc_Mode,prsp_cmd);
	}
	return AT_END;
}

