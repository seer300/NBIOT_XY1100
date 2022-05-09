/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "MQTTClient.h"
#include "xy_utils.h"
#include "at_global_def.h"
#include "xy_at_api.h"
#include "lwip/errno.h"
#include "lwip/sockets.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define  MQTT_NUM            2
#define  MQTT_NET_OK         0
#define  MQTT_NET_ERR       -1
#define  MQTT_NET_TIMEOUT   -2
#define  mqtt_client_commandtimeout(cfg_id)   (((pMQTT_client[cfg_id]) == NULL) ?  2000 : pMQTT_client[cfg_id]->command_timeout_ms)
#define  mqtt_client_socket(cfg_id)   (((pMQTT_client[cfg_id]) == NULL || pMQTT_client[cfg_id]->ipstack == NULL) ?  -1 : pMQTT_client[cfg_id]->ipstack->my_socket)

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
MQTTClient *pMQTT_client[MQTT_NUM]={NULL};
osThreadId_t g_mqttpacket_handle = NULL;
osMutexId_t  g_mqtt_mutex= NULL;

/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/

/*****************************************************************************************
 Function    : mqtt_reorganize_message
 Description : server TCP segmentation need local reorganize_message
 Input       : mqtt_id: AT-command input mqtt client id
 Return      : > 0 :mqtt client id matched ; -1:match failed
 *****************************************************************************************/
int mqtt_reorganize_message(MQTTClient* c,int length,int mqtt_id)
{
    if(c->reorganize_recv_len +length > c->reorganize_len)
    {
        c->need_reorganize = 0;
        c->reorganize_len = 0;
        if(c->reorganizeBuf !=NULL)
        {
            xy_free(c->reorganizeBuf);
            c->reorganizeBuf =NULL;
        }
        return 0;
    }
    else if(c->reorganize_recv_len +length < c->reorganize_len)
    {
        memcpy(c->reorganizeBuf[c->reorganize_recv_len], c->readbuf,length);
        c->reorganize_recv_len += length;
        return 0;
    }
    else if(c->reorganize_recv_len +length == c->reorganize_len)
    {
        memcpy(&c->reorganizeBuf[c->reorganize_recv_len], c->readbuf,length);
        softap_printf(USER_LOG, WARN_LOG,"[MQTT] publish packet reorganize cycle,readbufsize：%d,reorganize_len：%d",length,c->reorganize_len);

        memcpy(c->readbuf,c->reorganizeBuf,c->readbuf_size);

        mqtt_cycle(c,NULL,c->reorganize_len,mqtt_id);
        if(c->reorganizeBuf != NULL)
        {
            xy_free(c->reorganizeBuf);
            c->reorganizeBuf =NULL;
        }

        c->need_reorganize = 0;
        c->reorganize_len = 0;
        return 0;
    }

    return 0;
}

/*****************************************************************************************
 Function    : mqtt_version_is_right
 Description : check MQTT version information(only support MQTT v4)
 Input       : version: input mqtt version
 Return      : 1:version is right ; -1:version is error
 *****************************************************************************************/
bool mqtt_version_is_right(int version)
{
    return (version == 4);
}

/*****************************************************************************************
 Function    : mqtt_free_client
 Description : free MQTT client memory
 Input       : client_cfg_id: MQTT client id
 Return      : void
 *****************************************************************************************/
void mqtt_free_client(int client_cfg_id)
{
    int i ;

	if(pMQTT_client[client_cfg_id] == NULL)
		return;

    if( pMQTT_client[client_cfg_id]->ipstack->my_socket >= 0)
    {    	
		close(pMQTT_client[client_cfg_id]->ipstack->my_socket);
		pMQTT_client[client_cfg_id]->ipstack->my_socket =-1;
    }
        
    //free subscribe topic memory
    if (pMQTT_client[client_cfg_id]->cleansession)
    {
        for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        {
            if(pMQTT_client[client_cfg_id]->messageHandlers[i].topicFilter)
            {
                xy_free(pMQTT_client[client_cfg_id]->messageHandlers[i].topicFilter);
                pMQTT_client[client_cfg_id]->messageHandlers[i].topicFilter=NULL;
            }

        }
    }

    if(pMQTT_client[client_cfg_id]->ipstack !=NULL )
    {
        xy_free(pMQTT_client[client_cfg_id]->ipstack);
        pMQTT_client[client_cfg_id]->ipstack =NULL;
    }
    if(pMQTT_client[client_cfg_id]->buf !=NULL)
    {
        xy_free(pMQTT_client[client_cfg_id]->buf);
        pMQTT_client[client_cfg_id]->buf =NULL;
    }
    if(pMQTT_client[client_cfg_id]->readbuf !=NULL)
    {
        xy_free(pMQTT_client[client_cfg_id]->readbuf);
        pMQTT_client[client_cfg_id]->readbuf =NULL;
    }
	if(pMQTT_client[client_cfg_id]->subscribetopic != NULL)
	{
		xy_free(pMQTT_client[client_cfg_id]->subscribetopic);
		pMQTT_client[client_cfg_id]->subscribetopic = NULL;
	}
    if(pMQTT_client[client_cfg_id] != NULL)
    {
        xy_free(pMQTT_client[client_cfg_id]);
        pMQTT_client[client_cfg_id] =NULL;
    }

	return;
}

