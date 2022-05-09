#include "softap_macro.h"
#if IS_DSP_CORE

#include "softap_api.h"
#include "diag_out.h"
#include "ItemStruct.h"
#include "xy_log_send.h"
#include "diag_format.h"
#include "os_adapt.h"
extern int log_fifo_full_cnt;

int at_diag_info(char *at_buf,char **prsp_cmd)
{
	char cmd[10]={0};
	int  ret = 0;
	char  aucImeiSnBuf[NVM_MAX_SN_LEN + 1]={0};
	char *pp[1];
	char *result = NULL;
	int  i = 0;
	result = pvPortMalloc(256);
	memset(result, 0x0, 256);
	pp[0] = cmd;
	
	if (at_parse_param("%s,", at_buf, pp) != AT_OK)
	{
		*prsp_cmd = at_err_build(ATERR_PARAM_INVALID);
		vPortFree(result);
		return AT_END;
	}

	if (strstr(cmd,"LOSTINF"))
	{
		sprintf(result,"\r\n+LOSTINF:%d,%d,%d\r\n",log_fifo_full_cnt, log_memory_full_cnt, xy_log_size);
		AT_SEDN_TO_EXT(result);
	}


	return AT_END;

}

#else
#include "diag_out.h"
#include "ItemStruct.h"
#include "xy_utils.h"
#include "xy_log_send.h"
#include "diag_format.h"
#endif

unsigned int diag_msg_out(const char*  buf,
		unsigned int       buf_sz,
		unsigned char canLoss)
{

	(void)canLoss;
	return xy_log_trace_msg_send((char*)buf,  buf_sz);
}


void diag_send_data(const char*  buf,
		unsigned int        buf_sz)
{
	xy_log_trace((char*)buf, buf_sz);
}


extern unsigned short seq_no;
char szLogData[24];
void diag_send_header(unsigned short	trace_id,
		unsigned int	class_id,
		unsigned int 	buf_len)
{

	ItemHeader_t* itemHeader = (ItemHeader_t*)szLogData;
	szLogData[0] = 0x5A;
	szLogData[1] = 0xA5;
	szLogData[2] = 0xFE;
	szLogData[3] = Chip_1100; 
	itemHeader->u16Len = sizeof(ItemHeader_t) - HEADERSIZE - LENSIZE + buf_len;
	itemHeader->u28ClassId = class_id;
	itemHeader->u4TraceId = trace_id;
	itemHeader->u16SeqNum = get_Seq_num();
	itemHeader->u32Time = 0;
	xy_log_trace((char*)itemHeader, sizeof(ItemHeader_t));
}


