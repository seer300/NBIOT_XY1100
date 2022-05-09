#include "smartcard.h"
#include "xy_clk_config.h"
#include "factory_nv.h"
#include "oss_nv.h"
#include "xy_utils.h"
#include "rf_drv.h"
#include "sim_csp.h"

SimCard_Item SC7816_Item;

uint8_t ATR_Rsp[MAX_ATR_CHARACTER_LEN];


uint8_t default_TA1	=	0x11;


volatile uint8_t smartcard_config=0;

volatile uint8_t smartcard_IO_mode=0x07;// IO status: 0:CSP MODE, or 1:IO MODE; bit0:rst, bit1:clk, bit2:txd

osMutexId_t g_smartcard_mutex = NULL;


#define SM_GPIO_CONFIG	0x01
#define SM_INT_CONFIG	0x02



void SimCard_Reset(unsigned char ResetState)
{
	if(ResetState == RST_LOW )
	    HWREG(SIM_CSP_BASEADDR +  CSP_PIN_IO_DATA) &= ~(uint32_t)0x2;
	else
		HWREG(SIM_CSP_BASEADDR +  CSP_PIN_IO_DATA) |= (uint32_t)0x2;
	return;
}

void SimCard_ClkDelay(uint16_t clk)
{

	volatile uint32_t delay = (uint32_t)(clk*SC7816_Item.profile.clk_div*XY_PCLK_DIV/CLK_DELAY_CYCLE);
	while(delay--)
	{
			
	}
	
	//uint32_t clk_end=xthal_get_ccount();
}
extern void  L2_int0_csp1();
void SimCard_Init()
{	
	unsigned char sim_clk_pin,sim_rst_pin,sim_data_pin;
	// Disable TX and RX
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_RX_ENABLE) = ~((uint32_t)(CSP_TX_RX_ENABLE_RX_ENA | CSP_TX_RX_ENABLE_TX_ENA)); //CSP_RX_EN | CSP_TX_EN;
    
    // Fifo Reset
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_OP) 	= (uint32_t)CSP_TX_FIFO_OP_RESET;
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_OP) 	= (uint32_t)CSP_RX_FIFO_OP_RESET; //CSP_RX_FIFO_RESET;
    
    // Fifo Start
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_OP) 	= (uint32_t)CSP_TX_FIFO_OP_START;
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_OP)	= (uint32_t)CSP_RX_FIFO_OP_START; //CSP_RX_FIFO_START;

	
	HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) = (uint32_t)(	  CSP_MODE1_CSP_EN_Msk|CSP_LITTLE_ENDIAN1
														| CSP_MODE1_TFS_PIN_MODE_Msk
														| CSP_MODE1_CLK_PIN_MODE_Msk
														| CSP_MODE1_TXD_PIN_MODE_Msk); //CSP_ENABLE;TFS/CLK/TXD output IO mode
														
	HWREG(SIM_CSP_BASEADDR +  CSP_PIN_IO_DATA) &= ~(uint32_t)(CSP_PIN_IO_DATA_TFS_PIN_VALUE_Msk|CSP_PIN_IO_DATA_TXD_PIN_VALUE_Msk|CSP_PIN_IO_DATA_SCLK_PIN_VALUE_Msk);	//TFS/CLK/TXD output low

	if(    g_softap_fac_nv->sim_rst_pin >= GPIO_PAD_NUM_22 && g_softap_fac_nv->sim_rst_pin <= GPIO_PAD_NUM_24 \
		&& g_softap_fac_nv->sim_clk_pin >= GPIO_PAD_NUM_22 && g_softap_fac_nv->sim_clk_pin <= GPIO_PAD_NUM_24 \
		&& g_softap_fac_nv->sim_data_pin >= GPIO_PAD_NUM_22 && g_softap_fac_nv->sim_data_pin <= GPIO_PAD_NUM_24 )
	{
		sim_rst_pin = g_softap_fac_nv->sim_rst_pin;
		sim_clk_pin = g_softap_fac_nv->sim_clk_pin;
		sim_data_pin = g_softap_fac_nv->sim_data_pin;
	}
	else
	{
		sim_clk_pin = GPIO_PAD_NUM_22;
		sim_rst_pin = GPIO_PAD_NUM_23;
		sim_data_pin = GPIO_PAD_NUM_24;
	}
	
	smartcard_IO_mode = 0x07;// all IO mode
	//IO config
	if(0 == (smartcard_config&SM_GPIO_CONFIG))
	{
		GPIOPeripheralPad( GPIO_CSP1_SCLK, sim_clk_pin);
		GPIOPeripheralPad( GPIO_CSP1_TFS, sim_rst_pin);
		GPIOPeripheralPad( GPIO_CSP1_TXD, sim_data_pin);
		
		GPIODirModeSet(sim_clk_pin,GPIO_DIR_MODE_HW);
		GPIODirModeSet(sim_rst_pin,GPIO_DIR_MODE_HW);
		GPIODirModeSet(sim_data_pin,GPIO_DIR_MODE_HW);

		smartcard_config |= SM_GPIO_CONFIG;
	}
	
	SimCard_Deactivation();
}

uint16_t sim_rx_num 		=	0;


#define SIM_CSP_FIFO_DEEP	128

T_Response_APDU RxAPDU	=	{0};
uint8_t rcv_in_interrupt=0;
uint8_t sim_le_ins;
uint16_t sim_Le_sw=0;

void  L2_int0_csp1()
{

	uint8_t character;
	uint8_t xor_character;
	while(0==SIM_GetFlagStatus(CSP_FLAG_RXNE))
	{
		 character=SIM_ReceiveOneByteData();
		 xor_character = character^0xFF;
		 if(STATE_T0_CMD_NULL == SC7816_Item.T0_state || STATE_T0_CMD_NONE == SC7816_Item.T0_state)
		 {
		 	if(0x60 == character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_NULL;
			}
			else if(sim_le_ins == character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_INS;
			}
			else if(sim_le_ins == xor_character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_COMPLEMENT_INS;
			}
			else if(0x90 == (character&0xF0) || 0x60 == (character&0xF0))
			{
				RxAPDU.sw1	=	character;
				SC7816_Item.T0_state	=	STATE_T0_CMD_SW1;
				HWREG(SIM_CSP_BASEADDR +  CSP_INT_ENABLE) 		 = 	0;
				rcv_in_interrupt=0;
				break;
			}
			else
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
				HWREG(SIM_CSP_BASEADDR +  CSP_INT_ENABLE) 		 = 	0;
				rcv_in_interrupt=0;
				break;
			}
		 }
		 else 
		 {
		 	 RxAPDU.data[sim_rx_num]=character;
			 sim_rx_num++;
			 if(STATE_T0_CMD_COMPLEMENT_INS == SC7816_Item.T0_state)
			 {
			 	SC7816_Item.T0_state	=	STATE_T0_CMD_NONE;
			 }
			 if(sim_rx_num+SIM_CSP_FIFO_DEEP>sim_Le_sw)
			 {
			 	HWREG(SIM_CSP_BASEADDR +  CSP_INT_ENABLE) 		 = 	0;
				rcv_in_interrupt=0;
				break;
			 }	
			 
		 }
	}

	HWREG(SIM_CSP_BASEADDR +  CSP_INT_STATUS)		 =	(uint32_t)CSP_INT_RX_DONE;

}

