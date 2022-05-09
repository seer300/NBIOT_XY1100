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

#ifndef __AGENT_TINY_DEMO_H_
#define __AGENT_TINY_DEMO_H_

#include "agenttiny.h"
#include "atiny_osdep.h"
#include "xy_utils.h"
#include "liblwm2m.h"

#if XY_DTLS
#include "mbedtls/ssl.h"
#include "dtls_interface.h"
#endif

#define DATALIST_MAX_LEN 8
#define DOWNSTREAM_LEN_MAX 1400
#define DOWNSTREAM_BUFFER_TOTAL 2560 //BC260 ���л������洢�ֽ���

#define CDP_NEED_RECOVERY(status) (status == ATINY_DATA_SUBSCRIBLE || status == 2 || status == 5 || status == 6 || status == 7)

void agent_tiny_entry(void);

typedef struct
{
    char ip_addr_str[16];
    unsigned   short port;
} cdp_server_settings_t;

typedef struct buffer_list_s
{
    struct buffer_list_s *next;
    char *data;
    int data_len;
    cdp_msg_type_e type;
	uint8_t seq_num;
} buffer_list_t;

typedef struct
{
    int pending_num;
    int sent_num;
    int error_num;

    buffer_list_t *head;
    buffer_list_t *tail;
} upstream_info_t;

typedef struct
{
    int buffered_num;
    int received_num;
    int dropped_num;
#if VER_QUCTL260
	int buffered_total;
#endif

    buffer_list_t *head;
    buffer_list_t *tail;
} downstream_info_t;

extern int data_send_flag;
extern int data_send_len;
extern int data_rcv_len;
extern uint8_t *prcvbuf;
extern uint8_t *pdatabuf;

int is_cdp_running();
void cdp_lwm2m_init();
void cdp_lwm2m_process(void);
void app_data_report(void);
void app_downdata_recv(void);
void seq_callback(unsigned long eventId, void *param, int paramLen);
void cdp_set_seq_and_rai(uint8_t sec_num, uint8_t raiflag);
void cdp_get_seq_and_rai(uint8_t* raiflag,uint8_t* seq_num);
void cdp_QLWULDATASTATUS_report(uint8_t seq_num);


int app_data_report_check();
void cdp_set_report_event(xy_module_type_t type,const char *arg, int code);
void cdp_con_flag_init(int type, uint16_t seq_num);
void cdp_set_cur_seq_num(uint8_t seq_num);
uint8_t cdp_get_cur_seq_num();
int cdp_get_con_send_flag();
void cdp_con_flag_deint();
void cdp_softap_var_nv_init();
void cdp_downbuffered_save();

#if XY_DTLS
int atiny_dtls_shakehand(mbedtls_ssl_context *ssl, const dtls_shakehand_info_s *info);
#endif



#endif 
