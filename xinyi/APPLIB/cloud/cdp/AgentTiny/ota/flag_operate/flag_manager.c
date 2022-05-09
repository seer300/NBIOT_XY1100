/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/
#include "softap_macro.h"
#include "xy_utils.h"
#if TELECOM_VER

#include <stddef.h>
#include <string.h>
#include "flag_manager.h"
#include "atiny_fota_state.h"

#define FLASH_UNIT_SIZE (256)
#define FLASH_FLAG_SIZE (512)

static flag_op_s g_flag_op;
cdp_fota_info_t* g_fota_info = NULL;  //fota data info

int flag_init(flag_op_s *flag)
{
    if (NULL == flag
        || NULL == flag->func_flag_read
        || NULL == flag->func_flag_write)
    return -1;
    if(g_fota_info == NULL)
    {
        g_fota_info = xy_zalloc(sizeof(cdp_fota_info_t));
        if(*(unsigned int*)FOTA_STATEINFO_FLASH_BASE == 0xAA)
            xy_flash_read(FOTA_STATEINFO_FLASH_BASE, g_fota_info, sizeof(cdp_fota_info_t));
    }

    g_flag_op.func_flag_read = flag->func_flag_read;
    g_flag_op.func_flag_write = flag->func_flag_write;

    return 0;
}

int flag_read(flag_type_e flag_type, void *buf, int32_t len)
{
	//Read flash data 
    switch (flag_type)
    {
    case FLAG_BOOTLOADER:
		if(g_fota_info->used == 0XAA)
        	memcpy(buf, &(g_fota_info->fota_upgrade_info), len);
        break;
    case FLAG_APP:
		if(g_fota_info->used == 0XAA)
        	memcpy(buf, &(g_fota_info->fota_observer_info), len);
        break;
    default:
        return -1;
    }

    return 0;
}

int flag_write(flag_type_e flag_type, const void *buf, int32_t len)
{
	//Copy data to flash
    switch (flag_type)
    {
    case FLAG_BOOTLOADER:
		g_fota_info->used = 0XAA;
        memcpy(&(g_fota_info->fota_upgrade_info), buf, len);
        return 0;
    case FLAG_APP:
		g_fota_info->used = 0XAA;
        memcpy(&(g_fota_info->fota_observer_info), buf, len);
		xy_printf("[CDP] take fota observer !!!!");
        break;
    default:
        break;
    }
    return g_flag_op.func_flag_write(buf, len);
}

#endif