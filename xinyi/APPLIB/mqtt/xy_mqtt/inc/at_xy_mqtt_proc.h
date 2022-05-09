#pragma once

#include "MQTTClient.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define MQTT_CONTEXT_NUM_MAX           (5)

/* default configuration */
#define MQTT_PROTOCOL_VERSION_DEFAULT    (0)
#define MQTT_KEEPALIVE_DEFAULT           (120)
#define MQTT_SESSION_DEFAULT             (1)
#define MQTT_WILLFLAG_DEFAULT            (0)

#define MQTT_SHOWFLAG_DEFAULT            (0)
#define MQTT_ECHOMODE_DEFAULT            (1)
#define MQTT_SEND_FORMAT_DEFAULT         (0)
#define MQTT_RECV_FORMAT_DEFAULT         (0)

#define MQTT_PKT_TIMEOUT_DEFAULT      (30)
#define MQTT_RETRY_TIMES_DEFAULT      (0)
#define MQTT_TIMEOUT_NOTICE_DEFAULT   (0)
#define MQTT_PORT_DEFAULT             (1883)

#define MQTT_TX_BUF_DEFAULT           (4+16+256+1460)    /*header+variable header+topic+payload*/
#define MQTT_RX_BUF_DEFAULT           (4+16+256+1460)    /*header+variable header+topic+payload*/
#define MQTT_YIELD_TIMEOUT_MS_DEFAULT (2000)
#define MQTT_TCP_CONNECT_ID_DEFAULT   (0xff)
#define MQTT_CLOUD_TYPE_DEFAULT       (0xff)

#define MQTT_MAX_SUBSCRIBE_NUM        (4)                /*最大可订阅取消订阅的数目*/

#define MQTT_PASSTHR_COULD_MAX_LEN    (1460)
/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/	
typedef struct {
	int tcpconnectID;
	int msgID;
	int qos;
	int retain;
	char *topic;
	void *pContext;
}publish_params_t;

typedef struct {
	char *product_key; 
    char *device_name; 
    char *device_secret; 
}aliauth_t;  

typedef struct {
	int send_data_format;
    int recv_data_format;
}DataFormat_t;

typedef enum
{	
	CLOUD_TYPE_DEFAULT, /*need client id, user name, passwd*/
   	CLOUD_TYPE_ALI,
    CLOUD_TYPE_TENCENT,		
}mqtt_cloud_type_e;

typedef enum mqttdatamode
{
	ASCII_STRING = 0,  
	HEX_ASCII_STRING,   //such as: "1234"=>1852(2 bytes)
	BIT_STREAM,
} mqttdata_mode_e;

typedef enum {
	MQTT_CONFIG_BASE,
		
	/*Configure the MQTT protocol version*/
	MQTT_CONFIG_VERSION,
	
	/*Configure the keep-alive time*/
	MQTT_CONFIG_KEEPALIVE,
	
	/*Configure the session type*/
	MQTT_CONFIG_SESSION,
	
	/*Configure timeout of message delivery*/
	MQTT_CONFIG_TIMEOUT,
	
	/*Configure Will Information*/
	MQTT_CONFIG_WILL,
	
	/*Configure Alibaba device information for Alibaba Cloud*/
	MQTT_CONFIG_ALIAUTH,

	/*Configure or query whether to display the length of received data*/
	MQTT_CONFIG_SHOWRECVLEN,

	/*Configure or query whether to send data to UART in data mode Echo input data*/
	MQTT_CONFIG_ECHOMODE,

	/*Configure or query the format of sending and receiving data*/
	MQTT_CONFIG_DATAFORMAT,
	
	MQTT_CONFIG_OPEN,
	
	MQTT_CONFIG_MAX,
}mqtt_cfg_type_e;

typedef enum {
    MQTT_CONTEXT_NOT_USED = 0,
	MQTT_CONTEXT_CONFIGED,
	MQTT_CONTEXT_OPENED,
	MQTT_CONTEXT_CONNECTED
}mqtt_context_e;

