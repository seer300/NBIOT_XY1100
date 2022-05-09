#include "adc.h"

//record the otp is valid or not, 1 is valid
unsigned char otp_valid_flag = 0;
Flash_OTP_CAL_Def flash_otp_cal;

static float sVBATMultiCoef = 4.98;

void ADC_Init(unsigned int adc_mode, unsigned int sample_speed, unsigned int range)
{
	unsigned char adc_clock;
	unsigned char adc_ch_en;
	unsigned char adc_sensor;
	unsigned char adc_channal;
	unsigned char adc_range;
	
	switch(adc_mode)
	{
		case ADC_MODE_DOUBLE_P:
		case ADC_MODE_DOUBLE_N:
		case ADC_MODE_DOUBLE_PN:
		{
			adc_clock   = 0x41;
			adc_ch_en   = 0x00;
			adc_channal = 0x00;
			if(range == ADC_RANGE_1V)
			{
				adc_sensor = 0x0C;
				adc_range  = 0x00;
			}
			else
			{
				adc_sensor = 0x00;
				adc_range  = 0x0C;
			}
			break;
		}
		case ADC_MODE_SINGLE_P:
		{
			adc_clock   = 0x41;
			adc_ch_en   = 0x01;
			adc_channal = 0x00;
			if(range == ADC_RANGE_1V)
			{
				adc_sensor = 0x04;
				adc_range  = 0x00;
			}
			else
			{
				adc_sensor = 0x00;
				adc_range  = 0x04;
			}
			break;
		}
		case ADC_MODE_SINGLE_N:
		{
			adc_clock   = 0x41;
			adc_ch_en   = 0x02;
			adc_channal = 0x00;
			if(range == ADC_RANGE_1V)
			{
				adc_sensor = 0x08;
				adc_range  = 0x00;
			}
			else
			{
				adc_sensor = 0x00;
				adc_range  = 0x08;
			}
			break;
		}
		case ADC_MODE_VBAT_CHECK:
		{
			adc_clock   = 0x51;
			adc_ch_en   = 0x01;
			adc_sensor  = 0x00;
			adc_channal = 0x01;
			adc_range   = 0x00;
			break;
		}
		default:
			return;
	}
	
	// Single-ended: 
	//    use channal P:     reg0x49: 0x01  reg0x4E: 0x04
	//    use channal N:     reg0x49: 0x02  reg0x4E: 0x08
	// Double-ended:
	//    both channal used: reg0x49: 0x00  reg0x4E: 0x00
	// attenuation: 
	//    if use attenuation, set the reg0x50 corresponding bit to 1,
	//  and the reg0x4E corresponding bit to 0
	HWREGB(0xA0110048) = (HWREGB(0xA0110048) & 0xAE) | adc_clock;	// aux adc power up and bbldo power up
	HWREGB(0xA011004B) = 0x01;										// adc delay adj
	if(((HWREGB(0x2601F0F8) & 0x01) == 0x01) || ((HWREGB(0x2601F0DA) & 0x08) == 0x08))
	{
		HWREGB(0xA0110049) = (HWREGB(0xA0110049) & 0xFC) | adc_ch_en | 0x80;	// adc channel enb
	}
	else
	{
		HWREGB(0xA0110049) = (HWREGB(0xA0110049) & 0xFC) | adc_ch_en;	// adc channel enb
	}
	
	HWREGB(0xA0058024) |= 0x03;										// aux_levelshift ldo levelshift
	HWREGB(0xA0110046) = (HWREGB(0xA0110046) & 0xC0) | sample_speed;// clk div set
	HWREGB(0xA0110050) = adc_range;									// attenu disable
	
	HWREGB(0xA011004E) = (HWREGB(0xA011004E) & 0xF0) | adc_sensor;	// sensor selection
	HWREGB(0xA011004A) = adc_channal;								// channel selection
}


void ADC_DeInit(void)
{
	//bug 3781
	if(((HWREGB(0x2601F0F8) & 0x01) == 0x00) && ((HWREGB(0x2601F0DA) & 0x08) == 0x00))
	{
		HWREGB(0xA0110048) &= 0xAE;	// aux adc power up and bbldo power down
		HWREGB(0xA0110049) &= 0xFC;	// adc channel disable
		//HWREGB(0xA0058024) &= 0xFC;	// aux_levelshift ldo levelshift, [maql] these ctrl_en bits can keep == 1
	}
	else
	{
		HWREGB(0xA0110048) &= 0xEE;	// aux adc power up and bbldo power down
		HWREGB(0xA0110049) = (HWREGB(0xA0110049) & 0xFC) | 0x80;	// adc channel disable, [maql] if rx/tx, keep trx_en == 1
		//HWREGB(0xA0058024) = (HWREGB(0xA0058024) & 0xFD) | 0x0D;	// aux_levelshift ldo levelshift, [maql] if rx/tx, keep other ctrl_en == 1, or these ctrl_en bits can keep == 1
	}

	HWREGB(0xA011004B)  = 0x00;	// adc delay adj
	HWREGB(0xA0110046)  = 0x00;	// clk div set
	HWREGB(0xA0110050)  = 0x00;	// attenu
	
	HWREGB(0xA011004E)  = 0x00;	// close sensor
	//HWREGB(0xA011004A) = 0x00;
}


