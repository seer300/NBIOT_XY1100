#include "dma.h"

#include "osal.h"

osMutexId_t dma_transfer_mutex = NULL;

#if !USE_ROM_DMA
//*****************************************************************************
//
//! Gets the DMA error status.
//!
//! This function returns the DMA error status.  It should be called from
//! within the DMA error interrupt handler to determine if a DMA error
//! occurred.
//!
//! \return Returns non-zero if a DMA error is pending.
//
//*****************************************************************************
unsigned long
DMAErrorStatusGet(unsigned long ulChannelNum)
{
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));

    return((HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_CTRL) & DMAC_CTRL_ERR_Msk) ? true : false);
}

//*****************************************************************************
//
//! Clears the DMA error interrupt.
//!
//! This function clears a pending DMA error interrupt.  It should be called
//! from within the DMA error interrupt handler to clear the interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
DMAErrorStatusClear(unsigned long ulChannelNum)
{
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));

    HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_CTRL) |= DMAC_CTRL_ERR_Msk;
}


//*****************************************************************************
//
//! Requests a DMA channel to start a transfer.
//!
//! \param ulChannelNum is the channel number on which to request a DMA
//! transfer.
//!
//! This function allows software to request a DMA channel to begin a
//! transfer.  This could be used for performing a memory to memory transfer,
//! or if for some reason a transfer needs to be initiated by software instead
//! of the peripheral associated with that channel.
//!
//! \note If the channel is \b UDMA_CHANNEL_SW and interrupts are used, then
//! the completion will be signaled on the DMA dedicated interrupt.  If a
//! peripheral channel is used, then the completion will be signaled on the
//! peripheral's interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
DMAChannelRequest(unsigned long ulChannelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));

    //
    // Set the bit for this channel in the enable set register.
    //
    
    if(ulChannelNum == DMA_CHANNEL_0)
    {
        HWREG(DMAC_CH0_CTRL) |= (DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk);
    }
    else if(ulChannelNum == DMA_CHANNEL_1)
    {
        HWREG(DMAC_CH1_CTRL) |= (DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk);
    }
    else if(ulChannelNum == DMA_CHANNEL_2)
    {
        HWREG(DMAC_CH2_CTRL) |= (DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk);
    }
    else if(ulChannelNum == DMA_CHANNEL_3)
    {
        HWREG(DMAC_CH3_CTRL) |= (DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk);
    }
    
//    HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_CTRL) |= (DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk);
}

//*****************************************************************************
//
//! Config the DMA channel.
//!
//! \return None.
//
//*****************************************************************************
void
DMAChannelControlSet(unsigned long ulChannelNum, unsigned long ulControl)
{
	//
    // Check the arguments.
    //
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));

	//
	// config DMA channel.
	//
    HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_CTRL) = ulControl;
}

//*****************************************************************************
//
//! Config the DMA Source/Destination/Transfer Byte Size.
//!
//! \return None.
//
//*****************************************************************************
void
DMAChannelTransferSet(unsigned long ulChannelNum, void *pvSrcAddr, void *pvDstAddr, unsigned long ulTransferSize)
{
	//
    // Check the arguments.
    //
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));

    ASSERT(ulTransferSize < 0x100000);
    
	//
	// Config the DMA Source/Destination/Transfer Byte Size.
	//
    HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_SA) = (unsigned int)pvSrcAddr;
    HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_DA) = (unsigned int)pvDstAddr;
    HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_TC) = ulTransferSize;
}

//*****************************************************************************
//
//! Config the DMA Next Pointer.
//!
//! \return None.
//
//*****************************************************************************
void
DMAChannelNextPointerSet(unsigned long ulChannelNum, void *pvNextPointer)
{
	//
    // Check the arguments.
    //
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));

	//
    // Config DMA next pointer.
    //
    HWREG(DMAC_BASE + 0x20*ulChannelNum + DMAC_CHx_NP) = (unsigned int)pvNextPointer;
}

//*****************************************************************************
//
//! Registers an interrupt handler for the DMA controller.
//!
//! \param ulIntChannel identifies which DMA interrupt is to be registered.
//! \param pfnHandler is a pointer to the function to be called when the
//! interrupt is activated.
//!
//! \return None.
//
//*****************************************************************************
void
DMAIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    //
    // Check the arguments.
    //
    ASSERT(pfnHandler);
    ASSERT((ulIntChannel == INT_DMAC0) || (ulIntChannel == INT_DMAC1) ||
           (ulIntChannel == INT_DMAC2) || (ulIntChannel == INT_DMAC3));

    //
    // Register the interrupt handler.
    //
    IntRegister(ulIntChannel, g_pRAMVectors, pfnHandler);

    //
    // Enable the memory management fault.
    //
    IntEnable(ulIntChannel);
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for the DMA controller.
//!
//! \param ulIntChannel identifies which DMA interrupt to unregister.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
DMAIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors)
{
	//
    // Check the arguments.
    //
	ASSERT((ulIntChannel == INT_DMAC0) || (ulIntChannel == INT_DMAC1) ||
           (ulIntChannel == INT_DMAC2) || (ulIntChannel == INT_DMAC3));
	
    //
    // Disable the interrupt.
    //
    IntDisable(ulIntChannel);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(ulIntChannel, g_pRAMVectors);
}

