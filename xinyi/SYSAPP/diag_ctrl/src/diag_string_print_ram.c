#include "softap_macro.h"
#if IS_DSP_CORE
#include "diag_string_print.h"
#include <stdarg.h>

U32 diag_static_log_ram(u8 src_id, u8 lev,const char*fmt,...)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
		return LOG_FALSE;
	}

	va_list      args;
 	va_start(args, fmt);    
	
	U32 ret = diag_static_log(src_id, lev, fmt, args);
	va_end(args);
	return ret;
}

U32 diag_static_logM3_ram(u8 src_id,u8 lev, u32 m3Time, const char* log_buf)
{
	if(is_print_log() != LOG_TRUE)
	{
		return LOG_FALSE;
	}

	
	U32 ret = diag_static_logM3(src_id, lev, m3Time, log_buf);
	return ret;
}

U32 diag_dynamic_log_ram(int dyn_id, u8 src_id,u8 lev, char *fmt, ...)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
		return LOG_FALSE;
	}

	va_list      args;
 	va_start(args, fmt);    
	
	U32 ret = diag_dynamic_log(dyn_id, src_id, lev, fmt, args);
	va_end(args);
	return ret;
}
						
#endif
