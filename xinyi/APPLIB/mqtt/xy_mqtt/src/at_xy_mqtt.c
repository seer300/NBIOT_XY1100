#include "xy_utils.h"
#include "at_global_def.h"
#include "xy_at_api.h" 
#include "at_xy_mqtt.h"
#include "at_xy_mqtt_proc.h"
#include "xy_passthrough.h"

/*******************************************************************************************
 Function    : at_QMTCFG_req
 Description : Configure Optional Parameters of MQTT
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 *******************************************************************************************/

/********** add by cjh for bc260y 20221119 **********/
#if VER_QUCTL260
extern osTimerId_t passthr_timer;
extern passthr_timeout_callback(uint16_t *timer);
#endif
/******add end******/
int at_QMTCFG_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;
		char *cfgType = xy_malloc(strlen(at_buf));
		mqtt_context_t *mqttCurContext = NULL;
			
		if (at_parse_param("%s,%d", at_buf, cfgType, &tcpconnectID) != AT_OK){
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (!at_strcasecmp(cfgType, "version")) {
			int vsn = -1;
			
			if (at_parse_param(",,%d", at_buf, &vsn) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;	
			}

			if (vsn == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", (mqttCurContext->mqtt_conn_data.MQTTVersion - 3));
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", MQTT_PROTOCOL_VERSION_DEFAULT);
				}
			}
			else {
				if ((vsn != 0) && (vsn != 1)) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_VERSION, (void *)&vsn);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
			}
		}
		else if (!at_strcasecmp(cfgType, "keepalive")) {
			int keepalivetime = -1;

			if (at_parse_param(",,%d", at_buf, &keepalivetime) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;	
			}

			if (keepalivetime == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", mqttCurContext->mqtt_conn_data.keepAliveInterval);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", MQTT_KEEPALIVE_DEFAULT);
				}
			}
			else {
				if (keepalivetime < 0 || keepalivetime > 3600) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_KEEPALIVE, (void *)&keepalivetime);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
			}
		}
		else if (!at_strcasecmp(cfgType, "session")) {
			int clean_session = -1;

			if (at_parse_param(",,%d", at_buf, &clean_session) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;	
			}

			if (clean_session == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", mqttCurContext->mqtt_conn_data.cleansession);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", MQTT_SESSION_DEFAULT);
				}
			}
			else {
				if ((clean_session != 0) && (clean_session != 1)) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_SESSION, (void *)&clean_session);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
			}	
		}
		else if (!at_strcasecmp(cfgType, "timeout")) {
			int pkt_timeout = -1;
			int retry_times = -1;
		    int timeout_notice = -1;
			mqtt_timeout_param_t timeout_data = {0};

			if (at_parse_param(",,%d,%d,%d", at_buf, &pkt_timeout, &retry_times, &timeout_notice) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;	
			}

			if (pkt_timeout == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d,%d,%d\r\n\r\nOK\r\n", mqttCurContext->timeout_data.pkt_timeout, mqttCurContext->timeout_data.retry_times, mqttCurContext->timeout_data.timeout_notice);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d,%d,%d\r\n\r\nOK\r\n", MQTT_PKT_TIMEOUT_DEFAULT, MQTT_RETRY_TIMES_DEFAULT, MQTT_TIMEOUT_NOTICE_DEFAULT);
				}
			}
			else {
				if (pkt_timeout < 1 || pkt_timeout > 60 || retry_times < 0 || retry_times > 10 || timeout_notice < 0 || timeout_notice > 1) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}

				timeout_data.pkt_timeout = pkt_timeout;
				timeout_data.retry_times = retry_times;
				timeout_data.timeout_notice = timeout_notice;
				
				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_TIMEOUT, (void *)&timeout_data);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
			}	
		}
		else if (!at_strcasecmp(cfgType, "will")) { 
			int will_fg = -1;
			int will_qos = -1;
		    int will_retain = -1;
			int will_msg_len = -1;
			char *will_topic = xy_malloc(strlen(at_buf));
			char *will_msg = xy_malloc(strlen(at_buf));
			mqtt_will_param_t will_data = {0};
			
			memset(will_topic, 0, strlen(at_buf));
			memset(will_msg, 0, strlen(at_buf));

			if (at_parse_param(",,%d,%d,%d,%s,%s", at_buf, &will_fg, &will_qos, &will_retain, will_topic, will_msg) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto will_error;	
			}

			if (will_fg == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					if (mqttCurContext->mqtt_conn_data.willFlag == 1) {
						if (mqttCurContext->mqtt_conn_data.will.message.cstring != NULL) {
							*prsp_cmd = xy_malloc(64 + strlen(mqttCurContext->mqtt_conn_data.will.topicName.cstring) + strlen(mqttCurContext->mqtt_conn_data.will.message.cstring));
                			snprintf(*prsp_cmd, 64 + strlen(mqttCurContext->mqtt_conn_data.will.topicName.cstring) + strlen(mqttCurContext->mqtt_conn_data.will.message.cstring), "\r\n+QMTCFG: %d,%d,%d,\"%s\",\"%s\"\r\n\r\nOK\r\n", mqttCurContext->mqtt_conn_data.willFlag, mqttCurContext->mqtt_conn_data.will.qos, mqttCurContext->mqtt_conn_data.will.retained, mqttCurContext->mqtt_conn_data.will.topicName.cstring, mqttCurContext->mqtt_conn_data.will.message.cstring);
						}
						else {
							*prsp_cmd = xy_malloc(64 + strlen(mqttCurContext->mqtt_conn_data.will.topicName.cstring));
                			snprintf(*prsp_cmd, 64 + strlen(mqttCurContext->mqtt_conn_data.will.topicName.cstring), "\r\n+QMTCFG: %d,%d,%d,\"%s\",\"\"\r\n\r\nOK\r\n", mqttCurContext->mqtt_conn_data.willFlag, mqttCurContext->mqtt_conn_data.will.qos, mqttCurContext->mqtt_conn_data.will.retained, mqttCurContext->mqtt_conn_data.will.topicName.cstring);
						}
					}
					else {
						*prsp_cmd = xy_malloc(64);
                		snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", 0);
					}
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", MQTT_WILLFLAG_DEFAULT);
				}
			}
			else {
				if ((will_fg != 0) && (will_fg != 1)) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto will_error;
				}
				
				will_data.willFlag = will_fg;
				if (will_fg == 1) {
					if (will_qos < 0 || will_qos > 2 || will_retain < 0 || will_retain > 1 || strlen(will_topic) < 1 || strlen(will_topic) > 256 || strlen(will_msg) > 256) {
						*prsp_cmd = BC26_AT_ERR_BUILD();
						goto will_error;
					}
					
					will_data.will.qos = will_qos;
					will_data.will.retained = will_retain;
					will_data.will.topicName.cstring = will_topic;
					will_data.will.message.cstring = will_msg;
				}

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_WILL, (void *)&will_data);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto will_error;
				}
			}	

		will_error:
			if (will_topic != NULL) {
				xy_free(will_topic);
			}
			if (will_msg != NULL) {
				xy_free(will_msg);
			}
		}
		else if (!at_strcasecmp(cfgType, "aliauth")) { 
			char* product_key = xy_malloc(strlen(at_buf));
			char* device_name = xy_malloc(strlen(at_buf));
			char* device_secret = xy_malloc(strlen(at_buf));
			aliauth_t aliauth_data = {0};

			memset(product_key, 0, strlen(at_buf));
			memset(device_name, 0, strlen(at_buf));
			memset(device_secret, 0, strlen(at_buf));

			if (at_parse_param(",,%s,%s,%s", at_buf, product_key, device_name, device_secret) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto aliauth_error;	
			}

			if (!strlen(product_key)) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					if (mqttCurContext->aliauth_data.product_key != NULL) {
						*prsp_cmd = xy_malloc(64 + strlen(mqttCurContext->aliauth_data.product_key) + strlen(mqttCurContext->aliauth_data.device_name) + strlen(mqttCurContext->aliauth_data.device_secret));
                		snprintf(*prsp_cmd, 64 + strlen(mqttCurContext->aliauth_data.product_key) + strlen(mqttCurContext->aliauth_data.device_name) + strlen(mqttCurContext->aliauth_data.device_secret), "\r\n+QMTCFG: \"%s\",\"%s\",\"%s\"\r\n\r\nOK\r\n", mqttCurContext->aliauth_data.product_key, mqttCurContext->aliauth_data.device_name, mqttCurContext->aliauth_data.device_secret);
					}
				}
			}
			else {
					if (strlen(product_key) == 0 || strlen(device_name) == 0 || strlen(device_secret) == 0) {
						*prsp_cmd = BC26_AT_ERR_BUILD();
						goto aliauth_error;

					}
					aliauth_data.product_key = product_key; 
					aliauth_data.device_name = device_name;
					aliauth_data.device_secret = device_secret;
					
					res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_ALIAUTH, (void *)&aliauth_data);
					if (res != XY_OK) {
						*prsp_cmd = BC26_AT_ERR_BUILD();
						goto aliauth_error;
					}
			}
	
		aliauth_error:
			if (product_key != NULL) {
				xy_free(product_key);
			}
			if (device_name != NULL) {
				xy_free(device_name);
			}
			if (device_secret != NULL) {
				xy_free(device_secret);
			}
		}
		else if (!at_strcasecmp(cfgType, "showrecvlen")) {
			int show_flag = -1;

			if (at_parse_param(",,%d", at_buf, &show_flag) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;	
			}

			if (show_flag == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", mqttCurContext->show_flag);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", MQTT_SHOWFLAG_DEFAULT);
				}
			}
			else {
				if ((show_flag != 0) && (show_flag != 1)) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_SHOWRECVLEN, (void *)&show_flag);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
			}	
		}
		else if (!at_strcasecmp(cfgType, "echomode")) {
			int echo_mode = -1;

			if (at_parse_param(",,%d", at_buf, &echo_mode) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;	
			}

			if (echo_mode == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", mqttCurContext->echo_mode);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d\r\n\r\nOK\r\n", MQTT_ECHOMODE_DEFAULT);
				}
			}
			else {
				if ((echo_mode != 0) && (echo_mode != 1)) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_ECHOMODE, (void *)&echo_mode);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
			}	
		
		}
		else if (!at_strcasecmp(cfgType, "dataformat")) {
			int send_format = -1;
			int recv_format = -1;
			DataFormat_t data_format = {0};

			if (at_parse_param(",,%d,%d", at_buf, &send_format, &recv_format) != AT_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;	
			}

			if (send_format == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d,%d\r\n\r\nOK\r\n", mqttCurContext->data_format.send_data_format, mqttCurContext->data_format.recv_data_format);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\r\n+QMTCFG: %d,%d\r\n\r\nOK\r\n", MQTT_SEND_FORMAT_DEFAULT, MQTT_RECV_FORMAT_DEFAULT);
				}
			}
			else {
				if (send_format < 0 || send_format > 1 || recv_format < 0 || recv_format > 1) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}

				data_format.send_data_format = send_format;
				data_format.recv_data_format = recv_format;

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_DATAFORMAT, (void *)&data_format);
				if (res != XY_OK) {
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
			}	
		}
		else {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
	exit:
		if (cfgType != NULL) {
			xy_free(cfgType);
		}
		return AT_END;	
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(400);
		snprintf(*prsp_cmd, 400, "\r\n+QMTCFG: \"will\",(0-4),(0,1),(0-2),(0,1),<will_topic>,<will_msg>\r\n+QMTCFG: \"timeout\",(0-4),(1-60),(0-10),(0,1)\r\n+QMTCFG: \"session\",(0-4),(0,1)\r\n+QMTCFG: \"keepalive\",(0-4),(0-3600)\r\n+QMTCFG: \"aliauth\",(0-4),<product_key>,<device_name>,<device_secret>\r\n+QMTCFG: \"version\",(0-4),(0,1)\r\n+QMTCFG: \"showrecvlen\",(0-4),(0,1)\r\n+QMTCFG: \"echomode\",(0-4),(0,1)\r\n+QMTCFG: \"dataformat\",(0-4),(0,1),(0,1)\r\n\r\nOK\r\n");
		return AT_END;	
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

/*******************************************************************************************
 Function    : at_QMTOPEN_req
 Description : Open a Network for MQTT Client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QMTOPEN=<tcpconnectID>,"<host_name>",<port>
 *******************************************************************************************/
int at_QMTOPEN_req(char *at_buf, char **prsp_cmd)
{	
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;
		int port = -1;
		char* host_name = xy_malloc(strlen(at_buf));
   
		memset(host_name, 0, strlen(at_buf));

		if (!ps_netif_is_ok()) {
        	*prsp_cmd = BC26_AT_ERR_BUILD();
       		 goto exit;
        }
		
		if (at_parse_param("%d,%s,%d", at_buf, &tcpconnectID, host_name, &port) != AT_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX || port < 0 || port > 65535 || strlen(host_name) <= 0 || strlen(host_name) > 100) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
		res = mqtt_client_open(tcpconnectID, host_name, port);
		if (res != XY_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

	exit:
		xy_free(host_name);
		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY) {
		int idx = 0;
		*prsp_cmd = xy_malloc(138 * MQTT_CONTEXT_NUM_MAX);
		memset(*prsp_cmd, 0, 138 * MQTT_CONTEXT_NUM_MAX);
	
		for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
			mqtt_context_t *mqttCurContext = NULL;
	  		mqttCurContext = mqttFindContextBytcpid(idx);
	
      		if (mqttCurContext != NULL) {
				if (mqttCurContext->addrinfo_data.host != NULL){		
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 138 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTOPEN: %d,\"%s\",%d", mqttCurContext->tcpconnectID, mqttCurContext->addrinfo_data.host, mqttCurContext->addrinfo_data.port);
				}
        	}
		}
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 138 * MQTT_CONTEXT_NUM_MAX, "\r\n\r\nOK\r\n");
		return AT_END;
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(48);
		snprintf(*prsp_cmd, 48, "\r\n+QMTOPEN: (0-4),<host_name>,<1-65535>\r\n\r\nOK\r\n");
		return AT_END;
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