void ADC_Open(unsigned int adc_mode, unsigned int sample_speed, unsigned int range)
{
	volatile unsigned int delay;

	WAIT_DSP_ADC_IDLE();
	SET_ARM_ADC_BUSY();

	ADC_Init(adc_mode, sample_speed, range);

	delay = 1000;
	while(delay--);
}


void ADC_Close(void)
{
	ADC_DeInit();

	SET_ARM_ADC_IDLE();
}


short ADC_Get_Real_Value(void)
{
	short real_val;
	uint8_t retry = 0;
	volatile uint32_t delay;

	// if read to 0, retry 3 times
	while(1)
	{
		real_val = (HWREGH(0xA0110044) & 0x0FFF);

		if((real_val != 0) || (retry >= 3))
		{
			break;
		}
		else
		{
			retry++;
			delay = 1000;
			while(delay--);
		}
	}

	if(real_val & 0x0800)
	{
		real_val |= 0xF000;	// signed data extend to 16 bits
	}

	return real_val;
}


short ADC_Get_Converte_Value(unsigned int adc_mode, short real_value)
{
	short conv_val;

	switch(adc_mode)
	{
		case ADC_MODE_DOUBLE_P:
		case ADC_MODE_DOUBLE_N:
		case ADC_MODE_DOUBLE_PN:
			conv_val = real_value;
			break;
		case ADC_MODE_SINGLE_P:
		case ADC_MODE_VBAT_CHECK:
			conv_val = real_value + 2048;
			break;
		case ADC_MODE_SINGLE_N:
			conv_val = 2047 - real_value;
			break;
		default:
			return ADC_MODE_ERROR;
	}
	
	return conv_val;
}


short ADC_Get_Voltage_Without_Cal(unsigned int adc_mode, unsigned int range, short conv_val)
{
	short voltage;
	int rounding_val;
	int multiple;

	rounding_val = conv_val;

	if((adc_mode != ADC_MODE_VBAT_CHECK) && (range == ADC_RANGE_4V))
		multiple = 4;
	else
		multiple = 1;

	switch(adc_mode)
	{
		case ADC_MODE_DOUBLE_P:
		case ADC_MODE_DOUBLE_N:
		case ADC_MODE_DOUBLE_PN:
			if(rounding_val >= 0)
			{
				voltage = (short)((rounding_val * 10000 * multiple + 10240) / 20480);
			}
			else
			{
				voltage = (short)((rounding_val * 10000 * multiple - 10240) / 20480);
			}
			break;
		case ADC_MODE_SINGLE_P:
		case ADC_MODE_SINGLE_N:
			voltage = (short)((rounding_val * 10000 * multiple + 20475) / 40950);
			break;
		case ADC_MODE_VBAT_CHECK:
			voltage = (short)((rounding_val * 10000 * 4.98 + 20475) / 40950);
			break;
		default:
			return ADC_MODE_ERROR;
	}
	
	return voltage;
}



