#ifndef _FACTORY_NV_H_
#define _FACTORY_NV_H_
#include  <stdint.h>

#define FAC_NV_LOAD_ADDR 0x27003000
#define FAC_NV_EXEC_ADDR 0x27003000

#define G_LRRC_SUPPORT_BAND_MAX_NUM      19
#define D_NVM_MAX_SIZE_IMEI              9
#define D_NVM_MAX_SIZE_IMEI_SV           10

#define NVM_MAX_ADD_SPECTRUM_EMISSION_MAX_NUM        5

#define NVM_MAX_DCN_ID_NUM                 32 
#define NVM_MAX_CANDIDATE_FREQ_NUM         32
#define NVM_MAX_OPERATOR_NUM               3 
#define NVM_MAX_OPERATOR_PLMN_NUM          10
#define NVM_MAX_EPLMN_NUM                  16

#define NVM_MAX_PRE_EARFCN_NUM             G_LRRC_SUPPORT_BAND_MAX_NUM
#define NVM_MAX_PRE_BAND_NUM               5
#define NVM_MAX_LOCK_FREQ_NUM              20
#define NVM_MAX_SN_LEN                     64

#define NAS_IMSI_LTH   9
#define G_LRRC_PRE_EARFCN_INVALID           1111111


typedef struct
{
    unsigned char    ucProfile0x0002SupportFlg;
    unsigned char    ucProfile0x0003SupportFlg;
    unsigned char    ucProfile0x0004SupportFlg;
    unsigned char    ucProfile0x0006SupportFlg;
    unsigned char    ucProfile0x0102SupportFlg;
    unsigned char    ucProfile0x0103SupportFlg;
    unsigned char    ucProfile0x0104SupportFlg;
}NVM_ROHC_PROFILE_STRU;

typedef enum
{
    LRRC_MAX_ROHC_SESSION_CS2 = 0, 
    LRRC_MAX_ROHC_SESSION_CS4,
    LRRC_MAX_ROHC_SESSION_CS8,
    LRRC_MAX_ROHC_SESSION_CS12
}LRRC_MAX_ROHC_SESSION_VALUE;

typedef struct
{    
    unsigned char     ucSuppBand;
	unsigned char     ucPowerClassNB20dBmR13Flg;
    unsigned char     ucAddSpectrumEmissionNum;
    unsigned char     aucAddSpectrumEmission[NVM_MAX_ADD_SPECTRUM_EMISSION_MAX_NUM];
}LRRC_SUPPORT_BAND_STRU;

typedef struct
{
    unsigned char     ucSuppBand;
    unsigned char     ucEnable;        /* 0:not Valid 1:Valid */
    unsigned short    usOffset;        /*EndFreq = ulStartFreq + usOffset*/
    unsigned long     ulStartFreq;
}LRRC_PRE_BAND_STRU;

typedef struct
{
    unsigned char    ucApn[22];
    unsigned char    auPlmnList[NVM_MAX_OPERATOR_PLMN_NUM][3];      /* Byte Order: MCC2 MCC1 | MNC3 MCC3 | MNC2 MNC1 */
                                                                    //china mobile: 64F000,64F020,64F070,64F080,64F040,64F031,000000,000000,000000,000000
                                                                    //china unicom: 64F010,64F060,64F090,000000,000000,000000,000000,000000,000000,000000
                                                                    //china telecom:64F030,64F050,64F011,000000,000000,000000,000000,000000,000000,000000
    unsigned long    aulPreEarfcn[NVM_MAX_PRE_EARFCN_NUM];
    LRRC_PRE_BAND_STRU    aPreBand[NVM_MAX_PRE_BAND_NUM];
}LNB_NAS_OPERATOR_CFG_DATA_STUR;

typedef struct {
    unsigned char    aucImei[D_NVM_MAX_SIZE_IMEI]; /* IMEI */               //##8,74,85,2,1,51,69,147,9##&&
    unsigned char    ucReserved1;                                           //##0##
    unsigned char    aucImeisv[D_NVM_MAX_SIZE_IMEI_SV];/* IMEI(SV) */       //##9,67,85,2,1,51,69,147,121,240##&&
}T_ImeiInfo;

typedef struct
{
#define     D_AUTH_PROT_NONE            0
#define     D_AUTH_PROT_PAP             1
#define     D_AUTH_PROT_CHAP            2
    unsigned short                      ucUseFlg:1;                        //##0##
    unsigned short                      ucUsernameLen:5;                   //##0##
    unsigned short                      ucPasswordLen:5;                   //##0##
    unsigned short                      ucAuthProt:3;                      //##0##
    unsigned short                      ucPadding:2;                       //##0##
#define     D_PCO_AUTH_MAX_LEN          16 
    unsigned char                       aucUsername[D_PCO_AUTH_MAX_LEN];   //##0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##
    unsigned char                       aucPassword[D_PCO_AUTH_MAX_LEN];   //##0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##
}T_PCO_AUTH_INFO_STRU;

