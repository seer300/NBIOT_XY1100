#include "qspi_flash.h"


void *qspi_memcpy(unsigned char *dest, const unsigned char *src, unsigned int size)
{
	unsigned char *dptr = dest;
	const unsigned char *ptr = src;
	const unsigned char *end = src + size;

	while (ptr < end)
		*dptr++ = *ptr++;

	return dest;
}

void *qspi_memset(char *inptr, int ch, unsigned int size)
{
	char *ptr = inptr;
	char *end = ptr + size;

	while (ptr < end)
		*ptr++ = ch;

	return ptr;
}

static unsigned int qspi_apb_cmd2addr(const unsigned char *addr_buf,
	unsigned int addr_width)
{
	unsigned int addr;

	addr = (addr_buf[0] << 16) | (addr_buf[1] << 8) | addr_buf[2];

	if (addr_width == 4)
		addr = (addr << 8) | addr_buf[3];

	return addr;
}

void qspi_apb_controller_enable(unsigned char *reg_base)
{
	unsigned int reg;
	reg = readl(reg_base + QSPI_REG_CONFIG);
	reg |= QSPI_REG_CONFIG_ENABLE;
	writel(reg, reg_base + QSPI_REG_CONFIG);
}

void qspi_apb_controller_disable(unsigned char *reg_base)
{
	unsigned int reg;
	reg = readl(reg_base + QSPI_REG_CONFIG);
	reg &= ~QSPI_REG_CONFIG_ENABLE;
	writel(reg, reg_base + QSPI_REG_CONFIG);
}

void qspi_wait_indirect_complete(void)
{
    while(!(HWREG(0xA0030070) & 0x20));  //wait indirect complete status

    HWREG(0xA0030070) |=0x20;  //clear indirect complete status
}

void qspi_rbuf_wait_idle(void)
{
    while((HWREG(0xA003002C) & 0x0000FFFF) != 0);
}

void qspi_wbuf_wait_idle(void)
{
    while((HWREG(0xA003002C) & 0xFFFF0000) != 0);
}

/* Return 1 if idle, otherwise return 0 (busy). */
void qspi_wait_idle(unsigned char *reg_base)
{
	while(QSPI_REG_IS_IDLE(reg_base) == 0)
    {
    }
}

void qspi_apb_readdata_capture(unsigned char *reg_base,
				unsigned int bypass, unsigned int delay)
{
	unsigned int reg;
	qspi_apb_controller_disable(reg_base);

	reg = readl(reg_base + QSPI_REG_RD_DATA_CAPTURE);

	if (bypass)
		reg |= QSPI_REG_RD_DATA_CAPTURE_BYPASS;
	else
		reg &= ~QSPI_REG_RD_DATA_CAPTURE_BYPASS;

	reg &= ~(QSPI_REG_RD_DATA_CAPTURE_DELAY_MASK
		<< QSPI_REG_RD_DATA_CAPTURE_DELAY_LSB);

	reg |= (delay & QSPI_REG_RD_DATA_CAPTURE_DELAY_MASK)
		<< QSPI_REG_RD_DATA_CAPTURE_DELAY_LSB;

	writel(reg, reg_base + QSPI_REG_RD_DATA_CAPTURE);

	qspi_apb_controller_enable(reg_base);
}