short ADC_Get_Rounding_Value(unsigned int adc_mode, short conv_val)
{
	short rounding_val;
	
	if(otp_valid_flag != 1)
		return ADC_CALI_ERROR;

	switch(adc_mode)
	{
		case ADC_MODE_DOUBLE_P:
			if(conv_val >= 0)
			{
				rounding_val = (short)(((conv_val * double_gainerror_P) + double_offset_P + (DOUBLE_EXPAND_MULTIPLE / 2)) / DOUBLE_EXPAND_MULTIPLE);
			}
			else
			{
				rounding_val = (short)(((conv_val * double_gainerror_P) + double_offset_P - (DOUBLE_EXPAND_MULTIPLE / 2)) / DOUBLE_EXPAND_MULTIPLE);
			}
			break;
		case ADC_MODE_DOUBLE_N:
			if(conv_val >= 0)
			{
				rounding_val = (short)(((conv_val * double_gainerror_N) + double_offset_N + (DOUBLE_EXPAND_MULTIPLE / 2)) / DOUBLE_EXPAND_MULTIPLE);
			}
			else
			{
				rounding_val = (short)(((conv_val * double_gainerror_N) + double_offset_N - (DOUBLE_EXPAND_MULTIPLE / 2)) / DOUBLE_EXPAND_MULTIPLE);
			}
			break;
		case ADC_MODE_DOUBLE_PN:
			if(conv_val >= 0)
			{
				rounding_val = (short)(((conv_val * double_gainerror_PN) + double_offset_PN + (DOUBLE_EXPAND_MULTIPLE / 2)) / DOUBLE_EXPAND_MULTIPLE);
			}
			else
			{
				rounding_val = (short)(((conv_val * double_gainerror_PN) + double_offset_PN - (DOUBLE_EXPAND_MULTIPLE / 2)) / DOUBLE_EXPAND_MULTIPLE);
			}
			break;
		case ADC_MODE_SINGLE_P:
		case ADC_MODE_VBAT_CHECK:
			if((conv_val >= 0) && (conv_val < 1024))
			{
				rounding_val = (short)(((conv_val * single_P_gainerror1) + single_P_offset1 + (SINGLE_P_EXPAND_MULTIPLE / 2)) / SINGLE_P_EXPAND_MULTIPLE);
			}
			else if((conv_val >= 1024) && (conv_val < 2048))
			{
				rounding_val = (short)(((conv_val * single_P_gainerror2) + single_P_offset2 + (SINGLE_P_EXPAND_MULTIPLE / 2)) / SINGLE_P_EXPAND_MULTIPLE);
			}
			else if((conv_val >= 2048) && (conv_val < 3072))
			{
				rounding_val = (short)(((conv_val * single_P_gainerror3) + single_P_offset3 + (SINGLE_P_EXPAND_MULTIPLE / 2)) / SINGLE_P_EXPAND_MULTIPLE);
			}
			else if((conv_val >= 3072) && (conv_val < 4096))
			{
				rounding_val = (short)(((conv_val * single_P_gainerror4) + single_P_offset4 + (SINGLE_P_EXPAND_MULTIPLE / 2)) / SINGLE_P_EXPAND_MULTIPLE);
			}
			else
			{
				return ADC_RANGE_ERROR;
			}
			break;
		case ADC_MODE_SINGLE_N:
			if((conv_val >= 0) && (conv_val < 1024))
			{
				rounding_val = (short)(((conv_val * single_N_gainerror1) + single_N_offset1 + (SINGLE_N_EXPAND_MULTIPLE / 2)) / SINGLE_N_EXPAND_MULTIPLE);
			}
			else if((conv_val >= 1024) && (conv_val < 2048))
			{
				rounding_val = (short)(((conv_val * single_N_gainerror2) + single_N_offset2 + (SINGLE_N_EXPAND_MULTIPLE / 2)) / SINGLE_N_EXPAND_MULTIPLE);
			}
			else if((conv_val >= 2048) && (conv_val < 3072))
			{
				rounding_val = (short)(((conv_val * single_N_gainerror3) + single_N_offset3 + (SINGLE_N_EXPAND_MULTIPLE / 2)) / SINGLE_N_EXPAND_MULTIPLE);
			}
			else if((conv_val >= 3072) && (conv_val < 4096))
			{
				rounding_val = (short)(((conv_val * single_N_gainerror4) + single_N_offset4 + (SINGLE_N_EXPAND_MULTIPLE / 2)) / SINGLE_N_EXPAND_MULTIPLE);
			}
			else
			{
				return ADC_RANGE_ERROR;
			}
			break;
		default:
			return ADC_MODE_ERROR;
	}
	
	return rounding_val;
}


short ADC_Get_Voltage_With_Cal(unsigned int adc_mode, unsigned int range, short conv_val)
{
	short voltage;
	int rounding_val;
	int multiple;

	if(otp_valid_flag != 1)
		return ADC_CALI_ERROR;

	rounding_val = (int)ADC_Get_Rounding_Value(adc_mode, conv_val);

	if((adc_mode != ADC_MODE_VBAT_CHECK) && (range == ADC_RANGE_4V))
		multiple = 4;
	else
		multiple = 1;

	switch(adc_mode)
	{
		case ADC_MODE_DOUBLE_P:
		case ADC_MODE_DOUBLE_N:
		case ADC_MODE_DOUBLE_PN:
			if(rounding_val >= 0)
			{
				voltage = (short)((rounding_val * 10000 * multiple + 10240) / 20480);
			}
			else
			{
				voltage = (short)((rounding_val * 10000 * multiple - 10240) / 20480);
			}
			break;
		case ADC_MODE_SINGLE_P:
		case ADC_MODE_SINGLE_N:
			voltage = (short)((rounding_val * 10000 * multiple + 20475) / 40950);
			break;
		case ADC_MODE_VBAT_CHECK:
			{
				//Modify by huangxb 2022-01-19 For VBAT 
				if (HWREG(OTPINFO_RAM_ADDR + 0x54) == 0xFFFFFFFF)
				{
					voltage = (short)((rounding_val * 10000 * 4.98 + 20475) / 40950);
				}
				else
				{
					short vBat3500 = HWREGH((OTPINFO_RAM_ADDR + 0x56));
					short vBat2750 = HWREGH((OTPINFO_RAM_ADDR + 0x54));
					sVBATMultiCoef = ((float)(flash_otp_cal.real_code_P700*2.5)/(vBat3500*1.0)) + ((float)(flash_otp_cal.real_code_P550*2.5)/(vBat2750*1.0));
					voltage = (short)((rounding_val * 10000 * sVBATMultiCoef + 20475) / 40950);
				}
				break;
			}
		default:
			return ADC_MODE_ERROR;
	}

	return voltage;
}


