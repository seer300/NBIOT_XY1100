#pragma once

#include "softap_macro.h"

#define AT_SOCKET_PASSTHR_MAX_LEN  1024

//0 "Initial"（初始状态） 1 "Connecting"（正在连接） 2 "Connected"（已连接） 3 "Closing"（正在关闭） 4 "Remote Closing"（正在远程关闭）
typedef enum BC26_SOCKET_STATE
{
    BC26_SOCK_INITIAL = 0,
    BC26_SOCK_CONNECTING,
    BC26_SOCK_CONNECTED,
    BC26_SOCK_CLOSING,
    BC26_SOCK_REMOTECLOSING,
}
BC26_SOCKET_STATE_T;

#if AT_SOCKET&&VER_QUCTL260

int at_QICFG_req(char *at_buf, char **prsp_cmd);
int at_QIOPEN_req(char *at_buf, char **prsp_cmd);
int at_QICLOSE_req(char *at_buf, char **prsp_cmd);
int at_QISEND_req(char *at_buf, char **prsp_cmd);
int at_QISTATE_req(char *at_buf, char **prsp_cmd);

#endif //AT_SOCKET