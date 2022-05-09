/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_proxy.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "inter_core_msg.h"
#include "ipcmsg.h"
#include "ps_netif_api.h"
#include "xy_utils.h"
#if TELECOM_VER || MOBILE_VER
#include "cloud_utils.h"
#endif
#if TELECOM_VER
#include "cdp_backup.h"
#endif
#if MOBILE_VER
#include "onenet_backup_proc.h"
#endif

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define XY_PROXY_THREAD_PRIO osPriorityNormal2
#define XY_PROXY_THREAD_STACKSIZE 0xC00

/*******************************************************************************
 *						   Global variable definitions				           *
 ******************************************************************************/
osMessageQueueId_t xy_proxy_msg_q = NULL; //proxy msg queue
osMutexId_t xy_proxy_mutex = NULL;

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
void xy_proxy_ctl(void)
{
    //公有云FOTA升级后的信息上报操作
#if TELECOM_VER || MOBILE_VER
#if XY_FOTA
    cloud_fota_init();
#endif
#endif
	
	xy_proxy_msg_t *msg = NULL;
	osThreadAttr_t thread_attr = {0};
	
	while(1) 
	{
		osMessageQueueGet(xy_proxy_msg_q, (void *)(&msg), NULL, osWaitForever);
		switch(msg->msg_id)
		{
			case PROXY_MSG_AT_RECV:
				proc_at_proxy_req((xy_proxy_msg_t *)msg);
				break;
			case PROXY_MSG_PS_PDP_ACT:
			{
				ps_netif_activate((struct pdp_active_info *)(msg->data));
				NETDOG_AT_STATICS(dbg_pdp_act_succ++);
			}
				break;
			//always,not permit user  to deactivate PDP,if receive PS CERV DEACT,do PDP act by dead cycle
			case PROXY_MSG_PS_PDP_DEACT:
			{
				ps_netif_deactivate(*((unsigned char *)msg->data), IPV4_TYPE);
				NETDOG_AT_STATICS(dbg_pdp_deact_num++);
				break;
			}
			case PROXY_MSG_INTER_CORE_WAIT_FREE:
			{
				shm_msg_write(msg->data, 4, ICM_FREE);			
				break;
			}
#if TELECOM_VER
			case PROXY_MSG_RESUME_CDP_UPDATE:
			{
                if (g_cdp_rtc_resume_TskHandle == NULL)
                {
					thread_attr.name	   = "cdp_keelive_update";
					thread_attr.priority   = XY_OS_PRIO_NORMAL1;
					thread_attr.stack_size = 0x400;
                    g_cdp_rtc_resume_TskHandle = osThreadNew((osThreadFunc_t)(cdp_rtc_resume_update_process), NULL, &thread_attr);
                }

				break;
			}
#endif
#if MOBILE_VER
			case PROXY_MSG_RESUME_ONENET_UPDATE:
			{
	            if (g_onenet_rtc_resume_TskHandle == NULL)
	            {
					thread_attr.name	   = "onenet_keelive_update";
					thread_attr.priority   = XY_OS_PRIO_NORMAL1;
					thread_attr.stack_size = 0x400;
	                g_onenet_rtc_resume_TskHandle = osThreadNew((osThreadFunc_t)(onenet_rtc_resume_process), NULL, &thread_attr);
	            }
                break;
			}
#endif
			default:
				break;
		}
		xy_free(msg);
	}
}

int send_msg_2_proxy(int msg_id, void *buff, int len)
{
	osMutexAcquire(xy_proxy_mutex, osWaitForever);
	xy_assert(xy_proxy_msg_q != NULL);
	xy_proxy_msg_t *msg = NULL;
	
	msg = xy_malloc(sizeof(xy_proxy_msg_t) + len);
	msg->msg_id = msg_id;
	
	msg->size = len;

	if (buff != NULL)
		memcpy(msg->data, buff, len);
	osMessageQueuePut(xy_proxy_msg_q, &msg, 0, osWaitForever);
	osMutexRelease(xy_proxy_mutex);
	return XY_OK;
}

void xy_proxy_init(void)
{
	osThreadAttr_t thread_attr = {0};

	xy_proxy_msg_q = osMessageQueueNew(30, sizeof(void *), NULL);
	xy_proxy_mutex = osMutexNew(NULL);
	
	thread_attr.name	   = XY_PROXY_THREAD_NAME;
	thread_attr.priority   = XY_PROXY_THREAD_PRIO;
	thread_attr.stack_size = XY_PROXY_THREAD_STACKSIZE;
	osThreadNew((osThreadFunc_t)(xy_proxy_ctl), NULL, &thread_attr);
}
