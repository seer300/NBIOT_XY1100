/*********************************************************************************
* @file        mqtt_opencpu_demo.c
* @ingroup     cloud
* @brief       使用MQTT API实现与MQTT服务器的通信，提供TCP、MQTT断链回调函数，提供断链重连示例
* @attention
* @note         !!!该源文件默认不编译，用户不得直接在该文件中修改！！！\n
*               而应该是自行创建新的私有的源文件，将demo示例代码拷贝过去后再做修改。
*/
#include "xy_api.h"

#if DEMO_TEST && MQTT
#include "xy_utils.h"
#include "xy_demo.h"
#include "MQTTClient.h"
#include "MQTTTimer.h"

/*authentication information*/
#define MQTT_ClientId       "57950b7ee9f842aab21a16b51c327568"
#define MQTT_UserName       "tr1234567"
#define MQTT_Password       "CK833bttbRSNbHpxdqq0EDtMj4CWn5YnedECF1nblQY"

/*MQTT message param*/
#define MQTT_Topic          "device_control"
#define MQTT_CleanSession   1
#define MQTT_QOS            1

/*will information*/
#define MQTT_Will_Flag      1
#define MQTT_Will_Topic     "device_control"
#define MQTT_Will_Message   "will message"
#define MQTT_Will_Qos       1
#define MQTT_Will_Retained  0

#define MQTT_Command_Timeout 5000
#define MQTT_KeepLive       600
#define MQTT_Sendbuf_Size   200
#define MQTT_Readbuf_Size   200
/*SELECT ERROR CODE*/
#define SELETE_TIMEOUT      -2
#define SELETE_NET_ERR      -1

static osThreadId_t g_mqtttest_handle = NULL;
static osThreadId_t g_mqttsend_handle = NULL;
static osThreadId_t g_mqttdownlink_handle = NULL;
static osSemaphoreId_t mqtt_reconnect_sem = NULL;
static MQTTClient *pMQTT_test_client={NULL};
/**
 * @brief Demo的任务消息队列
 */
osMessageQueueId_t mqtt_demo_sendmsg_q = NULL; //发送消息队列
extern int MQTTSendPacket(MQTTClient* c, int length, Timer* timer);
struct Demo_Options
{
    char* host;         /* server IP or domain name */
    int port;
    char* proxy_host;
    int proxy_port;
    int MQTTVersion;
} network_options =
{
    "mqtt.ctwing.cn",
    1883,
    "mqtt.ctwing.cn",
    1885,
    4,
};

enum MQTT_DEMO_MSG_TYPE
{
    MQTT_DEMO_PUBLISH = 1,
    MQTT_DEMO_KEEPLIVE ,
    MQTT_DEMO_RECONNECT,
    MQTT_DEMO_TCP_RECONNECT,
};

enum MQTT_DEMO_KEEPLIVE_TYPE
{
    MQTT_KEEPLIVE_SUCCESS = 0,
    MQTT_KEEPLIVE_FAILURE = -1,
    MQTT_KEEPLIVE_NEED = -2,
};

typedef struct
{
    int   cmdType;
    int payloadlen;
    void *payload;
}mqttSendMsg_t;

void mqtt_user_defined_process(MessageData* md)
{
    MQTTMessage* m = md->message;

    xy_printf("[MQTTdemo]payload was %s", m->payload);//only show receive MQTT publish message content
}

int mqtt_keepalive_check(MQTTClient* c)
{
    int rc = MQTT_KEEPLIVE_SUCCESS;

    if (c->keepAliveInterval == 0)
        goto exit;

    if (TimerIsExpired(&c->last_sent) || TimerIsExpired(&c->last_received))
    {
        if (c->ping_outstanding)
            rc = MQTT_KEEPLIVE_FAILURE; /* PINGRESP not received in keepalive interval */
        else
        {
            rc = MQTT_KEEPLIVE_NEED;
        }
    }

exit:
    return rc;
}