/*****************************************************************************************
 Function    : mqtt_keepalive
 Description : MQTT keepalive function
 Input       : c: MQTT client
 Return      : success:0 ;fail:-1
 *****************************************************************************************/
int mqtt_keepalive(MQTTClient* c)
{
    int rc = SUCCESS;
    Timer  lastsent = {0};
    Timer  lastreceive ={0};
    Timer  var ={0};

    if (c->keepAliveInterval == 0)
        goto exit;

    var.end_time.tv_sec = 10;
    xy_timersub(&c->last_sent.end_time, &var.end_time, &lastsent.end_time);
    xy_timersub(&c->last_received.end_time, &var.end_time, &lastreceive.end_time);

    if (TimerIsExpired(&lastsent) || TimerIsExpired(&lastreceive))
    {
        if (c->ping_outstanding)
            rc = FAILURE; /* PINGRESP not received in keepalive interval */
        else
        {
            Timer timer;
            TimerInit(&timer);
            TimerCountdownMS(&timer, 200);
            int len = MQTTSerialize_pingreq(c->buf, c->buf_size);
            if (len > 0 && (rc = MQTTSendPacket(c, len, &timer)) == SUCCESS) // send the ping packet
                c->ping_outstanding = 1;

            softap_printf(USER_LOG, WARN_LOG,"[MQTT]send keeplive pkt len=%d rc=%d",len,rc);
        }
    }

exit:
    return rc;
}

/*****************************************************************************************
 Function    : mqtt_cycle
 Description : process MQTT downlink packet
 Input       : c :MQTT client
               messageHandler: receive MQTT publish packet for specific topics handler
 Return      : success:0 ;fail:-1
 *****************************************************************************************/
