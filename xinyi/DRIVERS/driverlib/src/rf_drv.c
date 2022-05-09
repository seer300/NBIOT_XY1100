#include "rf_drv.h"
#include "softap_macro.h"

TableType2 GAIN_TABLE_RX_HB_LNA[GAIN_TABLE_SIZE_RX_HB_LNA] = {
    { 265,	31,	0},
    { 217,	31,	5},
    { 163,	31,	8},
    { 108,	29,	10},
    {  69,	18,	10},
    {  30,	 0,	6},
    { -18,	 0,	9},
    { -64,	 0,	11},
    {-128,	 0,	13},
};


TableType1 GAIN_TABLE_RX_LB_LNA[GAIN_TABLE_SIZE_RX_LB_LNA] = {
    { 220, 7},
    { 160, 6},
    { 100, 5},
    {  40, 4},
    { -20, 3},
    { -80, 2},
    {-140, 1},
};


TableType2 GAIN_TABLE_RX_MIXER[GAIN_TABLE_SIZE_RX_MIXER] = {
    {240,	1,	15},
    {206,	0,	15},
    {173,	1,	4},
    {144,	0,	5},
    {115,	0,	2},
};


TableType3 GAIN_TABLE_TX_LB_MIXER[GAIN_TABLE_SIZE_TX_LB_MIXER] = {
    {12,    63},
    {11,    56},
    {10,    50},
    { 9,    44},
    { 8,    39},
    { 7,    35},
    { 6,    31},
    { 5,    28},
    { 4,    25},
    { 3,    22},
    { 2,    19},
    { 1,    17},
    { 0,    15},
    {-1,	14},
    {-2,	12},
    {-3,	11},
    {-4,	10},
    {-5,	8},
    {-6,	7},
};


TableType3 GAIN_TABLE_TX_LB_PA[GAIN_TABLE_SIZE_TX_LB_PA] = {
    {18,	255},
    {12,	15},
    { 6,	3},
    { 0,	1},
};


TableType3 GAIN_TABLE_TX_BB[GAIN_TABLE_SIZE_TX_BB] = {
    {  6,	9},
    {  3,	8},
    {  0,	7},
    { -3,	6},
    { -6,	5},
    { -9,	4},
    {-12,	3},
    {-15,	2},
    {-18,	1},
    {-21,	0},
};


TableType4 GAIN_TABLE_TX_HB_MIXER[GAIN_TABLE_SIZE_TX_HB_MIXER] = {
    {  58, 63},
    {  49, 57},
    {  38, 50},
    {  29, 45},
    {  19, 40},
    {  10, 36},
    {   0, 32},
    { -11, 28},
    { -20, 25},
    { -31, 22},
    { -48, 18},
    { -58, 16},
    { -69, 14},
    { -75, 13},
    { -89, 11},
    { -97, 10},
    {-106, 9 },
    {-116, 8 },
    {-127, 7 },
    {-139, 6 },
    {-153, 5 },
    {-171, 4 },
    {-193, 3 },
    {-222, 2 },
    {-266, 1 },
    {-362, 0 },
};


TableType4 GAIN_TABLE_TX_HB_PAD[GAIN_TABLE_SIZE_TX_HB_PAD] = {
    { 190, 255},
    { 180, 228},
    { 170, 203},
    { 160, 181},
    { 150, 161},
    { 140, 144},
    { 130, 128},
    { 120, 114},
    { 110, 102},
    { 100, 91 },
    {  90, 81 },
    {  80, 72 },
    {  71, 65 },
    {  61, 58 },
    {  50, 51 },
    {  41, 46 },
    {  31, 41 },
    {  22, 37 },
    {  12, 33 },
    {   1, 29 },
    {  -8, 26 },
    { -18, 23 },
    { -26, 21 },
    { -35, 19 },
    { -45, 17 },
    { -56, 15 },
    { -68, 13 },
    { -75, 12 },
    { -83, 11 },
    { -91, 10 },
    {-100, 9  },
    {-110, 8  },
    {-122, 7  },
    {-135, 6  },
    {-151, 5  },
    {-170, 4  },
    {-195, 3  },
    {-231, 2  },
    {-291, 1  },
};


TableType4 GAIN_TABLE_TX_HB_PA[GAIN_TABLE_SIZE_TX_HB_PA] = {
    {160, 255},
    {148, 127},
    {135, 63 },
    {119, 31 },
    { 99, 15 },
    { 74, 7  },
    { 39, 3  },
    {-20, 1  },
};


void rf_drv_delay(uint32_t uldelay)
{
    uint32_t i;
    
    for(i = 0; i < uldelay; i++)
    {
    }
}

uint8_t RF_TABLE_TYPE3_Search_by_dB(int8_t dB, TableType3 *table, uint8_t size)
{
    uint8_t i;
    
    if(dB >= table[0].dB)
    {
        return 0;
    }
    else
    {
        for(i = 0; i < size - 1; i++)
        {
            if((dB <= table[i].dB) && (dB > table[i+1].dB))
            {
                return i;
            }
        }
        
        return i;
    }
}

uint8_t RF_TABLE_TYPE4_Search_by_dBTen(int16_t dBTen, TableType4 *table, uint8_t size)
{
    uint8_t i;
    
    if(dBTen >= table[0].dBTen)
    {
        return 0;
    }
    else
    {
        for(i = 0; i < size - 1; i++)
        {
            if((dBTen <= table[i].dBTen) && (dBTen > table[i+1].dBTen))
            {
                return i;
            }
        }
        
        return i;
    }
}