void mqtt_downlink_buf_recv(MQTTClient *c)
{
    int ret = -1;
    int timeout   = 2000;
    struct timeval tv;
    fd_set read_fds,exceptfds;

    FD_ZERO(&read_fds);
    FD_ZERO(&exceptfds);

    FD_SET(c->ipstack->my_socket, &read_fds);
    FD_SET(c->ipstack->my_socket, &exceptfds);

    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    ret = select(c->ipstack->my_socket+1, &read_fds, NULL, &exceptfds, &tv);

    if (ret == 0)
    {
        xy_printf("[MQTTdemo] select timeout");
        return ;
    }
    else if(ret < 0)
    {
        close(c->ipstack->my_socket);
        c->ipstack->my_socket =-1;
        xy_printf("[MQTTdemo] select error ret=%d,err %d", ret, errno);
        return ;
    }

    if(FD_ISSET(c->ipstack->my_socket, &exceptfds))
    {
        close(c->ipstack->my_socket);
        c->ipstack->my_socket =-1;
        return ;
    }

    if(FD_ISSET(c->ipstack->my_socket, &read_fds))
    {
        ret = recv(c->ipstack->my_socket, c->readbuf, c->readbuf_size, 0);

        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                xy_printf("[MQTTdemo] no data available for now");
                return ;
            }
            else
            {
                xy_printf("[MQTTdemo] error accured when recv: 0x%x", errno);
                close(c->ipstack->my_socket);
                c->ipstack->my_socket =-1;
                return ;
            }
        }
        else if (ret == 0)
        {
            xy_printf("[MQTTdemo] socket was closed by peer");
            close(c->ipstack->my_socket);
            c->ipstack->my_socket =-1;
            return ;
        }
        else
        {
            xy_printf("[MQTTdemo] packet arrived");
            xy_mqtt_downlink_process(pMQTT_test_client,mqtt_user_defined_process,1); //处理下行数据
        }

    }

    return ;
}
void mqtt_downlink_message_process()
{
    int rc = 0;
    mqttSendMsg_t *mqttEventMsg = NULL;
    xy_printf("[MQTTdemo] process begin c=%p socket=%d",pMQTT_test_client->ipstack,pMQTT_test_client->ipstack->my_socket);
    while(pMQTT_test_client->close_sock != 1)
    {
        if(pMQTT_test_client->ipstack->my_socket < 0 )
        {
            mqttEventMsg = xy_zalloc(sizeof(mqttSendMsg_t));
            mqttEventMsg->cmdType = MQTT_DEMO_TCP_RECONNECT;
            osMessageQueuePut(mqtt_demo_sendmsg_q, &mqttEventMsg, 0, osWaitForever);
            if(mqtt_reconnect_sem != NULL)
                osSemaphoreAcquire(mqtt_reconnect_sem, osWaitForever);
        }

        if(pMQTT_test_client->ipstack->my_socket >= 0 )
        {
            rc = mqtt_keepalive_check(pMQTT_test_client);
            if( rc == MQTT_KEEPLIVE_FAILURE)
            {
                if((errno == ECONNABORTED)||(errno == ECONNRESET)||(errno == ENOTCONN)||(errno == EBADE))
                {
                    xy_printf("[MQTTdemo] keeplive error,%d",errno);
                    mqttEventMsg = xy_zalloc(sizeof(mqttSendMsg_t));
                    mqttEventMsg->cmdType = MQTT_DEMO_RECONNECT;
                    osMessageQueuePut(mqtt_demo_sendmsg_q, &mqttEventMsg, 0, osWaitForever);
                    if(mqtt_reconnect_sem != NULL)
                        osSemaphoreAcquire(mqtt_reconnect_sem, osWaitForever);
                }
            }
            else if(rc == MQTT_KEEPLIVE_NEED)
            {
                mqttEventMsg = xy_zalloc(sizeof(mqttSendMsg_t));
                mqttEventMsg->cmdType = MQTT_DEMO_KEEPLIVE;
                osMessageQueuePut(mqtt_demo_sendmsg_q, &mqttEventMsg, 0, osWaitForever);
            }
            //select downlink message
            mqtt_downlink_buf_recv(pMQTT_test_client);
        }
    }

}