int mqtt_cycle(MQTTClient* c,messageHandler messageHandler,int length,int mqtt_id)
{
    MQTTHeader header = {0};
    int packet_type =0;
    unsigned short mypacketid =0;
    unsigned char *recv_data = NULL;
    unsigned char *recv_data_ptr = NULL;
    char *publish_rsp = NULL;
    //char *cachepublish_rsp = NULL;
    char *hexpayload_rsp = NULL;
    char *topic_rsp   = NULL;
    char *suback_rsp  = (char*)xy_zalloc(30 + strlen((const void *)(c->subscribetopic)));
    char *mqtt_rsp    = (char*)xy_zalloc(60);
    int len = 0,
        offset=0,
        rem_len=0 , /* read remaining length */
        payload_len=0 , /* read payload length */
        rc = SUCCESS;
    Timer timer;
    TimerInit(&timer);
    MQTTConnackData data={0};

    if (c == NULL)
        goto exit;

    TimerCountdownMS(&timer, c->command_timeout_ms);

    while(offset < length){
        header.byte = c->readbuf[offset];
        packet_type = header.bits.type;

        if ((c->keepAliveInterval > 0) && (packet_type >=1 ))
            TimerCountdown(&c->last_received, c->keepAliveInterval); // Calculate the timeout according to the latest message

        softap_printf(USER_LOG, WARN_LOG,"[MQTT] cycle packet_type = %d\n",packet_type);

        recv_data = c->readbuf + offset;
        if(c->need_reorganize == 1)
        {
            softap_printf(USER_LOG, WARN_LOG,"[MQTT] publish packet reorganize,before %d,come %d，total %d,%s",c->reorganize_recv_len,strlen(c->readbuf),c->reorganize_len);
            mqtt_reorganize_message(c,length,mqtt_id);
            goto exit;
        }

        switch (packet_type)
        {
            default:
            {
                /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
                rc = packet_type;
                goto exit;
            }
            case -1:
            case  0: /* timed out reading packet */
                break;
            case CONNACK:
            {
                c->waitflag.waitConAck.flag = 0;
                if (MQTTDeserialize_connack(&data.sessionPresent, &data.rc, recv_data, c->readbuf_size) == 1)
                {
                    if(data.rc  == 0)
                        c->isconnected =1;
                    else
                    {
                        rc = FAILURE;
                        MQTTCloseSession(c);
                    }
                    snprintf(mqtt_rsp,60, "\r\n+MQCONNACK:%d,%d\r\n",mqtt_id,data.rc);
                    send_rsp_str_to_ext(mqtt_rsp);

                }
                else if(MQTTDeserialize_connack(&data.sessionPresent, &data.rc, recv_data, c->readbuf_size) == 0)
                {
                    snprintf(mqtt_rsp,60, "\r\n+MQCONNACK:%d deserialize error\r\n",mqtt_id);
                    send_rsp_str_to_ext(mqtt_rsp);
                    rc = FAILURE;
                    goto exit;
                }

                break;
            }
            case PUBACK:
            {
                c->waitflag.waitPubAck.flag = 0;
                unsigned short mypacketid;
                unsigned char dup, type;
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, recv_data, c->readbuf_size) != 1)
                {
                    snprintf(mqtt_rsp,60, "\r\n+MQPUBACK:%d deserialize error\r\n",mqtt_id);
                    send_rsp_str_to_ext(mqtt_rsp);
                    rc = FAILURE;
                    goto exit;
                }

                snprintf(mqtt_rsp,60, "\r\n+MQPUBACK:%d\r\n",mqtt_id);
                send_rsp_str_to_ext(mqtt_rsp);
                break;
            }
            case SUBACK:
            {
                c->waitflag.waitSubAck.flag = 0;
                MQTTSubackData subackdata;
                int count = 0;
                subackdata.grantedQoS = QOS0;
                if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&subackdata.grantedQoS, recv_data, c->readbuf_size) == 1)
                {
                    if (subackdata.grantedQoS != 0x80)
                    {
                        rc = MQTTSetMessageHandler(c, c->subscribetopic, messageHandler);
                        snprintf(suback_rsp,(30 + strlen(c->subscribetopic)), "\r\n+MQSUBACK:%d,%s,%d\r\n",mqtt_id,c->subscribetopic,subackdata.grantedQoS);
                        send_rsp_str_to_ext(suback_rsp);
                    }
                    else
                    {
                        snprintf(mqtt_rsp,60, "\r\n+MQSUBACK:subscribe fail\r\n");
                        send_rsp_str_to_ext(mqtt_rsp);
                    }

                }
                else if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&subackdata.grantedQoS, recv_data, c->readbuf_size) == 0)
                {
                    snprintf(mqtt_rsp,60, "\r\n+MQSUBACK:%d deserialize error\r\n",mqtt_id);
                    send_rsp_str_to_ext(mqtt_rsp);
                    rc = FAILURE;
                    c->ping_outstanding = 0;
                    if (c->cleansession)
                        MQTTCleanSession(c);
                    goto exit;
                }
                if((c != NULL) &&(c->subscribetopic !=NULL))
                {
                    xy_free(c->subscribetopic);
                    c->subscribetopic = NULL;
                }

                break;
            }
            case UNSUBACK:
            {
                c->waitflag.waitUnSubAck.flag = 0;
                if (MQTTDeserialize_unsuback(&mypacketid, recv_data, c->readbuf_size) == 1)
                {
                    /* remove the subscription message handler associated with this topic, if there is one */
                    MQTTSetMessageHandler(c, c->subscribetopic, NULL);
                    snprintf(suback_rsp,(30 + strlen(c->subscribetopic)), "\r\n+MQUNSUBACK:%d,%s\r\n",mqtt_id,c->subscribetopic);
                    send_rsp_str_to_ext(suback_rsp);

                }
                else if(MQTTDeserialize_unsuback(&mypacketid, recv_data, c->readbuf_size) == 0)
                {
                    snprintf(mqtt_rsp,60, "\r\n+MQUNSUBACK:%d deserialize error\r\n",mqtt_id);
                    send_rsp_str_to_ext(mqtt_rsp);
                    rc = FAILURE;
                    goto exit;
                }
                if((c != NULL) &&(c->subscribetopic !=NULL))
                {
                    xy_free(c->subscribetopic);
                    c->subscribetopic = NULL;
                }
                break;
            }
            case PUBLISH:
            {
                MQTTString topicName;
                MQTTMessage msg;
                int intQoS;
                msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */

                if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
                   (unsigned char**)&msg.payload, (int*)&msg.payloadlen, recv_data, c->readbuf_size) != 1)
                {
                    rc = FAILURE;
                    goto exit;
                }
                else if((length-offset-((unsigned char *)msg.payload - recv_data)) < msg.payloadlen)
                {
                    /*Reorganize if TCP segments*/
                    c->need_reorganize = 1;
                    c->reorganize_len = msg.payloadlen + ((unsigned char *)msg.payload - recv_data);
                    c->reorganize_recv_len = length;
                    c->reorganizeBuf = (char*)xy_zalloc(c->reorganize_len + 1);
                    memcpy(c->reorganizeBuf, recv_data,c->reorganize_len);
                    snprintf(mqtt_rsp,60, "\r\n+MQPUB:TCP segmentation,packet reorganize \n");
                    softap_printf(USER_LOG, WARN_LOG,"[MQTT] publish reorganize total:%d,rcvbuf:%d\n",c->reorganize_len,c->reorganize_recv_len);
                    send_rsp_str_to_ext(mqtt_rsp);
                    goto exit;
                }
                else
                {
                    msg.qos = (enum QoS)intQoS;
                    deliverMessage(c, &topicName, &msg);

                    topic_rsp = (char*)xy_zalloc(topicName.lenstring.len + 1);
                    memcpy(topic_rsp,topicName.lenstring.data,topicName.lenstring.len);
                    topic_rsp[topicName.lenstring.len]='\0';

                    hexpayload_rsp = (char*)xy_zalloc(msg.payloadlen * 2 +1);
                    bytes2hexstr(msg.payload, msg.payloadlen,  hexpayload_rsp, msg.payloadlen * 2+1);
                    publish_rsp = (char*)xy_zalloc(30 + topicName.lenstring.len +msg.payloadlen*2);
                    snprintf(publish_rsp,(30 + topicName.lenstring.len +msg.payloadlen*2), "\r\n+MQPUB:%d,%s,%d,%d,%d,%d,%s\r\n",mqtt_id,topic_rsp,msg.qos,msg.retained,msg.dup,msg.payloadlen,hexpayload_rsp);
                    send_rsp_str_to_ext(publish_rsp);

                    if (msg.qos != QOS0)
                    {
                        osMutexAcquire(g_mqtt_mutex, osWaitForever);
                        if (msg.qos == QOS1)
                            len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
                        else if (msg.qos == QOS2)
                            len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
                        if (len <= 0)
                        {
                            softap_printf(USER_LOG, WARN_LOG,"[MQTT] downlink publish Serialize pubreply fail\n");
                            rc = FAILURE;
                        }
                        else
                        {
                            rc = MQTTSendPacket(c, len, &timer);
                        }
                        osMutexRelease(g_mqtt_mutex);
                    }
                }
                break;
            }
            case PUBREC:
            case PUBREL:
            {
                unsigned short mypacketid;
                unsigned char dup, type;
                osMutexAcquire(g_mqtt_mutex, osWaitForever);
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, recv_data, c->readbuf_size) != 1)
                    rc = FAILURE;
                else if ((len = MQTTSerialize_ack(c->buf, c->buf_size,
                    (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                    rc = FAILURE;
                else if ((rc = MQTTSendPacket(c, len, &timer)) != SUCCESS)
                    rc = FAILURE; // there was a problem

                osMutexRelease(g_mqtt_mutex);
                snprintf(mqtt_rsp,60, "\r\n+%s:%d,%d,%d\r\n",(packet_type == PUBREC) ? "MQPUBREC" : "MQPUBREL",mqtt_id,dup,mypacketid);
                send_rsp_str_to_ext(mqtt_rsp);
                break;
            }
            case PUBCOMP:
            {
                c->waitflag.waitPubAck.flag = 0;
                unsigned short mypacketid;
                unsigned char dup, type;
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, recv_data, c->readbuf_size) != 1)
                    rc = FAILURE;

                snprintf(mqtt_rsp,60, "\r\n+MQPUBCOMP:%d,%d,%d\r\n",mqtt_id,dup,mypacketid);
                send_rsp_str_to_ext(mqtt_rsp);
                break;
            }
            case PINGRESP:
                c->ping_outstanding = 0;
                break;
        }

        recv_data_ptr = recv_data + 1; //recvdata 中remain_len位置
        rem_len = MQTTPacket_decodeBuf(recv_data_ptr, &payload_len);
        offset += 1 + rem_len + payload_len;//HEAD+REAMINLEN+PAYLOADLEN
        softap_printf(USER_LOG, WARN_LOG,"[MQTTTTTT] rem_len:%d ,pay_len:%d,offset:%d,length:%d,buf:%p,recv:%p\n",rem_len,payload_len,offset,length,c->readbuf,recv_data);

    }

