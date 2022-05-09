#include "xy_utils.h"
#include "uart.h"
#include "csp.h"
#include "stdarg.h"

#ifdef DEBUG
static unsigned char
UARTBaseValid(unsigned long ulBase)
{
    return(ulBase == UART1_BASE);
}
#endif


//*****************************************************************************
//
//! Enables transmitting and receiving.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function sets the UARTEN, TXE, and RXE bits
//!
//! \return None.
//
//*****************************************************************************
void
UARTEnable(unsigned long ulBase)
{
    unsigned long data;
    unsigned long ultimeout;
   
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    
    //Get timeout value
    ultimeout = HWREG(ulBase + UART_RX_TIMEOUT); 

    //
    // Reset UART 
    //
    HWREG(ulBase + UART_CONTROL) = (HWREG(ulBase + UART_CONTROL) | (UART_CTRL_TXDIS | UART_CTRL_RXDIS | UART_CTRL_TXRES | UART_CTRL_RXRES));

    data = HWREG(ulBase + UART_CONTROL);

    while (( data & (UART_CTRL_TXRES | UART_CTRL_RXRES)) != 0) {
        data = HWREG(ulBase + UART_CONTROL);
    }

    //
    // Enable RX, TX, and the UART.
    //
    HWREG(ulBase + UART_CONTROL) = (UART_CTRL_TXEN | UART_CTRL_RXEN);
    
    //Re-configure timeout value
    HWREG(ulBase + UART_RX_TIMEOUT) = ultimeout;
}


#if !USE_ROM_UART
//*****************************************************************************
//
//! Sets the type of parity.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulParity specifies the type of parity to use.
//!
//! This function sets the type of parity to use for transmitting and expect
//! when receiving.  The \e ulParity parameter must be one of
//! \b UART_MODE_PARITY_NONE, \b UART_MODE_PARITY_EVEN, \b UART_MODE_PARITY_ODD,
//! \b UART_MODE_PARITY_ONE, or \b UART_MODE_PARITY_ZERO.
//!
//! \return None.
//
//*****************************************************************************
void
UARTParityModeSet(unsigned long ulBase, unsigned long ulParity)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    ASSERT((ulParity == UART_MODE_PARITY_NONE) ||
           (ulParity == UART_MODE_PARITY_EVEN) ||
           (ulParity == UART_MODE_PARITY_ODD) ||
           (ulParity == UART_MODE_PARITY_ONE) ||
           (ulParity == UART_MODE_PARITY_ZERO));

    //
    // Set the parity mode.
    //
    HWREG(ulBase + UART_MODE) = ((HWREG(ulBase + UART_MODE) & ~UART_MODE_PARITY_Msk) | ulParity);
}

//*****************************************************************************
//
//! Gets the type of parity currently being used.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function gets the type of parity used for transmitting data and
//! expected when receiving data.
//!
//! \return Returns the current parity settings, specified as one of
//! \b UART_MODE_PARITY_NONE, \b UART_MODE_PARITY_EVEN, \b UART_MODE_PARITY_ODD,
//! \b UART_MODE_PARITY_ONE, or \b UART_MODE_PARITY_ZERO.
//
//*****************************************************************************
unsigned long
UARTParityModeGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return the current parity setting.
    //
    return(HWREG(ulBase + UART_MODE) & UART_MODE_PARITY_Msk);
}

//*****************************************************************************
//
//! Sets the FIFO level at which interrupts are generated.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulTxLevel is the transmit FIFO interrupt level: 
//!	0, disable; 1~63, trigger bytes
//! \param ulRxLevel is the receive FIFO interrupt level:
//!	0, disable; 1~63, trigger bytes
//! This function sets the FIFO level at which transmit and receive interrupts
//! are generated.
//!
//! \return None.
//
//*****************************************************************************
void
UARTFIFOLevelSet(unsigned long ulBase, unsigned long ulTxLevel, unsigned long ulRxLevel)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    ASSERT(ulTxLevel < UART_TX_FIFO_BYTES);
    ASSERT(ulRxLevel < UART_RX_FIFO_BYTES);

    //
    // Set the FIFO interrupt levels.
    //
    HWREG(ulBase + UART_TX_TRIGGER) = ulTxLevel;
    HWREG(ulBase + UART_RX_TRIGGER) = ulRxLevel;
}