typedef struct
{
    unsigned char    ucUsed:1;                                              //##0##
    unsigned char    ucPdpType:3;                                           //##0##
    unsigned char    ucAtCid:4;                                             //##0##

    unsigned char    ucH_comp:1;                                            //##0##
    unsigned char    ucSecurePco:1;                                         //##0##
    unsigned char    ucPadding6:6;
    unsigned char    ucPadding;

    unsigned char    ucNSLPI:1;                                             //##0##
    unsigned char    ucP_cid:1;                                             //##0##, cid index
    unsigned char    ucApnLen:6;                                            //##0##
    
#define FACTORY_USER_SET_APN_LEN    38
    unsigned char    ucApn[FACTORY_USER_SET_APN_LEN];                       //##0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##
    T_PCO_AUTH_INFO_STRU tAuthInfo; //40 byte
}LNB_NAS_USER_SET_CID_INFO;

typedef struct
{
    unsigned char    ucAcsStratumRelType:4;                                 //##0##
	unsigned char    ucMultiToneSupportFlg:1;                               //##1##
    unsigned char    ucMultiDRBsupportFlg:1;                                //##1##
	unsigned char    ucMaxROHCContextSessions:2;                            //##0##

    unsigned char    ucDataInactMonR14SupFlg:1;                            //##0##
    unsigned char    ucRaiSupptR14SupFlg:1;                                //##0##
    unsigned char    ucMulCarrNprachR14SupFlg:1;                           //##0##
    unsigned char    ucTwoHarqProR14SupFlg:1;                              //##0##
    unsigned char    ucPwrClassNb14dBmR14SupFlg:1;                         //##0##
    unsigned char    ucMulCarrPagSupportFlg:1;                             //##0##
	unsigned char    ucMultiCarrierSupportFlg:1;                           //##0##
	unsigned char    ucMultiNSPmaxSupportFlg:1;                            //##0##
	
    unsigned char    ucLRrcPdcpParamFlg:1;                                 //##1##
	unsigned char    ucProfile0x0002SupportFlg:1;                          //##0##
	unsigned char    ucProfile0x0003SupportFlg:1;                          //##0##
	unsigned char    ucProfile0x0004SupportFlg:1;                          //##0##
	unsigned char    ucProfile0x0006SupportFlg:1;                          //##0##
	unsigned char    ucProfile0x0102SupportFlg:1;                          //##0##
	unsigned char    ucProfile0x0103SupportFlg:1;                          //##0##
	unsigned char    ucProfile0x0104SupportFlg:1;                          //##0##
	
    unsigned char    ucUeCategory:2;                                       //##0##
	unsigned char    ucInterferenceRandomisationR14:1;                     //##0##
	unsigned char    ucEarlyContentionResolutionR14:1;                     //##0##

    unsigned char    ucInterFreqRSTDmeasFlg:1;                             //##1##
    unsigned char    ucAddNeighbCellInfoListFlg:1;                         //##1##
    unsigned char    ucPrsIdFlg:1;                                         //##1##
    unsigned char    ucTpSeparationViaMutingFlg:1;                         //##0##

	
	unsigned char    ucSupportBandNum;                                     //##3##

    unsigned char    ucAddPrsCfgFlg:1;                                     //##0##
    unsigned char    ucPrsBasedTbsFlg:1;                                   //##1##
    unsigned char    ucAddPathReportFlg:1;                                 //##1##
    unsigned char    ucDensePrsCfgFlg:1;                                   //##0##  
    unsigned char    ucPrsOccGroupFlg:1;                                   //##0##
    unsigned char    ucPrsFreqHoppingFlg:1;                                //##0##
    unsigned char    ucPeriodicalReportingFlg:1;                           //##1##
    unsigned char    ucMultiPrbNprsFlg:1;                                  //##1##

    unsigned char    ucIdleStateForMeasFlg:1;                              //##1##
    unsigned char    ucMaxSupptPrsBandwidthFlg:1;                          //##0##
    unsigned char    ucMaxSupptPrsBandwidth:3;                             //##0##
    unsigned char    ucMaxSupptPrsCfgFlg:1;                                //##0##
    unsigned char    ucMaxSupptPrsCfg:2;                                   //##0##1/2    

    unsigned char    ucCrtdcpRptIndFlg:1;                                  //##0##
    unsigned char    ucPadding2:7;                                         //##0##
	LRRC_SUPPORT_BAND_STRU    aSupportBandInfo[G_LRRC_SUPPORT_BAND_MAX_NUM];
}T_UeCapa;

