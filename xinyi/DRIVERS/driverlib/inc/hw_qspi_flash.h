#ifndef __HW_QSPI_FLASH_H__
#define __HW_QSPI_FLASH_H__

/* Transfer mode */
#define QSPI_INST_TYPE_SINGLE			0
#define QSPI_INST_TYPE_DUAL			1
#define QSPI_INST_TYPE_QUAD			2

#define QSPI_STIG_DATA_LEN_MAX			8

#define QSPI_DUMMY_CLKS_PER_BYTE		8
#define QSPI_DUMMY_BYTES_MAX			4

/****************************************************************************
 * Controller's configuration and status register (offset from QSPI_BASE)
 ****************************************************************************/
#define	QSPI_REG_CONFIG                     0x00
#define	QSPI_REG_CONFIG_ENABLE              BIT(0)
#define	QSPI_REG_CONFIG_CLK_POL             BIT(1)
#define	QSPI_REG_CONFIG_CLK_PHA             BIT(2)
#define	QSPI_REG_CONFIG_DIRECT              BIT(7)
#define	QSPI_REG_CONFIG_DECODE              BIT(9)
#define	QSPI_REG_CONFIG_XIP_NEXT_READ       BIT(17)
#define	QSPI_REG_CONFIG_XIP_IMM             BIT(18)
#define	QSPI_REG_CONFIG_CHIPSELECT_LSB      10
#define	QSPI_REG_CONFIG_BAUD_LSB            19
#define	QSPI_REG_CONFIG_IDLE_LSB            31
#define	QSPI_REG_CONFIG_CHIPSELECT_MASK     0xF
#define	QSPI_REG_CONFIG_BAUD_MASK           0xF

#define	QSPI_REG_RD_INSTR                   0x04
#define	QSPI_REG_RD_INSTR_OPCODE_LSB		0
#define	QSPI_REG_RD_INSTR_TYPE_INSTR_LSB	8
#define	QSPI_REG_RD_INSTR_TYPE_ADDR_LSB	    12
#define	QSPI_REG_RD_INSTR_TYPE_DATA_LSB	    16
#define	QSPI_REG_RD_INSTR_MODE_EN_LSB		20
#define	QSPI_REG_RD_INSTR_DUMMY_LSB		    24
#define	QSPI_REG_RD_INSTR_TYPE_INSTR_MASK	0x3
#define	QSPI_REG_RD_INSTR_TYPE_ADDR_MASK	0x3
#define	QSPI_REG_RD_INSTR_TYPE_DATA_MASK	0x3
#define	QSPI_REG_RD_INSTR_DUMMY_MASK		0x1F

#define	QSPI_REG_WR_INSTR                   0x08
#define	QSPI_REG_WR_INSTR_OPCODE_LSB        0

#define	QSPI_REG_DELAY                      0x0C
#define	QSPI_REG_DELAY_TSLCH_LSB            0
#define	QSPI_REG_DELAY_TCHSH_LSB		    8
#define	QSPI_REG_DELAY_TSD2D_LSB		    16
#define	QSPI_REG_DELAY_TSHSL_LSB		    24
#define	QSPI_REG_DELAY_TSLCH_MASK		    0xFF
#define	QSPI_REG_DELAY_TCHSH_MASK		    0xFF
#define	QSPI_REG_DELAY_TSD2D_MASK		    0xFF
#define	QSPI_REG_DELAY_TSHSL_MASK		    0xFF

#define	QSPI_REG_RD_DATA_CAPTURE		        0x10
#define	QSPI_REG_RD_DATA_CAPTURE_BYPASS	        BIT(0)
#define	QSPI_REG_RD_DATA_CAPTURE_DELAY_LSB	    1
#define	QSPI_REG_RD_DATA_CAPTURE_DELAY_MASK	    0xF

#define	QSPI_REG_SIZE				    0x14
#define	QSPI_REG_SIZE_ADDRESS_LSB       0
#define	QSPI_REG_SIZE_PAGE_LSB          4
#define	QSPI_REG_SIZE_BLOCK_LSB         16
#define	QSPI_REG_SIZE_ADDRESS_MASK      0xF
#define	QSPI_REG_SIZE_PAGE_MASK         0xFFF
#define	QSPI_REG_SIZE_BLOCK_MASK        0x3F

#define	QSPI_REG_SRAMPARTITION			0x18
#define	QSPI_REG_INDIRECTTRIGGER		0x1C

#define	QSPI_REG_REMAP                  0x24
#define	QSPI_REG_MODE_BIT			    0x28

#define	QSPI_REG_SDRAMLEVEL			    0x2C
#define	QSPI_REG_SDRAMLEVEL_RD_LSB		0
#define	QSPI_REG_SDRAMLEVEL_WR_LSB		16
#define	QSPI_REG_SDRAMLEVEL_RD_MASK     0xFFFF
#define	QSPI_REG_SDRAMLEVEL_WR_MASK     0xFFFF