exit:
    if(mqtt_rsp)
        xy_free(mqtt_rsp);
    if(publish_rsp)
        xy_free(publish_rsp);
    if(suback_rsp)
        xy_free(suback_rsp);
    if(topic_rsp)
        xy_free(topic_rsp);
    if(hexpayload_rsp)
        xy_free(hexpayload_rsp);

    memset(c->readbuf,0,c->readbuf_size);
    return rc;
}

/*****************************************************************************
 Function    : mqtt_select_timeout_pro
 Description : SELECT function timeout handler
 Input       : linkstate_rsp: report string
 Return      : void
 *****************************************************************************/
void mqtt_select_timeout_pro(char* linkstate_rsp )
{
    int i;

    for (i = 0; i < MQTT_NUM; i++)
    {
        if(pMQTT_client[i] != NULL)
        {
            if(pMQTT_client[i]->waitflag.waitConAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitConAck.timeout)))
            {
                snprintf(linkstate_rsp,60, "\r\n+MQDISCON:%d\r\n",i);
                send_rsp_str_to_ext(linkstate_rsp);
                mqtt_free_client(i);
            }
            else if(pMQTT_client[i]->waitflag.waitSubAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitSubAck.timeout)))
            {
                snprintf(linkstate_rsp,60, "\r\n+MQDISCON:%d\r\n",i);
                send_rsp_str_to_ext(linkstate_rsp);
                mqtt_free_client(i);
            }
            else if(pMQTT_client[i]->waitflag.waitPubAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitPubAck.timeout)))
            {
                snprintf(linkstate_rsp,60, "\r\n+MQDISCON:%d\r\n",i);
                send_rsp_str_to_ext(linkstate_rsp);
                mqtt_free_client(i);
            }
            else if(pMQTT_client[i]->waitflag.waitUnSubAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitUnSubAck.timeout)))
            {
                snprintf(linkstate_rsp,60,"\r\n+MQDISCON:%d\r\n",i);
                send_rsp_str_to_ext(linkstate_rsp);
                mqtt_free_client(i);
            }
        }
    }

	return;
}

/*****************************************************************************
 Function    : mqtt_buf_recv
 Description : use SELECT function to receive MQTT downlink packet
 Input       : socketnum:MQTT client id
 Return      : 0:timeout ; < 0:net error ;> 0 success
 *****************************************************************************/
