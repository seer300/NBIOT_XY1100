#include "spi.h"

#if !USE_ROM_SPI
//*****************************************************************************
//
//! Configures the synchronous serial interface.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulSPIClk is the rate of the clock supplied to the SSI module.
//! \param ulProtocol specifies the data transfer protocol.
//! \param ulMode specifies the mode of operation.
//! \param ulBitRate specifies the clock rate.
//! \param ulDataWidth specifies number of bits transferred per frame.
//!
//! This function configures the synchronous serial interface.  It sets
//! the SPI protocol, mode of operation, bit rate, and data width.
//!
//! The \e ulProtocol parameter defines the data frame format.  The
//! \e ulProtocol parameter can be one of the following values:
//! \b SPI_FRF_MOTO_MODE_0, \b SPI_FRF_MOTO_MODE_1, \b SPI_FRF_MOTO_MODE_2,
//! \b SPI_FRF_MOTO_MODE_3.  The Motorola frame formats imply the following 
//! polarity and phase configurations:
//!
//! <pre>
//! Polarity Phase       Mode
//!   0       0   SPI_FRF_MOTO_MODE_0
//!   0       1   SPI_FRF_MOTO_MODE_1
//!   1       0   SPI_FRF_MOTO_MODE_2
//!   1       1   SPI_FRF_MOTO_MODE_3
//! </pre>
//!
//! The \e ulDataWidth parameter defines the width of the data transfers, and
//! can be a value between 8, 16, 24 and 32 bits.
//!
//! \return None.
//
//*****************************************************************************
void
SPIConfigSetExpClk(unsigned long ulBase, unsigned long ulDivide,
                   unsigned long ulProtocol, unsigned long ulMode,
                   unsigned long ulDataWidth)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    ASSERT((ulProtocol == SPI_FRF_MOTO_MODE_0) ||
           (ulProtocol == SPI_FRF_MOTO_MODE_1) ||
           (ulProtocol == SPI_FRF_MOTO_MODE_2) ||
           (ulProtocol == SPI_FRF_MOTO_MODE_3));
    ASSERT((ulMode == SPI_CONFIG_MODE_MASTER) ||
           (ulMode == SPI_CONFIG_MODE_SLAVE));
    ASSERT((ulDivide == SPI_CONFIG_CLK_DIV_2)   || (ulDivide == SPI_CONFIG_CLK_DIV_4)  || (ulDivide == SPI_CONFIG_CLK_DIV_8) ||
           (ulDivide == SPI_CONFIG_CLK_DIV_16)  || (ulDivide == SPI_CONFIG_CLK_DIV_32) || (ulDivide == SPI_CONFIG_CLK_DIV_64) ||
           (ulDivide == SPI_CONFIG_CLK_DIV_128) || (ulDivide == SPI_CONFIG_CLK_DIV_256));
    ASSERT((ulDataWidth == SPI_CONFIG_WORD_SIZE_BITS_8)  || (ulDataWidth == SPI_CONFIG_WORD_SIZE_BITS_16) ||
           (ulDataWidth == SPI_CONFIG_WORD_SIZE_BITS_24) || (ulDataWidth == SPI_CONFIG_WORD_SIZE_BITS_32));

    //
    // Set the configure.
    //
    HWREG(ulBase + SPI_CONFIG) = (HWREG(ulBase + SPI_CONFIG) & ~(SPI_CONFIG_MODE_Msk | SPI_CONFIG_CPOL_Msk | SPI_CONFIG_CPHA_Msk | SPI_CONFIG_CLK_DIV_Msk | SPI_CONFIG_WORD_SIZE_Msk)) |
                                  ulDivide | ulProtocol | ulMode | ulDataWidth;
}

//*****************************************************************************
//
//! Enables the synchronous serial interface.
//!
//! \param ulBase specifies the SPI module base address.
//!
//! This function enables operation of the synchronous serial interface.  The
//! synchronous serial interface must be configured before it is enabled.
//!
//! \return None.
//
//*****************************************************************************
void
SPIEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Set the enable bit.
    //
    HWREG(ulBase + SPI_ENABLE) = 0x1;
}