uint32_t SimCard_GetSMClkDiv()
{
	//max_pclk > 100M
	uint32_t clk_div = 25;
	uint32_t sm_clk;
	
	for(;clk_div > 0;clk_div--)
	{
		sm_clk = (XY_PCLK)/clk_div;
		if(  sm_clk >= SM_CLK_MIN && sm_clk <= SM_CLK_MAX )
			break;
	}

	if(clk_div != 0)
		return clk_div;
	else
		return 1;
}
void SimCard_GetCSPParam( int F_div_D,int CSP_sm_clk_div,int* p_sample_div,int* p_CSP_clk_div)
{
	int CSP_clk_div;
	int sample_div;
	int best_sample_div=0x40;

	int product;
	unsigned int diff;
	unsigned int minDiff = 0xFFFFFFFF;

	//get the best CSP_clk_div and sample_div
	 for(sample_div = 0x40; sample_div > 2; sample_div--)
    {
        CSP_clk_div = ((2*CSP_sm_clk_div*F_div_D)/sample_div +1)/2 -1;
        
        if(CSP_clk_div == 0 || CSP_clk_div >= 0x1000 || (CSP_clk_div + 1)*sample_div < 16)
        {
            continue;
        }

		product = sample_div * (CSP_clk_div + 1);
		if(F_div_D*CSP_sm_clk_div >= product)
		{
			diff = F_div_D*CSP_sm_clk_div - product;
		}
		else
		{
			diff = product - F_div_D*CSP_sm_clk_div;
		}
        
        if(diff < minDiff)
        {
            minDiff = diff;
            best_sample_div = sample_div;
        }
		
		if(diff == 0)
		{
			break;
		}
			
    }
    
    sample_div = best_sample_div;

    CSP_clk_div = ((2*CSP_sm_clk_div*F_div_D)/sample_div +1)/2 -1;
	*p_sample_div = sample_div;
	*p_CSP_clk_div = CSP_clk_div+1;
	
}


void CSP_InitConfig( int F_div_D,int CSP_sm_clk_div, int endian_ctrl, int idle_bit_num)
{
	uint8_t tx_data_len 	 = 0x07; 				// 8 bit data
	uint8_t tx_sync_len 	 = 0x09; 				// ( 1 start bit + 8 data bit + 1 parity bit), TFS valid bit()
	//active state + idle state
	uint8_t tx_frm_len 		 = 0x09+idle_bit_num; 	// (1 start bit, 8 data bit, 1 parity bit, x guard time, 10+x bit actually)
	uint8_t tx_shifter_len	 = 0x08;				//(8 bit data  plus 1 bit even parity)


	uint8_t rx_data_len 	 = 0x07; 				// 8 bit data
	uint8_t rx_frm_len 		 = 0x0a; 				//(1 startbit + 8 databit + 1 paritybit + 1 guard time)
	uint8_t rx_shifter_len 	 = 0x08;

	uint8_t txfifo_threshold = 0x08; 
	uint8_t rxfifo_threshold = 0x18;
	(void) endian_ctrl;
	int sample_div,CSP_clk_div;
    //Enable Smart Card mode and set the clock generation divisor
   	//int CSP_baud_rate 		= ((2*CSP_sm_clk_div*F_div_D)/sample_div +1)/2 -1;
    SimCard_GetCSPParam(F_div_D,CSP_sm_clk_div,&sample_div,&CSP_clk_div);
    // clear all pending uart interrupts
    HWREG(SIM_CSP_BASEADDR +  CSP_INT_STATUS) 		 = 	(uint32_t)CSP_INT_MASK_ALL;

    HWREG(SIM_CSP_BASEADDR +  CSP_MODE2) 			 =  (uint32_t)(1 | (1 << 8) | ( (CSP_clk_div-1) << 21));
    HWREG(SIM_CSP_BASEADDR +  CSP_AYSNC_PARAM_REG) 	 = 	(uint32_t)(CSP_ASYNC_TIMEOUT |((sample_div-1)<<16));
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_DMA_IO_CTRL) 	|= 	(uint32_t)CSP_TX_DMA_IO_CTRL_IO_DMA_SEL_Msk; 			// CSP_TX_IO_MODE;		//IO mode operation to transfer data
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_DMA_IO_CTRL) 	|= 	(uint32_t)CSP_RX_DMA_IO_CTRL_IO_DMA_SEL_Msk; 			// CSP_RX_IO_MODE;
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_DMA_IO_LEN) 	 = 	(uint32_t)0;
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_DMA_IO_LEN) 	 = 	(uint32_t)0;
   

    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FRAME_CTRL) 	 = 	(uint32_t)(tx_data_len | (tx_sync_len << 8) | (tx_frm_len << 16) | (tx_shifter_len << 24));

	
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FRAME_CTRL) 	 = 	(uint32_t)(rx_data_len | (rx_frm_len << 8) | (rx_shifter_len << 16));
   
    
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_CTRL) 	 = (uint32_t)(txfifo_threshold << 2);
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_CTRL) 	 = (uint32_t)(rxfifo_threshold << 2);
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_LEVEL_CHK) = (uint32_t)0x06 | (0x04 << 10) | (0x02 << 20);
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_LEVEL_CHK) = (uint32_t)0x02 | (0x04 << 10) | (0x06 << 20);

    // Enable TX and RX
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_RX_ENABLE) 	 = (uint32_t)CSP_TX_RX_ENABLE_RX_ENA | CSP_TX_RX_ENABLE_TX_ENA;	 //CSP_RX_EN | CSP_TX_EN;
    
    // Fifo Reset
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_OP) 		 = (uint32_t)CSP_TX_FIFO_OP_RESET;
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_OP) 		 = (uint32_t)CSP_RX_FIFO_OP_RESET; 								//CSP_RX_FIFO_RESET;

	// Fifo Start
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_OP) 		 = (uint32_t)CSP_TX_FIFO_OP_START;
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_OP) 		 = (uint32_t)CSP_RX_FIFO_OP_START; 								//CSP_RX_FIFO_START;
}



uint8_t Character_Receive(uint8_t *Data, uint32_t TimeOut)
{

	volatile uint32_t Counter = 0;

	TimeOut=TimeOut*SC7816_Item.profile.clk_div*XY_PCLK_DIV/WHILE_DELAY_CLK;

    while((SIM_GetFlagStatus(CSP_FLAG_RXNE)) && (Counter < TimeOut))
    {
    		Counter++;
    }
	
    if(Counter < TimeOut)
    {
        *Data = SIM_ReceiveOneByteData();
        return SC_SUCCESS;  
    }
    else
    {   	

        return SC_FAILURE; 
    }
}

static uint8_t Character_Send(uint8_t Data, uint32_t TimeOut)
{

	uint32_t Counter = 0;

	while((SIM_GetFlagStatus(  CSP_FLAG_TC)) && (Counter < TimeOut))
	{
		Counter++;
	}
	
	if(Counter != TimeOut)
	{
		SIM_SendOneByteData(Data);
		return SC_SUCCESS;			
	}
	else
	{
		return SC_FAILURE;			
	}

}


uint32_t Etu_2_Clk(uint32_t etu)
{
	return (uint32_t)((SC7816_Item.profile.Fi*etu)/SC7816_Item.profile.Di);
}


uint8_t switch_next_state(uint8_t current_state, uint8_t T_bitmap)
{
	uint8_t current_bit = (current_state-STATE_ATR_PARSE_T0)%4;
	uint8_t bit_offset 	= 0;
	while((current_bit+bit_offset) < 4)
	{
		if(T_bitmap&(1<<(current_bit+bit_offset)))
		{
			return current_state+bit_offset+1;
		}
		bit_offset++;
	}
	return STATE_ATR_PARSE_HISTORY_BYTES;
}

uint8_t Get_Max_Div(uint16_t Max_frequency)
{

	//XY_PCLK/1000=92 160
	uint8_t clk_div=1;
	for (clk_div=1;clk_div<255;clk_div++)
	{
		if(Max_frequency*clk_div>=XY_PCLK/1000)
		{
			break;
		}
		
	}
	return clk_div;

}

