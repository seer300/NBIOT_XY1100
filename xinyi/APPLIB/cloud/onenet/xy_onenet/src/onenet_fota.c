
/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#if MOBILE_VER
#include "xy_system.h"
#include "xy_utils.h"
#include "at_onenet.h"
#include <cis_if_sys.h>
#include <cis_def.h>
#include <cis_config.h>
#include <cis_api.h>
#include <qspi_flash.h>
#include "dma.h"
#include "sha.h"
#include "rtc_tmr.h"
#include "xy_fota.h"
#include "factory_nv.h"
#include "cloud_utils.h"
#include "xy_watchdog.h"
/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/

#define CIS_FOTA_OFFSET		3600

/*******************************************************************************
 *                        Local function declarations                          *
 ******************************************************************************/

/*******************************************************************************
 *                         Local variable definitions                          *
 ******************************************************************************/

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
static unsigned int  s_cis_fwState = 0;				//cis fota update state
static unsigned int  s_cis_fwRes = 0;				//cis fota update result
static unsigned int  s_cis_fwSize = 0;				//cis fota update data size

unsigned int rcv_size = 0;
osTimerId_t cis_fota_timer_overdue = NULL;

extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];
extern int g_FOTAing_flag;
/*******************************************************************************
 *                      Inline function implementations                        *
 ******************************************************************************/

/*******************************************************************************
 *                      Local function implementations                         *
 ******************************************************************************/

//Get firmware version
uint32_t cissys_getFwVersion(uint8_t **version)
{
	char *tmpVersion = (char *)*version;
	unsigned int len = 0;
	sprintf(tmpVersion, "%s", g_softap_fac_nv->versionExt);

	xy_printf("[CIS] get fwVersion:%s", *version);

	len = strlen(tmpVersion);
	return len;	
}

//Get cell id from nv
uint32_t cissys_getCellId(int *cellid)
{
	xy_get_CELLID(cellid);
}

//Get battery level (TODO)
uint32_t cissys_getFwBatteryLevel()
{
	//TO DO
	uint32_t btLevel = xy_getVbatCapacity();
	xy_printf("[CIS] battery level:%u",btLevel);
	return btLevel;
}

//Get battery voltage (TODO)
uint32_t cissys_getFwBatteryVoltage()
{
	//TO DO
	uint32_t btVoltage = xy_getVbat();
	xy_printf("[CIS] battery Voltage:%u(mv)",btVoltage);

	return btVoltage;
}

//Get available memory for firmware udpate (TODO)
uint32_t cissys_getFwAvailableMemory()
{
	uint32_t availableMemory = cloud_get_ResveredMem();
	xy_printf("[CIS] available Memory:%u(Byte)",availableMemory);

	return availableMemory;
}

//Get radio signal strength (TODO)
uint32_t cissys_getRadioSignalStrength()
{
	//TO DO
	int radioSignalStrength = -1;
	xy_get_RSSI(&radioSignalStrength);
	xy_printf("[CIS] RadioSignal Strength:%u",radioSignalStrength);

	return (uint32_t)radioSignalStrength;
}



//check firmware data after downloading (TODO)
bool cissys_checkFwValidation(cissys_callback_t *cb)
{
	if(ota_delta_check())
	{
	    cb->onEvent((cissys_event_t)cissys_event_fw_validate_fail, NULL, cb->userData, 0);
		return false;

	}
	else
	{
    	cb->onEvent((cissys_event_t)cissys_event_fw_validate_success, NULL, cb->userData, 0);
		return true;
	}
}

//erase flash data after updating (TODO)
bool cissys_eraseFwFlash(cissys_callback_t *cb)
{
	//TO DO 
	if(1)
	{
		//erase fota data success
		cb->onEvent((cissys_event_t)cissys_event_fw_erase_success, NULL, cb->userData, 0);
		return true;
	}
	else
	{
		//erase fota data failed
		cb->onEvent((cissys_event_t)cissys_event_fw_erase_fail, NULL, cb->userData, 0);
		return false;
	}
}

//clear the firmware size info (TODO)
void cissys_ClearFwBytes(void)
{
	s_cis_fwSize = 0;
}

