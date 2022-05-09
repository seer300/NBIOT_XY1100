#include "at_ps_proxy.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "atc_ps.h"
#include "xy_utils.h"

extern void xy_atc_data_req(unsigned short usDataLen, unsigned char*pucData);

//平台实现，PS调用，发送PS相关的URC及结果码，参考  at_rcv_from_nearps/send_rsp_str_to_ext
void SendAtInd2User(char *pAt, unsigned int ulAtLen)
{
    xy_printf("[SendAtInd2User] %s", (const char *)pAt);
    send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, pAt, ulAtLen, NEARPS_DSP_FD);
}

//PS实现，平台调用，发送从串口接收来的PS相关AT命令给ATC
void SendAt2AtcAp(char *pAt, unsigned int ulAtLen)
{
    //SendAtInd2User("\r\nOK\r\n",strlen("\r\nOK\r\n"));
    xy_printf("SendAt2AtcAp:%s", pAt);

    xy_atc_data_req(ulAtLen, (unsigned char *)pAt);
}

//PS实现，平台调用，用于发送从DSP接收到的核间消息给ATC
void SendShmMsg2AtcAp(char *pBuf, unsigned int BufLen)
{
    ATC_AP_MSG_HEADER_STRU *pMsgHeader;

    pMsgHeader = (ATC_AP_MSG_HEADER_STRU *)pBuf;

    xy_printf("SendShmMsg2AtcAp:msgId=%d, len=%d", pMsgHeader->ulMsgName, BufLen);

    AtcAp_SendMsg2AtcAp(pBuf, &g_AtcApInfo.msgInfo);
}