uint32_t ATR_Character_Parse(uint8_t ATR_character, uint8_t *T_bitmap, uint8_t * history_bytes)
{
	uint8_t T_indicate = 0;
	uint16_t Fi_Table[16] =
	{
		372, 	372, 	558, 	744, 
		1116, 	1488, 	1860, 	0,
		0, 		512, 	768, 	1024, 
		1536, 	2048, 	0, 		0
	};
	uint8_t Di_Table[] = 
	{
		0,		1,		2,		4, 
		8,		16,		32,		64,
		12,		20,		0,		0,
		0,		0,		0,		0
	};
	/*uint16_t F_Max_Div1000[]=
	{
		4000,		5000,		6000,		8000,
		12000,		16000,		20000,		0,
		0,			5000,		7500,		10000,
		15000,		20000,		0,			0
	};*/
		
	switch(SC7816_Item.atr_state)
	{
		case STATE_ATR_PARSE_TS:
			
			SC7816_Item.atr_state	=	STATE_ATR_PARSE_T0;
			break;
		
		case STATE_ATR_PARSE_T0:
			
			*T_bitmap				=	(ATR_character&0xF0)>>4;
			*history_bytes			=	ATR_character&0x0F;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);					
			break;
			
		case STATE_ATR_PARSE_TA1:
			
			default_TA1				=	ATR_character;
			SC7816_Item.profile.Fi	=	Fi_Table[(ATR_character&0xF0)>>4];
			SC7816_Item.profile.Di	=	Di_Table[ATR_character&0x0F];
			//SC7816_Item.profile.clk_div = Get_Max_Div(F_Max_Div1000[(ATR_character&0xF0)>>4]);
			if(SC7816_Item.profile.Fi == 0 || SC7816_Item.profile.Di == 0)
			{
				SC7816_Item.atr_state   =   STATE_ATR_FAILURE;
			}
			else
			{
				SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			}
			
			break;
			
		case STATE_ATR_PARSE_TB1:
			
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
		
		case STATE_ATR_PARSE_TC1:
			
			if(255==ATR_character)
			{
				SC7816_Item.profile.guard_time=2;
			}
			else
			{
				SC7816_Item.profile.guard_time=2+ATR_character;
			}
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TD1:
			
			T_indicate	=	ATR_character&0x0F;
			*T_bitmap 	= 	(ATR_character&0xF0)>>4;
			if(T_indicate == PROTOCOL_T15)
			{
				T_indicate=0;
			}
			else
			{
				SC7816_Item.profile.T_indicated|=1<<T_indicate;
			}
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TA2:
			
			SC7816_Item.profile.TA2_SpecificMode=ATR_character&0x10;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TB2:
			
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
		
		case STATE_ATR_PARSE_TC2:
			
			SC7816_Item.profile.WI	=	ATR_character;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TD2:
			
			T_indicate	=	ATR_character&0x0F;
			*T_bitmap 	= 	(ATR_character&0xF0)>>4;
			
			SC7816_Item.profile.T_indicated|=1<<T_indicate;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);

			break;
			
		case STATE_ATR_PARSE_TAi:
			
			if(SC7816_Item.profile.T_indicated&(1<<PROTOCOL_T15))
			{
				SC7816_Item.profile.class_clock=ATR_character;
			}
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TBi:
		case STATE_ATR_PARSE_TCi:
			
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
		
		case STATE_ATR_PARSE_TDi:
			
			T_indicate				=	ATR_character&0x0F;
			*T_bitmap 				= 	(ATR_character&0xF0)>>4;
			SC7816_Item.profile.T_indicated|=1<<T_indicate;
			SC7816_Item.atr_state	=	switch_next_state(STATE_ATR_PARSE_TD2,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_HISTORY_BYTES:
			
			if(*history_bytes)
			{
				*history_bytes=(*history_bytes)-1;
			}
			if(0 == *history_bytes)
			{
				if(SC7816_Item.profile.T_indicated&(~(1<<PROTOCOL_T0)))
				{
					SC7816_Item.atr_state	=	STATE_ATR_PARSE_TCK;//only T=0 is indicated,TCK shall be absent,else TCK shall be present
				}
				else
				{
					SC7816_Item.atr_state	=	STATE_ATR_SUCCESS;
				}
			}
			break;
			
		case STATE_ATR_PARSE_TCK:
		default :
			break;
		
	}

	return SC7816_Item.atr_state;
}




uint8_t ATR_Receive(uint8_t *characters)
{

	uint8_t 	result			=	SC_FAILURE;
	uint8_t 	data			=	0;
	uint8_t 	checksum 		= 	0;
	uint8_t 	T_bitmap		=	0;
	
	uint8_t		rcv_num		 	= 	0;
	uint8_t 	history_bytes	=	0;
	uint32_t 	waiting_time 	= 	40000;
	uint8_t 	null_data_num	=	0;
	uint8_t 	debug_atr_data[30] = {0};
	
	memset(debug_atr_data,0,sizeof(debug_atr_data));
	
	while(data!= 0x3 && data != 0x3B)
	{
		result = Character_Receive(&data,waiting_time);
		
		if(SC_FAILURE ==result)
		{
			xy_printf( "uSim TS timeout!debug_atr_data:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", \
						debug_atr_data[0],debug_atr_data[1],debug_atr_data[2],debug_atr_data[3],debug_atr_data[4],\
						debug_atr_data[5],debug_atr_data[6],debug_atr_data[7],debug_atr_data[8],debug_atr_data[9],\
						debug_atr_data[10],debug_atr_data[11],debug_atr_data[12],debug_atr_data[13],debug_atr_data[14] );	
			
			SC7816_Item.atr_state=STATE_ATR_FAILURE;
			return SC_FAILURE;
		}
		else if(data!= 0x3 && data != 0x3B)
		{
			debug_atr_data[null_data_num] = data;
			null_data_num++;
			
			xy_printf("uSim TS dirty %d",null_data_num);
			// by default,1 byte = 12 etu,1 etu = DEFAULT_Fd/DEFAULT_Dd,40000/372/12=9
			if(null_data_num > 15)
			{
				xy_printf("uSim TS dirty exit!debug_atr_data:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", \
						debug_atr_data[0],debug_atr_data[1],debug_atr_data[2],debug_atr_data[3],debug_atr_data[4],\
						debug_atr_data[5],debug_atr_data[6],debug_atr_data[7],debug_atr_data[8],debug_atr_data[9],\
						debug_atr_data[10],debug_atr_data[11],debug_atr_data[12],debug_atr_data[13],debug_atr_data[14] );
				SC7816_Item.atr_state=STATE_ATR_FAILURE;
				return SC_FAILURE;
			}
		}
	}
	if(0x3 == data)
	{
		debug_atr_data[null_data_num++] = data;
		
		do{
			result = Character_Receive(&data,waiting_time);
			if(SC_FAILURE != result)
			{
				debug_atr_data[null_data_num++] = data;
			}
		}while(SC_FAILURE != result && null_data_num < sizeof(debug_atr_data));
			
		//xy_assert(0);
		xy_printf("uSim TS not support!debug_atr_data:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", \
						debug_atr_data[0],debug_atr_data[1],debug_atr_data[2],debug_atr_data[3],debug_atr_data[4],\
						debug_atr_data[5],debug_atr_data[6],debug_atr_data[7],debug_atr_data[8],debug_atr_data[9],\
						debug_atr_data[10],debug_atr_data[11],debug_atr_data[12],debug_atr_data[13],debug_atr_data[14] );	
			
		SC7816_Item.atr_state=STATE_ATR_FAILURE;
		return SC_FAILURE;
	}

	characters[rcv_num]		=	data;
	
	waiting_time			=	Etu_2_Clk(DEFAULT_WT_ETU);
	
	SC7816_Item.atr_state	=	STATE_ATR_PARSE_TS;
	
	while(1)
	{

		ATR_Character_Parse(characters[rcv_num], &T_bitmap, &history_bytes);
		
		if(SC7816_Item.atr_state==STATE_ATR_SUCCESS)
		{
			return SC_SUCCESS;
		}
		else if(SC7816_Item.atr_state==STATE_ATR_FAILURE)
		{
			return SC_FAILURE;
		}
		
		rcv_num++;

		if(SC_FAILURE == Character_Receive(&characters[rcv_num],waiting_time))
		{
			SC7816_Item.atr_state		=	STATE_ATR_FAILURE;
			return SC_FAILURE;
		}

		checksum^=characters[rcv_num];
		
		if(SC7816_Item.atr_state==STATE_ATR_PARSE_TCK)
		{
			if(0 == checksum)
			{
				SC7816_Item.atr_state	=	STATE_ATR_SUCCESS;
				return SC_SUCCESS;
			}
			else
			{
				SC7816_Item.atr_state	=	STATE_ATR_FAILURE;
				return SC_FAILURE;
			}
		}
	}
		
}


