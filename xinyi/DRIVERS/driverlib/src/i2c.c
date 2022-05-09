#include "xy_utils.h"
#include "i2c.h"

#if !USE_ROM_I2C

//*****************************************************************************
//
//! Initializes the I2C Master block.
//!
//! \param ulBase is the base address of the I2C Master module.
//! \param ulI2CClk is the rate of the clock supplied to the I2C module.
//! \param bFast set up for fast data transfers
//!
//! This function initializes operation of the I2C Master block.  Upon
//! successful initialization of the I2C block, this function will have set the
//! bus speed for the master, and will have enabled the I2C Master block.
//!
//! If the parameter \e bFast is \b true, then the master block will be set up
//! to transfer data at 400 kbps; otherwise, it will be set up to transfer data
//! at 100 kbps.
//!
//! The peripheral clock will be the same as the processor clock.  This will be
//! the value returned by SysCtlClockGet(), or it can be explicitly hard coded
//! if it is constant and known (to save the code/execution overhead of a call
//! to SysCtlClockGet()).
//!
//! This function replaces the original I2CMasterInit() API and performs the
//! same actions.  A macro is provided in <tt>i2c.h</tt> to map the original
//! API to this API.
//!
//! \return None.
//
//*****************************************************************************
void
I2CMasterInitExpClk(unsigned long ulBase, unsigned long ulI2CClk,
                    unsigned char bFast)
{
    unsigned long ulSCLFreq;
    unsigned long ulDiv;
    unsigned long ulDiff;
    unsigned long ulMinDiff = 0x0FFFFFFF;
    unsigned char ucDiva, ucDivb, ucMinDiva;
    
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Must enable the device before doing anything else.
    //
    I2CMasterEnable(ulBase);

    //
    // Get the desired SCL speed.
    //
    if(bFast == true)
    {
        ulSCLFreq = 400000;
    }
    else
    {
        ulSCLFreq = 100000;
    }

    //
    //
    ulDiv = ulI2CClk / (22 * ulSCLFreq);
    
    for(ucDiva = 0; ucDiva < 4; ucDiva++)
    {
        ucDivb = ulDiv / (ucDiva + 1);
        
        if(ucDivb == 0 || ucDivb > 64)
        {
            continue;
        }
        
        ulDiff = ulDiv - ucDivb * (ucDiva + 1);
        
        if(ulDiff < ulMinDiff)
        {
            ulMinDiff = ulDiff;
            ucMinDiva = ucDiva;
        }
    }
    
    ucDivb = ulDiv / (ucMinDiva + 1);
    
    HWREG(ulBase + I2C_CTL) = (HWREG(ulBase + I2C_CTL) & (~I2C_CTL_DIVISOR_Msk)) | (ucMinDiva << I2C_CTL_DIVISOR_A_Pos) | ((ucDivb << I2C_CTL_DIVISOR_B_Pos));
}

//*****************************************************************************
//
//! Initializes the I2C Slave block.
//!
//! \param ulBase is the base address of the I2C Slave module.
//! \param ucSlaveAddr 7-bit slave address
//!
//! This function initializes operation of the I2C Slave block.  Upon
//! successful initialization of the I2C blocks, this function will have set
//! the slave address and have enabled the I2C Slave block.
//!
//! The parameter \e ucSlaveAddr is the value that will be compared against the
//! slave address sent by an I2C master.
//!
//! \return None.
//
//*****************************************************************************
void
I2CSlaveInit(unsigned long ulBase, unsigned long ucSlaveAddr)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Must enable the device before doing anything else.
    //
    I2CSlaveEnable(ulBase);

    //
    // Set up the slave address.
    //
    HWREG(ulBase + I2C_ADDR) = ucSlaveAddr;
}

//*****************************************************************************
//
//! Enables the I2C Master block.
//!
//! \param ulBase is the base address of the I2C Master module.
//!
//! This will enable operation of the I2C Master block.
//!
//! \return None.
//
//*****************************************************************************
void
I2CMasterEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Enable the master block.
    //
    HWREG(ulBase + I2C_CTL) |= I2C_CTL_MASTER;
}

//*****************************************************************************
//
//! Enables the I2C Slave block.
//!
//! \param ulBase is the base address of the I2C Slave module.
//!
//! This will enable operation of the I2C Slave block.
//!
//! \return None.
//
//*****************************************************************************
void
I2CSlaveEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Enable the clock to the slave block.
    //

    //
    // Enable the slave.
    //
    HWREG(ulBase + I2C_CTL) &= (~I2C_CTL_MASTER);
}