uint8_t REG_Bus_Field_Set(uint32_t ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, uint32_t ulValue)
{
    uint8_t ucBitBoundary;
    uint8_t ucRegValueByte;
    uint8_t ucMaskByte;
	uint8_t ucRegBaseOffset;
    uint32_t ulRegValueWord;
    uint32_t ulRegOffset;
    uint32_t ulMaskWord;
    uint32_t ulRegValue;
    
    RegAccessMode reg_access_mode = AccessModeUnknow;
	
	ucRegBaseOffset = ulRegBase & 0x03;
	
	if(ucRegBaseOffset)
	{
		ulRegBase -= ucRegBaseOffset;
		
		ucBit_high += (ucRegBaseOffset << 3);
		
		ucBit_low  += (ucRegBaseOffset << 3);
	}
    
    if(ucBit_high < ucBit_low || (ucBit_high - ucBit_low) > 31)
    {
        return 1;
    }
    
    if(ucBit_high - ucBit_low > 7)
    {
        reg_access_mode = AccessModeWord;
    }
    else if((ucBit_high & 0xF8) != (ucBit_low & 0xF8))
    {
        reg_access_mode = AccessModeWord;
    }
    else
    {
        reg_access_mode = AccessModeByte;
    }
    
    if(reg_access_mode == AccessModeWord)
    {
        if((ucBit_high - ucBit_low == 31) && ((ucBit_low & 0x1F) == 0x00))
        {
            ulRegOffset = (ucBit_low >> 5) << 2;
            
            HWREG(ulRegBase + ulRegOffset) = ulValue;
        }
        else if((ucBit_high & 0xE0) == (ucBit_low & 0xE0))
        {
            ulRegOffset = (ucBit_low >> 5) << 2;
        
            ucBitBoundary = ucBit_low & 0x1F;
            
            ulMaskWord = ((uint32_t)((uint32_t)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
            
            HWREG(ulRegBase + ulRegOffset) = (ulRegValueWord & (~(ulMaskWord << ucBitBoundary))) | (ulValue << ucBitBoundary);
        }
        else
        {
            // First Word
            ulRegOffset = (ucBit_low >> 5) << 2;
        
            ucBitBoundary = ucBit_low & 0x1F;
            
            ulMaskWord = ((uint32_t)((uint32_t)0x01 << (32 - (ucBit_low & 0x1F)))) - 0x01;
            
            ulRegValue = ulValue & ulMaskWord;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
            
            HWREG(ulRegBase + ulRegOffset) = (ulRegValueWord & (~(ulMaskWord << ucBitBoundary))) | (ulRegValue << ucBitBoundary);
            
            // Second Word
            ulRegOffset += 4;
            
            ucBitBoundary = 0;
            
            ulMaskWord = ((uint32_t)((uint32_t)0x01 << ((ucBit_high & 0x1F) + 1))) - 0x01;
            
            ulRegValue = ulValue >> (32 - (ucBit_low & 0x1F));
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
            
            HWREG(ulRegBase + ulRegOffset) = (ulRegValueWord & (~(ulMaskWord << ucBitBoundary))) | (ulRegValue << ucBitBoundary);
        }
    }
    else
    {
        ulRegOffset = ucBit_low >> 3;
        
        ucBitBoundary = ucBit_low & 0x07;
        
        ucMaskByte = ((uint8_t)((uint8_t)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
        
        ucRegValueByte = HWREGB(ulRegBase + ulRegOffset);
        
        HWREGB(ulRegBase + ulRegOffset) = (ucRegValueByte & (~(ucMaskByte << ucBitBoundary))) | (ulValue << ucBitBoundary);
    }
    
    return 0;
}


uint8_t REG_Bus_Field_Get(uint32_t ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, uint32_t *ulValue)
{
    uint8_t ucBitBoundary;
    uint8_t ucRegValueByte;
    uint8_t ucMaskByte;
	uint8_t ucRegBaseOffset;
    uint32_t ulRegValueWord;
    uint32_t ulRegOffset;
    uint32_t ulMaskWord;
	uint32_t ulRegValue;
    
    RegAccessMode reg_access_mode = AccessModeUnknow;
	
	ucRegBaseOffset = ulRegBase & 0x03;
	
	if(ucRegBaseOffset)
	{
		ulRegBase -= ucRegBaseOffset;
		
		ucBit_high += (ucRegBaseOffset << 3);
		
		ucBit_low  += (ucRegBaseOffset << 3);
	}
    
    if(ucBit_high < ucBit_low || (ucBit_high - ucBit_low) > 31)
    {
        return 1;
    }
    
    if(ucBit_high - ucBit_low > 7)
    {
        reg_access_mode = AccessModeWord;
    }
    else if((ucBit_high & 0xF8) != (ucBit_low & 0xF8))
    {
        reg_access_mode = AccessModeWord;
    }
    else
    {
        reg_access_mode = AccessModeByte;
    }
    
    if(reg_access_mode == AccessModeWord)
    {
		
		if((ucBit_high - ucBit_low == 31) && ((ucBit_low & 0x1F) == 0x00))
        {
			ulRegOffset = (ucBit_low >> 5) << 2;
			
			*ulValue = HWREG(ulRegBase + ulRegOffset);
		}
		else if((ucBit_high & 0xE0) == (ucBit_low & 0xE0))
		{
			ulRegOffset = (ucBit_low >> 5) << 2;
        
			ucBitBoundary = ucBit_low & 0x1F;
			
			ulMaskWord = ((uint32_t)((uint32_t)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
			
			ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
			
			*ulValue = (ulRegValueWord >> ucBitBoundary) & ulMaskWord;
		}
		else
        {
            // First Word
            ulRegOffset = (ucBit_low >> 5) << 2;
        
            ucBitBoundary = ucBit_low & 0x1F;
            
            ulMaskWord = ((uint32_t)((uint32_t)0x01 << (32 - (ucBit_low & 0x1F)))) - 0x01;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
			
			ulRegValue = (ulRegValueWord >> ucBitBoundary) & ulMaskWord;
            
            // Second Word
            ulRegOffset += 4;
            
            ucBitBoundary = 0;
            
            ulMaskWord = ((uint32_t)((uint32_t)0x01 << ((ucBit_high & 0x1F) + 1))) - 0x01;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
			
			*ulValue = (((ulRegValueWord & ulMaskWord) << (32 - (ucBit_low & 0x1F))) | ulRegValue);
        }
        
    }
    else
    {
        ulRegOffset = ucBit_low >> 3;
        
        ucBitBoundary = ucBit_low & 0x07;
        
        ucMaskByte = ((uint8_t)((uint8_t)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
        
        ucRegValueByte = HWREGB(ulRegBase + ulRegOffset);
        
        *ulValue = (ucRegValueByte >> ucBitBoundary) & ucMaskByte;
    }
    
    return 0;
}

void PRCM_Clock_Mode_Force_XTAL(void)
{
	HWREGB(PRCM_BASE + 0x0C) |= 0x01;
}

void PRCM_Clock_Mode_Auto(void)
{
	HWREGB(PRCM_BASE + 0x0C) &= 0xFE;
}

void RF_RFPLL_LDO_Power_On(void)
{
	HWREGB(REG_8_ANARFPLL8) |= ANARFPLL8_RFPLL_LDO_PU_Msk;
}

void RF_RFPLL_LDO_Power_Off(void)
{
	HWREGB(REG_8_ANARFPLL8) &= (uint8_t)(~ANARFPLL8_RFPLL_LDO_PU_Msk);
}

void RF_RFPLL_LDO_Filter_En(void)
{
	HWREGB(REG_8_ANARFPLL8) |= ANARFPLL8_RFPLL_LDO_FILTER_EN_Msk;
}

void RF_RFPLL_LDO_Filter_Dis(void)
{
	HWREGB(REG_8_ANARFPLL8) &= (uint8_t)(~ANARFPLL8_RFPLL_LDO_FILTER_EN_Msk);
}

void RF_RFPLL_Power_On(void)
{
	HWREGB(REG_8_ANARFPLL8) |= ANARFPLL8_RFPLL_PU_Msk;
}

void RF_RFPLL_Power_Off(void)
{
	HWREGB(REG_8_ANARFPLL8) &= (uint8_t)(~ANARFPLL8_RFPLL_PU_Msk);
}

void RF_RFPLL_Cal_On(void)
{
	HWREGB(REG_8_ANARFPLL8) |= ANARFPLL8_RFPLL_CAL_EN_Msk;
}

void RF_RFPLL_Cal_Off(void)
{
	HWREGB(REG_8_ANARFPLL8) &= ((uint8_t)~ANARFPLL8_RFPLL_CAL_EN_Msk);
}

void RF_RFPLL_On(void)
{
    RF_RFPLL_LDO_Power_On();
    
    // Delay 1 32-KHz Cycle
    rf_drv_delay(0x100);
    
    RF_RFPLL_LDO_Filter_En();
    
    RF_RFPLL_Power_On();
}

void RF_RFPLL_Off(void)
{
    RF_RFPLL_LDO_Power_Off();
    
    RF_RFPLL_LDO_Filter_Dis();
    
    RF_RFPLL_Power_Off();
}


void RF_BBPLL_Cal_On(void)
{
	HWREGB(PRCM_BASE + 0x5B) |= 0x04;
}

void RF_BBPLL_Cal_Off(void)
{
	HWREGB(PRCM_BASE + 0x5B) &= 0xFB;
}


void RF_BBPLL_Switch_Outer_Clock(void)
{
	HWREGB(0xa0058024) = 0x04;							//Comment enable rfldo_cntl_en
	HWREGB(0x2601f0da) = 0x06;							//Comment enable rfpll ldo
    
	REG_Bus_Field_Set(0x2601f0df,  32,  31, 0x3);		//Comment enable bbpll_bypass and bbpll_pu
	REG_Bus_Field_Set(0xa0110060, 122, 122, 0x1);		//Comment enable bbpll bypass clk
}


void RF_RFPLL_VCO_Freq_Set(uint64_t ulFreqHz)
{
    uint64_t tmp;
    
    uint32_t ulDivN  = 0;
    uint32_t ulFracN = 0;
    
    tmp = (ulFreqHz << 24) / XTAL_CLK;
    
    ulDivN  = (tmp >> 24);
    ulFracN = tmp & 0xFFFFFF;
    
    RF_RFPLL_DivN_Set(ulDivN);
    RF_RFPLL_FracN_Set(ulFracN);
}

void RF_RFPLL_LO_Freq_Set(uint32_t ulFreqHz)
{
    uint64_t rfpll_vco_hz = 0;
    uint8_t  rfpll_div = 0;
    
    RF_RFPLL_Cal_Off();
	
	if(ulFreqHz >= RFPLL_DIV1_RANGE_LOW && ulFreqHz <= RFPLL_DIV1_RANGE_HIGH)
    {
        rfpll_div = 1;
    }
    else if(ulFreqHz >= RFPLL_DIV2_RANGE_LOW && ulFreqHz <= RFPLL_DIV2_RANGE_HIGH)
    {
        rfpll_div = 2;
    }
    else if(ulFreqHz >= RFPLL_DIV3_RANGE_LOW && ulFreqHz <= RFPLL_DIV3_RANGE_HIGH)
    {
        rfpll_div = 3;
    }
    else if(ulFreqHz >= RFPLL_DIV4_RANGE_LOW && ulFreqHz <= RFPLL_DIV4_RANGE_HIGH)
    {
        rfpll_div = 4;
    }
    
    RF_RFPLL_LO_Div_Set(rfpll_div - 1);
    
    rfpll_vco_hz = (uint64_t)((uint64_t)ulFreqHz << 2) * rfpll_div;
    
    if(rfpll_vco_hz > RFPLL_HIGH_LOW_BAND_Hz)
    {
        RF_RFPLL_VCO_High_Band_Set();
    }
    else
    {
        RF_RFPLL_VCO_Low_Band_Set();
    }
    
    RF_RFPLL_VCO_Freq_Set((uint64_t)rfpll_vco_hz);
    
    RF_RFPLL_On();
    
    // When power up, need to wait
    rf_drv_delay(0x10);
    
    RF_RFPLL_IBIAS_RC_Filter_En();
    RF_RFPLL_AVDDFILTER_EN_VCO_SEL_Set(0x01);
    
    rf_drv_delay(0x10);
    
    RF_RFPLL_Cal_On();
}

void RF_RFPLL_LO_Div_Set(uint8_t ucValue)
{
	REG_Bus_Field_Set(RFPLL_CNTL_BASE, RFPLL_CNTL_B_HIGH, RFPLL_CNTL_B_LOW, ucValue & 0x03);
}

void RF_RFPLL_VCO_High_Band_Set(void)
{
	REG_Bus_Field_Set(RFPLL_CNTL_BASE, RFPLL_CNTL_HB_MODE_HIGH, RFPLL_CNTL_HB_MODE_LOW, 0x1);
}

void RF_RFPLL_VCO_Low_Band_Set(void)
{
    REG_Bus_Field_Set(RFPLL_CNTL_BASE, RFPLL_CNTL_HB_MODE_HIGH, RFPLL_CNTL_HB_MODE_LOW, 0x0);
}

uint8_t RF_RFPLL_VCO_BandMode_Get(void)
{
    uint32_t ulBandMode;
    
    REG_Bus_Field_Get(RFPLL_CNTL_BASE, RFPLL_CNTL_HB_MODE_HIGH, RFPLL_CNTL_HB_MODE_LOW, &ulBandMode);
    
    return (uint8_t)ulBandMode;
}

extern unsigned int g_xtal_switch;
void RF_BBPLL_Freq_Set(uint32_t ulFreqHz)
{
	//uint64_t FreqHz = ulFreqHz * 10;
	//uint64_t tmp = 0;
	
	uint32_t ulDivN  = 0;
	uint32_t ulFracN = 0;
	uint32_t tmpFracN = 0;
	
	// Not found bbpll_ldo
	// Not found bbpll_ldo_filter_en
	// Not found bbpll_pu
	
	(void) ulFreqHz;

	RF_BBPLL_Cal_Off();
#if SAMPLE_RATE_MODE == 1
#if 0
	if(g_xtal_switch == 0)
	{
    	RF_BBPLL_DivN_Set(20);
    	RF_BBPLL_FracN_Set(0x666666);
	}
	else if (g_xtal_switch == 1)
	{
    	RF_BBPLL_DivN_Set(30);
    	RF_BBPLL_FracN_Set(0x211544);		
	}
#endif
	uint32_t xtalCLK = 38400000;
	uint32_t bbpllCLK = 391680000;

	if (g_xtal_switch == 0)
	{
		xtalCLK = 38400000;
	}
	else
	{
		xtalCLK = g_xtal_switch;
	}

	ulDivN	= (bbpllCLK*2)/xtalCLK;
	tmpFracN = (bbpllCLK*2)%xtalCLK;
	ulFracN = (uint32_t)((tmpFracN/(xtalCLK*1.0))*0xFFFFFF);

    RF_BBPLL_DivN_Set(ulDivN);
    RF_BBPLL_FracN_Set(ulFracN);		
#endif
    HWREGB(0xA011006F) &=0xFD;
	RF_BBPLL_Cal_On();
}

void RF_BBPLL_DivN_Set(uint32_t ulDivN)
{
	ulDivN &= 0x3FF;
	
	HWREGB(PRCM_BASE + 0x5A) = (uint8_t)ulDivN;
	
	HWREGB(PRCM_BASE + 0x5B) = (HWREGB(PRCM_BASE + 0x5B) & 0xFC) | (ulDivN >> 8);
}

uint32_t RF_BBPLL_DivN_Get(void)
{
    return ((HWREG(PRCM_BASE + 0x58) >> 16) & 0x3FF);
}

void RF_BBPLL_FracN_Set(uint32_t ulFracN)
{
	HWREG(PRCM_BASE + 0x54) = ulFracN;
}

uint32_t RF_BBPLL_FracN_Get(void)
{
    return HWREG(PRCM_BASE + 0x54);
}

void RF_RFPLL_DivN_Set(uint32_t ulDivN)
{
	ulDivN &= 0x3FF;
	
	HWREGB(DFE_REG_BASE + 0xD3) = (uint8_t)ulDivN;
	
	HWREGB(DFE_REG_BASE + 0xD4) = (HWREGB(DFE_REG_BASE + 0xD4) & 0xFC) | (ulDivN >> 8);
}

void RF_RFPLL_FracN_Set(uint32_t ulFracN)
{
	HWREGB(DFE_REG_BASE + 0xD7) = ulFracN & 0xFF;
	HWREGB(DFE_REG_BASE + 0xD8) = (ulFracN >> 8)  & 0xFF;
	HWREGB(DFE_REG_BASE + 0xD9) = (ulFracN >> 16) & 0xFF;
}

uint8_t RF_BBPLL_Lock_Status_Get(void)
{
	return ((HWREGB(PRCM_BASE + 0x0D) >> 2) & 0x1);
}

uint8_t RF_RFPLL_Lock_Status_Get(void)
{
    return ((HWREGB(REG_8_ANARFPLL4) >> ANARFPLL4_RFPLL_LOCK_DET_Pos) & 0x1);
}

void RF_BBPLL_CPLDO_Set(uint8_t ucValue)
{
	REG_Bus_Field_Set(BBPLL_CNTL_BASE, BBPLL_CNTL_RES_CNTL_LOLDO_HIGH, BBPLL_CNTL_RES_CNTL_LOLDO_LOW, ucValue & 0x0F);
}

void RF_BBPLL_Ref_Cycle_Cnt_Set(uint32_t ulValue)
{
	REG_Bus_Field_Set(BBPLL_CNTL_BASE, BBPLL_CNTL_REF_CYCLE_CNT_HIGH, BBPLL_CNTL_REF_CYCLE_CNT_LOW, ulValue & 0xFFFF);
}

void RF_BBPLL_ReadOut_Sel_Set(uint8_t ucValue)
{
	REG_Bus_Field_Set(BBPLL_CNTL_BASE, BBPLL_CNTL_READ_OUT_SEL_HIGH, BBPLL_CNTL_READ_OUT_SEL_LOW, ucValue & 0x01);
}

void RF_BBPLL_Lock_Threshold_Set(uint8_t ucValue)
{
	REG_Bus_Field_Set(BBPLL_CNTL_BASE, BBPLL_CNTL_LOCK_TH_BIN_HIGH, BBPLL_CNTL_LOCK_TH_BIN_LOW, ucValue & 0x1F);
}

void RF_RFLDO_CNTL_En(void)
{
	HWREGB(REG_8_ANACNTL_EN) |= ANACNTL_EN_RFLDO_CNTL_EN_Msk;
}

void RF_RFLDO_CNTL_Dis(void)
{
	HWREGB(REG_8_ANACNTL_EN) &= (uint8_t)(~ANACNTL_EN_RFLDO_CNTL_EN_Msk);
}

void RF_BBLDO_CNTL_En(void)
{
	HWREGB(REG_8_ANACNTL_EN) |= ANACNTL_EN_BBLDO_CNTL_EN_Msk;
}

void RF_BBLDO_CNTL_Dis(void)
{
	HWREGB(REG_8_ANACNTL_EN) &= (uint8_t)(~ANACNTL_EN_BBLDO_CNTL_EN_Msk);
}

void RF_RFLDO_Power_On(void)
{
    HWREGB(REG_8_ANARXPWRCTL1) |= ANARXPWRCTL1_RFLDO_PU_Msk;
}

void RF_RFLDO_Power_Off(void)
{
    HWREGB(REG_8_ANARXPWRCTL1) &= (uint8_t)(~ANARXPWRCTL1_RFLDO_PU_Msk);
}

void RF_BBLDO_Power_On(void)
{
	HWREGB(PRCM_BASE + 0x48) |= 0x40;
}

void RF_BBLDO_Power_Off(void)
{
	HWREGB(PRCM_BASE + 0x48) &= 0xBF;
}

void RF_AUXLDO_CNTL_En(void)
{
	HWREGB(REG_8_ANACNTL_EN) |= ANACNTL_EN_AUX_CNTL_EN_Msk;
}

void RF_AUXLDO_CNTL_Dis(void)
{
	HWREGB(REG_8_ANACNTL_EN) &= (uint8_t)(~ANACNTL_EN_AUX_CNTL_EN_Msk);
}

void RF_BBLDO_TRX_EN(void)
{
    HWREGB(REG_8_ANAAUX2) |= ANAAUX2_BBLDO_TRX_EN_Msk;
}

void RF_BBLDO_TRX_Dis(void)
{
    HWREGB(REG_8_ANAAUX2) &= (uint8_t)(~ANAAUX2_BBLDO_TRX_EN_Msk);
}

void RF_RFPLL_IBIAS_RC_Filter_En(void)
{
    REG_Bus_Field_Set(RFPLL_CNTL_BASE, RFPLL_CNTL_IBIAS_RC_FILTER_EN_HIGH, RFPLL_CNTL_IBIAS_RC_FILTER_EN_LOW, 0x1);
}

void RF_RFPLL_AVDDFILTER_EN_VCO_SEL_Set(uint8_t ucValue)
{
    REG_Bus_Field_Set(RFPLL_CNTL_BASE, RFPLL_CNTL_AVDDFILTER_EN_VCO_SEL_HIGH, RFPLL_CNTL_AVDDFILTER_EN_VCO_SEL_LOW, ucValue & 0x1);
}

void RF_RXHB_LNA_BIAS_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) |= ANARXPWRCTL2_RXHB_LNA_BIAS_PU_Msk;
}

void RF_RXHB_LNA_BIAS_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) &= (uint8_t)(~ANARXPWRCTL2_RXHB_LNA_BIAS_PU_Msk);
}

void RF_RXHB_LNA_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) |= ANARXPWRCTL2_RXHB_LNA_PU_Msk;
}

void RF_RXHB_LNA_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) &= (uint8_t)(~ANARXPWRCTL2_RXHB_LNA_PU_Msk);
}

