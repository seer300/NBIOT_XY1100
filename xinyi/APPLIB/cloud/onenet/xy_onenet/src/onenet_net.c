
/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "cis_api.h"
#include "cis_if_net.h"
#include <stdio.h>
#include <cis_list.h>
#include <cis_if_sys.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "cis_internals.h"
#include "at_onenet.h"
#include "lwip/sockets.h"
#include "lwip/opt.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define MAX_PACKET_SIZE            (1400)//512  // Maximal size of radio packet
#define CIST_SOCK_RECV_STACK_SIZE  1024*3
/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/

/*******************************************************************************
 *                        Local function declarations                          *
 ******************************************************************************/

/*******************************************************************************
 *                         Local variable definitions                          *
 ******************************************************************************/

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
extern osSemaphoreId_t g_cis_rcv_sem;
extern osSemaphoreId_t cis_poll_sem;
unsigned short g_onenet_localPort = 0;
unsigned short g_onenet_remotePort = 0;
/*******************************************************************************
 *                      Inline function implementations                        *
 ******************************************************************************/

/*******************************************************************************
 *                      Local function implementations                         *
 ******************************************************************************/
static int create_cisnet_sock(cisnet_t ctx)
{
    struct sockaddr_in sock_addr = {0};
    struct sockaddr_in local_addr= {0};
    socklen_t n = sizeof(struct sockaddr_in);
    struct addrinfo hints;
    struct addrinfo *addr_list;
    struct addrinfo *cur;
    struct sockaddr_in *addr;
    //struct addrinfo *cur;
    char   port[6];
    int ret;
    if (ctx == NULL)
    {
        return -1;
    }

    /* Do name resolution with both IPv6 and IPv4 */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    sprintf(port, "%d", ctx->port);
    if ((ret = getaddrinfo(ctx->host, port, &hints, &addr_list)) != 0)
    {
        softap_printf(USER_LOG, WARN_LOG, "getaddrinfo failed: 0x%x", ret);
        return -1;
    }

    for(cur = addr_list;cur != NULL;cur = cur->ai_next)
    {
        if(cur->ai_family == AF_INET)
        {
            addr = (struct sockaddr_in*)cur->ai_addr;
            memset(ctx->host,0x0,sizeof(ctx->host));
            inet_ntop(AF_INET, &addr->sin_addr, ctx->host, 16);
        }
    }

    /*Only support for ipv4 now*/
    ctx->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ctx->sock < 0)
    {
        freeaddrinfo(addr_list);
    	softap_printf(USER_LOG, WARN_LOG, "[CIS] Create socket error");	
        return -1;
    }
#if	0	
#if CIS_ENABLE_DM
	if(ctx->context != NULL)
	{
		st_context_t * ctxP = (st_context_t *)ctx->context;
		if(ctxP->isDM != TRUE)
		{
#endif 
#endif
			if(g_onenet_localPort != 0)
			{
				sock_addr.sin_family = AF_INET;
			    sock_addr.sin_port = lwip_htons(g_onenet_localPort);
			    if (bind(ctx->sock, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr)))
			    {
			    	freeaddrinfo(addr_list);
					close(ctx->sock);
			        return -1;
			    }
			}

            if (connect(ctx->sock, addr_list->ai_addr, addr_list->ai_addrlen) != 0)
            {
            	freeaddrinfo(addr_list);
                close(ctx->sock);
			    return -1;
            }
            
            if(g_onenet_localPort == 0)
            {
                getsockname(ctx->sock, (struct sockaddr*)&local_addr, &n);
                g_onenet_remotePort = ctx->port;
                g_onenet_localPort = ntohs(local_addr.sin_port);
            }

#if 0			
#if CIS_ENABLE_DM
		}
	}
#endif 
#endif
	freeaddrinfo(addr_list);

    return 0;
}

