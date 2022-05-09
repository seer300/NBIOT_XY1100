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
#if TELECOM_VER

#include "xy_utils.h"
#include "ota_port.h"
#include <string.h>
#include <stdlib.h>
#include <liblwm2m.h>
#include "atiny_context.h"
#include "firmware_update.h"
#include "internals.h"
#include "atiny_fota_state.h"
#include "xy_fota.h"


extern cdp_fota_info_t* g_fota_info;  //fota data info
uint32_t cdp_recv_size = 0;

static int hal_check_flash_param(ota_flash_type_e type, int32_t len, uint32_t location)
{
	(void) type;
	(void) len;
	(void) location;

	//T.B.D
	return 0;
}

static int hal_read_flash(ota_flash_type_e type, void *buf, int32_t len, uint32_t location)
{
	(void) buf;

    if (hal_check_flash_param(type, len, location) != 0)
    {
        return -1;
    }

	//T.B.D
    return 0;
}

//hal adapter for writing flash
static int hal_write_flash(ota_flash_type_e type, const void *buf, uint32_t len, uint32_t location)
{
    //handle_data_t *handle = NULL;

    if (hal_check_flash_param(type, len, location) != 0)
    {
        return -1;
    }
	//保存fota下载之前平台下发5/0/3等observe的一些状态信息
    if(OTA_UPDATE_INFO == type)
    {
        xy_flash_write(FOTA_STATEINFO_FLASH_BASE, g_fota_info, sizeof(cdp_fota_info_t));
    }
    else
    {        
    	//proc and copy pkt data to flash
        if(ota_write_to_flash(cdp_recv_size, buf, len))
            return -1;
        cdp_recv_size += len;
    }

    return 0;
    
}

void hal_init_ota(void)
{
    cdp_recv_size = 0;
    return;
}


void hal_get_ota_opt(ota_opt_s *opt)
{
    if (opt == NULL)
    {
        return;
    }

    memset(opt, 0, sizeof(*opt));
    opt->read_flash = hal_read_flash;
    opt->write_flash = hal_write_flash;
    opt->flash_block_size = 0x200;
}

#endif

