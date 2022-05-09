/***************************************************************************************************************************
 * 芯翼自研的FOTA方案相关API接口，该源文件客户无权修改，只能调用相应的API接口。如果有额外需求，可与芯翼FAE联系！ *
 **************************************************************************************************************************/
#include "xy_utils.h"
#include <qspi_flash.h>
#include "xy_system.h"
#include "xy_fota.h"
#include "dma.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "xy_flash.h"
#include "xy_watchdog.h"
#if TELECOM_VER
#include "ota_crc.h"
#endif
#include "rsa.h"
#include "xy_at_api.h"
#include "sys_init.h"
#include "xy_app_hook.h"

#if XY_FOTA

#define UPROUND(raw, base)    ((((raw)+(base)-1)/(base))*(base))
#define DOWNROUND(raw, base)  (((raw)/(base))*(base))

/**
* @brief macro for fota flag to star
*/
#define FOTA_UPGRADE_NEEDED 0x1
#define FOTA_UPGRADE_BYPASS 0x0

/**
* @brief fota package magic num
*/
#define MAGIC_NUM 3513

/**
* @brief fota package hmac key
*/
#define HMAC_KEY "XY1100"

/**
* @brief fota package data buffer length
*/
#define BUF_LEN 1024

/**
* @brief 差分包中用户可携带数据的长度
*/ 
#define USER_FOTA_DATA_LEN 64

/**
* @brief 差分包中RSA加密SHA校验值后的长度
*/ 
#define DELTA_RSA_SHA_LEN 80

/**
* @brief head struct for fota detla file
*/
typedef struct {
    uint32_t fota_flag;
    uint32_t fota_base_addr;
    uint8_t ming_sha[20];
    uint32_t fota_end_addr;
    uint8_t reserve[16];
    uint32_t fota_info_crc;
} Flash_Secondary_Boot_Fota_Def;

/**
* @brief 差分包数据头部信息结构体,用户不需要关注
*/
typedef struct
{
    uint32_t magic_num;
    uint8_t total_sha[80];
    uint32_t version;
    uint8_t old_image_version_sha[20];	///< old_image_version_sha
    uint8_t new_image_version_sha[20];	///< new_image_version_sha
    uint32_t image_num;
    uint32_t extra_num;
    uint32_t user_delta_info_num;
    uint32_t backup_start_addr;
    uint32_t backup_len;
    uint32_t fota_packet_base;              ///< 差分包存放地址
    uint32_t lzma_model_properties;
    uint32_t lzma_dict_size;
    uint32_t user_data_len;
} delta_head_info_t;

/**
* @brief 差分包存放的首地址和FLASH的大小
*/ 
static uint32_t  g_flash_base;
static uint32_t  g_flash_maxlen;

/**
* @brief 差分包存放FLASH的大小
*/ 
static uint32_t g_recv_size = 0;

/**
* @brief 差分包信息结构体
*/
delta_head_info_t *g_head_info = NULL;

/*******************************************************************************
 *                  FOTA flash信息和FOTA升级周期接口                                    *
 ******************************************************************************/
    
/*获取存放差分包的FLASH的信息：FLASH区域起始地址和大小*/
void xy_get_OTA_flash_info(uint32_t *addr, int *len)
{
    extern uint32_t _Flash_Used;
    extern uint32_t _Ram_Text;
    extern uint32_t _Ram_Data;
    uint32_t FLASH_LEN = (uint32_t)&_Flash_Used;
    uint32_t RAM_LEN = (uint32_t)&_Ram_Text + (uint32_t)&_Ram_Data;

    //根据ARM核的image长度计算FOTA差分包存放区域的首地址,各image长度要4K对齐
    *addr = ARM_FLASH_BASE_ADDR + UPROUND(FLASH_LEN, FLASH_SECTOR_LENGTH) + UPROUND(RAM_LEN, FLASH_SECTOR_LENGTH);
    //根据FLASH分区计算FOTA差分包存放区域的大小
    *len = ARM_FLASH_BASE_ADDR + ARM_FLASH_BASE_LEN - *addr - USER_EXT_FLASH_LEN_MAX;

    return;
}

