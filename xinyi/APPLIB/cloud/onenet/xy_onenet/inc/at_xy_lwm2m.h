#ifndef _AT_XY_LWM2M_H
#define _AT_XY_LWM2M_H
#include "xy_lwm2m.h"

int insert_cached_urc_node(xy_lwm2m_cached_urc_common_t *node);

int at_proc_qlaconfig_req(char *at_buf, char **rsp_cmd);
int at_proc_qlacfg_req(char *at_buf, char **rsp_cmd);
int at_proc_qlareg_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaupdate_req(char *at_buf, char **rsp_cmd);
int at_proc_qladereg_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaaddobj_req(char *at_buf, char **rsp_cmd);
int at_proc_qladelobj_req(char *at_buf, char **rsp_cmd);
int at_proc_qlardrsp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlawrrsp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaexersp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaobsrsp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlanotify_req(char *at_buf, char **rsp_cmd);
int at_proc_qlard_req(char *at_buf, char **rsp_cmd);
int at_proc_qlstatus_req(char *at_buf, char **rsp_cmd);
int at_proc_qlarecover_req(char *at_buf, char **rsp_cmd);
int at_proc_qlasend_req(char *at_buf, char **prsp_cmd);

#endif
