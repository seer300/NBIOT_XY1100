#include "xy_utils.h"
#include "csp.h"


void CSPUARTModeSet(unsigned long ulBase, unsigned long ulPclk, unsigned long ulBaudrate, unsigned char ucDatabits, unsigned char ucStopbits)
{
	CSPOperationModeSet(ulBase, CSP_MODE1_ASYN);
		
	CSPEndianModeSet(ulBase, CSP_MODE1_ENDIAN_LSBF);
	
	CSPClockSet(ulBase, CSP_MODE_UART, ulPclk, ulBaudrate);
    
    CSPTxFrameSet(ulBase, 1, ucDatabits, ucDatabits + 1, ucDatabits + 1 + ucStopbits);
	
	CSPRxFrameSet(ulBase, 1, ucDatabits, ucDatabits + 1 + ucStopbits);
    
    HWREG(ulBase + CSP_INT_STATUS) = CSP_INT_ALL;
    
    HWREG(ulBase + CSP_TX_DMA_IO_CTRL) = CSP_TX_DMA_IO_CTRL_IO_MODE;
    
    HWREG(ulBase + CSP_RX_DMA_IO_CTRL) = CSP_RX_DMA_IO_CTRL_IO_MODE;
    
    HWREG(ulBase + CSP_TX_DMA_IO_LEN) = 0;
    
    HWREG(ulBase + CSP_RX_DMA_IO_LEN) = 0;
    
    // CSP FIFO Enable
    HWREG(ulBase + CSP_TX_RX_ENABLE) = (CSP_TX_RX_ENABLE_TX_ENA | CSP_TX_RX_ENABLE_RX_ENA);

    // Fifo Reset
    HWREG(ulBase + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_RESET;
    HWREG(ulBase + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_RESET;

    // Fifo Start
    HWREG(ulBase + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_START;
    HWREG(ulBase + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_START;

	//Bug 1636
	//CSP set to UART, and the baundrate is 9600, error rate is high in Specific clock frequency
	//if(ulPclk < 25000000 && ulBaudrate == 9600)    //if(ulPclk == 12240000 && ulBaudrate == 9600)
	{
		//HWREG(ulBase + CSP_AYSNC_PARAM_REG) = (HWREG(ulBase + CSP_AYSNC_PARAM_REG) & (~CSP_AYSNC_PARAM_TIMEOUT_NUM_Msk)) | (0xFFFF << CSP_AYSNC_PARAM_TIMEOUT_NUM_Pos);
		//HWREG(ulBase + CSP_AYSNC_PARAM_REG) = (HWREG(ulBase + CSP_AYSNC_PARAM_REG) & (~CSP_AYSNC_PARAM_DIV2_Msk)) | (0x9 << CSP_AYSNC_PARAM_DIV2_Pos);
		//HWREG(ulBase + CSP_MODE2) = (HWREG(ulBase + CSP_MODE2) & (~CSP_MODE2_CSP_CLK_DIVISOR_Msk)) | (0x7F << CSP_MODE2_CSP_CLK_DIVISOR_Pos);
	}
    
    // CSP Enable
    HWREG(ulBase + CSP_MODE1) |= CSP_MODE1_CSP_EN;
}

void CSPIrdaMode(unsigned long ulBase, unsigned long ulPclk, unsigned long ulBaudrate, unsigned char ucDatabits, unsigned char ucStopbits, unsigned long ulIdleLevel, unsigned long ulWidth)
{
    CSPOperationModeSet(ulBase, CSP_MODE1_ASYN);
		
	CSPEndianModeSet(ulBase, CSP_MODE1_ENDIAN_LSBF);
	
	CSPClockSet(ulBase, CSP_MODE_IRDA, ulPclk, ulBaudrate);
    
    CSPIrdaIdleLevelSet(ulBase, ulIdleLevel);
    
    CSPIrdaPulseWidthSet(ulBase, ulWidth);
    
    CSPIrdaEnable(ulBase, CSP_MODE1_HPSIR_EN);
    
    CSPTxFrameSet(ulBase, 1, ucDatabits, ucDatabits + 1, ucDatabits + 1 + ucStopbits);
	
	CSPRxFrameSet(ulBase, 1, ucDatabits, ucDatabits + 1 + ucStopbits);
    
    HWREG(ulBase + CSP_INT_STATUS) = CSP_INT_ALL;
    
    HWREG(ulBase + CSP_TX_DMA_IO_CTRL) = CSP_TX_DMA_IO_CTRL_IO_MODE;
    
    HWREG(ulBase + CSP_RX_DMA_IO_CTRL) = CSP_RX_DMA_IO_CTRL_IO_MODE;
    
    HWREG(ulBase + CSP_TX_DMA_IO_LEN) = 0;
    
    HWREG(ulBase + CSP_RX_DMA_IO_LEN) = 0;
    
    // CSP FIFO Enable
    HWREG(ulBase + CSP_TX_RX_ENABLE) = (CSP_TX_RX_ENABLE_TX_ENA | CSP_TX_RX_ENABLE_RX_ENA);

    // Fifo Reset
    HWREG(ulBase + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_RESET;
    HWREG(ulBase + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_RESET;

    // Fifo Start
    HWREG(ulBase + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_START;
    HWREG(ulBase + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_START;
    
    // CSP Enable
    HWREG(ulBase + CSP_MODE1) |= CSP_MODE1_CSP_EN;
}

#if !USE_ROM_CSP
void CSPSPIMode(unsigned long ulBase, unsigned long ulMode1, unsigned long ulMode2, unsigned long ulTxFrameCtl, unsigned long ulRxFrameCtl)
{
    // clear all pending uart interrupts
    HWREG(ulBase + CSP_INT_STATUS) = CSP_INT_ALL;

    HWREG(ulBase + CSP_MODE1) = ulMode1;

    HWREG(ulBase + CSP_MODE2) = ulMode2;
    
    HWREG(ulBase + CSP_TX_DMA_IO_CTRL) = CSP_TX_DMA_IO_CTRL_IO_MODE;
    
    HWREG(ulBase + CSP_RX_DMA_IO_CTRL) = CSP_RX_DMA_IO_CTRL_IO_MODE;
    
    HWREG(ulBase + CSP_TX_DMA_IO_LEN) = 0;
    
    HWREG(ulBase + CSP_RX_DMA_IO_LEN) = 0;
    
    HWREG(ulBase + CSP_TX_FRAME_CTRL) = ulTxFrameCtl;
    
    HWREG(ulBase + CSP_RX_FRAME_CTRL) = ulRxFrameCtl;
    
    // CSP FIFO Enable
    HWREG(ulBase + CSP_TX_RX_ENABLE) = (CSP_TX_RX_ENABLE_TX_ENA | CSP_TX_RX_ENABLE_RX_ENA);

    // Fifo Reset
    HWREG(ulBase + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_RESET;
    HWREG(ulBase + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_RESET;

    // Fifo Start
    HWREG(ulBase + CSP_TX_FIFO_OP) = CSP_TX_FIFO_OP_START;
    HWREG(ulBase + CSP_RX_FIFO_OP) = CSP_RX_FIFO_OP_START;
    
    // CSP Enable
    HWREG(ulBase + CSP_MODE1) |= CSP_MODE1_CSP_EN;
}


void CSPFIFOLevelSet(unsigned long ulBase, unsigned long ulTxFifo, unsigned long ulRxFifo)
{
    //
    // Set the FIFO interrupt levels.
    //
    HWREG(ulBase + CSP_TX_FIFO_CTRL) = ulTxFifo;
    HWREG(ulBase + CSP_RX_FIFO_CTRL) = ulRxFifo;
}

unsigned char CSPCharGet(unsigned long ulBase)
{
    while(HWREG(ulBase + CSP_RX_FIFO_STATUS) & CSP_RX_FIFO_STATUS_EMPTY_Msk)
    {
    }
    
    return(HWREG(ulBase + CSP_RX_FIFO_DATA));
}

void CSPCharPut(unsigned long ulBase, unsigned char ucData)
{
    
    while(HWREG(ulBase + CSP_TX_FIFO_STATUS) & CSP_TX_FIFO_STATUS_FULL_Msk)
    {
    }
    
    HWREG(ulBase + CSP_TX_FIFO_DATA) = ucData;
}
#endif

long CSPChectRxFifo(unsigned long ulBase)
{
    if(!(HWREG(ulBase + CSP_RX_FIFO_STATUS) & CSP_RX_FIFO_STATUS_EMPTY_Msk))
    {
        return 0;
    }
    else
    {
        return (-1);
    }
}

#if !USE_ROM_CSP
long CSPCharGetNonBlocking(unsigned long ulBase)
{
    if(!(HWREG(ulBase + CSP_RX_FIFO_STATUS) & CSP_RX_FIFO_STATUS_EMPTY_Msk))
    {
        return(HWREG(ulBase + CSP_RX_FIFO_DATA));
    }
    else
    {
        return (-1);
    }
}

long CSPCharPutNonBlocking(unsigned long ulBase, unsigned char ucData)
{
    if(!(HWREG(ulBase + CSP_TX_FIFO_STATUS) & CSP_TX_FIFO_STATUS_FULL_Msk))
    {
        HWREG(ulBase + CSP_TX_FIFO_DATA) = ucData;
        
        return (true);
    }
    else
    {
        return (false);
    }
}

void CSPOperationModeSet(unsigned long ulBase, unsigned long ulOperationMode)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_SYNC_MODE_Msk) | ulOperationMode;
}

void CSPClockModeSet(unsigned long ulBase, unsigned long ulClockMode)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_CLOCK_MODE_Msk) | ulClockMode;
}

void CSPIrdaEnable(unsigned long ulBase, unsigned long ulIrdaEn)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_HPSIR_EN_Msk) | ulIrdaEn;
}

void CSPEndianModeSet(unsigned long ulBase, unsigned long ulEndianMode)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_ENDIAN_CTRL_Msk) | ulEndianMode;
}

void CSPEnable(unsigned long ulBase)
{
    HWREG(ulBase + CSP_MODE1) |= CSP_MODE1_CSP_EN;
}

void CSPDisable(unsigned long ulBase)
{
    HWREG(ulBase + CSP_MODE1) &= (~CSP_MODE1_CSP_EN);
}

void CSPDrivenEdgeSet(unsigned long ulBase, unsigned long ulRxEdge, unsigned long ulTxEdge)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~(CSP_MODE1_RXD_ACT_EDGE_Msk | CSP_MODE1_TXD_ACT_EDGE_Msk)) | (ulRxEdge | ulTxEdge);
}