typedef struct
{
    unsigned char    ucATLockEarfcnFlg;
    unsigned char    ucATLockCellIDFlg;
    unsigned short   usATLockCellID;

    unsigned long    ulATLockEarfcn;
}T_AtLockEarfcnCellInfo;
typedef struct
{
    unsigned char    ucMsgService:1;
    unsigned char    ucCmmsN:2;
    unsigned char    ucScaLen:4;
    unsigned char    ucPadding:1;
    unsigned char    ucToSca;
#define              D_SMS_SCA_SIZE_MAX          12 
    unsigned char    aucSca[D_SMS_SCA_SIZE_MAX];
}T_SMS_CFG_INFO;

typedef struct
{
	unsigned char    ucNfCapabilitySupportFlg:1;                           //##0##
	unsigned char    ucS1UDataSupportFlg:1;                                //##1##
	unsigned char    ucERwOpdnSupportFlg:1;                                //##0##
	unsigned char    ucExtPeriodicTimersSupportFlg:1;                      //##1##
	unsigned char    ucEdrxSupportFlg:1;                                   //##1##
	unsigned char    ucPsmSupportFlg:1;                                    //##1##
	unsigned char    ucAUTV:1;                                             //##1##
	unsigned char    ucCpBackOffSupportFlg:1;                              //##0##

	unsigned char    ucRestrictECSupportFlg:1;                             //##0##
	unsigned char    uc3gppsDataOffSupportFlg:1;                           //##0##
	unsigned char    ucRdsSupportFlg:1;                                    //##0##
	unsigned char    ucApnRateControlSupportFlg:1;                         //##1##
	unsigned char    ucPsmEnableFlg:1;                                     //##1##&&
    unsigned char    ucEdrxEnableFlg:1;                                    //##0##&&
	unsigned char    ucAutoConnectFlg:1;                                   //##1##&&
	unsigned char    ucL2THPFlag:1;                                        //##0##

	unsigned char    ucActTimeBakFlg:1;                                    //##0##
	unsigned char    ucEdrxsValBakFlg:1;                                   //##0##
	unsigned char    ucTraceLevel:1;                                       //##0##
	unsigned char    ucPowerTest:1;                                        //##0##
	unsigned char    aucBandScan:2;                                        //##0##
	unsigned char    ucFreqScanType:2;                                     //##0##

	unsigned char    ucPreferCiotOpt:2;                                    //##1##
	unsigned char    ucDataInactTimer:5;                                   //##0##
    unsigned char    ucPSRptInfoFlg:1;                                     //##0##

	unsigned char    ucReqPeriodiTauValue;                                 //##72##&&
	unsigned char    ucReqActTimeValue;                                    //##33##&&
	unsigned char    ucReqEdrxValue;                                       //##2##&&
	unsigned char    ucSinrOffset;                                         //##0##

	unsigned char    ucPowOffSleep:1;                                      //##0##
	unsigned char    ucUpCIoTSupportFlg:1;                                 //##0##
	unsigned char    ucSuppBandNum:6;                                      //##2##&&
	unsigned char    aucSupportBandList[G_LRRC_SUPPORT_BAND_MAX_NUM];      //##5,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##&&

	LNB_NAS_OPERATOR_CFG_DATA_STUR      aOperatorCfgData[NVM_MAX_OPERATOR_NUM]; //0:china mobile,1:china unicom,2:china telecom
    LNB_NAS_USER_SET_CID_INFO tCidInfo0; //cid index = 0
    unsigned short      usOosTimerLen;                                     //##60##&&
    unsigned char    ucSecAlg;
	unsigned char    ucPsTestTauLen;                                       //##0##

    unsigned long    aulLockFreqList[NVM_MAX_LOCK_FREQ_NUM];          //##1200,1201,1575,1949,2400,2401,2525,2649,3450,3451,3625,3799,0,0,0,0,0,0,0,0##

	unsigned char    ucLockFreqNum;                                   //##12##
    unsigned char    ucUpReqFlag:1;//1                                                    //##0##&&
    unsigned char    ucCellReselForbitFlg:1;  /* 0: can cell reselection; 1: can not  */  //##0##
    unsigned char    ucUpRptFlag:1; //####&& /*0: Up Capa not support for Pulbic Net;  1: Up Report depend on Nv*/
    unsigned char    ucPsSpeedFlag:1; /*0:    1: update vrr=vrh when appear SnGap*/ 
    unsigned char    ucPsmReqFlag:1;
    unsigned char    ucWakeupInitUsimFlg:1;
    unsigned char    ucCciotoptN:2;                                        //##0##
    unsigned char    ucLockFreqListStat:1;
    unsigned char    ucNoSimSlpTimeLen:7;                                 //##127##&&
    unsigned char    ucPTWValue;                                          //##3##&&

    unsigned char    ucUlRateCtrlBufSize;
    unsigned char    ucERegMode:3;                                         //##0##&&
    unsigned char    ucNipInfoN:1;                                         //##0##
    unsigned char    ucCgerepMode:1;                                       //##1##
    unsigned char    ucCrtdcpRepValue:1;                                   //##0##
    unsigned char    ucEDrxMode:2;                                         //##0##

    unsigned char    ucCtzrReport:2;                                       //##3##  改为##0##    add 20230214
    unsigned char    ucStorageOperType:4;                                  //##0##
    unsigned char    ucPadding6:2;                                         //##1##

    unsigned char    ucNgactrN:1;                                          //##0##
    unsigned char    ucNptwEDrxMode:2;                                     //##0##
    unsigned char    ucNotSinglBand:1;                                     //##0##
    unsigned char    ucSkipPreEarfcnFlg:1;                                 //##0##
    unsigned char    ucCidInfoForMtNetFlg:1;                               //##0##
    unsigned char    ucCellSrchInd:1;                                      //##0##
    unsigned char    ucPadding4:1;                                         //####&&

    LNB_NAS_USER_SET_CID_INFO tCidInfo1; //cid index = 1

    T_AtLockEarfcnCellInfo tAtLockEarfcnCell;

    unsigned char    ucReqPeriodiTauValue_Bak;                             //##0##
    unsigned char    ucReqActTimeValue_Bak;                                //##0##
    unsigned char    ucReqEdrxValue_Bak;                                   //##0##
    unsigned char    ucPTWValue_Bak;                                       //##0##

    unsigned short   usSyncTimePeriod;                                     //##0##
    unsigned short   usBarringReleaseDelay:11;                             //##0##
    unsigned char    ucEpcoSupportFlg:1;                                   //##1##
    unsigned char    ucNonIpNoSmsFlg:1;                                    //##0##
    unsigned char    ucT3412ExtChgReportFlg:1;                             //##1##
    unsigned char    ucMtNetTestCloseFlg:1;                                //##0##
    unsigned char    ucAtHeaderSpaceFlg:1;                                 //##0##
    unsigned char    ucDiscLen;                                            //##0##

    unsigned char    ucCciotBakFlg:1;                                      //##0##
    unsigned char    ucUpCIoTSupportFlg_Bak:1;                             //##0##
    unsigned char    ucPreferCiotOpt_Bak:2;                                //##0##
    unsigned char    ucUiccDebugFlg:1;                                     //##0##
    unsigned char    ucNrnpdmRepValue:1;                                   //##0##
    unsigned char    ucIpv4DnsReqForbFlg:1;                                //##0##
    unsigned char    ucIpv6DnsReqForbFlg:1;                                //##0##
    unsigned char    ucCnecN;                                              //##0##

    unsigned char    ucCsconN:2;                                           //##0##
    unsigned char    ucNotStartPsFlg:1;                                    //##0##
    unsigned char    ucIpv6AddressFormat:1;                                //##0##
    unsigned char    ucIpv6SubnetNotation:1;                               //##0##
    unsigned char    ucIpv6LeadingZeros:1;                                 //##0##
    unsigned char    ucIpv6CompressZeros:1;                                //##0##
    unsigned char    ucPadding7:1;                                         //##0##

    
    T_SMS_CFG_INFO   tSmsCfgInfo;
    unsigned char  aucPadding3[562];/* in all :1536 */
}T_NasNvInfo;