void qspi_apb_config_baudrate_div(unsigned char *reg_base,
	unsigned int ref_clk_hz, unsigned int sclk_hz)
{
	unsigned int reg;
	unsigned int div;

	qspi_apb_controller_disable(reg_base);
	reg = readl(reg_base + QSPI_REG_CONFIG);
	reg &= ~(QSPI_REG_CONFIG_BAUD_MASK << QSPI_REG_CONFIG_BAUD_LSB);

	/*
	 * The baud_div field in the config reg is 4 bits, and the ref clock is
	 * divided by 2 * (baud_div + 1). Round up the divider to ensure the
	 * SPI clock rate is less than or equal to the requested clock rate.
	 */
	div = DIV_ROUND_UP(ref_clk_hz, sclk_hz * 2) - 1;

	/* ensure the baud rate doesn't exceed the max value */
	if (div > QSPI_REG_CONFIG_BAUD_MASK)
		div = QSPI_REG_CONFIG_BAUD_MASK;

	//debug("%s: ref_clk %dHz sclk %dHz Div 0x%x, actual %dHz\n", __func__,
	//      ref_clk_hz, sclk_hz, div, ref_clk_hz / (2 * (div + 1)));

	reg |= (div << QSPI_REG_CONFIG_BAUD_LSB);
	writel(reg, reg_base + QSPI_REG_CONFIG);

	qspi_apb_controller_enable(reg_base);
}

void qspi_apb_set_clk_mode(unsigned char *reg_base, unsigned int mode)
{
	unsigned int reg;

	qspi_apb_controller_disable(reg_base);
	reg = readl(reg_base + QSPI_REG_CONFIG);
	reg &= ~(QSPI_REG_CONFIG_CLK_POL | QSPI_REG_CONFIG_CLK_PHA);

	if (mode & SPI_CPOL)
		reg |= QSPI_REG_CONFIG_CLK_POL;
	if (mode & SPI_CPHA)
		reg |= QSPI_REG_CONFIG_CLK_PHA;

	writel(reg, reg_base + QSPI_REG_CONFIG);

	qspi_apb_controller_enable(reg_base);
}

void qspi_apb_chipselect(unsigned char *reg_base,
	unsigned int chip_select, unsigned int decoder_enable)
{
	unsigned int reg;

	qspi_apb_controller_disable(reg_base);

	//debug("%s : chipselect %d decode %d\n", __func__, chip_select,
	//      decoder_enable);

	reg = readl(reg_base + QSPI_REG_CONFIG);
	/* docoder */
	if (decoder_enable) {
		reg |= QSPI_REG_CONFIG_DECODE;
	} else {
		reg &= ~QSPI_REG_CONFIG_DECODE;
		/* Convert CS if without decoder.
		 * CS0 to 4b'1110
		 * CS1 to 4b'1101
		 * CS2 to 4b'1011
		 * CS3 to 4b'0111
		 */
		chip_select = 0xF & ~(1 << chip_select);
	}

	reg &= ~(QSPI_REG_CONFIG_CHIPSELECT_MASK
			<< QSPI_REG_CONFIG_CHIPSELECT_LSB);
	reg |= (chip_select & QSPI_REG_CONFIG_CHIPSELECT_MASK)
			<< QSPI_REG_CONFIG_CHIPSELECT_LSB;
	writel(reg, reg_base + QSPI_REG_CONFIG);

	qspi_apb_controller_enable(reg_base);
}

void qspi_apb_delay(unsigned char *reg_base,
	unsigned int ref_clk, unsigned int sclk_hz,
	unsigned int tshsl_ns, unsigned int tsd2d_ns,
	unsigned int tchsh_ns, unsigned int tslch_ns)
{
	unsigned int ref_clk_ns;
	unsigned int sclk_ns;
	unsigned int tshsl, tchsh, tslch, tsd2d;
	unsigned int reg;

	qspi_apb_controller_disable(reg_base);

	/* Convert to ns. */
	ref_clk_ns = DIV_ROUND_UP(1000000000, ref_clk);

	/* Convert to ns. */
	sclk_ns = DIV_ROUND_UP(1000000000, sclk_hz);

	/* The controller adds additional delay to that programmed in the reg */
	if (tshsl_ns >= sclk_ns + ref_clk_ns)
		tshsl_ns -= sclk_ns + ref_clk_ns;
	if (tchsh_ns >= sclk_ns + 3 * ref_clk_ns)
		tchsh_ns -= sclk_ns + 3 * ref_clk_ns;
	tshsl = DIV_ROUND_UP(tshsl_ns, ref_clk_ns);
	tchsh = DIV_ROUND_UP(tchsh_ns, ref_clk_ns);
	tslch = DIV_ROUND_UP(tslch_ns, ref_clk_ns);
	tsd2d = DIV_ROUND_UP(tsd2d_ns, ref_clk_ns);

	reg = ((tshsl & QSPI_REG_DELAY_TSHSL_MASK)
			<< QSPI_REG_DELAY_TSHSL_LSB);
	reg |= ((tchsh & QSPI_REG_DELAY_TCHSH_MASK)
			<< QSPI_REG_DELAY_TCHSH_LSB);
	reg |= ((tslch & QSPI_REG_DELAY_TSLCH_MASK)
			<< QSPI_REG_DELAY_TSLCH_LSB);
	reg |= ((tsd2d & QSPI_REG_DELAY_TSD2D_MASK)
			<< QSPI_REG_DELAY_TSD2D_LSB);
	writel(reg, reg_base + QSPI_REG_DELAY);

	qspi_apb_controller_enable(reg_base);
}

