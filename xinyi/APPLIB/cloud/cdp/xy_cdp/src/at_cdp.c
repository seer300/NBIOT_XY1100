#include "softap_macro.h"
#if TELECOM_VER
#include "xy_utils.h"
#include "at_cdp.h"
#include "xy_cdp_api.h"
#include "cdp_backup.h"
#include "agent_tiny_demo.h"
#include "agenttiny.h"
#include "atiny_context.h"
#include "at_global_def.h"
#include "xy_at_api.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "lwip/inet.h"
#include "net_app_resume.h"
#include "at_ps_cmd.h"
#include "xy_system.h"
#include "atiny_fota_manager.h"
#include "atiny_fota_state.h"

#if XY_DTLS
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_internal.h"
#endif

#define CDP_NO_RUNNING -2

int g_send_status = 0;
extern cdp_regInfo_t *g_cdp_regInfo;
extern osThreadId_t g_lwm2m_TskHandle;

extern char g_req_type; 
extern upstream_info_t *upstream_info;
extern downstream_info_t *downstream_info;
extern uint8_t *g_endpointName;
extern osSemaphoreId_t cdp_wait_sem;
extern int cdp_register_fail_flag;

#define MSGTYPE_TRANS(type) \
{\
   if(type == 0x0000)\
   		type = cdp_NON;\
   else if(type == 0x0001) \
   		type = cdp_NON_RAI; \
   else if(type == 0x0100)   \
   	    type = cdp_CON;       \
   else if(type == 0x0101)     \
   		type = cdp_CON_WAIT_REPLY_RAI; \
   else if(type == 0x0010)\
   		type = cdp_NON_WAIT_REPLY_RAI; \
   else \
   	    type = XY_ERR;\
}

#define NMGSTYPE_TRANS(type) \
{\
   if(type == 0)\
   		type = cdp_NON;\
   else if(type == 1) \
   		type = cdp_NON_RAI; \
   else if(type == 2)   \
   	    type = cdp_NON_WAIT_REPLY_RAI;       \
   else if(type == 100)     \
   		type = cdp_CON; \
   else if(type == 101)\
   		type = cdp_CON_WAIT_REPLY_RAI; \
   else if(type == 102)\
   		type = cdp_CON_WAIT_REPLY_RAI; \
   else \
   	    type = XY_ERR;\
}




//AT+NMGS/AT+QLWULDATA\AT+QLWULDATAEX通用子接口
int cdp_send_data(char *data, int len, int msg_type, uint8_t seq_num)
{
    int pending_num = -1;
    
    if(data == NULL)
        return XY_ERR;
    
    if(len>MAX_REPORT_DATA_LEN || len<=0)
        return XY_ERR;
	
	if(cdp_find_match_udp_node(seq_num) == XY_OK)
		return XY_ERR;
   
    if (!ps_netif_is_ok()) 
        return XY_ERR;

    resume_net_app(CDP_TASK);
    
	if(is_cdp_running() == 0)	  
	{
		return CDP_NO_RUNNING;
	}
	
	handle_data_t *handle = (handle_data_t *)g_phandle;
	lwm2m_context_t *context = handle->lwm2m_context;
	if(context->state != STATE_READY)	
	{
		return XY_ERR;
	}
	
	if(app_data_report_check() == XY_ERR)
		return XY_ERR;
	
    pending_num = get_upstream_message_pending_num();
    if(pending_num >= 8)
    {
        return XY_ERR;
    }
	
#if !VER_QUECTEL && !VER_QUCTL260
	g_send_status = 0;
#endif

    if (send_message_via_lwm2m(data, len, msg_type,seq_num))
        return XY_ERR;
	
	cdp_add_sninfo_node(seq_num,msg_type);
	
    return XY_OK;
}


/*****************************************************************************
 Function    : +NMGS=<length>,<data>
 Description : through the CDP sent information to the network from the terminal  
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NMGS=<length>,<data>[,<type>] 
 *****************************************************************************/
int at_NMGS_req(char *at_buf, char **prsp_cmd)
{
	int len = -1;
    int type = 0;
    int ret = -1;
    char *chr = at_buf;
    char *src_data = xy_zalloc(strlen(at_buf));
    char *tans_data = NULL;

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}

    if (at_parse_param("%d(1-1024),%s,%d,%d", at_buf, &len,src_data,&type) != AT_OK)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }
	
	NMGSTYPE_TRANS(type);
    if (type == XY_ERR)
    {
       *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
       goto ERR_PROC;
    }

    if(cdp_get_con_send_flag())
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }
	
 	tans_data = xy_zalloc(len);
	if (hexstr2bytes(src_data, len * 2, tans_data, len) == -1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	
	if(cdp_send_data(tans_data,len,type,0) != XY_OK)
	{			
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	    goto ERR_PROC;
	}
	cdp_con_flag_init(type,0);
ERR_PROC:
    if(src_data)
        xy_free(src_data);

    if(tans_data)
        xy_free(tans_data);
    return AT_END;
}

/*****************************************************************************
 Function    : +NMGSEXT=<msg_type><length>,<data>
 Description : through the CDP sent information to the network from the terminal  
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NMGSEXT=<msg_type><length>,<data>
                msg_type=0 , CON
                msg_type=1 , NON
 *****************************************************************************/
int at_NMGS_EXT_req(char *at_buf, char **prsp_cmd)
{
	int len = -1;
	int type = -1;
	int ret = -1;
	char *chr = at_buf;
	char *data = xy_zalloc(strlen(at_buf));
	char *tans_data = NULL;

	if(g_req_type != AT_CMD_REQ)
	{
	    *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	    goto ERR_PROC;
	}

	if (!ps_netif_is_ok()) {
	    *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
	    goto ERR_PROC;
	}

	if (at_parse_param("%d,%d", at_buf, &type,&len) != AT_OK)
	{
	    *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	    goto ERR_PROC;
	}

	if(len>MAX_REPORT_DATA_LEN || len<=0)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (type > cdp_NON || type < cdp_CON)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

	//解析含有引号的ascii字符串
	tans_data = xy_malloc(len);
	if(get_ascii_data(",,%s",(char*)at_buf,len,data) == AT_OK)
	{
	    memcpy(tans_data, data, len);
	}
	else 
	{
		//下面用来解析十六进制不带引号字符串，需要过滤到带引号的情况
		//否则AT+NMGSEXT=0,3,"112233" 和 AT+NMGSEXT=0,3,112233无法区分
		if((chr = at_strnchr(chr, '"', 2)) != NULL)
		{
			 *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        goto ERR_PROC;
		}
			
		if (at_parse_param(",%d,%s", at_buf,&len,data) != AT_OK)
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        goto ERR_PROC;
	    }

		if(strlen(data) == len * 2) //字符串
		{
			 if (hexstr2bytes(data, len * 2, tans_data, len) == -1)
	        {
	            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	            goto ERR_PROC;
	        }
		}
	    else
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        goto ERR_PROC;
	    }
	}

	resume_net_app(CDP_TASK);

	if (is_cdp_running() == 0)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}

	if(g_phandle->lwm2m_context->state != STATE_READY)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}

	g_send_status = 0;

	ret = send_message_via_lwm2m(tans_data, len, type,0);

	if (ret == -1)
	{
	    *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	    goto ERR_PROC;
	}

	ERR_PROC:
	if(data)
	    xy_free(data);

	if(tans_data)
	    xy_free(tans_data);
	return AT_END;
}


