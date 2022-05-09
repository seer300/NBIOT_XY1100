#include "softap_macro.h"
#if IS_DSP_CORE
#include "diag_protocol_print.h"
#include "xy_mem.h"
#include "xy_utils.h"

U32 print_nas_ram(u8 dirction, char *pMsg, int nLen)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
	    xy_free(pMsg);
		return LOG_FALSE;
	}
	
	return print_nas(dirction, pMsg, nLen);
}

U32 print_rrc_ram(u8 msg_id, char *pMsg, int nLen)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
	    xy_free(pMsg);
		return LOG_FALSE;
	}

	return print_rrc(msg_id, pMsg, nLen);
}

U32 print_asn1_ber_ram(u8 msg_id, char *pMsg, int nLen)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
	    xy_free(pMsg);
		return LOG_FALSE;
	}

	return print_asn1_ber(msg_id, pMsg, nLen);
}
#endif


