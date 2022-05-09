#include "flash_vendor.h"
#include "ipcmsg.h"
#include "xy_clk_config.h"

QSPI_FLASH_Def xinyi_flash;

void flash_vendor_delay(unsigned long uldelay)
{
    volatile unsigned long i;
    
    for(i = 0; i < uldelay; i++)
    {
    }
}

void
FLASH_ChipErase(QSPI_FLASH_Def *flash_vendor)
{
    unsigned char flash_cmd;
    unsigned char flash_cmd_rw;
    
	FLASH_WriteEnable(flash_vendor);

    flash_cmd = FLASH_CMD_CHIP_ERASE;
    qspi_apb_command_write(flash_vendor->regbase, 1, &flash_cmd, 0, &flash_cmd_rw);
}


void
FLASH_SectorErase(QSPI_FLASH_Def *flash_vendor, unsigned int address)
{
	unsigned char flash_cmd[4];
    unsigned char flash_cmd_rw;
	
	FLASH_WriteEnable(flash_vendor);
    
    flash_cmd[0] = FLASH_CMD_SECTOR_ERASE;
	flash_cmd[1] = (address >> 16) & 0xFF;
	flash_cmd[2] = (address >>  8) & 0xFF;
	flash_cmd[3] = (address >>  0) & 0xFF;
    qspi_apb_command_write(flash_vendor->regbase, 4, &flash_cmd[0], 0, &flash_cmd_rw);
}

void
FLASH_BlockErase32K(QSPI_FLASH_Def *flash_vendor, unsigned int address)
{
	unsigned char flash_cmd[4];
    unsigned char flash_cmd_rw;
	
	FLASH_WriteEnable(flash_vendor);
    
    flash_cmd[0] = FLASH_CMD_BLOCK_ERASE_32K;
	flash_cmd[1] = (address >> 16) & 0xFF;
	flash_cmd[2] = (address >>  8) & 0xFF;
	flash_cmd[3] = (address >>  0) & 0xFF;
    qspi_apb_command_write(flash_vendor->regbase, 4, &flash_cmd[0], 0, &flash_cmd_rw);
}

void
FLASH_BlockErase64K(QSPI_FLASH_Def *flash_vendor, unsigned int address)
{
	unsigned char flash_cmd[4];
    unsigned char flash_cmd_rw;
	
	FLASH_WriteEnable(flash_vendor);
    
    flash_cmd[0] = FLASH_CMD_BLOCK_ERASE_64K;
	flash_cmd[1] = (address >> 16) & 0xFF;
	flash_cmd[2] = (address >>  8) & 0xFF;
	flash_cmd[3] = (address >>  0) & 0xFF;
    qspi_apb_command_write(flash_vendor->regbase, 4, &flash_cmd[0], 0, &flash_cmd_rw);
}


void
FLASH_WriteData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum)
{
    unsigned long ulBlockNum;
    unsigned long ulLeftByte;
    unsigned long i;
	unsigned char *srcDataBuf = NULL;

    DMAChannelControlSet(ulChannelNum, DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W | DMAC_CTRL_WORD_SIZE_32b);

	if (ulDstAddr%4 != 0) {
		unsigned long shiftsize = ulDstAddr & 3;
		srcDataBuf = xy_zalloc(ulLenByte + 4);
		unsigned char *tempBUF = srcDataBuf;
		unsigned char tempData[4] = {0};
		
		ulDstAddr &= 0xFFFFFFFC;
		memcpy((void *)tempData, (void *)ulDstAddr, 4);
		
		for (i=0; i<shiftsize; i++) {
			*tempBUF++ = tempData[i];
		}

		DMAChannelTransfer_withMutex((unsigned long)(tempBUF), ulSrcAddr, ulLenByte, ulChannelNum);

		ulSrcAddr = (unsigned long)srcDataBuf;

		ulLenByte += shiftsize;
	}

    qspi_apb_indirect_enable(flash_vendor);
    
    ulBlockNum = ulLenByte >> 15;
    
    ulLeftByte = ulLenByte & 0x00007FFF;
    
    for(i = 0; i < ulBlockNum; i++)
    {
        qspi_apb_indirect_write(flash_vendor, ulDstAddr, FLASH_INDIRECT_MAX_BYTE);
    
        DMAChannelTransfer_withMutex(ulDstAddr, ulSrcAddr, FLASH_INDIRECT_MAX_BYTE, ulChannelNum);

        qspi_wbuf_wait_idle();
        qspi_wait_idle(flash_vendor->regbase);

        qspi_wait_indirect_complete();
        
        ulDstAddr += FLASH_INDIRECT_MAX_BYTE;
        ulSrcAddr += FLASH_INDIRECT_MAX_BYTE;
    }
    
    if(ulLeftByte != 0)
    {
        qspi_apb_indirect_write(flash_vendor, ulDstAddr, ulLeftByte);
    
        DMAChannelTransfer_withMutex(ulDstAddr, ulSrcAddr, ulLeftByte, ulChannelNum);

        qspi_wbuf_wait_idle();
        qspi_wait_idle(flash_vendor->regbase);

        qspi_wait_indirect_complete();
    }

	if (srcDataBuf != NULL) xy_free(srcDataBuf);
}


