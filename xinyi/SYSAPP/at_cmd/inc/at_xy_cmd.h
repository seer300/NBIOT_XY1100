#pragma once

#define MODULE_130  1


#if  MODULE_130

#define MODULE 		"SLM130X"
#define TVERSION 	"T05"
#define SDATE		"S0221"
#define SDKVERSION	"0.2.1.C21"
#define MVERSION	"M003"
#define SI_NUM		HAL_GPIO_PIN_NUM_13
#else

#define MODULE 		"SLM160X"
#define TVERSION 	"T03"
#define SDATE		"S0221"
#define SDKVERSION	"0.2.1.C21"
#define MVERSION	"M003"
#define SI_NUM		HAL_GPIO_PIN_NUM_11
#define LED_NUM		HAL_GPIO_PIN_NUM_10

#endif

typedef struct
{	
    unsigned char    atWakeup;   
}user_nv_data_t;



/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_FORCEDL_req(char *at_buf, char **prsp_cmd);
int at_FORCEDL_info(char *at_buf);
int at_NV_req(char *at_buf, char **prsp_cmd);
int at_TEST_req(char *at_buf, char **prsp_cmd);
int at_FLASH_req(char *at_buf, char **prsp_cmd);
int at_DUMP_req(char *at_buf, char **prsp_cmd);
int at_NUESTATS_req(char *at_buf, char **prsp_cmd);
int at_NPSMR_req(char *at_buf, char **prsp_cmd);
int at_CMVER_req(char *at_buf, char **prsp_cmd);
int at_HVER_req(char *at_buf, char **prsp_cmd);
int at_CGMM_req(char *at_buf, char **prsp_cmd);
int at_CGMI_req(char *at_buf, char **prsp_cmd);
int at_ATI_req(char *at_buf,char **prsp_cmd);
int at_QGMR_req(char *at_buf,char **prsp_cmd);
int at_QVERTIME_req(char *at_buf,char **prsp_cmd);
int at_SGSW_req(char *at_buf,char **prsp_cmd);
int at_QATWAKEUP_req(char *at_buf, char **prsp_cmd);
int at_QCGDEFCONT_req(char *at_buf,char **prsp_cmd);