void RF_RXHB_LNA_RST(void)
{

}

void RF_RXHB_LNA_EN(void)
{
    HWREGB(REG_8_ANARXCFG1) |= ANARXCFG1_RXHB_LNA_EN_Msk;
}

void RF_RXHB_LNA_Dis(void)
{
    HWREGB(REG_8_ANARXCFG1) &= (uint8_t)(~ANARXCFG1_RXHB_LNA_EN_Msk);
}

void RF_RXHB_LNA_BYP_EN(void)
{
    HWREGB(REG_8_RXHB_LNA_RO_ADJ) |= RXHB_LNA_RO_ADJ_RXHB_LNA_BYP_EN_Msk;
}

void RF_RXHB_LNA_BYP_Dis(void)
{
    HWREGB(REG_8_RXHB_LNA_RO_ADJ) &= (uint8_t)(~RXHB_LNA_RO_ADJ_RXHB_LNA_BYP_EN_Msk);
}

void RF_RXHB_LNA_RST_BYP(void)
{
    HWREGB(REG_8_RXHB_LNA_RO_ADJ) |= RXHB_LNA_RO_ADJ_RXHB_LNA_RST_BYP_Msk;
}

void RF_RXHB_LNA_RST_BYPVAL(void)
{
    HWREGB(REG_8_RXHB_LNA_RO_ADJ) |= RXHB_LNA_RO_ADJ_RXHB_LNA_RST_BYPVAL_Msk;
}

