#include "gpio.h"

#if !USE_ROM_GPIO

void GPIOPeripheralPad(unsigned char ucPeriNum, unsigned char ucPadNum)
{
    unsigned long ulPADSEL;
    
    ulPADSEL = GPIO_PAD_SEL0 + ucPadNum * 4;
    
    HWREG(GPIO_BASE + ulPADSEL) = ucPeriNum;
}


void GPIODirModeSet(unsigned char ucPins, unsigned long ulPinIO)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    if(ulPinIO == GPIO_DIR_MODE_HW)
    {
        ulValue = HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg) = (ulValue | ulOffsetBit);
        
        ulValue = HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg) = (ulValue | ulOffsetBit);
    }
    else
    {
        ulValue = HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
        
        ulValue = HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
        
        if(ulPinIO == GPIO_DIR_MODE_IN)
        {
            ulValue = HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg);
            HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
            
            ulValue = HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg);
            HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
        }
        else if(ulPinIO == GPIO_DIR_MODE_OUT)
        {
            ulValue = HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg);
            HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
            
            ulValue = HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg);
            HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
        }
        else if(ulPinIO == GPIO_DIR_MODE_INOUT)
        {
            ulValue = HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg);
            HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
            
            ulValue = HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg);
            HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
        }
    }
}

void GPIOOutputODEn(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_OD0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_OD0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
}

void GPIOOutputODDis(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_OD0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_OD0 + ulOffsetReg) = (ulValue | ulOffsetBit);
}

void GPIOAnalogEn(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_ANALOG_EN0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_ANALOG_EN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
}

void GPIOAnalogDis(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_ANALOG_EN0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_ANALOG_EN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
}

void GPIOPullupEn(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_PULLUP0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_PULLUP0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
}

void GPIOPullupDis(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_PULLUP0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_PULLUP0 + ulOffsetReg) = (ulValue | ulOffsetBit);
}

void GPIOPulldownEn(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_PULLDOWN0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_PULLDOWN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
}

void GPIOPulldownDis(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_PULLDOWN0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_PULLDOWN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
}

void GPIOPinSet(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_DOUT0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_DOUT0 + ulOffsetReg) = (ulValue | ulOffsetBit);
}


void GPIOPinClear(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_DOUT0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_DOUT0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
}


unsigned char GPIOPinRead(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_DIN0 + ulOffsetReg);
    
    return (ulValue & ulOffsetBit) ? 1 : 0;
}

void GPIOIntEnable(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_INTEN0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_INTEN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
}

void GPIOIntDisable(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_INTEN0 + ulOffsetReg);
    HWREG(GPIO_BASE + GPIO_INTEN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
}

void GPIOIntTypeSet(unsigned char ucPins, unsigned char ucConfig)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_INT_TYPE_LEVEL0 + ulOffsetReg);
    
    if(ucConfig == GPIO_INT_EDGE)
    {
        HWREG(GPIO_BASE + GPIO_INT_TYPE_LEVEL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
    else if(ucConfig == GPIO_INT_LEVEL)
    {
        HWREG(GPIO_BASE + GPIO_INT_TYPE_LEVEL0 + ulOffsetReg) = (ulValue | ulOffsetBit);
    }
}

void GPIOIntEdgeSet(unsigned char ucPins, unsigned char ucConfig)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_INT_BOTH_EDGE0 + ulOffsetReg);
    
    if(ucConfig == GPIO_INT_EDGE_SINGLE)
    {
        HWREG(GPIO_BASE + GPIO_INT_BOTH_EDGE0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
    else if(ucConfig == GPIO_INT_EDGE_BOTH)
    {
        HWREG(GPIO_BASE + GPIO_INT_BOTH_EDGE0 + ulOffsetReg) = (ulValue | ulOffsetBit);
    }
}

void GPIOIntSingleEdgeSet(unsigned char ucPins, unsigned char ucConfig)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_INT_FALL_EDGE0 + ulOffsetReg);
    
    if(ucConfig == GPIO_INT_EDGE_RISE)
    {
        HWREG(GPIO_BASE + GPIO_INT_FALL_EDGE0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
    else if(ucConfig == GPIO_INT_EDGE_FALL)
    {
        HWREG(GPIO_BASE + GPIO_INT_FALL_EDGE0 + ulOffsetReg) = (ulValue | ulOffsetBit);
    }
}

unsigned long GPIOIntStatusGet(unsigned char ucPins)
{
    if(ucPins < 32)
    {
        return (HWREG(GPIO_BASE + GPIO_INT_STAT0));
    }
    else
    {
        return (HWREG(GPIO_BASE + GPIO_INT_STAT1));
    }
}

unsigned long GPIOIntMaskGet(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_INT_MASK0 + ulOffsetReg);
    
    return ((ulValue & ulOffsetBit) ? 1 : 0);
}

