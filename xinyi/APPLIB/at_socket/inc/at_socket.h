#pragma once

#include "softap_macro.h"

#if AT_SOCKET

int at_NSOCR_req(char *at_buf, char **prsp_cmd);
int at_NSOST_req(char *at_buf, char **prsp_cmd);
int at_NSOSTF_req(char *at_buf, char **prsp_cmd);
int at_NSORF_req(char *at_buf, char **prsp_cmd);
int at_NSOCL_req(char *at_buf, char **prsp_cmd);
int at_NSOCO_req(char *at_buf, char **prsp_cmd);
int at_NSOSD_req(char *at_buf, char **prsp_cmd);
int at_NSONMI_req(char *at_buf, char **prsp_cmd);
int at_NSOSDEX_req(char *at_buf, char **prsp_cmd);
int at_NSOSTEX_req(char *at_buf, char **prsp_cmd);
int at_NSOSTFEX_req(char *at_buf, char **prsp_cmd);
int at_NQSOS_req(char *at_buf, char **prsp_cmd);
int at_NSOSTATUS_req(char *at_buf, char **prsp_cmd);
int at_XDSEND_req(char *at_buf);

#endif //AT_SOCKET


