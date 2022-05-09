#include "softap_macro.h"
#include "xy_nbiot_module_define.h"
#define HEADERLEN 3
#define MAX_PACKET_LEN 	300
#define DATALENSIZE sizeof(unsigned short)
extern unsigned char log_filter[XYLOG_MAX_BLOCK];
unsigned char charaucATBuffer[MAX_PACKET_LEN]={'\0'};

#if IS_DSP_CORE
#include "diag_read_msg.h"
#include "basetype.h"
#include "TypeDefine.h"
#include "sysappreq.h"
#include "diag_out.h"
#include "diag_mem_dump.h"
#include "ItemStruct.h"
#include "hw_memmap.h"
#include "DataTypes.h"
#include "uart.h"
#include "csp.h"
#include "os_adapt.h"
#include "softap_api.h"
#include "sysappcnf.h"
#include "diag_cmd_cnf.h"

extern	u8 air_filter;
extern	u8 primitive_filter[MAX_PS_BLOCK][MAX_MSG_ID];

extern U16 max_log_size;

extern volatile int logview_alive;
extern unsigned char    aucIternalVersion[];
extern volatile int log_fifo_full_cnt;
extern volatile int log_memory_full_cnt;
extern volatile int xy_log_size;

#else
#include "diag_read_msg.h"
#include "basetype.h"
#include "TypeDefine.h"
#include "diag_out.h"
#include "ItemStruct.h"
#include "hw_memmap.h"
#include "uart.h"
#include "csp.h"
#include "xy_utils.h"
#include "xy_utils.h"
#include "xy_nbiot_module_define.h"
#include "diag_string_print.h"
#include "sysappreq.h"
#include "diag_mem_dump.h"
#endif

unsigned char IsItemHeaderChar(unsigned int index)
{
	volatile unsigned char tempChar = 0;

	switch(index)
	{
		case 0:
			tempChar = 0x5A;
			break;
		case 1:
			tempChar = 0xA5;
			break;
		case 2:
			tempChar = 0xFE;
			break;
		default:
			tempChar = 0xFF;
			break;
	}
	return tempChar;
}


unsigned int handleSysApp(unsigned int subCmdId, unsigned char *subCmdBuf)
{
	char szTmp[64];
	ItemHeader_t *tmp_ptr = (ItemHeader_t *)subCmdBuf;
	filterInfo * filter = NULL;
	unsigned int  Ret = LOG_TRUE;
	unsigned short len = tmp_ptr->u16Len - sizeof(ItemHeader_t) + HEADERLEN + DATALENSIZE + 1;
	switch(subCmdId)
	{
#if IS_DSP_CORE
		case XY_SYSAPP_ASSERT_REQ:
			xy_assert(0);
			break;
		case XY_SYSAPP_FILTER_REQ:			
				filter = tmp_ptr->u8Payload;
				memcpy(log_filter, filter->log_filter, XYLOG_MAX_BLOCK);
				air_filter = filter->air_filter;
				memcpy(primitive_filter, filter->primitive_filter, MAX_PS_BLOCK * MAX_MSG_ID);
				diag_send_cmd(XY_SYSAPPCNF_LOG, XY_SYSAPP_FILTER_CNF,PLATFORM, NULL, 0);
			break;
		case XY_SYSAPP_MAXLEN_REQ:
			{
				memcpy(&max_log_size,tmp_ptr->u8Payload, sizeof(U16));
				diag_send_cmd(XY_SYSAPPCNF_LOG, XY_SYSAPP_MAXLEN_CNF,PLATFORM, NULL, 0);
			}
			break;
		case XY_SYSAPP_HEART_REQ:
			{
				//first revceive heart cnf
				if(tmp_ptr->u16SeqNum == 0 || logview_alive == 1)
				{
					diag_send_cmd(XY_SYSAPPCNF_LOG, XY_SYSAPP_HEART_CNF,PLATFORM, aucIternalVersion, strlen(aucIternalVersion));
				}
				else
				{
					if(log_memory_full_cnt != 0 || log_fifo_full_cnt != 0)
					{
						PrintLogSt(PLATFORM, WARN_LOG, "logInfo: list lost %d momery lost %d momery  use space %d", log_fifo_full_cnt, log_memory_full_cnt, xy_log_size);
						log_fifo_full_cnt = 0;
						log_memory_full_cnt = 0;
					}
				}	
				logview_alive = 2;
			}
			break;
#else
		(void)len;
		(void)filter;
		(void)szTmp;
#endif
#if !IS_DSP_CORE
		case XY_AT_CMD:
		 	{
		 	}
				break;
#endif
		default:
			Ret = LOG_FALSE;
			break;
	}

	return Ret;
}


