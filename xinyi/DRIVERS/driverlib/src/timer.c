#include "timer.h"

//*****************************************************************************
//
//! \internal
//! Checks a timer base address.
//!
//! \param ulBase is the base address of the timer module.
//!
//! This function determines if a timer module base address is valid.
//!
//! \return Returns \b true if the base address is valid and \b false
//! otherwise.
//
//*****************************************************************************
#ifdef DEBUG
static unsigned char
TimerBaseValid(unsigned long ulBase)
{
    return((ulBase == TIMER1_BASE) || (ulBase == TIMER2_BASE) ||
           (ulBase == TIMER3_BASE) || (ulBase == TIMER4_BASE));
}
#endif

#if !USE_ROM_TIMER
//*****************************************************************************
//
//! Enables the timer(s).
//!
//! \param ulBase is the base address of the timer module.
//!
//! This function enables operation of the timer module.  The timer must be
//! configured before it is enabled.
//!
//! \return None.
//
//*****************************************************************************
void
TimerEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Enable the timer(s) module.
    //
    HWREG(ulBase + TIMER_CTL) |= TIMER_CTL_TEN_EN;
}

//*****************************************************************************
//
//! Disables the timer(s).
//!
//! \param ulBase is the base address of the timer module.
//!
//! This function disables operation of the timer module.
//!
//! \return None.
//
//*****************************************************************************
void
TimerDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Disable the timer module.
    //
    HWREG(ulBase + TIMER_CTL) &= ~(TIMER_CTL_TEN_EN);
}

//*****************************************************************************
//
//! Configures the timer(s).
//!
//! \param ulBase is the base address of the timer module.
//! \param ulConfig is the configuration for the timer.
//!
//! \return None.
//
//*****************************************************************************
void
TimerConfigure(unsigned long ulBase, unsigned long ulConfig)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Disable the timers.
    //
    HWREG(ulBase + TIMER_CTL) &= ~(TIMER_CTL_TEN_EN);

    //
    // Set the global timer configuration.
    //
    HWREG(ulBase + TIMER_CTL) = ulConfig;
}

//*****************************************************************************
//
//! Controls the output level.
//!
//! \param ulBase is the base address of the timer module.
//! \param bInvert specifies the output level.
//!
//! This function sets the PWM output level for the specified timer.  If the
//! \e bInvert parameter is \b true, then the timer's output is made active
//! low; otherwise, it is made active high.
//!
//! \return None.
//
//*****************************************************************************
void
TimerControlLevel(unsigned long ulBase, unsigned char bInvert)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Set the output levels as requested.
    //
    HWREG(ulBase + TIMER_CTL) = (bInvert ?
                                   (HWREG(ulBase + TIMER_CTL) | TIMER_CTL_TPOL_TRUE) :
                                   (HWREG(ulBase + TIMER_CTL) & ~(TIMER_CTL_TPOL_TRUE)));
}


//*****************************************************************************
//
//! Set the timer prescale value.
//!
//! \param ulBase is the base address of the timer module.
//! \param ulValue is the timer prescale value; must be between 0 and 255,
//! inclusive.
//!
//! This function sets the value of the input clock prescaler.  The prescaler
//! is only operational when in 16-bit mode and is used to extend the range of
//! the 16-bit timer modes.
//!
//! \note The availability of the prescaler varies with the Stellaris part and
//! timer mode in use.  Please consult the datasheet for the part you are using
//! to determine whether this support is available.
//!
//! \return None.
//
//*****************************************************************************
void
TimerPrescaleSet(unsigned long ulBase, unsigned long ulValue)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));
    ASSERT((ulValue == TIMER_CTL_PRES_DIVIDE_1)  || (ulValue == TIMER_CTL_PRES_DIVIDE_2)  || (ulValue == TIMER_CTL_PRES_DIVIDE_4)  || (ulValue == TIMER_CTL_PRES_DIVIDE_8) ||
           (ulValue == TIMER_CTL_PRES_DIVIDE_16) || (ulValue == TIMER_CTL_PRES_DIVIDE_32) || (ulValue == TIMER_CTL_PRES_DIVIDE_64) || (ulValue == TIMER_CTL_PRES_DIVIDE_128));

    //
    // Set the timer prescaler if requested.
    //

    HWREG(ulBase + TIMER_CTL) = (HWREG(ulBase + TIMER_CTL) & (~TIMER_CTL_PRES_Msk)) | ulValue;
}

//*****************************************************************************
//
//! Get the timer prescale value.
//!
//! \param ulBase is the base address of the timer module.
//! \param ulTimer specifies the timer; must be one of \b TIMER_A or
//! \b TIMER_B.
//!
//! This function gets the value of the input clock prescaler.  The prescaler
//! is only operational when in 16-bit mode and is used to extend the range of
//! the 16-bit timer modes.
//!
//! \note The availability of the prescaler varies with the Stellaris part and
//! timer mode in use.  Please consult the datasheet for the part you are using
//! to determine whether this support is available.
//!
//! \return The value of the timer prescaler.
//
//*****************************************************************************
unsigned long
TimerPrescaleGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Return the appropriate prescale value.
    //
    return(HWREG(ulBase + TIMER_CTL) & TIMER_CTL_PRES_Msk);
}

//*****************************************************************************
//
//! Sets the timer load value.
//!
//! \param ulBase is the base address of the timer module.
//! \param ulTimer specifies the timer(s) to adjust; must be one of \b TIMER_A,
//! \b TIMER_B, or \b TIMER_BOTH.  Only \b TIMER_A should be used when the
//! timer is configured for 32-bit operation.
//! \param ulValue is the load value.
//!
//! This function sets the timer load value; if the timer is running then the
//! value will be immediately loaded into the timer.
//!
//! \return None.
//
//*****************************************************************************
void
TimerLoadSet(unsigned long ulBase, unsigned long ulValue)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Set the timer load value if requested.
    //

    HWREG(ulBase + TIMER_CNT) = ulValue;

}