void RF_RXHB_LO_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) |= ANARXPWRCTL2_RXHB_LO_PU_Msk;
}
void RF_RXHB_LO_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) &= (uint8_t)(~ANARXPWRCTL2_RXHB_LO_PU_Msk);
}

void RF_RXHB_GM_BIAS_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) |= ANARXPWRCTL2_RXHB_GM_BIAS_PU_Msk;
}

void RF_RXHB_GM_BIAS_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) &= (uint8_t)(~ANARXPWRCTL2_RXHB_GM_BIAS_PU_Msk);
}

void RF_RXLB_LNA_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) |= ANARXPWRCTL2_RXLB_LNA_PU_Msk;
}

void RF_RXLB_LNA_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) &= (uint8_t)(~ANARXPWRCTL2_RXLB_LNA_PU_Msk);
}

void RF_RXLB_LO_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) |= ANARXPWRCTL2_RXLB_LO_PU_Msk;
}

void RF_RXLB_LO_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) &= (uint8_t)(~ANARXPWRCTL2_RXLB_LO_PU_Msk);
}

void RF_RXLB_GM_BIAS_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) |= ANARXPWRCTL2_RXLB_GM_BIAS_PU_Msk;
}

void RF_RXLB_GM_BIAS_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL2) &= (uint8_t)(~ANARXPWRCTL2_RXLB_GM_BIAS_PU_Msk);
}

void RF_RXBB_TIA_Power_on(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) |= ANARXPWRCTL1_RXBB_TIA_PU_Msk;
}

void RF_RXBB_TIA_Power_off(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) &= (~ANARXPWRCTL1_RXBB_TIA_PU_Msk);
}

void RF_RXBB_PGA_BIAS_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) |= ANARXPWRCTL1_RXBB_PGA_BIAS_PU_Msk;
}

void RF_RXBB_PGA_BIAS_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) &= (uint8_t)(~ANARXPWRCTL1_RXBB_PGA_BIAS_PU_Msk);
}

void RF_RXBB_PGA_I_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) |= ANARXPWRCTL1_RXBB_PGA_PU_I_Msk;
}

void RF_RXBB_PGA_I_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) &= (uint8_t)(~ANARXPWRCTL1_RXBB_PGA_PU_I_Msk);
}

void RF_RXBB_PGA_Q_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) |= ANARXPWRCTL1_RXBB_PGA_PU_Q_Msk;
}

void RF_RXBB_PGA_Q_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) &= (uint8_t)(~ANARXPWRCTL1_RXBB_PGA_PU_Q_Msk);
}

void RF_RXBB_ADC_BIAS_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) |= ANARXPWRCTL1_RXBB_ADC_BIAS_PU_Msk;
}

void RF_RXBB_ADC_BIAS_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) &= (uint8_t)(~ANARXPWRCTL1_RXBB_ADC_BIAS_PU_Msk);
}

void RF_RXBB_ADC_Power_On(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) |= ANARXPWRCTL1_RXBB_ADC_PU_Msk;
}

void RF_RXBB_ADC_Power_Off(void)
{
	HWREGB(REG_8_ANARXPWRCTL1) &= (uint8_t)(~ANARXPWRCTL1_RXBB_ADC_PU_Msk);
}

void RF_RXBB_ADC_RSTB(void)
{
	
}

void RF_RXBB_ADC_RST_BYP_EN(void)
{
    HWREGB(REG_8_ANARXCFG1) |= ANARXCFG1_RXBB_ADC_RST_BYP_Msk;
}

void RF_RXBB_ADC_RST_BYP_DIS(void)
{
    HWREGB(REG_8_ANARXCFG1) &= (uint8_t)(~ANARXCFG1_RXBB_ADC_RST_BYP_Msk);
}

void RF_RXBB_ADC_RSTB_BYPVAL_EN(void)
{
    HWREGB(REG_8_ANARXCFG1) |= ANARXCFG1_RXBB_ADC_RSTB_BYPVAL_Msk;
}

void RF_RXBB_ADC_RSTB_BYPVAL_DIS(void)
{
    HWREGB(REG_8_ANARXCFG1) &= (uint8_t)(~ANARXCFG1_RXBB_ADC_RSTB_BYPVAL_Msk);
}

uint8_t RF_TXLDO_PALDO_BYPASS_Flag_Get(void)
{
    return ((HWREGB(REG_8_ANATXCFG23) >> ANATXCFG23_TXLDO_PALDO_BYP_FLAG_Pos) & 0x01);
}

void RF_TXLDO_PALDO_CNTL_En(void)
{
	HWREGB(REG_8_ANACNTL_EN) |= ANACNTL_EN_TXLDO_PALDO_CNTL_EN_Msk;
}

void RF_TXLDO_PALDO_CNTL_Dis(void)
{
	HWREGB(REG_8_ANACNTL_EN) &= (uint8_t)(~ANACNTL_EN_TXLDO_PALDO_CNTL_EN_Msk);
}

void RF_TXLDO_PALDO_Power_On(void)
{
    HWREGB(REG_8_ANATXPWRCTL) |= ANATXPWRCTL_TXLDO_PALDO_PU_Msk;
}
void RF_TXLDO_PALDO_Power_Off(void)
{
    HWREGB(REG_8_ANATXPWRCTL) &= (uint8_t)(~ANATXPWRCTL_TXLDO_PALDO_PU_Msk);
}

void RF_TXHB_TSSI_Power_On(void)
{
    HWREGB(REG_8_ANATXPWRCTL) |= ANATXPWRCTL_TXHB_TSSI_PU_Msk;
}

void RF_TXHB_TSSI_Power_Off(void)
{
    HWREGB(REG_8_ANATXPWRCTL) &= (uint8_t)(~ANATXPWRCTL_TXHB_TSSI_PU_Msk);
}

void RF_TXHB_BIAS_Power_On(void)
{
    HWREGB(REG_8_TXHB_BIAS_PAD_PU) |= TXHB_BIAS_PAD_PU_TXHB_BIAS_PU_Msk;
}

void RF_TXHB_BIAS_Power_Off(void)
{
    HWREGB(REG_8_TXHB_BIAS_PAD_PU) &= (uint8_t)(~TXHB_BIAS_PAD_PU_TXHB_BIAS_PU_Msk);
}

//void RF_TXHB_MX_GM_BIAS_RST(void);

void RF_TXHB_MX_Power_On(void)
{
    HWREGB(REG_8_TXHB_BIAS_PAD_PU) |= TXHB_BIAS_PAD_PU_TXHB_MX_PU_Msk;
}

void RF_TXHB_MX_Power_Off(void)
{
    HWREGB(REG_8_TXHB_BIAS_PAD_PU) &= (uint8_t)(~TXHB_BIAS_PAD_PU_TXHB_MX_PU_Msk);
}

void RF_TXHB_MX_LO_BIAS_RST(void)
{
    HWREGB(REG_8_TXHB_BIAS_PAD_PU) |= TXHB_BIAS_PAD_PU_TXHB_MX_LO_BIAS_RST_Msk;
}

void RF_TXHB_PAD_Power_On(void)
{
    HWREGB(REG_8_TXHB_BIAS_PAD_PU) |= TXHB_BIAS_PAD_PU_TXHB_PAD_PU_Msk;
}

void RF_TXHB_PAD_Power_Off(void)
{
    HWREGB(REG_8_TXHB_BIAS_PAD_PU) &= (uint8_t)(~TXHB_BIAS_PAD_PU_TXHB_PAD_PU_Msk);
}