void mqtt_downlink_messgae_rev()
{
    xy_printf("[MQTTdemo]downlink thread start soc = %d",pMQTT_test_client->ipstack->my_socket);

    mqtt_downlink_message_process();

    if(mqtt_reconnect_sem != NULL)
        osSemaphoreDelete(mqtt_reconnect_sem);

    g_mqttdownlink_handle = NULL;

    xy_printf("[MQTTdemo]downlink_messgae thread end");
    osThreadExit();

}

void mqtt_downlink_messgae_process_thread_create()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqttdownlink_handle != NULL)
    {
        return;
    }
	thread_attr.name	   = "mqtt_downlink_messgae_process";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0xE00;
    g_mqttdownlink_handle = osThreadNew ((osThreadFunc_t)(mqtt_downlink_messgae_rev),NULL,&thread_attr);
}

void mqtt_connect_param_init(MQTTPacket_connectData *data)
{
    data->MQTTVersion = network_options.MQTTVersion;
    data->clientID.cstring = MQTT_ClientId;
    data->username.cstring = MQTT_UserName;
    data->password.cstring = MQTT_Password;
    data->keepAliveInterval= MQTT_KeepLive;
    data->cleansession     = MQTT_CleanSession;

    data->willFlag               = MQTT_Will_Flag;
    data->will.message.cstring   = MQTT_Will_Message;
    data->will.qos               = MQTT_Will_Qos;
    data->will.retained          = MQTT_Will_Retained;
    data->will.topicName.cstring = MQTT_Will_Topic;
    return;
}

void mqtt_publish_param_init(MQTTMessage* pubmsg)
{
    pubmsg->payload = "abc";              // What to publish
    pubmsg->payloadlen = strlen(pubmsg->payload);
    pubmsg->qos = MQTT_QOS;
    pubmsg->retained = 0;
    pubmsg->dup = 0;
    return;
}

void mqtt_free_demo_memory(MQTTClient* c)
{
    int i;
    if(c == NULL)
        return;
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if(c->messageHandlers[i].topicFilter)
        {
            xy_free((void *)(c->messageHandlers[i].topicFilter));
            c->messageHandlers[i].topicFilter = NULL;
        }
    }
        
    if(c->ipstack)
        xy_free(c->ipstack);
    if(c->buf)
        xy_free(c->buf);
    if(c->readbuf)
        xy_free(c->readbuf);
    
    xy_free(c);
    c = NULL;
    return;
}

static int mqtt_client_tcp_mqtt_reconnect()
{
    memset(pMQTT_test_client->buf, 0, MQTT_Sendbuf_Size);
    memset(pMQTT_test_client->readbuf, 0, MQTT_Readbuf_Size);
    if(NetworkConnect(pMQTT_test_client->ipstack, network_options.host, network_options.port) != XY_OK)
    {
        xy_printf("[MQTTdemo]create socket error");
    }
    else
    {
        if(mqtt_reconnect_sem != NULL)
            osSemaphoreRelease(mqtt_reconnect_sem);
        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        mqtt_connect_param_init(&data);

        /*send MQTT connect message*/
        if(xy_mqtt_connect(pMQTT_test_client, &data,1) != XY_OK)
        {
            xy_printf("[MQTTDEMO] connect failed\r\n");
            return XY_ERR;
        }
    }
    return XY_OK;
}

static int mqtt_client_mqtt_reconnect()
{
    memset(pMQTT_test_client->buf, 0, MQTT_Sendbuf_Size);
    memset(pMQTT_test_client->readbuf, 0, MQTT_Readbuf_Size);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    mqtt_connect_param_init(&data);

    /*send MQTT connect message*/
    if(xy_mqtt_connect(pMQTT_test_client, &data,1) != XY_OK)
    {
        return XY_ERR;
    }
    return XY_OK;
}