void init_SimCard_Item()
{

	SC7816_Item.atr_state				=	STATE_ATR_NONE;
	SC7816_Item.pps_state				=	STATE_PPS_NONE;
	SC7816_Item.T0_state				=	STATE_T0_CMD_NONE;
	SC7816_Item.current_procedure		=	PROCEDURE_NONE;

	memset(&SC7816_Item.profile,0x00,sizeof(SC7816_Item.profile));
		
	SC7816_Item.profile.Fi				=	DEFAULT_Fd;
	SC7816_Item.profile.Di				=	DEFAULT_Dd;
	//SC7816_Item.profile.clk_div			=	CLK_CSP_SM_DIV;
	SC7816_Item.profile.clk_div			=	SimCard_GetSMClkDiv();
	SC7816_Item.profile.WI				=	10;

	SC7816_Item.profile.guard_time		=	2;

	SC7816_Item.profile.T_indicated		=	1<<PROTOCOL_T0;
	SC7816_Item.profile.T_protocol_used	=	PROTOCOL_T0;

}
//If the answer does not begin within 40000 clockcycles with RST at state H,the interface device shall perform a deactivation

void SimCard_ColdReset(char vol_class)
{

	uint8_t		result	=	SC_FAILURE;

	uint16_t 	F_div_D	=	0;

	init_SimCard_Item();

	SC7816_Item.current_procedure		  =	PROCEDURE_COLD_RST_ACTIVATION;

	//RST LOW
	SimCard_Reset(RST_LOW);					//rst low
	SC7816_Item.atr_state				  =	STATE_ATR_RST_LOW;
	


	if(g_softap_fac_nv->xtal_type == 1)
	{
		sim_ioldo2_on();
	}
	
	switch (vol_class)
	{
		//case VOLTAGE_CLASS_A_SUPPORT://5V not supported
		//	break;
		case VOLTAGE_CLASS_B_SUPPORT://3V
			REG_Bus_Field_Set(PRCM_BASE + 0x70, 6, 3, 0x8);
			HWREGB(0xa0058021) &= ~(1<<6);
			break;
		
		case VOLTAGE_CLASS_C_SUPPORT://1.8V
			REG_Bus_Field_Set(PRCM_BASE + 0x70, 6, 3, 0xA);
			HWREGB(0xa0058021) |= (1<<6);
			break;
			
	}
	SimCard_ClkDelay(8000);//wait till simvcc stable,8000*0.25us=2ms
	//IO INPUT
	//HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) |= (CSP_MODE1_TXD_PIN_MODE_Msk|CSP_MODE1_TXD_IO_MODE_Msk);			//IO input
	//SC7816_Item.atr_state				  =	STATE_ATR_IO_MODE_IN;
	
	//CLK provided
	if(smartcard_IO_mode & 0x02)
	{//clk pin in io mode,config csp mode
		HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &=  ~(uint32_t)(CSP_MODE1_CLK_PIN_MODE_Msk); //CLK csp mode
		smartcard_IO_mode &= ~0x02;
	}
	HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) =  (uint32_t)CSP_SM_EN | CSP_SM_RETRY| (SC7816_Item.profile.clk_div-1);		//clk provided
	SC7816_Item.atr_state				  =	STATE_ATR_CLK_INIT;

	//IO PULLUP
	SimCard_ClkDelay(50);
	if(smartcard_IO_mode & 0x04)
	{//txd pin in io mode,config csp mode
		HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &=  ~(uint32_t)(CSP_MODE1_TXD_PIN_MODE_Msk); //TXD csp mode,pull up by extern 20k
		smartcard_IO_mode &= ~0x04;
	}
	//HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &= ~(CSP_MODE1_TXD_PIN_MODE_Msk|CSP_MODE1_TXD_IO_MODE_Msk);		//io pull up in 200clk
	SC7816_Item.atr_state				  =	STATE_ATR_IO_PULLUP;


	F_div_D=SC7816_Item.profile.Fi/SC7816_Item.profile.Di;
	CSP_InitConfig(F_div_D,SC7816_Item.profile.clk_div,CSP_LITTLE_ENDIAN1,2);


	SimCard_ClkDelay(400);
	/*
	if(0 == (smartcard_config&SM_GPIO_CONFIG))
	{
		GPIOPeripheralPad( GPIO_CSP1_TFS, GPIO_PAD_NUM_23);
		GPIODirModeSet(GPIO_PAD_NUM_23,GPIO_DIR_MODE_HW);
		smartcard_config |=SM_GPIO_CONFIG;
	}
	*/
	SimCard_Reset(RST_HIGH);
	SC7816_Item.atr_state				  =	STATE_ATR_RST_HIGH;		//rst high after 400clk


	memset(ATR_Rsp,0x00,sizeof(ATR_Rsp));
	result = ATR_Receive(ATR_Rsp);							//wait atr from 400clk to 40000clk
	

	if(SC_FAILURE==result)
	{

		xy_printf("uSim ATR Failed: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,F/D=%d,clkdiv=%d", ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9],F_div_D,SC7816_Item.profile.clk_div);

		return;
		
	}
	else if(SC7816_Item.profile.class_clock !=0 && (SC7816_Item.profile.class_clock & vol_class) == 0)
	{
		xy_printf("uSim VoltageClass: %d Failed",vol_class);
		return;
	}
	else
	{
		SIM_SetGuardTime(SC7816_Item.profile.guard_time);

		xy_printf("uSim ATR = %x,%x,%x,%x,%x,%x,%x,%x,%x,%x", ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9]);

	}
	
	if(SC7816_Item.profile.T_indicated&(1<<PROTOCOL_T1))
	{
		xy_printf("uSim T=1, not support!");
	}
	
	if(SC7816_Item.profile.TA2_SpecificMode)
	{
	
		//Change the baudrate of sending data based on ATR req from smartcard
	  	F_div_D = SC7816_Item.profile.Fi/SC7816_Item.profile.Di;


		HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) =  (uint32_t)CSP_SM_EN | CSP_SM_RETRY| (SC7816_Item.profile.clk_div-1);		//clk provided

		//HWREG(SIM_CSP_BASEADDR +  CSP_AYSNC_PARAM_REG) 	 = 	(uint32_t)(CSP_ASYNC_TIMEOUT |((SC7816_Item.profile.clk_div-1)<<16));
		
        SIM_SmartCard_BD(F_div_D,SC7816_Item.profile.clk_div);

		
		SC7816_Item.current_procedure	=	PROCEDURE_T0_CMD;
		
		SIM_SmartCard_WT(10);
		
	}
	else
	{
	
		SC7816_Item.current_procedure	=	PROCEDURE_PPS;
		
	}
}