void RF_TXHB_PA_Power_On(void)
{
    HWREGB(REG_8_ANATXPWRCTL) |= TXHB_BIAS_PAD_PU_TXHB_PA_PU_Msk;
}

void RF_TXHB_PA_Power_Off(void)
{
    HWREGB(REG_8_ANATXPWRCTL) &= (uint8_t)(~TXHB_BIAS_PAD_PU_TXHB_PA_PU_Msk);
}

//void RF_TXHB_On(void);
//void RF_TXHB_Off(void);

void RF_TXLB_TSSI_Power_On(void)
{
    HWREGB(REG_8_ANATXPWRCTL) |= ANATXPWRCTL_TXLB_TSSI_PU_Msk;
}

void RF_TXLB_TSSI_Power_Off(void)
{
    HWREGB(REG_8_ANATXPWRCTL) &= (uint8_t)(~ANATXPWRCTL_TXLB_TSSI_PU_Msk);
}

void RF_TXLB_BIAS_Power_On(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) |= TXLB_BIAS_PA_PU_TXLB_BIAS_PU_Msk;
}

void RF_TXLB_BIAS_Power_Off(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) &= (uint8_t)(~TXLB_BIAS_PA_PU_TXLB_BIAS_PU_Msk);
}

void RF_TXLB_MX_LO_BIAS_RST(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) |= TXLB_BIAS_PA_PU_TXLB_MX_LO_BIAS_RST_Msk;
}
void RF_TXLB_MX_Power_On(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) |= TXLB_BIAS_PA_PU_TXLB_MX_PU_Msk;
}

void RF_TXLB_MX_Power_Off(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) &= (uint8_t)(~TXLB_BIAS_PA_PU_TXLB_MX_PU_Msk);
}

void RF_TXLB_PA_Power_On(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) |= TXLB_BIAS_PA_PU_TXLB_PA_PU_Msk;
}
void RF_TXLB_PA_Power_Off(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) &= (uint8_t)(~TXLB_BIAS_PA_PU_TXLB_PA_PU_Msk);
}
void RF_TXLB_PA_GM_BIAS_RST(void)
{
    HWREGB(REG_8_TXLB_BIAS_PA_PU) |= TXLB_BIAS_PA_PU_TXLB_PA_GM_BIAS_RST_Msk;
}
//void RF_TXLB_On(void);
//void RF_TXLB_Off(void);

void RF_TXBB_Power_On(void)
{
    HWREGB(REG_8_TXBB_PU) |= TXBB_PU_TXBB_PU_Msk;
}

void RF_TXBB_Power_Off(void)
{
    HWREGB(REG_8_TXBB_PU) &= (uint8_t)(~TXBB_PU_TXBB_PU_Msk);
}

void RF_TXBB_RSTB(void)
{
    HWREGB(REG_8_TXBB_PU) |= TXBB_PU_TXBB_RSTB_Msk;
}

//void RF_TXBB_On(void);
//void RF_TXBB_Off(void);

//void RF_TX_CTUNE_Set(uint8_t ModuleId, uint8_t ctune_value);

// [191:188] = {tx_l, rx_l, rx_h, tx_h}
void RF_Band_Enable_Set(uint8_t ucValue)
{
	REG_Bus_Field_Set(RFPLL_CNTL_BASE, RFPLL_CNTL_EN_TXL_HIGH, RFPLL_CNTL_EN_TXH_LOW, ucValue & 0x0F);
}

void RF_RX_LB_NF_Set(void)
{
    //BITS    0x2601f111    5    5    1          //Comment increase pga op2 current for linearity
    //BYTE    0x2601f10c    0x17                 //Comment setup rxlb lna cap setting
    //BITS    0x2601f104    3    0    1111       //Comment increase rxlb lna current
    //BITS    0x2601f104    32    28    11111    //Comment increase gm idac aux_n 
    //BITS    0x2601f104    37    33    11111    //Comment increase gm idac aux_p
    //BITS    0x2601f08d    3    0    1111       //Comment increase gm gc to maximum
    
    REG_Bus_Field_Set(DFE_REG_BASE + 0x111,  5,  5, 0x01);
    HWREGB(DFE_REG_BASE + 0x10c) = 0x17;
    REG_Bus_Field_Set(DFE_REG_BASE + 0x104,  3,  0, 0x0F);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x104, 32, 28, 0x1F);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x104, 37, 33, 0x1F);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x08d,  3,  0, 0x0F);
}

void RF_RX_On(uint32_t RxLOFreqHz)
{
    uint8_t band_mode = RFPLL_BAND_MODE_LOW;
    
    RF_RFLDO_CNTL_En();
    
    RF_RFPLL_LO_Freq_Set(RxLOFreqHz);
    
    rf_drv_delay(0x10);
    
    band_mode = RF_RFPLL_VCO_BandMode_Get();
    
    if(band_mode == RFPLL_BAND_MODE_LOW)
    {
        RF_Band_Enable_Set(BAND_ENABLE_RXL);
    }
    else if(band_mode == RFPLL_BAND_MODE_HIGH)
    {
        RF_Band_Enable_Set(BAND_ENABLE_RXH);
    }
    
    RF_RFLDO_CNTL_En();
    RF_BBLDO_CNTL_En();
    
    RF_RFLDO_Power_On();
    RF_BBLDO_Power_On();
    
    rf_drv_delay(0x10);
    
    RF_BBLDO_TRX_EN();
    
    RF_RXBB_TIA_Power_on();
    RF_RXBB_PGA_BIAS_Power_On();
    RF_RXBB_PGA_I_Power_On();
    RF_RXBB_PGA_Q_Power_On();

    rf_drv_delay(0x10);

    if(band_mode == RFPLL_BAND_MODE_LOW)
    {
        RF_RXLB_LNA_Power_On();
        RF_RXLB_LO_Power_On();
        RF_RXLB_GM_BIAS_Power_On();
    }
    else if(band_mode == RFPLL_BAND_MODE_HIGH)
    {
        RF_RXHB_LNA_BIAS_Power_On();
        RF_RXHB_LNA_Power_On();
        RF_RXHB_LO_Power_On();
        RF_RXHB_GM_BIAS_Power_On();
    }
    
    RF_RX_LB_NF_Set();
    
    RF_RXBB_ADC_BIAS_Power_On();
    
    rf_drv_delay(0x10);
    
    RF_RXBB_ADC_Power_On();
    
    rf_drv_delay(0x10);
    
    RF_RXBB_ADC_RST_BYP_EN();
    RF_RXBB_ADC_RSTB_BYPVAL_EN();
}

void RF_RX_Capture_ADC_Input(void)
{
    //BITS    0x2601f0b9     5     4    00   //Comment txbb test mux on
    //BITS    0x2601f10e    34    32    110  //Comment connect pga output to test pin<3:1>
    //BITS    0x2601f10e    31    31    0    //Comment connect pga output to test pin<0>
    //BITS    0x2601f10e    26    26    0    //Comment lpf model disable
    //BITS    0x2601f10e    16    16    0    //Comment pga swap iq disable  
    
    REG_Bus_Field_Set(DFE_REG_BASE + 0x0b9,  5,  4, 0);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x10e, 34, 32, 6);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x10e, 31, 31, 0);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x10e, 26, 26, 0);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x10e, 16, 16, 0);
}


void RF_TX_On(uint32_t TxLOFreqHz)
{
    uint8_t band_mode = RFPLL_BAND_MODE_LOW;
    
    RF_RFLDO_CNTL_En();
    
    RF_RFPLL_LO_Freq_Set(TxLOFreqHz);
    
    rf_drv_delay(0x10);
    
    band_mode = RF_RFPLL_VCO_BandMode_Get();
    
    if(band_mode == RFPLL_BAND_MODE_LOW)
    {
        RF_Band_Enable_Set(BAND_ENABLE_TXL);
    }
    else if(band_mode == RFPLL_BAND_MODE_HIGH)
    {
        RF_Band_Enable_Set(BAND_ENABLE_TXH);
    }
    
    RF_TXLDO_PALDO_CNTL_En();
    RF_BBLDO_CNTL_En();
    
    RF_RFLDO_Power_On();
    RF_BBLDO_Power_On();
    
    rf_drv_delay(0x100);
    
    RF_TXBB_Power_On();
    
    RF_TXBB_RSTB();
    
    rf_drv_delay(0x100);
    
    RF_BBLDO_TRX_EN();
    
    RF_TXLDO_PALDO_Power_On();

    rf_drv_delay(0x10);

    if(band_mode == RFPLL_BAND_MODE_LOW)
    {
        RF_TXLB_BIAS_Power_On();
        
        rf_drv_delay(0x50);     // Maql: These delay logic may be deleted later.
        
        RF_TXLB_MX_Power_On();
        
        rf_drv_delay(0x50);
        
        RF_TXLB_PA_Power_On();
        
        rf_drv_delay(0x50);
    }
    else if(band_mode == RFPLL_BAND_MODE_HIGH)
    {
        RF_TXHB_BIAS_Power_On();
        
        rf_drv_delay(0x50);
        
        RF_TXHB_MX_Power_On();
        RF_TXHB_PAD_Power_On();
        
        rf_drv_delay(0x50);
        
        RF_TXHB_PA_Power_On();
        
        rf_drv_delay(0x50);
    }
    
    //BITS    0x2601f0b5    7    4    1011      //Comment tune txlb mx ctune
    //BITS    0x2601f0b7    7    4    0011      //Comment tune txlb pa ctune
    //BITS    0x2601f0c8    80    77    1111    //Comment maximize txlb mx gm gc
    
    REG_Bus_Field_Set(DFE_REG_BASE + 0x0b5,  7,  4, 0x0B);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x0b7,  7,  4, 0x03);
    REG_Bus_Field_Set(DFE_REG_BASE + 0x0c8, 80, 77, 0x0F);
}

