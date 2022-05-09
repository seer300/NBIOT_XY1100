#pragma once

/****搜索OLD_ATC宏值，可以查看与之前代码的差异****/


//平台实现，PS调用，发送PS相关的URC及结果码，参考  at_rcv_from_nearps/send_rsp_str_to_ext
void  SendAtInd2User (char *pAt, unsigned int ulAtLen);

//PS实现，平台调用，发送从串口接收来的PS相关AT命令给ATC
void  SendAt2AtcAp (char *pAt, unsigned int ulAtLen);


//PS实现，平台调用，用于发送从DSP接收到的核间消息给ATC
void  SendShmMsg2AtcAp(char *pBuf, unsigned int BufLen);

/**
 * @brief 由PS调用，透传PS的跨核消息给对方核，内部使用ICM_PS_SHM_MSG
 */
int send_ps_shm_msg(void *msg, int msg_len);