//save firmware data and write to flash
uint32_t	cissys_writeFwBytes(uint32_t size, uint8_t * buffer, cissys_callback_t * cb)
{
	//unsigned long offset = 0;
	st_context_t *context = cb->userData;
	if((s_cis_fwSize+size) > cloud_get_ResveredMem())
	{
		cb->onEvent((cissys_event_t)cissys_event_write_fail, NULL, cb->userData, 0);
		return 1;
	}
	
    if(ota_write_to_flash(rcv_size, buffer, size))
    {
        cb->onEvent((cissys_event_t)cissys_event_write_fail, NULL, cb->userData, 0);
        return 1;
    }
    
#if CIS_ENABLE_UPDATE	
	if(context->fw_request.block1_more == 0)
	{			
        softap_printf(USER_LOG, WARN_LOG, "[CIS]last fota pkt");
	}
#endif	

    rcv_size = rcv_size + size;
	softap_printf(USER_LOG, WARN_LOG, "[CIS]block size:%d, had rcv_size:%d", size, rcv_size);
    
	cb->onEvent((cissys_event_t)cissys_event_write_success, NULL, cb->userData, 0);
	return 1;
	
}


//record the firmware data size
void cissys_savewritebypes(uint32_t size)
{
	s_cis_fwSize = s_cis_fwSize + size;
}

//execute updating command after downloading firmware data
bool cissys_updateFirmware(cissys_callback_t * cb)
{
    cb->onEvent((cissys_event_t)cissys_event_fw_update_success, NULL, cb->userData, 0);
    return true;

}

//read fota context 
bool cissys_readContext(cis_fw_context_t * context)
{
	cis_fw_context_t *fw_context = context;
	if(NULL == fw_context)
		return 1;

	fw_context->state = s_cis_fwState;
	fw_context->result = s_cis_fwRes;
	fw_context->savebytes = s_cis_fwSize;

	return 0;
}

//Get saved firmware data size 
int cissys_getFwSavedBytes()
{
	return 	s_cis_fwSize;
}

//Set updating result and reboot
bool cissys_setFwUpdateResult(uint8_t result)
{
	s_cis_fwRes = result;
	xy_printf("[CIS] fw result:%d", result);
	if(result != FOTA_RESULT_DISCONNECT)
	{
        if(cis_fota_timer_overdue != NULL)
        {
            osTimerDelete(cis_fota_timer_overdue);
            cis_fota_timer_overdue = NULL;
        }
	}
	
	if(result == FOTA_RESULT_SUCCESS)
	{
		osDelay(4000);
#if XY_FOTA
		ota_update_start();
#endif
	}

	return true;
}

static void cissys_rtc_cb(void *para)
{
	int fota_ret = 2;       //fota fail
	//st_context_t* context = (st_context_t*)para;
	//xy_printf("[CIS] Fota rtc callback. Ticks:%u", cissys_gettime());

	
	xy_flash_write(FOTA_RESULT_FLASH_BASE, &fota_ret, 4);
	//reboot
	soft_reset_by_wdt(RB_BY_FOTA);

	//cissys_setFwUpdateResult(FOTA_RESULT_SUCCESS);
	//reset_fotaIDIL(context,CIS_EVENT_FIRMWARE_UPDATE_FAILED);
}
//Reset for fota; 
//if failed fota and unregister need reset 
void cissys_resetFota(void)
{
    rcv_size = 0;
	s_cis_fwSize = 0;
	s_cis_fwState = 0;
	s_cis_fwRes = 0;	
}

//Save firmware downloading state
bool cissys_setFwState(uint8_t state)
{
	osTimerAttr_t timer_attr = {0};

	if(s_cis_fwState == state)
	{
		xy_printf("[CIS] fw state:%d, no need to set");
		return 0;
	}


	s_cis_fwState = state;
	xy_printf("[CIS] fw state:%d", state);

	switch (state)
	{
		case  FOTA_STATE_DOWNLOADING:

			g_FOTAing_flag = 1;

			//fota超时处理
            if(cis_fota_timer_overdue != NULL)
            {
                osTimerDelete(cis_fota_timer_overdue);
                cis_fota_timer_overdue = NULL;
            }
			timer_attr.name = "cis_fota_overdue";
            cis_fota_timer_overdue = osTimerNew((osTimerFunc_t)(cissys_rtc_cb), osTimerOnce, NULL, &timer_attr);
            osTimerStart(cis_fota_timer_overdue, CIS_FOTA_OFFSET*1000);

			break;
		case  FOTA_STATE_DOWNLOADED:
			g_FOTAing_flag = 1;
			xy_printf("[CIS]switch to FOTA DOWNLOADED STATE");
			break;
		case  FOTA_STATE_UPDATING:
			g_FOTAing_flag = 1;
			break;
		case  FOTA_STATE_IDIL:
			g_FOTAing_flag = 0;
			break;
		default:
			break;
	}
	
	return 0;
}

#endif