void qspi_apb_controller_init(QSPI_FLASH_Def *plat)
{
	unsigned reg;

	qspi_apb_controller_disable(plat->regbase);

	/* Configure the device size and address bytes */
	reg = readl(plat->regbase + QSPI_REG_SIZE);
	/* Clear the previous value */
	reg &= ~(QSPI_REG_SIZE_PAGE_MASK << QSPI_REG_SIZE_PAGE_LSB);
	reg &= ~(QSPI_REG_SIZE_BLOCK_MASK << QSPI_REG_SIZE_BLOCK_LSB);
	reg &= ~QSPI_REG_SIZE_ADDRESS_MASK;
	reg |= (plat->page_size << QSPI_REG_SIZE_PAGE_LSB);
	reg |= (plat->block_size << QSPI_REG_SIZE_BLOCK_LSB);
	reg |= (plat->addr_bytes - 1);
	writel(reg, plat->regbase + QSPI_REG_SIZE);

	/* Configure the remap address register, no remap */
	writel(0, plat->regbase + QSPI_REG_REMAP);

	/* Indirect mode configurations */
//	writel((plat->sram_size/2), plat->regbase + CQSPI_REG_SRAMPARTITION);

	/* Disable all interrupts */
	writel(0, plat->regbase + QSPI_REG_IRQMASK);

	qspi_apb_controller_enable(plat->regbase);
}

static int qspi_apb_exec_flash_cmd(unsigned char *reg_base,
	unsigned int reg)
{
	/* Write the CMDCTRL without start execution. */
	writel(reg, reg_base + QSPI_REG_CMDCTRL);
	/* Start execute */
	reg |= QSPI_REG_CMDCTRL_EXECUTE;
	writel(reg, reg_base + QSPI_REG_CMDCTRL);

	reg = readl(reg_base + QSPI_REG_CMDCTRL);
	while((reg & QSPI_REG_CMDCTRL_INPROGRESS) != 0)
    {
        reg = readl(reg_base + QSPI_REG_CMDCTRL);
    }

	/* Polling QSPI idle status. */
	qspi_wait_idle(reg_base);

	return 0;
}

/* For command RDID, RDSR. */
int qspi_apb_command_read(unsigned char *reg_base,
	unsigned int cmdlen, const unsigned char *cmdbuf, unsigned int rxlen,
	unsigned char *rxbuf)
{
	unsigned int reg;
	unsigned int read_len;
	int status;

	if (!cmdlen || rxlen > QSPI_STIG_DATA_LEN_MAX || rxbuf == NULL) {
		//softap_printf(USER_LOG, WARN_LOG, "QSPI: Invalid input arguments cmdlen %d rxlen %d\n",
		//       cmdlen, rxlen);
		return -22;
	}

	reg = cmdbuf[0] << QSPI_REG_CMDCTRL_OPCODE_LSB;

	reg |= (0x1 << QSPI_REG_CMDCTRL_RD_EN_LSB);

	/* 0 means 1 byte. */
	reg |= (((rxlen - 1) & QSPI_REG_CMDCTRL_RD_BYTES_MASK)
		<< QSPI_REG_CMDCTRL_RD_BYTES_LSB);
	status = qspi_apb_exec_flash_cmd(reg_base, reg);
	if (status != 0)
		return status;

	reg = readl(reg_base + QSPI_REG_CMDREADDATALOWER);

	/* Put the read value into rx_buf */
	read_len = (rxlen > 4) ? 4 : rxlen;
	qspi_memcpy(rxbuf, (unsigned char *)&reg, read_len);
	rxbuf += read_len;

	if (rxlen > 4) {
		reg = readl(reg_base + QSPI_REG_CMDREADDATAUPPER);

		read_len = rxlen - read_len;
		qspi_memcpy(rxbuf, (unsigned char *)&reg, read_len);
	}
	return 0;
}

