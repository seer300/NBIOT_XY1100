#pragma once

/* �����汾����˵��
** о��SDK�汾˵������<V1100B10012R05C2102P02>Ϊ��˵��
** V1100--о����оƬ�ͺ�XY1100
** B10012--о���ڲ���git��֧
** R05--δ֪
** C21--�ͻ���ţ���һ��SDK����C21Ӧ��MG��XY�ı�ţ�C00���շ��汾������ĳ���ͻ�ר�ð汾
** 02--���߰汾
** P02--���߰汾����

** ���ڴ�MG�汾��������
** MODULE--ģ���ͺ�
** TVERSION--�汾��
** SDATE--����
** SDKVERSION--о��SDK�汾���ֶηֱ�Ϊ���߰汾.���߰汾����.�ͻ����
** MVERSION--�ͻ����ư汾
*/

/* �汾��V1100B10012R05C2102P02
** ʱ�䣺20230116
** �޸���Ϣ��
** [1]MODEMģ�飬�Ż����������EDRXż�ִ�������⣬���������Ż�
** [2]MODEMģ�飬�Ż�������R16������ݴ����Ż�
*/

#define MODULE_130  1


//#define MODULE 		"SLM130X"
#define MODULE 		"SLM130G"
#define TVERSION 	"T01"
#define SDATE		"S0918"
#define SDKVERSION	"0.2.3.C21"  /*20230224 patch03*/
#define MVERSION	"M024"
#define SI_NUM			HAL_GPIO_PIN_NUM_13
#define LED_NUM		HAL_GPIO_PIN_NUM_4


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
int at_MGEDRXRPT_rep(char *at_buf,char **prsp_cmd);
int at_QFOTADL_req(char *at_buf,char **prsp_cmd);
int at_130G_NIDD_req(char *at_buf, char **prsp_cmd);
int at_NIDD_urc_req(char *at_buf, char **prsp_cmd);