void mqtt_buf_recv(char* printstring)
{
    int i = 0;
    int ret = -1;
    int maxsocket = -1;
    struct timeval tv;
    fd_set read_fds,exceptfds;
	int timeout = 2000;

    softap_printf(USER_LOG, WARN_LOG, "mqtt_buf_recv s0:%d s1:%d",  mqtt_client_socket(0), mqtt_client_socket(1));
    FD_ZERO(&read_fds);
    FD_ZERO(&exceptfds);

    for (i = 0; i < MQTT_NUM; i++)
    {
        if(mqtt_client_socket(i) < 0)
        {
            continue;
        }
        //AT cmd disconnect tcp link
        else if(pMQTT_client[i]->close_sock)
        {
            close(pMQTT_client[i]->ipstack->my_socket);
            pMQTT_client[i]->ipstack->my_socket= -1;
            continue;
        }
        else
        {
            if(maxsocket < mqtt_client_socket(i))
            {
                maxsocket = mqtt_client_socket(i);
            }
            FD_SET(mqtt_client_socket(i), &read_fds);
            FD_SET(mqtt_client_socket(i), &exceptfds);
        }
    }

    if (maxsocket < 0)
    {
        return ;
    }


    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    ret = select(maxsocket+1, &read_fds, NULL, &exceptfds, &tv);


   if (ret == 0)
   {
        softap_printf(USER_LOG, WARN_LOG, "select NET TIMEOUT maxsocket=%d", maxsocket);
        mqtt_select_timeout_pro(printstring);
        return ;
   }
   else if(ret < 0)
   {
        softap_printf(USER_LOG, WARN_LOG, "select error ret=%d,err %d", ret, errno);
        return ;
   }

   for (i = 0; i < MQTT_NUM; i++)
   {
       if((mqtt_client_socket(i) >= 0) && (FD_ISSET(mqtt_client_socket(i), &exceptfds)))
       {
           snprintf(printstring,60, "\r\n+MQDISCON:%d\r\n",i);
           send_rsp_str_to_ext(printstring);
           mqtt_free_client(i);
           return ;
       }

       if((mqtt_client_socket(i) >= 0) && (FD_ISSET(mqtt_client_socket(i), &read_fds)))
       {
           ret = recv(mqtt_client_socket(i), pMQTT_client[i]->readbuf, pMQTT_client[i]->readbuf_size, 0);

           if (ret < 0)
           {
               if (errno == EWOULDBLOCK) // time out
                   mqtt_select_timeout_pro(printstring);
               return ;
           }
           else if (ret == 0)
           {
               if(pMQTT_client[i]->close_sock != 1 )
               {
                   snprintf(printstring,60,"\r\n+MQDISCON:%d\r\n",i);
                   send_rsp_str_to_ext(printstring);
                   mqtt_free_client(i);
               }
               return;
           }
           else
           {
               //process downlink pkt
               mqtt_cycle(pMQTT_client[i],NULL,ret,i);
           }

       }

   }

    return;
}

/*******************************************************************************************
 Function    : mqtt_downdata_recv
 Description : process MQTT downlink packet according to the SELECT function return value
 Input       : void
 Return      : void
 *******************************************************************************************/
void mqtt_downdata_recv(void)
{
    int i         = 0;
    int numBytes  = 0;
    int socketnum = -1;
    char *linkstate_rsp = (char*)xy_zalloc(60);
    while(1)
    {
        if(mqtt_client_socket(0) < 0 && mqtt_client_socket(1) < 0)
        {
            softap_printf(USER_LOG, WARN_LOG, "[MQTT] quit process downlink pkt thread");
            if(linkstate_rsp)
                xy_free(linkstate_rsp);
            return;
        }

        softap_printf(USER_LOG, WARN_LOG, "[MQTT] app_downdata_recv s0:%d s1%d", mqtt_client_socket(0), mqtt_client_socket(1));

        mqtt_buf_recv(linkstate_rsp);

        for (i = 0; i < MQTT_NUM; i++)
        {
            if(pMQTT_client[i] != NULL)
            {
                osMutexAcquire(g_mqtt_mutex, osWaitForever);
                mqtt_keepalive(pMQTT_client[i]);
                osMutexRelease(g_mqtt_mutex);
            }
        }
    }

}

/*******************************************************************************************
 Function    : mqtt_deal_packet
 Description : process MQTT downlink packet function
 Input       : void
 Return      : void
 *******************************************************************************************/
void mqtt_deal_packet()
{
    softap_printf(USER_LOG, WARN_LOG, "[MQTT]process downlink pkt thread start");

    //wait PDP active
    if(xy_wait_tcpip_ok(2*60) == XY_ERR)
        xy_assert(0);

    mqtt_downdata_recv();

	g_mqttpacket_handle = NULL;
	osThreadExit();

    softap_printf(USER_LOG, WARN_LOG, "[MQTT]process downlink pkt thread end");
}

/*******************************************************************************************
 Function    : mqtt_task_create
 Description : create MQTT process downlink packet task
 Input       : void
 Return      : void
 *******************************************************************************************/
void mqtt_task_create()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqttpacket_handle != NULL)
        return;
	thread_attr.name       = "mqtt_process_packet";
	thread_attr.priority   = XY_OS_PRIO_NORMAL1;
	thread_attr.stack_size = 0x1000;
    g_mqttpacket_handle = osThreadNew((osThreadFunc_t)(mqtt_deal_packet), NULL, &thread_attr);
}


/*******************************************************************************************
 Function    : at_MQNEW_req
 Description : establish a new mqtt connection to the mqtt server over the TCP protocol
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +EMQNEW=<server>,<port>,<command_timeout_ms>,<bufsize>[,<cid>]
 *******************************************************************************************/