//*****************************************************************************
//
//! Gets the FIFO level at which interrupts are generated.
//!
//! \param ulBase is the base address of the UART port.
//! \param pulTxLevel is a pointer to storage for the transmit FIFO level:
//!	0, disable; 1~63, trigger bytes
//! \param pulRxLevel is a pointer to storage for the receive FIFO level:
//!	0, disable; 1~63, trigger bytes
//!
//! This function gets the FIFO level at which transmit and receive interrupts
//! are generated.
//!
//! \return None.
//
//*****************************************************************************
void
UARTFIFOLevelGet(unsigned long ulBase, unsigned long *pulTxLevel, unsigned long *pulRxLevel)
{
    unsigned long ulTemp;

    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Read the FIFO level register.
    //
    ulTemp = HWREG(ulBase + UART_TX_TRIGGER);

    //
    // Extract the transmit FIFO levels.
    //
    *pulTxLevel = ulTemp & 0x3F;
    
    //
    // Read the FIFO level register.
    //
    ulTemp = HWREG(ulBase + UART_RX_TRIGGER);

    //
    // Extract the receive FIFO levels.
    //
    *pulRxLevel = ulTemp & 0x3F;
}

//*****************************************************************************
//
//! Sets the configuration of a UART.
//! if ulPreDivide == 1, UART Source Clock will be devided by 8 first;
//! UART Sample Rate = Select Clock / CD;
//! UART Bandrate = UART Sample Rate / (ulBDIV + 1);
//! \return None.
//
//*****************************************************************************
void
UARTConfigSetExpClk(unsigned long ulBase, unsigned long ulPreDivide, unsigned long ulCD, unsigned long ulBDIV, unsigned long ulConfig)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    ASSERT(ulPreDivide == 0 || ulPreDivide == 1);
    ASSERT(ulCD > 0 && ulCD < 0x10000);
    ASSERT(ulBDIV > 4 && ulBDIV < 0x100);

    //
    // Stop the UART.
    //
    UARTDisable(ulBase);

    if(ulPreDivide == 1)
    {
        HWREG(ulBase + UART_MODE) |= UART_MODE_SEL_CLOCK_UART_DIV_8;
    }
    else
    {
        HWREG(ulBase + UART_MODE) &= ~(UART_MODE_SEL_CLOCK_UART_DIV_8);
    }

    //
    // Set the baud rate.
    //
    HWREG(ulBase + UART_BAUD_RATE) = (ulCD & UART_Baudrate_CD_Msk);
    HWREG(ulBase + UART_BAUD_DIV)  = (ulBDIV & UART_Baudrate_BDIV_Msk);

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(ulBase + UART_MODE) = ((HWREG(ulBase + UART_MODE) & ~(UART_MODE_CHANNEL_MODE_BITS_Msk | UART_MODE_DATA_BITS_Msk | UART_MODE_PARITY_Msk | UART_MODE_STOP_BITS_Msk)) | ulConfig);

    //
    // Clear the flags register.
    //

    //
    // Start the UART.
    //
    UARTEnable(ulBase);
}

//*****************************************************************************
//
//! Gets the current configuration of a UART.
//!
//! \return None.
//
//*****************************************************************************
void
UARTConfigGetExpClk(unsigned long ulBase, unsigned long *pulPreDivide, unsigned long *pulCD, unsigned long *pulBDIV, unsigned long *pulConfig)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    *pulPreDivide = (HWREG(ulBase + UART_MODE) & UART_MODE_SEL_CLOCK_UART_DIV_8);
    *pulCD = (HWREG(ulBase + UART_BAUD_RATE) & UART_Baudrate_CD_Msk);
    *pulBDIV = (HWREG(ulBase + UART_BAUD_DIV) & UART_Baudrate_BDIV_Msk);

    //
    // Get the parity, data length, and number of stop bits.
    //
    *pulConfig = (HWREG(ulBase + UART_MODE) & (UART_MODE_CHANNEL_MODE_BITS_Msk | UART_MODE_DATA_BITS_Msk | UART_MODE_PARITY_Msk | UART_MODE_STOP_BITS_Msk));
}

