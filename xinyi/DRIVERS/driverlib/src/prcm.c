#include "prcm.h"

void prcm_delay(unsigned long uldelay)
{
    unsigned long i;
    
    for(i = 0; i < uldelay; i++)
    {
    }
}

#if !USE_ROM_PRCM
// 0 means: LOAD_IMAGE
// 1 means: NOT_LOAD_IMAGE
unsigned char AON_DSP_Load_Flag_Get(void)
{
    volatile uint8_t core_load_flag = 0;
    
    core_load_flag = HWREGB(AON_AONGPREG);
    
    if(core_load_flag & AONGPREG_CORE_LOAD_FLAG_DSP)
    {
        return NOT_LOAD_IMAGE;
    }
    
    return LOAD_IMAGE;
}


unsigned char AON_ARM_Load_Flag_Get(void)
{
    volatile uint8_t core_load_flag = 0;
    
    core_load_flag = HWREGB(AON_AONGPREG);
    
    if(core_load_flag & AONGPREG_CORE_LOAD_FLAG_ARM)
    {
        return NOT_LOAD_IMAGE;
    }
    
    return LOAD_IMAGE;
}


void AON_DSP_Load_Flag_Set(uint8_t flag)
{
    if(LOAD_IMAGE == flag)
    {
        HWREGB(AON_AONGPREG) &= (uint8_t)(~AONGPREG_CORE_LOAD_FLAG_DSP);
    }
    else
    {
        HWREGB(AON_AONGPREG) |= (uint8_t)AONGPREG_CORE_LOAD_FLAG_DSP;
    }
}


void AON_ARM_Load_Flag_Set(uint8_t flag)
{
    if(LOAD_IMAGE == flag)
    {
        HWREGB(AON_AONGPREG) &= (uint8_t)(~AONGPREG_CORE_LOAD_FLAG_ARM);
    }
    else
    {
        HWREGB(AON_AONGPREG) |= (uint8_t)AONGPREG_CORE_LOAD_FLAG_ARM;
    }
}
#endif


void AON_Flash_Delay_Get(void)
{
    volatile uint8_t flash_delay_config = 0;
    
    flash_delay_config = ((HWREGB(AON_AONGPREG) >> 3) & 0x07);
    
    if(0x00 == flash_delay_config)
    {
        prcm_delay(160000);
    }
    else if(0x01 == flash_delay_config)
    {
        prcm_delay(140000);
    }
    else if(0x02 == flash_delay_config)
    {
        prcm_delay(110000);
    }
    else if(0x03 == flash_delay_config)
    {
        prcm_delay(80000);
    }
    else if(0x04 == flash_delay_config)
    {
        prcm_delay(40000);
    }
    else if(0x05 == flash_delay_config)
    {
        prcm_delay(20000);
    }
    else if(0x06 == flash_delay_config)
    {
        prcm_delay(10000); // 0.65 ms in FPGA (61.44 MHz Clock)
    }
    else if(0x07 == flash_delay_config)
    {
        prcm_delay(5000);
    }
}


#if !USE_ROM_PRCM
void AON_Flash_Delay_Set(uint8_t value)
{
    HWREGB(AON_AONGPREG) = ((HWREGB(AON_AONGPREG) & (uint8_t)(~AONGPREG_FLASH_DELAY)) | ((value & 0x7) << 3));
}


unsigned char AON_Addr_Overlap_Unused_Mem(unsigned char mem_status, unsigned long ulAddr, unsigned long ulSizebyte)
{
    if(mem_status == 0x00)
    {
        return 0;
    }
    
    // SYS MEM 0
    if(mem_status & 0x01)
    {
        if((((ulAddr + ulSizebyte) > 0x20000000) && ((ulAddr + ulSizebyte) <= 0x20020000)) || ((ulAddr >= 0x20000000) && (ulAddr < 0x20020000)) || ((ulAddr <= 0x20000000) && ((ulAddr + ulSizebyte) >= 0x20020000)))
        {
            return 1;
        }
    }
    
    // SYS MEM 1
    if(mem_status & 0x02)
    {
        if((((ulAddr + ulSizebyte) > 0x20020000) && ((ulAddr + ulSizebyte) <= 0x20040000)) || ((ulAddr >= 0x20020000) && (ulAddr < 0x20040000)) || ((ulAddr <= 0x20020000) && ((ulAddr + ulSizebyte) >= 0x20040000)))
        {
            return 1;
        }
    }
    
    // SYS MEM 2
    if(mem_status & 0x04)
    {
        if((((ulAddr + ulSizebyte) > 0x20040000) && ((ulAddr + ulSizebyte) <= 0x20060000)) || ((ulAddr >= 0x20040000) && (ulAddr < 0x20060000)) || ((ulAddr <= 0x20040000) && ((ulAddr + ulSizebyte) >= 0x20060000)))
        {
            return 1;
        }
    }
    
    // SYS MEM 3
    if(mem_status & 0x08)
    {
        if((((ulAddr + ulSizebyte) > 0x20060000) && ((ulAddr + ulSizebyte) <= 0x20080000)) || ((ulAddr >= 0x20060000) && (ulAddr < 0x20080000)) || ((ulAddr <= 0x20060000) && ((ulAddr + ulSizebyte) >= 0x20080000)))
        {
            return 1;
        }
    }
    
    return 0;
}


void PRCM_Clock_Enable(unsigned long ulBase, unsigned long ulModule)
{
    HWREG(ulBase + PRCM_CKG_CTL) |= ulModule;
}


void PRCM_Clock_Disable(unsigned long ulBase, unsigned long ulModule)
{
    HWREG(ulBase + PRCM_CKG_CTL) &= (~ulModule);
}


unsigned long PRCM_CHIP_VER_GET(unsigned long ulBase)
{
    return (HWREG(ulBase + PRCM_CHIP_VER));
}
#endif


unsigned char PRCM_BOOTMODE_GET(unsigned long ulBase)
{
    volatile unsigned long ulReg;
    unsigned char ucBootMode;
    
    ulReg = HWREG(ulBase + PRCM_BOOTMODE_CTL);
    
    ucBootMode = (unsigned char)(ulReg & 0x07);
    
    return ucBootMode;
}

unsigned char hclk_div_reg[16]={0,0,1,0,5,6,7,8,0,9,0,10,0,0,0,11};

void PRCM_Clock_Ctl_Reset(unsigned long ulBase,unsigned char pclk_div, unsigned char hclk_div, unsigned char dsp_div)
{
	volatile unsigned char ulReg = HWREGB(ulBase+PRCM_CLKRST_CTL);
	(void) dsp_div;
	if(pclk_div)
		ulReg = ((ulReg&0x3F)|((pclk_div-1)<<6));
	if(hclk_div)
		ulReg = ((ulReg&0xE1)|(hclk_div_reg[hclk_div-1]<<1));
	HWREGB(ulBase+PRCM_CLKRST_CTL) = ulReg;
	//ulReg = HWREGB(ulBase+PRCM_DSPCLKRSTCTL);
	//HWREGB(ulBase+PRCM_DSPCLKRSTCTL) = ((ulReg&0xE1)|(hclk_div_reg[dsp_div-1]<<1));
}



