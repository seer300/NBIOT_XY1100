/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include <coap.h>
#include "lwip/sockets.h"
#include "xy_utils.h"
#include "at_global_def.h"
#include "xy_at_api.h"
#include "xy_coap_api.h"

extern bool xy_coap_messgae_process_can_quit(coap_context_t *ctx);
extern void xy_coap_clear_option();
extern coap_context_t *xy_coap_client_context_init(void);
extern int xy_coap_create_socket(coap_context_t  *ctx,char *ipaddr,unsigned short port);

typedef struct CoapOptionData
{
    int optNum;
    unsigned char *optValue;
} CoapOptionData;

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
coapClientConf coapClientCfg = {0};

osThreadId_t g_coaprecvpacket_handle = NULL;
int g_stop_recv_packet_flag = 0;

/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/

/*******************************************************************************************
 Function    : coap_recv_packet_task
 Description : receive COAP packet function task
 Input       : void
 Return      : void
 *******************************************************************************************/
void coap_recv_packet_task()
{
    softap_printf(USER_LOG, WARN_LOG,"[COAP] recv packet thread begin\n");
    //wait PDP active
    if(xy_wait_tcpip_ok(2*60) == XY_ERR)
        xy_assert(0);

    xy_coap_asy_receive(coapClientCfg.coap_client,&g_stop_recv_packet_flag);

	g_coaprecvpacket_handle = NULL;
	osThreadExit();

    softap_printf(USER_LOG, WARN_LOG,"[COAP] recv packet thread end\n");
}

/*******************************************************************************************
 Function    : coap_task_create
 Description : create recv COAP packet task
 Input       : void
 Return      : void
 *******************************************************************************************/
void coap_task_create()
{
	osThreadAttr_t thread_attr = {0};

    if (g_coaprecvpacket_handle != NULL)
        return;
	thread_attr.name       = "coap_recv_packet";
	thread_attr.priority   = XY_OS_PRIO_NORMAL1;
	thread_attr.stack_size = 0x1000;
    g_coaprecvpacket_handle = osThreadNew((osThreadFunc_t)(coap_recv_packet_task), NULL, &thread_attr);
}

/*****************************************************************************
 Function    : at_COAPCREATE_req
 Description : create COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPCREATE=<server>,<port>
 *****************************************************************************/
int at_COAPCREATE_req(char *at_buf, char **prsp_cmd)
{
    unsigned short port = 0;
    char * remote_ip    = xy_zalloc(strlen(at_buf));
    void *p[] = {remote_ip,&port};

    softap_printf(USER_LOG, WARN_LOG,"[COAP] CREATE BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%s,%d", at_buf, p) != AT_OK || !strcmp(remote_ip,"") || port < 1 ||port > 65535 || coapClientCfg.coap_client != NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    coapClientCfg.coap_client = xy_coap_client_create(remote_ip,port);

    if (coapClientCfg.coap_client == NULL ) /*create failed*/
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    coap_task_create();
    softap_printf(USER_LOG, WARN_LOG,"[COAP] CREATE END\n");

ERR_PROC:
    if(remote_ip)
        xy_free(remote_ip);
    return AT_END;
}

/*****************************************************************************
 Function    : at_COAPDEL_req
 Description : delete COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPDEL
 *****************************************************************************/
int at_COAPDEL_req(char *at_buf, char **prsp_cmd)
{
    int  time = 0;

    (void) at_buf;

    softap_printf(USER_LOG, WARN_LOG,"[COAP] DEL BEGIN\n");

    if(g_req_type != AT_CMD_ACTIVE)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

	if(coapClientCfg.coap_client == NULL)
    {
        goto ERR_PROC;
    }

    if(!xy_coap_messgae_process_can_quit(coapClientCfg.coap_client))
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    g_stop_recv_packet_flag = 1;//stop recv thread
    while(g_coaprecvpacket_handle != NULL)
    {
        time +=200;
        osDelay(200);
        if (time > 2000)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            goto ERR_PROC;
        }

    }
    xy_coap_clear_option();
    xy_coap_client_delete(coapClientCfg.coap_client);
    coapClientCfg.coap_client = NULL;
    g_stop_recv_packet_flag = 0;
    softap_printf(USER_LOG, WARN_LOG,"[COAP] DEL END\n");
ERR_PROC:

    return AT_END;
}