void CSPSyncValidLevelSet(unsigned long ulBase, unsigned long ulRFS, unsigned long ulTFS)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~(CSP_MODE1_RFS_ACT_LEVEL_Msk | CSP_MODE1_TFS_ACT_LEVEL_Msk)) | (ulRFS | ulTFS);
}

void CSPClockIdleModeSet(unsigned long ulBase, unsigned long ulClkIdleMode)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_CLK_IDLE_MODE_Msk) | ulClkIdleMode;
}

void CSPClockIdleLevelSet(unsigned long ulBase, unsigned long ulClkIdleLevel)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_CLK_IDLE_LEVEL_Msk) | ulClkIdleLevel;
}

void CSPPinModeSet(unsigned long ulBase, unsigned long ulCSPPin, unsigned long ulPinMode, unsigned long ulPinDirection)
{
    if(ulCSPPin != CSP_PIN_CLK && ulCSPPin != CSP_PIN_RFS && ulCSPPin != CSP_PIN_TFS && ulCSPPin != CSP_PIN_RXD && ulCSPPin != CSP_PIN_TXD)
    {
        return ;
    }
    
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~((1 << ulCSPPin) | (1 << (ulCSPPin+5)))) | (ulPinMode << ulCSPPin) | (ulPinDirection << (ulCSPPin+5));
}