/*****************************************************************************
 Function    : PENDING=<pending>,SENT=<sent>,ERROR=<error>
 Description : Query the status of upstream information
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NQMGS   PENDING=<pending>,SENT=<sent>,ERROR=<error>    OK
 *****************************************************************************/
int at_NQMGS_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	*prsp_cmd = xy_zalloc(60);
    if(upstream_info == NULL)
		sprintf(*prsp_cmd, "\r\nPENDING=0,SENT=0,ERROR=0\r\n\r\nOK\r\n");
	else
    	sprintf(*prsp_cmd, "\r\nPENDING=%d,SENT=%d,ERROR=%d\r\n\r\nOK\r\n", 
            get_upstream_message_pending_num(), get_upstream_message_sent_num(), get_upstream_message_error_num());
	return AT_END;
}

/*****************************************************************************
 Function    : BUFFERED=<buffered>,RECEIVED=<received>,DROPPED=<dropped>
 Description : Query the status of the downstream information received from the network
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NQMGR   BUFFERED=<buffered>,RECEIVED=<received>,DROPPED=<dropped>    OK
 *****************************************************************************/
int at_NQMGR_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}
	
	*prsp_cmd = xy_zalloc(60);
#if !VER_QUCTL260
	if(downstream_info == NULL)
		sprintf(*prsp_cmd, "\r\nBUFFERED=0,RECEIVED=0,DROPPED=0\r\n\r\nOK\r\n");
	else
#endif
    sprintf(*prsp_cmd, "\r\nBUFFERED=%d,RECEIVED=%d,DROPPED=%d\r\n\r\nOK\r\n", 
        get_downstream_message_buffered_num(), get_downstream_message_received_num(), get_downstream_message_dropped_num());
	return AT_END;
}

/*****************************************************************************
 Function    : <length>,<data>
 Description : returns the oldest buffered messages and deletes them from the buffer
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NMGR  <length>,<data>  ok
 *****************************************************************************/
int at_NMGR_req(char *at_buf, char **prsp_cmd)
{
    int data_len = 0;
    char *data = NULL;

    (void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

#if !VER_QUECTEL && !VER_QUCTL260
	if(is_cdp_running() == 0)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
    }
#endif

    data = (char *)get_message_via_lwm2m((int *)&data_len);

#if !VER_QUECTEL && !VER_QUCTL260
	if (data_len == 0)
	{
	   *prsp_cmd = xy_zalloc(16);
	   sprintf(*prsp_cmd,"\r\n0\r\n\r\nOK\r\n");
	}
#endif

    if (data_len > 0)
    {
        *prsp_cmd = xy_zalloc(20 + data_len*2);
		sprintf(*prsp_cmd,"\r\n+NMGR: %d,", data_len);
		bytes2hexstr(data, data_len, *prsp_cmd + strlen(*prsp_cmd), data_len * 2+1);
		strcat(*prsp_cmd, "\r\n\r\nOK\r\n");
        xy_free(data);
    }
	return AT_END;
}

/*****************************************************************************
 Function    : +NNMI=<status>
 Description : Sets or gets instructions for recving new information
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NNMI=<status>
 *****************************************************************************/
int at_NNMI_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type != AT_CMD_REQ && g_req_type != AT_CMD_QUERY)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
		
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	if(g_req_type == AT_CMD_REQ)
	{
	    int nnmi = -1;
	    void *p[] = {&nnmi};
		
		if (at_parse_param_2("%d", at_buf, p) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(nnmi>2 || nnmi<0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		g_softap_var_nv->cdp_nnmi = nnmi;
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(40);
#if VER_QUCTL260
		snprintf(*prsp_cmd, 40, "\r\n+NNMI: %d\r\n\r\nOK\r\n", g_softap_var_nv->cdp_nnmi);
#else
		snprintf(*prsp_cmd, 40, "\r\n+NNMI:%d\r\n\r\nOK\r\n", g_softap_var_nv->cdp_nnmi);
#endif
	}
	return AT_END;
}

/*****************************************************************************
 Function    : +NSMI=<status>
 Description : Sets or gets instructions for sending new information
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NSMI=<status>
 *****************************************************************************/
int at_NSMI_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type != AT_CMD_REQ && g_req_type != AT_CMD_QUERY)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
		
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}
	
	if(g_req_type == AT_CMD_REQ)
	{
	    int nsmi = -1;
	    void *p[] = {&nsmi};
		if (at_parse_param_2("%d", at_buf, p) != AT_OK) {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(nsmi>1 || nsmi<0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		g_softap_var_nv->cdp_nsmi = nsmi;
	}
	else if(g_req_type == AT_CMD_QUERY)
	{	
		*prsp_cmd = xy_zalloc(40);
		snprintf(*prsp_cmd, 40, "\r\n+NSMI:%d\r\n\r\nOK\r\n", g_softap_var_nv->cdp_nsmi);
	}
	
	return AT_END;
}

/*****************************************************************************
 Function    : at_send_NSMI
 Description : Send instructions for send information
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : NULL
 *****************************************************************************/
void at_send_NSMI(int state,uint8_t seq_num)
{
#if !VER_QUCTL260
	if(g_softap_var_nv->cdp_nsmi == 1)
	{
		char *rsp_cmd = xy_zalloc(40);
		if(state==0)
		{
			sprintf(rsp_cmd,"\r\n+NSMI:SENT\r\n");	
		}	
		else
		{
			if(seq_num == 0)
				sprintf(rsp_cmd,"\r\n+NSMI:DISCARDED\r\n");
			else
				sprintf(rsp_cmd,"\r\n+NSMI:DISCARDED,%d\r\n",seq_num);
		}
		send_urc_to_ext(rsp_cmd);
		xy_free(rsp_cmd);
	}
#else 
	char *rsp_cmd = xy_zalloc(40);
	if(state==0)
		sprintf(rsp_cmd,"\r\n+QLWEVTIND: 4\r\n");		
	else
		sprintf(rsp_cmd,"\r\n+QLWEVTIND: 5\r\n");
	send_urc_to_ext(rsp_cmd);
	xy_free(rsp_cmd);
#endif
}

/*****************************************************************************
 Function    : +NCDP=<ip_addr>[,<port>]
 Description : Set and query the IP address and port of the CDP server
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NCDP=<ip_addr>[,<port>]
               AT+NCDP?      +NCDP:192.168.5.1,5683       OK
 *****************************************************************************/
int at_NCDP_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	    char *ip_addr_str = xy_zalloc(80);
		
	    int port = -1;

		if (at_parse_param("%s,%d", at_buf, ip_addr_str, &port) != AT_OK) {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}
		
		if(port == -1 || port == 0)
		{
#if VER_QUECTEL
			if(port == 0 || (port == -1 && g_softap_fac_nv->cloud_server_port == 0))
				port = 5683;
			else if(port == -1 && g_softap_fac_nv->cloud_server_port != 0)
				port =g_softap_fac_nv->cloud_server_port;
#else

			if(port == -1)
			{
				if(g_softap_fac_nv->cdp_dtls_switch == 1)
					port = 5684;
				else
					port = 5683;
			}
#endif
		}
		
		if((strlen(ip_addr_str) <= 0)||(strlen(ip_addr_str) >= 40) || (port < 0) || (port > 65535))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}

        if(set_cdp_server_settings(ip_addr_str, (u16_t)port) < 0)
    	{
    		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
    	}
			