/*****************************************************************************
 Function    : at_COAPHEAD_req
 Description : config COAP head
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPHEAD=<msgid>,<tkl>,<token>
 *****************************************************************************/
int at_COAPHEAD_req(char *at_buf, char **prsp_cmd)
{
    int  msgid = -1;
    int  tkl   = -1;
    char * token = NULL;
    token = xy_zalloc(strlen(at_buf));
    void *p[] = {&msgid,&tkl,token};

    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG HEAD BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%d,%d,%s", at_buf, p) != AT_OK  ||msgid < 0 || msgid > 65535||tkl < 0 || tkl > 8 ||strlen(token) != tkl)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (coapClientCfg.coap_client == NULL )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    xy_config_coap_head(msgid,token,tkl);

    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG HEAD END\n");

ERR_PROC:
    if(token)
        xy_free(token);
    return AT_END;
}

int commaCount(char* buf)
{
    int count = 0;
    while(*buf != '\0')
    {
        if(*buf == ',')
            count++;

        buf++;
    }

    return count;
}

/*****************************************************************************
 Function    : at_COAPOPTION_req
 Description : config COAP options
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPOPTION=<opt_count>,<opt_name>,<opt_value>
 *****************************************************************************/
int at_COAPOPTION_req(char *at_buf, char **prsp_cmd)
{
    int  i;
    int  opt_count = 0;
    char *form =NULL;
    //char* test = ",%d,%c";
    char *format = NULL;
    void *p[] = {&opt_count};

    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG OPTION BEGIN\n");

    if(g_req_type == AT_CMD_REQ) {

        if (at_parse_param_2("%d", at_buf, p) != AT_OK || opt_count <=0 || opt_count > 10 || commaCount(at_buf) != 2*opt_count)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }

		if (coapClientCfg.coap_client == NULL )
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            return AT_END;
        }

        format = xy_zalloc(opt_count*strlen(",%d,%s,"));
        softap_printf(USER_LOG, WARN_LOG,"[COAP] opt_count = %d\n",opt_count);
        /*dynamic parse param*/
        void *param[2*opt_count];
        CoapOptionData option[opt_count];
        memset (&option,0,sizeof(CoapOptionData)*opt_count);
        form = format;
        for(i=0;i <2*opt_count;i+=2)
        {
            softap_printf(USER_LOG, WARN_LOG,"[COAP] i = %d",i);
            param[i] =&option[i/2].optNum;
            option[i/2].optValue = xy_zalloc(strlen(at_buf));
            param[i+1] =option[i/2].optValue;

            *form++ = ',';
            *form++ = '%';
            *form++ = 'd';
            *form++ = ',';
            *form++ = '%';
            *form++ = 's';
        }

        *form++ = '\0';
        if (at_parse_param_2(format, at_buf, param) != AT_OK )
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto ERR_PROC;
        }

        xy_coap_clear_option();

        for(i=0;i <opt_count;i++)
        {
            if(xy_config_coap_option(option[i].optNum,option[i].optValue) != XY_OK)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
                goto ERR_PROC;
            }
        }

        softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG OPTION END\n");

    ERR_PROC:
        for(i=0;i <opt_count;i++)
        {
            if(option[i].optValue)
                xy_free(option[i].optValue);
        }
        if(format)
            xy_free(format);

    }
    else if(g_req_type == AT_CMD_TEST) {
        *prsp_cmd = xy_zalloc(80);
        snprintf(*prsp_cmd, 80, "\r\n+QCOAPOPTION: <opt_count>,<opt_name>,\"<opt_value>\"[,...]\r\n\r\nOK\r\n");
    }
    else {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }

    return AT_END;
}

/*****************************************************************************
 Function    : at_COAPSEND_req
 Description : send COAP messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPSEND=<type>,<method>,<data_len>,<data>
 *****************************************************************************/