//*****************************************************************************
//
//! Disables transmitting and receiving.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function clears the UARTEN, TXE, and RXE bits, waits for the end of
//! transmission of the current character, and flushes the transmit FIFO.
//!
//! \return None.
//
//*****************************************************************************
void
UARTDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Wait for end of TX.
    //
//    while(!(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_TEMPTY))
//    {
//    }

    //
    // Disable and flush the FIFO.
    //

    //
    // Disable the UART.
    //
    HWREG(ulBase + UART_CONTROL) |= (UART_CTRL_TXDIS | UART_CTRL_RXDIS);
}

//*****************************************************************************
//
//! Determines if there are any characters in the receive FIFO.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function returns a flag indicating whether or not there is data
//! available in the receive FIFO.
//!
//! \return Returns \b true if there is data in the receive FIFO or \b false
//! if there is no data in the receive FIFO.
//
//*****************************************************************************
unsigned char
UARTCharsAvail(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return the availability of characters.
    //
    return((HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_REMPTY) ? false : true);
}

//*****************************************************************************
//
//! Determines if there is any space in the transmit FIFO.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function returns a flag indicating whether or not there is space
//! available in the transmit FIFO.
//!
//! \return Returns \b true if there is space available in the transmit FIFO
//! or \b false if there is no space available in the transmit FIFO.
//
//*****************************************************************************
unsigned char
UARTSpaceAvail(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return the availability of space.
    //
    return((HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_TFUL) ? false : true);
}

//*****************************************************************************
//
//! Receives a character from the specified port.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function gets a character from the receive FIFO for the specified
//! port.
//!
//! \return Returns the character read from the specified port, cast as a
//! \e long.  A \b -1 is returned if there are no characters present in the
//! receive FIFO.  The UARTCharsAvail() function should be called before
//! attempting to call this function.
//
//*****************************************************************************
long
UARTCharGetNonBlocking(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // See if there are any characters in the receive FIFO.
    //
    if(!(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_REMPTY))
    {
        //
        // Read and return the next character.
        //
        return(HWREG(ulBase + UART_FIFO_DATA));
    }
    else
    {
        //
        // There are no characters, so return a failure.
        //
        return(-1);
    }
}

//*****************************************************************************
//
//! Waits for a character from the specified port.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function gets a character from the receive FIFO for the specified
//! port.  If there are no characters available, this function waits until a
//! character is received before returning.
//!
//! \return Returns the character read from the specified port, cast as a
//! \e long.
//
//*****************************************************************************
long
UARTCharGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Wait until a char is available.
    //
    while(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_REMPTY)
    {
    }

    //
    // Now get the char.
    //
    return(HWREG(ulBase + UART_FIFO_DATA));
}

//*****************************************************************************
//
//! Waits for characters from the specified port.
//!
//*****************************************************************************
void
UARTNCharGet(unsigned long ulBase, unsigned char *pulRecvData, unsigned long ulLenByte)
{
    unsigned long i;
    
    for(i = 0; i < ulLenByte; i++)
    {
        while(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_REMPTY)
        {
        }
    
        pulRecvData[i] = HWREG(ulBase + UART_FIFO_DATA);
    }
}
#endif

//*****************************************************************************
//
//! Waits for N character from the specified port.
//!
//! \param ulBase is the base address of the UART port.
//!
//! \param pulRecvData is the pointer of memory to store received characters.
//!
//! \param ulLenByte is the expected number of characters to receive.
//!
//! \return Returns the character bytes read from the specified port, cast as a
//! \e long.
//
//*****************************************************************************
unsigned long UARTCharGetWithTimeout(unsigned long ulBase, unsigned char *pulRecvData, unsigned long ulLenByte)
{
    unsigned long ulRecvLenByte = 0;
    //volatile unsigned long reg;
    volatile unsigned long timeout;
    
    for(ulRecvLenByte = 0; ulRecvLenByte < ulLenByte; ulRecvLenByte++)
    {
        timeout = 0x400000;
        
        while(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_REMPTY)
        {
            timeout--;
            
            if(timeout < 0x10)
            {
                return ulRecvLenByte;
            }
        }
    
        pulRecvData[ulRecvLenByte] = HWREG(ulBase + UART_FIFO_DATA);
    }
    
    return ulRecvLenByte;
}