typedef enum {
	MQTT_STATE_DEFAULT      = 0,
	MQTT_STATE_OPEN		    = 1 << 0,
	MQTT_STATE_CLOSE	    = 1 << 1,
	MQTT_STATE_CONNECT		= 1 << 2,
	MQTT_STATE_DISCONNECT   = 1 << 3,
	MQTT_STATE_SUBSCRIBE	= 1 << 4,
	MQTT_STATE_UNSUBSCRIBE  = 1 << 5,
	MQTT_STATE_PUBLISH      = 1 << 6,
}mqtt_event_state_e;

typedef enum {
	MQTT_OPEN_REQ,
	MQTT_CLOSE_REQ,
	MQTT_CONN_REQ,
	MQTT_DISC_REQ,
	MQTT_SUB_REQ,
	MQTT_UNSUB_REQ,
	MQTT_PUB_REQ,  
	MQTT_PUB_REC, 
    MQTT_PUB_REL, 
    MQTT_PUB_ACK,
    MQTT_PUB_COMP, 
	MQTT_KEEPALIVE_REQ,
	MQTT_TCP_DISCONN_UNEXPECTED_REQ,
	MQTT_REQ_MAX,		
}mqtt_req_type_e;

typedef struct {
	int   req_type;
	int   tcpconnectID;
	int   msg_id;
	int   server_ack_mode;
	int   retained;
	int   message_len;
	void *pContext;
    char *message;
	int   count;   //用于记录实际订阅/取消订阅的主题的数目
	int   qos[MQTT_MAX_SUBSCRIBE_NUM];
	char *topicFilters[MQTT_MAX_SUBSCRIBE_NUM];
}mqtt_req_param_t;

typedef struct {
	char* host;
	int port;
}mqtt_addrinfo_param_t;

typedef struct {
	int pkt_timeout;
	int retry_times;
	int timeout_notice;
}mqtt_timeout_param_t;

typedef struct {
	unsigned char willFlag;
	MQTTPacket_willOptions will;
}mqtt_will_param_t;

typedef struct {
	int tcpconnectID;

	int is_used; 
	int state;  
	
	int cloud_type;
	
	Network* ipstack;

	int sendbuf_size;
    int readbuf_size;
    char *sendbuf;
    char *readbuf;

	int show_flag;
	int echo_mode;
	
	mqtt_addrinfo_param_t addrinfo_data;

	mqtt_timeout_param_t timeout_data;
	
	MQTTPacket_connectData mqtt_conn_data;

	aliauth_t aliauth_data;

	DataFormat_t data_format;

	MQTTClient *mqtt_client;
}mqtt_context_t;   

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
mqtt_context_t *mqttFindContextBytcpid(int tcpconnectID);
mqtt_context_t *mqttCreateContext(int tcpconnectID, char *mqttUri, int mqttPort, int txBufLen, int rxBufLen, int mode);
int mqttDeleteContext(mqtt_context_t *mqttContext);
int mqttConfigContext(int tcpconnectID, int cfgType, void* cfgData);

int mqtt_client_config(int tcpconnectID, int cfgType, void* cfgData);
int mqtt_client_open(int tcpconnectID, char *mqttUri, int mqttPort);
int mqtt_client_close(int tcpconnectID);
int mqtt_client_connect(int tcpconnectID, char *clientId, char* userName, char* passWord);
int mqtt_client_disconnect(int tcpconnectID);
int mqtt_client_subscribe(int tcpconnectID, int msgId, int count, char *topicFilters[], int requestedQoSs[]);
int mqtt_client_unsubscribe(int tcpconnectID, int msgId, int count, char* topicFilters[]);
int mqtt_client_publish(int tcpconnectID, int msgId, int qos, int retained, char* mqttPubTopic, int message_len, char* mqttMessage);
int mqtt_client_publish_passthr_proc(char* buf, uint32_t len, void *param);