free_out:
		xy_free(ip_addr_str);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(80);
		if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
		{
			snprintf(*prsp_cmd, 80, "\r\n+NCDP:\r\n\r\nOK\r\n");
		}
		else
		{
			snprintf(*prsp_cmd, 80, "\r\n+NCDP:%s,%d\r\n\r\nOK\r\n", (char *)get_cdp_server_ip_addr_str(), (int)get_cdp_server_port());
		}
	}
    else
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
        
	return AT_END;
}

/*****************************************************************************
 Function    : +QREGSWT=<type>
 Description : Configure the registration mode for the CDP platform
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+ QREGSWT =0
               OK
 *****************************************************************************/
int at_QREGSWT_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	    char type = -1;
        
	    if (at_parse_param("%1d", at_buf, &type) != AT_OK) {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
	    if (type >=0 && type <= 2)
	    { 
			g_softap_fac_nv->cdp_register_mode = type;
			SAVE_FAC_PARAM(cdp_register_mode);
	    }
	    else
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	    }
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(24);
		snprintf(*prsp_cmd, 24, "\r\n+QREGSWT:%d\r\n\r\nOK\r\n", g_softap_fac_nv->cdp_register_mode);
	}
    else
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
	
    return AT_END;
}

/*****************************************************************************
 Function    : +QLWEVTIND=<type>\r\n      
 Description : Used for set lwm2m event report mode
 Input       : at_buf   ---type
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QLWEVTIND=1
               OK
 *****************************************************************************/
int at_QLWEVTIND_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type != AT_CMD_REQ && g_req_type != AT_CMD_QUERY
		&& g_req_type != AT_CMD_TEST)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
		
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	if(g_req_type == AT_CMD_REQ)
	{
	    int mode = -1; 
	    if (at_parse_param("%d", at_buf, &mode) != AT_OK) {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
        
	    if (mode >=0 && mode <= 1)
	    { 
	    	if(mode == 0)
				g_softap_var_nv->cdp_event_report_disable = 1;
			else if(mode == 1)
				g_softap_var_nv->cdp_event_report_disable = 0;
	    }
	    else
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	    }
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(30);
		if(g_softap_var_nv->cdp_event_report_disable == 1)
		{
			if(g_softap_var_nv->cdp_lwm2m_event_status == 10)//服务器拒绝
				snprintf(*prsp_cmd, 30, "\r\n+QLWEVTIND:0,255\r\n\r\nOK\r\n");
			else
				snprintf(*prsp_cmd, 30, "\r\n+QLWEVTIND:0,%d\r\n\r\nOK\r\n", g_softap_var_nv->cdp_lwm2m_event_status);
		}
		else if(g_softap_var_nv->cdp_event_report_disable == 0)
		{
			if(g_softap_var_nv->cdp_lwm2m_event_status == 10)//服务器拒绝
				snprintf(*prsp_cmd, 30, "\r\n+QLWEVTIND:1,255\r\n\r\nOK\r\n");
			else
				snprintf(*prsp_cmd, 30, "\r\n+QLWEVTIND:1,%d\r\n\r\nOK\r\n", g_softap_var_nv->cdp_lwm2m_event_status);
		}
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(30);
		snprintf(*prsp_cmd, 30, "\r\n+QLWEVTIND:(0,1)\r\n\r\nOK\r\n");
	}
    return AT_END;
}


extern int deregister_lwm2m_task();
extern int cdp_lifetime;
/*****************************************************************************
 Function    : +QLWSREGIND=<type><lifetime>
 Description : Used for registration to register the CDP platform, 
               only when configured for manual registration
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+ QLWSREGIND =0
               OK
 *****************************************************************************/