#if !USE_ROM_UART
//*****************************************************************************
//
//! Sends a character to the specified port.
//!
//! \param ulBase is the base address of the UART port.
//! \param ucData is the character to be transmitted.
//!
//! This function writes the character \e ucData to the transmit FIFO for the
//! specified port.  This function does not block, so if there is no space
//! available, then a \b false is returned, and the application must retry the
//! function later.
//!
//! \return Returns \b true if the character was successfully placed in the
//! transmit FIFO or \b false if there was no space available in the transmit
//! FIFO.
//
//*****************************************************************************
unsigned char
UARTCharPutNonBlocking(unsigned long ulBase, unsigned char ucData)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // See if there is space in the transmit FIFO.
    //
    if(!(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_TFUL))
    {
        //
        // Write this character to the transmit FIFO.
        //
        HWREG(ulBase + UART_FIFO_DATA) = ucData;

        //
        // Success.
        //
        return(true);
    }
    else
    {
        //
        // There is no space in the transmit FIFO, so return a failure.
        //
        return(false);
    }
}

//*****************************************************************************
//
//! Waits to send a character from the specified port.
//!
//! \param ulBase is the base address of the UART port.
//! \param ucData is the character to be transmitted.
//!
//! This function sends the character \e ucData to the transmit FIFO for the
//! specified port.  If there is no space available in the transmit FIFO, this
//! function waits until there is space available before returning.
//!
//! \return None.
//
//*****************************************************************************
void
UARTCharPut(unsigned long ulBase, unsigned char ucData)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Wait until space is available.
    //
    while(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_TFUL)
    {
    }

    //
    // Send the char.
    //
    HWREG(ulBase + UART_FIFO_DATA) = ucData;
}


//*****************************************************************************
//
//! Waits to send characters from the specified port.
//!
//! \return None.
//
//*****************************************************************************
void
UARTNCharPut(unsigned long ulBase, unsigned char *pucData, unsigned long ulLenByte)
{
    unsigned long i;
    
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    
    for(i = 0; i < ulLenByte; i++)
    {
        //
        // Wait until space is available.
        //
        while(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_TFUL)
        {
        }

        //
        // Send the char.
        //
        HWREG(ulBase + UART_FIFO_DATA) = pucData[i];
    }
}

//*****************************************************************************
//
//! Causes a BREAK to be sent.
//!
//! \param ulBase is the base address of the UART port.
//! \param bBreakState controls the output level.
//!
//! Calling this function with \e bBreakState set to \b true asserts a break
//! condition on the UART.  Calling this function with \e bBreakState set to
//! \b false removes the break condition.  For proper transmission of a break
//! command, the break must be asserted for at least two complete frames.
//!
//! \return None.
//
//*****************************************************************************
void
UARTBreakCtl(unsigned long ulBase, unsigned char bBreakState)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Set the break condition as requested.
    //
    HWREG(ulBase + UART_CONTROL) =
				 (bBreakState ?
         (HWREG(ulBase + UART_CONTROL) | UART_CONTROL_BREAK_START):
         (HWREG(ulBase + UART_CONTROL) | UART_CONTROL_BREAK_STOP));
}

//*****************************************************************************
//
//! Registers an interrupt handler for a UART interrupt.
//!
//! \param ulBase is the base address of the UART port.
//! \param pfnHandler is a pointer to the function to be called when the
//! UART interrupt occurs.
//!
//! This function does the actual registering of the interrupt handler.  This
//! function enables the global interrupt in the interrupt controller; specific
//! UART interrupts must be enabled via UARTIntEnable().  It is the interrupt
//! handler's responsibility to clear the interrupt source.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntRegister(unsigned long ulBase, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
#ifdef PLATFORM_ARM_M3
    unsigned long ulInt;

    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Determine the interrupt number based on the UART port.
    //
    ulInt = INT_UART1;

    //
    // Register the interrupt handler.
    //
    IntRegister(ulInt, g_pRAMVectors, pfnHandler);

    //
    // Enable the UART interrupt.
    //
    IntEnable(ulInt);