void uart_cmd_process(unsigned char *str)
{
	ItemHeader_t *tmp_ptr = (ItemHeader_t *)str;

	switch(tmp_ptr->u4TraceId)
	{
		case XY_SYSAPPREQ_LOG:
		{
			handleSysApp(tmp_ptr->u28ClassId, str);

		}
			break;
		default:
			break;
	}
	return ;

}

void uart_cmd_dump(unsigned char *str)
{
	ItemHeader_t *tmp_ptr = (ItemHeader_t *)str;
	switch(tmp_ptr->u4TraceId)
	{
		case XY_SYSAPPREQ_LOG:
			if(tmp_ptr->u28ClassId == XY_SYSAPP_MEMREADY_REQ)
			{				
				set_pc_info(1);
			}
			break;
		default:
			break;
	}
	return ;

}

enum
{
	REV_HEADER,
	REV_LEN,
	REV_DATA,
	REV_CHKSUM,
	REV_CHIP,
};

volatile int status = REV_HEADER;
volatile unsigned short log_index = 0;
volatile unsigned char chipType = Chip_1100;
void checkIndex()
{
	if(log_index >= MAX_PACKET_LEN)
	{
		log_index = 0;
		status = REV_HEADER;
	}
}

void ResetMem()
{
	log_index = 0;
	status = REV_HEADER;
}
extern int at_write_to_uart(char *buf,int size);
void ParseData(unsigned char cspATCdata, char bDumpFile)
{
	charaucATBuffer[log_index] = (unsigned char)cspATCdata;
	volatile int i = 0;
	switch(status)
	{
		case REV_HEADER:
		{
			if(IsItemHeaderChar(log_index) == charaucATBuffer[log_index])
			{
				log_index++;
			}
			else
			{
				log_index = 0;
			}

			if(log_index == HEADERLEN)
			{
				status = REV_CHIP;
			}
			
		}
			break;
		case REV_CHIP:
		{
			chipType = charaucATBuffer[log_index];
			log_index++;
			status = REV_LEN;
		}
			break;
		case REV_LEN:
		{
			if(log_index == HEADERLEN + DATALENSIZE)
			{
				 status = REV_DATA;
			}

			log_index ++;
			checkIndex();
		}
			break;
		case REV_DATA:
		{
			volatile ItemHeader_t *log_itemHeader = (ItemHeader_t *)charaucATBuffer;
			 if(log_itemHeader->u16Len > MAX_PACKET_LEN || log_itemHeader->u16Len == 0)
			 {
				  ResetMem();
				  break;
			 }

			if(log_index == log_itemHeader->u16Len + HEADERLEN + DATALENSIZE)
			{
				if(chipType == Chip_1100)
				{
					if(bDumpFile == REV_PCCMD)
					{
						uart_cmd_process(charaucATBuffer);
					}
					else if(bDumpFile == START_DUMP)
					{
						uart_cmd_dump(charaucATBuffer);
					}
					
					ResetMem();
					break;
					
				}
				else if(chipType == Chip_1100E)
				{
					status = REV_CHKSUM;
				}
			}
			
			log_index++;
			checkIndex();
		}
			break;
		case REV_CHKSUM:
		{				
			unsigned char cData = 0x00;
			for(i = 0; i < log_index; i++)
			{
				cData ^= charaucATBuffer[i];
			}

			if(cData == cspATCdata)
			{
				if(bDumpFile == REV_PCCMD)
				{
					uart_cmd_process(charaucATBuffer);
				}
				else if(bDumpFile == START_DUMP)
				{
					uart_cmd_dump(charaucATBuffer);
				}
			}
			ResetMem();
		}
			break;
		default:
			break;
	}

}

void PC_CMD(char bDumpFile)
{
#if !IS_DSP_CORE
	if(g_softap_fac_nv->open_log == 4)
	{
		while(!(HWREG(CSP4_BASE + CSP_RX_FIFO_STATUS) & CSP_RX_FIFO_STATUS_EMPTY_Msk))
		{
			unsigned char cspATCdata = CSPCharGet(CSP4_BASE);
			ParseData(cspATCdata, bDumpFile);
		}
	}
	else
	{
		while(!(HWREG(CSP3_BASE + CSP_RX_FIFO_STATUS) & CSP_RX_FIFO_STATUS_EMPTY_Msk))
		{
			unsigned char cspATCdata = CSPCharGet(CSP3_BASE);
			ParseData(cspATCdata, bDumpFile);
		}
	}
#else
	while(!(HWREG(CSP3_BASE + CSP_RX_FIFO_STATUS) & CSP_RX_FIFO_STATUS_EMPTY_Msk))
	{
		unsigned char cspATCdata = CSPCharGet(CSP3_BASE);
		ParseData(cspATCdata, bDumpFile);
	}
#endif
}

#if IS_DSP_CORE
void xy_diag_read(void)
{
  for(;;)
  {  	  
      if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY))
	  {
		  PC_CMD(REV_PCCMD);
  	  }
  }
}

#endif
