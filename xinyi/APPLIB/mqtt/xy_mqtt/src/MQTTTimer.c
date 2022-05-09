/****************************************************************************************************
Copyright:   2018-2020, XinYi Info Tech Co.,Ltd.
File name:   MQTTTimer.c
Description: MQTT protocol API function
Author:  gaoj
Version:
Date:    2020.7.20
History:
 ****************************************************************************************************/
#include "MQTTClient.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
void xy_gettimeofday(struct timeval *time)
{
    time->tv_sec  = xy_rtc_get_sec(0);
    time->tv_usec  = 0;
}

void xy_timersub(struct timeval *a, struct timeval *b, struct timeval *res)
{
    long tv_sec = 0;
    long tv_usec = 0;

    tv_usec = a->tv_usec - b->tv_usec;
    if(tv_usec < 0)
    {
        tv_usec += 1000000;
        tv_sec -= 1;
    }
    tv_sec = tv_sec + a->tv_sec - b->tv_sec;

    res->tv_sec  = tv_sec;
    res->tv_usec = tv_usec;
}

void xy_timeradd(struct timeval *a, struct timeval *b, struct timeval *res)
{
    long tv_sec = 0;
    long tv_usec = 0;

    tv_usec = a->tv_usec + b->tv_usec;
    if(tv_usec >= 1000000)
    {
        tv_usec -= 1000000;
        tv_sec += 1;
    }
    tv_sec = tv_sec + a->tv_sec + b->tv_sec;

    res->tv_sec  = tv_sec;
    res->tv_usec = tv_usec;
}

void TimerInit(Timer* timer)
{
    timer->end_time = (struct timeval){0, 0};
}

char TimerIsExpired(Timer* timer)
{
    struct timeval now, res;
    xy_gettimeofday(&now);
    xy_timersub(&timer->end_time, &now, &res);

    return (res.tv_sec < 0) || (res.tv_sec == 0 && res.tv_usec <= 0);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout)
{
    struct timeval now;
    xy_gettimeofday(&now);
    struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
    xy_timeradd(&now, &interval, &timer->end_time);
}


void TimerCountdown(Timer* timer, unsigned int timeout)
{
    struct timeval now;
    xy_gettimeofday(&now);
    struct timeval interval = {timeout, 0};
    xy_timeradd(&now, &interval, &timer->end_time);
}


int TimerLeftMS(Timer* timer)
{
    struct timeval now, res;
    xy_gettimeofday(&now);
    xy_timersub(&timer->end_time, &now, &res);
    return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}


int xy_mqtt_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 100;
    }

    setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

    int bytes = 0;
    while (bytes < len)
    {
        int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
        if (rc == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
              bytes = -1;
            break;
        }
        else if (rc == 0)
        {
            bytes = 0;
            break;
        }
        else
            bytes += rc;
    }
    return bytes;
}


int xy_mqtt_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval tv;

    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

    setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(struct timeval));
    int rc = write(n->my_socket, buffer, len);
    return rc;
}


void NetworkInit(Network* n)
{
    n->my_socket = -1;
    n->mqttread = xy_mqtt_read;
    n->mqttwrite = xy_mqtt_write;
}


int NetworkConnect(Network* n, char* addr, unsigned short port)
{
    int type = SOCK_STREAM;
    struct sockaddr_in address;
    int rc = -1;
    char serverPort[6];
    sa_family_t family = AF_INET;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

    n->my_socket = socket(family, type, 0);

    sprintf(serverPort, "%d", port);

    if ((rc = getaddrinfo(addr, serverPort, &hints, &result)) == 0)
    {
        struct addrinfo* res = result;

        /* prefer ip4 addresses */
        while (res)
        {
            if (res->ai_family == AF_INET)
            {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET)
        {
            address.sin_port = htons(port);
            address.sin_family = family = AF_INET;
            address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
        }
        else
            rc = -1;

        freeaddrinfo(result);
    }

    if (rc == 0)
    {
        if (n->my_socket != -1)
            rc = connect(n->my_socket, (struct sockaddr*)&address, sizeof(address));
        else
            rc = -1;
    }

    return rc;
}


void NetworkDisconnect(Network* n)
{
    close(n->my_socket);
}