int at_QLWSREGIND_req(char *at_buf, char **prsp_cmd)
{
    int type = -1;
    int lifetime = 0;

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

	if (!ps_netif_is_ok()) {
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		return AT_END;
	}
	
    resume_net_app(CDP_TASK);

    if (at_parse_param("%d", at_buf, &type) != AT_OK) {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
	
    if (type == 0)
    {
        if (at_parse_param(",%d", at_buf, &lifetime) != AT_OK) {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }

		if(g_softap_fac_nv->cdp_lifetime > 0)
			cdp_lifetime = g_softap_fac_nv->cdp_lifetime;
		else
		{
			if(0 == lifetime)
	        {
	            cdp_lifetime = 0;
	        }
	        else
	        {
	          //  if(lifetime < 120 || lifetime > LWM2M_DEFAULT_LIFETIME *30)
	            if(lifetime < 15 || lifetime > LWM2M_DEFAULT_LIFETIME *30)
	            {
	                *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	                return AT_END;
	            }
	            else
	            {
	                cdp_lifetime = lifetime;
	            }
	        }
		}
			
#if !IS_DSP_CORE
			if(CHECK_SDK_TYPE(OPERATION_VER))
				xy_work_lock(LOCK_EXT);
#endif
        	if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
        	{
        		xy_printf("cdp server addr is empty!");
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#if !IS_DSP_CORE
				if(CHECK_SDK_TYPE(OPERATION_VER))
					xy_work_unlock(LOCK_EXT);
#endif

        	}
            else if(start_cdp_lwm2m() == 0)
            {
            	xy_printf("registed failed!");
#if !IS_DSP_CORE
				if(CHECK_SDK_TYPE(OPERATION_VER))
					xy_work_unlock(LOCK_EXT);
#endif

#if VER_QUECTEL
				goto end;
#else
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#endif
        }
    }
    else if (type == 1)
    {
        if(deregister_lwm2m_task() == 0)
        {
        	xy_printf("deregisted failed!");
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto end;
       	}
#if !IS_DSP_CORE
		if(CHECK_SDK_TYPE(OPERATION_VER))
			xy_work_unlock(LOCK_EXT);
#endif		
    }
    else
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
end:
    return AT_END;
}

extern int cdp_update_proc(xy_lwm2m_server_t *targetP);
/*****************************************************************************
 Function    : + CDPUPDATE
 Description : Used for update to CDP platform, 
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+CDPUPDATE
               OK
 *****************************************************************************/

int at_CDPUPDATE_req(char *at_buf, char **prsp_cmd)
{
	int ret = -1;

	(void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }
        
	if (!ps_netif_is_ok()) {
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		goto ERR_PROC;
	}

    resume_net_app(CDP_TASK);

	if (is_cdp_running() == 0)    
    {
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
    }

	handle_data_t *handle = (handle_data_t *)g_phandle;
    lwm2m_context_t *context = handle->lwm2m_context;
	if(context->state != STATE_READY)	
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}

    xy_lwm2m_server_t *targetP = context->serverList;
    if(targetP == NULL)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

    ret = cdp_update_proc(targetP);
	if (ret == -1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}

ERR_PROC:
	return AT_END;
}


/*****************************************************************************
 Function    : +NMSTATUS？\r\n    +NMSTATUS: <registration_status>\r\n
 Description : Used for check cdp server status, 
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NMSTATUS?
               OK
 *****************************************************************************/
int at_NMSTATUS_req(char *at_buf, char **prsp_cmd)
{
   (void) at_buf;
	if((g_req_type != AT_CMD_QUERY) && (g_req_type != AT_CMD_TEST))
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

#if !VER_QUCTL260 && !VER_QUECTEL
	if (!ps_netif_is_ok())
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		return AT_END;
	}
#endif

#if VER_QUCTL260
	if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
#endif

	*prsp_cmd = xy_malloc(310);
	if(g_req_type == AT_CMD_TEST)
	{
		snprintf(*prsp_cmd, 310, 
		"\r\nUNINITIALISED\r\n\r\nNO_UE_IP\r\n\r\nINIITIALISING\r\n\r\nREGISTERING\r\n\r\nREGISTERED\r\n"
#if VER_QUECTEL
		"\r\nMISSING_CONFIG\r\n\r\nINIITIALISED\r\n\r\nINIT_FAILED\r\n\r\nDEREGISTERED\r\n\r\nMO_DATA_ENABLED\r\n"
		"\r\nREJECTED_BY_SERVER\r\n\r\nTIMEOUT_AND_RETRYING\r\n\r\nREG_FAILED\r\n\r\nDEREG_FAILED\r\n"
#endif
		"\r\nOK\r\n");
		return AT_END;
	}

	if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
	{
#if VER_QUCTL260
		snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: UNINITIALISED\r\n\r\nOK\r\n");
#else
		snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS:NO_UE_IP\r\n\r\nOK\r\n");
#endif
		return AT_END;
	}

#if VER_QUCTL260
	if(g_phandle != NULL)
	{
		lwm2m_context_t *context = (lwm2m_context_t *)(g_phandle->lwm2m_context);
		if(context == NULL || (context != NULL && context->state <= STATE_REGISTER_REQUIRED))
		{
		   snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: INIITIALISING\r\n\r\nOK\r\n");
		   return AT_END;
		}
		else if(context->state == STATE_REGISTERING)
		{
		   snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: REGISTERING\r\n\r\nOK\r\n");
		   return AT_END;
		}
	}

 	if(g_softap_var_nv->cdp_lwm2m_event_status == ATINY_REG_OK)
	   snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: REGISTERED\r\n\r\nOK\r\n");
	else if(g_softap_var_nv->cdp_lwm2m_event_status == ATINY_DEREG)
	   snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: DEREGISTERED\r\n\r\nOK\r\n");
	else if(g_softap_var_nv->cdp_lwm2m_event_status == ATINY_REG_FAIL || g_softap_var_nv->cdp_lwm2m_event_status == 255)
	   snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: UNINITIALISED\r\n\r\nOK\r\n");
	else if(g_softap_var_nv->cdp_lwm2m_event_status == 10) //服务器拒绝
	   snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: REJECTED_BY_SERVER\r\n\r\nOK\r\n");
	else
		snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS: REGISTERED_AND_OBSERVED\r\n\r\nOK\r\n");
#else
   if(g_phandle != NULL)
   {
	   lwm2m_context_t *context = (lwm2m_context_t *)(g_phandle->lwm2m_context);
	   if(context == NULL || (context != NULL && context->state <= STATE_REGISTER_REQUIRED))
	   {
		  snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS:INIITIALISING\r\n\r\nOK\r\n");
		  return AT_END;
	   }
	   else if(context->state == STATE_REGISTERING)
	   {
		  snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS:REGISTERING\r\n\r\nOK\r\n");
		  return AT_END;
	   }
   }
   
   if(g_softap_var_nv->cdp_lwm2m_event_status == 255)
	  snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS:UNINITIALISED\r\n\r\nOK\r\n");
   else
	   snprintf(*prsp_cmd, 96, "\r\n+NMSTATUS:REGISTERED\r\n\r\nOK\r\n");
#endif
   return AT_END;
}

/*****************************************************************************
 Function    : +QLWULDATA=<length>,<data>
 Description : through the CDP sent information to the network from the terminal  
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QLWULDATA=<length>,<data>
 Note        : 功能同AT+NMGS
 *****************************************************************************/
int at_QLWULDATA_req(char *at_buf, char **prsp_cmd)
{
	int len = -1;
	int ret = -1;
	char *tans_data = NULL;
	char *src_data = xy_zalloc(strlen(at_buf));
	int seq_num = 0;

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}

		
	if (at_parse_param("%d,%s,%d", at_buf, &len,src_data, &seq_num) != AT_OK)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}

	if(seq_num < 0 || seq_num > 255)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}

	if(len>MAX_REPORT_DATA_LEN || len<=0 || strlen(src_data)!=len * 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}

	tans_data = xy_zalloc(len);
	if (hexstr2bytes(src_data, len * 2, tans_data, len) == -1)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto ERR_PROC;
	}
	