void SimCard_warmReset2(char vol_class)
{

	uint8_t result 		= 	SC_FAILURE;
	uint16_t F_div_D	=	0;
	
	init_SimCard_Item();


	SC7816_Item.current_procedure	=	PROCEDURE_WARM_RST_ACTIVATION;
	SC7816_Item.atr_state			=	STATE_ATR_NONE;

	//RST LOW
	SimCard_Reset(RST_HIGH);

	
	//SC7816_Item.atr_state			=	STATE_ATR_RST_LOW;

	if(g_softap_fac_nv->xtal_type == 1)
	{
		sim_ioldo2_on();
	}
	
	switch (vol_class)
	{
		//case VOLTAGE_CLASS_A_SUPPORT://5V not supported
		//	break;
		case VOLTAGE_CLASS_B_SUPPORT://3V
			REG_Bus_Field_Set(PRCM_BASE + 0x70, 6, 3, 0x8);
			HWREGB(0xa0058021) &= ~(1<<6);
			break;
		
		case VOLTAGE_CLASS_C_SUPPORT://1.8V
			REG_Bus_Field_Set(PRCM_BASE + 0x70, 6, 3, 0xA);
			HWREGB(0xa0058021) |= (1<<6);
			break;
			
	}

	//IO PULLUP
	SimCard_ClkDelay(50);
	if(smartcard_IO_mode & 0x04)
	{//txd pin in io mode,config csp mode
		HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &=  ~(uint32_t)(CSP_MODE1_TXD_PIN_MODE_Msk); //TXD csp mode,pull up by extern 20k
		smartcard_IO_mode &= ~0x04;
	}
	//HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &= ~(CSP_MODE1_TXD_PIN_MODE_Msk|CSP_MODE1_TXD_IO_MODE_Msk);		//io pull up in 200clk
	SC7816_Item.atr_state				  =	STATE_ATR_IO_PULLUP;
	
	SimCard_ClkDelay(8000);//wait till simvcc stable,8000*0.25us=2ms


	F_div_D=SC7816_Item.profile.Fi/SC7816_Item.profile.Di;
	CSP_InitConfig(F_div_D,SC7816_Item.profile.clk_div,CSP_LITTLE_ENDIAN1,2);

	
	//CLK provided
	if(smartcard_IO_mode & 0x02)
	{//clk pin in io mode,config csp mode
		HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &=  ~(uint32_t)(CSP_MODE1_CLK_PIN_MODE_Msk); //CLK csp mode
		smartcard_IO_mode &= ~0x02;
	}
	HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) =  (uint32_t)CSP_SM_EN | CSP_SM_RETRY| (SC7816_Item.profile.clk_div-1);		//clk provided
	SC7816_Item.atr_state				  =	STATE_ATR_CLK_INIT;

	SimCard_ClkDelay(400);//wait clk stable

	//RST LOW
	SimCard_Reset(RST_LOW);					//rst low
	SC7816_Item.atr_state				  =	STATE_ATR_RST_LOW;
	
	SimCard_ClkDelay(400+200);

	SimCard_Reset(RST_HIGH);
	SC7816_Item.atr_state				  =	STATE_ATR_RST_HIGH;		//rst high after 400clk


	memset(ATR_Rsp,0x00,sizeof(ATR_Rsp));
	result = ATR_Receive(ATR_Rsp);							//wait atr from 400clk to 40000clk
	

	if(SC_FAILURE==result)
	{

		xy_printf("uSim ATR Failed: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,F/D=%d,clkdiv=%d", ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9],F_div_D,SC7816_Item.profile.clk_div);

		return;
		
	}
	else if(SC7816_Item.profile.class_clock !=0 && (SC7816_Item.profile.class_clock & vol_class) == 0)
	{
		xy_printf("uSim VoltageClass: %d Failed",vol_class);
		return;
	}
	else
	{
		SIM_SetGuardTime(SC7816_Item.profile.guard_time);

		xy_printf("uSim ATR = %x,%x,%x,%x,%x,%x,%x,%x,%x,%x", ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9]);

	}
	
	if(SC7816_Item.profile.T_indicated&(1<<PROTOCOL_T1))
	{
		xy_printf("uSim T=1, not support!");
	}
	
	if(SC7816_Item.profile.TA2_SpecificMode)
	{
	
		//Change the baudrate of sending data based on ATR req from smartcard
	  	F_div_D = SC7816_Item.profile.Fi/SC7816_Item.profile.Di;


		HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) =  (uint32_t)CSP_SM_EN | CSP_SM_RETRY| (SC7816_Item.profile.clk_div-1);		//clk provided

		//HWREG(SIM_CSP_BASEADDR +  CSP_AYSNC_PARAM_REG) 	 = 	(uint32_t)(CSP_ASYNC_TIMEOUT |((SC7816_Item.profile.clk_div-1)<<16));
		
        SIM_SmartCard_BD(F_div_D,SC7816_Item.profile.clk_div);

		
		SC7816_Item.current_procedure	=	PROCEDURE_T0_CMD;
		
		SIM_SmartCard_WT(10);
		
	}
	else
	{
	
		SC7816_Item.current_procedure	=	PROCEDURE_PPS;
		
	}

	
}


void SimCard_warmReset()
{

	uint8_t result 		= 	SC_FAILURE;
	uint16_t F_div_D	=	0;

	SIM_SmartCard_WT(8);
	
	init_SimCard_Item();


	SC7816_Item.current_procedure	=	PROCEDURE_WARM_RST_ACTIVATION;
	SC7816_Item.atr_state			=	STATE_ATR_NONE;

	//RST LOW
	SimCard_Reset(RST_LOW);
	SC7816_Item.atr_state			=	STATE_ATR_RST_LOW;

	//IO HIGH
	SimCard_ClkDelay(50);				//set IO high in 200clk
	if(smartcard_IO_mode & 0x04)
	{//txd pin in io mode,config csp mode
		HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &=  ~(uint32_t)(CSP_MODE1_TXD_PIN_MODE_Msk); //TXD csp mode,pull up by extern 20k
		smartcard_IO_mode &= ~0x04;
	}
	//HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &= ~(CSP_MODE1_TXD_PIN_MODE_Msk|CSP_MODE1_TXD_IO_MODE_Msk);
	SC7816_Item.atr_state			=	STATE_ATR_IO_PULLUP;



	F_div_D	= SC7816_Item.profile.Fi/SC7816_Item.profile.Di;

	CSP_InitConfig(F_div_D, SC7816_Item.profile.clk_div,CSP_LITTLE_ENDIAN1,2);

	SimCard_ClkDelay(400);				//rst low for 400 clk at least
	SimCard_Reset(RST_HIGH);
	SC7816_Item.atr_state			=	STATE_ATR_RST_HIGH;	


	result = ATR_Receive(ATR_Rsp);	//wait atr from 400clk to 40000clk

	if(SC_FAILURE==result)
	{
		xy_printf("uSim warm reset ATR Failed: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,F/D=%d,clkdiv=%d", ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9],F_div_D,SC7816_Item.profile.clk_div);
		return;
	}
	else
	{
		SIM_SetGuardTime(SC7816_Item.profile.guard_time);
	}


	//if TA2 is present with card in specific mode, use specific values
	//if TA2 is present with card in negotiable mode, use PPS exchange
	//if TA2 is absent, use PPS exchage
	if(SC7816_Item.profile.TA2_SpecificMode)
	{
	
		//Change the baudrate of sending data based on ATR req from smartcard
	  	F_div_D = SC7816_Item.profile.Fi/SC7816_Item.profile.Di;
        SIM_SmartCard_BD(F_div_D, SC7816_Item.profile.clk_div);
		
		SC7816_Item.current_procedure	=	PROCEDURE_T0_CMD;
		
		SIM_SmartCard_WT(10);
		
	}
	else
	{
	
		SC7816_Item.current_procedure	=	PROCEDURE_PPS;
		
	}

	
}