//*****************************************************************************
//
//! Disables the synchronous serial interface.
//!
//! \param ulBase specifies the SPI module base address.
//!
//! This function disables operation of the synchronous serial interface.
//!
//! \return None.
//
//*****************************************************************************
void
SPIDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Clear the enable bit.
    //
    HWREG(ulBase + SPI_ENABLE) = 0;
}

//*****************************************************************************
//
//! Registers an interrupt handler for the synchronous serial interface.
//!
//! \param ulBase specifies the SPI module base address.
//! \param pfnHandler is a pointer to the function to be called when the
//! synchronous serial interface interrupt occurs.
//!
//! This sets the handler to be called when an SSI interrupt
//! occurs.  This will enable the global interrupt in the interrupt controller;
//! specific SPI interrupts must be enabled via SPIIntEnable().  If necessary,
//! it is the interrupt handler's responsibility to clear the interrupt source
//! via SPIIntClear().
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
SPIIntRegister(unsigned long ulBase, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    unsigned long ulInt;

    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Determine the interrupt number based on the SPI port.
    //
    ulInt = INT_SPI;

    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    IntRegister(ulInt, g_pRAMVectors, pfnHandler);

    //
    // Enable the synchronous serial interface interrupt.
    //
    IntEnable(ulInt);
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for the synchronous serial interface.
//!
//! \param ulBase specifies the SSI module base address.
//!
//! This function will clear the handler to be called when a SPI
//! interrupt occurs.  This will also mask off the interrupt in the interrupt
//! controller so that the interrupt handler no longer is called.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
SPIIntUnregister(unsigned long ulBase, unsigned long *g_pRAMVectors)
{
    unsigned long ulInt;

    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Determine the interrupt number based on the SPI port.
    //
    ulInt = INT_SPI;

    //
    // Disable the interrupt.
    //
    IntDisable(ulInt);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(ulInt, g_pRAMVectors);
}

//*****************************************************************************
//
//! Enables individual SPI interrupt sources.
//!
//! \param ulBase specifies the SPI module base address.
//! \param ulIntFlags is a bit mask of the interrupt sources to be enabled.
//!
//! Enables the indicated SSI interrupt sources.  Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor.  The \e ulIntFlags parameter can be any of the
//! \b SPI_INT_RX_OVERFLOW, \b SPI_INT_MODE_FAIL, \b SPI_INT_TX_FIFO_NF, or 
//! \b SPI_INT_TX_FIFO_FULL, \b SPI_INT_RX_FIFO_NE, \b SPI_INT_RX_FIFO_FULL or
//! \b SPI_INT_TX_FIFO_UNDERFLOW values.
//!
//! \return None.
//
//*****************************************************************************
void
SPIIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Enable the specified interrupts.
    //
    HWREG(ulBase + SPI_IEN) |= ulIntFlags;
}
#endif
//*****************************************************************************
//
//! Disables individual SPI interrupt sources.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulIntFlags is a bit mask of the interrupt sources to be disabled.
//!
//! Disables the SPI interrupt sources. The \e ulIntFlags can be any of the
//! \b SPI_INT_RX_OVERFLOW, \b SPI_INT_MODE_FAIL, \b SPI_INT_TX_FIFO_NF, or 
//! \b SPI_INT_TX_FIFO_FULL, \b SPI_INT_RX_FIFO_NE, \b SPI_INT_RX_FIFO_FULL,
//! \b SPI_INT_TX_FIFO_UNDERFLOW values.
//!
//! \return None.
//
//*****************************************************************************
void
SPIIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Disable the specified interrupts.
    //
    HWREG(ulBase + SPI_IDIS) |= ulIntFlags;
}

#if !USE_ROM_SPI
//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \param ulBase specifies the SSI module base address.
//! \param bMasked is \b false if the raw interrupt status is required or
//! \b true if the masked interrupt status is required.
//!
//! This function returns the interrupt status for the SSI module.  Either the
//! raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \return The current interrupt status, enumerated as a bit field of
//! \b SPI_INT_RX_OVERFLOW, \b SPI_INT_MODE_FAIL, \b SPI_INT_TX_FIFO_NF, or 
//! \b SPI_INT_TX_FIFO_FULL, \b SPI_INT_RX_FIFO_NE, \b SPI_INT_RX_FIFO_FULL,
//! \b SPI_INT_TX_FIFO_UNDERFLOW.
//
//*****************************************************************************
unsigned long
SPIIntStatus(unsigned long ulBase, unsigned char bMasked)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(HWREG(ulBase + SPI_IMASK));
    }
    else
    {
        return(HWREG(ulBase + SPI_STATUS));
    }
}
#endif