void RF_TX_Capture_DAC_Output(void)
{
    //BITS    0x2601f0b9    5    4    11    //Comment txbb test mux on
    
    REG_Bus_Field_Set(DFE_REG_BASE + 0x0b9,  5,  4, 0x03);
}
#if 0
void DFE_RX_DDFS_LO_FCW_Set(int32_t ddfs_FreqHz)
{
    uint32_t reg_value;
    
    // Qiaohui
    
    HWREG(DFE_RX_LO_FCW) = reg_value;
}

int32_t DFE_RX_DDFS_LO_FCW_Get(void)
{
    volatile uint32_t reg_value;
    int32_t ddfs_FreqHz;
    
    reg_value = HWREG(DFE_RX_LO_FCW);
    
    // Qiaohui
    
    return ddfs_FreqHz;
}

void DFE_RX_DDFS_LO_PHS_ADJ_Set(int32_t phase_offset)
{
    uint32_t reg_value;
    
    // Qiaohui
    
    HWREG(DFE_RX_LO_PHS_ADJ) = reg_value;
}

int32_t DFE_RX_DDFS_LO_PHS_ADJ_Get(void)
{
    volatile uint32_t reg_value;
    int32_t phase_offset;
    
    reg_value = HWREG(DFE_RX_LO_PHS_ADJ);
    
    // Qiaohui
    
    return phase_offset;
}

void DFE_TX_DDFS_LO_FCW_Set(int32_t ddfs_FreqHz)
{
    uint32_t reg_value;
    
    // Qiaohui
    
    HWREG(DFE_TX_LO_FCW) = reg_value;
}

int32_t DFE_TX_DDFS_LO_FCW_Get(void)
{
    volatile uint32_t reg_value;
    int32_t ddfs_FreqHz;
    
    reg_value = HWREG(DFE_TX_LO_FCW);
    
    // Qiaohui
    
    return ddfs_FreqHz;
}

void DFE_TX_DDFS_LO_PHS_ADJ_Set(int32_t phase_offset)
{
    uint32_t reg_value;
    
    // Qiaohui
    
    HWREG(DFE_TX_LO_PHS_ADJ) = reg_value;
}

int32_t DFE_TX_DDFS_LO_PHS_ADJ_Get(void)
{
    volatile uint32_t reg_value;
    int32_t phase_offset;
    
    reg_value = HWREG(DFE_TX_LO_PHS_ADJ);
    
    // Qiaohui
    
    return phase_offset;
}
#endif
void DFE_RX_Single_Gain_Set(uint8_t GainId, uint32_t GainValue)
{
    if(GainId == RX_HB_GAIN_LNA_GC)
    {
        HWREGB(REG_8_RXHB_LNA_GC) = GainValue & 0x1F;
    }
    else if(GainId == RX_HB_GAIN_LNA_RO_ADJ)
    {
        HWREGB(REG_8_RXHB_LNA_RO_ADJ) = (HWREGB(REG_8_RXHB_LNA_RO_ADJ) & 0xF0) | (GainValue & 0xF);
    }
    else if(GainId == RX_HB_GAIN_MIXER_GC)
    {
        HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) = (HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) & 0xF0) | (GainValue & 0xF);
    }
    else if(GainId == RX_HB_GAIN_MIXER_SD_ADJ)
    {
        HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) = (HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) & 0x0F) | ((GainValue & 0xF) << 4);
    }
    else if(GainId == RX_LB_GAIN_LNA_GC)
    {
        HWREGB(REG_8_RXLB_LNA_GC) = GainValue & 0x7;
    }
    else if(GainId == RX_LB_GAIN_MIXER_GC)
    {
        HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) = (HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) & 0xF0) | (GainValue & 0xF);
    }
    else if(GainId == RX_LB_GAIN_MIXER_SD_ADJ)
    {
        HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) = (HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) & 0x0F) | ((GainValue & 0xF) << 4);
    }
    else if(GainId == RX_BB_GAIN_PGA_GC)
    {
        HWREGB(REG_8_RXBB_PGA_GC) = GainValue & 0x1F;
    }
    else if(GainId == RX_DFE_GAIN_CIC)
    {
        HWREG(DFE_RX_GAIN_CIC) = GainValue & 0x1FFFF;
    }
    else if(GainId == RX_DFE_GAIN_ACI)
    {
        HWREGH(DFE_RX_GAIN_ACI) = GainValue & 0xFFFF;
    }
}

uint32_t DFE_RX_Single_Gain_Get(uint8_t GainId)
{
    if(GainId == RX_HB_GAIN_LNA_GC)
    {
        return (HWREGB(REG_8_RXHB_LNA_GC) & 0x1F);
    }
    else if(GainId == RX_HB_GAIN_LNA_RO_ADJ)
    {
        return (HWREGB(REG_8_RXHB_LNA_RO_ADJ) & 0xF);
    }
    else if(GainId == RX_HB_GAIN_MIXER_GC)
    {
        return (HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) & 0xF);
    }
    else if(GainId == RX_HB_GAIN_MIXER_SD_ADJ)
    {
        return ((HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) >> 4) & 0xF);
    }
    else if(GainId == RX_LB_GAIN_LNA_GC)
    {
        return (HWREGB(REG_8_RXLB_LNA_GC) & 0x7);
    }
    else if(GainId == RX_LB_GAIN_MIXER_GC)
    {
        return (HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) & 0xF);
    }
    else if(GainId == RX_LB_GAIN_MIXER_SD_ADJ)
    {
        return ((HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) >> 4) & 0xF);
    }
    else if(GainId == RX_BB_GAIN_PGA_GC)
    {
        return (HWREGB(REG_8_RXBB_PGA_GC) & 0x1F);
    }
    else if(GainId == RX_DFE_GAIN_CIC)
    {
        return (HWREG(DFE_RX_GAIN_CIC) & 0x1FFFF);
    }
    else if(GainId == RX_DFE_GAIN_ACI)
    {
        return (HWREGH(DFE_RX_GAIN_ACI) & 0xFFFF);
    }
    
    return 0;
}

