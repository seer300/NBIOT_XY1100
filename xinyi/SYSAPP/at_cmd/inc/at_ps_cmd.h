#pragma once

/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "lwip/ip_addr.h"
#include "osal.h"
#include "xy_ps_api.h"
#include "xy_net_api.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define BC26_MAX_CID_NUM    11

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
struct dns_server_info { 
    ip4_addr_t dns4;
#if LWIP_IPV6
	ip6_addr_t dns6;
#endif
};

typedef struct
{
	unsigned char imsi[IMSI_LEN];	   //455201033543990
	unsigned char meid[IMEI_LEN];	   //460040772710417
	int      cell_id; //099F6298
} ps_info_t;

//sim card type,see  g_uicc_type
typedef enum
{
	UICC_UNKNOWN = 0,
	UICC_TELECOM, //telecom
	UICC_MOBILE,  //mobile
	UICC_UNICOM,  //unicom
} SIM_card_type_t;

typedef void (*ps_urc_Callback_t)(unsigned long eventId, void *param, int paramLen);

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern char g_OOS_flag;
extern unsigned char *g_ueiccid;
extern short g_ps_pdn_state; //mark actived pdn cid
extern int g_null_udp_rai;
extern unsigned char g_working_cid;
extern int g_check_valid_sim; //^SIMST
extern int g_uicc_type;
extern int8_t g_netifIp_type;
extern osSemaphoreId_t g_out_OOS_sem;

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_AT_req(char *at_buf, char **prsp_cmd);
int at_XYIMEIMSI_info(char *at_buf);
int at_XYRAI_req(char *at_buf, char **prsp_cmd);
int at_NSET_req(char *at_buf, char **prsp_cmd);
int at_CGATT_req(char *at_buf, char **prsp_cmd);
int at_ECHOMODE_req(char *at_buf, char **prsp_cmd);
void ps_urc_register_callback_init();