int at_COAPSEND_req(char *at_buf, char **prsp_cmd)
{
    int  data_len = 0;
    char *tans_data = NULL;
    char *method = xy_zalloc(strlen(at_buf));
    char *type   = xy_zalloc(strlen(at_buf));
    char *data   = xy_zalloc(strlen(at_buf));
    void *p[] = {type,method,&data_len,data};

    softap_printf(USER_LOG, WARN_LOG,"[COAP] SEND BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%s,%s,%d,%s", at_buf, p) != AT_OK ||strlen(method) > 6||strlen(type) > 3|| data_len > 1000 || data_len < 0 || strlen(data) != data_len * 2)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (coapClientCfg.coap_client == NULL )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    if(data_len != 0)
    {
        tans_data = xy_zalloc(data_len + 1);
        if (hexstr2bytes(data, data_len * 2, tans_data, data_len) == -1)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            goto ERR_PROC;
        }
        tans_data[data_len]= '\0';
        softap_printf(USER_LOG, WARN_LOG,"[COAP] tans_data = %d,%s\n",tans_data,tans_data);
    }

    if (0 != xy_coap_asy_send(coapClientCfg.coap_client, type,method, tans_data,data_len))
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    softap_printf(USER_LOG, WARN_LOG,"[COAP] SEND END\n");

ERR_PROC:
    if(method)
        xy_free(method);
    if(type)
        xy_free(type);
    if(data)
        xy_free(data);
    if(tans_data)
        xy_free(tans_data);
    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPCFG_req
 Description : create COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPCFG="Showra"[,<Showra>]
               +QCOAPCFG="Showrspopt"[,<Showrspopt>]
 *****************************************************************************/
int at_QCOAPCFG_req(char *at_buf, char **prsp_cmd)
{
    int showValue = -1;
    char* showStr = xy_zalloc(strlen(at_buf));

    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG BEGIN\n");

    if(g_req_type == AT_CMD_REQ)
    {
        if (at_parse_param("%s,%d", at_buf, showStr,&showValue) != AT_OK  || !strcmp(showStr,""))
            goto PRAR_ERR_PROC;
		
        if(!strcmp(showStr,"Showra"))
        {
            if(showValue == -1)
            {
                *prsp_cmd = xy_zalloc(50);
                snprintf(*prsp_cmd,50, "\r\n+QCOAPCFG: \"Showra\",%d\r\n\r\nOK\r\n",coapClientCfg.is_show_ipport);
            }
            else if(showValue ==0 || showValue ==1 )
                coapClientCfg.is_show_ipport = showValue;
            else
                goto PRAR_ERR_PROC;

        }
        else if(!strcmp(showStr,"Showrspopt"))
        {
            if(showValue == -1)
            {
                *prsp_cmd = xy_zalloc(50);
                snprintf(*prsp_cmd,50, "\r\n+QCOAPCFG: \"Showrspopt\",%d\r\n\r\nOK\r\n",coapClientCfg.is_show_opt);
            }
            else if(showValue ==0 || showValue ==1 )
                coapClientCfg.is_show_opt = showValue;
            else
                goto PRAR_ERR_PROC;
        }
        else
            goto PRAR_ERR_PROC;

    }
    else if(g_req_type == AT_CMD_QUERY)
    {
        softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG AT_CMD_QUERY\n");
    }
    else if(g_req_type == AT_CMD_TEST)
    {
        softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG AT_CMD_TEST\n");
    }
    else
        goto PRAR_ERR_PROC;

    if(showStr)
        xy_free(showStr);
    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG END\n");
    return AT_END;

PRAR_ERR_PROC:
    if(showStr)
        xy_free(showStr);
    *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    return AT_END;
}


/*****************************************************************************
 Function    : at_QCOAPADDRES_req
 Description : config COAP head
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPADDRES=<length>,"<resource>"
 *****************************************************************************/
int at_QCOAPADDRES_req(char *at_buf, char **prsp_cmd)
{
    uint8_t length   = -1;
    char* resource = xy_zalloc(strlen(at_buf));
    coap_resource_t *coap_resource = NULL;

    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG RES BEGIN\n");

    if(g_req_type == AT_CMD_REQ)
    {
        if (at_parse_param("%1d,%s", at_buf, &length,resource) != AT_OK || length > 128||strlen(resource) != length)
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        goto ERR_PROC;
	    }

		if (coapClientCfg.coap_client == NULL )
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
	        goto ERR_PROC;
	    }
		
	    coap_resource = coap_resource_init(coap_make_str_const(resource), 0);
	    coap_add_attr(coap_resource, coap_make_str_const(resource), coap_make_str_const(resource), 0);
	    coap_add_resource(coapClientCfg.coap_client, coap_resource);

	    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG RES END\n");
		
    }
	else if(g_req_type == AT_CMD_TEST) {

		*prsp_cmd = xy_zalloc(60);
		snprintf(*prsp_cmd, 60, "\r\n+QCOAPADDRES: <1-128>,\"<resource>\"\r\n\r\nOK\r\n");
	}
	else {

		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
	}