/* For commands: WRSR, WREN, WRDI, CHIP_ERASE, BE, etc. */
int qspi_apb_command_write(unsigned char *reg_base, unsigned int cmdlen,
	const unsigned char *cmdbuf, unsigned int txlen,  const unsigned char *txbuf)
{
	unsigned int reg = 0;
	unsigned int addr_value;
	unsigned int wr_data;
	unsigned int wr_len;

	if (!cmdlen || cmdlen > 5 || txlen > 8 || cmdbuf == NULL) {
		//softap_printf(USER_LOG, WARN_LOG, "QSPI: Invalid input arguments cmdlen %d txlen %d\n",
		//       cmdlen, txlen);
		return -22;
	}

	reg |= cmdbuf[0] << QSPI_REG_CMDCTRL_OPCODE_LSB;

	if (cmdlen == 4 || cmdlen == 5) {
		/* Command with address */
		reg |= (0x1 << QSPI_REG_CMDCTRL_ADDR_EN_LSB);
		/* Number of bytes to write. */
		reg |= ((cmdlen - 2) & QSPI_REG_CMDCTRL_ADD_BYTES_MASK)
			<< QSPI_REG_CMDCTRL_ADD_BYTES_LSB;
		/* Get address */
		addr_value = qspi_apb_cmd2addr(&cmdbuf[1],
			cmdlen >= 5 ? 4 : 3);

		writel(addr_value, reg_base + QSPI_REG_CMDADDRESS);
	}

	if (txlen) {
		/* writing data = yes */
		reg |= (0x1 << QSPI_REG_CMDCTRL_WR_EN_LSB);
		reg |= ((txlen - 1) & QSPI_REG_CMDCTRL_WR_BYTES_MASK)
			<< QSPI_REG_CMDCTRL_WR_BYTES_LSB;

		wr_len = txlen > 4 ? 4 : txlen;
		qspi_memcpy((unsigned char *)&wr_data, txbuf, wr_len);
		writel(wr_data, reg_base +
			QSPI_REG_CMDWRITEDATALOWER);

		if (txlen > 4) {
			txbuf += wr_len;
			wr_len = txlen - wr_len;
			qspi_memcpy((unsigned char *)&wr_data, txbuf, wr_len);
			writel(wr_data, reg_base +
				QSPI_REG_CMDWRITEDATAUPPER);
		}
	}

	/* Execute the command */
	return qspi_apb_exec_flash_cmd(reg_base, reg);
}