#if VER_QUECTEL
	ret = cdp_send_data(tans_data,len,cdp_NON,(uint8_t)seq_num);
#else
	ret = cdp_send_data(tans_data,len,0,(uint8_t)seq_num);
#endif

	if(ret != XY_OK)
	{
#if VER_QUECTEL
		if(ret == CDP_NO_RUNNING)
		{	
		    if(start_cdp_lwm2m() == 0)
	        {
	        	xy_printf("registed failed!");
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        }
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto ERR_PROC;
		}
#endif			
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	    goto ERR_PROC;
	}
	ERR_PROC:
	if(tans_data)
		xy_free(tans_data);
	if(src_data)
		xy_free(src_data);
	return AT_END;
}

/*****************************************************************************
 Function    : +QLWULDATAEX=<length>,<data>,<msg_type>
 Description : through the CDP sent information to the network from the terminal  
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QLWULDATAEX=<length>,<data>,<msg_type>
			   msg_type=0x0000  表示发送NON信息
			   msg_type=0x0001  表示发送NON信息之后主动触发RAI
			   msg_type=0x0100  表示发送CON信息
			   msg_type=0x0101  表示发送CON信息收到回复之后主动触发RAI
 Note        : 功能同AT+NMGS_EXT
 *****************************************************************************/
int at_QLWULDATAEX_req(char *at_buf, char **prsp_cmd)
{
    int len = -1;
    int type = -1;
    int ret = -1;
    char *chr = at_buf;
    char *data = xy_zalloc(strlen(at_buf));
    char *tans_data = NULL;
	int seq_num = 0;

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}

    if (at_parse_param("%d,%s,%d,%d", at_buf, &len,data,&type,&seq_num) != AT_OK)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }
	
	MSGTYPE_TRANS(type);
    if (type == XY_ERR)
    {
       *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
       goto ERR_PROC;
    }

    if(len>MAX_REPORT_DATA_LEN || len<=0 || seq_num < 0 || seq_num > 255
		|| cdp_get_con_send_flag())
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }
	
   if(strlen(data) == len)
    {   
        if((chr = at_strnchr(chr, '"', 2)) == NULL)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto ERR_PROC;
        }
        tans_data = xy_malloc(len);
        memcpy(tans_data, data, len);
    }
    else if(strlen(data) == len * 2)
    {
        if((chr = at_strnchr(chr, '"', 2)) != NULL)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto ERR_PROC;
        }
                
        tans_data = xy_malloc(len);
        if (hexstr2bytes(data, len * 2, tans_data, len) == -1)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto ERR_PROC;
        }
    }
    else
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }
	
	ret = cdp_send_data(tans_data,len,type,seq_num);
	if(ret != XY_OK)
	{
#if VER_QUECTEL
		if(ret == CDP_NO_RUNNING)
		{	
		    if(start_cdp_lwm2m() == 0)
	        {
	        	xy_printf("registed failed!");
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        }
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto ERR_PROC;
		}
#endif			
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	    goto ERR_PROC;
	}
	cdp_con_flag_init(type,seq_num);
ERR_PROC:
    if(data)
        xy_free(data);

    if(tans_data)
        xy_free(tans_data);
    return AT_END;
}


/*****************************************************************************
 Function    : + QLWULDATASTATUS\r\n       +QLWULDATASTATUS: <status>
 Description : Used for check send data status, 
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QLWULDATASTATUS?
               OK
 *****************************************************************************/
int at_QLWULDATASTATUS_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
	uint8_t seq_num = 0;

    if(g_req_type != AT_CMD_QUERY)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
	}
#if !VER_QUECTEL && !VER_QUCTL260
	if(!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if(upstream_info == NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }
#endif

    *prsp_cmd = xy_zalloc(36);
#if VER_QUECTEL || VER_QUCTL260
	seq_num = cdp_get_cur_seq_num();
	if(seq_num != 0 && cdp_find_match_udp_node(seq_num) == XY_OK)
	{
       	 sprintf(*prsp_cmd, "\r\n+QLWULDATASTATUS:0,%d\r\n\r\nOK\r\n",seq_num);
	}
	
	if(!seq_num)
	{
		sprintf(*prsp_cmd, "\r\n+QLWULDATASTATUS: %d\r\n\r\nOK\r\n", g_send_status);
	}
	else
	{
		sprintf(*prsp_cmd, "\r\n+QLWULDATASTATUS:%d,%d\r\n\r\nOK\r\n", g_send_status,seq_num);
	}
#else
	if(upstream_info->pending_num > 0)
    {
        sprintf(*prsp_cmd, "\r\n+QLWULDATASTATUS:0\r\n\r\nOK\r\n");
    }
    else
    {
        sprintf(*prsp_cmd, "\r\n+QLWULDATASTATUS:%d\r\n\r\nOK\r\n", g_send_status);
    }
#endif

ERR_PROC:
	return AT_END;
}

/*****************************************************************************
 Function    : +CDPRMLFT\r\n    +CDPRMLFT:< left_lifetime>
 Description : Used for get cdp remain lifetime
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+CDPRMLFT
               +CDPRMLFT:86387
               OK
 *****************************************************************************/
