#include "softap_macro.h"
#if IS_DSP_CORE
#include "softap_macro.h"
#include "basetype.h"
#include "diag_cmd_cnf.h"
#include "diag_format.h"
#include "diag_out.h"
#include "Typedefine.h"
#include "xy_nbiot_module_define.h"
#include "os_adapt.h"
#include "Itemstruct.h"
#include "softap_api.h"
void diag_send_cmd_flash(U32 type_id, U32 cmd_id, u8 src_id, char* data, int nLen)
{
	U32 ret = 0;
	CommonCnf_t * msgHeader = 0;

	msgHeader = (CommonCnf_t*)OSXY_Debug_Alloc(sizeof(CommonCnf_t) + nLen + DIAG_TAIL_LEN, 1);

	if(msgHeader == NULL)
	{
		return;
	}	
	
	if(nLen > 0)
	{
		memcpy(msgHeader->u8Payload, data, nLen);
	}

	msgHeader->u8SrcId = src_id;
	msgHeader->u16Len = nLen;

	diag_packet_header((char*)msgHeader, sizeof(CommonCnf_t) + nLen, type_id, cmd_id);
	ret = diag_msg_out((char*)msgHeader, sizeof(CommonCnf_t) + nLen,  1);
	if(ret == LOG_FALSE)
	{
		xy_free(msgHeader);
	}
	
	return LOG_TRUE;
}

void diag_wireshark_dataM3_flash(char* data, int nLen, u8 type, u32 m3Time)
{
	U32 ret = 0;
	CommonCnf_t * msgHeader = 0;
    WireShark_t * wireShark = 0;
	int nAll = sizeof(CommonCnf_t) + sizeof(WireShark_t) + nLen;
	msgHeader = (CommonCnf_t*)OSXY_Debug_Alloc(nAll + DIAG_TAIL_LEN, 1);
	if(msgHeader == NULL)
	{
		return;
	}	
	
	wireShark = msgHeader->u8Payload;
	if(nLen > 0)
	{
		memcpy(wireShark->u8Payload, data, nLen);
	}

	msgHeader->u8SrcId = WIRESHARCK_M3;
	msgHeader->u16Len = nLen + sizeof(WireShark_t);
	wireShark->len = nLen;
	wireShark->type = type;
	diag_packet_header_m3((char*)msgHeader, nAll, XY_SYSAPPCNF_LOG, XY_SYSAPP_WIRESHARK_IND, m3Time);
	ret = diag_msg_out((char*)msgHeader, nAll,  1);	
	if(ret == LOG_FALSE)
	{
		xy_free(msgHeader);
	}
	
	return LOG_TRUE;
}

#endif

