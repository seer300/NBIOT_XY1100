//create by yb
//modified 2018-03-22
#include "sim_csp.h"
#include "xinyi_hardware.h"



#define	CSP_ASYNC_TIMEOUT         8	//Number of bit cycles, 8 ETU cycles
char  CSP_GuardTime_dbg=0;
volatile csp_regs *csp1 = (csp_regs *)0xa0120000;


//=================================================================================================================
/**
  * @brief  Enables or disables the CSP's Smart Card mode.
  * @param  CSPx: where x can be 1, 2, 3 or 4 to select the CSP or
  *         CSP peripheral.
  * @param  NewState: new state of the Smart Card mode.
  *          This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void SIM_SmartCardCmd( FunctionalState NewState)
{
    if (NewState != DISABLE)
    {
    	HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) |= (uint32_t)CSP_SM_EN_Msk;
    	//HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) |= (uint32_t)CSP_MODE1_CSP_EN_Msk; //CSP_ENABLE;
    }
    else
    {
    	HWREG(SIM_CSP_BASEADDR +  CSP_SM_CFG) &=  ~(uint32_t)CSP_SM_EN_Msk;
    	//HWREG(SIM_CSP_BASEADDR +  CSP_MODE1) &= ~(uint32_t)CSP_MODE1_CSP_EN_Msk; //CSP_ENABLE;
    }
}

//=================================================================================================================
void SIM_SetGuardTime( uint8_t CSP_GuardTime)
{
    int tx_data_len, tx_sync_len, tx_frm_len, tx_shifter_len;
#if 1
	//if(CSP_GuardTime_dbg!=0)
		//CSP_GuardTime=CSP_GuardTime;
    tx_data_len = 0x07; // 8 bit data
    tx_sync_len = 0x09; // ( 1 start bit + 8 data bit + 1 parity bit), TFS valid bit()
    //active state + idle state
    tx_frm_len = 0x09+CSP_GuardTime; // (1 start bit, 8 data bit, 1 parity bit, x guard time, 10+x bit actually)
    tx_shifter_len = 0x08;	//(8 bit data  plus 1 bit even parity)
    HWREG(SIM_CSP_BASEADDR +  CSP_TX_FRAME_CTRL) = (uint32_t)tx_data_len |
                        (tx_sync_len << 8) |
                        (tx_frm_len << 16) |
                        (tx_shifter_len << 24);
#endif
   return;
}
//Change baudrate of simcard
void SIM_SmartCard_BD( int F_div_D,int CSP_sm_clk_div)
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
	
	HWREG(SIM_CSP_BASEADDR +  CSP_AYSNC_PARAM_REG) 	 = 	(uint32_t)(CSP_ASYNC_TIMEOUT |((sample_div-1)<<16));
    HWREG(SIM_CSP_BASEADDR +  CSP_MODE2)= (HWREG(SIM_CSP_BASEADDR +  CSP_MODE2)&0x801fffff)|(CSP_clk_div<< 21);
}
//Wait previous RX transfer is done before switching to Tx mode
//when wt_time=1; wait time, 16 ~ 32 etu; every increase would add 32etu, typical waiting 32etu by one loop
void SIM_SmartCard_WT(int wt_time)
{
 int i;
 for(i=0; i< wt_time; i++) {
	//Clear previous rx_timeout_INT and then wait another int happens
    HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS) |= (1U<<11);
    while((HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS)&(1U<<11))!= (1U<<11));
    //Clear previous rx_timeout_INT and then wait another int happens
    HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS) |= (1U<<11);
    while((HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS)&(1U<<11))!= (1U<<11));
	HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS) |= (1U<<11);
 }
}

void SIM_TxAllOut()
{
    while((HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS)&(1U<<12))!= (1U<<12));
    HWREG(SIM_CSP_BASEADDR + CSP_INT_STATUS) = (1U<<12);
}