int at_MQNEW_req(char *at_buf, char **prsp_cmd)
{
    int  i           = 0;
    char *remote_ip  = NULL;
    char *readbuf    = NULL;
    char *sendbuf    = NULL;
    char *command_timeout_str= xy_zalloc(strlen(at_buf));
    char *remote_port_str    = xy_zalloc(strlen(at_buf));
    Network *ipstack = NULL;
    int  buf_size    = 0;
    int  command_timeout = 0;
    int  client_cfg_id = -1;
    int  remote_port = 0;
    remote_ip = xy_zalloc(strlen(at_buf));
    void *p[] = {remote_ip,remote_port_str,command_timeout_str,&buf_size};

    softap_printf(USER_LOG, WARN_LOG, "[MQTT] NEW BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%s,%s,%s,%d,%d", at_buf, p) != AT_OK || !strcmp(remote_ip,""))
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    /*port 最大5个字节*/
    if(strlen(remote_port_str) < 5)
    {
        remote_port = strtol(remote_port_str,NULL,10);
    }
    else if (strlen(remote_port_str) == 5)
    {
        if(strncmp(remote_port_str, "65536", 5) < 0)
        {
            remote_port = strtol(remote_port_str,NULL,10);
        }
        else
            remote_port = 0;
    }
    else
        remote_port = 0;

    /*command_timeout 最大5个字节【90000】]*/
    if(strlen(command_timeout_str) < 6)
        command_timeout = strtol(command_timeout_str,NULL,10);
    else
        command_timeout = 0;

    if(remote_port<=0 || remote_port > 65535|| command_timeout < 2000 || command_timeout > 90000  || buf_size<=0 || buf_size > 1000)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    for (i = 0; i < MQTT_NUM; i++)
    {
        if(pMQTT_client[i] == NULL)
        {
            client_cfg_id = i;
            break;
        }
    }

    if(client_cfg_id == -1)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    ipstack = xy_zalloc(sizeof(Network));
    sendbuf = xy_zalloc(buf_size);
    readbuf = xy_zalloc(buf_size);
    NetworkInit(ipstack);
    if( SUCCESS !=NetworkConnect(ipstack, remote_ip, remote_port))
    {
        if( ipstack != NULL && ipstack->my_socket >= 0)
        {
            softap_printf(USER_LOG, WARN_LOG, "[MQTT] CLOSE socket %d client id %d\n",ipstack->my_socket,client_cfg_id);
            close(ipstack->my_socket);
        }
        if(ipstack)
            xy_free(ipstack );

        if(sendbuf)
            xy_free(sendbuf);

        if(readbuf)
            xy_free(readbuf);

        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }
    pMQTT_client[client_cfg_id] = xy_zalloc(sizeof(MQTTClient));
    MQTTClientInit(pMQTT_client[client_cfg_id], ipstack, command_timeout, sendbuf, buf_size, readbuf, buf_size);
    mqtt_task_create();

    *prsp_cmd = xy_zalloc(30);
    snprintf(*prsp_cmd, 30, "\r\n+MQNEW:%d\r\n\r\nOK\r\n", client_cfg_id);
    softap_printf(USER_LOG, WARN_LOG, "[MQTT] NEW END\n");

ERR_PROC:
    if(remote_ip)
        xy_free(remote_ip);
    if(command_timeout_str)
        xy_free(command_timeout_str);
    if(remote_port_str)
        xy_free(remote_port_str);
    return AT_END;
}

/*********************************************************************************************************************************************
 Function    : at_MQCON_req
 Description : send MQTT connect messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQCON=<mqtt_id>,<version>,<client_id>,<keepalive_interval>,<cleansession>,<will_flag>[,<will_options>][,<username>,<password>]
 *********************************************************************************************************************************************/
