
/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "at_ps_cmd.h"
#include "low_power.h"
#include <cis_if_sys.h>
#include <cis_def.h>
#include <cis_config.h>
#include <cis_api.h>
#include "at_onenet.h"
#include "xy_system.h"
/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/

/*******************************************************************************
 *                        Local function declarations                          *
 ******************************************************************************/

/*******************************************************************************
 *                         Local variable definitions                          *
 ******************************************************************************/

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/

/*******************************************************************************
 *                      Inline function implementations                        *
 ******************************************************************************/

/*******************************************************************************
 *                      Local function implementations                         *
 ******************************************************************************/

/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/
cis_ret_t cissys_init(void *context,const cis_cfg_sys_t* cfg,cissys_callback_t* event_cb)
{
	(void) cfg;

#if	CIS_ENABLE_UPDATE
	st_context_t *contextP = (st_context_t *)context;
	if(contextP == NULL)
		return CIS_RET_ERROR;
	//xy_printf("[~~~~~~]init fw callback");
	contextP->g_fotacallback.onEvent = event_cb->onEvent;
	contextP->g_fotacallback.userData = event_cb->userData;
	
//	cis_memcmp(&contextP->g_fotacallback, event_cb, sizeof(cissys_callback_t));
#endif	
    return CIS_RET_OK;
}
clock_t	cissys_tick(void)
{
    return osKernelGetTickCount();
}
uint32_t cissys_gettime()
{
    return xy_rtc_get_sec(0); //osKernelGetTickCount(); //global_timepoint_get(0);
}
void cissys_sleepms(uint32_t ms)
{
    osDelay(ms);
}
void cissys_logwrite(uint8_t* buffer,uint32_t length)
{
	(void) length;

	xy_printf("%s", buffer);
}

#if 0
void* cissys_malloc(size_t length)
{
    return xy_zalloc(length);
}
#endif
void cissys_free(void* buffer)
{
    xy_free(buffer);
}

void* cissys_memset( void* s, int c, size_t n)
{
    return memset(s, c, n);
}
void* cissys_memcpy(void* dst, const void* src, size_t n)
{
	return memcpy(dst, src, n);
}
void* cissys_memmove(void* dst, const void* src, size_t n)
{
	return memmove(dst, src, n);
}
int cissys_memcmp( const void* s1, const void* s2, size_t n)
{
    return memcmp(s1, s2, n);
}
void cissys_fault(uint16_t id)
{
	(void) id;

	xy_printf("cissys_fault");
}
uint32_t cissys_rand()
{
    return osKernelGetTickCount() + global_timepoint_get(0);
}

void cissys_assert(bool flag)
{
	xy_assert(flag);
}

bool cissys_load(uint8_t* buffer,uint32_t length)
{
	(void) buffer;
	(void) length;

    return true;
}
bool cissys_save(uint8_t* buffer,uint32_t length)
{
	(void) buffer;
	(void) length;

    return true;
}

uint8_t     cissys_getSN(char* buffer,uint32_t maxlen)
{
	
	if(maxlen < 32)
		return 0;
	
	xy_get_SN(buffer, maxlen);
		
	//xy_printf("TEST  SN： %s", buffer);
	return strlen(buffer);
}

void cissys_reboot()
{
	xy_reboot();
}

#endif