/*获取预设的FOTA升级周期*/
int xy_get_OTA_period()
{
	return  g_softap_fac_nv->fota_period*24*60*60;
}

/*设置预设的FOTA升级周期*/
void xy_set_OTA_period(int days)
{
	g_softap_fac_nv->fota_period = days;
}

/*检查预设的FOTA升级周期是否超时*/
int xy_check_OTA_timeout()
{
    int cur_sec = 0;

    cur_sec = xy_rtc_get_sec(0);

    //从未做FOTA，需立即执行FOTA检查
    if(g_softap_var_nv->last_FOTA_time == 0)
    	return  XY_OK;

	//每次都执行FOTA升级检查
	else if(g_softap_fac_nv->fota_period == 0)
		return  XY_OK;

	//周期性检查已超时，执行FOTA检查
	else if(cur_sec > xy_get_OTA_period() + g_softap_var_nv->last_FOTA_time)
		return  XY_OK;

	else
		return  XY_ERR;
}

/*记录最新一次FOTA升级检查的时刻点*/
void  update_last_OTA_check_time()
{
	g_softap_var_nv->last_FOTA_time = xy_rtc_get_sec(0);
}

/*******************************************************************************
 *                          FOTA API 接口                                        *
 ******************************************************************************/

/* 检验差分包是否适用于该版本，根据差分包中携带的SHA校验值判断 */
static int ota_version_check(delta_head_info_t *head)
{
    uint8_t temp_sha[SHA1HashSize] = {0};

    memcpy(temp_sha, (const void *)(VERSION_MIMG_SHA), SHA1HashSize);

    //源版本SHA1校验值比较
    if(memcmp(head->old_image_version_sha, temp_sha, SHA1HashSize) != 0)
    {
        xy_rearrange_DWORD(temp_sha, SHA1HashSize);

        if(memcmp(head->old_image_version_sha, temp_sha, SHA1HashSize) != 0)
        {
            softap_printf(USER_LOG, WARN_LOG, "error: image version error");
            return XY_ERR;
        }
    }
       
    return XY_OK;
}


/*对差分包数据进行处理，检查差分包头的完整性，版本检测，最后将差分包数据写入flash*/
static int ota_downlink_packet_proc(char* data, uint32_t size)
{
    uint32_t offset = 0;
    char * input_data =data;
    uint32_t input_len = size;
    
    if(g_recv_size + input_len > g_flash_maxlen)
    {
        softap_printf(USER_LOG, WARN_LOG, "error:Dtle file size too large!");
        return XY_ERR;
    }

    if(g_recv_size + input_len < (sizeof(delta_head_info_t)))
    {
        memcpy((char *)g_head_info + g_recv_size, input_data, input_len);
        g_recv_size += input_len;
        return XY_OK;
    }

    if(g_recv_size < (sizeof(delta_head_info_t)))
    {
        memcpy((char *)g_head_info + g_recv_size, input_data, (sizeof(delta_head_info_t)) - g_recv_size);
        input_data += (sizeof(delta_head_info_t)) - g_recv_size;
        input_len -= (sizeof(delta_head_info_t)) - g_recv_size;
        g_recv_size = (sizeof(delta_head_info_t));

        if(g_head_info->magic_num != MAGIC_NUM)
        {
            softap_printf(USER_LOG, WARN_LOG, "error: magic_num(%d) != %d", g_head_info->magic_num, MAGIC_NUM);
            return XY_ERR;
        }

        if(ota_version_check(g_head_info))
            return XY_ERR;

        /* 差分包过大flash不足 */
        if(g_head_info->fota_packet_base < g_flash_base)
        {
            softap_printf(USER_LOG, WARN_LOG, "error: delta is too big");
            return XY_ERR;
        }

		g_flash_maxlen = g_flash_maxlen - (g_head_info->fota_packet_base - g_flash_base);
        g_flash_base = g_head_info->fota_packet_base;
        softap_printf(USER_LOG, WARN_LOG, "delta_save_addr: %x %x", g_head_info->fota_packet_base, g_flash_maxlen);
    }

    if(g_recv_size == (sizeof(delta_head_info_t)))
    {
        xy_flash_erase(g_flash_base, g_flash_maxlen);
        xy_flash_write_no_erase(g_flash_base, g_head_info, (sizeof(delta_head_info_t)));
        if(input_len > 0)
        {
            xy_flash_write_no_erase(g_flash_base + (sizeof(delta_head_info_t)), input_data, input_len);
        }
        g_recv_size += input_len;
    }
    else
    {
        xy_flash_write_no_erase(g_flash_base + g_recv_size, input_data, input_len);
        g_recv_size += input_len;
    }
    
    return XY_OK;
}

