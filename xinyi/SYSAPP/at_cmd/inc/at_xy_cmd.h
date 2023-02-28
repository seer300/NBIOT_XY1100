#pragma once

/* 软件版本规则说明
** 芯翼SDK版本说明，以<V1100B10012R05C2102P02>为例说明
** V1100--芯翼的芯片型号XY1100
** B10012--芯翼内部的git分支
** R05--未知
** C21--客户编号，上一个SDK中是C21应该MG于XY的编号，C00是普发版本，不是某个客户专用版本
** 02--基线版本
** P02--基线版本补丁

** 基于此MG版本命名如下
** MODULE--模组型号
** TVERSION--版本号
** SDATE--日期
** SDKVERSION--芯翼SDK版本，字段分别为基线版本.基线版本补丁.客户编号
** MVERSION--客户定制版本
*/

/* 版本：V1100B10012R05C2102P02
** 时间：20230116
** 修改信息：
** [1]MODEM模块，优化调整，针对EDRX偶现大电流问题，测量抑制优化
** [2]MODEM模块，优化调整，R16后向兼容处理优化
*/

#define MODULE_130  0

#if  MODULE_130

#define MODULE 		"SLM130X"
#define TVERSION 	"T06"
#define SDATE		"S0228"
#define SDKVERSION	"0.2.3.C21"  /*20230224 patch03*/
#define MVERSION	"M003"
#define SI_NUM		HAL_GPIO_PIN_NUM_13

#else

#define MODULE 		"SLM160X"
#define TVERSION 	"T06"
#define SDATE		"S0228"
#define SDKVERSION	"0.2.3.C21"
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




