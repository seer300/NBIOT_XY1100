#include "sema.h"


void SEMAMasterEnable(unsigned long ulBase, unsigned char ucSemaMaster, unsigned char ucSemaSlave)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    // Master Enable
    HWREG(ulSemaReg) |= (1 << (ucSemaMaster - 1));
}


void SEMAMasterDisable(unsigned long ulBase, unsigned char ucSemaMaster, unsigned char ucSemaSlave)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    // Master Disable
    HWREG(ulSemaReg) = (HWREG(ulSemaReg) & ~(1 << (ucSemaMaster - 1)));
}


void SEMARequest(unsigned long ulBase, unsigned char ucSemaMaster, unsigned char ucSemaSlave)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    // wait until actually granted
    while((HWREG(ulSemaReg) & SEMA_SEMA_OWNER_Msk) != (((unsigned long)(ucSemaMaster)) << SEMA_SEMA_OWNER_Pos))
    {
		// Request
    	HWREG(ulSemaReg) = (SEMA_SEMA_REQ | (1<<(ucSemaMaster-1)));
    }
}



void SEMARequestNonBlocking(unsigned long ulBase, unsigned char ucSemaMaster, unsigned char ucSemaSlave, unsigned char ucMasterMask)
{
    unsigned long ulSemaReg;
    (void) ucSemaMaster;
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    // Request
    HWREG(ulSemaReg) |= (SEMA_SEMA_REQ | ucMasterMask);
}


void SEMARequestPriSet(unsigned long ulBase, unsigned char ucSemaSlave, unsigned long ucPriority)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    HWREG(ulSemaReg) |= ucPriority;
}


unsigned char SEMAMasterGet(unsigned long ulBase, unsigned char ucSemaSlave)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    return ((HWREG(ulSemaReg) & SEMA_SEMA_OWNER_Msk) >> SEMA_SEMA_OWNER_Pos);
}


unsigned long SEMASlaveGet(unsigned long ulBase, unsigned char ucSemaMaster)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_M0GNT + ((ucSemaMaster - 1) << 2);
    
    return (HWREG(ulSemaReg));
}


void SEMAReset(unsigned long ulBase)
{
    HWREG(ulBase + REG_SEMA_CTL) |= SEMA_CTL_RST;
}


unsigned char SEMAReqQueueState(unsigned long ulBase)
{
    return ((HWREG(ulBase + REG_SEMA_CTL) & SEMA_CTL_REQ_FULL_Msk) ? 1 : 0);
}


void SEMAMasterIntEnable(unsigned long ulBase, unsigned long ulSemaMasterInt)
{
    HWREG(ulBase + REG_SEMA_CTL) |= ulSemaMasterInt;
}

void SEMARelease(unsigned long ulBase, unsigned char ucSemaMaster, unsigned char ucSemaSlave)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    // Release
    do
	{
    	HWREG(ulSemaReg) = HWREG(ulSemaReg) & ~(SEMA_SEMA_REQ| (1<<(ucSemaMaster-1))) ;
    }while((HWREG(ulSemaReg) & SEMA_SEMA_OWNER_Msk) == (((unsigned long)(ucSemaMaster)) << SEMA_SEMA_OWNER_Pos));
	
    
}

#if !USE_ROM_SEMA
void SEMAIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    //
    // Register the interrupt handler.
    //
    IntRegister(INT_SEMAPHORE, g_pRAMVectors, pfnHandler);

    //
    // Enable the interrupt.
    //
    IntEnable(INT_SEMAPHORE);
}

void SEMAIntUnregister(unsigned long *g_pRAMVectors)
{
	//
	// Disable the interrupt.
	//
	IntDisable(INT_SEMAPHORE);

	//
	// Unregister the interrupt handler.
	//
	IntUnregister(INT_SEMAPHORE, g_pRAMVectors);
}
#endif


