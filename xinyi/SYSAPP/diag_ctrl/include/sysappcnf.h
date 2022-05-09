#ifndef SYSAPPCNF_H
#define SYSAPPCNF_H
#include "xy_nbiot_msg_define.h"
#include "TypeDefine.h"
typedef enum
{
    XY_SUBAPPCNF_BASE = XY_SYSAPP_CNF << 16,
    MSG_SHOW_NAME(XY_SYSAPP_DETECT_CNF, nullStruct),
    MSG_SHOW_NAME(XY_SYSAPP_READNV_CNF, nullStruct),
    MSG_SHOW_NAME(XY_SYSAPP_WRITENV_CNF, nullStruct),
    MSG_SHOW_NAME(XY_SYSAPP_MEMREADY_CNF, nullStruct),
    MSG_SHOW_NAME(XY_SYSAPP_MEMHAVE_IND, nullStruct),
	MSG_SHOW_NAME(XY_SYSAPP_FILTER_CNF, nullStruct),  
	MSG_SHOW_NAME(XY_SYSAPP_MAXLEN_CNF, nullStruct),  
	MSG_SHOW_NAME(XY_SYSAPP_WIRESHARK_IND, nullStruct),
	MSG_SHOW_NAME(XY_SYSAPP_HEART_CNF, nullStruct),	
}XY_SUBAPP_CNF;
#endif // SYSAPPCNF_H