void
FLASH_ReadData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum)
{
    unsigned long ulBlockNum;
    unsigned long ulLeftByte;
    unsigned long i;
	unsigned char *dstDataBuf = NULL;
	unsigned long ulSaveAddr = ulDstAddr;
	unsigned long shiftsize = ulSrcAddr & 3;

	if (ulSrcAddr%4 != 0) {
		dstDataBuf = xy_zalloc(ulLenByte + 4);
	
		ulSrcAddr &= 0xFFFFFFFC;
		
		ulDstAddr = (unsigned long)dstDataBuf;
		ulLenByte += shiftsize;
	}
    
    DMAChannelControlSet(ulChannelNum, DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W | DMAC_CTRL_WORD_SIZE_32b);

    ulBlockNum = ulLenByte >> 15;
    
    ulLeftByte = ulLenByte & 0x00007FFF;
    
    for(i = 0; i < ulBlockNum; i++)
    {
        DMAChannelTransfer_withMutex(ulDstAddr, ulSrcAddr, FLASH_INDIRECT_MAX_BYTE, ulChannelNum);
        
        ulDstAddr += FLASH_INDIRECT_MAX_BYTE;
        ulSrcAddr += FLASH_INDIRECT_MAX_BYTE;
    }
    
    if(ulLeftByte != 0)
    {
        DMAChannelTransfer_withMutex(ulDstAddr, ulSrcAddr, ulLeftByte, ulChannelNum);
    }

    qspi_rbuf_wait_idle();
    qspi_wait_idle(flash_vendor->regbase);

	if (dstDataBuf != NULL) {
		DMAChannelTransfer_withMutex(ulSaveAddr, (unsigned long)dstDataBuf + shiftsize, ulLenByte -shiftsize, ulChannelNum);
		xy_free(dstDataBuf);
	}
}

void
FLASH_WriteEnable(QSPI_FLASH_Def *flash_vendor)
{
	unsigned char flash_cmd;
    unsigned char flash_cmd_rw;
	
	flash_cmd = FLASH_CMD_WRITE_ENABLE;
    qspi_apb_command_write(flash_vendor->regbase, 1, &flash_cmd, 0, &flash_cmd_rw);
    
    flash_cmd = FLASH_CMD_READ_STATUS_REG1; // [S7:S0]
    flash_cmd_rw = 0x00;
    qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw);
	while((flash_cmd_rw & STATUS_REG1_WEL) == 0x00) {
		qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw);
    }
}

void
FLASH_WaitIdle(QSPI_FLASH_Def *flash_vendor)
{
	unsigned char flash_cmd;
    unsigned char flash_cmd_rw;

    qspi_rbuf_wait_idle();
    qspi_wbuf_wait_idle();
    qspi_wait_idle(flash_vendor->regbase);
	
    flash_cmd = FLASH_CMD_READ_STATUS_REG1; //
    flash_cmd_rw = 0x00;
    qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw);
    while((flash_cmd_rw & STATUS_REG1_WIP) != 0x00) {
		qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw);
    }
}

void
FLASH_EnableQEFlag(QSPI_FLASH_Def *flash_vendor)
{
	unsigned char flash_cmd;
    unsigned char flash_cmd_rw[2];
    unsigned char status_reg2;
    
    flash_cmd = FLASH_CMD_READ_STATUS_REG2;
    flash_cmd_rw[0] = 0x00;
    qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw[0]);
    
    if((flash_cmd_rw[0] & STATUS_REG2_QE) == STATUS_REG2_QE)
    {
        return;
    }

    status_reg2 = flash_cmd_rw[0];
    
    FLASH_WriteEnable(flash_vendor);
    
    if(FLASH_GD25Q32 != flash_vendor->flash_type)
    {
        flash_cmd = FLASH_CMD_WRITE_STATUS_REG;
        //flash_cmd_rw[0] = 0x00;
        flash_cmd_rw[1] = (status_reg2 | STATUS_REG2_QE);
        qspi_apb_command_write(flash_vendor->regbase, 1, &flash_cmd, 2, &flash_cmd_rw[0]);
    }
	else
	{
		flash_cmd = FLASH_CMD_WRITE_STATUS_REG2;
		flash_cmd_rw[0] = (status_reg2 | STATUS_REG2_QE);
		qspi_apb_command_write(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw[0]);
	}
    
    flash_cmd = FLASH_CMD_READ_STATUS_REG2;
    flash_cmd_rw[0] = 0x00;
    qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw[0]);
	while((flash_cmd_rw[0] & STATUS_REG2_QE) == 0x00) {
		qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw[0]);
    }
}