/* 从FLASH中读取差分包所有数据进行SHA校验 */
int ota_delta_check()
{
    bool ret = XY_ERR;
	PUBLIC_KEY_Def * pub_key = NULL;
    uint8_t digest[SHA1HashSize];
	uint8_t digest_decrypt[SHA1HashSize];
    uint8_t *buf= NULL;
    int  read_size = 0;
    delta_head_info_t *head_info;

    if(g_flash_base == 0 || g_recv_size == 0)
    {
    	 softap_printf(USER_LOG, WARN_LOG, "[OTA]:check no recv data\n");
		 return XY_ERR;
    }

    buf = (uint8_t *)xy_zalloc(1024);
    head_info = (delta_head_info_t *)xy_zalloc(sizeof(delta_head_info_t));

    //初始化HMAC校验结构体
    HMACContext *hmac_context = (HMACContext *)xy_zalloc(sizeof(HMACContext));
    hmacReset(hmac_context, SHA1, (const uint8_t*)HMAC_KEY, strlen(HMAC_KEY));
    xy_flash_read(g_flash_base + read_size, head_info, sizeof(delta_head_info_t));
    
    //差分包前84个字节不作为校验内容
    hmacInput(hmac_context, (const uint8_t *)&(head_info->version), sizeof(delta_head_info_t) - DELTA_RSA_SHA_LEN - 4);
    read_size += sizeof(delta_head_info_t);
    
    while((read_size + 1024) < g_recv_size)
    {
        memset(buf, 0x00, 1024);
        xy_flash_read(g_flash_base + read_size, buf, 1024);
        read_size += 1024;
        if(hmacInput(hmac_context, (const uint8_t *)buf, 1024))
            goto error;
    }

    memset(buf, 0x00, 1024);
    xy_flash_read(g_flash_base + read_size, buf, g_recv_size - read_size);
    if(hmacInput(hmac_context, (const uint8_t *)(buf), (uint32_t)(g_recv_size - read_size)))
        goto error;

    hmacResult(hmac_context, digest);

    //将差分包中加密的校验值解密
    pub_key = (PUBLIC_KEY_Def *)(PUBLIC_KEY);
    rsa_public_decrypt(head_info->total_sha, DELTA_RSA_SHA_LEN, pub_key, (char *)digest_decrypt);

    //计算的校验值与差分包中的校验值进行比较
    if(memcmp(digest, digest_decrypt, SHA1HashSize) == 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "[OTA]: FW validate success");
        //handle user_date
        xy_fota_private_data_hook(head_info->fota_packet_base + sizeof(delta_head_info_t), head_info->user_data_len);
        ret = XY_OK;
    }
    else
    {
        softap_printf(USER_LOG, WARN_LOG, "[OTA]: FW validate failed");
        goto error;
    }

    return ret;

error: 
    g_flash_base =0;
    g_flash_maxlen = 0;
    g_recv_size = 0;
    
    if(g_head_info != NULL)
    {
        xy_free(g_head_info);
        g_head_info = NULL;
    }
    softap_printf(USER_LOG, WARN_LOG, "[ota_ram_deinit]xy_fota_deInit");
    return XY_ERR;
}