void cis_sock_recv_thread(void* lpParam)
{
    
    cisnet_t netctx = (cisnet_t)lpParam;
    //st_context_t* net_context = (st_context_t*)netctx->context;
	int sock = netctx->sock;
	while(0 == netctx->quit && netctx->state == 1)
	{
		struct timeval tv = {2,0};
		fd_set readfds, writefds, exceptfds;
		int result;

		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
		/*
			* This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
			* with the precedent function)
			*/
		//[XY]Add for onenet loop
		result = select(sock + 1, &readfds, &writefds, &exceptfds, &tv);
		softap_printf(USER_LOG, WARN_LOG, "[CIS]select result(%d): %d %s\n", result, errno, strerror(errno));
		if (result < 0)
		{
			softap_printf(USER_LOG, WARN_LOG, "Error in select(): %d %s\n", errno, strerror(errno));
            goto TAG_END;
		}
		else if (result > 0)
		{
			uint8_t buffer[MAX_PACKET_SIZE];
			int numBytes;

			/*
				* If an event happens on the socket
				*/
			if (FD_ISSET(sock, &readfds))
			{
				struct sockaddr_storage addr;
				socklen_t addrLen;

				addrLen = sizeof(addr);

				/*
					* We retrieve the data received
					*/
				numBytes = recvfrom(sock, (char*)buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

				if (numBytes < 0)
				{
					if(errno == 0)continue;
					softap_printf(USER_LOG, WARN_LOG, "Error in recvfrom(): %d %s\n", errno, strerror(errno));
				}
				else if (numBytes > 0)
				{
					char s[INET_ADDRSTRLEN];
					uint16_t  port;
			
					struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
					inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET_ADDRSTRLEN);
					port = saddr->sin_port;

					softap_printf(USER_LOG, WARN_LOG, "\n%d bytes received from [%s]:%hu\n", numBytes, s, ntohs(port));
					
                    uint8_t* data = (uint8_t*)cis_malloc(numBytes);
                    cis_memcpy(data,buffer,numBytes);

                    struct st_net_packet *packet = (struct st_net_packet*)cis_malloc(sizeof(struct st_net_packet));
                    packet->next = NULL;
                    packet->buffer = data;
                    packet->length = numBytes;
                    //packet->netctx = netctx;
					osMutexAcquire(netctx->onenet_socket_mutex, osWaitForever);
                    netctx->g_packetlist = (struct st_net_packet*)cis_packet_add(netctx->g_packetlist, packet);
					osMutexRelease(netctx->onenet_socket_mutex);
					//[XY]Add for onenet loop
					if(cis_poll_sem != NULL)
						osSemaphoreRelease(cis_poll_sem);

				}
			}
		}
	}
TAG_END:
	softap_printf(USER_LOG, WARN_LOG, "socket recv thread exit..\n");
    close(sock);
    //netctx->cis_sock_recv_thread_id = -1;
	netctx->cis_sock_recv_thread_id = NULL;
	osSemaphoreRelease(g_cis_rcv_sem);


	osThreadExit();
	return ;
}

/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/
cis_ret_t cisnet_init(void *context,const cisnet_config_t* config,cisnet_callback_t cb)
{
    softap_printf(USER_LOG, WARN_LOG, "cisnet_init\n");
    cis_memcpy(&((struct st_cis_context *)context)->netConfig,config,sizeof(cisnet_config_t));
    ((struct st_cis_context *)context)->netCallback.onEvent = cb.onEvent;
	 //vTaskDelay(1000);
    //((struct st_cis_context *)context)->netAttached = 1;
	return CIS_RET_OK;
}

cis_ret_t cisnet_create(cisnet_t* netctx,const char* host,void* context)
{
    softap_printf(USER_LOG, WARN_LOG, "cisnet_create\n");
    st_context_t * ctx = (st_context_t *)context;

	//if (ctx->netAttached != 1) return CIS_RET_ERROR;

    cisnet_t pctx = (cisnet_t)cissys_malloc(sizeof(struct st_cisnet_context));
	if (pctx == NULL) {
		softap_printf(USER_LOG, WARN_LOG, "Cannot malloc cisnet_t");
		return CIS_RET_ERROR;
	}

	/* PEER_PORT used for test, this port use as server port*/
	pctx->port = (int)strtol((const char *)(ctx->serverPort),NULL,10);
	 /*The address is type string, need address gethostbyname ? */
	strcpy(pctx->host, host);
    pctx->context = context;
    pctx->cis_sock_recv_thread_id = NULL;

	if (create_cisnet_sock(pctx) < 0) {
		softap_printf(USER_LOG, WARN_LOG, "Cannot create socket");
		cissys_free(pctx);
		return CIS_RET_ERROR;
	}

	cloud_mutex_create(&pctx->onenet_socket_mutex);
	(*netctx) = pctx;

	return CIS_RET_OK;
}

void cisnet_destroy(cisnet_t netctx)
{
	struct st_net_packet *temp, *node;
    softap_printf(USER_LOG, WARN_LOG, "cisnet_destroy\n");
    if (netctx->context) {
		//close(netctx->sock); // let the sock die naturally, close the sock in recv thread before thread exits
		netctx->context = NULL;
	}
    //CIS_LIST_FREE(netctx->g_packetlist);
    node = netctx->g_packetlist;
    while (node != NULL)
    {
		temp = node;
		node = node->next;
		cis_free(temp->buffer);
		cis_free(temp);
	}
	cissys_free(netctx);
	osMutexDelete(netctx->onenet_socket_mutex);
	netctx->onenet_socket_mutex = NULL;
}