typedef struct
{
    int  magic_num;       //erase flash magic num,such as 5A5A5A5A
    T_UeCapa            tUeCapa;
    T_NasNvInfo         tNasNv;
}T_PsNvInfo;

enum Peri_Remap_Type{
	REMAP_SWD = 0,
 	REMAP_CSP2,
	REMAP_UART,
	REMAP_CSP3,
	REMAP_I2C1,
	REMAP_SPI,
	REMAP_TMR1,
	REMAP_JTAG,
	REMAP_LED,
	REMAP_RFSWITCH,
	REMAP_SIM,
	
	REMAP_PERI_MAX
};

typedef struct
{
	unsigned short txDcQ_LP;
	unsigned short txDcI_LP;
	unsigned short txDcQ_HP;
	unsigned short txDcI_HP;
}rf_mt_dc_t;

typedef struct
{
	rf_mt_dc_t 	txDC_LB;
	rf_mt_dc_t 	txDC_HB;
	unsigned short		txIqAmp_LB;
	unsigned short		txIqAmp_HB;
}rf_mt_dc_iq_t;

typedef struct
{
	rf_mt_dc_iq_t 	txDcIqNv_LLT;
	rf_mt_dc_iq_t 	txDcIqNv_HLT;
	rf_mt_dc_iq_t 	txDcIqNv_LMT;
	rf_mt_dc_iq_t 	txDcIqNv_HMT;
	rf_mt_dc_iq_t 	txDcIqNv_LHT;
	rf_mt_dc_iq_t 	txDcIqNv_HHT;
	
	unsigned char	txDcIqNvFlag_LLT;
	unsigned char	txDcIqNvFlag_HLT;
	unsigned char	txDcIqNvFlag_LMT;
	unsigned char	txDcIqNvFlag_HMT;
	unsigned char	txDcIqNvFlag_LHT;
	unsigned char	txDcIqNvFlag_HHT;
	unsigned char	padding[2];
}rf_dciq_t;