int at_CDPRMLFT_req(char *at_buf, char **prsp_cmd)
{
    int app_type = 0;
    int remain_lifetime = -1;

    (void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

    if (g_phandle != NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->state != STATE_READY)
        goto error;

    if (!cdp_handle_exist())
    {
        if(!NET_NEED_RECOVERY(CDP_TASK))
            goto error;

        cdp_read_regInfo(OFFSET_NETINFO_PARAM(cdp_regInfo));

        remain_lifetime = g_cdp_regInfo->regtime + g_cdp_regInfo->lifetime - cloud_gettime_s();
    }
    else if ( g_phandle !=NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->serverList != NULL)
    {
        remain_lifetime = g_phandle->lwm2m_context->serverList->registration +
            g_phandle->lwm2m_context->serverList->lifetime - cloud_gettime_s();
    }
    else
        goto error;
    
    *prsp_cmd = xy_zalloc(40);
    if (remain_lifetime >= 0)
    {
        sprintf(*prsp_cmd,"\r\n+CDPRMLFT:%d\r\n", remain_lifetime);        
    }
    else
    {
        sprintf(*prsp_cmd,"\r\n+CDPRMLFT:error\r\n");
    }
    strcat(*prsp_cmd, "\r\nOK\r\n");

	return AT_END;

error:
    *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
    return AT_END;
}

/*****************************************************************************
 Function    : +QSECSWT=<type>[,<NAT type>]
 Description : 该命令用来设置数据加密模式
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QSECSWT=<status>
 *****************************************************************************/
int at_QSECSWT_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	
#if !VER_QUECTEL
		if (g_lwm2m_TskHandle != NULL)
	    {
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			return AT_END;
	    }
#endif

 #if XY_DTLS           
	    int dtls_switch = -1;
        int nat_type = 0;
		if (at_parse_param("%d,%d", at_buf,&dtls_switch, &nat_type) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		if(dtls_switch >= 2 || dtls_switch < 0 || nat_type > 1 || nat_type < 0)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(!strcmp(at_buf,"0,1\r") || !strcmp(at_buf,"0,0\r"))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		g_softap_fac_nv->cdp_dtls_switch = dtls_switch;
		SAVE_FAC_PARAM(cdp_dtls_switch);
		
        if(g_softap_fac_nv->cdp_dtls_switch == 1)
        {
			g_softap_fac_nv->cdp_dtls_nat_type = nat_type;
			SAVE_FAC_PARAM(cdp_dtls_nat_type);
        }
#else
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
#endif
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
        if(g_softap_fac_nv->cdp_dtls_switch == 1){
		    snprintf(*prsp_cmd, 40, "\r\n+QSECSWT:%d,%d\r\n\r\nOK\r\n", 
                g_softap_fac_nv->cdp_dtls_switch, g_softap_fac_nv->cdp_dtls_nat_type);
        }else{
            snprintf(*prsp_cmd, 40, "\r\n+QSECSWT:%d\r\n\r\nOK\r\n", g_softap_fac_nv->cdp_dtls_switch);
        }
        
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;
}

/*****************************************************************************
 Function    : +QSETPSK=<pskid>,<psk>
 Description : 该命令用来配置 PSK ID 和 PSK
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QSETPSK=<status>
 *****************************************************************************/
int at_QSETPSK_req(char *at_buf, char **prsp_cmd)
{
   if(g_req_type == AT_CMD_REQ)
	{
	
#if XY_DTLS
		char* psk = xy_zalloc(strlen(at_buf));
        char* psk_temp  = xy_zalloc(strlen(at_buf));
        char* psk_id = xy_zalloc(strlen(at_buf));
		
#if !VER_QUECTEL
		if((g_lwm2m_TskHandle != NULL) || (g_softap_fac_nv->cdp_dtls_switch == 0))
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            goto ERR_PROC;
        }
#endif
		if (at_parse_param("%s,%s", at_buf, psk_id, psk_temp) != AT_OK) {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto ERR_PROC;
		}

		if((strlen(psk_id) != 15 && strlen(psk_id) != 1) || 
			(strlen(psk_id) == 1 && psk_id[0] != '0'))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto ERR_PROC;
		}

		if (strlen(psk_temp) != 32 || hexstr2bytes(psk_temp, 32, psk, 16) == -1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto ERR_PROC;
		}
			
		if(strlen(psk_id) == 15)
		{
			if(!is_digit_str(psk_id))
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				goto ERR_PROC;
			}
		}

		if(strlen(psk_id) == 1 && psk_id[0] == '0')
    	{
    		xy_get_IMEI(psk_id, 16);
    	}
		
		memcpy(g_softap_fac_nv->cdp_pskid, psk_id, 16);
		SAVE_FAC_PARAM(cdp_pskid);
		
	    memcpy(g_softap_fac_nv->cloud_server_auth, psk, 16);
		SAVE_FAC_PARAM(cloud_server_auth);

ERR_PROC:
		if(psk)
			xy_free(psk);
		if(psk_id)
			xy_free(psk_id);
		if(psk_temp)
			xy_free(psk_temp);
		return AT_END;
#else
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        return AT_END;
#endif

	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(48);
		snprintf(*prsp_cmd, 48, "\r\n+QSETPSK:%s,***\r\n\r\nOK\r\n",g_softap_fac_nv->cdp_pskid);
	}
    else
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }

    return AT_END;
}


/*****************************************************************************
 Function    : +QRESETDTLS
 Description : 该命令用来重置 DTLS 模式
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QRESETDTLS  ok
 *****************************************************************************/
int at_QRESETDTLS_req(char *at_buf, char **prsp_cmd)
{
    int data_len = 0;
    char *data = NULL;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

    if(is_cdp_running() == 0)
	{
#if !VER_QUECTEL
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#endif
		return AT_END;
	}
		
#if XY_DTLS
	if(g_softap_fac_nv->cdp_dtls_switch == 0)
    {
    
#if !VER_QUECTEL
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#endif
        return AT_END;
    }
	
    handle_data_t *handle = (handle_data_t *)g_phandle;
    connection_t  *connList = handle->client_data.connList;
    if(connList == NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        return AT_END;
    }
        
    mbedtls_ssl_context *ssl = (mbedtls_ssl_context *)connList->net_context;
    
    if(ssl == NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        return AT_END;
    }

	if(ssl->state > MBEDTLS_SSL_HELLO_REQUEST  && ssl->state < MBEDTLS_SSL_HANDSHAKE_OVER)
		return AT_END;
	
    if(ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER)
    {
        connList->dtls_renegotiate_flag = true;
    }

	
    *prsp_cmd = xy_malloc(48);
    snprintf(*prsp_cmd, 48, "\r\nOK\r\n");
#else
    *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
    return AT_END;
#endif
	return AT_END;
}

/*****************************************************************************
 Function    : AT+QDTLSSTAT？
 Description : 该命令用来查询 DTLS 当前链路状态
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QDTLSSTAT? +QDTLSSTAT:0  ok
 *****************************************************************************/
int at_QDTLSSTAT_req(char *at_buf, char **prsp_cmd)
{
	int data_len = 0;
	char *data = NULL;

	if(g_req_type != AT_CMD_QUERY)
	{
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	    return AT_END;
	}

#if !VER_QUECTEL
	if(is_cdp_running() == 0)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
    }
#endif


#if XY_DTLS
#if !VER_QUECTEL
	if(g_softap_fac_nv->cdp_dtls_switch == 0)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
    }