/*******************************************************************************************
 Function    : at_QMTCLOSE_req
 Description : Close a Network for MQTT Client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QMTCLOSE=<tcpconnectID>
 *******************************************************************************************/
int at_QMTCLOSE_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;

		if (!ps_netif_is_ok()) {
        	*prsp_cmd = BC26_AT_ERR_BUILD();
       		 goto exit;
        }
				
		if (at_parse_param("%d", at_buf, &tcpconnectID) != AT_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
		res = mqtt_client_close(tcpconnectID);
		if (res != XY_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
	exit:
		return AT_END;
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+QMTCLOSE: (0-4)\r\n\r\nOK\r\n");
		return AT_END;
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

/*******************************************************************************************
 Function    : at_QMTCONN_req
 Description : Connect a Client to MQTT Server
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QMTCONN=<tcpconnectID>,"<clientID>"[,"<username>"[,"<password>"]]
 *******************************************************************************************/
int at_QMTCONN_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;
		int clientID_len = 0;
		int username_len = 0;
		int password_len = 0;
		char* clientID = xy_malloc(strlen(at_buf));
		char* username = xy_malloc(strlen(at_buf));
		char* password = xy_malloc(strlen(at_buf));

		memset(clientID, 0, strlen(at_buf));
		memset(username, 0, strlen(at_buf));
		memset(password, 0, strlen(at_buf));

		if (!ps_netif_is_ok()) {
        	*prsp_cmd = BC26_AT_ERR_BUILD();
       		 goto exit;
        }
				
		if (at_parse_param("%d,%s,%s,%s", at_buf, &tcpconnectID, clientID, username, password) != AT_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		clientID_len = strlen(clientID);
		username_len = strlen(username);
		password_len = strlen(password);	
		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX || clientID_len == 0) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (username_len == 0 && password_len != 0){
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
		res = mqtt_client_connect(tcpconnectID, (clientID_len != 0 ? (char *)clientID : NULL), (username_len != 0 ? (char *)username : NULL), (password_len != 0 ? (char *)password : NULL));
		if (res != XY_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
	exit:
		if (clientID != NULL) {
			xy_free(clientID);
		}
		if (username != NULL) {
			xy_free(username);
		}
		if (password != NULL) {
			xy_free(password);
		}
		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY) {
		int idx = 0;
		*prsp_cmd = xy_malloc(32 * MQTT_CONTEXT_NUM_MAX);
		 memset(*prsp_cmd, 0, 32 * MQTT_CONTEXT_NUM_MAX);
		
		for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
			mqtt_context_t *mqttCurContext = NULL;
	  		mqttCurContext = mqttFindContextBytcpid(idx);

			if (mqttCurContext != NULL) {
	          	if (((mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) && (mqttCurContext->state & MQTT_STATE_CLOSE)) || ((mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) && (mqttCurContext->state & MQTT_STATE_DISCONNECT)))  {
	              	snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,4", mqttCurContext->tcpconnectID);
	          	}
				else if ((mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) && (!(mqttCurContext->state & MQTT_STATE_CLOSE)) && (!(mqttCurContext->state & MQTT_STATE_DISCONNECT))) {
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,3", mqttCurContext->tcpconnectID);
				}
				else if ((mqttCurContext->is_used == MQTT_CONTEXT_OPENED) && (mqttCurContext->state & MQTT_STATE_CONNECT)) {
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,2", mqttCurContext->tcpconnectID);
				}
	          	else if ((mqttCurContext->is_used == MQTT_CONTEXT_OPENED) && (!(mqttCurContext->state & MQTT_STATE_CONNECT))) {
	              	snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,1", mqttCurContext->tcpconnectID);
	          	}
	        }
		}
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 138 * MQTT_CONTEXT_NUM_MAX, "\r\n\r\nOK\r\n");
		return AT_END;	
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(64);
		snprintf(*prsp_cmd, 64, "\r\n+QMTCONN: (0-4),<clientID>,<username>,<password>\r\n\r\nOK\r\n");
		return AT_END;
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

/*******************************************************************************************
 Function    : at_QMTDISC_req
 Description : Disconnect a Client from MQTT Server
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QMTDISC=<tcpconnectID>
 *******************************************************************************************/
int at_QMTDISC_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;

		if (!ps_netif_is_ok()) {
        	*prsp_cmd = BC26_AT_ERR_BUILD();
       		 goto exit;
        }
				
		if (at_parse_param("%d", at_buf, &tcpconnectID) != AT_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
		res = mqtt_client_disconnect(tcpconnectID);
		if (res != XY_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
	exit:
		return AT_END;
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+QMTDISC: (0-4)\r\n\r\nOK\r\n");
		return AT_END;
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

/*******************************************************************************************
 Function    : at_QMTSUB_req
 Description : Subscribe to Topics
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QMTSUB=<TCP_connectID>,<msgID>,<topic>,<QoS>[,<topic>,<QoS>...]
 *******************************************************************************************/
int at_QMTSUB_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int idx = 0;
		int res = -1;
		int tcpconnectID = -1;
		int msgID = -1;
		int count = 0;
		char *topicFilters[4] = {0};
		int requestedQoSs[4] = {-1, -1, -1, -1};

		for (idx = 0; idx < 4; idx++) {
			topicFilters[idx] = xy_malloc(strlen(at_buf));
			memset(topicFilters[idx], 0, strlen(at_buf));
		}
		
		if (!ps_netif_is_ok()) {  
        	*prsp_cmd = BC26_AT_ERR_BUILD();
       		 goto exit;
        }
		
		if (at_parse_param("%d,%d,%s,%d,%s,%d,%s,%d,%s,%d", at_buf, &tcpconnectID, &msgID, topicFilters[0], &requestedQoSs[0], topicFilters[1], &requestedQoSs[1], topicFilters[2], &requestedQoSs[2], topicFilters[3], &requestedQoSs[3]) != AT_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
  		}

		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX || msgID < 1 ||  msgID > 65535) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		for (idx = 0; idx < 4; idx++) {
			if (strlen(topicFilters[idx]) != 0) {
				if (strlen(topicFilters[idx]) > 256 || requestedQoSs[idx] < 0 || requestedQoSs[idx] > 2) {
					*prsp_cmd = BC26_AT_ERR_BUILD();   
					goto exit;
				}	
				count += 1;
			}
			else {
				break;
			}
		}
		if (count == 0) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
		res = mqtt_client_subscribe(tcpconnectID, msgID, count, topicFilters, requestedQoSs);
		if (res != XY_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
	exit:
		for (idx = 0; idx < 4; idx++) {
			if (topicFilters[idx] != NULL) {
				xy_free(topicFilters[idx]);
			}
		}
		return AT_END;
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "\r\n+QMTSUB: (0-4),(1-65535),<topic>,(0-2),<topic>,(0-2),...\r\n\r\nOK\r\n");
		return AT_END;
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

/*******************************************************************************************
 Function    : at_QMTUNS_req
 Description : Unsubscribe from Topics
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QMTUNS=<TCP_connectID>,<msgID>,<topic>[,<topic>…]
 *******************************************************************************************/
int at_QMTUNS_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int idx = 0;
		int res = -1;
		int tcpconnectID = -1;
		int msgID = -1;
		int count = 0;
		char *topicFilters[4] = {0};

		for (idx = 0; idx < 4; idx++) {
			topicFilters[idx] = xy_malloc(strlen(at_buf));
			memset(topicFilters[idx], 0, strlen(at_buf));
		}

		if (!ps_netif_is_ok()) {
        	*prsp_cmd = BC26_AT_ERR_BUILD();
       		 goto exit;
        }
				
		if (at_parse_param("%d,%d,%s,%s,%s,%s", at_buf, &tcpconnectID, &msgID, topicFilters[0], topicFilters[1], topicFilters[2], topicFilters[3]) != AT_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX || msgID < 1 ||  msgID > 65535) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		for (idx = 0; idx < 4; idx++) {
			if (strlen(topicFilters[idx]) != 0) {
				if (strlen(topicFilters[idx]) > 256) {
					*prsp_cmd = BC26_AT_ERR_BUILD();   
					goto exit;
				}	  
				count += 1; 
			}
			else {
				break;
			}
		}
		if (count == 0) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
		res = mqtt_client_unsubscribe(tcpconnectID, msgID, count, topicFilters);
		if (res != XY_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
	exit:
		for (idx = 0; idx < 4; idx++) {
			if (topicFilters[idx] != NULL) {
				xy_free(topicFilters[idx]);
			}
		}
		return AT_END;
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "\r\n+QMTUNS: (0-4),(1-65535),<topic>,<topic>,...\r\n\r\nOK\r\n");
		return AT_END;
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

#if VER_QUCTL260
#include "at_xy_mqtt_proc.h"
extern mqtt_context_t  mqttContext[MQTT_CONTEXT_NUM_MAX];
int MessageExtract(char *fmt_parm, char *at_str, int ascii_len, char *param_data, int tcpconnectID)
{
	int i = 0;
	char *end_temp;
	char *fmt_temp;
	int douhao_num = 0;
	int str_len = 0;
	end_temp = strstr(fmt_parm, "%s");
	xy_assert(end_temp != NULL);
	xy_assert(strstr(end_temp + 2, "%s") == NULL);
	fmt_temp = fmt_parm;	
	while (fmt_temp < end_temp)
	{
		if (*fmt_temp == ',')
			douhao_num++;
		fmt_temp++;
	}
	while (douhao_num != 0)
	{
		at_str = strchr(at_str, ',');
		if (at_str == NULL)
			return 8001;	//ATERR_PARAM_INVALID
		at_str++;
		douhao_num--;
	}
	end_temp = strchr(at_str, '\r');
	str_len = end_temp - at_str;	
	if(!str_len)
		return 0;	
	if(mqttContext[tcpconnectID].data_format.send_data_format)
		ascii_len+=ascii_len;
	memcpy(param_data, at_str, ascii_len);
	*(param_data + ascii_len) = '\0';	
	return AT_OK;
}
#endif
/*******************************************************************************************
 Function    : at_QMTPUB_req
 Description : Publish Messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QMTPUB=<tcpconnectID>,<msgID>,<qos>,<retain>,"<topic>",<len>
 *******************************************************************************************/
int at_QMTPUB_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;
		int msgID = -1;
		int qos = -1;
		int retain = -1;
		int  message_len = 0;
		mqtt_context_t *mqttCurContext = NULL;
		publish_params_t *publish_data = NULL;
		char *topic = xy_malloc(strlen(at_buf));
		char *message = xy_malloc(strlen(at_buf));

		memset(topic, 0, strlen(at_buf));
		memset(message, 0, strlen(at_buf));
		
		if (!ps_netif_is_ok()) {
        	*prsp_cmd = BC26_AT_ERR_BUILD();
       		 goto exit;
        }
#if VER_QUCTL260
		if(at_strnchr(at_buf, ',', 6) != NULL)
		{
				if (at_parse_param("%d(0-4),%d,%d,%d,%s,%d", at_buf, &tcpconnectID, &msgID, &qos, &retain, topic, &message_len) != AT_OK) 
				{
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
				if(MessageExtract(",,,,,,%s", at_buf, message_len, message, tcpconnectID) > 0)
				{
					*prsp_cmd = BC26_AT_ERR_BUILD();
					goto exit;
				}
		}

#endif
		if (at_parse_param("%d,%d,%d,%d,%s,%d,%s", at_buf, &tcpconnectID, &msgID, &qos, &retain, topic, &message_len, message) != AT_OK) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (tcpconnectID < 0 || tcpconnectID >= MQTT_CONTEXT_NUM_MAX || msgID < 0 ||  msgID > 65535 || qos < 0 || qos > 2 || retain < 0 || retain > 1 || strlen(topic) < 1 ||strlen(topic) > 256 ||message_len < 0 || message_len > 1460) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
   		}

		if (((qos == 0) && (msgID != 0)) || ((qos != 0) && (msgID == 0)) || ((strlen(message) != 0) && (message_len == 0))) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
		if (mqttCurContext == NULL) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (mqttCurContext->state & MQTT_STATE_PUBLISH) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}
		
		if (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED) {
			*prsp_cmd = BC26_AT_ERR_BUILD();
			goto exit;
		}

		if (strlen(message)) {
			res = mqtt_client_publish(tcpconnectID, msgID, qos, retain, topic, message_len, message);
			if (res != XY_OK) {
				*prsp_cmd = BC26_AT_ERR_BUILD();
				goto exit;
			}
		}
		else {
			publish_data = xy_malloc(sizeof(publish_params_t));
			publish_data->tcpconnectID = tcpconnectID;
			publish_data->msgID = msgID;
			publish_data->qos = qos;
			publish_data->retain = retain;
			publish_data->topic = topic;
			publish_data->pContext = (void *)mqttCurContext;
			
			app_passthr_info_t mqtt_passthr = {0};
			mqtt_passthr.app_type = APP_MQTT;
			
			if (mqttCurContext->data_format.send_data_format == HEX_ASCII_STRING) {
				mqtt_passthr.recv_len = message_len * 2;
				mqtt_passthr.max_len = MQTT_PASSTHR_COULD_MAX_LEN *2;
			}
			else {
				mqtt_passthr.recv_len = message_len;
				mqtt_passthr.max_len = MQTT_PASSTHR_COULD_MAX_LEN;
			}
				
			mqtt_passthr.param = (void *)publish_data;
			mqtt_passthr.proc = mqtt_client_publish_passthr_proc;
			
			into_Passthr_Mode(mqtt_passthr);
			xy_free(message);
			send_urc_to_ext("\r\n>\r\n");
#if VER_QUCTL260
/********** add by cjh for bc260y 20221119 **********/
			if (passthr_timer == NULL){
				osTimerAttr_t timer_attr = {0};
				timer_attr.name = "timeout_timer";
            	passthr_timer = osTimerNew((osTimerFunc_t)(passthr_timeout_callback), osTimerOnce, NULL, &timer_attr);
				osTimerStart(passthr_timer, 60*1000);
				}
#endif
/*****add end*******/
			return AT_ASYN;
		}

	exit: 
		if (message != NULL) {
			xy_free(message);
		}
		if (topic != NULL) {
			xy_free(topic);	
		}
		return AT_END;
	}
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "\r\n+QMTPUB: (0-4),(0-65535),(0-2),(0,1),<topic>,(0-1460),<msg>\r\n\r\nOK\r\n");
		return AT_END;
	}
	else {
		*prsp_cmd = BC26_AT_ERR_BUILD();
        return AT_END;
	}
}

extern osMutexId_t mqtt_mutex;
static uint16_t  s_mqtt_inited = 0;
void at_xy_mqtt_init(void)
{
   if(!s_mqtt_inited)
   {
   		cloud_mutex_create(&mqtt_mutex);
        s_mqtt_inited = 1;
   }
   return;
}