/*
In the following three cases:
	overrun of WI 
	erroneous PPS response
	unsuccessful PPS exchange
the interface device shall perform a deactivation
*/
volatile PPS_element PPS_response_debug	=	{0};
void PPS_Exchange()
{

	uint8_t result 				= 	SC_FAILURE;
	uint8_t checksum 			= 	0;
	uint16_t F_div_D			=	0;
	//uint8_t Negotiation_result 	= 	1;
	
	uint32_t waiting_time 		=	Etu_2_Clk(DEFAULT_WT_ETU);
	
	PPS_element PPS_request		=	{0};
	PPS_element PPS_response	=	{0};


	if(PROCEDURE_PPS != SC7816_Item.current_procedure)
	{
		xy_assert(0);
		return;
	}

	SC7816_Item.pps_state		=	STATE_PPS_NONE;

	
    SIM_SmartCard_WT(8);
	PPS_request.PPSS	=	0xFF;
	Character_Send(PPS_request.PPSS,waiting_time);	//
	PPS_request.PPS0	=	0x10;
	Character_Send(PPS_request.PPS0,waiting_time);	//
	PPS_request.PPS1	=	default_TA1;
	Character_Send(PPS_request.PPS1,waiting_time);	//
	PPS_request.PCK		=	PPS_request.PPSS^PPS_request.PPS0^PPS_request.PPS1;
	Character_Send(PPS_request.PCK,waiting_time);	//
	
	SIM_TxAllOut();

	result	=	Character_Receive(&PPS_response.PPSS,waiting_time);
	if(result==SC_FAILURE)
	{
	
		SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}
	
	result	=	Character_Receive(&PPS_response.PPS0,waiting_time);
	if(result==SC_FAILURE)
	{
	
		SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}
	
	if(PPS_response.PPS0&0x10)
	{
		result	=	Character_Receive(&PPS_response.PPS1,waiting_time);
		if(result==SC_FAILURE)
		{
			
			SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}
	}
	
	if(PPS_response.PPS0&0x20)
	{
		result	=	Character_Receive(&PPS_response.PPS2,waiting_time);
		if(result==SC_FAILURE)
		{
			
			SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}
	}
	
	if(PPS_response.PPS0&0x40)
	{
		result	=	Character_Receive(&PPS_response.PPS3,waiting_time);
		if(result==SC_FAILURE)
		{
			
			SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}
	}
	
	result	=	Character_Receive(&PPS_response.PCK,waiting_time);
	if(result==SC_FAILURE)
	{
	
		SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}

	checksum = 	PPS_response.PCK^PPS_response.PPSS^PPS_response.PPS0^PPS_response.PPS1^PPS_response.PPS2^PPS_response.PPS3;

	if(checksum ||((PPS_response.PPS0&0x0F)	!= (PPS_request.PPS0&0x0F)))
	{
		//Negotiation_result=0;
		SC7816_Item.pps_state=STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}
	if(PPS_response.PPS0&0x10)
	{
		if(PPS_request.PPS1 != PPS_response.PPS1)
		{
			//Negotiation_result=0;
			SC7816_Item.pps_state=STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}

	}
	else
	{
		SC7816_Item.profile.Fi	=	DEFAULT_Fd;
		SC7816_Item.profile.Di	=	DEFAULT_Dd;
	}

	
	F_div_D = SC7816_Item.profile.Fi/SC7816_Item.profile.Di;
	
	//Change the baudrate of sending data based on ATR req from smartcard
	
	//HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) =  (uint32_t)CSP_SM_EN | CSP_SM_RETRY| (SC7816_Item.profile.clk_div-1);		//clk provided

	//HWREG(SIM_CSP_BASEADDR +  CSP_AYSNC_PARAM_REG) 	 = 	(uint32_t)(CSP_ASYNC_TIMEOUT |((SC7816_Item.profile.clk_div-1)<<16));
	
	SIM_SmartCard_BD(F_div_D,SC7816_Item.profile.clk_div);
	SC7816_Item.pps_state			=	STATE_PPS_SUCCESS;
	SC7816_Item.current_procedure	=	PROCEDURE_T0_CMD;
    SIM_SmartCard_WT(10);

	xy_printf("uSim PPS config:F_div_D = %d, CLK_DIV = %d", F_div_D, SC7816_Item.profile.clk_div);


}

void sim_ioldo2_off()
{
	HWD_REG_WRITE08(0xa0058013, (HWD_REG_READ08(0xa0058013)&0xf)|(0x5<<4));
}
void sim_ioldo2_on()
{
	HWD_REG_WRITE08(0xa0058013, HWD_REG_READ08(0xa0058013)&0xbf);
}

void SimCard_Deactivation()
{

	if(SC7816_Item.current_procedure==PROCEDURE_DEACTIVATION)
	{
		return;
	
	}

	//RST LOW
	SimCard_Reset(RST_LOW);
	SimCard_ClkDelay(2);

	//CLK LOW
	SIM_SmartCardCmd(DISABLE);
	if(0 == (smartcard_IO_mode & 0x02))
	{//clk pin in csp mode,config io mode
		HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) =  (HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) | CSP_MODE1_CLK_PIN_MODE_Msk)&(~CSP_MODE1_SCLK_IO_MODE_Msk); //CLK output IO mode
		smartcard_IO_mode |= 0x02;
	}
	HWREG(SIM_CSP_BASEADDR +  CSP_PIN_IO_DATA) &= ~(uint32_t)CSP_PIN_IO_DATA_SCLK_PIN_VALUE_Msk;	//CLK output low
	SimCard_ClkDelay(10);

	//IO LOW
	if(0 == (smartcard_IO_mode & 0x04))
	{//txd pin in csp mode,config io mode
		HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) =  (HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) | CSP_MODE1_TXD_PIN_MODE_Msk)&(~CSP_MODE1_TXD_IO_MODE_Msk); //TXD output IO mode
		smartcard_IO_mode |= 0x04;
	}
	HWREG(SIM_CSP_BASEADDR +  CSP_PIN_IO_DATA) &= ~(uint32_t)CSP_PIN_IO_DATA_TXD_PIN_VALUE_Msk;	//TXD output low
	//HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) 		|= (uint32_t)(CSP_MODE1_TXD_PIN_MODE_Msk|CSP_MODE1_TXD_IO_MODE_Msk);
	//GPIOPullupDis(GPIO_PAD_NUM_24);
	//GPIOPulldownEn(GPIO_PAD_NUM_24);

	//vcc deactivated
	if(g_softap_fac_nv->xtal_type == 1)
	{//XO type
		sim_ioldo2_off();
	}
	
	// Disable TX and RX
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_RX_ENABLE) = ~((uint32_t)(CSP_TX_RX_ENABLE_RX_ENA | CSP_TX_RX_ENABLE_TX_ENA)); //CSP_RX_EN | CSP_TX_EN;
    
    // Fifo Reset
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_OP) 	= (uint32_t)CSP_TX_FIFO_OP_RESET;
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_OP) 	= (uint32_t)CSP_RX_FIFO_OP_RESET; //CSP_RX_FIFO_RESET;
    
    // Fifo Start
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FIFO_OP) 	= (uint32_t)CSP_TX_FIFO_OP_START;
    HWREG(SIM_CSP_BASEADDR +  CSP_RX_FIFO_OP)	= (uint32_t)CSP_RX_FIFO_OP_START; //CSP_RX_FIFO_START;


	SC7816_Item.current_procedure=PROCEDURE_DEACTIVATION;

}



uint8_t SIM_ClkCtrl(uint8_t ctrl_mode, uint32_t timeout)
{
	uint16_t clk_stop_state = SC7816_Item.profile.class_clock&0xC0;
	if(clk_stop_state == CLK_STOP_NOT_SUPPORT)
	{
		return SC_FAILURE;
	}
	if (ctrl_mode == SIM_CLK_STOP)
    {

		//OutputTraceMessage_Usim((7 << 8 | 0x00000001),"uSim SC7816_command clk stop");

    	SimCard_ClkDelay(timeout);
		HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) 	 = 	(HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) & 0xFFFFFE7F) | 0x180;
		if(CLK_STOP_STATE_HIGH_LOW != clk_stop_state)
		{
			if(0 == (smartcard_IO_mode & 0x02))
			{//clk pin in csp mode,config io mode
				HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) =  (HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) | CSP_MODE1_CLK_PIN_MODE_Msk)&(~CSP_MODE1_SCLK_IO_MODE_Msk); //CLK output IO mode
				smartcard_IO_mode |= 0x02;
			}
			
			
			if(CLK_STOP_STATE_LOW_ONLY == clk_stop_state)
			{
				//GPIOPullupDis(GPIO_PAD_NUM_22);
				//GPIOPulldownEn(GPIO_PAD_NUM_22);
				HWREG(SIM_CSP_BASEADDR +  CSP_PIN_IO_DATA) &= ~(uint32_t)CSP_PIN_IO_DATA_SCLK_PIN_VALUE_Msk;	//CLK output low
			}
			else if(CLK_STOP_STATE_HIGH_ONLY == clk_stop_state)
			{
				//GPIOPulldownDis(GPIO_PAD_NUM_22);
				//GPIOPullupEn(GPIO_PAD_NUM_22);
				HWREG(SIM_CSP_BASEADDR +  CSP_PIN_IO_DATA) |= (uint32_t)CSP_PIN_IO_DATA_SCLK_PIN_VALUE_Msk;	//CLK output high
			}
		}
		SC7816_Item.current_procedure			 =	PROCEDURE_CLK_STOP;
    }
    else
    {

		//OutputTraceMessage_Usim((7 << 8 | 0x00000001),"uSim SC7816_command clk start");
		if(CLK_STOP_STATE_HIGH_LOW != clk_stop_state)
		{
			if(smartcard_IO_mode & 0x02)
			{//clk pin in io mode,config csp mode
				HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &= ~CSP_MODE1_CLK_PIN_MODE_Msk; //CLK csp mode
				smartcard_IO_mode &= ~0x02;
			}
		}

    	HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG)	 = 	(HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) & 0xFFFFFE7F) | 0x80;
		SimCard_ClkDelay(timeout);
		
		SC7816_Item.current_procedure			 =	PROCEDURE_T0_CMD;
    }
    return SC_SUCCESS;
}

