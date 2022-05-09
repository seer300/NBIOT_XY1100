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

void diag_send_cmd(U32 type_id, U32 cmd_id, u8 src_id, char* data, int nLen)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
	 	return;
	}
	
	return diag_send_cmd_flash(type_id, cmd_id, src_id, data, nLen);
}

void diag_wireshark_dataM3(char* data, int nLen, u8 type, u32 m3Time)
{
	if(is_print_log() != LOG_TRUE)
	{
		return;
	}

	return diag_wireshark_dataM3_flash(data, nLen, type, m3Time);
}

#endif