cis_ret_t cisnet_connect(cisnet_t netctx)
{
	osThreadAttr_t thread_attr = {0};

    softap_printf(USER_LOG, WARN_LOG, "cisnet_connect\n");
    //char cis_sock_recv_thread_name[32] = {0};
	st_context_t* net_context = (st_context_t*)netctx->context;
    netctx->state = 1;
    //snprintf(cis_sock_recv_thread_name, 32, "cis_sock_recv_task_%d", netctx->sock);
    if (netctx->cis_sock_recv_thread_id == NULL)
    {
#if CIS_ENABLE_DM 
        if (net_context->isDM)
        {
			thread_attr.name	   = "cisdm_rcv";
			thread_attr.priority   = XY_OS_PRIO_NORMAL1;
			thread_attr.stack_size = 0xA00;
            netctx->cis_sock_recv_thread_id = osThreadNew ((osThreadFunc_t)(cis_sock_recv_thread),netctx,&thread_attr);

        }
        else
        {
#endif            
			thread_attr.name	   = "cisck_rcv";
			thread_attr.priority   = XY_OS_PRIO_NORMAL1;
			thread_attr.stack_size = 0xA00;
            netctx->cis_sock_recv_thread_id = osThreadNew ((osThreadFunc_t)(cis_sock_recv_thread),netctx,&thread_attr);

            //time maybe delay
            osThreadSetLowPowerFlag(netctx->cis_sock_recv_thread_id, osLowPowerProhibit);
#if CIS_ENABLE_DM 

        }
#endif
    }
	if (netctx->cis_sock_recv_thread_id == NULL)
        return CIS_RET_ERROR;
	net_context->netCallback.onEvent(netctx, cisnet_event_connected, NULL, netctx->context);
	return CIS_RET_OK;
}

cis_ret_t cisnet_disconnect(cisnet_t netctx)
{
    softap_printf(USER_LOG, WARN_LOG, "cisnet_disconnect\n");
	st_context_t* net_context = (st_context_t*)netctx->context;
    netctx->state = 0;

    if(netctx->cis_sock_recv_thread_id != NULL)
    {
        osDelay(2000); //收包线程不参与唤醒，此处唤醒会执行到收包线程释放信号量
        osSemaphoreAcquire(g_cis_rcv_sem, osWaitForever);
    }
    g_onenet_localPort = 0; //下次创建重新获取localport
	net_context->netCallback.onEvent(netctx, cisnet_event_disconnect, NULL, netctx->context);
	return CIS_RET_OK;
}

cis_ret_t cisnet_write(cisnet_t netctx,const uint8_t* buffer,uint32_t length, uint8_t raiflag)
{
    int nbSent;
    size_t addrlen;
    size_t offset;
    uint8_t type_offset = 2;
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(netctx->port);
    saddr.sin_addr.s_addr = inet_addr(netctx->host);
    
    addrlen = sizeof(saddr);
    offset = 0;
    while (offset != length)
    {
        //nbSent = sendto(netctx->sock, (const char*)buffer + offset, length - offset, 0, (struct sockaddr *)&saddr, addrlen);

        nbSent = sendto2(netctx->sock, (const char*)buffer + offset, length - offset, 0, (struct sockaddr *)&saddr, addrlen, 0, raiflag);

        if (nbSent == -1){
            softap_printf(USER_LOG, WARN_LOG, "socket sendto [%s:%d] failed.\n", netctx->host, ntohs(saddr.sin_port));
            return -1;
        }else{
            softap_printf(USER_LOG, WARN_LOG, "socket sendto [%s:%d] %d bytes\n", netctx->host, ntohs(saddr.sin_port), nbSent);
        }
        offset += nbSent;
    }

    return CIS_RET_OK;
}

cis_ret_t cisnet_read(cisnet_t netctx,uint8_t** buffer,uint32_t *length)
{
	if(netctx->g_packetlist != NULL){
		osMutexAcquire(netctx->onenet_socket_mutex, osWaitForever);
        struct st_net_packet* delNode;
        *buffer = netctx->g_packetlist->buffer;
        *length = netctx->g_packetlist->length;
         delNode = netctx->g_packetlist;
         netctx->g_packetlist = netctx->g_packetlist->next;
         cis_free(delNode);
		 osMutexRelease(netctx->onenet_socket_mutex);
         return CIS_RET_OK;
    }

    return CIS_RET_ERROR;
}

cis_ret_t cisnet_free(cisnet_t netctx,uint8_t* buffer,uint32_t length)
{
	(void) netctx;
	(void) length;

	cissys_free(buffer);
	return CIS_RET_OK;
}

#endif