#endif

	if(g_softap_fac_nv->cdp_dtls_switch == 0 || !cdp_handle_exist() 
		|| (g_phandle == NULL && cdp_register_fail_flag == 0))
	{
		*prsp_cmd = xy_malloc(48);
		snprintf(*prsp_cmd, 48, "\r\n+QDTLSSTAT:1\r\n\r\nOK\r\n");
		return AT_END;
	}

	*prsp_cmd = xy_malloc(48);
	handle_data_t *handle = (handle_data_t *)g_phandle;
	if(g_phandle == NULL && cdp_register_fail_flag == 1)
	{
		snprintf(*prsp_cmd, 48, "\r\n+QDTLSSTAT:3\r\n\r\nOK\r\n");
		return AT_END;
	}
	
    connection_t  *connList = handle->client_data.connList;
	if(connList != NULL)
	{
		mbedtls_ssl_context *ssl = (mbedtls_ssl_context *)connList->net_context;
		if(ssl != NULL && ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER)
		{
			if(connList->dtls_renegotiate_flag == true)
				snprintf(*prsp_cmd, 48, "\r\n+QDTLSSTAT:1\r\n\r\nOK\r\n");
			else
				snprintf(*prsp_cmd, 48, "\r\n+QDTLSSTAT:0\r\n\r\nOK\r\n");
		}
		else
			snprintf(*prsp_cmd, 48, "\r\n+QDTLSSTAT:2\r\n\r\nOK\r\n");
	}
	else if(cdp_register_fail_flag == 1)
		snprintf(*prsp_cmd, 48, "\r\n+QDTLSSTAT:3\r\n\r\nOK\r\n");
	else
	{
		snprintf(*prsp_cmd, 48, "\r\n+QDTLSSTAT:2\r\n\r\nOK\r\n");
	}
	return AT_END;
#else
    *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    return AT_END;
#endif
}

/*****************************************************************************
 Function    : AT+QCRITICALDATA
 Description : 查询是否发送紧急数据
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QCRITICALDATA=1
 *****************************************************************************/
int at_QCRITICALDATA_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	    int state = -1;
		if (at_parse_param("%d", at_buf, &state) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(state == 1)
		{
			if(g_FOTAing_flag || is_cdp_running() == 0)
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	
	return AT_END;
}

/*****************************************************************************
 Function    : AT+QCFG
 Description : 配置平台生命周期、接入平台时的端点名称和绑定模式
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QCFG="LWM2M/lifetime",900
 *****************************************************************************/
#if 0
int at_QCFG_req(char *at_buf, char **prsp_cmd)
{	
	uint8_t  *lwm2m_param_type = xy_zalloc(strlen(at_buf));
	uint8_t *endpoint_Name = NULL;
	if((g_req_type == AT_CMD_QUERY) || (g_req_type == AT_CMD_TEST))
    {
        goto PROC;
    }
	
	if(at_parse_param("%s", at_buf, lwm2m_param_type) != AT_OK) {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto PROC;
	}
	if(at_strcasecmp(lwm2m_param_type,"LWM2M/Lifetime") == 0)
	{
		int lifetime = -1;
		if (at_parse_param(",%d", at_buf, &lifetime) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto PROC;
		}
		
		if(lifetime == -1)
		{
			if(!at_strcasecmp(at_buf,"\"LWM2M/Lifetime\"\r"))
			{
				*prsp_cmd = xy_malloc(48);
#if VER_QUECTEL
				snprintf(*prsp_cmd, 48, "\r\n+QCFG: \"LWM2M/Lifetime\",%ld\r\n\r\nOK\r\n",  g_softap_fac_nv->cdp_lifetime);
#else
				snprintf(*prsp_cmd, 48, "\r\n+QCFG:\"LWM2M/Lifetime\",%ld\r\n\r\nOK\r\n",  g_softap_fac_nv->cdp_lifetime);
#endif
			}
			else
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto PROC;
		}

		if(lifetime >= 0 && lifetime <= 30*LWM2M_DEFAULT_LIFETIME)
		{
#if !VER_QUECTEL
			if (is_cdp_running())    
		    {
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
				goto PROC;
		    }
#endif

#if VER_QUECTEL
			if(lifetime >= 1 && lifetime <= 900)
				g_softap_fac_nv->cdp_lifetime = 900;
#else
			if(lifetime >= 1 && lifetime <= 120)
				g_softap_fac_nv->cdp_lifetime = 120;
#endif
			else
				g_softap_fac_nv->cdp_lifetime = lifetime;
			SAVE_FAC_PARAM(cdp_lifetime);
			xy_printf("set telecom lifetime is %d",g_softap_fac_nv->cdp_lifetime);
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		}
		goto PROC;
	}
	else if(at_strcasecmp(lwm2m_param_type,"LWM2M/EndpointName") == 0)
	{
		endpoint_Name = xy_zalloc(strlen(at_buf));
		if (at_parse_param(",%s", at_buf, endpoint_Name) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto PROC;
		}

		if(strlen((const char *)endpoint_Name) == 0)
		{
			if(!at_strcasecmp(at_buf,"\"LWM2M/EndpointName\"\r"))
			{
				if(g_endpointName == NULL && g_cdp_regInfo == NULL)
				{
					cdp_read_regInfo(OFFSET_NETINFO_PARAM(cdp_regInfo));
					int len = 50 + strlen(g_cdp_regInfo->endpointname);
					*prsp_cmd = xy_zalloc(len);
#if VER_QUECTEL
					snprintf(*prsp_cmd, len, "\r\n+QCFG: \"LWM2M/EndpointName\",\"%s\"\r\n\r\nOK\r\n", g_cdp_regInfo->endpointname);
#else
					snprintf(*prsp_cmd, len, "\r\n+QCFG:\"LWM2M/EndpointName\",\"%s\"\r\n\r\nOK\r\n", g_cdp_regInfo->endpointname);
#endif
				}
				else
				{
					if(g_endpointName != NULL)
					{
						int len = 50 + strlen(g_endpointName);
						*prsp_cmd = xy_zalloc(len);
#if VER_QUECTEL
						snprintf(*prsp_cmd, len, "\r\n+QCFG: \"LWM2M/EndpointName\",\"%s\"\r\n\r\nOK\r\n", g_endpointName);
#else
						snprintf(*prsp_cmd, len, "\r\n+QCFG:\"LWM2M/EndpointName\",\"%s\"\r\n\r\nOK\r\n", g_endpointName);
#endif
						
					}
					else if(g_cdp_regInfo != NULL)
					{
						int len = 50 + strlen(g_cdp_regInfo->endpointname);
						*prsp_cmd = xy_zalloc(len);
#if VER_QUECTEL
						snprintf(*prsp_cmd, len, "\r\n+QCFG: \"LWM2M/EndpointName\",\"%s\"\r\n\r\nOK\r\n", g_cdp_regInfo->endpointname);
#else
						snprintf(*prsp_cmd, len, "\r\n+QCFG:\"LWM2M/EndpointName\",\"%s\"\r\n\r\nOK\r\n", g_cdp_regInfo->endpointname);
#endif
					}
				}
			}
			else
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto PROC;
		}

		if(strlen((const char *)endpoint_Name) > 255)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		}
#if !VER_QUECTEL
		else if (is_cdp_running() || NET_NEED_RECOVERY(CDP_TASK))    
	    {
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto PROC;
	    }
#endif	
		else if(cdp_set_endpoint_name(endpoint_Name) == NULL)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		}
		goto PROC;
	}
	else if(at_strcasecmp(lwm2m_param_type,"LWM2M/BindingMode") == 0)
	{
		int bindingMode = -1;
		if (at_parse_param(",%d", at_buf, &bindingMode) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto PROC;
		}

		if(bindingMode == -1)
		{
			if(!at_strcasecmp(at_buf,"\"LWM2M/BindingMode\"\r"))
			{
				*prsp_cmd = xy_malloc(48);
#if VER_QUECTEL
				snprintf(*prsp_cmd, 48, "\r\n+QCFG: \"LWM2M/BindingMode\",%1d\r\n\r\nOK\r\n", (g_softap_fac_nv->binding_mode == 0)?1:(g_softap_fac_nv->binding_mode));
#else
				snprintf(*prsp_cmd, 48, "\r\n+QCFG:\"LWM2M/BindingMode\",%1d\r\n\r\nOK\r\n", (g_softap_fac_nv->binding_mode == 0)?1:(g_softap_fac_nv->binding_mode));
#endif
			}
			else
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto PROC;
		}

		if(bindingMode == 1 || bindingMode == 2 )
		{
#if !VER_QUECTEL
			if (is_cdp_running())    
		    {
				*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
				goto PROC;
		    }
#endif	
			g_softap_fac_nv->binding_mode = (uint8_t)bindingMode;
			SAVE_FAC_PARAM(binding_mode);
		}
		else
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		}
		goto PROC;
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
PROC:
	if(lwm2m_param_type)
		xy_free(lwm2m_param_type);
	if(endpoint_Name)
		xy_free(endpoint_Name);
	return AT_END;
}
#endif
/*****************************************************************************
 Function    : +QLWFOTAIND
 Description : 该命令用于设置 DFOTA 升级模式,
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QLWFOTAIND=0
               OK
 *****************************************************************************/