void DFE_RX_Single_Gain_Set_By_dBTen(uint8_t GainId, int16_t dBTen)
{
    uint32_t GainReg1;
    uint32_t GainReg2;
    
    uint8_t i;

    if(GainId == RX_HB_GAIN_LNA)
    {
        if(dBTen >= GAIN_TABLE_RX_HB_LNA[0].dBTen)
        {
            GainReg1 = GAIN_TABLE_RX_HB_LNA[0].GainReg1;
            GainReg2 = GAIN_TABLE_RX_HB_LNA[0].GainReg2;
        }
        else
        {
            for(i = 0; i < GAIN_TABLE_SIZE_RX_HB_LNA - 1; i++)
            {
                if((dBTen <= GAIN_TABLE_RX_HB_LNA[i].dBTen) && (dBTen > GAIN_TABLE_RX_HB_LNA[i+1].dBTen))
                {
                    GainReg1 = GAIN_TABLE_RX_HB_LNA[i].GainReg1;
                    GainReg2 = GAIN_TABLE_RX_HB_LNA[i].GainReg2;
                    
                    break;
                }
            }
            
            if(i == GAIN_TABLE_SIZE_RX_HB_LNA - 1)
            {
                GainReg1 = GAIN_TABLE_RX_HB_LNA[i].GainReg1;
                GainReg2 = GAIN_TABLE_RX_HB_LNA[i].GainReg2;
            }
        }
        
        HWREGB(REG_8_RXHB_LNA_GC) = GainReg1 & 0x1F;

        HWREGB(REG_8_RXHB_LNA_RO_ADJ) = (HWREGB(REG_8_RXHB_LNA_RO_ADJ) & 0xF0) | (GainReg2 & 0xF);
        
        if(GainReg1 & 0x1F)
        {
            RF_RXHB_LNA_BYP_Dis();
            
            RF_RXHB_LNA_Power_On();
        }
        else
        {
            RF_RXHB_LNA_BYP_EN();
            
            RF_RXHB_LNA_Power_Off();
        }
    }
    else if(GainId == RX_LB_GAIN_LNA)
    {
        if(dBTen >= GAIN_TABLE_RX_LB_LNA[0].dBTen)
        {
            GainReg1 = GAIN_TABLE_RX_LB_LNA[0].GainReg;
        }
        else
        {
            for(i = 0; i < GAIN_TABLE_SIZE_RX_LB_LNA - 1; i++)
            {
                if((dBTen <= GAIN_TABLE_RX_LB_LNA[i].dBTen) && (dBTen > GAIN_TABLE_RX_LB_LNA[i+1].dBTen))
                {
                    GainReg1 = GAIN_TABLE_RX_LB_LNA[i].GainReg;
                    
                    break;
                }
            }
            
            if(i == GAIN_TABLE_SIZE_RX_LB_LNA - 1)
            {
                GainReg1 = GAIN_TABLE_RX_LB_LNA[i].GainReg;
            }
        }
        
        HWREGB(REG_8_RXLB_LNA_GC) = GainReg1 & 0x7;
    }
    else if(GainId == RX_HB_GAIN_MIXER || GainId == RX_LB_GAIN_MIXER)
    {
        if(dBTen >= GAIN_TABLE_RX_MIXER[0].dBTen)
        {
            GainReg1 = GAIN_TABLE_RX_MIXER[0].GainReg1;
            GainReg2 = GAIN_TABLE_RX_MIXER[0].GainReg2;
        }
        else
        {
            for(i = 0; i < GAIN_TABLE_SIZE_RX_MIXER - 1; i++)
            {
                if((dBTen <= GAIN_TABLE_RX_MIXER[i].dBTen) && (dBTen > GAIN_TABLE_RX_MIXER[i+1].dBTen))
                {
                    GainReg1 = GAIN_TABLE_RX_MIXER[i].GainReg1;
                    GainReg2 = GAIN_TABLE_RX_MIXER[i].GainReg2;
                    
                    break;
                }
            }
            
            if(i == GAIN_TABLE_SIZE_RX_MIXER - 1)
            {
                GainReg1 = GAIN_TABLE_RX_MIXER[i].GainReg1;
                GainReg2 = GAIN_TABLE_RX_MIXER[i].GainReg2;
            }
        }
        
        if(GainId == RX_HB_GAIN_MIXER)
        {
            HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) = ((uint8_t)(GainReg2 << 4) & 0xF0) | ((uint8_t)GainReg1 & 0x0F);
        }
        else
        {
            HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) = ((uint8_t)(GainReg2 << 4) & 0xF0) | ((uint8_t)GainReg1 & 0x0F);
        }
    }
    else if(GainId == RX_BB_GAIN_PGA_GC)
    {
        dBTen = dBTen / 10;
        
        if(dBTen >= -6 && dBTen <= 24)
        {
            GainReg1 = (uint8_t)(dBTen + 7);
            
            HWREGB(REG_8_RXBB_PGA_GC) = GainReg1 & 0x1F;
        }
    }
    else if(GainId == RX_DFE_GAIN_CIC)
    {
        // Qiaohui
        
//        HWREG(DFE_RX_GAIN_CIC) = GainReg1 & 0x1FFFF;
    }
    else if(GainId == RX_DFE_GAIN_ACI)
    {
        // Qiaohui
        
//        HWREGH(DFE_RX_GAIN_ACI) = GainReg1 & 0xFFFF;
    }
}


int16_t  DFE_RX_Single_Gain_Get_dBTen(uint8_t GainId)
{
    int16_t GainReg1;
    int16_t GainReg2;
    
    uint8_t rxhb_lna_bypass;
    uint8_t rxhb_lna_pu;
    uint8_t i;
    
    if(GainId == RX_HB_GAIN_LNA)
    {
        rxhb_lna_bypass = HWREGB(REG_8_RXHB_LNA_RO_ADJ) & RXHB_LNA_RO_ADJ_RXHB_LNA_BYP_EN_Msk;
        rxhb_lna_pu     = HWREGB(REG_8_ANARXPWRCTL2)    & ANARXPWRCTL2_RXHB_LNA_PU_Msk;
        
        GainReg1 = HWREGB(REG_8_RXHB_LNA_GC) & 0x1F;
        GainReg2 = HWREGB(REG_8_RXHB_LNA_RO_ADJ) & 0x0F;
        
        for(i = 0; i < GAIN_TABLE_SIZE_RX_HB_LNA - 1; i++)
        {
            if((GainReg1 == GAIN_TABLE_RX_HB_LNA[i].GainReg1) && (GainReg2 == GAIN_TABLE_RX_HB_LNA[i].GainReg2))
            {
                if((GainReg1 == 0) && (rxhb_lna_bypass == 1) && (rxhb_lna_pu == 0))
                {
                    return GAIN_TABLE_RX_HB_LNA[i].dBTen;
                }
            }
        }
    }
    else if(GainId == RX_LB_GAIN_LNA)
    {
        GainReg1 = HWREGB(REG_8_RXLB_LNA_GC) & 0x7;
        
        for(i = 0; i < GAIN_TABLE_SIZE_RX_LB_LNA; i++)
        {
            if(GainReg1 == GAIN_TABLE_RX_LB_LNA[i].GainReg)
            {
                return GAIN_TABLE_RX_LB_LNA[i].dBTen;
            }
        }
    }
    else if(GainId == RX_HB_GAIN_MIXER || GainId == RX_LB_GAIN_MIXER)
    {
        if(GainId == RX_HB_GAIN_MIXER)
        {
            GainReg1 =  HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) & 0x0F;
            GainReg2 = (HWREGB(REG_8_RXHB_GM_GC_SD_ADJ) >> 4) & 0x0F;
        }
        else
        {
            GainReg1 =  HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) & 0x0F;
            GainReg2 = (HWREGB(REG_8_RXLB_GM_GC_SD_ADJ) >> 4) & 0x0F;
        }
        
        for(i = 0; i < GAIN_TABLE_SIZE_RX_MIXER; i++)
        {
            if((GainReg1 == GAIN_TABLE_RX_MIXER[i].GainReg1) && (GainReg2 == GAIN_TABLE_RX_MIXER[i].GainReg2))
            {
                return GAIN_TABLE_RX_MIXER[i].dBTen;
            }
        }
    }
    else if(GainId == RX_BB_GAIN_PGA_GC)
    {
        GainReg1 = HWREGB(REG_8_RXBB_PGA_GC) & 0x1F;
        
        return (((int16_t)GainReg1 - 7) * 10);
    }
    else if(GainId == RX_DFE_GAIN_CIC)
    {
        // Qiaohui
        
//        GainReg1 = HWREG(DFE_RX_GAIN_CIC) & 0x1FFFF;
    }
    else if(GainId == RX_DFE_GAIN_ACI)
    {
        // Qiaohui
        
//        GainReg1 = HWREGH(DFE_RX_GAIN_ACI) & 0xFFFF;
    }
    
    return 0;
}

void DFE_TX_Single_Gain_Set(uint8_t GainId, uint32_t GainValue)
{
    if(GainId == TX_HB_MX_CAS_GC)
    {
        HWREGB(REG_8_TXHB_MX_CAS_GC) = GainValue & 0x3F;
    }
    else if(GainId == TX_HB_PAD_GC)
    {
        HWREGB(REG_8_TXHB_PAD_GC) = GainValue & 0xFF;
    }
    else if(GainId == TX_HB_PA_GC)
    {
        HWREGB(REG_8_TXHB_PA_GC) = GainValue & 0xFF;
    }
    else if(GainId == TX_LB_MX_CAS_GC)
    {
        HWREGB(REG_8_TXLB_MX_CAS_GC) = GainValue & 0x3F;
    }
    else if(GainId == TX_LB_PA_GC)
    {
        HWREGB(REG_8_TXLB_PA_GC) = GainValue & 0xFF;
    }
    else if(GainId == TX_BB_GC)
    {
        HWREGB(REG_8_TXBB_GC) = GainValue & 0x0F;
    }
}

uint32_t DFE_TX_Single_Gain_Get(uint8_t GainId)
{
    if(GainId == TX_HB_MX_CAS_GC)
    {
        return (HWREGB(REG_8_TXHB_MX_CAS_GC) & 0x3F);
    }
    else if(GainId == TX_HB_PAD_GC)
    {
        return (HWREGB(REG_8_TXHB_PAD_GC)& 0xFF);
    }
    else if(GainId == TX_HB_PA_GC)
    {
        return (HWREGB(REG_8_TXHB_PA_GC) & 0xFF);
    }
    else if(GainId == TX_LB_MX_CAS_GC)
    {
        return (HWREGB(REG_8_TXLB_MX_CAS_GC) & 0x3F);
    }
    else if(GainId == TX_LB_PA_GC)
    {
        return (HWREGB(REG_8_TXLB_PA_GC) & 0xFF);
    }
    else if(GainId == TX_BB_GC)
    {
        return (HWREGB(REG_8_TXBB_GC) & 0x0F);
    }
    
    return 0;
}

