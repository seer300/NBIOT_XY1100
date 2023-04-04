/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_cmd.h"
#include "at_tcpip_cmd.h"
#include "at_hardware_cmd.h"
#include "at_ps_cmd.h"
#include "at_system_cmd.h"
#include "at_xy_cmd.h"
#include "at_netdog.h"
#include "at_worklock.h"
#if	BLE_ENABLE
#include "ble_ctl.h"
#endif

#if SPECIAL_USER1
#include "special_user_cmd.h"
#include "xy_log_flash.h"
#endif

#if MOBILE_VER
#include "at_onenet.h"
#include "at_xy_lwm2m.h"
#endif

#if ANDLINK_VER
#include "at_andlink.h"
#endif

#if TELECOM_VER
#include "at_cdp.h"
#endif

#if VER_QUCTL260 && TELECOM_VER
#include "at_cdp_bc26.h"
#endif


#if CTWING_VER
#include "at_ctwing.h"
#endif

#if XY_PING
#include "at_ping.h"
#endif

#if XY_PERF
#include "at_perf.h"
#endif

#if XY_WIRESHARK
#include "at_wireshark.h"
#endif

#if	AT_SOCKET
#include "at_socket_context.h"
#include "at_socket.h"
#endif

#if AT_SOCKET&&VER_QUCTL260
#include "at_socket_bc26.h"
#endif

#if MQTT
#include "at_xy_mqtt.h"	
#endif

#if LIBCOAP
#include "at_coap.h"
#endif

#if HTTP
#include "at_http.h"
#endif

/*******************************************************************************
 *						  Global function declarations 		                   *
 ******************************************************************************/
/* used for demo command,some demo command file have no header,so use extern instead*/
#if XY_FOTA
extern int at_NFWUPD_req(char *at_buf, char **prsp_cmd);
#endif
#if XY_DM
extern int at_XYDMP_req(char *at_buf, char **prsp_cmd);
extern int at_QSREGENABLE_req(char *at_buf, char **prsp_cmd);

#endif
#if MQTT
extern int at_MQNEW_req(char *at_buf, char **prsp_cmd);
extern int at_MQCON_req(char *at_buf, char **prsp_cmd);
extern int at_MQDISCON_req(char *at_buf, char **prsp_cmd);
extern int at_MQSUB_req(char *at_buf, char **prsp_cmd);
extern int at_MQUNSUB_req(char *at_buf, char **prsp_cmd);
extern int at_MQPUB_req(char *at_buf, char **prsp_cmd);
extern int at_MQSTATE_req(char *at_buf, char **prsp_cmd);

#endif