//size is 560 bytes,user can use 1800 bytes in tail 
typedef struct {
    char  deepsleep_enable;    //##1##&& 是否允许芯片进入深睡开关，一旦关闭，释放锁后也不会进深睡
    char  work_mode;           //##0##&& 芯片启动模式，0表示双核启动，1表示仅M3核启动；推荐使用0，若用户需要使用1，必须与FAE确认
    char  ver_type;            //##2## 软件版本功能分支，位图表示；bit 0:入网入库版本; bit 1:标准SDK版本;bit 2:金卡定制SDK版本;bit 3:中移物联定制SDK版本;bit 4:对标移远SDK版本
    char  set_tau_rtc;         //##1##&& 深睡时是否设置TAU定时器
 	 
    char  lpm_standby_enable;        //##1## standby睡眠开关
    char  wfi_enable;                //##1## wfi睡眠开关
    char  offtime;                   //##0##&& unused
    uint8_t at_standby_delay;       //##0##&& 用于外部输入at命令时设置开standby的软timer时间,单位秒

	char  g_NPSMR_enable;           //##0##&& NPSMR上报开关，若打开，则深睡时上报+NPSMR:
	char  pin_reset;                //##0##&& PIN reset是否擦除工作态NV，包括易变NV、云NV等
	char  keep_retension_mem;       //##0##&& 4K的retension内存深睡是否断电开关，为1时，深睡保持供电；为0时，深睡断电，需要回写易变NV到flash中
	uint8_t deepsleep_delay;		//##30##&& 非RTC唤醒或有外部AT命令输入，系统会延迟若干秒后自动进入深睡；除非用户主动申请锁

	uint8_t ext_lock_disable;		 //##0## 外部输入AT命令请求自动上锁的开关，若设为1，表示不自动上锁，则当deepsleep_delay超时后会自动进入深睡
	uint8_t demotest;				 //##0## 内部使用
	char  down_data;                 //##1##&& 是否跳过T3324直接进入深睡；为1时，会正常执行T3324空闲态任务；0则表示跳过T3324直接进入PSM态
	char  off_debug;                 //##0##&& 软件debug开关；为0时，debug调试模式，软件断言不会自动重启；为1时，release模式，软件断言会触发自动重启
	
	char  save_cloud;                //##0## 深睡保存云状态机到flash中的开关，若设置为1，唤醒后可以快速云恢复，无需执行云配置等信息
	char  keep_cloud_alive;          //##0## 保持云在线的开关，若打开，则lifetime超时，需要唤醒系统进行云update
	char  fota_period;               //##0## 天为单位，用于终端自行触发的FOTA升级检查，公有云用户无需使用
	char  fota_mode;				 //##0##&& 公有云的FOTA开关，默认值为0，表示打开云FOTA

    char  watchdog_enable;           //##1##&& 看门狗启动开关
	char  hard_dog_mode;             //##0##&& unused. 
	uint8_t  hard_dog_time;          //##0##&& unused. 
	uint8_t  user_dog_time;          //##0##&& 软看门狗时长，单位为分钟，系统上电后开启，超时后调用user_dog_timeout_hook回调接口，具体实现用户完成

	
    char  cdp_register_mode;        //##0## CDP注册模式;0:由用户通过AT命令或API进行手工注册;1:系统上电后自动注册CDP，2：禁止注册
	char  g_NITZ;                   //##1##&& 世界时间的设置模式;1：由3gpp上报的+CTZEU:更新;0：依靠用户输入AT+CCLK=<yy/MM/dd,hh:mm:ss>[<+-zz>]来更新
	char  open_log;    		        //##1##&& log的输出模式;0,无log;1,输出所有log;2,仅输出M3核log;3,仅输出用户通过xy_printf打印的log
	char  sim_vcc_ctrl;             //##2## SIM卡电压自适应配置位图；bit 0-2(0,always 3V;1,always 1.8V;2,first 1.8V;3,first 3V) bit 3(retry interval between 3v and 1v8,0:20s,1:1s) bit 4-6(current result,2 is 3V;4 is 1.8V)
	
	char  deepsleep_urc;            //##1##  Deep Sleep 事件 URC 上报
	char  high_freq_data;           //##0##  unused
	char  g_drop_up_dbg;            //##0##  unused
	char  g_drop_down_dbg;          //##0##  unused

	char  test;                     //##0## 测试专用，bit 0,xy_zalloc too big will assert;
    uint8_t data_format;            //##0## 业务收发数据格式，0-收发文本;1-发十六进制收文本；2-发文本收十六进制；3-收发十六进制
    uint16_t cloud_ip_type;         //##0##每两位表示一个云的ipv6 ipv4使能情况，由低到高依次为cdp、onenet、socket、ctwing

	char  close_urc;                //##0## 主动上报URC的上报开关，1为关闭所有的主动上报
	char  ring_enable;              //##0## 专用于通过URC进行外部MCU的唤醒，普通用户无需关注
	uint16_t ra_timeout;			//##0## 等待ipv6 ra报文超时时间,取值[0-65535]
	
	char  close_rai;                //##0## 释放锁时RAI触发开关；0：释放锁时平台后台自动触发RAI的发送;1：释放锁时不内部自动触发RAI，而是由用户在完成远程通信时，通过调用AT+XYRAI主动触发RAI的释放
	char  open_rtc_urc;             //##0## 用户RTC唤醒时，URC主动上报的开关；默认值0表示不发送URC
	char  min_mah;                  //##0##  unused
	char  min_mVbat;                //##0##  unused
	
	uint16_t    backup_threshold;   //##0##  unused
	char  open_tau_urc;				//##0##  3GPP相关超时事件唤醒后，上报URC的开关；1表示发送URC
	char  padding14[5];             //##0## 
	
	uint8_t  log_length_limit;      //##0## M3核log输出的单条最大长度，2的幂次方
	char  padding7;
	uint16_t  uart_rate; 		   //##4##&& AT串口波特率设置,单位粒度为2400bit;其中，低9位为设置的波特率；高7位为波特率自适应的结果；二者只能一种有效
	
	uint8_t dump_mem_into_flash;   //##0##&& 软件死机时保存现场到flash的开关
	uint8_t  aon_uvlo_en;          //##0##&& unused
	uint8_t hclk_div;              //##10## Can be 4, 5, 6, 7, 8, 10, 12
	uint8_t pclk_div;              //##1##  Can be 1, 2, 3, 4
	
	char  deepsleep_threshold;      //##0## threshold time of DRX/eDRX into deepsleep,seconds
	char  padding23;
	char  xtal_32k;                 //##0## RTC时钟模式；0：外置32K晶体供时钟;1：用芯片内部的RC振荡器供时钟，但必须关闭所有睡眠
	char  ps_utcwkup_dis;		    // unused

	uint32_t  rc32k_freq;		   //##0## 0,10 *rc32k frequence

	char  log_fifo_limit;			//##0##&& log队列的深度，默认缓存50条log消息
	char  close_at_uart;            //##0##  AT的uart串口的开关.1表示关闭AT通道，进而对应的PIN脚可以由客户重新配置做它用   
	char  padding8;                 //##0##&&
	char  resetctl;                 //##0##&&RESET PIN脚脉冲持续时长所表示的模式；0：20ms内为唤醒中断，超过20ms则为PIN_RESET;1,6s内为唤醒中断，超过6s则为PIN_RESET;

	char  log_size;					//##0##&& M3核为DSP核的log分配的内存缓存，0表示默认20K，单位粒度为1024字节
	char  cmee_mode;				//##1## AT命令ERR错误上报模式； 0:\r\nERROR\r\n  1:\r\nCME ERROR:错误码\r\n  2:\r\nCME ERROR:ERROR_STRING\r\n
	char  dsp_ext_pool;             //##20##&& M3核为DSP核分配的额外堆内存空间，单位粒度为1024字节
	uint8_t  binding_mode;          //##0## CDP专用的lwm2m绑定模式；1:UDP mode  2:UDP queue mode
	
	uint32_t cdp_lifetime;          //##0## CDP的lifetime时长，单位秒；取值为0-30*86400,0:禁用生命周期，不会发送更新注册包；1-30*86400 开启生命周期
	uint8_t obs_autoack;			//##0## onenet config参数：是否启用模块自动响应订阅请求
    uint8_t write_format;			//##0## onenet config参数：接收写操作数据的输出模式,0 十六进制字符串显示,1 字符串显示
    uint8_t ack_timeout;			//##0## onenet config参数：应答超时重传时间
	char  padding4[5];

	uint32_t dm_reg_time;          //##2592000## 联通DM的注册周期,默认值为2592000 sec(30 days)
	
 	uint16_t peri_remap_enable;    //##1931##&& 外设使能位图，与下面每个功能PIN脚紧藕；default: 0x078B		for each bit,0:disable 1:enable    92
 									//bit0:SWD(M3)	bit1:CSP2(AT UART); bit2:UART(USER UART); bit3:CSP3(LOG UART); 
									//	 bit4:I2C1 		bit5:SPI; 		 bit6:TMR1(PWM); 	  bit7:JTAG(DSP); 	 
									//   bit8:LED;   bit9:RF switch      bit10:SIM
	uint8_t swd_swclk_pin;          //##2##&&  ARM swd for JLINK
	uint8_t swd_swdio_pin;          //##3##&&  ARM swd for JLINK
	
	uint8_t csp2_txd_pin;           //##15##&&  used for AT        96
	uint8_t csp2_rxd_pin;           //##16##&&  used for AT 
	uint8_t uart_txd_pin;           //##255##&&
	uint8_t uart_rxd_pin;           //##255##&&
	
	uint8_t csp3_txd_pin;          //##14##&&   used for log      100
	uint8_t csp3_rxd_pin;          //##17##&&   used for log 
	uint8_t i2c1_sclk_pin;         //##255##&&
	uint8_t i2c1_sda_pin;          //##255##&&

	uint8_t spi_sclk_pin;         //##255##&&                    104
	uint8_t spi_mosi_pin;         //##255##&&
	uint8_t spi_miso_pin;         //##255##&&
	uint8_t spi_ss_n_pin;         //##255##&&

	uint8_t tmr1_pwm_outp_pin;    //##255##&&               108
	uint8_t tmr1_pwm_outn_pin;    //##255##&&
	uint8_t jtag_jtdi_pin;        //##18##&&  dsp JTAG
	uint8_t jtag_jtms_pin;        //##19##&&  dsp JTAG
	
	uint8_t jtag_jtdo_pin;       //##20##&&  dsp JTAG                   112
	uint8_t jtag_jtck_pin;       //##21##&&  dsp JTAG
	uint8_t led_pin;             //##5##&&  used for debug DSP,0XFF is close LED debug

	uint8_t rf_switch_v0_pin;    //##0## 
	uint8_t rf_switch_v1_pin;    //##1## 
	uint8_t rf_switch_v2_pin;    //##4## 

	uint8_t sim_clk_pin;        //##22##&&
	uint8_t sim_rst_pin;        //##23##&&
	uint8_t sim_data_pin;       //##24##&&  
	char  padding9[3];

	uint32_t dm_inteval_time;   //##0## 中移DM的注册周期,default 1440 minutes(24 hours)  


	uint8_t pmu_ioldo_sel;      //##0## 修改引脚电压         wsl
	uint8_t cdp_dtls_switch;    //##0## CDP使用DTLS的开关;1,open
	uint8_t cdp_dtls_nat_type;  //##0## unused
	uint8_t cdp_dtls_max_timeout;//##0## unused
	
    uint8_t need_start_dm;     //##0## 公有云DM开关；若打开，会造成功耗和流量的大大增加
    uint8_t dm_retry_num;      //##5## 电信和联通DM自注册失败的重试次数
    uint16_t dm_retry_time;    //##3600## 电信和联通DM自注册失败后间隔若干时间后再进行重启，缺省值为3600 sec
    
    uint16_t cloud_server_port;  //##0## CDP和onenet的云服务器port端口
    uint8_t regver;              //##2## unused
    uint8_t onenet_config_len;   //##0## onenet默认配置的长度，即onenet_config_hex有效长度

	uint8_t cloud_server_auth[16]; //##""##&& CDP或onenet的秘钥                                                    
    uint8_t product_ver[28];    //##""##&& tail char must is '\0'                                                          
    uint8_t modul_ver[20];     //##"XYS-XYM110"##&& module version,tail char must is '\0'                                                  
    uint8_t hardver[20];      //##"XYM110_HW_V1.0"##&& hardware version,tail char must is '\0'                                             
    uint8_t versionExt[28];     //##"XYM110_SW_V0.3.7"##&& external version,tail char must is '\0'                                       
    uint32_t ref_pridns6[4];   //##"240c::6666"字节形式## ipv6默认主dns服务器地址。以字节形式保存
    uint32_t ref_secdns6[4];   //##"240c::6644"字节形式## ipv6默认辅dns服务器地址。以字节形式保存
    uint8_t cdp_pskid[16];     //##""##&& pskid for cdp 用来存储15位pskid，cdp dtls 设置psk时用到                                                 
    uint8_t onenet_config_hex[120]; //##""##&& onenet默认配置                                               
    uint8_t dm_app_key[64];   //##"M100000329"## onenet dm appkey                                                                                                                               
    uint8_t dm_app_pwd[64];   //##"43648As94o1K8Otya74T2719D51cmy58"## onenet dm password for dm appkey                                           
	
    uint8_t cdp_dfota_type;   //##0## DFOTA 升级模式,0自动升级,1受控升级                                                                                                                                                
    uint8_t tx_low_power_flag;//##0##&&
    uint8_t tx_multi_tone_power_flag;	//##1##&& RF Tx 15kHz@12Tone reduce 1dB 
    char  tx_3tone_power_flag;  //##0##&& RF TX 3Tone reduce 1dB

    uint8_t rf_switch_mode_txl;//##5##&& GPIO4:1 GPIO1:0 GPIO0:1                      552
    uint8_t rf_switch_mode_rxl;//##7##&& GPIO4:1 GPIO1:1 GPIO0:1
    uint8_t rf_switch_mode_txh;//##6##&& GPIO4:1 GPIO1:1 GPIO0:0
    uint8_t rf_switch_mode_rxh;//##4##&& GPIO4:1 GPIO1:0 GPIO0:0

	uint8_t rfWaittingTime;//##122##&&                                                       556
	uint8_t xtal_type;//##0##&& TCXO:0   XO:1	
	uint8_t rfInsertData;//##5##
	uint8_t dm_serv_flag;

	uint32_t  xtal_freq;//##38400000##&&                                               560
	uint32_t  bb_threshold_1Tone;//##18000##&&
	uint32_t  bb_threshold_3Tone;//##18000##&&
	uint32_t  bb_threshold_6Tone;//##18000##&&
	uint32_t  bb_threshold_12Tone;//##18000##&&

	uint8_t   cloud_server_ip[40];  //##""##&& CDP云服务器IP地址或者域名
	
    uint32_t ref_pridns4;   //##1920103026("114.114.114.114"字节形式)## ipv4默认主dns服务器地址。以字节形式保存
    uint32_t ref_secdns4;   //##134744072("8.8.8.8"字节形式)## ipv4默认辅dns服务器地址。以字节形式保存
    
    uint8_t   reserved[126];		//##0 128-2## 
    uint8_t   mgedrxrpt_ind; //20230404 MG add for control EDRX mode "ENTER/EXIT DEEPSLEEP" URC
    uint8_t   epco_ind;    //20230217 MG add for:AT+QCFG="epco",0/1
}softap_fac_nv_t;