#endif	
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for a UART interrupt.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function does the actual unregistering of the interrupt handler.  It
//! clears the handler to be called when a UART interrupt occurs.  This
//! function also masks off the interrupt in the interrupt controller so that
//! the interrupt handler no longer is called.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntUnregister(unsigned long ulBase, unsigned long *g_pRAMVectors)
{
#ifdef PLATFORM_ARM_M3
    unsigned long ulInt;

    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Determine the interrupt number based on the UART port.
    //
    ulInt = INT_UART1;

    //
    // Disable the interrupt.
    //
    IntDisable(ulInt);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(ulInt, g_pRAMVectors);
#endif	
}

//*****************************************************************************
//
//! Enables individual UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is the bit mask of the interrupt sources to be enabled.
//!
//! This function enables the indicated UART interrupt sources.  Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! The \e ulIntFlags parameter is the logical OR of any of the following:
//!
//! - \b UART_INT_TOVR     0x1000 // The Transmitter FIFO Overflow interrupt is enabled.
//! - \b UART_INT_TNFUL    0x0800 // The Transmitter FIFO Nearly Full interrupt is enabled.
//! - \b UART_INT_TTRIG    0x0400 // The Transmitter FIFO Trigger interrupt is enabled.
//! - \b UART_INT_DMSI     0x0200 // The Delta Modem Status Indicator interrupt is enabled.
//! - \b UART_INT_TIMEOUT  0x0100 // The Receiver Timeout Error interrupt is enabled.
//! - \b UART_INT_PARE     0x0080 // The Receiver Parity Error interrupt is enabled.
//! - \b UART_INT_FRAME    0x0040 // The Receiver Framing Error interrupt is enabled.
//! - \b UART_INT_ROVR     0x0020 // The Receiver Overflow Error interrupt is enabled.
//! - \b UART_INT_TFUL     0x0010 // The Transmitter FIFO Full interrupt is enabled.
//! - \b UART_INT_TEMPTY   0x0008 // The Transmitter FIFO Empty interrupt is enabled.
//! - \b UART_INT_RFUL     0x0004 // The Receiver FIFO Full interrupt is enabled.
//! - \b UART_INT_REMPTY   0x0002 // The Receiver FIFO Empty interrupt is enabled.
//! - \b UART_INT_RTRIG    0x0001 // The Receiver FIFO Trigger interrupt is enabled.
//! - \b UART_INT_ALL      0x1FFF 
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Enable the specified interrupts.
    //
    HWREG(ulBase + UART_INT_ENA) |= ulIntFlags;
}

//*****************************************************************************
//
//! Disables individual UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is the bit mask of the interrupt sources to be disabled.
//!
//! This function disables the indicated UART interrupt sources.  Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! The \e ulIntFlags parameter has the same definition as the \e ulIntFlags
//! parameter to UARTIntEnable().
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Disable the specified interrupts.
    //
    HWREG(ulBase + UART_INT_DIS) |= ulIntFlags;
}

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \param ulBase is the base address of the UART port.
//! \param bMasked is \b false if the raw interrupt status is required and
//! \b true if the masked interrupt status is required.
//!
//! This function returns the interrupt status for the specified UART.  Either
//! the raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \return Returns the current interrupt status, enumerated as a bit field of
//! values described in UARTIntEnable().
//
//*****************************************************************************
unsigned long
UARTIntStatus(unsigned long ulBase, unsigned char bMasked)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(HWREG(ulBase + UART_INT_MASK));
    }
    else
    {
        return(HWREG(ulBase + UART_INT_STAT));
    }
}