unsigned long GPIOConflictStatusGet(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_PAD_CFT0 + ulOffsetReg);
    
    return ((ulValue & ulOffsetBit) ? 1 : 0);
}

void GPIOConflictStatusClear(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    HWREG(GPIO_BASE + GPIO_PAD_CFT0 + ulOffsetReg) = ulOffsetBit;
}

unsigned long GPIOAllocationStatusGet(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_PAD_BLKS0 + ulOffsetReg);
    
    return ((ulValue & ulOffsetBit) ? 1 : 0);
}

void GPIODrvStrengthSet(unsigned char ucPins, unsigned char ucDrvStrength)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    ulOffsetReg = (ucPins / 10) * 4;
    
    ulOffsetBit = (ucPins % 10) * 3;
    
    ulValue = HWREG(GPIO_BASE + GPIO_DRVCTL0 + ulOffsetReg);
    
    ulValue = (ulValue & (~(0x7 << ulOffsetBit))) | ucDrvStrength;
    
    HWREG(GPIO_BASE + GPIO_DRVCTL0 + ulOffsetReg) = ulValue;
}

void GPIOIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    //
    // Register the interrupt handler.
    //
    IntRegister(INT_GPIO, g_pRAMVectors, pfnHandler);

    //
    // Enable the UART interrupt.
    //
    IntEnable(INT_GPIO);
}

void GPIOIntUnregister(unsigned long *g_pRAMVectors)
{
	//
	// Disable the interrupt.
	//
	IntDisable(INT_GPIO);

	//
	// Unregister the interrupt handler.
	//
	IntUnregister(INT_GPIO, g_pRAMVectors);
}


void GPIOModeSet(unsigned char ucPins, unsigned char ucMode)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg);

    if(ucMode == GPIO_MODE_HW)
    {
        HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg) = (ulValue | ulOffsetBit);        
    }
    else
    {
        HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
}

void GPIOPadConfigSel(unsigned char ucPins, unsigned char ucConfig)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    ulValue = HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg);

    if(ucConfig == GPIO_CTL_PERIPHERAL)
    {
        HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg) = (ulValue | ulOffsetBit);
    }
    else
    {
        HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
}

void GPIOPadModeSet(unsigned char ucPins, unsigned char ucMode, unsigned char ucConfig)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    if(ucMode == GPIO_MODE_HW)
    {
        ulValue = HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg) = (ulValue | ulOffsetBit);

        ulValue = HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg);
        
        if(ucConfig == GPIO_CTL_PERIPHERAL)
        {
            HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg) = (ulValue | ulOffsetBit);
        }
        else
        {
            HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
        }
    }
    else
    {
        ulValue = HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_CTRL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));

        ulValue = HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_CFGSEL0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
}

void GPIODirectionSet(unsigned char ucPins, unsigned long ulPinIO)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    unsigned long ulValue;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    if(ulPinIO == GPIO_DIR_MODE_IN)
    {
        ulValue = HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
            
        ulValue = HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
    }
    else if(ulPinIO == GPIO_DIR_MODE_OUT)
    {
        ulValue = HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
        
        ulValue = HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
    else if(ulPinIO == GPIO_DIR_MODE_INOUT)
    {
        ulValue = HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_INPUTEN0 + ulOffsetReg) = (ulValue | ulOffsetBit);
        
        ulValue = HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg);
        HWREG(GPIO_BASE + GPIO_OUTPUTEN0 + ulOffsetReg) = (ulValue & (~ulOffsetBit));
    }
}

#endif