static int mqtt_client_connect_subscribe()
{
    int rc = XY_OK;
    char *sendbuf = xy_zalloc(MQTT_Sendbuf_Size);
    char *readbuf = xy_zalloc(MQTT_Readbuf_Size);
    char* test_topic = xy_zalloc(strlen(MQTT_Topic)+1);
    pMQTT_test_client = xy_zalloc(sizeof(MQTTClient));
    pMQTT_test_client->ipstack = xy_zalloc(sizeof(Network));

    strncpy(test_topic,MQTT_Topic,strlen(MQTT_Topic));

    /*network init*/
    NetworkInit(pMQTT_test_client->ipstack);
    NetworkConnect(pMQTT_test_client->ipstack, network_options.host, network_options.port);
    MQTTClientInit((MQTTClient*)(pMQTT_test_client), pMQTT_test_client->ipstack, (unsigned int)(MQTT_Command_Timeout),
            (unsigned char *)(sendbuf), (size_t)(MQTT_Sendbuf_Size), (unsigned char *)(readbuf), (size_t)(MQTT_Readbuf_Size));

    /*Create thread to receive downlink message asynchronously*/
    mqtt_downlink_messgae_process_thread_create();
    xy_printf("[MQTTdemo]Starting mqtt_downlink_thread success");
    osDelay(2000);

    /*Init MQTT connect message Parameters*/
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    mqtt_connect_param_init(&data);

    /*send MQTT connect message*/
    rc = xy_mqtt_connect(pMQTT_test_client, &data,1);
    if(rc != XY_OK)
    {
        xy_printf("[MQTTdemo] connect failed\r\n");
        return rc;
    }
    /*send MQTT subscribe message*/
    rc = xy_mqtt_subscribe(pMQTT_test_client, test_topic, MQTT_QOS,1);

    return rc;
}

void mqtt_demo_tcp_disconnect_cb()
{
    xy_printf("[MQTTdemo]TCP DISCONNECT");

    if(mqtt_client_tcp_mqtt_reconnect() == XY_OK)
        xy_printf("[MQTTdemo] TCP reconnect success\r\n");
    else
        xy_printf("[MQTTdemo] TCP reconnect failed\r\n");
}

void mqtt_demo_mqtt_disconnect_cb()
{
    xy_printf("[MQTTdemo]MQTT DISCONNECT");

    if(mqtt_client_mqtt_reconnect() == XY_OK)
        xy_printf("[MQTTdemo] MQTT reconnect success\r\n");
    else
        xy_printf("[MQTTdemo] MQTT reconnect failed\r\n");
}

void mqtt_demo_send_task()
{
    int ret = XY_ERR;
    mqttSendMsg_t *mqttSendMsg = NULL;
    int msgType = 0xff;
    char* test_topic = xy_zalloc(strlen(MQTT_Topic)+1);
    strncpy(test_topic,MQTT_Topic,strlen(MQTT_Topic));
    MQTTMessage pubmsg ;

    if(mqtt_client_connect_subscribe() != XY_OK)
        xy_printf("[MQTTdemo] subscribe failed\r\n");


    while(pMQTT_test_client->close_sock != 1)
    {
        /* recv msg (block mode) */
        osMessageQueueGet(mqtt_demo_sendmsg_q, &mqttSendMsg ,NULL, osWaitForever);
        msgType = (int)mqttSendMsg->cmdType;
        xy_printf("[MQTTdemo]SendTask  recv msg=%d",msgType);

        switch(msgType)
        {
            case MQTT_DEMO_PUBLISH:
                xy_printf("[MQTTdemo]start send mqtt publish packet");
                mqtt_publish_param_init(&pubmsg);
                /*send MQTT publish message*/
                ret = xy_mqtt_publish(pMQTT_test_client, test_topic, &pubmsg,1);

                if(mqttSendMsg->payload != NULL)
                {
                    xy_free(mqttSendMsg->payload);
                    mqttSendMsg->payload = NULL;
                }

                /* check send result*/
                if(ret == XY_OK)
                {
                    xy_printf("[MQTTdemo]send mqtt publish packet ok");
                }
                else
                {
                    xy_printf("[MQTTdemo]send mqtt publish packet fail");
                    if(errno == ECONNABORTED ||errno ==  ECONNRESET ||errno ==  ENOTCONN|| errno == EIO)
                    {
                        xy_printf("[MQTTdemo]find need reconnect when publish packet");
                        xy_mqtt_disconnect(pMQTT_test_client,1);

                        mqtt_demo_tcp_disconnect_cb();
                    }
                    else
                    {
                        xy_printf("[MQTTdemo]socket err");
                    }
                }
                break;
            case MQTT_DEMO_KEEPLIVE:
                xy_printf("[MQTTdemo]keeplive process ");
                Timer timer;
                TimerInit(&timer);
                TimerCountdownMS(&timer, 1000);
                int len = MQTTSerialize_pingreq(pMQTT_test_client->buf, pMQTT_test_client->buf_size);
                if (len > 0 && (ret = MQTTSendPacket(pMQTT_test_client, len, &timer)) == SUCCESS) // send the ping packet
                    pMQTT_test_client->ping_outstanding = 1;
                xy_printf("[MQTT]send keeplive pkt len=%d rc=%d",len,ret);
                break;
            case MQTT_DEMO_TCP_RECONNECT:
                xy_printf("[MQTTdemo]find tcp need reconnect ");
                mqtt_demo_tcp_disconnect_cb();
                break;
            case MQTT_DEMO_RECONNECT:
                xy_printf("[MQTTdemo]find mqtt need reconnect ");
                mqtt_demo_mqtt_disconnect_cb();
                break;
            default:
                break;
        }
        xy_free(mqttSendMsg);
    }

    g_mqttsend_handle = NULL;
    osThreadExit();
}