uint8_t dirty_character[30];
uint8_t dirty_num=0;

uint8_t sim_csp_int_en=0;


uint8_t T0_Cmd_Handler(uint8_t *Txbuff, uint8_t *Rxbuff, uint32_t* len)
{
	uint8_t result			=	SC_FAILURE;
	uint8_t character		=	0;
	uint8_t xor_character	=	0;
	uint8_t p3 				= 	Txbuff[4];
	uint8_t tx_num			=	0;
	
	uint8_t is_rcv_more		=	0;

	T_Command_APDU TxAPDU	=	{0};

	uint32_t waiting_time 	= 	SC7816_Item.profile.WI*960*SC7816_Item.profile.Fi+480*SC7816_Item.profile.Fi;

	

	if((PROCEDURE_T0_CMD != SC7816_Item.current_procedure) && (PROCEDURE_CLK_STOP != SC7816_Item.current_procedure))
	{
		SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;

		xy_printf("uSim Not in T0 state");		

		return SC_FAILURE;
	}
	
	TxAPDU.cla				=	Txbuff[0];
	TxAPDU.ins				=	Txbuff[1];
	TxAPDU.p1				=	Txbuff[2];
	TxAPDU.p2				=	Txbuff[3];
	
	if(*len==5)
	{
		TxAPDU.Lc		=	0;
		if(0==Txbuff[4])
		{
			TxAPDU.Le	=	256;
		}
		else
		{
			TxAPDU.Le	=	(uint16_t)Txbuff[4];
		}
	}
	else
	{
		TxAPDU.Lc		=	Txbuff[4];
		TxAPDU.Le		=	0;
		TxAPDU.data		=	(uint8_t *)(Txbuff+5);
	}


	RxAPDU.data			=	Rxbuff;
	RxAPDU.sw1			=	0;
	RxAPDU.sw2			=	0;
	sim_rx_num			=	0;
	dirty_num			=	0;

	SIM_ClkCtrl(SIM_CLK_RESTART,SIM_CLK_RESTART_TIMEOUT);

	SIM_SmartCard_WT(8);


	SIM_resetTxFifo();
	SIM_resetRxFifo();
	
	SC7816_Item.T0_state=STATE_T0_CMD_NONE;

	if(sim_csp_int_en&&(TxAPDU.Le>SIM_CSP_FIFO_DEEP-2))
	{
		rcv_in_interrupt	=	1;
		sim_le_ins			=	TxAPDU.ins;
		sim_Le_sw			=	TxAPDU.Le+2;
		is_rcv_more			=	0;
		
		// clear all pending uart interrupts
   		HWREG(SIM_CSP_BASEADDR +  CSP_INT_STATUS) 		 = 	(uint32_t)CSP_INT_MASK_ALL;
		HWREG(SIM_CSP_BASEADDR +  CSP_INT_ENABLE) 		 = 	(uint32_t)(CSP_INT_RX_DONE);
	}
	else{
		rcv_in_interrupt	=	0;
		is_rcv_more			=	1;

	}


	Character_Send(TxAPDU.cla,waiting_time);	
	Character_Send(TxAPDU.ins,waiting_time);	
	Character_Send(TxAPDU.p1,waiting_time);	
	Character_Send(TxAPDU.p2,waiting_time);	
	Character_Send(p3,waiting_time);	



	SIM_TxAllOut();


	while(1)
	{
		if(rcv_in_interrupt)
		{
			continue;
		}
		else if(1 == is_rcv_more)
		{
			result=Character_Receive(&character,waiting_time);
			xor_character = character^0xFF;
			if(SC_FAILURE == result)
			{
				//xy_assert(0);
				SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
				return SC_FAILURE;
			}
			if(0x60 == character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_NULL;
			}
			else if(TxAPDU.ins == character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_INS;
			}
			else if(TxAPDU.ins == xor_character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_COMPLEMENT_INS;
			}
			else if(0x90 == (character&0xF0) || 0x60 == (character&0xF0))
			{
				RxAPDU.sw1	=	character;
				SC7816_Item.T0_state	=	STATE_T0_CMD_SW1;
			}
			else
			{
				//xy_assert(0);
				dirty_character[dirty_num]=character;
				dirty_num++;
				if(30==dirty_num)
				{
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;
					//xy_assert(0);
				}
				continue;
			}

		}
		
		switch (SC7816_Item.T0_state)
		{
			case STATE_T0_CMD_NULL:
			case STATE_T0_CMD_NONE:
				break;
			case STATE_T0_CMD_INS:
			{
				if(TxAPDU.Le)
				{
					while(sim_rx_num<TxAPDU.Le)
					{
						result=Character_Receive(&character,waiting_time);
						if(SC_FAILURE == result)
						{
							SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
							return SC_FAILURE;
						}
						RxAPDU.data[sim_rx_num]=character;
						sim_rx_num++;
					}
				}
				else if(TxAPDU.Lc)
				{
					SIM_SmartCard_WT(1);

					//TODO: clear TX and RX fifo	
					SIM_resetTxFifo();
					SIM_resetRxFifo();
					while(tx_num<TxAPDU.Lc)
					{
						Character_Send(TxAPDU.data[tx_num],waiting_time);	
						
						tx_num++;
					}
					SIM_TxAllOut();
				}
				else 
				{
					xy_printf("uSim INS wrong Lc and Le");
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;

				}
				break;
			}
			case STATE_T0_CMD_COMPLEMENT_INS:
			{
				if(TxAPDU.Le)
				{
					if(sim_rx_num<TxAPDU.Le)
					{
						result=Character_Receive(&character,waiting_time);
						if(SC_FAILURE == result)
						{
							SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
							return SC_FAILURE;
						}
						RxAPDU.data[sim_rx_num]=character;
						sim_rx_num++;						

					}
				}
				else if(TxAPDU.Lc)
				{

					SIM_SmartCard_WT(1);
			
					if(tx_num<TxAPDU.Lc)
					{
						Character_Send(TxAPDU.data[tx_num],waiting_time);	
						tx_num++;
					}
					SIM_TxAllOut();

				}
				else 
				{
					xy_printf("uSim COMPLEMENT INS wrong Lc and Le");
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;

				}
				break;
			}
			case STATE_T0_CMD_SW1:
			{
				
				result=Character_Receive(&character,waiting_time);
				if(SC_FAILURE == result)
				{
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;
				}
				RxAPDU.sw2=character;
				SC7816_Item.T0_state 		= 	STATE_T0_CMD_SUCCESS;
				break;
			}
			default:
			{
				xy_printf("uSim wrong STATE_T0_CMD");
				SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
				return SC_FAILURE;
			}
		}

		if(STATE_T0_CMD_SUCCESS == SC7816_Item.T0_state)
		{
			/*uint16_t sw=(uint16_t)((RxAPDU.sw1 << 8) | RxAPDU.sw2);

			if(sw == 0x6B00 || sw == 0x6D00 || sw == 0x6E00 || sw == 0x6F00)
			{
				if(g_sim_avoid_assert)
				{
				}
				else
					xy_assert(0);
			}
			*/
			if(TxAPDU.Le&&sim_rx_num)
			{
				if(sim_rx_num<TxAPDU.Le)
				{
					xy_printf("uSim sim_rx_num less than Le");
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;

				}
				else
				{
					RxAPDU.data[TxAPDU.Le] 		= RxAPDU.sw1;
					RxAPDU.data[TxAPDU.Le+1] 	= RxAPDU.sw2;
					*len 						= TxAPDU.Le+2;
				}
			}
			else
			{
				RxAPDU.data[0] = RxAPDU.sw1;
				RxAPDU.data[1] = RxAPDU.sw2;
				*len = 2;
			}
			if((RxAPDU.sw1!=0x90) && (RxAPDU.sw1!=0x91) && (RxAPDU.sw1!=0x61)  && (RxAPDU.sw1!=0x6C) && (RxAPDU.sw1!=0x63))
			{

				xy_printf("uSim T0_Cmd_Handler : current_procedure=%d,[CLA,INS,P1,P2,P3]=[%x,%x,%x,%x,%x],sw=[%x %x]", SC7816_Item.current_procedure,TxAPDU.cla,TxAPDU.ins,TxAPDU.p1,TxAPDU.p2,p3,RxAPDU.sw1,RxAPDU.sw2);

			}

			SIM_ClkCtrl(SIM_CLK_STOP,SIM_CLK_STOP_TIMEOUT);

			return SC_SUCCESS;
		}
		else{
				is_rcv_more	=	1;
			}
		
	}


}