/* Opcode + Address (3/4 bytes) + dummy bytes (0-4 bytes) */
int qspi_apb_indirect_read_setup(QSPI_FLASH_Def *plat,
	unsigned int cmdlen, unsigned int rx_width, const unsigned char *cmdbuf)
{
	unsigned int reg;
	unsigned int rd_reg;
	unsigned int addr_value;
//	unsigned int dummy_clk;
	unsigned int dummy_bytes;
	unsigned int addr_bytes;

	/*
	 * Identify addr_byte. All NOR flash device drivers are using fast read
	 * which always expecting 1 dummy byte, 1 cmd byte and 3/4 addr byte.
	 * With that, the length is in value of 5 or 6. Only FRAM chip from
	 * ramtron using normal read (which won't need dummy byte).
	 * Unlikely NOR flash using normal read due to performance issue.
	 */
	if (cmdlen >= 5)
		/* to cater fast read where cmd + addr + dummy */
		addr_bytes = cmdlen - 2;
	else
		/* for normal read (only ramtron as of now) */
		addr_bytes = cmdlen - 1;

	/* Setup the indirect trigger address */
	writel((unsigned int)plat->ahbbase,
	       plat->regbase + QSPI_REG_INDIRECTTRIGGER);

	/* Setup the indirect trigger range */
	writel(0x0F, plat->regbase + QSPI_REG_INDIRECTTRIGGER_RANGE);

	/* Configure the opcode */
	rd_reg = cmdbuf[0] << QSPI_REG_RD_INSTR_OPCODE_LSB;

	if (rx_width & SPI_RX_QUAD)
		/* Instruction and address at DQ0, data at DQ0-3. */
		rd_reg |= QSPI_INST_TYPE_QUAD << QSPI_REG_RD_INSTR_TYPE_DATA_LSB;

	/* Get address */
	addr_value = qspi_apb_cmd2addr(&cmdbuf[1], addr_bytes);
	writel(addr_value, plat->regbase + QSPI_REG_INDIRECTRDSTARTADDR);

	/* The remaining lenght is dummy bytes. */
	dummy_bytes = cmdlen - addr_bytes - 1;
	if (dummy_bytes) {
		if (dummy_bytes > QSPI_DUMMY_BYTES_MAX)
			dummy_bytes = QSPI_DUMMY_BYTES_MAX;

//		rd_reg |= (1 << QSPI_REG_RD_INSTR_MODE_EN_LSB);
//#if defined(CONFIG_SPL_SPI_XIP) && defined(CONFIG_SPL_BUILD)
//		writel(0x0, plat->regbase + QSPI_REG_MODE_BIT);
//#else
//		writel(0xFF, plat->regbase + QSPI_REG_MODE_BIT);
//#endif

		/* Convert to clock cycles. */
//		dummy_clk = dummy_bytes * QSPI_DUMMY_CLKS_PER_BYTE;
		/* Need to minus the mode byte (8 clocks). */
//		dummy_clk -= QSPI_DUMMY_CLKS_PER_BYTE;

//		if (dummy_clk)
//			rd_reg |= (dummy_clk & QSPI_REG_RD_INSTR_DUMMY_MASK)
//				<< QSPI_REG_RD_INSTR_DUMMY_LSB;
	}

	writel(rd_reg, plat->regbase + QSPI_REG_RD_INSTR);

	/* set device size */
	reg = readl(plat->regbase + QSPI_REG_SIZE);
	reg &= ~QSPI_REG_SIZE_ADDRESS_MASK;
	reg |= (addr_bytes - 1);
	writel(reg, plat->regbase + QSPI_REG_SIZE);
	return 0;
}

//static unsigned int qspi_get_rd_sram_level(QSPI_FLASH_Def *plat)
//{
//	unsigned int reg = readl(plat->regbase + QSPI_REG_SDRAMLEVEL);
//	reg >>= QSPI_REG_SDRAMLEVEL_RD_LSB;
//	return reg & QSPI_REG_SDRAMLEVEL_RD_MASK;
//}


int qspi_apb_indirect_read_execute(QSPI_FLASH_Def *plat,
	unsigned int n_rx, unsigned char *rxbuf)
{
	(void) rxbuf;
	writel(n_rx, plat->regbase + QSPI_REG_INDIRECTRDBYTES);

	/* Start the indirect read transfer */
	writel(QSPI_REG_INDIRECTRD_START,
	       plat->regbase + QSPI_REG_INDIRECTRD);

	return 0;
}

