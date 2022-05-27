#include "softap_macro.h"
#if VER_QUCTL260
#include "at_cdp_bc26.h"
#include "xy_cdp_api.h"
#include "cdp_backup.h"
#include "agent_tiny_demo.h"
#include "net_app_resume.h"

#include "agenttiny.h"
#include "atiny_context.h"
#include "xy_at_api.h"
#include "softap_nv.h"
#include "oss_nv.h"

extern int deregister_lwm2m_task();
extern int cdp_lifetime;

/*****************************************************************************
 Function    : +NCDPOPEN=<ip_addr>[,<port>]
 Description : Set and query the IP address and port of the CDP server
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NCDPOPEN=<ip_addr>[,<port>[,<psk>]]
               AT+NCDPOPEN=?       OK
 *****************************************************************************/
int at_NCDPOPEN_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	    char *ip_addr_str = xy_zalloc(strlen(at_buf));
		char* psk = xy_zalloc(strlen(at_buf));
        char* psk_temp  = xy_zalloc(strlen(at_buf));
		int port = 5683;

		if (!ps_netif_is_ok()) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			goto free_out;
		}

		if (at_parse_param("%s,%d[0-65535],%s", at_buf, ip_addr_str, &port, psk_temp) != AT_OK) {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}

		if(port == 0)
			port = 5683;
		
		if((strlen(ip_addr_str) <= 0)||(strlen(ip_addr_str) > 40) || (port < 0) || (port > 65535) || (strlen(psk_temp) > 256))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}

		if(set_cdp_server_settings(ip_addr_str, (u16_t)port) < 0)
		{
		   *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		   goto free_out;
		}

		//如果设置了PSK，则说明需要进行加密，只支持DTLS加密
		if(strlen(psk_temp) > 0 && strlen(psk_temp) <= 256)
		{
			if (strlen(psk_temp) != 32 || hexstr2bytes(psk_temp, 32, psk, 16) == -1)
			{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				goto free_out;
			}

			memcpy(g_softap_fac_nv->cloud_server_auth, psk, 16);
			SAVE_FAC_PARAM(cloud_server_auth);

			g_softap_fac_nv->cdp_dtls_switch = 1;
			SAVE_FAC_PARAM(cdp_dtls_switch);

			g_softap_fac_nv->cdp_dtls_nat_type = 0;
			SAVE_FAC_PARAM(cdp_dtls_nat_type);

	    	char *imei_temp = xy_zalloc(16);
	    	xy_get_IMEI(imei_temp, 16);	
			memcpy(g_softap_fac_nv->cdp_pskid, imei_temp, 16);
			SAVE_FAC_PARAM(cdp_pskid);
			xy_free(imei_temp);
				
		}
		else //若没有设置PSK，则使用非加密注册
		{
			g_softap_fac_nv->cdp_dtls_switch = 0;
			SAVE_FAC_PARAM(cdp_dtls_switch);
		}
		
        if(start_cdp_lwm2m() == 0)
        {
        	xy_printf("registed failed!");
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        }	
free_out:
		xy_free(ip_addr_str);
		xy_free(psk);
		xy_free(psk_temp);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(20);
		snprintf(*prsp_cmd, 20, "\r\nOK\r\n");
	}
	else
		prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	return AT_END;
}



/*****************************************************************************
 Function    : +NCDPCLOSE 
 Description : Used for deregistration of the CDP platform
 Input       : None
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NCDPCLOSE
               OK
 *****************************************************************************/
int at_NCDPCLOSE_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;

	if(g_req_type != AT_CMD_ACTIVE)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	if (!ps_netif_is_ok()) 
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		return AT_END;
	}

	resume_net_app(CDP_TASK);

	if(deregister_lwm2m_task() == 0)
	{
		xy_printf("deregisted failed!");
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		
	}
	
    return AT_END;   	
}

/*****************************************************************************
 Function    : +NCFG=<mode>[,<value>] 
 Description : Used for configuration of the CDP platform
 Input       : None
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NCFG=0,86400
               OK
 *****************************************************************************/
int at_NCFG_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int mode = -1;
		int lifetime = -1;
		if (at_parse_param("%d(0-1),%d[0-2592000]", at_buf, &mode, &lifetime) != AT_OK) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(lifetime == -1)
		{
			*prsp_cmd = xy_zalloc(40);
			if(mode == 0)
				snprintf(*prsp_cmd, 40, "\r\n+NCFG: %d\r\n\r\nOK\r\n", g_softap_fac_nv->cdp_lifetime);
			else
				snprintf(*prsp_cmd, 40, "\r\n+NCFG: 0\r\n\r\nOK\r\n");
			return AT_END;
		}

		if(lifetime > 0 && mode == 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		
		if(lifetime > 0 && lifetime <= 900)
			g_softap_fac_nv->cdp_lifetime = 900;
		else
			g_softap_fac_nv->cdp_lifetime = lifetime;
		
		cdp_lifetime = g_softap_fac_nv->cdp_lifetime;
		SAVE_FAC_PARAM(cdp_lifetime);
	}
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_zalloc(60);
		snprintf(*prsp_cmd, 60, "\r\n+NCFG: 0[,(0-2592000)]\r\n+NCFG: 1[,(0-1)]\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

    return AT_END;   	
}
#endif

