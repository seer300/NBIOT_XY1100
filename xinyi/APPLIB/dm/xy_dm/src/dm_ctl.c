#include "xy_at_api.h"
#include "at_ps_cmd.h"
#include "at_global_def.h"
#include "factory_nv.h"
#include "xy_utils.h"
#include "lwip/netif.h"
#include "oss_nv.h"
#include "xy_system.h"
#include "xy_net_api.h"

#define MAX_FINDSTR_SIZE 30

void dm_print_str(char * buf)
{
	char * str = (char *)xy_zalloc(56);
	int len;
	
	str[55] = '\0';
	softap_printf(USER_LOG, WARN_LOG, "\n");	
	softap_printf(USER_LOG, WARN_LOG, "[DM]");
	for(len = 0; len <= strlen(buf); len+=55){
		strncpy(str, buf + len, 55);
		softap_printf(USER_LOG, WARN_LOG, "%s",str);	
	}
	softap_printf(USER_LOG, WARN_LOG, "\n");	
	xy_free(str);
}

int findstr(char *s_str,char *d_str) 
{
	int i,j,k,count=0;
	char temp[MAX_FINDSTR_SIZE];
	int length=strlen(d_str);
	if(length>=MAX_FINDSTR_SIZE)
	{
		return 0;
	}
	for(i=0;i<strlen(s_str);i++) 
	{
		k=0;
		for(j=i;j<i+length;j++)
		{
			temp[k++]=s_str[j];
		}
		temp[k]='\0';
		if(!strcmp(temp,d_str)) 
		{
			count++;
			break;
		}
	}
	return count;
}

const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char * base64_encode( const unsigned char * bindata, char * base64, int binlength )
{	 
	int i, j;	 
	unsigned char current;	  
	for ( i = 0, j = 0 ; i < binlength ; i += 3 )	 
	{		 
		current = (bindata[i] >> 2) ;		 
		current &= (unsigned char)0x3F; 	   
		base64[j++] = base64char[(int)current]; 	   
		current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ; 	   
		if ( i + 1 >= binlength )		 
		{			 
			base64[j++] = base64char[(int)current]; 		   
			base64[j++] = '=';			  
			base64[j++] = '=';			  
			break;		  
		}		 
		current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );		 
		base64[j++] = base64char[(int)current]; 	   
		current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;		
		if ( i + 2 >= binlength )		 
		{			 
			base64[j++] = base64char[(int)current]; 		   
			base64[j++] = '=';			  
			break;		  
		}		 
		current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );		 
		base64[j++] = base64char[(int)current]; 	   
		current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;		 
		base64[j++] = base64char[(int)current];    
	}	 
	base64[j] = '\0';	 
	return base64;
}

//extern unsigned int g_cmcc_dm_rtcflag;
void dm_netif_event_callback(PsNetifStateChangeEvent event)
{
	xy_get_UICC_TYPE(&g_uicc_type);
	softap_printf(USER_LOG, WARN_LOG, "dm_netif_event_callback, netif up,g_uicc_type=%d\n",g_uicc_type);

	switch(g_uicc_type)
	{
	case UICC_TELECOM: //telecom
		telecom_dm_init();
		break;
#if MOBILE_VER
	case UICC_MOBILE: //mobile
		cmcc_dm_init();
#endif
		break;
	case UICC_UNICOM: //unicom
		unicom_dm_init();
		break;
	default:
		{
			//xy_assert(0);
			NETDOG_AT_STATICS(dbg_dm_count_num++);
			break;
		}
	}
}

int at_XYDMP_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int mode;
	    int interval = 0;
		char *key = xy_zalloc(64);
		char *pwd = xy_zalloc(64);
		int test = 0;
		
		void *p[] = {&mode, &interval, key, pwd, &test};
		if (at_parse_param_2("%d,%d,%64s,%64s,%d", at_buf, p) != AT_OK) {
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}
		if(interval <= 0 || interval > 1440)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}
		if(mode != 0 && mode != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}
		if(test != 0 && test != 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto free_out;
		}

		if(mode == 0)
		{
			g_softap_fac_nv->need_start_dm = 0;
		}
		else
		{
			g_softap_fac_nv->need_start_dm = 1;			
			g_softap_fac_nv->dm_inteval_time = interval;
			
			memset(g_softap_fac_nv->dm_app_key, 0, 64);
			memset(g_softap_fac_nv->dm_app_pwd, 0, 64);
			memcpy(g_softap_fac_nv->dm_app_key, key, 64);
			memcpy(g_softap_fac_nv->dm_app_pwd, pwd, 64);
			g_softap_fac_nv->dm_serv_flag = test;
		}
		SAVE_SOFTAP_FAC();
free_out:
		xy_free(key);
		xy_free(pwd);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_zalloc(100);
		snprintf(*prsp_cmd, 100, "\r\n+XYDMP:%d,%d,%s,%s,%d\r\n\r\nOK\r\n",
			g_softap_fac_nv->need_start_dm, g_softap_fac_nv->dm_inteval_time, 
			g_softap_fac_nv->dm_app_key, g_softap_fac_nv->dm_app_pwd,
			g_softap_fac_nv->dm_serv_flag);
	}
    else
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
	return AT_END;
}

int at_QSREGENABLE_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	    int type = -1;
	    if (at_parse_param("%d", at_buf, &type) != AT_OK) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
        
	    if (type >=0 && type < 2)
	    { 	
			g_softap_fac_nv->need_start_dm = type;
			SAVE_FAC_PARAM(need_start_dm);	
	    }
	    else
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	    }
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "\r\n+QSREGENABLE:%d\r\n\r\nOK\r\n", g_softap_fac_nv->need_start_dm);
	}
    else
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
	
	return AT_END;
}




void dm_ctl_init(void)
{
	register_app_at_req("AT+XYDMP", at_XYDMP_req);

	if(g_softap_fac_nv->need_start_dm == 1) //dm switch
	{
		xy_regist_psnetif_callback(EVENT_PSNETIF_VALID, dm_netif_event_callback);
	}
}