void SC7816_command(uint8_t *pApduBuf,uint8_t *pRxBuffer,uint32_t *uLen)
{

	static unsigned char s_simvcc_select_done = 0;
	uint32_t csp_int_status;
	char loop,loopmax=1,select_class,retry_delay,store_rlt;
	char vol_class[2];
	
	if(NULL == g_smartcard_mutex)
	{
		g_smartcard_mutex = osMutexNew(NULL);
	}

	osMutexAcquire(g_smartcard_mutex, osWaitForever); 

	if(*uLen == 1)
	{
		switch(pApduBuf[0])
    	{
	    	case  Pw_on: 
		    {
				select_class = (g_softap_fac_nv->sim_vcc_ctrl)&7;
				store_rlt = (g_softap_fac_nv->sim_vcc_ctrl&0X70)>>4;
				retry_delay = (g_softap_fac_nv->sim_vcc_ctrl&0X08)>>3;
				
				if(g_softap_fac_nv->xtal_type == 0)
				{//tcxo
					if( select_class == 0)
					{
						vol_class[0] = VOLTAGE_CLASS_B_SUPPORT;//3V
						loopmax = 1;
					}
					else
					{
						xy_assert(0);
					}
				}
				else
				{//xo
					if( select_class == 0)
					{
						vol_class[0] = VOLTAGE_CLASS_B_SUPPORT;//3V
						loopmax = 1;
					}
					else if( select_class == 1)
					{
						vol_class[0] = VOLTAGE_CLASS_C_SUPPORT;//1.8V
						loopmax = 1;
					}
					else if(select_class==3 && store_rlt==NV_SIMVCC_1V8_BIT) //&& do_store==1 
					{
						vol_class[0] = VOLTAGE_CLASS_C_SUPPORT;//1.8V
						vol_class[1] = VOLTAGE_CLASS_B_SUPPORT;//3V
						loopmax = 2;
					}
					else if(select_class==2 && store_rlt==NV_SIMVCC_3V_BIT)  //&& do_store==1 
					{
						vol_class[0] = VOLTAGE_CLASS_B_SUPPORT;//3V
						vol_class[1] = VOLTAGE_CLASS_C_SUPPORT;//1.8V
						loopmax = 2;
					}
					else if(select_class == 2)
					{
						vol_class[0] = VOLTAGE_CLASS_C_SUPPORT;//1.8V
						vol_class[1] = VOLTAGE_CLASS_B_SUPPORT;//3V
						loopmax = 2;
					}
					else if(select_class == 3)
					{
						vol_class[0] = VOLTAGE_CLASS_B_SUPPORT;//3V
						vol_class[1] = VOLTAGE_CLASS_C_SUPPORT;//1.8V
						loopmax = 2;
					}
					else
						xy_assert(0);
				}
				
				
				loop = 0;
				//if POWERON or DEEPSLEEP or RESET,do VCC select,else only do VCC check once
				if(s_simvcc_select_done == 1)
					loopmax = 1;
				
				while(loop < loopmax)
				{
					SimCard_Init();
					SimCard_ColdReset(vol_class[(int)(loop)]);

					if(SC7816_Item.current_procedure ==	PROCEDURE_PPS)
					{
						PPS_Exchange();
						if(SC7816_Item.current_procedure !=	PROCEDURE_T0_CMD)
						{
							xy_printf("uSim PPS failed:PPS_response = %x,%x,%x,%x,%x", PPS_response_debug.PPSS, PPS_response_debug.PPS0, PPS_response_debug.PPS1, PPS_response_debug.PPS2, PPS_response_debug.PPS3);
						}
					}
					
					if(SC7816_Item.current_procedure ==	PROCEDURE_T0_CMD)
					{
						pRxBuffer[0] = 0x90;
			            pRxBuffer[1] = 0x00;
			            *uLen = 2;

						SIM_ClkCtrl(SIM_CLK_STOP,SIM_CLK_STOP_TIMEOUT);

						if(store_rlt!=vol_class[(int)(loop)])
						{
							g_softap_fac_nv->sim_vcc_ctrl = (g_softap_fac_nv->sim_vcc_ctrl&0X8F)|(vol_class[(int)(loop)]<<4);
							SAVE_FAC_PARAM(sim_vcc_ctrl);
						}
						break;
					}
					else
					{
						pRxBuffer[0] = 0x63;
		                pRxBuffer[1] = 0x01;
						*uLen=2;
						SimCard_Deactivation();
						
						loop++;
						if(	loop >= loopmax 
							|| (SC7816_Item.profile.class_clock !=0 && (SC7816_Item.profile.class_clock & vol_class[(int)(loop)]) == 0) )
						{

							xy_printf("uSim initial failed!");
							//if SIM init failed,set  simcc by user setting
							if(select_class==1 || select_class==2)
							{
								REG_Bus_Field_Set(PRCM_BASE + 0x70, 6, 3, 0xA);
								HWREGB(0xa0058021) |= (1<<6);
							}
							else
							{
								REG_Bus_Field_Set(PRCM_BASE + 0x70, 6, 3, 0x8);
								HWREGB(0xa0058021) &= ~(1<<6);
							}
							break;//out while
						}
						else if(loop==1 && loopmax==2 )
						{

							if(retry_delay)
							{
								osDelay(1000);// 10ms at least,1s,not wait too long
							}
							else
							{
								osDelay(20000);//  10ms at least,20s to drop to below 0.4v
							}
								
						}
						else
						{
							xy_assert(0);
						}
						
					}
				}
				s_simvcc_select_done = 1;
				break;
			}
			case  Pw_off:    
		    {

				if(SC7816_Item.current_procedure!=PROCEDURE_DEACTIVATION)
				{
					SIM_ClkCtrl(SIM_CLK_RESTART,SIM_CLK_RESTART_TIMEOUT);

					SIM_SmartCard_WT(8);
		        	SimCard_Deactivation();
				}
		    	
		        pRxBuffer[0] = 0x90;
		        pRxBuffer[1] = 0x00;
		        *uLen = 2;

		        break;
		    }
			default:
			{
				pRxBuffer[0] = 0x63;
		        pRxBuffer[1] = 0x01;
		        *uLen = 2;

		        break;
			}
		}
	}
	else	
    {
    	T0_Cmd_Handler(pApduBuf,pRxBuffer,uLen);
		while(SC7816_Item.T0_state==STATE_T0_CMD_FAILURE)
		{
			csp_int_status = HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS);
			HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS)=(uint32_t)CSP_INT_MASK_ALL;
			if(csp_int_status&CSP_INT_RX_OFLOW)
			{

				xy_printf("uSim SC7816_command Try again,[CLA,INS,P1,P2,P3]=[%x,%x,%x,%x,%x]", pApduBuf[0],pApduBuf[1],pApduBuf[2],pApduBuf[3],pApduBuf[4]);

				T0_Cmd_Handler(pApduBuf,pRxBuffer,uLen);
			}
			else
			{
				pRxBuffer[0] = 0x63;
				pRxBuffer[1] = 0x01;
				*uLen=2;
				//If the answer does not begin within 40000 clockcycles with RST at state H,the interface device shall perform a deactivation
				SimCard_Deactivation();
				//g_uicc_focus=1;
				break;
			}
		}

	}

	osMutexRelease(g_smartcard_mutex); 


}