void CSPIrdaPulseWidthSet(unsigned long ulBase, unsigned long ulWidth)
{
    
    HWREG(ulBase + CSP_MODE2) = (HWREG(ulBase + CSP_MODE2) & (~CSP_MODE2_IRDA_DATA_WIDTH_Msk)) | ulWidth;
}

void CSPIrdaIdleLevelSet(unsigned long ulBase, unsigned long ulIrdaIdleLevel)
{
    HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_IrDA_IDLE_LEVEL_Msk) | ulIrdaIdleLevel;
}
#endif

void CSPClockSet(unsigned long ulBase, unsigned long ulCSPMode, unsigned long ulPclk, unsigned ulClock)
{
    unsigned long ulSampleDiv;
    unsigned long ulCSPClkDiv = 2;
    unsigned long ulIrdaWidthDiv;
    
    unsigned long diff;
    unsigned long minSampleDiv = 16;
    unsigned long minDiff = 0x0FFFFFFF;
    unsigned long ulPclkCalculate;
	
    if(ulCSPMode == CSP_MODE_UART)
    {
        //
        // Calculate the best ulSampleDiv and ulCSPClkDiv.
        //
        for(ulSampleDiv = 0x40; ulSampleDiv > 2; ulSampleDiv--)
        {
            ulCSPClkDiv = (2 * ulPclk / ulSampleDiv / ulClock + 1) / 2 - 1;
            
            if(ulCSPClkDiv == 0 || ulCSPClkDiv >= 0x1000 || (ulCSPClkDiv + 1)*ulSampleDiv < 16)
            {
                continue;
            }

			ulPclkCalculate = ulClock * ulSampleDiv * (ulCSPClkDiv + 1);
			if(ulPclk >= ulPclkCalculate)
			{
				diff = ulPclk - ulPclkCalculate;
			}
			else
			{
				diff = ulPclkCalculate - ulPclk;
			}
            
            if(diff < minDiff)
            {
                minDiff = diff;
                minSampleDiv = ulSampleDiv;
            }
			
			if(diff == 0)
			{
				break;
			}
				
        }
        
        ulSampleDiv = minSampleDiv;
        
        ulCSPClkDiv = (2 * ulPclk / ulSampleDiv / ulClock + 1) / 2 - 1;

        HWREG(ulBase + CSP_AYSNC_PARAM_REG) = (HWREG(ulBase + CSP_AYSNC_PARAM_REG) & ~CSP_AYSNC_PARAM_DIV2_Msk) | ((ulSampleDiv - 1) << CSP_AYSNC_PARAM_DIV2_Pos);
    }
    else if(ulCSPMode == CSP_MODE_IRDA)
    {
        ulSampleDiv = 16;
        
        ulCSPClkDiv = (2 * ulPclk / ulSampleDiv / ulClock + 1) / 2 - 1;
        
        ulIrdaWidthDiv = 16 * ulPclk / 10000000 + 1;
        
        HWREG(ulBase + CSP_MODE1) = (HWREG(ulBase + CSP_MODE1) & ~CSP_MODE1_IRDA_WIDTH_DIV_Msk) | (ulIrdaWidthDiv << CSP_MODE1_IRDA_WIDTH_DIV_Pos);

        HWREG(ulBase + CSP_AYSNC_PARAM_REG) = (HWREG(ulBase + CSP_AYSNC_PARAM_REG) & ~CSP_AYSNC_PARAM_DIV2_Msk) | ((ulSampleDiv - 1) << CSP_AYSNC_PARAM_DIV2_Pos);
    }
    else if(ulCSPMode == CSP_MODE_SPI)
    {
        ulCSPClkDiv = ulPclk / ulClock / 2 - 1;
    }
    else if(ulCSPMode == CSP_MODE_SIM)
    {
        // ulCSPClkDiv = ;
    }
    
    HWREG(ulBase + CSP_MODE2) = (HWREG(ulBase + CSP_MODE2) & ~CSP_MODE2_CSP_CLK_DIVISOR_Msk) | ((ulCSPClkDiv & 0x3FF) << CSP_MODE2_CSP_CLK_DIVISOR_Pos);
    HWREG(ulBase + CSP_TX_FRAME_CTRL) = (HWREG(ulBase + CSP_TX_FRAME_CTRL) & ~CSP_TX_FRAME_CTRL_CSP_CLK_DIVISOR_Msk) | ((ulCSPClkDiv & 0xC00) << (CSP_TX_FRAME_CTRL_CSP_CLK_DIVISOR_Pos - 10));
}

