/*
 * diag_IsPrintLog.cxx
 *
 *  Created on: 2019-9-6
 *      Author: quzm
 */
#include "softap_macro.h"
#if IS_DSP_CORE
#include "os_adapt.h"
#include "diag_IsPrintLog.h"
#include "softap_api.h"

extern volatile int logview_alive;
u8 is_print_log()
{

	if(g_factory_nv->softap_fac_nv.open_log == 100)
	{
		return LOG_TRUE;
	}

	// open_log is 0 no log
	if(g_factory_nv->softap_fac_nv.open_log == 0 || logview_alive == 0)
	{
		return LOG_FALSE;
	}

	return LOG_TRUE;
}

u8 is_printDsp_log()
{
	if(g_factory_nv->softap_fac_nv.open_log == 100)
	{
		return LOG_TRUE;
	}
	

	if(g_factory_nv->softap_fac_nv.open_log != 1 || logview_alive == 0)
	{
		return LOG_FALSE;
	}

	return LOG_TRUE;
}
#endif