//*****************************************************************************
//
//! Gets the current timer value.
//!
//! \param ulBase is the base address of the timer module.
//! \param ulTimer specifies the timer; must be one of \b TIMER_A or
//! \b TIMER_B.  Only \b TIMER_A should be used when the timer is configured
//! for 32-bit operation.
//!
//! This function reads the current value of the specified timer.
//!
//! \return Returns the current value of the timer.
//
//*****************************************************************************
unsigned long
TimerValueGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Return the appropriate timer value.
    //
    return(HWREG(ulBase + TIMER_CNT));
}

//*****************************************************************************
//
//! Sets the timer match value.
//!
//! \param ulBase is the base address of the timer module.
//! \param ulTimer specifies the timer(s) to adjust; must be one of \b TIMER_A,
//! \b TIMER_B, or \b TIMER_BOTH.  Only \b TIMER_A should be used when the
//! timer is configured for 32-bit operation.
//! \param ulValue is the match value.
//!
//! This function sets the match value for a timer.  This is used in capture
//! count mode to determine when to interrupt the processor and in PWM mode to
//! determine the duty cycle of the output signal.
//!
//! \return None.
//
//*****************************************************************************
void
TimerMatchSet(unsigned long ulBase, unsigned long ulValue)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Set the timer match value if requested.
    //

     HWREG(ulBase + TIMER_RLD) = ulValue;
}

//*****************************************************************************
//
//! Gets the timer match value.
//!
//! \param ulBase is the base address of the timer module.
//! \param ulTimer specifies the timer; must be one of \b TIMER_A or
//! \b TIMER_B.  Only \b TIMER_A should be used when the timer is configured
//! for 32-bit operation.
//!
//! This function gets the match value for the specified timer.
//!
//! \return Returns the match value for the timer.
//
//*****************************************************************************
unsigned long
TimerMatchGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Return the appropriate match value.
    //
    return(HWREG(ulBase + TIMER_RLD));
}

void
TimerPWMSet(unsigned long ulBase, unsigned long ulValue)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Set the timer match value if requested.
    //

     HWREG(ulBase + TIMER_PWM) = ulValue;
}

unsigned long
TimerPWMGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));

    //
    // Return the appropriate match value.
    //
    return(HWREG(ulBase + TIMER_PWM));
}

//*****************************************************************************
//
//! Registers an interrupt handler for the timer interrupt.
//!
//! \param ulIntChannel is the interrupt channel number.
//! \param pfnHandler is a pointer to the function to be called when the timer
//! interrupt occurs.
//!
//! This function sets the handler to be called when a timer interrupt occurs.
//! In addition, this function enables the global interrupt in the interrupt
//! controller; specific timer interrupts must be enabled via TimerIntEnable().
//! It is the interrupt handler's responsibility to clear the interrupt source
//! via TimerIntClear().
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
TimerIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    //
    // Check the arguments.
    //
    ASSERT((ulIntChannel == INT_TIMER1) || (ulIntChannel == INT_TIMER2) || (ulIntChannel == INT_TIMER3) || (ulIntChannel == INT_TIMER4));

	//
	// Register the interrupt handler.
	//
	IntRegister(ulIntChannel, g_pRAMVectors, pfnHandler);

	//
	// Enable the interrupt.
	//
	IntEnable(ulIntChannel);
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for the timer interrupt.
//!
//! \param ulIntChannel is the interrupt channel number.
//!
//! This function clears the handler to be called when a timer interrupt
//! occurs.  This function also masks off the interrupt in the interrupt
//! controller so that the interrupt handler no longer is called.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
TimerIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors)
{
    //
    // Check the arguments.
    //
    ASSERT((ulIntChannel == INT_TIMER1) || (ulIntChannel == INT_TIMER2) || (ulIntChannel == INT_TIMER3) || (ulIntChannel == INT_TIMER4));

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
//! Enables individual timer interrupt sources.
//!
//! \param ulBase is the base address of the timer module.
//! \param ulIntFlags is the bit mask of the interrupt sources to be enabled.
//!
//! Enables the indicated timer interrupt sources.  Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor.
//!
//! The \e ulIntFlags parameter must be one of the following:
//!
//! TIMER_CTL_TICONFIG_OUTER_EVENT, Timer Interrupt only on defined Input Capture/Deassertion Events
//! TIMER_CTL_TICONFIG_INNER_EVENT, Timer Interrupt only on defined Reload/Compare Events
//! TIMER_CTL_TICONFIG_ALL_EVENT,   Timer Interrupt occurs on all defined Reload, Compare and Input Events
//!
//! \return None.
//
//*****************************************************************************
void
TimerIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    //
    // Check the arguments.
    //
    ASSERT(TimerBaseValid(ulBase));
    ASSERT((ulIntFlags == TIMER_CTL_TICONFIG_OUTER_EVENT) || (ulIntFlags == TIMER_CTL_TICONFIG_INNER_EVENT) || (ulIntFlags == TIMER_CTL_TICONFIG_ALL_EVENT));

    //
    // Enable the specified interrupts.
    //
    HWREG(ulBase + TIMER_CTL) = (HWREG(ulBase + TIMER_CTL) & (~TIMER_CTL_TICONFIG_Msk)) | ulIntFlags;
}

#endif

