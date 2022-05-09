#pragma once

/*******************************************************************************
 *							   Include header files						       *
 ******************************************************************************/
#include  "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define  XY_PING_MAX_DATA_LENGTH    1500
#define  PING_THREAD_STACK_SIZE     0x600   //ping thread stack size, is different than its in dsp core 
#define  PING6_THREAD_STACK_SIZE    0xc00   //ipV6 ping thread stack size, is different than its in dsp core 

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct
{
    char  host[REMOTE_SERVER_LEN];
    int   data_len;
	int   ping_num;
    int   time_out;
    int   interval_time;
	int   ip_type;
	int   rai_val;
} ping_arguments_t;

typedef struct
{
	int ping_send_num;
	int ping_reply_num;
	int longest_rtt;
    int shortest_rtt;
	int time_average;
} ping_info_t;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern int g_ping_stop;
extern osThreadId_t at_ping_thread_id;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
void process_ping(ping_arguments_t *ping_arguments);
int stop_ping();
int start_ping(int ip_type, char *host_ip, int data_len, int ping_num, int time_out, int interval_time, int rai_val);
int send_packet_to_ping(char *data, int len);


