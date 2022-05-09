/** 
* @file        xy_fota.h
* @brief       芯翼自研的FOTA算法的用户二次开发接口，主要为差分包的下载和校验相关
* @attention   芯翼自研的FOTA方案仅供参考
*/
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sha.h"

/****************************************************************************************************************************************
***************************************下面接口用于终端本地通过定时器触发周期性FOTA升级检测**************************************************
****************************************************************************************************************************************/
/**
 * @brief 获取本地FOTA升级检测的周期时长，单位为秒
 * @note  该接口仅用于本地主动检查FOTA是否升级的场景，如通过http；\n
 *** 对于公有云，如onenet、ctwing等，无需使用该接口，而是由云端主动推送。
 */
int  xy_get_OTA_period();

/**
 * @brief 设置本地FOTA升级检测的周期时长，单位为天
 * @note  该接口仅用于本地主动检查FOTA是否升级的场景，如通过http；\n
 *** 对于公有云，如onenet、ctwing等，无需使用该接口，而是由云端主动推送。
 */
void  xy_set_OTA_period(int days);

/**
 * @brief 检查本地FOTA升级是否已经超时，若超时，则应该与服务器进行链接，尝试FOTA升级
 * @return  bool,see @ref xy_ret_Status_t. XY_OK表示已超时，需要与服务器联系，尝试执行FOTA
 *
 * @note  该接口仅用于本地主动检查FOTA是否升级的场景，如通过http；\n
 *** 对于公有云，如onenet、ctwing等，无需使用该接口，而是由云端主动推送。
 * @warning  建议在确定TCPIP网路已通情况下调用该接口，否则会报XY_Err_NoConnected错误。
 */
int  xy_check_OTA_timeout();

/**
 * @brief 记录最新一次FOTA升级检查的时刻点，以便下次系统上电时检查是否需要FOTA升级检查
 * @note   该接口在与FOTA服务器通信完成后调用，不能在任务初始化时调用，以防止由于无线环境等原因未与FOTA服务器通信而错误更新时刻点
 * @warning  仅用于终端本地触发的FOTA升级检查，且用户必须调用，否则会造成每次上电后都会连接服务器进行FOTA是否升级的检查
 */
void  update_last_OTA_check_time();


/****************************************************************************************************************************************
***************************************下面接口用于差分包的下载与保存，并触发升级********************************************************
****************************************************************************************************************************************/
/**
 * @brief 当获取到差分包数据后，检验差分包版本信息并写入flash中
 * @param recved_size 为该函数处理过的数据总大小
 * @param data 差分包数据
 * @param data 差分包数据大小
 * @return BOOL,see  @ref  xy_ret_Status_t.
 * @note  断电续升时，需自行保存recved_size。
 */
int ota_write_to_flash(uint32_t recved_size, char* data, uint32_t size);

/**
 * @brief 从flash中读取所有差分包，进行SHA校验
 * @return BOOL,see  @ref  xy_ret_Status_t。
 * @warning 需要先使用ota_write_to_flash将差分包数据保存到flash中，
 * * * * * 然后使用该接口从flash中读取出来计算并SHA校验。
 */
int ota_delta_check();

/**
 * @brief 更新二级boot的升级信息，重启开始差分升级 
 * @note  芯翼的差分升级是在二级boot中执行，调用该接口后会自行进行重启升级。
 */
void ota_update_start();

/**
 * @brief  二级boot中升级完成后，执行重启，进入大版本后，调用该接口查询最终的升级结果
 * @return BOOL,see  @ref  xy_ret_Status_t.
 * @note  该接口指示升级最终结果,可供用户上报云端升级结果
 */
int ota_get_update_result();

/**
 * @brief  清除flash中保存的升级结果
 * @note  使用ota_get_update_result获取升级结果后，必须该接口清理升级结果数据
 */
void ota_clear_update_result();