int at_MQCON_req(char *at_buf, char **prsp_cmd)
{
    int  mqtt_id = -1;
    int  ret     = -1;
    int  version = 0;
    int  cleansession      = 0;
    int  will_flag         = 0;
    int  qos               = 0;
    int  retained          = 0;
    int  willlen           = 0;
    unsigned short  keepalive_interval= 0;
	char *tans_data = NULL;
    char *client_id = xy_zalloc(strlen(at_buf));
    char *username  = xy_zalloc(strlen(at_buf));
    char *password  = xy_zalloc(strlen(at_buf));
    char *topicName = xy_zalloc(strlen(at_buf));
    char *willmessage= xy_zalloc(strlen(at_buf));
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    void *p[] = {&mqtt_id,&version,client_id,&keepalive_interval,&cleansession,&will_flag,topicName,&qos,&retained,&willlen,willmessage,username,password};

    softap_printf(USER_LOG, WARN_LOG, "[MQTT] CONNECT BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%d,%d,%s,%2d,%d,%d,%s,%d,%d,%d,%s,%s,%s", at_buf, p) != AT_OK)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(mqtt_id < 0 || mqtt_id >= MQTT_NUM || pMQTT_client[mqtt_id] == NULL ||!strcmp(client_id,"") || !mqtt_version_is_right(version) ||  willlen < 0
            || keepalive_interval < 10 || keepalive_interval > 0xFFFF|| cleansession < 0 ||  cleansession > 1 || retained  < 0 ||  retained > 1 || will_flag  < 0 ||  will_flag > 1)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

	if (will_flag)
	{
		if (willlen != 0)
		{
			if (willlen * 2 != strlen(willmessage)) 
	    	{
				*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
				goto ERR_PROC;
				
	    	}

			tans_data = xy_zalloc(willlen);
	    	if (hexstr2bytes(willmessage, willlen * 2, tans_data, willlen) == -1)
	    	{
	        	*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	        	goto ERR_PROC;
	    	}
	    }
	}
				
    if(pMQTT_client[mqtt_id]->isconnected == 1 || pMQTT_client[mqtt_id]->waitflag.waitConAck.flag == 1)
    {
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto ERR_PROC;
    }

    data.willFlag = will_flag;
    data.MQTTVersion = version;

    data.clientID.cstring = client_id;
    softap_printf(USER_LOG, WARN_LOG, "[MQTT]connect client=%s",data.clientID.cstring);
    if(will_flag)
    {
        data.will.topicName.cstring = topicName;
        data.will.qos = qos;
        data.will.retained = retained;
        data.will.message.lenstring.len = willlen;
        data.will.message.lenstring.data = tans_data;
    }

    data.keepAliveInterval = keepalive_interval;
    data.cleansession = cleansession;

    data.username.cstring = username;
    data.password.cstring = password;

    osMutexAcquire(g_mqtt_mutex, osWaitForever);
    ret = xy_mqtt_connect(pMQTT_client[mqtt_id], &data,0);
    osMutexRelease(g_mqtt_mutex);
    if (ret != SUCCESS)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitConAck.timeout);
    TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitConAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
    pMQTT_client[mqtt_id]->waitflag.waitConAck.flag = 1;

    softap_printf(USER_LOG, WARN_LOG, "[MQTT] CONNECT END\n");
ERR_PROC:
    if(willmessage)
        xy_free(willmessage);
    if(topicName)
        xy_free(topicName);
    if(password)
        xy_free(password);
    if(username)
        xy_free(username);
    if(client_id)
        xy_free(client_id);
	if(tans_data)
        xy_free(tans_data);
    return AT_END;
}

/*****************************************************************************
 Function    : at_MQDISCON_req
 Description : send MQTT disconnect messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQDISCON=<mqtt_id>
 *****************************************************************************/
int at_MQDISCON_req(char *at_buf, char **prsp_cmd)
{
    int  time = 0;
    int  ret  = -1;
    int  mqtt_id = -1;
    void *p[] = {&mqtt_id};
    char *mqtt_rsp = (char*)xy_zalloc(60);

    softap_printf(USER_LOG, WARN_LOG, "MQTT DISCONNECT BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%d", at_buf, p) != AT_OK)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(mqtt_id < 0 || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(!pMQTT_client[mqtt_id]->isconnected)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    osMutexAcquire(g_mqtt_mutex, osWaitForever);
    ret = xy_mqtt_disconnect(pMQTT_client[mqtt_id],0);
    osMutexRelease(g_mqtt_mutex);
    if (ret != SUCCESS)
    {
        softap_printf(USER_LOG, WARN_LOG, "MQTT disconnect failed!");
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    pMQTT_client[mqtt_id]->close_sock = 1;
    while(pMQTT_client[mqtt_id]->ipstack->my_socket >= 0)
    {
        time +=200;
        osDelay(200);
        if (time > pMQTT_client[mqtt_id]->command_timeout_ms)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            goto ERR_PROC;
        }

    }

    softap_printf(USER_LOG, WARN_LOG, "MQTT DISCONNECT END\n");

    mqtt_free_client(mqtt_id);
    snprintf(mqtt_rsp,60, "\r\n+MQDISCON:%d\r\n",mqtt_id);
    send_rsp_str_to_ext(mqtt_rsp);

ERR_PROC:
    if(mqtt_rsp)
        xy_free(mqtt_rsp);
    return AT_END;
}

/*****************************************************************************
 Function    : at_MQSUB_req
 Description : send MQTT subscribe messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQSUB=<mqtt_id>,<topic>,<QoS>
 *****************************************************************************/
int at_MQSUB_req(char *at_buf, char **prsp_cmd)
{
    int  ret = -1;
    int  mqtt_id = -1;
    int  qos     = 0;
    char *topic = xy_zalloc(strlen(at_buf));
    void *p[] = {&mqtt_id,topic,&qos};

    softap_printf(USER_LOG, WARN_LOG, "[MQTT] subscribe BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%d,%s,%d", at_buf, p) != AT_OK || qos < 0 || qos > 2)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(mqtt_id < 0 || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(!pMQTT_client[mqtt_id]->isconnected || pMQTT_client[mqtt_id]->waitflag.waitSubAck.flag == 1)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    osMutexAcquire(g_mqtt_mutex, osWaitForever);
    ret = xy_mqtt_subscribe(pMQTT_client[mqtt_id], topic, qos,0);
    osMutexRelease(g_mqtt_mutex);
    if (ret !=SUCCESS)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }
    else
    {
        TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitSubAck.timeout);
        TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitSubAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
        pMQTT_client[mqtt_id]->waitflag.waitSubAck.flag = 1;
    }

    softap_printf(USER_LOG, WARN_LOG,"[MQTT] subscribe END\n");