#if SPECIAL_USER1
extern int at_XYCONFIG_req(char *at_buf, char **prsp_cmd);
extern int at_XYSEND_req(char *at_buf, char **prsp_cmd);
extern int at_XYRECV_req(char *at_buf, char **prsp_cmd);
#endif

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
/*just for at cmd have no declaration */
struct at_serv_proc_e at_basic_req[] = {
	{"AT+NITZ", at_NITZ_req},
#if VER_GXX
	{"AT+CTZU", at_CTZU_req},
#endif
	{"AT+QSCLK", at_QSCLK_req},
	{"AT+MGSLEEP", at_MGSLEEP_req},
	{"AT+CFUN", at_CFUN_req},/*20230311 MG*/
	{"AT+QRST", at_QRST_req},
	{"AT+QCFG", at_QCFG_req},
	{"AT+CCLK", at_CCLK_req},
	{"AT+FOTACTR", at_FOTACTR_req},
	{"AT+FORCEDL", at_FORCEDL_req},
	{"AT+RESET", at_RESET_req},
	{"AT+NV", at_NV_req},
	{"AT+TEST", at_TEST_req},
	{"AT+XYCNNT", at_XYCNNT_req},
	{"AT+ZADC", at_ZADC_req},
	{"AT+VBAT", at_VBAT_req},
	{"AT+CBC", at_CBC_req},	
	{"AT+QADC", at_QADC_req}, 	
#if (DSP_DYNAMIC_ON_OFF == 1)
	{"AT+CPOFF", at_CPOFF_req},
#endif
#if	BLE_ENABLE
	{"AT+XYBLE", at_XYBLE_req},
#endif		
#if XY_FOTA
	{"AT+NFWUPD", at_NFWUPD_req},
#endif
	{"AT+RESETCTL", at_RESETCTL_req},
	{"AT+STANDBY", at_STANDBY_req},
	{"AT+OFFTIME", at_OFFTIME_req},
	{"AT+DUMP", at_DUMP_req},
	{"AT+NETDOG", at_NETDOG_req},
	{"AT+TCPIPTEST", at_TCPIPTEST_req},
#if XY_WIRESHARK
	{"AT+WIRESHARK", at_WIRESHARK_req},
#endif
	{"AT", at_AT_req},
//	{"AT+LOADDSP", at_LOADDSP_req},
	{"ATE", at_ECHOMODE_req},
	{"AT+WORKLOCK", at_WORKLOCK_req},
	{"AT+UARTSET", at_UARTSET_req},
	{"AT+IPR", at_IPR_req},
//	{"AT+WTD", at_WTD_req},		//已废弃
	{"AT+QCHIPINFO", at_QCHIPINFO_req},
//	{"AT+WTD", at_WTD_req},		//�ѷ���
	{"AT+FLASH", at_FLASH_req},
	{"AT+CMDNS", at_CMDNS_req},
	{"AT+CMNTP", at_CMNTP_req},
	{"AT+NRB", at_RB_req},
	{"AT+CMRB", at_RB_req},
	{"AT+RB", at_RB_req},
	{"AT+CGMR", at_CMVER_req},
	{"AT+SWVER", at_CMVER_req},
	{"AT+HVER", at_HVER_req},
	{"AT+CGMM", at_CGMM_req},
	{"AT+CGMI", at_CGMI_req},
	{"ATI", at_ATI_req},
	{"AT+QGMR", at_QGMR_req},
	{"AT+QVERTIME", at_QVERTIME_req},
	{"AT+NSET", at_NSET_req},
	{"AT+CGATT", at_CGATT_req},
#if WAKEUP_MCU_BY_URC
	{"AT+CMSRI", at_CMSRI_req},
#endif
	{"AT+XNTP", at_XNTP_req},
	{"AT+NUESTATS", at_NUESTATS_req},
	{"AT+MEMSTATS", at_NUESTATS_req},
	{"AT+NATSPEED", at_NATSPEED_req},
	{"AT+NPSMR", at_NPSMR_req},
	{"AT+CPOF", at_CPOF_req},
	{"AT+FASTOFF", at_CPOF_req},
	{"AT+QPOWD", at_QPOWD_req},
	{"AT+ABOFF", at_ABOFF_req},
	{"AT+XYRAI", at_XYRAI_req},
	{"AT+XDNSCFG", at_XDNSCFG_req},
	{"AT+XDNS", at_XDNS_req},
	{"AT+QDNS", at_QDNS_req},
	{"AT+QIDNSCFG", at_QIDNSCFG_req},
#if VER_QUCTL260
	{"AT+QNTP", at_QNTP_req},
	{"AT+QIDNSGIP", at_QIDNSGIP_req},
    {"AT+QICFG", at_QICFG_req},
    {"AT+QIOPEN", at_QIOPEN_req},
    {"AT+QICLOSE", at_QICLOSE_req},
    {"AT+QISEND", at_QISEND_req},
    {"AT+QISTATE", at_QISTATE_req},
#endif //VER_QUCTL260
#if XY_PING
	{"AT+NPING", at_NPING_req},
	{"AT+NPING6", at_NPING6_req},
	{"AT+NPINGSTOP", at_NPINGSTOP_req},
#endif
#if XY_PERF
	{"AT+XYPERF", at_XYPERF_req},
#endif
#if	AT_SOCKET
	{"AT+SEQUENCE", at_SEQUENCE_req},
	{"AT+NQSOS", at_NQSOS_req},
	{"AT+NSOCR", at_NSOCR_req},
	{"AT+NSOST", at_NSOST_req},
	{"AT+NSOSTF", at_NSOSTF_req},
	{"AT+NSORF", at_NSORF_req},
	{"AT+NSOCL", at_NSOCL_req},
	{"AT+NSONMI", at_NSONMI_req},
	{"AT+NSOCO", at_NSOCO_req},
	{"AT+NSOSD", at_NSOSD_req},
	{"AT+NSOSDEX", at_NSOSDEX_req},
	{"AT+NSOSTEX", at_NSOSTEX_req},
	{"AT+NSOSTFEX", at_NSOSTFEX_req},
	{"AT+NSOSTATUS", at_NSOSTATUS_req},
#endif
#if XY_DM
	{"AT+XYDMP", at_XYDMP_req},
	{"AT+QSREGENABLE", at_QSREGENABLE_req},
#endif
#if MQTT
	{"AT+MQNEW", at_MQNEW_req},
	{"AT+MQCON", at_MQCON_req},
	{"AT+MQDISCON", at_MQDISCON_req},
	{"AT+MQSUB", at_MQSUB_req},
	{"AT+MQUNSUB", at_MQUNSUB_req},
	{"AT+MQPUB", at_MQPUB_req},
	{"AT+MQSTATE", at_MQSTATE_req},
	{"AT+QMTCFG", at_QMTCFG_req},
	{"AT+QMTOPEN", at_QMTOPEN_req},
	{"AT+QMTCLOSE", at_QMTCLOSE_req},
	{"AT+QMTCONN", at_QMTCONN_req},
	{"AT+QMTDISC", at_QMTDISC_req},
	{"AT+QMTSUB", at_QMTSUB_req},
	{"AT+QMTUNS", at_QMTUNS_req},
	{"AT+QMTPUB", at_QMTPUB_req},
#endif
#if TENCENT_VER
	{"AT+TCDEVINFOSET", at_TCDEVINFOSET_req},
	{"AT+TCMQTTCONN", at_TCMQTTCONN_req},
	{"AT+TCMQTTDISCONN", at_TCMQTTDISCONN_req},
	{"AT+TCMQTTPUB", at_TCMQTTPUB_req},
	{"AT+TCMQTTPUBL", at_TCMQTTPUBL_req},
	{"AT+TCMQTTPUBRAW", at_TCMQTTPUBRAW_req},
	{"AT+TCMQTTSUB", at_TCMQTTSUB_req},
	{"AT+TCMQTTUNSUB", at_TCMQTTUNSUB_req},
	{"AT+TCMQTTSTATE", at_TCMQTTSTATE_req},  
#endif
#if LIBCOAP
	{"AT+COAPCREATE", at_COAPCREATE_req},
	{"AT+COAPDEL", at_COAPDEL_req},
	{"AT+COAPHEAD", at_COAPHEAD_req},
	{"AT+COAPOPTION", at_COAPOPTION_req},
	{"AT+COAPSEND", at_COAPSEND_req},
	{"AT+QCOAPSEND", at_QCOAPSEND_req},
	{"AT+QCOAPDATASTATUS", at_QCOAPDATASTATUS_req},
	{"AT+QCOAPADDRES", at_QCOAPADDRES_req},
	{"AT+QCOAPCFG", at_QCOAPCFG_req},
	{"AT+QCOAPCREATE", at_QCOAPCREATE_req},
	{"AT+QCOAPHEAD", at_QCOAPHEAD_req},
	{"AT+QCOAPOPTION", at_COAPOPTION_req},
	{"AT+QCOAPDEL", at_COAPDEL_req},
#endif
#if MOBILE_VER
	{"AT+MIPLCONFIG", at_proc_miplconfig_req},
	{"AT+MIPLCREATE", at_onenet_create_req},
	{"AT+MIPLDELETE", at_proc_mipldel_req},
	{"AT+MIPLOPEN", at_proc_miplopen_req},
	{"AT+MIPLADDOBJ", at_proc_mipladdobj_req},
	{"AT+MIPLDELOBJ", at_proc_mipldelobj_req},
	{"AT+MIPLCLOSE", at_proc_miplclose_req},
	{"AT+MIPLNOTIFY", at_proc_miplnotify_req},
	{"AT+MIPLREADRSP", at_proc_miplread_req},
	{"AT+MIPLWRITERSP", at_proc_miplwrite_req},
	{"AT+MIPLEXECUTERSP", at_proc_miplexecute_req},
	{"AT+MIPLOBSERVERSP", at_proc_miplobserve_req},
	{"AT+MIPLVER", at_proc_miplver_req},
	{"AT+MIPLDISCOVERRSP", at_proc_discover_req},
	{"AT+MIPLPARAMETERRSP", at_proc_setparam_req},
	{"AT+MIPLUPDATE", at_proc_update_req},
	{"AT+ONETRMLFT", at_proc_rmlft_req},

#if LWM2M_COMMON_VER
	{"AT+QLACONFIG", at_proc_qlaconfig_req},
	{"AT+QLACFG", at_proc_qlacfg_req},
	{"AT+QLAREG", at_proc_qlareg_req},
	{"AT+QLAUPDATE", at_proc_qlaupdate_req},
	{"AT+QLADEREG", at_proc_qladereg_req},
	{"AT+QLAADDOBJ", at_proc_qlaaddobj_req},
	{"AT+QLADELOBJ", at_proc_qladelobj_req},
	{"AT+QLARDRSP", at_proc_qlardrsp_req},
	{"AT+QLAWRRSP", at_proc_qlawrrsp_req},
	{"AT+QLAEXERSP", at_proc_qlaexersp_req},
	{"AT+QLAOBSRSP", at_proc_qlaobsrsp_req},
	{"AT+QLANOTIFY", at_proc_qlanotify_req},
	{"AT+QLASENDDATA", at_proc_qlasend_req},
	{"AT+QLARD", at_proc_qlard_req},
	{"AT+QLASTATUS", at_proc_qlstatus_req},
	{"AT+QLARECOVER", at_proc_qlarecover_req},
#endif
#endif
#if ANDLINK_VER
    {"AT+MALCREATE", at_proc_alcreate_req},
    {"AT+MALDELETE", at_proc_aldel_req},
    {"AT+MALCFG", at_proc_alconf_req},
    {"AT+MALOPEN", at_proc_alopen_req},
    {"AT+MALCLOSE", at_proc_alclose_req},
    {"AT+MALNOTIFY", at_proc_alnotify_req},
    {"AT+MALREADRSP", at_proc_alread_req},
    {"AT+MALWRITERSP", at_proc_alwrite_req},
    {"AT+MALUPDATE", at_proc_alupdate_req},
#endif
#if TELECOM_VER
	{"AT+NCDP", at_NCDP_req},
	{"AT+NMGS", at_NMGS_req},
	{"AT+NMGSEXT", at_NMGS_EXT_req},
	{"AT+NMGR", at_NMGR_req},
	{"AT+NNMI", at_NNMI_req},
	{"AT+NSMI", at_NSMI_req},
	{"AT+NQMGS", at_NQMGS_req},
	{"AT+NQMGR", at_NQMGR_req},
	{"AT+QLWSREGIND", at_QLWSREGIND_req},
	{"AT+QREGSWT", at_QREGSWT_req},
	{"AT+CDPUPDATE", at_CDPUPDATE_req},
	{"AT+NMSTATUS", at_NMSTATUS_req},
	{"AT+QLWULDATA", at_QLWULDATA_req},
	{"AT+QLWULDATAEX", at_QLWULDATAEX_req},
	{"AT+QLWULDATASTATUS", at_QLWULDATASTATUS_req},
	{"AT+CDPRMLFT", at_CDPRMLFT_req},
	{"AT+QREGSWT", at_QREGSWT_req},
	{"AT+QSECSWT", at_QSECSWT_req},
	{"AT+QSETPSK", at_QSETPSK_req},
	{"AT+QRESETDTLS", at_QRESETDTLS_req},
	{"AT+QDTLSSTAT", at_QDTLSSTAT_req},
	{"AT+QCFG", at_QCFG_req},
	{"AT+QLWFOTAIND", at_QLWFOTAIND_req},
	{"AT+QLWEVTIND", at_QLWEVTIND_req},
	{"AT+QCRITICALDATA", at_QCRITICALDATA_req},
#if VER_QUCTL260
	{"AT+NCDPOPEN", at_NCDPOPEN_req},
	{"AT+NCDPCLOSE", at_NCDPCLOSE_req},
	{"AT+NCFG", at_NCFG_req},
#endif
#endif

#if CTWING_VER
    {"AT+CTLWVER", at_CTLWVER_req},
    {"AT+CTLWGETSRVFRMDNS", at_CTLWGETSRVFRMDNS_req},
    {"AT+CTLWSETSERVER", at_CTLWSETSERVER_req},
    {"AT+CTLWSETLT", at_CTLWSETLT_req},
    {"AT+CTLWSETPSK", at_CTLWSETPSK_req},
    {"AT+CTLWSETAUTH", at_CTLWSETAUTH_req},
    {"AT+CTLWSETPCRYPT", at_CTLWSETPCRYPT_req},
    {"AT+CTLWSETMOD", at_CTLWSETMOD_req},
    {"AT+CTLWREG", at_CTLWREG_req},
    {"AT+CTLWUPDATE", at_CTLWUPDATE_req},
    {"AT+CTLWDTLSHS", at_CTLWDTLSHS_req},
    {"AT+CTLWDEREG", at_CTLWDEREG_req},
    {"AT+CTLWGETSTATUS", at_CTLWGETSTATUS_req},
    {"AT+CTLWCFGRST", at_CTLWCFGRST_req},
    {"AT+CTLWSESDATA", at_CTLWSESDATA_req},
    {"AT+CTLWSEND", at_CTLWSEND_req},
    {"AT+CTLWRECV", at_CTLWRECV_req},
    {"AT+CTLWGETRECVDATA", at_CTLWGETRECVDATA_req},
#endif

#if SPECIAL_USER1
    {"AT+ICTEMP",at_ICTEMP_req},
	{"AT+RAND", at_RAND_req},
	{"AT+CREATEKEY", at_CREATEKEY_req},
	{"AT+LOGW", at_LOGW_req},
	{"AT+LOGR", at_LOGR_req},
	{"AT+TMSTAT", at_TMSTAT_req},
    {"AT+TMSTOP", at_TMSTOP_req},
	{"AT+XYCONFIG", at_XYCONFIG_req},
    {"AT+XYSEND", at_XYSEND_req},
    {"AT+XYRECV", at_XYRECV_req},
#endif

#if HTTP
	{"AT+HTTPCREATE",at_http_create},
	{"AT+HTTPCFG", at_http_cfg},
	{"AT+HTTPHEADER", at_http_header},
	{"AT+HTTPCONTENT", at_http_content},
	{"AT+HTTPSEND", at_http_send},
	{"AT+HTTPCLOSE", at_http_close},
#endif

	{"AT+SGSW", at_SGSW_req},     //add new command wsl 20220330
	{"AT+QATWAKEUP", at_QATWAKEUP_req},     
	{"AT+QCGDEFCONT", at_QCGDEFCONT_req},
	{"AT+MGEDRXRPT", at_MGEDRXRPT_rep}, //MG add 20230404
	
	{0, 0} //can not delete!!!
};

struct at_inform_proc_e at_basic_info[] = {
	{"+FORCEDL:", at_FORCEDL_info},
#if AT_SOCKET
	{"+TCPACK:", at_TCPACK_info},
#endif
	{0, 0} //can not delete!!!
};

struct at_serv_proc_e *g_at_basic_req = at_basic_req;
struct at_inform_proc_e *g_at_basic_info = at_basic_info;