//*****************************************************************************
//
//! Registers an interrupt handler for the I2C module.
//!
//! \param ulBase is the base address of the I2C Master module.
//! \param pfnHandler is a pointer to the function to be called when the
//! I2C interrupt occurs.
//!
//! This sets the handler to be called when an I2C interrupt occurs.  This will
//! enable the global interrupt in the interrupt controller; specific I2C
//! interrupts must be enabled via I2CMasterIntEnable() and
//! I2CSlaveIntEnable().  If necessary, it is the interrupt handler's
//! responsibility to clear the interrupt source via I2CMasterIntClear() and
//! I2CSlaveIntClear().
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
I2CIntRegister(unsigned long ulBase, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    unsigned long ulInt;

    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Determine the interrupt number based on the I2C port.
    //
    ulInt = (ulBase == I2C1_BASE) ? INT_I2C1 : INT_I2C2;

    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    IntRegister(ulInt, g_pRAMVectors, pfnHandler);

    //
    // Enable the I2C interrupt.
    //
    IntEnable(ulInt);
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for the I2C module.
//!
//! \param ulBase is the base address of the I2C Master module.
//!
//! This function will clear the handler to be called when an I2C interrupt
//! occurs.  This will also mask off the interrupt in the interrupt controller
//! so that the interrupt handler no longer is called.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
I2CIntUnregister(unsigned long ulBase, unsigned long *g_pRAMVectors)
{
    unsigned long ulInt;

    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Determine the interrupt number based on the I2C port.
    //
    ulInt = (ulBase == I2C1_BASE) ? INT_I2C1 : INT_I2C2;

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
//! Enables the I2C Master interrupt.
//!
//! \param ulBase is the base address of the I2C Master module.
//!
//! Enables the I2C Master interrupt source.
//!
//! \return None.
//
//*****************************************************************************
void
I2CIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Enable the master interrupt.
    //
    HWREG(ulBase + I2C_INT_EN) |= ulIntFlags;
}


//*****************************************************************************
//
//! Disables the I2C Master interrupt.
//!
//! \param ulBase is the base address of the I2C Master module.
//!
//! Disables the I2C Master interrupt source.
//!
//! \return None.
//
//*****************************************************************************
void
I2CIntDisable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Disable the master interrupt.
    //
    HWREG(ulBase + I2C_INT_DIS) |= ulIntFlags;
}


//*****************************************************************************
//
//! Gets the current I2C Master interrupt status.
//!
//! \param ulBase is the base address of the I2C Master module.
//! \param bMasked is false if the raw interrupt status is requested and
//! true if the masked interrupt status is requested.
//!
//! This returns the interrupt status for the I2C Master module.  Either the
//! raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \return The current interrupt status, returned as \b true if active
//! or \b false if not active.
//
//*****************************************************************************
unsigned char
I2CIntStatus(unsigned long ulBase, unsigned char bMasked)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(HWREG(ulBase + I2C_INT_MASK));
    }
    else
    {
        return(HWREG(ulBase + I2C_INT_STAT));
    }
}

//*****************************************************************************
//
//! Clears I2C Master interrupt sources.
//!
//! \param ulBase is the base address of the I2C Master module.
//!
//! The I2C Master interrupt source is cleared, so that it no longer asserts.
//! This must be done in the interrupt handler to keep it from being called
//! again immediately upon exit.
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
I2CIntClear(unsigned long ulBase)
{
    volatile unsigned long ulReg;
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Clear the I2C master interrupt source.
    //
    ulReg = HWREG(ulBase + I2C_INT_STAT);
}


//*****************************************************************************
//
//!
//! \return None.
//
//*****************************************************************************
void
I2CAddrSet(unsigned long ulBase, unsigned long ulAddr, unsigned char ucNorMode, unsigned char ucReceive)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));
    
    if(ucNorMode)
    {
        HWREG(ulBase + I2C_CTL) |= I2C_CTL_NEA_N;
        ulAddr &= 0x7F;
    }
    else
    {
        HWREG(ulBase + I2C_CTL) &= (~I2C_CTL_NEA_N);
        ulAddr &= 0x3FF;
    }
    
    if(ucReceive)
    {
        HWREG(ulBase + I2C_CTL) |= I2C_CTL_M_READ;
    }
    else
    {
        HWREG(ulBase + I2C_CTL) &= (~I2C_CTL_M_READ);
    }
    
    HWREG(ulBase + I2C_ADDR) = ulAddr;
}


