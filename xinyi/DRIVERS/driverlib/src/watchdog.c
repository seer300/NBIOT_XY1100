#include "watchdog.h"

//*****************************************************************************
//
//! Determines if the watchdog timer is enabled.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! This will check to see if the watchdog timer is enabled.
//!
//! \return Returns \b true if the watchdog timer is enabled, and \b false
//! if it is not.
//
//*****************************************************************************
unsigned char
WatchdogRunning(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // See if the watchdog timer module is enabled, and return.
    //
    return(HWREG(ulBase + WDT_CTL) & WDT_CTL_WATCHDOG_EN);
}

//*****************************************************************************
//
//! Enables the watchdog timer.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! This will enable the watchdog timer counter and interrupt.
//!
//! \note This function will have no effect if the watchdog timer has
//! been locked.
//!
//! \sa WatchdogLock(), WatchdogUnlock()
//!
//! \return None.
//
//*****************************************************************************
void
WatchdogEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Enable the watchdog timer module.
    //
    HWREG(ulBase + WDT_CTL) |= WDT_CTL_WATCHDOG_EN;
}

//*****************************************************************************
//
//! Enables the watchdog timer reset.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! Enables the capability of the watchdog timer to issue a reset to the
//! processor upon a second timeout condition.
//!
//! \note This function will have no effect if the watchdog timer has
//! been locked.
//!
//! \sa WatchdogLock(), WatchdogUnlock()
//!
//! \return None.
//
//*****************************************************************************
void
WatchdogResetEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Enable the watchdog reset.
    //
    HWREG(ulBase + WDT_CTL) |= WDT_CTL_RST_EN;
}

//*****************************************************************************
//
//! Disables the watchdog timer reset.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! Disables the capability of the watchdog timer to issue a reset to the
//! processor upon a second timeout condition.
//!
//! \return None.
//
//*****************************************************************************
void
WatchdogResetDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Disable the watchdog reset.
    //
    HWREG(ulBase + WDT_CTL) &= ~(WDT_CTL_RST_EN);
}


void
WatchdogTimerRepeatEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Enable the watchdog reset.
    //
    HWREG(ulBase + WDT_CTL) |= WDT_CTL_COUNTER_EN;
}

void
WatchdogTimerRepeatDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Disable the watchdog reset.
    //
    HWREG(ulBase + WDT_CTL) &= ~(WDT_CTL_COUNTER_EN);
}

//*****************************************************************************
//
//! Sets the watchdog timer reload value.
//!
//! \param ulBase is the base address of the watchdog timer module.
//! \param ulLoadVal is the load value for the watchdog timer.
//!
//! This function sets the value to load into the watchdog timer when the count
//! reaches zero for the first time; if the watchdog timer is running when this
//! function is called, then the value will be immediately loaded into the
//! watchdog timer counter.  If the \e ulLoadVal parameter is 0, then an
//! interrupt is immediately generated.
//!
//! \note This function will have no effect if the watchdog timer has
//! been locked.
//!
//! \sa WatchdogLock(), WatchdogUnlock(), WatchdogReloadGet()
//!
//! \return None.
//
//*****************************************************************************
void
WatchdogReloadSet(unsigned long ulBase, unsigned long ulLoadVal)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Set the load register.
    //
    HWREG(ulBase + WDT_DAT) = ulLoadVal;
}

//*****************************************************************************
//
//! Gets the current watchdog timer value.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! This function reads the current value of the watchdog timer.
//!
//! \return Returns the current value of the watchdog timer.
//
//*****************************************************************************
unsigned long
WatchdogValueGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Get the current watchdog timer register value.
    //
    return(HWREG(ulBase + WDT_DAT));
}

#if !USE_ROM_WDT
//*****************************************************************************
//
//! Registers an interrupt handler for watchdog timer interrupt.
//!
//! \param ulBase is the base address of the watchdog timer module.
//! \param pfnHandler is a pointer to the function to be called when the
//! watchdog timer interrupt occurs.
//!
//! This function does the actual registering of the interrupt handler.  This
//! will enable the global interrupt in the interrupt controller; the watchdog
//! timer interrupt must be enabled via WatchdogEnable().  It is the interrupt
//! handler's responsibility to clear the interrupt source via
//! WatchdogIntClear().
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
WatchdogIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    //
    // Check the arguments.
    //
//    ASSERT(ulBase == WDT_BASE);

    //
    // Register the interrupt handler.
    //
    IntRegister(INT_WDT, g_pRAMVectors, pfnHandler);

    //
    // Enable the watchdog timer interrupt.
    //
    IntEnable(INT_WDT);
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for the watchdog timer interrupt.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! This function does the actual unregistering of the interrupt handler.  This
//! function will clear the handler to be called when a watchdog timer
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
WatchdogIntUnregister(unsigned long *g_pRAMVectors)
{
    //
    // Check the arguments.
    //
//    ASSERT(ulBase == WDT_BASE);

    //
    // Disable the interrupt.
    //
    IntDisable(INT_WDT);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(INT_WDT, g_pRAMVectors);
}

//*****************************************************************************
//
//! Enables the watchdog timer interrupt.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! Enables the watchdog timer interrupt.
//!
//! \note This function will have no effect if the watchdog timer has
//! been locked.
//!
//! \sa WatchdogLock(), WatchdogUnlock(), WatchdogEnable()
//!
//! \return None.
//
//*****************************************************************************
void
WatchdogIntEnable(void)
{
    //
    // Check the arguments.
    //
//    ASSERT(ulBase == WDT_BASE);

    //
    // Enable the watchdog interrupt.
    //
    HWREG(WDT_BASE + WDT_CTL) |= WDT_CTL_INT_EN;
}

#endif

void
WatchdogIntDisable(void)
{
    //
    // Check the arguments.
    //
//    ASSERT(ulBase == WDT_BASE);

    //
    // Enable the watchdog interrupt.
    //
    HWREG(WDT_BASE + WDT_CTL) &= ~(WDT_CTL_INT_EN);
}


//*****************************************************************************
//
//! Gets the current watchdog timer interrupt status.
//!
//! \param ulBase is the base address of the watchdog timer module.
//! \param bMasked is \b false if the raw interrupt status is required and
//! \b true if the masked interrupt status is required.
//!
//! This returns the interrupt status for the watchdog timer module.  Either
//! the raw interrupt status or the status of interrupt that is allowed to
//! reflect to the processor can be returned.
//!
//! \return Returns the current interrupt status, where a 1 indicates that the
//! watchdog interrupt is active, and a 0 indicates that it is not active.
//
//*****************************************************************************
unsigned long
WatchdogIntStatus(unsigned long ulBase, unsigned char bMasked)
{
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return((HWREG(ulBase + WDT_CTL)) & WDT_CTL_INT_EN);
    }
    else
    {
        return(HWREG(ulBase + WDT_INT_STAT));
    }
}

//*****************************************************************************
//
//! Clears the watchdog timer interrupt.
//!
//! \param ulBase is the base address of the watchdog timer module.
//!
//! The watchdog timer interrupt source is cleared, so that it no longer
//! asserts.
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
WatchdogIntClear(unsigned long ulBase)
{
    volatile unsigned long ulReg;
    //
    // Check the arguments.
    //
    ASSERT(ulBase == WDT_BASE);

    //
    // Clear the interrupt source.
    //
    ulReg = HWREG(ulBase + WDT_INT_STAT);

    (void) ulReg;
}

//*****************************************************************************


void WatchdogDisable(unsigned long ulBase)
{
    HWREG(ulBase + WDT_CTL) &= (~WDT_CTL_WATCHDOG_EN);
}