ERR_PROC:
    if(resource)
        xy_free(resource);
    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPDATASTATUS_req
 Description : send COAP messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QCOAPDATASTATUS?
 *****************************************************************************/
int at_QCOAPDATASTATUS_req(char *at_buf, char **prsp_cmd)
{
    uint8_t send_status = 0;

    if(g_req_type != AT_CMD_QUERY)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

	*prsp_cmd = xy_zalloc(50);
    if (coapClientCfg.coap_client == NULL )
    {
        snprintf(*prsp_cmd,50, "\r\n+QCOAPDATASTATUS:%d\r\n\r\nOK\r\n",send_status);
        return AT_END;
    }

    xy_coap_client_get_data_status(coapClientCfg.coap_client,&send_status);
    snprintf(*prsp_cmd,50, "\r\n+QCOAPDATASTATUS:%d\r\n\r\nOK\r\n",send_status);

    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPCREATE_req
 Description : create COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPCREATE=<local_port>
 *****************************************************************************/
int at_QCOAPCREATE_req(char *at_buf, char **prsp_cmd)
{
	
	uint16_t localPort = 0;

    if(g_req_type == AT_CMD_REQ)
    {
        if (at_parse_param("%2d", at_buf, &localPort) != AT_OK || localPort < 1 ||localPort > 65535 || coapClientCfg.coap_client != NULL)
	    {
	        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        goto ERR_PROC;
	    }
	    coapClientCfg.local_port = localPort;
	    coapClientCfg.coap_client = xy_coap_client_context_init();
    }
	else if(g_req_type == AT_CMD_TEST) 
	{
		*prsp_cmd = xy_zalloc(50);
		snprintf(*prsp_cmd, 50, "\r\n+QCOAPCREATE:<1-65535>\r\n\r\nOK\r\n");
	}
	else {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
	}
	    
ERR_PROC:
    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPHEAD_req
 Description : config COAP head
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPHEAD=<mode>[,[<msgid>][,<tkl>,<token>]]
 *****************************************************************************/
int at_QCOAPHEAD_req(char *at_buf, char **prsp_cmd)
{
    uint8_t mode = -1;
    int  msgid = -1;
    int  tkl   = -1;
	int  parse_num = 0;
    char * token = xy_zalloc(strlen(at_buf));
    char * tokenStr = NULL;

    softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG HEAD BEGIN\n");

    if(g_req_type == AT_CMD_REQ) {

        parse_num = commaCount(at_buf)+1;
		if (at_parse_param("%1d", at_buf, &mode) != AT_OK  || mode >5 || mode < 1)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			goto ERR_PROC;
		}
		
		if (coapClientCfg.coap_client == NULL )
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            goto ERR_PROC;
        }
		
	   switch(mode)
	   {
		   case 1:
			   //Generate message ID and token values randomly.
				if(parse_num > 1) {
					*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			   		goto ERR_PROC;
				}
			   msgid = 0;
			   tkl = 0;
			   break;
		   case 2:
		   	   //Generate message ID randomly; configure token values.
		       if(at_parse_param(",%d,%s",at_buf,&tkl, token)!= AT_OK || tkl > 8 || tkl < 1 || strlen(token) != tkl*2 || parse_num > 3)
			   {
				   *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   msgid = 0;
			   break;
		   case 3:
			   //Configure message ID ; token value is not needed.
			  if(at_parse_param(",%d",at_buf,&msgid)!= AT_OK || msgid < 0 || msgid > 65535 || parse_num > 2)
			   {
				   *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   tkl = 8;
			   break;
		   case 4:
			   //Configure message ID; generate token values randomly.
			   if(at_parse_param(",%d",at_buf,&msgid)!= AT_OK || msgid < 0 || msgid > 65535 || parse_num > 2)
			   {
				   *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   tkl = 0;
			   break;
		   case 5:
			   //Configure message ID and token values.
		 	   if(at_parse_param(",%d,%d,%s", at_buf, &msgid, &tkl, token) != AT_OK || msgid < 0 
			   	|| msgid > 65535 || tkl > 8 || tkl < 1 || strlen(token) != tkl*2)
			   {
				   *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   break;
	   }
	   if(tkl != 0 && strlen(token) > 0)
	   {
		   tokenStr = xy_zalloc(tkl);
		   hexstr2bytes(token, tkl * 2, tokenStr, tkl);
	   }
	
	   xy_config_coap_head(msgid,tokenStr,tkl);
	
	   softap_printf(USER_LOG, WARN_LOG,"[COAP] CONFIG HEAD END\n");

	}
	else if(g_req_type == AT_CMD_TEST) {

		*prsp_cmd = xy_zalloc(70);
		snprintf(*prsp_cmd, 70, "\r\n+QCOAPHEAD: <mode>[,[<msgid>][,<tkl>,<token>]]\r\n\r\nOK\r\n");

	}
    else {

		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

   
ERR_PROC:
    if(token)
        xy_free(token);
    if(tokenStr)
        xy_free(tokenStr);
    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPSEND_req
 Description : send COAP messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPSEND=<type>,<method/rspcode>,<ip_addr>,<port>,<data_len>,<data>
 *****************************************************************************/
int at_QCOAPSEND_req(char *at_buf, char **prsp_cmd)
{
    uint8_t type = -1;
    uint8_t method = -1;
    uint16_t port = -1;
    int  data_len = 0;
    char *tans_data = NULL;
    char *data = xy_zalloc(strlen(at_buf));
    char *remote_ip = xy_zalloc(strlen(at_buf));
    void *p[] = {&type,&method,remote_ip,&port,&data_len,data};

    softap_printf(USER_LOG, WARN_LOG,"[COAP] SEND BEGIN\n");

    if(g_req_type == AT_CMD_REQ) {
		
		if (!ps_netif_is_ok()) {
			   *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			   goto ERR_PROC;
		}
		
	   if (at_parse_param_2("%1d,%1d,%s,%2d,%d,%s", at_buf, p) != AT_OK ||method > 505||type > 3|| data_len > 1000 || data_len < 0 || strlen(data) != data_len * 2
			   || !strcmp(remote_ip,"") || port < 1 ||port > 65535 ) {
		   *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		   goto ERR_PROC;
	   }
	
       if (coapClientCfg.coap_client == NULL )
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            goto ERR_PROC;
        }

		if(g_coaprecvpacket_handle == NULL) {
		    memcpy(coapClientCfg.server_ip,remote_ip,sizeof(coapClientCfg.server_ip));
			xy_coap_create_socket(coapClientCfg.coap_client,remote_ip,port);
			coap_task_create();
		}
		else if(memcmp(coapClientCfg.server_ip,remote_ip,sizeof(coapClientCfg.server_ip)) != 0)
		{
		    xy_coap_clear_option();
		    coap_session_release( coapClientCfg.coap_client->sessions );
            memcpy(coapClientCfg.server_ip,remote_ip,sizeof(coapClientCfg.server_ip));
            xy_coap_create_socket(coapClientCfg.coap_client,remote_ip,port);
		}
		
	   if(data_len != 0)
        {
           tans_data = xy_zalloc(data_len + 1);
           if (hexstr2bytes(data, data_len * 2, tans_data, data_len) == -1)
           {
               *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
               goto ERR_PROC;
           }
           tans_data[data_len]= '\0';
           softap_printf(USER_LOG, WARN_LOG,"[COAP] tans_data = %d,%s\n",tans_data,tans_data);
        }
	
	   if (0 != xy_coap_pkt_send_ec(coapClientCfg.coap_client, type,method, tans_data,data_len,0))
        {
           *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
           goto ERR_PROC;
        }
	
	   softap_printf(USER_LOG, WARN_LOG,"[COAP] SEND END\n");
	}
	else if(g_req_type == AT_CMD_TEST) {

		*prsp_cmd = xy_zalloc(90);
		snprintf(*prsp_cmd, 90, "\r\n+QCOAPSEND: <type>,<method/rspcode>,<ip_addr>,<port>[,<length>,<data>]\r\n\r\nOK\r\n");
	}
	else {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }
   
ERR_PROC:
    if(remote_ip)
        xy_free(remote_ip);
    if(data)
        xy_free(data);
    if(tans_data)
        xy_free(tans_data);
    return AT_END;
}

static uint16_t  s_coap_inited = 0;

void at_coap_init(void)
{
    if(!s_coap_inited)
    {
        s_coap_inited = 1;
    }
    return;
}