//*****************************************************************************
//
//! Clears UART interrupt sources.
//!
//! \param ulBase is the base address of the UART port.
//! \param ulIntFlags is a bit mask of the interrupt sources to be cleared.
//!
//! The specified UART interrupt sources are cleared, so that they no longer
//! xy_assert.  This function must be called in the interrupt handler to keep the
//! interrupt from being recognized again immediately upon exit.
//!
//! The \e ulIntFlags parameter has the same definition as the \e ulIntFlags
//! parameter to UARTIntEnable().
//!
//! \note Because there is a write buffer in the Cortex-M3 processor, it may
//! take several clock cycles before the interrupt source is actually cleared.
//! Therefore, it is recommended that the interrupt source be cleared early in
//! the interrupt handler (as opposed to the very last action) to avoid
//! returning from the interrupt handler before the interrupt source is
//! actually cleared.  Failure to do so may result in the interrupt handler
//! being immediately reentered (because the interrupt controller still sees
//! the interrupt source asserted).
//!
//! \return None.
//
//*****************************************************************************
void
UARTIntClear(unsigned long ulBase)
{
    volatile unsigned long ulReg;
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Clear the requested interrupt sources. Read and Clear.
    //
    ulReg = HWREG(ulBase + UART_INT_STAT);
}

//*****************************************************************************
//
//! Gets current receiver errors.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function returns the current state of each of the 4 receiver error
//! sources.  The returned errors are equivalent to the four error bits
//! returned via the previous call to UARTCharGet() or UARTCharGetNonBlocking()
//! with the exception that the overrun error is set immediately the overrun
//! occurs rather than when a character is next read.
//!
//! \return Returns a logical OR combination of the receiver error flags,
//! \b UART_RXERROR_FRAMING, \b UART_RXERROR_PARITY, \b UART_RXERROR_BREAK
//! and \b UART_RXERROR_OVERRUN.
//
//*****************************************************************************
unsigned long
UARTRxErrorGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Return the current value of the receive status register.
    //
    return(HWREG(ulBase + UART_CHAN_STAT) & (UART_CSR_TIMEOUT | UART_CSR_PARE | UART_CSR_FRAME | UART_CSR_ROVR));
}

//*****************************************************************************
//
//! Clears all reported receiver errors.
//!
//! \param ulBase is the base address of the UART port.
//!
//! This function is used to clear all receiver error conditions reported via
//! UARTRxErrorGet().  If using the overrun, framing error, parity error or
//! break interrupts, this function must be called after clearing the interrupt
//! to ensure that later errors of the same type trigger another interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
UARTRxErrorClear(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));

    //
    // Any read to the Error Clear Register will clear all bits which are
    // currently set.
    //
    UARTRxErrorGet(ulBase);
}

//*****************************************************************************
//
//! Send string from specific UART port.
//!
//! \param ulBase is the base address of the UART port.
//! \param str is the point of string.
//!
//! \return None.
//
//*****************************************************************************
void
UARTStringPut(unsigned long ulBase, unsigned char *str)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    
    //
    // Puts out the input string byte by byte.
    //
  	while(*str != 0)
	{
   		UARTCharPut(ulBase, *str++);
	}
}

#endif

//*****************************************************************************
//
//! Sets the configuration of a UART.
//!
//! \return None.
//
//*****************************************************************************
void
UARTConfigSetBaudrate(unsigned long ulBase, unsigned long ulPclk, unsigned long ulBaudrate, unsigned long ulConfig)
{
    unsigned long BDIV;
    unsigned long CD;
    unsigned long diff;
    unsigned long minBDIV = 3;
    unsigned long minDiff = 0x0FFFFFFF;
	unsigned long ulPclkCalculate;
    
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    
    //
    // Calculate the best BDIV and CD.
    //
    //for(BDIV = 4; BDIV < 255; BDIV++)
	for(BDIV = 254; BDIV > 3; BDIV--)
    {
        CD = ulPclk / (ulBaudrate * (BDIV + 1));
        
        if(CD == 0 || CD >= 0x10000 || (BDIV + 1)*CD < 16)
        {
            continue;
        }

		ulPclkCalculate = ulBaudrate * (BDIV + 1) * CD;
		
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
            minBDIV = BDIV;
        }

		if(diff == 0)
		{
			break;
		}
    }
    
    //
    // Bad case: There is no suitable BDIV and CD.
    //
    if(minDiff == 0x0FFFFFFF)
    {
        // HORSE_NOTE
    }

    //
    // Stop the UART.
    //
    UARTDisable(ulBase);

    //
    // Set the baud rate.
    //
    CD = ulPclk / (ulBaudrate * (minBDIV + 1));
    HWREG(ulBase + UART_BAUD_RATE) = (CD & UART_Baudrate_CD_Msk);
    HWREG(ulBase + UART_BAUD_DIV)  = (minBDIV & UART_Baudrate_BDIV_Msk);

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(ulBase + UART_MODE) = ((HWREG(ulBase + UART_MODE) & ~(UART_MODE_CHANNEL_MODE_BITS_Msk | UART_MODE_DATA_BITS_Msk | UART_MODE_PARITY_Msk | UART_MODE_STOP_BITS_Msk)) | ulConfig);

    //
    // Start the UART.
    //
    UARTEnable(ulBase);
}