void  
FLASH_EnterXIPMode(QSPI_FLASH_Def *flash_vendor, char xip_dummy)
{
    volatile unsigned int reg;
    
    // Disable Direct/Indirect Access
    
    FLASH_SetReadMode(flash_vendor, QSPI_READ_XIP);
	
    writel(xip_dummy, flash_vendor->regbase + QSPI_REG_MODE_BIT);
    
	/* enter XiP mode and enable direct mode */
	reg = readl(flash_vendor->regbase + QSPI_REG_CONFIG);
	reg |= QSPI_REG_CONFIG_ENABLE;
	reg |= QSPI_REG_CONFIG_DIRECT;
	reg |= QSPI_REG_CONFIG_XIP_NEXT_READ;

    writel(reg, flash_vendor->regbase + QSPI_REG_CONFIG);

    flash_vendor_delay(100);
    
    reg = HWD_REG_READ32(QSPI_DATA_BASE);
    
    flash_vendor_delay(100);
}

void
FLASH_ExitXIPMode(QSPI_FLASH_Def *flash_vendor)
{
    volatile unsigned int reg;

    
    FLASH_SetReadMode(flash_vendor, QSPI_READ_QUAD);
    FLASH_SetWriteMode(flash_vendor, QSPI_WRITE_QUAD);

    writel(0x00, flash_vendor->regbase + QSPI_REG_MODE_BIT);
    
    reg = readl(flash_vendor->regbase + QSPI_REG_CONFIG);
	reg |= QSPI_REG_CONFIG_ENABLE;
	reg |= QSPI_REG_CONFIG_DIRECT;
	reg &= (~QSPI_REG_CONFIG_XIP_NEXT_READ);

    writel(reg, flash_vendor->regbase + QSPI_REG_CONFIG);
    
    flash_vendor_delay(10);
    
    reg = HWD_REG_READ32(QSPI_DATA_BASE);
    
    flash_vendor_delay(10);
}

void
FLASH_SetReadMode(QSPI_FLASH_Def *flash_vendor, unsigned int mode)
{
    qspi_apb_set_read_instruction(flash_vendor->regbase, mode);
}

void
FLASH_SetWriteMode(QSPI_FLASH_Def *flash_vendor, unsigned int mode)
{
    qspi_apb_set_write_instruction(flash_vendor->regbase, mode);
}

/* Read Status Register command */
static uint8_t read_sr_cmd[3] = {0x05, 0x35, 0x15};
/* write Status Register command */
static uint8_t write_sr_cmd[3] = {0x01, 0x31, 0x11};

static uint8_t FLASH_ReadSR(QSPI_FLASH_Def *flash_vendor, FLASH_STATUS_REG sr)
{
	uint8_t sr_value = 0;
	qspi_apb_command_read(flash_vendor->regbase, 1, &read_sr_cmd[sr], 1, &sr_value);

	return sr_value;
}

static void FLASH_WriteSR(QSPI_FLASH_Def *flash_vendor, FLASH_STATUS_REG sr, uint8_t sr_value)
{
	uint8_t cmd;
	uint8_t value;

	FLASH_WriteEnable(flash_vendor);
	qspi_apb_command_write(flash_vendor->regbase, 1, &write_sr_cmd[sr], 1, &sr_value);
	
    cmd = FLASH_CMD_READ_STATUS_REG1;
    qspi_apb_command_read(flash_vendor->regbase, 1, &cmd, 1, &value);
    while((value & STATUS_REG1_WIP) != 0x00) {
		qspi_apb_command_read(flash_vendor->regbase, 1, &cmd, 1, &value);
    }

}