//*****************************************************************************
//
//! Puts a data element into the SPI transmit FIFO.
//!
//! \param ulBase specifies the SSI module base address.
//! \param ulData is the data to be transmitted over the SSI interface.
//!
//! This function places the supplied data into the transmit FIFO of the
//! specified SSI module.
//!
//! \note The upper 32 - N bits of the \e ulData are discarded by the hardware,
//! where N is the data width as configured by SSIConfigSetExpClk().  For
//! example, if the interface is configured for 8-bit data width, the upper 24
//! bits of \e ulData are discarded.
//!
//! \return None.
//
//*****************************************************************************
void
SPIDataPut(unsigned long ulBase, unsigned long ulData)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Wait until there is space.
    //
    while(!(HWREG(ulBase + SPI_STATUS) & SPI_INT_TX_FIFO_NF))
    {
    }

    //
    // Write the data to the SSI.
    //
    HWREG(ulBase + SPI_TXD) = ulData;
}

#if !USE_ROM_SPI
//*****************************************************************************
//
//! Puts a data element into the SPI transmit FIFO.
//!
//! \param ulBase specifies the SPI module base address.
//! \param ulData is the data to be transmitted over the SPI interface.
//!
//! This function places the supplied data into the transmit FIFO of the
//! specified SPI module.  If there is no space in the FIFO, then this function
//! returns a zero.
//!
//! \note The upper 32 - N bits of the \e ulData are discarded by the hardware,
//! where N is the data width as configured by SSIConfigSetExpClk().  For
//! example, if the interface is configured for 8-bit data width, the upper 24
//! bits of \e ulData are discarded.
//!
//! \return Returns the number of elements written to the SSI transmit FIFO.
//
//*****************************************************************************
long
SPIDataPutNonBlocking(unsigned long ulBase, unsigned long ulData)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    //
    // Check for space to write.
    //
    if(HWREG(ulBase + SPI_STATUS) & SPI_INT_TX_FIFO_NF)
    {
        HWREG(ulBase + SPI_TXD) = ulData;
        return(1);
    }
    else
    {
        return(0);
    }
}
#endif

//*****************************************************************************
//
//! Gets a data element from the SPI receive FIFO.
//!
//! \param ulBase specifies the SPI module base address.
//! \param pulData is a pointer to a storage location for data that was
//! received over the SSI interface.
//!
//! This function gets received data from the receive FIFO of the specified
//! SSI module and places that data into the location specified by the
//! \e pulData parameter.
//!
//! \note Only the lower N bits of the value written to \e pulData contain
//! valid data, where N is the data width as configured by
//! SSIConfigSetExpClk().  For example, if the interface is configured for
//! 8-bit data width, only the lower 8 bits of the value written to \e pulData
//! contain valid data.
//!
//! \return None.
//
//*****************************************************************************
void
SPIDataGet(unsigned long ulBase, unsigned long *pulData)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Wait until there is data to be read.
    //
    while(!(HWREG(ulBase + SPI_STATUS) & SPI_INT_RX_FIFO_NE))
    {
    }

    //
    // Read data from SSI.
    //
    *pulData = HWREG(ulBase + SPI_RXD);
}