/* Opcode + Address (3/4 bytes) */
int qspi_apb_indirect_write_setup(QSPI_FLASH_Def *plat,
	unsigned int cmdlen, const unsigned char *cmdbuf)
{
	unsigned int reg;
	unsigned int addr_bytes = cmdlen > 4 ? 4 : 3;

	if (cmdlen < 4 || cmdbuf == NULL) {
		//softap_printf(USER_LOG, WARN_LOG, "QSPI: iInvalid input argument, len %d cmdbuf 0x%08x\n",
		//       cmdlen, (unsigned int)cmdbuf);
		return -2;
	}
	/* Setup the indirect trigger address */
	writel((unsigned int)plat->ahbbase,
	       plat->regbase + QSPI_REG_INDIRECTTRIGGER);

	/* Setup the indirect trigger range */
	writel(0x0F, plat->regbase + QSPI_REG_INDIRECTTRIGGER_RANGE);

	/* Configure the opcode */
	reg = cmdbuf[0] << QSPI_REG_WR_INSTR_OPCODE_LSB;
	writel(reg, plat->regbase + QSPI_REG_WR_INSTR);

	/* Setup write address. */
	reg = qspi_apb_cmd2addr(&cmdbuf[1], addr_bytes);
	writel(reg, plat->regbase + QSPI_REG_INDIRECTWRSTARTADDR);

	reg = readl(plat->regbase + QSPI_REG_SIZE);
	reg &= ~QSPI_REG_SIZE_ADDRESS_MASK;
	reg |= (addr_bytes - 1);
	writel(reg, plat->regbase + QSPI_REG_SIZE);
	return 0;
}

int qspi_apb_indirect_write_execute(QSPI_FLASH_Def *plat,
	unsigned int n_tx, const unsigned char *txbuf)
{
	(void) txbuf;
	/* Configure the indirect write transfer bytes */
	writel(n_tx, plat->regbase + QSPI_REG_INDIRECTWRBYTES);

	/* Start the indirect write transfer */
	writel(QSPI_REG_INDIRECTWR_START,
	       plat->regbase + QSPI_REG_INDIRECTWR);

	return 0;
}

void qspi_apb_enter_xip(unsigned char *reg_base, char xip_dummy)
{
	unsigned int reg;

	/* enter XiP mode immediately and enable direct mode */
	reg = readl(reg_base + QSPI_REG_CONFIG);
	reg |= QSPI_REG_CONFIG_ENABLE;
	reg |= QSPI_REG_CONFIG_DIRECT;
	reg |= QSPI_REG_CONFIG_XIP_NEXT_READ;
	writel(reg, reg_base + QSPI_REG_CONFIG);

	/* keep the XiP mode */
	writel(xip_dummy, reg_base + QSPI_REG_MODE_BIT);

	/* Enable mode bit at devrd */
	reg = readl(reg_base + QSPI_REG_RD_INSTR);
	reg |= (1 << QSPI_REG_RD_INSTR_MODE_EN_LSB);
	writel(reg, reg_base + QSPI_REG_RD_INSTR);
}

void qspi_apb_exit_xip(unsigned char *reg_base)
{
    unsigned int reg;
    
    writel(0x00, reg_base + QSPI_REG_MODE_BIT);
    
    reg = readl(reg_base + QSPI_REG_CONFIG);
	reg |= QSPI_REG_CONFIG_ENABLE;
	reg |= QSPI_REG_CONFIG_DIRECT;
	reg &= (~QSPI_REG_CONFIG_XIP_NEXT_READ);
    writel(reg, reg_base + QSPI_REG_CONFIG);
    
    /* Disable mode bit at devrd */
	reg = readl(reg_base + QSPI_REG_RD_INSTR);
	reg &= (~(1 << QSPI_REG_RD_INSTR_MODE_EN_LSB));
	writel(reg, reg_base + QSPI_REG_RD_INSTR);
}

void qspi_apb_indirect_enable(QSPI_FLASH_Def *plat)
{
//    /* Setup the indirect trigger address */
//    writel((unsigned int)plat->ahbbase, plat->regbase + QSPI_REG_INDIRECTTRIGGER);
    
    /* Setup the indirect trigger range */
    writel(0x0F, plat->regbase + QSPI_REG_INDIRECTTRIGGER_RANGE);
}