//*****************************************************************************
//
//! Gets the DMA controller channel interrupt status.
//!
//! \param ulChannelNum is the channel number.
//!
//! This function is used to get the interrupt status of the DMA controller.
//! The returned value is true or false that indicates the specific channel is
//! requesting an interrupt.  This function can be used from within an
//! interrupt handler to determine or confirm which DMA channel has requested
//! an interrupt.
//!
//! \return Returns true or false indicates the channel is requesting an interrupt.
//
//*****************************************************************************
unsigned long
DMAIntStatus(unsigned long ulChannelNum)
{
    unsigned int bit_offset = 0;
    
	//
    // Check the arguments.
    //
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));
    
    if(ulChannelNum == DMA_CHANNEL_0)
    {
        bit_offset = (1UL << 0);
    }
    else if(ulChannelNum == DMA_CHANNEL_1)
    {
        bit_offset = (1UL << 1);
    }
    else if(ulChannelNum == DMA_CHANNEL_2)
    {
        bit_offset = (1UL << 2);
    }
    else if(ulChannelNum == DMA_CHANNEL_3)
    {
        bit_offset = (1UL << 3);
    }
    
    return((HWREG(DMAC_INT_STAT) & bit_offset) ? true : false);
}

//*****************************************************************************
//
//! Clears DMA interrupt status.
//!
//! \param ulChannelNum is the channel number.
//!
//! \return None.
//
//*****************************************************************************
void
DMAIntClear(unsigned long ulChannelNum)
{
    unsigned int bit_offset = 0;
    
	//
    // Check the arguments.
    //
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));
    
    if(ulChannelNum == DMA_CHANNEL_0)
    {
        bit_offset = (1UL << 0);
    }
    else if(ulChannelNum == DMA_CHANNEL_1)
    {
        bit_offset = (1UL << 1);
    }
    else if(ulChannelNum == DMA_CHANNEL_2)
    {
        bit_offset = (1UL << 2);
    }
    else if(ulChannelNum == DMA_CHANNEL_3)
    {
        bit_offset = (1UL << 3);
    }
    
    HWREG(DMAC_INT_STAT) = bit_offset;
}

//*****************************************************************************
//
//! Wait untile the specific DMA channel transfers done.
//!
//! \param ulChannelNum is the channel number.
//!
//! \return None.
//
//*****************************************************************************
void DMAChannelWaitIdle(unsigned long ulChannelNum)
{
    unsigned int bit_offset = 0;
    
	//
    // Check the arguments.
    //
    ASSERT((ulChannelNum == DMA_CHANNEL_0) || (ulChannelNum == DMA_CHANNEL_1) ||
           (ulChannelNum == DMA_CHANNEL_2) || (ulChannelNum == DMA_CHANNEL_3));

    if(ulChannelNum == DMA_CHANNEL_0)
    {
        bit_offset = (1UL << 0);
    }
    else if(ulChannelNum == DMA_CHANNEL_1)
    {
        bit_offset = (1UL << 1);
    }
    else if(ulChannelNum == DMA_CHANNEL_2)
    {
        bit_offset = (1UL << 2);
    }
    else if(ulChannelNum == DMA_CHANNEL_3)
    {
        bit_offset = (1UL << 3);
    }
    
    while((HWREG(DMAC_INT_STAT) & bit_offset) != bit_offset)
    {
    }
}

void DMAChannelTransfer(unsigned long ulDst, unsigned long ulSrc, unsigned long ulLenByte, unsigned long ulChannelNum)
{
    if(ulLenByte != 0)
    {
        DMAChannelTransferSet(ulChannelNum, (void *)ulSrc, (void *)ulDst, ulLenByte);
        DMAChannelRequest(ulChannelNum);
        DMAChannelWaitIdle(ulChannelNum);
        DMAIntClear(ulChannelNum);
    }
}

void DMAChannelTransfer_withMutex(unsigned long ulDst, unsigned long ulSrc, unsigned long ulLenByte, unsigned long ulChannelNum)
{
    if(ulLenByte != 0)
    {
    	if((osKernelGetState() == osKernelRunning) && (osCoreGetState() == osCoreNormal))
    	{
        	osMutexAcquire(dma_transfer_mutex, osWaitForever);
    	}
        DMAChannelTransferSet(ulChannelNum, (void *)ulSrc, (void *)ulDst, ulLenByte);
        DMAChannelRequest(ulChannelNum);
        DMAChannelWaitIdle(ulChannelNum);
        DMAIntClear(ulChannelNum);
        if((osKernelGetState() == osKernelRunning) && (osCoreGetState() == osCoreNormal))
    	{
        	osMutexRelease(dma_transfer_mutex);
		}
    }
}

void DMAWaitAESDone(void)
{
    while((HWREG(DMAC_INT_STAT) & 0xC) != 0xC)
	{
	}
	
	HWREG(DMAC_INT_STAT) = 0xC;
}

void DMAWaitChannelDone(unsigned char dmaChannels)
{
    while((HWREG(DMAC_INT_STAT) & dmaChannels) != dmaChannels)
	{
	}
	
	HWREG(DMAC_INT_STAT) = dmaChannels;
}
#endif