/* cmp is 0 or 1, see  FLASH_PROTECT_MODE for detail */
void FLASH_SetProtectMode(QSPI_FLASH_Def *flash_vendor, FLASH_PROTECT_MODE protect_mode, uint8_t cmp)
{
	volatile uint8_t value;

	/* set cmp */
	value = FLASH_ReadSR(flash_vendor, SR2);
	if(((value >> 6)&1) != cmp)
	{
		value &= ~(((uint8_t)1) << 6);
		value |= ((uint8_t)cmp&1) << 6;
		FLASH_WriteSR(flash_vendor, SR2, value);
	}
	
	/* set BP0 ~ BP4 */
	value = FLASH_ReadSR(flash_vendor, SR1);
	if(((value >> 2)&0x1F) != protect_mode)
	{
		value &= ~(((uint8_t)0x1F) << 2);
		value |= ((uint8_t)protect_mode&0x1F) << 2;
		FLASH_WriteSR(flash_vendor, SR1, value);
	}

}

//void
//FLASH_SetReadParameter(QSPI_FLASH_Def *flash_vendor, unsigned char parameter)
//{
//    unsigned char flash_cmd;
//    unsigned char flash_cmd_rw;

//    
//    flash_cmd = FLASH_CMD_READ_PARAMETER;
//    flash_cmd_rw = parameter;
//    qspi_apb_command_write(flash_vendor->regbase, 1, &flash_cmd, 1, &flash_cmd_rw);
//}
    
unsigned int
FLASH_GetDeviceID(QSPI_FLASH_Def *flash_vendor)
{
	unsigned char flash_cmd;
	unsigned char flash_cmd_rw[4];
	
	flash_cmd = FLASH_CMD_READ_DEVICEID;
	flash_cmd_rw[0] = 0x00;
	flash_cmd_rw[1] = 0x00;
	flash_cmd_rw[2] = 0x00;
	qspi_apb_command_read(flash_vendor->regbase, 1, &flash_cmd, 3, &flash_cmd_rw[0]);
	
	return (flash_cmd_rw[0] << 16 | flash_cmd_rw[1] << 8 | flash_cmd_rw[2]);
}

//返回的uid数组大小16Bytes
void FLASH_GetUniqueID(unsigned char uid[])
{
    unsigned long reg_val;

    FLASH_ExitXIPMode((QSPI_FLASH_Def *)&xinyi_flash);

    reg_val = HWREG(0xA0030004);

    HWREG(0xA0030004) = 0x0800004B;

    FLASH_ReadData((QSPI_FLASH_Def *)&xinyi_flash, QSPI_DATA_BASE, (unsigned long)(uid), 16, DMA_CHANNEL_0);
    FLASH_WaitIdle((QSPI_FLASH_Def *)&xinyi_flash);

    HWREG(0xA0030004) = reg_val;

    FLASH_EnterXIPMode((QSPI_FLASH_Def *)&xinyi_flash, 0xa0);
}

void sys_flash_init(void)
{
	QSPI_FLASH_Def  *flash_vendor = &xinyi_flash;
	flash_vendor->regbase = (unsigned char *)QSPI_BASE;
	flash_vendor->ahbbase = (unsigned char *)QSPI_DATA_BASE;
	flash_vendor->page_size = 256;
	flash_vendor->block_size = 16;
	flash_vendor->tshsl_ns = 60;
	flash_vendor->tsd2d_ns = 12;
	flash_vendor->tchsh_ns = 12;
	flash_vendor->tslch_ns = 12;
#ifdef FLASH_GIGA_4M
    flash_vendor->flash_type = FLASH_GD25Q32;  // FLASH_UNKNOWN
    flash_vendor->addr_bytes = 3;
    flash_vendor->otp_base = OTP_BASE_ADDR_GD25Q32;
#else
    flash_vendor->flash_type = FLASH_GD25Q16;  //  FLASH_UNKNOWN
    flash_vendor->addr_bytes = 3;
    flash_vendor->otp_base = OTP_BASE_ADDR_GD25Q16;
#endif

#if 0
	while(HWREG(FLASH_INIT_SYNC)!=0x5555aaaa){}
	HWREG(FLASH_INIT_SYNC)=0x6666bbbb;
#endif

#if 1
	qspi_apb_controller_init(flash_vendor);

	qspi_apb_config_baudrate_div(flash_vendor->regbase,OS_SYS_CLOCK,OS_SYS_CLOCK/4);
	qspi_apb_delay(flash_vendor->regbase,OS_SYS_CLOCK,OS_SYS_CLOCK/4,flash_vendor->tshsl_ns,flash_vendor->tsd2d_ns,flash_vendor->tchsh_ns,flash_vendor->tslch_ns);

    HWREG(0xA0030010) = 0x01;
#endif
}