int at_QLWFOTAIND_req(char *at_buf, char **prsp_cmd)
{
#ifndef CONFIG_FEATURE_FOTA
	*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
#else
    int type = -1;
	if(g_req_type != AT_CMD_REQ && g_req_type != AT_CMD_QUERY)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}
		
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		return AT_END;
	}

    if(g_req_type == AT_CMD_REQ)
    {
    	if (at_parse_param("%d", at_buf, &type) != AT_OK)
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        return AT_END;
	    }

	    if(type <= -1 || type >5 )
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        return AT_END;
	    }

	    if(g_softap_fac_nv->cdp_dfota_type == 0 && type > 1 && type < 6)
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	        return AT_END;
	    }

	    int ret = atiny_fota_manager_get_state(atiny_fota_manager_get_instance());
	    if((type == 0 || type == 1) && ret != ATINY_FOTA_IDLE)
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	        return AT_END;       
	    }

	    char *pkt_uri = atiny_fota_manager_get_pkg_uri(atiny_fota_manager_get_instance());
	    switch(type)
	    {
	    case 0:     
            if(pkt_uri != NULL && strlen(pkt_uri) != 0)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
                return AT_END;       
            }
            g_softap_fac_nv->cdp_dfota_type = type;
            SAVE_FAC_PARAM(cdp_dfota_type);
	        break;

	   case 1:
            if(pkt_uri != NULL && strlen(pkt_uri) != 0)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
                return AT_END;       
            }

            g_softap_fac_nv->cdp_dfota_type = type;
            SAVE_FAC_PARAM(cdp_dfota_type);
	        break;

	    case 2:
	        if(g_softap_fac_nv->cdp_dfota_type == 1 && ret == ATINY_FOTA_IDLE)
	        {
	            if(pkt_uri == NULL || strlen(pkt_uri) == 0)
	            {
	                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	                return AT_END;       
	            }

	            ret = atiny_fota_manager_start_download(atiny_fota_manager_get_instance());
	            if(ret != XY_OK)
	                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	        }   
	        else
	           *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED); 
	        break;

	    case 3:
	        if(g_softap_fac_nv->cdp_dfota_type == 1 && ret == ATINY_FOTA_IDLE)
	        {
	            if(pkt_uri == NULL || strlen(pkt_uri) == 0)
	            {
	                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	                return AT_END;       
	            }

	            atiny_fota_manager_set_update_result(atiny_fota_manager_get_instance(), ATINY_FIRMWARE_UPDATE_FAIL);
	            ret = atiny_fota_manager_rpt_state(atiny_fota_manager_get_instance(), ATINY_FOTA_IDLE);
	            if(ret != XY_OK)
	                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	            atiny_fota_manager_set_pkg_uri(atiny_fota_manager_get_instance(), NULL, 0);
	        }   
	        else
	           *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED); 
	        break;

	    case 4:
	        if(g_softap_fac_nv->cdp_dfota_type == 1 && ret == ATINY_FOTA_DOWNLOADED)
	        {
	            ret = atiny_fota_manager_execute_update(atiny_fota_manager_get_instance());
	            if(ret != XY_OK)
	                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	        }   
	        else
	           *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED); 
	        break;

	    case 5:
	        if(g_softap_fac_nv->cdp_dfota_type == 1 && ret == ATINY_FOTA_DOWNLOADED)
	        {
	            atiny_fota_manager_set_update_result(atiny_fota_manager_get_instance(), ATINY_FIRMWARE_UPDATE_FAIL);
	            ret = atiny_fota_manager_rpt_state(atiny_fota_manager_get_instance(), ATINY_FOTA_IDLE);
	            if(ret != XY_OK)
	                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	            atiny_fota_manager_set_pkg_uri(atiny_fota_manager_get_instance(), NULL, 0);
	        }   
	        else
	           *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED); 
	        break;

	    default:
	        break;
	    }

	    if(cdp_wait_sem != NULL)
	        osSemaphoreRelease(cdp_wait_sem);  
    }
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
		 snprintf(*prsp_cmd, 40, "\r\n+QLWFOTAIND:%d\r\n\r\nOK\r\n",g_softap_fac_nv->cdp_dfota_type);
	}
#endif
    return AT_END;
}


#endif

