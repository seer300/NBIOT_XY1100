/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_utils.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "cloud_utils.h"
#include "xy_ps_api.h"
#include "xy_net_api.h"
#include "qspi_flash.h"
#include "low_power.h"
#include "oss_nv.h"
#include "lwip/ip_addr.h"

#if TELECOM_VER
#ifdef CONFIG_FEATURE_FOTA
#include "atiny_fota_state.h"
extern cdp_fota_info_t* g_fota_info;
#endif
#endif

/*******************************************************************************
 *							   Macro definitions							   *
 ******************************************************************************/

/*******************************************************************************
 *							   Type definitions 							   *
 ******************************************************************************/

/*******************************************************************************
 *						  Local function declarations						   *
 ******************************************************************************/

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						Inline function implementations 					   *
 ******************************************************************************/

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
enum lwip_ip_addr_type cloud_get_ip_type(cloud_ip_type_e cloud_type)
{
    if ((g_softap_fac_nv->cloud_ip_type >> 2*cloud_type) & 0x0001)
        return IPADDR_TYPE_V4;
    else
        return IPADDR_TYPE_V6;

}

void cloud_set_ip_type(cloud_ip_type_e cloud_type,enum lwip_ip_addr_type ip_addr_type)
{
    if (ip_addr_type == IPADDR_TYPE_V4)
    {
        g_softap_fac_nv->cloud_ip_type |= (1 << 2*cloud_type );
        g_softap_fac_nv->cloud_ip_type &= ~(1 << 2*cloud_type+1 );
    }
    else
    {
        g_softap_fac_nv->cloud_ip_type |= (1 << (2*cloud_type+1));
        g_softap_fac_nv->cloud_ip_type &= ~(1 << (2*cloud_type));
    }
    SAVE_FAC_PARAM(cloud_ip_type);
}

uint64_t cloud_gettime_ms(void)
{
    return (uint64_t)xy_rtc_get_sec(0)*1000 ;
}

uint32_t cloud_gettime_s(void)
{
    return xy_rtc_get_sec(0);
}


unsigned int cloud_get_ResveredMem()
{
	unsigned int availableMemory = 0;
    unsigned int availableMemory_addr = 0;
#if XY_FOTA
    xy_get_OTA_flash_info(&availableMemory_addr, (int *)&availableMemory);
#endif
	return availableMemory;
}
int cloud_mutex_create(osMutexId_t *pMutexId)
{
    int ret = XY_ERR;
    *pMutexId = osMutexNew(NULL);
    if(*pMutexId != NULL)
        ret = XY_OK;
    return ret;
}

int cloud_mutex_destroy(osMutexId_t *pMutexId)
{
    int ret = XY_ERR;
    if (osMutexDelete(*pMutexId) == osOK)
    {
        *pMutexId = NULL;
        ret = XY_OK;
    }
    return ret;
}

int cloud_mutex_lock(osMutexId_t *pMutexId, uint32_t timeout)
{
    int ret = XY_ERR;
    if (osMutexAcquire(*pMutexId, timeout) == osOK)
        ret = XY_OK;
    return ret;
}

int cloud_mutex_unlock(osMutexId_t *pMutexId)
{
    long ret = XY_ERR;
    if (osMutexRelease(*pMutexId) == osOK)
        ret = XY_OK;
    return ret;
}

#if TELECOM_VER
#ifdef CONFIG_FEATURE_FOTA
void cdp_fota_info_set(upgrade_state_e state)
{
	if(g_fota_info == NULL)
		g_fota_info = (cdp_fota_info_t*)xy_malloc(sizeof(cdp_fota_info_t));

    //Check fota info flag
    if(*(unsigned int*)FOTA_STATEINFO_FLASH_BASE == 0xAA)
        xy_flash_read(FOTA_STATEINFO_FLASH_BASE, g_fota_info, sizeof(cdp_fota_info_t));
    else
	{
    	memset(g_fota_info, 0x00, sizeof(cdp_fota_info_t));
		return;
	}

    g_fota_info->fota_upgrade_info.upgrade_state = state;
    g_fota_info->fota_upgrade_info.crc_flag = calc_crc32(0, &g_fota_info->fota_upgrade_info, sizeof(upgrade_flag_s) - sizeof(uint32_t));
	//此处写flash的目的是为了防止升级完成后用户不重新注册，然后走了重启，这样平台升级状态无法改变
    xy_flash_write(FOTA_STATEINFO_FLASH_BASE, g_fota_info, sizeof(cdp_fota_info_t));
}

#if MG_DFOTA
//MG 20230508
void cdp_fota_info_get(upgrade_state_e *state)
{
	if(g_fota_info == NULL)
		g_fota_info = (cdp_fota_info_t*)xy_malloc(sizeof(cdp_fota_info_t));

    //Check fota info flag
    if(*(unsigned int*)FOTA_STATEINFO_FLASH_BASE == 0xAA)
        xy_flash_read(FOTA_STATEINFO_FLASH_BASE, g_fota_info, sizeof(cdp_fota_info_t));
    else
	{
    	memset(g_fota_info, 0x00, sizeof(cdp_fota_info_t));
		return;
	}

	*state = g_fota_info->fota_upgrade_info.upgrade_state;

}
//MG END
#endif
#endif
#endif

/*公有云FOTA升级结果上报，CDP的FOTA相关信息设置*/
void cloud_fota_init(void)
{
    char *at_str = NULL;
	int ret = ota_get_update_result(); 
    if(ret == XY_OK)
    {
#if TELECOM_VER
#ifdef CONFIG_FEATURE_FOTA
    	cdp_fota_info_set(OTA_SUCCEED);
#endif
#endif
    	at_str = xy_zalloc(64);
        strcat(at_str, "\r\n+FIRMWARE UPDATE SUCCESS\r\n");
        send_rsp_str_to_ext(at_str);
        xy_free(at_str);
        
        xy_fota_state_hook(4);
        ota_clear_update_result();
    }
    else if(ret == XY_ERR)
    {
#if TELECOM_VER
#ifdef CONFIG_FEATURE_FOTA
    	cdp_fota_info_set(OTA_FAILED);
#endif
#endif
    	at_str = xy_zalloc(64);
		strcat(at_str, "\r\n+FIRMWARE UPDATE FAILED\r\n");
		send_rsp_str_to_ext(at_str);
		xy_free(at_str);

		xy_fota_state_hook(5);
		ota_clear_update_result();
    }

}


