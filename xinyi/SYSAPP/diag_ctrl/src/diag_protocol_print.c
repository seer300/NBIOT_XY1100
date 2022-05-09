#include "softap_macro.h"
#if IS_DSP_CORE
#include "softap_macro.h"
#include "diag_protocol_print.h"
#include "diag_format.h"
#include "diag_out.h"
#include "itemstruct.h"
#include "Typedefine.h"
#include "xy_nbiot_module_define.h"
#include "xy_nbiot_msg_define.h"
#include "os_adapt.h"
#include "softap_api.h"

#define RRC_TYPE 0
#define NAS_TYPE 1
#define ASN1_BER_TYPE 2


u8 air_filter = 0xFF;

U32 diag_protocol_filter(u8 u8Type)
{
	u8 u8Filter = (1 << u8Type);
	if( u8Type >= 0xFF)
	{
		return LOG_FALSE;
	}

	if((air_filter & u8Filter) == 0)
	{
		return LOG_FALSE;
	}

	return LOG_TRUE;	
}


U32 print_nas(u8 dirction, char *pMsg, int nLen)
{
	int nSize = sizeof(Protocol_t) + sizeof(NasHeader_t) + nLen;
	U32 ret = LOG_TRUE;

	if(diag_protocol_filter(NAS_TYPE) == LOG_FALSE)
	{
	    xy_free(pMsg);
		return LOG_FALSE;
	}
	
	char* log_str = pMsg;//(char*)OSXY_Mem_Alloc(nSize);

	Protocol_t *prot = (Protocol_t *)log_str;
	prot->u8Type = NAS_TYPE;
	prot->u8Ver = 0;
	prot->u16Len = sizeof(NasHeader_t) + nLen;

	NasHeader_t *nas_header = (NasHeader_t *)prot->u8Payload;
	nas_header->u8Direction = dirction;
	nas_header->u16Len = nLen;

	diag_packet_header((char*)log_str, nSize, XY_PROTOCOL_LOG, NAS_TYPE);
	ret = diag_msg_out(log_str, nSize, 0);

	if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	
	return LOG_TRUE;
}

U32 print_rrc(u8 msg_id, char *pMsg, int nLen)
{
	int nSize = sizeof(Protocol_t) + sizeof(RrcHeader_t) + nLen;
	U32 ret = LOG_TRUE;
	
	if(diag_protocol_filter(RRC_TYPE) == LOG_FALSE)
	{
	    xy_free(pMsg);
		return LOG_FALSE;
	}
	
	char* log_str = pMsg;//(char*)OSXY_Mem_Alloc(nSize);

	Protocol_t *prot = (Protocol_t *)log_str;
	prot->u8Type = RRC_TYPE;
	prot->u8Ver = 0;
	prot->u16Len = sizeof(RrcHeader_t) + nLen;
	
	RrcHeader_t *rrc_header = (RrcHeader_t *)prot->u8Payload;
	rrc_header->u8MsgId = msg_id;
	rrc_header->u16Len = nLen;
	
	diag_packet_header((char*)log_str, nSize, XY_PROTOCOL_LOG, RRC_TYPE);
	ret = diag_msg_out(log_str, nSize, 0);

	if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	return LOG_TRUE;
}

U32 print_asn1_ber(u8 msg_id, char *pMsg, int nLen)
{
	int nSize = sizeof(Protocol_t) + sizeof(RrcHeader_t) + nLen;
	U32 ret = LOG_TRUE;
	
	if(diag_protocol_filter(ASN1_BER_TYPE) == LOG_FALSE)
	{
	    xy_free(pMsg);
		return LOG_FALSE;
	}
	
	char* log_str = pMsg;//(char*)OSXY_Mem_Alloc(nSize);

	Protocol_t *prot = (Protocol_t *)log_str;
	prot->u8Type = ASN1_BER_TYPE;
	prot->u8Ver = 0;
	prot->u16Len = sizeof(RrcHeader_t) + nLen;

	RrcHeader_t *rrc_header = (RrcHeader_t *)prot->u8Payload;
	rrc_header->u8MsgId = msg_id;
	rrc_header->u16Len = nLen;

	diag_packet_header((char*)log_str, nSize, XY_PROTOCOL_LOG, ASN1_BER_TYPE);
	ret = diag_msg_out(log_str, nSize, 0);

	if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	return LOG_TRUE;
}

#endif