#if !USE_ROM_CSP
void CSPTxFrameSet(unsigned long ulBase, unsigned long ulDelay, unsigned long ulDataLen, unsigned long ulSyncLen, unsigned long ulFrameLen)
{
    HWREG(ulBase + CSP_MODE2) = (HWREG(ulBase + CSP_MODE2) & ~CSP_MODE2_TXD_DELAY_LEN_Msk) | (ulDelay << CSP_MODE2_TXD_DELAY_LEN_Pos);
    HWREG(ulBase + CSP_TX_FRAME_CTRL) = (HWREG(ulBase + CSP_TX_FRAME_CTRL) & 0xE0000000) | (ulDataLen - 1) | (ulSyncLen - 1) << CSP_TX_FRAME_CTRL_TX_SYNC_LEN_Pos | (ulFrameLen - 1) << CSP_TX_FRAME_CTRL_TX_FRAME_LEN_Pos | (ulDataLen - 1) << CSP_TX_FRAME_CTRL_TX_SHIFTER_LEN_Pos;
}

void CSPRxFrameSet(unsigned long ulBase, unsigned long ulDelay, unsigned long ulDataLen, unsigned long ulFrameLen)
{
    HWREG(ulBase + CSP_MODE2) = (HWREG(ulBase + CSP_MODE2) & ~CSP_MODE2_RXD_DELAY_LEN_Msk) | (ulDelay << CSP_MODE2_RXD_DELAY_LEN_Pos);
    HWREG(ulBase + CSP_RX_FRAME_CTRL) = (HWREG(ulBase + CSP_RX_FRAME_CTRL) & 0xFFE00000) | (ulDataLen - 1) | (ulFrameLen - 1) << CSP_RX_FRAME_CTRL_RX_FRAME_LEN_Pos | (ulDataLen - 1) << CSP_RX_FRAME_CTRL_RX_SHIFTER_LEN_Pos;
}

