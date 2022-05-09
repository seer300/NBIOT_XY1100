#include "softap_macro.h"
#if IS_DSP_CORE
#include "diag_primitive_print.h"
#include "xy_mem.h"
#include "xy_utils.h"

U32 print_primitive_ram(char*pMsg)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
	    xy_free(pMsg);
	 	return LOG_FALSE;
	}
	
	
	return print_primitive(pMsg);
}
#endif

