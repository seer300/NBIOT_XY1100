#ifndef _ONENET_BACKUP_PROC_H
#define _ONENET_BACKUP_PROC_H
#include "at_onenet.h"
#include "rtc_tmr.h"
#include "cloud_utils.h"

#define SET_ONENET_REGINFO_PARAM(param,value)  \
        {  \
            if(g_onenet_regInfo != NULL ) \
            {  \
                g_onenet_regInfo->param = value;  \
            } \
        }

#define GET_ONENET_REGINFO_PARAM(param) ({(g_onenet_regInfo != NULL)? g_onenet_regInfo->param : -1; })

extern osThreadId_t g_onenet_exception_recovery_TskHandle;
extern osThreadId_t g_onenet_resume_TskHandle;
extern osThreadId_t g_onenet_rtc_resume_TskHandle;

typedef struct lwm2m_common_user_config_s
{
    int bootstrap_flag;
    char server_host[150];
    char endpoint_name[150];
    char server_ip[16];
    int port;
    int lifetime;
    int security_mode;
    char pading[2];
    char psk_id[150];
    char psk[256];
    int binding_mode;

    int ack_timeout;
    int retrans_max_times;
    int is_auto_ack;
    int access_mode;
    int access_mode_alternative;
    int platform;

    int recovery_mode;
    int lifetime_enable;
    int dtls_mode;
    int dtls_version;
} lwm2m_common_user_config_t;

typedef struct
{
    net_infos_t  net_info;
    unsigned int ref;						//onenet context ref index
    int flag;								//resume flag
    unsigned char endpointname[34];			//endpointname
    unsigned char location[24];				//server location
    unsigned int object_count;				//context object count
    onenet_object_t onenet_object[OBJECT_BACKUP_MAX];	//context object list
    // only record the security instance that describes lwm2m server, basically there is only one
    onenet_security_instance_t onenet_security_instance;	//std secruity instance
    unsigned int observed_count;			//observe count
    onenet_observed_t observed[OBSERVE_BACKUP_MAX];		//observe list
    int last_update_time;					//the newest update time
    int life_time;							//onenet life time
    unsigned short nextMid;				    //the newest message id
    uint8_t cloud_platform;				    //cloud_platform type:[0]onenet[1]common
    uint8_t platform_common_type;				//common_cloud_platform type:[0]yanfei[1]andlink
    lwm2m_common_user_config_t lwm2mUserConfig; //common_cloud_platform user config
    onenet_user_config_t onenet_user_config;    //onenet user config
} onenet_regInfo_t;

extern onenet_regInfo_t *g_onenet_regInfo;

void onenet_resume_task();
void onenet_exception_recovery_process();
void onenet_rtc_resume_process();
void onenet_netif_up_resume_process();
void onenet_keeplive_update_process();
rtc_timeout_cb_t onenet_notice_update_process(void *para);
void onenet_resume_state_process(int code);
#endif //#ifndef _ONENET_BACKUP_PROC_H