//*****************************************************************************
//
//! Indicates whether or not the I2C bus is busy.
//!
//! \param ulBase is the base address of the I2C Master module.
//!
//! This function returns an indication of whether or not the I2C bus is busy.
//! This function can be used in a multi-master environment to determine if
//! another master is currently using the bus.
//!
//! \return Returns \b true if the I2C bus is busy; otherwise, returns
//! \b false.
//
//*****************************************************************************
unsigned char
I2CBusBusy(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Return the bus busy status.
    //
    if(HWREG(ulBase + I2C_STAT) & I2C_STAT_BA_Msk)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

//*****************************************************************************
//
//! Controls the state of the I2C module.
//!
//! \param ulBase is the base address of the I2C module.
//! \param ulCmd command to be issued to the I2C module
//!
//! This function is used to control the state of the module send and
//! receive operations.  The \e ucCmd parameter can be one of the following
//! values:
//!
//! \return None.
//
//*****************************************************************************
void
I2CControlSet(unsigned long ulBase, unsigned long ulCmd)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Send the command.
    //
    HWREG(ulBase + I2C_CTL) = ulCmd;
}

//*****************************************************************************
//
//! Get the state of the I2C module.
//!
//*****************************************************************************
unsigned long
I2CControlGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Send the command.
    //
    return (HWREG(ulBase + I2C_CTL));
}

//*****************************************************************************
//
//! Transmits a byte from the I2C Master.
//!
//! \param ulBase is the base address of the I2C Master module.
//! \param ucData data to be transmitted from the I2C Master
//!
//! This function will place the supplied data into I2C Master Data Register.
//!
//! \return None.
//
//*****************************************************************************
void
I2CDataPut(unsigned long ulBase, unsigned char ucData)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Write the byte.
    //
    HWREG(ulBase + I2C_DATA) = ucData;
}

//*****************************************************************************
//
//! Receives a byte that has been sent to the I2C Master.
//!
//! \param ulBase is the base address of the I2C Master module.
//!
//! This function reads a byte of data from the I2C Master Data Register.
//!
//! \return Returns the byte received from by the I2C Master, cast as an
//! unsigned long.
//
//*****************************************************************************
unsigned long
I2CDataGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Read a byte.
    //
    return(HWREG(ulBase + I2C_DATA));
}

//*****************************************************************************
//
//! Gets the I2C Slave module status
//!
//! \param ulBase is the base address of the I2C Slave module.
//!
//! This function will return the action requested from a master, if any.
//! Possible values are:
//!
//! - \b I2C_SLAVE_ACT_NONE
//! - \b I2C_SLAVE_ACT_RREQ
//! - \b I2C_SLAVE_ACT_TREQ
//! - \b I2C_SLAVE_ACT_RREQ_FBR
//!
//! \return Returns \b I2C_SLAVE_ACT_NONE to indicate that no action has been
//! requested of the I2C Slave module, \b I2C_SLAVE_ACT_RREQ to indicate that
//! an I2C master has sent data to the I2C Slave module, \b I2C_SLAVE_ACT_TREQ
//! to indicate that an I2C master has requested that the I2C Slave module send
//! data, and \b I2C_SLAVE_ACT_RREQ_FBR to indicate that an I2C master has sent
//! data to the I2C slave and the first byte following the slave's own address
//! has been received.
//
//*****************************************************************************
unsigned long
I2CStatus(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));

    //
    // Return the slave status.
    //
    return(HWREG(ulBase + I2C_STAT));
}

void
I2CACKModeSet(unsigned long ulBase, unsigned char ucEnable)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));
    
    if(ucEnable)
    {
        HWREG(ulBase + I2C_CTL) |= I2C_CTL_ACKEN_EN;
    }
    else
    {
        HWREG(ulBase + I2C_CTL) &= (~I2C_CTL_ACKEN_EN);
    }
}

void
I2CFifoClear(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT((ulBase == I2C1_BASE) || (ulBase == I2C2_BASE));
    
    HWREG(ulBase + I2C_CTL) |= I2C_CTL_CLR_FIFO_EN;
}

#endif

//*****************************************************************************
