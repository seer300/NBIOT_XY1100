#include "mcnt.h"

void MCNTStart(void)
{
    HWREG(MEASURECNT_BASE + MCNT_MEASURECTRL) = 0x1;
}

void MCNTStop(void)
{
    HWREG(MEASURECNT_BASE + MCNT_MEASURECTRL) = 0x0;
}

void MCNTSetCNT32K(unsigned long ulCounter)
{
    HWREG(MEASURECNT_BASE + MCNT_CNT32K) = ulCounter;
}

unsigned long MCNTGetCNT32K(void)
{
    return (HWREG(MEASURECNT_BASE + MCNT_CNT32K));
}

unsigned long MCNTGetMCNT(void)
{
    return (HWREG(MEASURECNT_BASE + MCNT_CNTMEASURE));
}

unsigned char MCNTGetIntStatus(void)
{
    return (HWREG(MEASURECNT_BASE + MCNT_MCNTINT) & 0x01);
}

void MCNTSetIntStatus(unsigned char ucIntstatus)
{
    HWREG(MEASURECNT_BASE + MCNT_MCNTINT) = ucIntstatus;
}

void MCNTIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_MEASURECNT, g_pRAMVectors, pfnHandler);

    IntEnable(INT_MEASURECNT);
}

void MCNTIntUnregister(unsigned long *g_pRAMVectors)
{
    IntDisable(INT_MEASURECNT);

    IntUnregister(INT_MEASURECNT, g_pRAMVectors);
}