#if !USE_ROM_UART

//*****************************************************************************
//
//! UART Baudrate Auto Detection.
//!
//! \return None.
//
//*****************************************************************************
void
UARTAutoBaudrate(unsigned long ulBase)
{
    unsigned long CD = 1;
    unsigned long BDIV = 7;
    unsigned long abd_high = 0;
    unsigned long abd_low = 0;
    unsigned long abd_baudrate = 0;
    unsigned long diff;
    unsigned long minBDIV;
    unsigned long minDiff = 0x0FFFFFFF;
    
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    
    //
    // Stop the UART.
    //
    UARTDisable(ulBase);
    
    //
    // Set the reference baud rate: sel_clk / (CD * (BDIV + 1)).
    //
    HWREG(ulBase + UART_BAUD_RATE) = (CD & UART_Baudrate_CD_Msk);
    HWREG(ulBase + UART_BAUD_DIV)  = (BDIV & UART_Baudrate_BDIV_Msk);

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(ulBase + UART_MODE) = UART_MODE_DATA_BITS_8 | UART_MODE_PARITY_NONE | UART_MODE_STOP_BITS_1;
    
    //
    // Start Auto-Detection.
    //
    HWREG(ulBase + UART_ABD_CTRL) = 0x1;
    HWREG(ulBase + UART_ABD_CTRL) = 0x1;
    HWREG(ulBase + UART_ABD_CTRL) = 0x1;
    HWREG(ulBase + UART_ABD_CTRL) = 0x1;
    
    //
    // Wait untile Auto-Detection finished.
    //
    while((HWREG(ulBase + UART_ABD_STAT) & 0x1) != 0x1);
    
    //
    // Read Detected Value.
    //
    abd_high = (HWREG(ulBase + UART_ABD_TIMERH) & 0xFFFF);
    abd_low  = (HWREG(ulBase + UART_ABD_TIMERL) & 0xFFFF);
    
    abd_baudrate = (abd_high << 16) | abd_low;
    
    //
    // Calculate the best BDIV and CD.
    //
    for(BDIV = 4; BDIV < 255; BDIV++)
    {
        CD = abd_baudrate / (BDIV + 1);
        
        if(CD == 0 || CD >= 0x10000)
        {
            continue;
        }
        
        diff = abd_baudrate - CD * (BDIV + 1);
        
        if(diff < minDiff)
        {
            minDiff = diff;
            minBDIV = BDIV;
        }
    }
    
    //
    // Set the reference baud rate: sel_clk / (CD * (BDIV + 1)).
    //
    CD = abd_baudrate / (minBDIV + 1); 
    HWREG(ulBase + UART_BAUD_RATE) = (CD & UART_Baudrate_CD_Msk);
    HWREG(ulBase + UART_BAUD_DIV)  = (minBDIV & UART_Baudrate_BDIV_Msk);
}

//*****************************************************************************
//
//! Sets the configuration of a UART.
//!
//! \return None.
//
//*****************************************************************************
void
UARTConfigSetMode(unsigned long ulBase, unsigned long ulConfig)
{
    //
    // Check the arguments.
    //
    ASSERT(UARTBaseValid(ulBase));
    
    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(ulBase + UART_MODE) = ((HWREG(ulBase + UART_MODE) & ~(UART_MODE_CHANNEL_MODE_BITS_Msk | UART_MODE_DATA_BITS_Msk | UART_MODE_PARITY_Msk | UART_MODE_STOP_BITS_Msk)) | ulConfig);
}