//use  2332 bytes,all is 4080 bytes
typedef struct {
	T_PsNvInfo    tNvData; /*1536 Byte,must cache aligned*/
	rf_dciq_t     rf_dciq_nv;/*128 Byte,must cache aligned*/
	softap_fac_nv_t  softap_fac_nv;   //668 BYTES,begin from 0X680,platform factory nv
	
}factory_nv_t;

typedef struct
{
	T_ImeiInfo			tImei;
	unsigned char	 	ucSnLen;
	unsigned char       padding[3];
	unsigned char	 	aucSN[NVM_MAX_SN_LEN]; 
	unsigned int 		chksum;
	//unsigned int 		rfITflag;
}T_PsImeiSnInfo;

//extern T_PsImeiSnInfo 	gPsImeiSnInfo;
extern softap_fac_nv_t *g_softap_fac_nv;

#define POWER_CNT 68

typedef struct
{
	unsigned int txCaliFreq;
	char	txCaliTempera;
	char	txCaliVolt;	
	char 	reserved[2];
	short	bandPower[POWER_CNT];	
}rf_tx_calibration_t;

typedef struct
{
	unsigned int txFlatFreq;
	char	txCaliTempera;
	char	txCaliVolt;	
	char 	reserved[2];
	short	txFlatLPower;     	
	short	txFlatMPower;		
	short	txFlatHPower;		
	short   padding;
}rf_tx_flatten_t;

typedef struct
{
	unsigned int 	version;
	unsigned int 	nvFlag0;
	int 			freqErr;
	rf_tx_calibration_t txCaliLB;
	rf_tx_calibration_t txCaliHB;
	rf_tx_flatten_t		txFlatLB[12];   
	rf_tx_flatten_t		txFlatHB[12];	
	unsigned short 	txFactorLB;
	unsigned short 	txFactorHB;
	unsigned short 	txFactorLT;
	unsigned short 	txFactorHT;
	unsigned short 	txFactorVolt[8];
	unsigned int 	reserved[4];
	unsigned int 	nvFlag1;
	unsigned int	tSensorValue;
}rf_mt_nv_t;

typedef enum
{
	RF_CALI_NV = 0,
	RF_PSIMEI_SN  =0x400,
	RF_GOLDMACHINE_NV = 0x800,
	RF_STATE_RFIT = 0xA00,
	RF_UPDATE =0xFFF,
}rf_info_type;

typedef struct
{
	unsigned int msg_len;
	unsigned int msg_addr;
}rf_info_t;

#endif
