#pragma once

struct	rcv_data_nod *get_oldest_rcv_cdp(int skt_id);
int record_cdp_send_bytes(unsigned int size, int idx);
void del_cdp_rcv_nod(int skt_id,struct	rcv_data_nod *nod);
int  is_valid_cdp_by_idx(int idx);
extern int at_NCDP_req(char *at_buf, char **prsp_cmd);
extern int at_NMGS_req(char *at_buf, char **prsp_cmd);
extern int at_NMGS_EXT_req(char *at_buf, char **prsp_cmd);
extern int at_NQMGS_req(char *at_buf, char **prsp_cmd);
extern int at_NQMGR_req(char *at_buf, char **prsp_cmd);
extern int at_NNMI_req(char *at_buf, char **prsp_cmd);
extern int at_NSMI_req(char *at_buf, char **prsp_cmd);
extern int at_NMGR_req(char *at_buf, char **prsp_cmd);
extern int at_QLWSREGIND_req(char *at_buf, char **prsp_cmd);
extern int at_QREGSWT_req(char *at_buf, char **prsp_cmd);
extern int at_CDPUPDATE_req(char *at_buf, char **prsp_cmd);
extern int at_NMSTATUS_req(char *at_buf, char **prsp_cmd);
extern int at_QLWULDATASTATUS_req(char *at_buf, char **prsp_cmd);
extern void at_send_NSMI(int state,uint8_t seq_num);
extern int at_CDPRMLFT_req(char *at_buf, char **prsp_cmd);
extern int cdp_resume_task();
extern int at_QSECSWT_req(char *at_buf, char **prsp_cmd);
extern int at_QSETPSK_req(char *at_buf, char **prsp_cmd);
extern int at_QRESETDTLS_req(char *at_buf, char **prsp_cmd);
extern int at_QDTLSSTAT_req(char *at_buf, char **prsp_cmd);
//extern int at_QCFG_req(char *at_buf, char **prsp_cmd);
extern int at_QLWULDATA_req(char *at_buf, char **prsp_cmd);
extern int at_QLWULDATAEX_req(char *at_buf, char **prsp_cmd);
extern int at_QLWFOTAIND_req(char *at_buf, char **prsp_cmd);
extern int at_QLWEVTIND_req(char *at_buf, char **prsp_cmd);
extern int at_QCRITICALDATA_req(char *at_buf, char **prsp_cmd);