/* 将获取到的差分包数据写入到FLASH中 */
int ota_write_to_flash(uint32_t recved_size, char* data, uint32_t size)
{
    //开始升级前需初始化全局变量
    if(recved_size == 0)
    {
        g_flash_base =0;
        g_flash_maxlen = 0;
        g_recv_size = 0;
        
        if(g_head_info != NULL)
        {
            xy_free(g_head_info);
            g_head_info = NULL;
        }
        softap_printf(USER_LOG, WARN_LOG, "[ota_ram_init]xy_fota_Init");
    }

	//开始升级或者断电续升，需执行init操作
	if(g_recv_size == 0)
    {   
        //下载出错或续升后，已下载数据没有包含全部的差分包信息，需重新开始下载
        if(recved_size >0 && recved_size < sizeof(delta_head_info_t))
        {
            softap_printf(USER_LOG, WARN_LOG, "[ota_ram_deinit]please restart fota!");
            return XY_Err_NotAllowed;
        }
            
        //获取差分区域的起始地址和长度
        xy_get_OTA_flash_info(&(g_flash_base), &(g_flash_maxlen));

        //初始化已接收数据长度
        g_recv_size += recved_size;
        g_head_info = (delta_head_info_t *)xy_zalloc(sizeof(delta_head_info_t));


        //断电续升后需从flash中获取差分包头部信息
        if(g_recv_size >= sizeof(delta_head_info_t))
            xy_flash_read(g_flash_base, g_head_info, sizeof(delta_head_info_t));
        
        softap_printf(USER_LOG, WARN_LOG, "[ota_ram_deinit]xy_fota_Init");
    }

    if(ota_downlink_packet_proc(data, size))
        goto error;
    
    return XY_OK;
    
error: 
    g_flash_base =0;
    g_flash_maxlen = 0;
    g_recv_size = 0;
    
    if(g_head_info != NULL)
    {
        xy_free(g_head_info);
        g_head_info = NULL;
    }
    softap_printf(USER_LOG, WARN_LOG, "[ota_ram_deinit]xy_fota_deInit");
    return XY_ERR;
}

/* 设置二级BOOT的FOTA标志位和相关信息，并重启后开始由二级boot升级 */
void ota_update_start()
{
    Flash_Secondary_Boot_Fota_Def fota_def;
    
    memcpy(&fota_def, (const void *)(FOTA_UPDATE_INFO_ADDR), sizeof(Flash_Secondary_Boot_Fota_Def));

    //设置二级boot相关信息并保存到flash中
    fota_def.fota_flag = FOTA_UPGRADE_NEEDED;
    fota_def.fota_base_addr = g_flash_base;
    fota_def.fota_end_addr = g_flash_base + g_flash_maxlen;
    
    fota_def.fota_info_crc = (unsigned short)xy_chksum((void *)&fota_def, sizeof(Flash_Secondary_Boot_Fota_Def) - 4);

    xy_flash_write(FOTA_UPDATE_INFO_ADDR, &fota_def, sizeof(Flash_Secondary_Boot_Fota_Def));

    osDelay(200);
    
	//重启升级
	soft_reset_by_wdt(RB_BY_FOTA);
}


/* 二级boot中升级完成后，执行重启，进入大版本后，调用该接口查询最终的升级结果*/
int ota_get_update_result()
{
    if(*(uint32_t*)FOTA_RESULT_FLASH_BASE == 1)
    {
        return XY_OK; 
    }
    else if(*(uint32_t*)FOTA_RESULT_FLASH_BASE == 2)
    {
        return XY_ERR;
    }

    return XY_Err_NotAllowed;
}

/* 清除保存在flash中的升级结果*/
void ota_clear_update_result()
{
    xy_flash_erase(FOTA_RESULT_FLASH_BASE, FOTA_RESULT_FLASH_MAXLEN);
}

#else
int ota_delta_check(){return 1;}
int ota_write_to_flash(uint32_t recved_size, char* data, uint32_t size){return 1;}
void ota_update_start(){return;}    
int ota_get_update_result(){return 1;}
void ota_clear_update_result(){return;} 
#endif


