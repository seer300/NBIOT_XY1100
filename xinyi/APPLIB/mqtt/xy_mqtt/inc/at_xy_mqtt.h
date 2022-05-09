#pragma once

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_QMTCFG_req(char *at_buf, char **prsp_cmd);
int at_QMTOPEN_req(char *at_buf, char **prsp_cmd);
int at_QMTCLOSE_req(char *at_buf, char **prsp_cmd);
int at_QMTCONN_req(char *at_buf, char **prsp_cmd);
int at_QMTDISC_req(char *at_buf, char **prsp_cmd);
int at_QMTSUB_req(char *at_buf, char **prsp_cmd);
int at_QMTUNS_req(char *at_buf, char **prsp_cmd);
int at_QMTPUB_req(char *at_buf, char **prsp_cmd);

