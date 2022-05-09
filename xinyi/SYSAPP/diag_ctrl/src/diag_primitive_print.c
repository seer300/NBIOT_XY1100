#include "softap_macro.h"
#if IS_DSP_CORE
#include "softap_macro.h"
#include "diag_primitive_print.h"
#include "diag_format.h"
#include "diag_out.h"
#include "itemstruct.h"
#include "Typedefine.h"
#include "xy_nbiot_module_define.h"
#include "xy_nbiot_msg_define.h"
#include "os_adapt.h"
#include "xy_log.h"
#include "softap_api.h"

U16 max_log_size = 600;

u8 primitive_filter[MAX_PS_BLOCK][MAX_MSG_ID];

U32 diag_primitive_filter(U32 class_id, U32 msg_id)
{

	u8 index = (msg_id / 8);
	u8 bit_val = 1 << (msg_id % 8);
	
	if( class_id >= MAX_PS_BLOCK)
	{
		return LOG_FALSE;
	}

	if(index >= MAX_MSG_ID)
	{
		return LOG_FALSE;
	}
	
	if((primitive_filter[class_id][index] & bit_val) == 0)
	{
		return LOG_FALSE;
	}

	return LOG_TRUE;	
}


U32 print_primitive(char*pMsg)
{
	U32 ret  = 0;
	Message_t       * msgHeader  = (Message_t*)pMsg;
    MSG_HEADER_STRU *PsMsgHeader = (MSG_HEADER_STRU*)(pMsg+sizeof(Message_t));  
	if(diag_primitive_filter(PsMsgHeader->ulMsgClass , PsMsgHeader->ulMsgName) == LOG_FALSE)
	{
	    xy_free(msgHeader);
		return LOG_FALSE;
	}
	if(PsMsgHeader->ulMemSize >= max_log_size)
	{
		PrintLogSt(PLATFORM, WARN_LOG, "primitive size is more than %d msgclass %d, msgid %d, size %d", max_log_size, PsMsgHeader->ulMsgClass, PsMsgHeader->ulMsgName, PsMsgHeader->ulMemSize);
	}


	diag_packet_header((char*)msgHeader, sizeof(Message_t) + PsMsgHeader->ulMemSize, XY_MESSAGE_LOG, PsMsgHeader->ulMsgName);
	PsMsgHeader->ulMemSize -= sizeof(MSG_HEADER_STRU);
	ret = diag_msg_out((char*)msgHeader, sizeof(Message_t) + PsMsgHeader->ulMemSize,  1);

	if(ret == LOG_FALSE)
	{
		xy_free(msgHeader);
	}
	return LOG_TRUE;
}
#endif