void UARTWaitTxDone(unsigned long ulBase)
{
    while(!(HWREG(ulBase + UART_CHAN_STAT) & UART_CSR_TEMPTY));
}
#endif

// ========================== For softap_printf ===============================
void printch2(char ch)
{
    UARTCharPut(UART1_BASE, ch);
}

#if !USE_ROM_UART
void static printch(unsigned long ulBase, char ch)
{
    if(ulBase == UART1_BASE)
    {
        UARTCharPut(UART1_BASE, ch);
    }
    else if(ulBase == CSP1_BASE || ulBase == CSP1_BASE || ulBase == CSP2_BASE || ulBase == CSP3_BASE)
    {
        CSPCharPut(ulBase, ch);
    }
}

void static printdec(unsigned long ulBase, int dec)
{
    if(dec==0)
    {
       return;
    }
    printdec(ulBase, dec/10);
    printch(ulBase, (char)(dec%10 + '0'));
}

void static printflt(unsigned long ulBase, double flt)
{
    int tmpint = 0;
    
    tmpint = (int)flt;
    printdec(ulBase, tmpint);
    printch(ulBase, '.');
    flt = flt - tmpint;
    tmpint = (int)(flt * 1000000);
    printdec(ulBase, tmpint);
}

void static printstr(unsigned long ulBase, char* str)
{
    while(*str)
    {
        printch(ulBase, *str++);
    }
}

void static printbin(unsigned long ulBase, int bin)
{
    if(bin == 0)
    {
        printstr(ulBase, "0b");
        return;
    }
    printbin(ulBase, bin/2);
    printch(ulBase, (char)(bin%2 + '0'));
}

void static printhex(unsigned long ulBase, int hex)
{
    if(hex < 0)
    {
        hex = (int)(~hex) + 1;
        printstr(ulBase, "-");
        printhex(ulBase, hex);
        
        return;
    }
    
    if(hex==0)
    {
        printstr(ulBase, "0x");
        return;
    }
    printhex(ulBase, hex/16);
    if(hex%16 < 10)
    {
        printch(ulBase, (char)(hex%16 + '0'));
    }
    else
    {
        printch(ulBase, (char)(hex%16 - 10 + 'a' ));
    }
}

void UARTPrintf(unsigned long ulBase, char* fmt, ...)
{
    double vargflt = 0;
    int  vargint = 0;
    char* vargpch = NULL;
    char vargch = 0;
    char* pfmt = NULL;
    va_list vp;
    va_start(vp, fmt);
    pfmt = fmt;

    while(*pfmt)
    {
        if(*pfmt == '%')
        {
            switch(*(++pfmt))
            {
                case 'c':
                    vargch = va_arg(vp, int);
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                        mode, or a warning otherwise.In non-strict ANSI mode, 'type' is allowed to be any expression. */
                    printch(ulBase, vargch);
                    break;
                
                case 'd':
                case 'i':
                	vargint = va_arg(vp, int);
                    if(vargint ==0)
                    	printch(ulBase, vargint);
                    else
                    	printdec(ulBase, vargint);
                    break;
                    
                case 'f':
                    vargflt = va_arg(vp, double);
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                        mode, or a warning otherwise.In non-strict ANSI mode, 'type' is allowed to be any expression. */
                    printflt(ulBase, vargflt);
                    break;
                
                case 's':
                    vargpch = va_arg(vp, char*);
                    printstr(ulBase, vargpch);
                    break;
                
                case 'b':
                case 'B':
                    vargint = va_arg(vp, int);
                    printbin(ulBase, vargint);
                    break;
                
                case 'x':
                case 'X':
                    vargint = va_arg(vp, int);
                    printhex(ulBase, vargint);
                    break;
                
                case '%':
                    printch(ulBase, '%');
                    break;
                
                default:
                    break;
            }
            pfmt++;
        }
        else
        {
            printch(ulBase, *pfmt++);
        }
    }
    va_end(vp);
}
#endif