void CSPRxEnable(unsigned long ulBase)
{
    HWREG(ulBase + CSP_TX_RX_ENABLE) |= CSP_TX_RX_ENABLE_RX_ENA;
}

void CSPRxDisable(unsigned long ulBase)
{
    HWREG(ulBase + CSP_TX_RX_ENABLE) &= ~CSP_TX_RX_ENABLE_RX_ENA;
}

void CSPTxEnable(unsigned long ulBase)
{
    HWREG(ulBase + CSP_TX_RX_ENABLE) |= CSP_TX_RX_ENABLE_TX_ENA;
}

void CSPTxDisable(unsigned long ulBase)
{
    HWREG(ulBase + CSP_TX_RX_ENABLE) &= ~CSP_TX_RX_ENABLE_TX_ENA;
}


void CSPIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void(*pfnHandler)(void))
{
    //
    // Check the arguments.
    //
    ASSERT((ulIntChannel == INT_CSP1) || (ulIntChannel == INT_CSP2) || (ulIntChannel == INT_CSP3) || (ulIntChannel == INT_CSP4));
    
    //
	// Register the interrupt handler.
	//
	IntRegister(ulIntChannel, g_pRAMVectors, pfnHandler);

	//
	// Enable the interrupt.
	//
	IntEnable(ulIntChannel);
}

void CSPIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors)
{
    //
    // Check the arguments.
    //
    ASSERT((ulIntChannel == INT_CSP1) || (ulIntChannel == INT_CSP2) || (ulIntChannel == INT_CSP3) || (ulIntChannel == INT_CSP4));
    
    //
	// Disable the interrupt.
	//
	IntDisable(ulIntChannel);

	//
	// Unregister the interrupt handler.
	//
	IntUnregister(ulIntChannel, g_pRAMVectors);
}

void CSPIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Enable the specified interrupts.
    //
    HWREG(ulBase + CSP_INT_ENABLE) |= ulIntFlags;
}

void CSPIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Disable the specified interrupts.
    //
    HWREG(ulBase + CSP_INT_ENABLE) &= (~ulIntFlags);
}

unsigned long CSPIntStatus(unsigned long ulBase)
{
    return HWREG(ulBase + CSP_INT_STATUS);
}
#endif

void CSPIntClear(unsigned long ulBase, unsigned long ulIntFlags)
{
    HWREG(ulBase + CSP_INT_STATUS) = ulIntFlags;
}