#define	QSPI_REG_IRQSTATUS                  0x40
#define	QSPI_REG_IRQMASK                    0x44
#define QSPI_REG_IRQ_MODE_FAIL              BIT(0)
#define QSPI_REG_IRQ_UNDERFLOW              BIT(1)
#define QSPI_REG_IRQ_END_TRIGGER_INDIR      BIT(2)
#define QSPI_REG_IRQ_INDIR_NOT_ACCEPT       BIT(3)
#define QSPI_REG_IRQ_WR_PROTECTED_ERAR      BIT(4)
#define QSPI_REG_IRQ_ILLEGAL_AHB_ACCESS     BIT(5)
#define QSPI_REG_IRQ_INDIR_WATERMARK        BIT(6)
#define QSPI_REG_IRQ_RECV_OVERFLOW          BIT(7)
#define QSPI_REG_IRQ_TXFIFO_NFULL           BIT(8)
#define QSPI_REG_IRQ_TXFIFO_FULL            BIT(9)
#define QSPI_REG_IRQ_RXFIFO_NEMPTY          BIT(10)
#define QSPI_REG_IRQ_RXFIFO_FULL            BIT(11)
#define QSPI_REG_IRQ_INDIR_RD_SRAM_FULL     BIT(12)
#define QSPI_REG_IRQ_POLL_CYCLE_EXPIRED     BIT(13)
#define QSPI_REG_IRQ_ALL                    0x3FFF

#define	QSPI_REG_INDIRECTRD			    0x60
#define	QSPI_REG_INDIRECTRD_START		BIT(0)
#define	QSPI_REG_INDIRECTRD_CANCEL		BIT(1)
#define	QSPI_REG_INDIRECTRD_INPROGRESS	BIT(2)
#define	QSPI_REG_INDIRECTRD_DONE		BIT(5)

#define	QSPI_REG_INDIRECTRDWATERMARK	0x64
#define	QSPI_REG_INDIRECTRDSTARTADDR	0x68
#define	QSPI_REG_INDIRECTRDBYTES		0x6C

#define	QSPI_REG_INDIRECTTRIGGER_RANGE	0x80

#define	QSPI_REG_CMDCTRL			        0x90
#define	QSPI_REG_CMDCTRL_EXECUTE		    BIT(0)
#define	QSPI_REG_CMDCTRL_INPROGRESS	        BIT(1)
#define	QSPI_REG_CMDCTRL_DUMMY_LSB		    7
#define	QSPI_REG_CMDCTRL_WR_BYTES_LSB	    12
#define	QSPI_REG_CMDCTRL_WR_EN_LSB		    15
#define	QSPI_REG_CMDCTRL_ADD_BYTES_LSB	    16
#define	QSPI_REG_CMDCTRL_ADDR_EN_LSB	    19
#define	QSPI_REG_CMDCTRL_RD_BYTES_LSB	    20
#define	QSPI_REG_CMDCTRL_RD_EN_LSB		    23
#define	QSPI_REG_CMDCTRL_OPCODE_LSB	        24
#define	QSPI_REG_CMDCTRL_DUMMY_MASK	        0x1F
#define	QSPI_REG_CMDCTRL_WR_BYTES_MASK		0x7
#define	QSPI_REG_CMDCTRL_ADD_BYTES_MASK	    0x3
#define	QSPI_REG_CMDCTRL_RD_BYTES_MASK		0x7
#define	QSPI_REG_CMDCTRL_OPCODE_MASK		0xFF

#define	QSPI_REG_INDIRECTWR			        0x70
#define	QSPI_REG_INDIRECTWR_START		    BIT(0)
#define	QSPI_REG_INDIRECTWR_CANCEL		    BIT(1)
#define	QSPI_REG_INDIRECTWR_INPROGRESS		BIT(2)
#define	QSPI_REG_INDIRECTWR_DONE		    BIT(5)

#define	QSPI_REG_INDIRECTWRWATERMARK		0x74
#define	QSPI_REG_INDIRECTWRSTARTADDR		0x78
#define	QSPI_REG_INDIRECTWRBYTES		    0x7C

#define	QSPI_REG_CMDADDRESS			        0x94
#define	QSPI_REG_CMDREADDATALOWER		    0xA0
#define	QSPI_REG_CMDREADDATAUPPER		    0xA4
#define	QSPI_REG_CMDWRITEDATALOWER		    0xA8
#define	QSPI_REG_CMDWRITEDATAUPPER		    0xAC

#define QSPI_REG_IS_IDLE(base)              ((readl(base + QSPI_REG_CONFIG) >> QSPI_REG_CONFIG_IDLE_LSB) & 0x1)

#define QSPI_GET_RD_SRAM_LEVEL(reg_base)    (((readl(reg_base + QSPI_REG_SDRAMLEVEL)) >> QSPI_REG_SDRAMLEVEL_RD_LSB) & QSPI_REG_SDRAMLEVEL_RD_MASK)

#define QSPI_GET_WR_SRAM_LEVEL(reg_base)    (((readl(reg_base + QSPI_REG_SDRAMLEVEL)) >> QSPI_REG_SDRAMLEVEL_WR_LSB) & QSPI_REG_SDRAMLEVEL_WR_MASK)


#endif // __HW_QSOI_FLASH_H__
