#include "at_utils.h"
#include "xy_utils.h"


/**
 *  close_urc置1时也不能被丢弃的主动上报
 */
char* urc_cannot_drop[] = 
{
	//平台主动上报
	"REBOOTING",
	"+POWERON:",
	"+POWERDOWN:",
	"+NPSMR:",
#if TELECOM_VER
	"+QLWEVTIND:",
	"+QREGSWT:",
	"+NSMI:",
	"+NNMI",
	"+QLWULDATASTATUS:",
	"+QDTLSSTAT:",
#endif
#if MOBILE_VER
	"+MIPLDISCOVER:",
	"+MIPLOBSERVE:",
	"+MIPLREAD:",
	"+MIPLWRITE:",
	"+MIPLEXECUTE:",
	"+MIPLPARAMETER:",
	"+MIPLEVENT:",
    "+QLANOTIFY:",
    "+QLAURC:",
    "+QLARDRSP:",
    "+QLAEXERSP:",
    "+QLAWRRSP:",
    "+QLAOBSRSP:",
    "+QLAUPDATE:",
    "+QLAREG:",
    "+QLADEREG:",
    "+QLACONFIG:",
    "+QLACFG:",
    "+QLAADDOBJ:",
    "+QLARD:",
    "+QLASTATUS:",
    "+QLARECOVER:",
#endif
#if AT_SOCKET
	"+NSONMI:",
	"+NSOCL:",
#endif
#if MQTT
	"+MQCONNACK:",
	"+MQPUBACK:",
	"+MQSUBACK:",
	"+MQUNSUBACK:",
	"+MQPUB:",
	"+MQPUBREC:",
	"+MQPUBREL:",
	"+MQPUBCOMP:",
	"+MQDISCON:",
	"+QMTOPEN:",
	"+QMTCLOSE:",
	"+QMTCONN:",
	"+QMTDISC:",
	"+QMTSUB:",
	"+QMTUNS:",
	"+QMTPUB:",
	"+QMTSTAT:",
	"+QMTRECV:",
#endif
};

//该接口的上层接口drop_unused_urc会在idle线程调用，内部不能调用softap_printf接口，否则会有死机风险
int urc_can_drop(char *buf)
{
	unsigned int index;

	for (index = 0; index < sizeof(urc_cannot_drop) / sizeof(char *); index++)
	{
		if (!at_strncasecmp(buf, urc_cannot_drop[index], strlen(urc_cannot_drop[index])))
			return XY_ERR;
	}
	return XY_OK;
}