void qspi_apb_indirect_disable(QSPI_FLASH_Def *plat)
{
    writel((unsigned int)plat->ahbbase, plat->regbase + QSPI_REG_INDIRECTTRIGGER);
    
    /* ZI the indirect trigger range */
    writel(0x00, plat->regbase + QSPI_REG_INDIRECTTRIGGER_RANGE);
}

void qspi_apb_indirect_write(QSPI_FLASH_Def *plat, unsigned int dst, unsigned int n_bytes)
{
    unsigned int addr = dst & 0x00FFFFFF;
    
    /* Setup the indirect trigger address */
    writel(dst, plat->regbase + QSPI_REG_INDIRECTTRIGGER);

    /* Configure the indirect write transfer bytes */
    writel(n_bytes, plat->regbase + QSPI_REG_INDIRECTWRBYTES);

    /* Setup write address */
    writel(addr, plat->regbase + QSPI_REG_INDIRECTWRSTARTADDR);

    /* Start the indirect write transfer */
    writel(QSPI_REG_INDIRECTWR_START, plat->regbase + QSPI_REG_INDIRECTWR);
}

void qspi_apb_indirect_read(QSPI_FLASH_Def *plat, unsigned int src, unsigned int n_bytes)
{
    unsigned int addr = src & 0x00FFFFFF;

    /* Setup the indirect trigger address */
    writel(src, plat->regbase + QSPI_REG_INDIRECTTRIGGER);
    
    /* Configure the indirect write transfer bytes */
    writel(n_bytes, plat->regbase + QSPI_REG_INDIRECTRDBYTES);

    /* Setup read address */
    writel(addr, plat->regbase + QSPI_REG_INDIRECTRDSTARTADDR);

    /* Start the indirect read transfer */
    writel(QSPI_REG_INDIRECTRD_START, plat->regbase + QSPI_REG_INDIRECTRD);
}

void qspi_apb_set_read_instruction(unsigned char *reg_base, unsigned int config)
{
    /* Device Read Instruction Register */
    writel(config, reg_base + QSPI_REG_RD_INSTR);
}

void qspi_apb_set_write_instruction(unsigned char *reg_base, unsigned int config)
{
    /* Device Write Instruction Register */
    writel(config, reg_base + QSPI_REG_WR_INSTR);
}

void qspi_apb_interrupt_enable(unsigned char *reg_base, unsigned int intflags)
{
    unsigned long ulReg;
    
    ulReg = readl(reg_base + QSPI_REG_IRQMASK);
    
    ulReg |= intflags;
    
    writel(ulReg, reg_base + QSPI_REG_IRQMASK);
}

void qspi_apb_interrupt_disable(unsigned char *reg_base, unsigned int intflags)
{
    unsigned long ulReg;
    
    ulReg = readl(reg_base + QSPI_REG_IRQMASK);
    
    ulReg &= ~intflags;
    
    writel(ulReg, reg_base + QSPI_REG_IRQMASK);
}

unsigned long qspi_apb_interrupt_status_get(unsigned char *reg_base)
{
    return (readl(reg_base + QSPI_REG_IRQSTATUS));
}

void QSPIIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    //
    // Register the interrupt handler.
    //
    IntRegister(INT_QSPI, g_pRAMVectors, pfnHandler);

    //
    // Enable the UART interrupt.
    //
    IntEnable(INT_QSPI);
}

void QSPIIntUnregister(unsigned long *g_pRAMVectors)
{
    //
    // Disable the interrupt.
    //
    IntDisable(INT_QSPI);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(INT_QSPI, g_pRAMVectors);
}

//extern void qspi_workaround(void)
//{
//    HWREG(0xA00300C8) = 0x0F;
//    HWREG(0xA0030014) = (HWREG(0xA0030014) & 0xFFFF000F) | 0x40;
//}