void mqtt_demo_send_task_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqttsend_handle != NULL)
    {
        return;
    }
	thread_attr.name	   = "mqtt_send_task";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0xA00;
    g_mqttsend_handle = osThreadNew ((osThreadFunc_t)(mqtt_demo_send_task),NULL,&thread_attr);
}

void mqtt_demo_task()
{
    mqttSendMsg_t *mqttSendMsg =NULL;

    if(xy_wait_tcpip_ok(60) == XY_ERR)
        xy_assert(0);

    /*start mqtt send task*/
    mqtt_demo_send_task_init();

    while(1)
    {
        if(pMQTT_test_client->isconnected == 1)
        {
            break;
        }
        osDelay(1000);
    }

    while(1)
    {
        /*循环发送数据*/
        xy_printf("[MQTTdemo]send start");
        mqttSendMsg = xy_zalloc(sizeof(mqttSendMsg_t));
        mqttSendMsg->cmdType = MQTT_DEMO_PUBLISH;

        mqttSendMsg->payload = xy_zalloc(3);
        memcpy(mqttSendMsg->payload, "abc", 3);
        mqttSendMsg->payloadlen = 3;

        osMessageQueuePut(mqtt_demo_sendmsg_q, &mqttSendMsg, 0, osWaitForever);

        osDelay(5000);

        /*关闭mqtt demo处理示例*/
        if(false)
        {
            /*close send / receive thread */
            pMQTT_test_client->close_sock = 1;

            while(g_mqttdownlink_handle != NULL || g_mqttsend_handle != NULL)
            {
                osDelay(3000);
            }

            xy_printf("[MQTTdemo]close downlink recv thead");
            /* free MQTT client*/
            mqtt_free_demo_memory(pMQTT_test_client);
        }
    }

    osThreadExit();
}

//can call by  user_app_init
void mqtt_opencpu_demo_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqtttest_handle != NULL)
    {
        return;
    }

    if(mqtt_demo_sendmsg_q == NULL)
    {
        mqtt_demo_sendmsg_q = osMessageQueueNew(50, sizeof(void *), NULL);
    }

    if(mqtt_reconnect_sem == NULL)
    {
        mqtt_reconnect_sem = osSemaphoreNew(0xFFFF, 0, NULL);
    }
	
	thread_attr.name	   = "mqtt_test_demo";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0xA00;
    g_mqtttest_handle = osThreadNew ((osThreadFunc_t)(mqtt_demo_task),NULL,&thread_attr);
}
#endif