void DFE_TX_Single_Gain_Set_By_dBTen(uint8_t GainId, int16_t dBTen)
{
    uint8_t GainReg1;
//    uint8_t GainReg2;
    
    uint8_t i;
    
    if(GainId == TX_HB_MX_CAS_GC)
    {
        i = RF_TABLE_TYPE4_Search_by_dBTen(dBTen, (TableType4 *)&GAIN_TABLE_TX_HB_MIXER[0], GAIN_TABLE_SIZE_TX_HB_MIXER);

        GainReg1 = GAIN_TABLE_TX_HB_MIXER[i].GainReg;
        
        HWREGB(REG_8_TXHB_MX_CAS_GC) = GainReg1 & 0x3F;
    }
    else if(GainId == TX_HB_PAD_GC)
    {
        i = RF_TABLE_TYPE4_Search_by_dBTen(dBTen, (TableType4 *)&GAIN_TABLE_TX_HB_PAD[0], GAIN_TABLE_SIZE_TX_HB_PAD);

        GainReg1 = GAIN_TABLE_TX_HB_PAD[i].GainReg;
        
        HWREGB(REG_8_TXHB_PAD_GC) = GainReg1 & 0xFF;
    }
    else if(GainId == TX_HB_PA_GC)
    {
        i = RF_TABLE_TYPE4_Search_by_dBTen(dBTen, (TableType4 *)&GAIN_TABLE_TX_HB_PA[0], GAIN_TABLE_SIZE_TX_HB_PA);

        GainReg1 = GAIN_TABLE_TX_HB_PA[i].GainReg;
        
        HWREGB(REG_8_TXHB_PA_GC) = GainReg1 & 0xFF;
    }
    else if(GainId == TX_LB_MX_CAS_GC)
    {
        i = RF_TABLE_TYPE3_Search_by_dB(dBTen/10, (TableType3 *)&GAIN_TABLE_TX_LB_MIXER[0], GAIN_TABLE_SIZE_TX_LB_MIXER);

        GainReg1 = GAIN_TABLE_TX_LB_MIXER[i].GainReg;
        
        HWREGB(REG_8_TXLB_MX_CAS_GC) = GainReg1 & 0x3F;
    }
    else if(GainId == TX_LB_PA_GC)
    {
        i = RF_TABLE_TYPE3_Search_by_dB(dBTen/10, (TableType3 *)&GAIN_TABLE_TX_LB_PA[0], GAIN_TABLE_SIZE_TX_LB_PA);

        GainReg1 = GAIN_TABLE_TX_LB_PA[i].GainReg;
        
        HWREGB(REG_8_TXLB_PA_GC) = GainReg1 & 0xFF;
    }
    else if(GainId == TX_BB_GC)
    {
        i = RF_TABLE_TYPE3_Search_by_dB(dBTen/10, (TableType3 *)&GAIN_TABLE_TX_BB[0], GAIN_TABLE_SIZE_TX_BB);

        GainReg1 = GAIN_TABLE_TX_BB[i].GainReg;
        
        HWREGB(REG_8_TXBB_GC) = GainReg1 & 0x0F;
    }
}


int16_t DFE_TX_Single_Gain_Get_dBTen(uint8_t GainId)
{
    uint32_t GainReg1;
//    uint32_t GainReg2;
    
    uint8_t i;
    
    if(GainId == TX_HB_MX_CAS_GC)
    {
        GainReg1 = HWREGB(REG_8_TXHB_MX_CAS_GC) & 0x3F;
        
        for(i = 0; i < GAIN_TABLE_SIZE_TX_HB_MIXER; i++)
        {
            if(GainReg1 == GAIN_TABLE_TX_HB_MIXER[i].GainReg)
            {
                return GAIN_TABLE_TX_HB_MIXER[i].dBTen;
            }
        }
    }
    else if(GainId == TX_HB_PAD_GC)
    {
        GainReg1 = HWREGB(REG_8_TXHB_PAD_GC) & 0xFF;
        
        for(i = 0; i < GAIN_TABLE_SIZE_TX_HB_PAD; i++)
        {
            if(GainReg1 == GAIN_TABLE_TX_HB_PAD[i].GainReg)
            {
                return GAIN_TABLE_TX_HB_PAD[i].dBTen;
            }
        }
    }
    else if(GainId == TX_HB_PA_GC)
    {
        GainReg1 = HWREGB(REG_8_TXHB_PA_GC) & 0xFF;
        
        for(i = 0; i < GAIN_TABLE_SIZE_TX_HB_PA; i++)
        {
            if(GainReg1 == GAIN_TABLE_TX_HB_PA[i].GainReg)
            {
                return GAIN_TABLE_TX_HB_PA[i].dBTen;
            }
        }
    }
    else if(GainId == TX_LB_MX_CAS_GC)
    {
        GainReg1 = HWREGB(REG_8_TXLB_MX_CAS_GC) & 0x3F;
        
        for(i = 0; i < GAIN_TABLE_SIZE_TX_LB_MIXER; i++)
        {
            if(GainReg1 == GAIN_TABLE_TX_LB_MIXER[i].GainReg)
            {
                return (10 * GAIN_TABLE_TX_LB_MIXER[i].dB);
            }
        }
    }
    else if(GainId == TX_LB_PA_GC)
    {
        GainReg1 = HWREGB(REG_8_TXLB_PA_GC) & 0xFF;
        
        for(i = 0; i < GAIN_TABLE_SIZE_TX_LB_PA; i++)
        {
            if(GainReg1 == GAIN_TABLE_TX_LB_PA[i].GainReg)
            {
                return (10 * GAIN_TABLE_TX_LB_PA[i].dB);
            }
        }
    }
    else if(GainId == TX_BB_GC)
    {
        GainReg1 = HWREGB(REG_8_TXBB_GC) & 0x0F;
        
        for(i = 0; i < GAIN_TABLE_SIZE_TX_BB; i++)
        {
            if(GainReg1 == GAIN_TABLE_TX_BB[i].GainReg)
            {
                return (10 * GAIN_TABLE_TX_BB[i].dB);
            }
        }
    }
    
    return 0;
}


void DFE_RX_Delay_Counter_Set(uint8_t CounterId, uint32_t Value)
{
    if(CounterId == RX_DELAY_ACI)
    {
        HWREG(REG_32_RX_DLY_REG0) = Value;
    }
    else if(CounterId == RX_DELAY_CIC)
    {
        HWREG(REG_32_RX_DLY_REG1) = Value;
    }
    else if(CounterId == RX_DELAY_PGA)
    {
        HWREG(REG_32_RX_DLY_REG2) = Value;
    }
    else if(CounterId == RX_DELAY_DC_CAL)
    {
        HWREG(REG_32_RX_DLY_REG3) = Value;
    }
    else if(CounterId == RX_DELAY_MIXER)
    {
        HWREG(REG_32_RX_DLY_REG4) = Value;
    }
    else if(CounterId == RX_DELAY_LNA)
    {
        HWREG(REG_32_RX_DLY_REG5) = Value;
    }
    else if(CounterId == RX_DELAY_SUBFRAME)
    {
        HWREG(REG_32_RX_DLY_REG6) = Value;
    }
    else if(CounterId == RX_DELAY_ONE_MS)
    {
        HWREG(REG_32_RX_DLY_REG7) = Value;
    }
}

void DFE_TX_Delay_Counter_Set(uint8_t CounterId, uint32_t Value)
{
    if(CounterId == TX_DELAY_BB_GAIN)
    {
        HWREG(REG_32_TX_DLY_REG1) = Value;
    }
    else if(CounterId == TX_DELAY_RF_GAIN)
    {
        HWREG(REG_32_TX_DLY_REG2) = Value;
    }
    else if(CounterId == TX_DELAY_BB_PU)
    {
        HWREG(REG_32_TX_DLY_REG3) = Value;
    }
    else if(CounterId == TX_DELAY_RF_PU)
    {
        HWREG(REG_32_TX_DLY_REG4) = Value;
    }
}

int16_t DFE_RX_RSSI_CIC_Get(void)
{
    // Qiaohui, need to calculate dB?
    
    return (HWREG(REG_32_RX_RSSI_CIC) & 0x3FF);
}

int16_t DFE_RX_RSSI_ACI_Get(void)
{
    // Qiaohui, need to calculate dB?
    
    return ((HWREG(REG_32_RX_RSSI_CIC) >> 16) & 0x3FF);
}

int16_t DFE_TX_TSSI_AVG_Get(void)
{
    return (HWREGB(REG_8_TX_TSSI_AVG_NUM) & 0x0F);
}

void DFE_TX_TSSI_AVG_Num_Set(uint8_t avg_num)
{
    HWREGB(REG_8_TX_TSSI_AVG_NUM) = avg_num & 0x0F;
}

void DFE_TX_DC_EST_I_Set(int16_t value)
{
    HWREG(REG_32_TX_DC_EST_IQ) = (HWREG(REG_32_TX_DC_EST_IQ) & 0xFFFFF000) | (value & 0xFFF);
}

void DFE_TX_DC_EST_Q_Set(int16_t value)
{
    HWREG(REG_32_TX_DC_EST_IQ) = (HWREG(REG_32_TX_DC_EST_IQ) & 0xF000FFFF) | ((value & 0xFFF) << 16);
}

uint16_t DFE_TX_IQ_AMP_EST_Get(void)
{
    return (HWREG(REG_32_TX_IQ_AMP_EST) & 0xFFF);
}

uint16_t DFE_TX_IQ_PHS_EST_I_Get(void)
{
    return ((HWREG(REG_32_TX_IQ_AMP_EST) >> 16)& 0xFFF);
}

uint16_t DFE_TX_IQ_PHS_EST_Q_Get(void)
{
    return (HWREG(REG_32_TX_IQ_PHS_EST_Q) & 0xFFF);
}
