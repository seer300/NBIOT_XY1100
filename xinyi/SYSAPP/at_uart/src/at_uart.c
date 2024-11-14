#include "at_uart.h"
#include "oss_nv.h"
#include "xy_system.h"
#include "hw_gpio.h"
#include "hw_csp.h"
#include "replace.h"
#include "hw_memmap.h"
#include "csp.h"
#include "at_global_def.h"
#include "sys_init.h"
#include "gpio.h"
#include "at_ctl.h"
#include "xy_watchdog.h"

typedef enum
{
	AUTOBAUD_STATE_NONE,
	AUTOBAUD_STATE_SKIP,	
	AUTOBAUD_STATE_INIT,
	AUTOBAUD_STATE_WAITINT_FIRST,
	AUTOBAUD_STATE_WAITINT_SECOND,
	AUTOBAUD_STATE_WAITINT_THIRD,
	AUTOBAUD_STATE_VERIFY,
	AUTOBAUD_STATE_DONE,

}AUTOBAUD_STATE;

#define AUTOBAUD_SUPPORT_NUM  8

unsigned int g_autobaud_table[AUTOBAUD_SUPPORT_NUM] = {2400,4800,9600,19200,38400,57600,115200,230400};

unsigned int g_autobaud_state = AUTOBAUD_STATE_NONE;
volatile unsigned int g_autobaud_tick[3] = {0};
unsigned char g_autobaud_stop_standby = 0;
unsigned int g_autobaud_rate = 0;
volatile uint8_t csp_out_flag = 0;
osSemaphoreId_t g_at_uart_sem = NULL;

int at_write_to_uart(char *buf,int size)
{
	 int i;
	 //bug5325
	if(size <= 0 || g_autobaud_state == 0 || ((HWREG(AT_UART_CSP+CSP_MODE1) & 0x20) == 0) || g_softap_fac_nv->close_at_uart == 1)
		return -1;

	csp_out_flag |= 0x01;

	HWREG(AT_UART_CSP+CSP_INT_STATUS) = CSP_INT_CSP_TX_ALLOUT;
	 
	for(i = 0 ; i < size ; i++ )
	{
		CSPCharPut( AT_UART_CSP, *((char *)buf + i));
	}
	return 0;
}

// return: 
// 0: csp write allout not complete
// 1: csp write allout complete
uint32_t csp_write_allout_state()
{
	// AT CSP all out
	if((csp_out_flag & 0x01) && ( ( HWREG(AT_UART_CSP + CSP_INT_STATUS) & CSP_INT_CSP_TX_ALLOUT) == 0 ) && (g_softap_fac_nv->close_at_uart == 0))
		return 0;

	csp_out_flag = 0;
	return 1;
}

void wait_for_csp_tx_allout(void)
{
	volatile unsigned int delaycnt = 0;
	//special process,bug4939

	while(( HWREG(AT_UART_CSP + CSP_TX_FIFO_STATUS) & CSP_TX_FIFO_STATUS_EMPTY_Msk) == 0);
	for(delaycnt = 0;delaycnt<100000;delaycnt++);//for baudrate 2400,tx shifter,4ms
}

void xy_tick_delay_for_ATend(void)
{
	// tick_delay would not larger than 1ms,OS_SYS_CLOCK/1000
	uint32_t val_cur,val_target,tick_delay;

	osCoreEnterCritical();

	tick_delay = 2 * 10 * OS_SYS_CLOCK / g_current_baudrate;	//2 bytes,10 bits

	val_cur = SysTick->VAL;

	val_target = (val_cur + OS_SYS_CLOCK/(OS_TICK_RATE_HZ) - tick_delay ) % (OS_SYS_CLOCK/(OS_TICK_RATE_HZ));

	if(val_target > val_cur)
	{
		while( !(SysTick->VAL <= val_target && SysTick->VAL > val_cur) );
	}
	else if(val_target < val_cur )
	{
		while( !(SysTick->VAL <= val_target || SysTick->VAL > val_cur) );
	}
	else
	{
		//impossible actually
		xy_assert(0);
	}

	osCoreExitCritical();
}