#if !USE_ROM_SPI
//*****************************************************************************
//
//! Gets a data element from the SPI receive FIFO.
//!
//! \param ulBase specifies the SSI module base address.
//! \param pulData is a pointer to a storage location for data that was
//! received over the SSI interface.
//!
//! This function gets received data from the receive FIFO of the specified SPI
//! module and places that data into the location specified by the \e ulData
//! parameter.  If there is no data in the FIFO, then this function returns a
//! zero.
//!
//! \note Only the lower N bits of the value written to \e pulData contain
//! valid data, where N is the data width as configured by
//! SSIConfigSetExpClk().  For example, if the interface is configured for
//! 8-bit data width, only the lower 8 bits of the value written to \e pulData
//! contain valid data.
//!
//! \return Returns the number of elements read from the SSI receive FIFO.
//
//*****************************************************************************
long
SPIDataGetNonBlocking(unsigned long ulBase, unsigned long *pulData)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);

    //
    // Check for data to read.
    //
    if(HWREG(ulBase + SPI_STATUS) & SPI_INT_RX_FIFO_NE)
    {
        *pulData = HWREG(ulBase + SPI_RXD);
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
//! Set Tx FIFO Threshold.
//!
//*****************************************************************************
void
SPISetTxFifoThreshold(unsigned long ulBase, unsigned long ulThreshold)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    ASSERT(ulThreshold < SPI_FIFO_BYTES_TX);
    
    HWREG(ulBase + SPI_TX_THRESH) = ulThreshold;
}

//*****************************************************************************
//
//! Set Rx FIFO Threshold.
//!
//*****************************************************************************
void
SPISetRxFifoThreshold(unsigned long ulBase, unsigned long ulThreshold)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    ASSERT(ulThreshold < SPI_FIFO_BYTES_RX);
    
    HWREG(ulBase + SPI_RX_THRESH) = ulThreshold;
}

//*****************************************************************************
//
//! Set Delay for SPI.
//!
//*****************************************************************************
void
SPISetDelay(unsigned long ulBase, unsigned long ulDelay)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_DELAY) = ulDelay;
}

//*****************************************************************************
//
//! Enable Manual CS for SPI Master.
//!
//*****************************************************************************
void
SPIEnManualCS(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_CONFIG) |= SPI_CONFIG_MANUAL_CS_Msk;
}

//*****************************************************************************
//
//! Disable Manual CS for SPI Master.
//!
//*****************************************************************************
void
SPIDisManualCS(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_CONFIG) &= ~(SPI_CONFIG_MANUAL_CS_Msk);
}

//*****************************************************************************
//
//! Enable Manual Transmit for SPI.
//!
//*****************************************************************************
void
SPIEnManualTransmit(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_CONFIG) |= SPI_CONFIG_MANUAL_START_EN_Msk;
}

//*****************************************************************************
//
//! Disable Manual Transmit for SPI.
//!
//*****************************************************************************
void
SPIDisManualTransmit(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_CONFIG) &= ~(SPI_CONFIG_MANUAL_START_EN_Msk);
}

//*****************************************************************************
//
//! Start Manual Transmit for SPI.
//!
//*****************************************************************************
void
SPIStartManualTransmit(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_CONFIG) |= SPI_CONFIG_MANUAL_START_CMD_Msk;
}

//*****************************************************************************
//
//! Enable Mode Fail Generation for SPI.
//!
//*****************************************************************************
void
SPIEnModeFail(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_CONFIG) |= SPI_CONFIG_MODE_FAIL_GEN_Msk;
}

//*****************************************************************************
//
//! Disable Mode Fail Generation for SPI.
//!
//*****************************************************************************
void
SPIDisModeFail(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == SPI_BASE);
    
    HWREG(ulBase + SPI_CONFIG) &= ~(SPI_CONFIG_MODE_FAIL_GEN_Msk);
}

void
SPITxFifoFlush(unsigned long ulBase)
{
    HWREG(ulBase + SPI_TX_FIFO_OP) |= SPI_FIFO_OP_RESET_Msk;
}

void
SPITxFifoNormal(unsigned long ulBase)
{
    HWREG(ulBase + SPI_TX_FIFO_OP) &= ~SPI_FIFO_OP_RESET_Msk;
}

void
SPIRxFifoFlush(unsigned long ulBase)
{
    HWREG(ulBase + SPI_RX_FIFO_OP) |= SPI_FIFO_OP_RESET_Msk;
}

void
SPIRxFifoNormal(unsigned long ulBase)
{
    HWREG(ulBase + SPI_RX_FIFO_OP) &= ~SPI_FIFO_OP_RESET_Msk;
}
#endif