ERR_PROC:
	if (topic != NULL)
	{
		xy_free(topic);
	}
    return AT_END;
}

/*****************************************************************************
 Function    : at_MQUNSUB_req
 Description : send MQTT unsubscribe messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQUNSUB=<mqtt_id>,<topic>
 *****************************************************************************/
int at_MQUNSUB_req(char *at_buf, char **prsp_cmd)
{
    int  ret = -1;
    int  mqtt_id = -1;
    char *topic = xy_zalloc(strlen(at_buf));
    void *p[] = {&mqtt_id,topic};

    softap_printf(USER_LOG, WARN_LOG,"[MQTT] unsubscribe BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%d,%s", at_buf, p) != AT_OK )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(mqtt_id < 0 || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(!pMQTT_client[mqtt_id]->isconnected || pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.flag == 1)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    osMutexAcquire(g_mqtt_mutex, osWaitForever);
    ret = xy_mqtt_unsubscribe(pMQTT_client[mqtt_id], topic,0);
    osMutexRelease(g_mqtt_mutex);
    if (ret != SUCCESS)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.timeout);
    TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
    pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.flag = 1;

    softap_printf(USER_LOG, WARN_LOG,"[MQTT] unsubscribe END\n");

ERR_PROC:
	if (topic != NULL)
	{
		xy_free(topic);
	}
    return AT_END;
}

/*****************************************************************************
 Function    : at_MQPUB_req
 Description : send MQTT publish messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQPUB=<mqtt_id>,<topic>,<QoS>,<retained>,<dup>,<message_len>,<message>
 *****************************************************************************/
int at_MQPUB_req(char *at_buf, char **prsp_cmd)
{
    int  ret = -1;
    int  qos = 0;
    int  dup = 0;
    int  retained = 0;
    int  mqtt_id  = -1;
    int  message_len = 0;
    MQTTMessage pubmsg;
    char *tans_data = NULL;
    char *topic     = xy_zalloc(strlen(at_buf));
    char *message   = xy_zalloc(strlen(at_buf));
//    char *formatted_message = xy_zalloc(strlen(at_buf));
    void *p[] = {&mqtt_id,topic,&qos,&retained,&dup,&message_len,message};

    memset(&pubmsg, 0, sizeof(pubmsg));
    softap_printf(USER_LOG, WARN_LOG,"[MQTT] publish BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!ps_netif_is_ok()) {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%d,%s,%d,%d,%d,%d,%s", at_buf, p) != AT_OK )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(mqtt_id < 0 || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL || message_len >1000 || qos < 0 ||qos > 2
           || retained < 0  ||retained > 1 || dup < 0  ||dup > 1 )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if( message_len < 0 || strlen(message) != message_len * 2)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    tans_data = xy_zalloc(message_len);
    if (hexstr2bytes(message, message_len * 2, tans_data, message_len) == -1)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(!pMQTT_client[mqtt_id]->isconnected || pMQTT_client[mqtt_id]->waitflag.waitPubAck.flag == 1)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

//    pubmsg.payload = formatted_message;
//    sprintf(formatted_message, "{\"data\":11}",message);//json mode;
    pubmsg.payload = tans_data;
    pubmsg.payloadlen = message_len;
    pubmsg.qos = qos;
    pubmsg.retained = retained;
    pubmsg.dup = dup;

    osMutexAcquire(g_mqtt_mutex, osWaitForever);
    ret = xy_mqtt_publish(pMQTT_client[mqtt_id], topic, &pubmsg,0);
    osMutexRelease(g_mqtt_mutex);
    if (ret != SUCCESS)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitPubAck.timeout);
    TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitPubAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
    if(qos > 0)
        pMQTT_client[mqtt_id]->waitflag.waitPubAck.flag = 1;

    softap_printf(USER_LOG, WARN_LOG,"[MQTT] publish END\n");

ERR_PROC:
    if(topic)
        xy_free(topic);
    if(message)
        xy_free(message);
    if(tans_data)
        xy_free(tans_data);
//    if(formatted_message)
//        xy_free(formatted_message);
    return AT_END;
}

/*****************************************************************************
 Function    : at_MQSTATE_req
 Description : Check if MQTT is connected
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQSTATE=<mqtt_id>
 *****************************************************************************/
int at_MQSTATE_req(char *at_buf, char **prsp_cmd)
{
    int  ret = -1;
    int  mqtt_id  = -1;
	int  isconnected = 0;
    void *p[] = {&mqtt_id};

    if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (at_parse_param_2("%d", at_buf, p) != AT_OK )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if(mqtt_id < 0 || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

	isconnected = xy_mqtt_isconnected(pMQTT_client[mqtt_id]);

	*prsp_cmd = xy_zalloc(30);
    snprintf(*prsp_cmd, 30, "\r\n+MQSTATE:%d\r\n\r\nOK\r\n", isconnected);
	
ERR_PROC:
    return AT_END;
}

static uint16_t  s_mqtt_inited = 0;

void at_mqtt_init(void)
{
    if(!s_mqtt_inited)
    {
        cloud_mutex_create(&g_mqtt_mutex);
        s_mqtt_inited = 1;
    }
    return;
}