void at_uart_init()
{

	g_at_uart_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	if(g_softap_fac_nv->close_at_uart==0 && ((g_softap_fac_nv->peri_remap_enable & 0x2) != 0))
	{
		PRCM_Clock_Enable(PRCM_BASE,AT_UART_CSP_PRCM);

		HWREG(AT_UART_CSP + CSP_RX_FIFO_CTRL) = ((HWREG(AT_UART_CSP + CSP_RX_FIFO_CTRL) & 0x03)|((32-1)<<2));

		HWREG(AT_UART_CSP + CSP_AYSNC_PARAM_REG) = ((HWREG(AT_UART_CSP + CSP_AYSNC_PARAM_REG) & 0xffff0000) | 0x0a);

		CSPIntClear(AT_UART_CSP,CSP_INT_ALL);

		HWREG(AT_UART_CSP + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_RESET;

		// Fifo Start
		HWREG(AT_UART_CSP + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_START;
		
		CSPIntEnable(AT_UART_CSP,CSP_INT_RX_DONE);
		
	
		NVIC_IntRegister(CSP2_IRQn, at_uart_int_handler, 1);
		
	}
	else
	{
		ASSERT_PRINT_CLOSE();
	}
}


int at_uart_read( char *buf, unsigned long len)
{	
	long cspAtCdata = 0;
    
    unsigned long read_pos = 0;

	if (osOK == osSemaphoreAcquire(g_at_uart_sem, osWaitForever))
	{
		while(read_pos<len)
		{
			cspAtCdata = CSPCharGetNonBlocking(AT_UART_CSP);
			if(cspAtCdata == -1)
				break;
			*(buf + read_pos) = (char)cspAtCdata;
			read_pos++;
		}
		return read_pos;
	
	}

	return -1;
}

void at_uart_int_handler(void)
{
	if((CSPIntStatus(CSP2_BASE) & CSP_INT_RX_DONE) && (HWREG(CSP2_BASE + CSP_INT_ENABLE) & CSP_INT_RX_DONE))
	{
	    CSPIntDisable(CSP2_BASE,CSP_INT_RX_DONE);

	    CSPIntClear(CSP2_BASE,CSP_INT_RXFIFO_THD_REACH|CSP_INT_CSP_RX_TIMEOUT|CSP_INT_RX_DONE);

	 	CSPIntEnable(CSP2_BASE,CSP_INT_RXFIFO_THD_REACH|CSP_INT_CSP_RX_TIMEOUT);

	 	return;
	}

	if((CSPIntStatus(CSP2_BASE) & CSP_INT_CSP_RX_TIMEOUT) && (HWREG(CSP2_BASE + CSP_INT_ENABLE) & CSP_INT_CSP_RX_TIMEOUT))
	{
		CSPIntDisable(CSP2_BASE,CSP_INT_CSP_RX_TIMEOUT);
	}

	CSPIntDisable(CSP2_BASE,CSP_INT_RXFIFO_THD_REACH);

	CSPIntClear(CSP2_BASE,CSP_INT_RXFIFO_THD_REACH|CSP_INT_CSP_RX_TIMEOUT|CSP_INT_RX_DONE);

	osSemaphoreRelease(g_at_uart_sem);
}

//external mcu must send "AT\r\n",A=0x41,T=0x54
void AutoBaudStart()
{
	//配置为20ms,在2400波特率下，至少接收5个字节
	#define AUTOBAUD_SYSTICK_LOAD_RELOAD (XY_HCLK/50)
	volatile unsigned int gpio_status = 0;

	unsigned int bits_len1 = 0,bits_len2 = 0;

	volatile unsigned int systick_reload_flag = 0;
	unsigned int baudrate = 0;
	unsigned int checkvalue = 0;
	unsigned int edge_triggle_cnt = 0;
	unsigned long at_rxd_mask = 0;
	unsigned long last_time_autobaudrate = 0;

	volatile unsigned char softdelay = 0;
	unsigned int rxdpin_last;
	unsigned int rxdpin_current;
	
	if((g_softap_fac_nv->uart_rate & 0x1ff) !=0 || g_softap_fac_nv->close_at_uart ==1  \
		||(g_softap_fac_nv->peri_remap_enable & (1<<REMAP_CSP2)) == 0 || g_softap_fac_nv->csp2_rxd_pin == 0xff)
	{
		g_autobaud_state = AUTOBAUD_STATE_SKIP;
		return;
	}

	if(get_sys_up_stat() == UTC_WAKEUP || get_sys_up_stat() == EXTPIN_WAKEUP)
	{
		//if wakeup from deepsleep,use the baudrate last time got by autobaud
		last_time_autobaudrate = (g_softap_fac_nv->uart_rate >> 9)*2400;
		if(last_time_autobaudrate > 0 && last_time_autobaudrate <= g_autobaud_table[AUTOBAUD_SUPPORT_NUM-1])
		{
			CSPUARTModeSet(AT_UART_CSP,XY_PCLK,last_time_autobaudrate,8,1);

			if(last_time_autobaudrate > 9600)
			{
				g_autobaud_stop_standby = 1;
			}
			g_autobaud_rate = last_time_autobaudrate;
			g_autobaud_state = AUTOBAUD_STATE_SKIP;
			return;
		}
	}


	at_rxd_mask = (1 << g_softap_fac_nv->csp2_rxd_pin);
	
	g_autobaud_state = AUTOBAUD_STATE_INIT;
	
	while(1)
	{
		switch(g_autobaud_state)
		{
			case AUTOBAUD_STATE_INIT:
			{

				NVIC_DisableIRQ(GPIO_IRQn);

				//Pad应用冲突状态:写1，清除冲突状态，释放分配的外设，进入pad未分配状态;
				HWREG(GPIO_BASE + GPIO_PAD_CFT0) 		= 		at_rxd_mask;
				//每个bit，“0”是GPIO模式(软件控制)，“1”是相应引脚上的外设模式(硬件控制)
			    HWREG(GPIO_BASE + GPIO_CTRL0) 			&= 		~at_rxd_mask;
			    //软件模式下的IO输入使能控制寄存器，1为使能pad输入，0为禁用pad输入；
			    HWREG(GPIO_BASE + GPIO_INPUTEN0)  		|= 		at_rxd_mask;
			    //软件模式下的IO输出使能控制寄存器，0为使能输出
			    HWREG(GPIO_BASE + GPIO_OUTPUTEN0) 		|= 		at_rxd_mask;
				//上拉控制寄存器：0为pad上使能上拉，1为禁止上拉
				HWREG(GPIO_BASE + GPIO_PULLUP0)   		&= 		~at_rxd_mask;
				//下拉控制寄存器：0为使能下拉，1为禁止下拉
				HWREG(GPIO_BASE + GPIO_PULLDOWN0) 		|= 		at_rxd_mask;

				//在外设模式下配置选择控制寄存器，每个bit，1使能外设控制，0使能寄存器控制
				HWREG(GPIO_BASE + GPIO_CFGSEL0)		&=	(~at_rxd_mask);
				GPIOPeripheralPad(GPIO_CSP2_RXD,g_softap_fac_nv->csp2_rxd_pin);



				HWREG(AT_UART_CSP + CSP_MODE1) &= ~CSP_MODE1_CSP_EN;//disable csp2

				
				SysTick->LOAD  = (uint32_t)AUTOBAUD_SYSTICK_LOAD_RELOAD;                         /* 配置 reload 寄存器 */

				//读取时返回当前倒计数的值，
				//写它的同时还会清除CTRL中的COUNTFLAG标志（bit16）
			 	SysTick->VAL   = 0UL;
				//时钟源为内核时钟(FCLK)(39.168M)
			 	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
			                     //SysTick_CTRL_TICKINT_Msk   |
			                     SysTick_CTRL_ENABLE_Msk;                         /* 使能 SysTick 定时器 */
				
				g_autobaud_state = AUTOBAUD_STATE_WAITINT_FIRST;	
				break;
			}

			case AUTOBAUD_STATE_WAITINT_FIRST:
			{
				do{
					gpio_status = HWREG(GPIO_BASE + GPIO_DIN0);
					g_autobaud_tick[0] = SysTick->VAL;
				}while((gpio_status & at_rxd_mask) != 0);
				
				for(softdelay=0;softdelay<5;softdelay++);

				g_autobaud_state = AUTOBAUD_STATE_WAITINT_SECOND;	
				break;
			}

			case AUTOBAUD_STATE_WAITINT_SECOND:
			{
				//SysTick->CTRL的bit16 读取后会自动清零
				systick_reload_flag = SysTick->CTRL;
				systick_reload_flag = 0;

				do{
					gpio_status = HWREG(GPIO_BASE + GPIO_DIN0);
					
					if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
					{
						systick_reload_flag++;
					}
					
				}while((gpio_status & at_rxd_mask) == 0 && (systick_reload_flag < 2));
				for(softdelay=0;softdelay<5;softdelay++);

				if(systick_reload_flag < 2)
				{
					systick_reload_flag = SysTick->CTRL;
					systick_reload_flag = 0;

					do{
						gpio_status = HWREG(GPIO_BASE + GPIO_DIN0);
						
						g_autobaud_tick[1] = SysTick->VAL;
						if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
						{
							systick_reload_flag++;
						}
						
					}while((gpio_status & at_rxd_mask) != 0 && (systick_reload_flag < 2));
					for(softdelay=0;softdelay<5;softdelay++);

					if(systick_reload_flag < 2)
					{
						g_autobaud_state = AUTOBAUD_STATE_WAITINT_THIRD;
					}
					else
					{
						g_autobaud_state = AUTOBAUD_STATE_WAITINT_FIRST;
					}
				}
				else
				{
					g_autobaud_state = AUTOBAUD_STATE_WAITINT_FIRST;	
				}
				
				break;
			}

			case AUTOBAUD_STATE_WAITINT_THIRD:
			{
				systick_reload_flag = SysTick->CTRL;
				systick_reload_flag = 0;

				do{
					gpio_status = HWREG(GPIO_BASE + GPIO_DIN0);
					
					if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
					{
						systick_reload_flag++;
					}
					
				}while((gpio_status & at_rxd_mask) == 0 && (systick_reload_flag < 2));
				for(softdelay=0;softdelay<5;softdelay++);
				
				if(systick_reload_flag < 2)
				{
					systick_reload_flag = SysTick->CTRL;
					systick_reload_flag = 0;

					do{
						gpio_status = HWREG(GPIO_BASE + GPIO_DIN0);
						
						g_autobaud_tick[2] = SysTick->VAL;
						if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
						{
							systick_reload_flag++;
						}
						
					}while((gpio_status & at_rxd_mask) != 0 && (systick_reload_flag < 2));

					if(systick_reload_flag < 2)
					{
						g_autobaud_state = AUTOBAUD_STATE_VERIFY;
					}
					else
					{
						g_autobaud_state = AUTOBAUD_STATE_WAITINT_FIRST;
					}

				}
				else
				{
					g_autobaud_state = AUTOBAUD_STATE_WAITINT_FIRST;	
				}
				
				break;
			}

			//波特率计算与校验
			case AUTOBAUD_STATE_VERIFY:
			{
				edge_triggle_cnt = 0;
				//first falling to second falling
				//AUTOBAUD_SYSTICK_LOAD_RELOAD 先加，后取余，为了防止溢出的情况
				bits_len1 = (g_autobaud_tick[0] + AUTOBAUD_SYSTICK_LOAD_RELOAD - g_autobaud_tick[1]) % AUTOBAUD_SYSTICK_LOAD_RELOAD;
				bits_len2 = (g_autobaud_tick[0] + AUTOBAUD_SYSTICK_LOAD_RELOAD - g_autobaud_tick[2]) % AUTOBAUD_SYSTICK_LOAD_RELOAD;
				checkvalue = (bits_len2*10)/bits_len1;

				//理论值为40，这里取（36-44）90%~110%
				if( checkvalue>36 && checkvalue<44)
				{
					baudrate = XY_HCLK*8/bits_len2;
					char i;

					for(i=AUTOBAUD_SUPPORT_NUM-1;i >= 0;i--)
					{
						if( (baudrate>g_autobaud_table[(int)(i)]*8/10) && (baudrate<g_autobaud_table[(int)(i)]*12/10) )
						{
							baudrate = g_autobaud_table[(int)(i)];
							break;
						}
							
					}

					if(i >= 0)
					{						
						systick_reload_flag = SysTick->CTRL;
						systick_reload_flag = 0;

						rxdpin_last = HWREG(GPIO_BASE + GPIO_DIN0) & at_rxd_mask;

						//计算命令剩余的下降沿
						do{
							rxdpin_current = HWREG(GPIO_BASE + GPIO_DIN0) & at_rxd_mask;

							if(rxdpin_last >0 && rxdpin_current ==0)
							{
								edge_triggle_cnt++;
								systick_reload_flag = 0;//清零来确保传输完成
							}
								
							
							if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
							{
								systick_reload_flag++;
							}
							rxdpin_last = rxdpin_current;
							for(softdelay=0;softdelay<5;softdelay++);
							
						}while(/*edge_triggle_cnt < 10 && */systick_reload_flag < 2);//等待足够的时间来接收

						/*
							AT:T,4 falling edge
							AT\r\n:T\r\n,10 falling edge
							at:t,3 falling edge
							at\r\n:9 falling edge
						*/	
						/*
						if(systick_reload_flag < 2)
						{
							while( (HWREG(GPIO_BASE + GPIO_DIN0) & at_rxd_mask) == 0);//wait till io high,idle
							HWREG(GPIO_BASE + GPIO_CTRL0)		|=	at_rxd_mask;//peripheral mode
							g_autobaud_state = AUTOBAUD_STATE_DONE;
							
						}
						*/
						//后续的下降沿大于等于9才跳转到DONE状态
						//at\r\n:剩余9 个下降沿；			AT\r\n:剩余10个下降沿
						if(edge_triggle_cnt >= 9 )
						{
							while( (HWREG(GPIO_BASE + GPIO_DIN0) & at_rxd_mask) == 0);//等待高电平（空闲态）
							HWREG(GPIO_BASE + GPIO_CTRL0)		|=	at_rxd_mask;//peripheral mode
							g_autobaud_state = AUTOBAUD_STATE_DONE;
							
							
						}
						else
						{
							g_autobaud_state = AUTOBAUD_STATE_INIT;	
						}
						
					}
					else
					{
						g_autobaud_state = AUTOBAUD_STATE_WAITINT_FIRST;
					}
					
				}
				else
				{
					g_autobaud_state = AUTOBAUD_STATE_WAITINT_FIRST;
				}
				break;
			}
				
			case AUTOBAUD_STATE_DONE:
				break;
			default:
				break;
		}

		if(g_autobaud_state == AUTOBAUD_STATE_DONE)
			break;
			
	}

	HWREG(AT_UART_CSP + CSP_MODE1) 		&= ~CSP_MODE1_CSP_EN;
	CSPUARTModeSet(AT_UART_CSP,XY_PCLK,baudrate,8,1);

	char rsp[] = "\r\nOK\r\n";
	
	HWREG(AT_UART_CSP+CSP_INT_STATUS) = CSP_INT_ALL;
	for(unsigned char i = 0 ; i < strlen(rsp) ; i++ )
	{
		CSPCharPut( AT_UART_CSP, *((char *)rsp + i));
	}
	while((HWREG(AT_UART_CSP+CSP_INT_STATUS)&CSP_INT_CSP_TX_ALLOUT) == 0);
	HWREG(AT_UART_CSP+CSP_INT_STATUS) = CSP_INT_CSP_TX_ALLOUT;
	//HWREG(AT_UART_CSP + CSP_INT_STATUS) = CSP_INT_RX_DONE;
	
	NVIC_ClearPendingIRQ(GPIO_IRQn);

	if(baudrate > 9600)
		g_autobaud_stop_standby = 1;
	
	g_autobaud_rate = baudrate;	

	//保存的是自动波特率，低9位还是0；重启后还是会等待命令，进行自动波特率适配
	g_softap_fac_nv->uart_rate = (g_autobaud_rate/2400)<<9;

	SAVE_FAC_PARAM(uart_rate);
}

extern void soft_sample_fill_char(char *buf, int *data_len);
extern void soft_sample_print_wakeup_reason(void);

void AT_uart_rx_task(void)
{
    char aucATBuffer[100] = {0};
    int len = 0;

    while(1)
    {
		//memset(aucATBuffer,0,sizeof(aucATBuffer));
		len = at_uart_read(aucATBuffer,sizeof(aucATBuffer));
		CSPIntEnable(AT_UART_CSP,CSP_INT_RX_DONE);
		
		// bug 3706, standby wakeup response two consecutive 8003
		soft_sample_fill_char(aucATBuffer, &len);

		if((len >= 0) )//|| (farps_handle_once_flag == 1))
		{
			xy_assert(len <= 100);
            {
                at_recv_from_uart(aucATBuffer,len,FARPS_UART_FD);
				soft_sample_print_wakeup_reason();
			}
		}
    }
}

//-1: channel fifo is null
// 0: channel fifo is not null
int check_channel_fifo()
{
    int wait_sum = 0;

WAIT_PROC:
	//delay some time to wait AT finish to recv
	//sleep several ms, probit dirty tail affect next AT req
	if (g_current_baudrate == 2400)
	{
        wait_sum += 8;
		osDelay(8);
	}
	else if (g_current_baudrate <= 9600)
	{
        wait_sum += 4;
		osDelay(4);
	}
	else if (g_current_baudrate <= 115200) // baudrate 115200,very low risk for FIFO overflow
	{
        wait_sum += 2;
		osDelay(2);
	}
	else
	{
        wait_sum += 1;
		xy_tick_delay_for_ATend();
	}

	//判断at uart fifo是否为空
	if (CSPChectRxFifo(AT_UART_CSP) == -1)
    {
        if (wait_sum < WAIT_ATRECV_INTERVAL)
        {
            goto WAIT_PROC;
        }
        else
        {
		    return -1;
        }    
    }

	return 0;
}

uint32_t creat_AT_uart_rx_task()
{
    uint32_t uwRet = pdFAIL;
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "AT_uart_rx";
	thread_attr.priority   = osPriorityNormal3;
	thread_attr.stack_size = 0x480;
	osThreadNew ((osThreadFunc_t)(AT_uart_rx_task),NULL,&thread_attr);
	
    return uwRet;
}
