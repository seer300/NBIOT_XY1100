#include "softap_macro.h"
#if IS_DSP_CORE
#include "softap_api.h"
#include "xy_log_send.h"
#include "debugprint.h"
#include "ItemStruct.h"
#include "xy_log.h"
#include "diag_out.h"
#include "diag_read_msg.h"
#include "os_adapt.h"
#include "ipcmsg.h"
#define XYLOG_STACK_SIZE 1024
#define XYLOG_MAXPRO_SIZE 20480 + 2048
#define IPCMSG_NORMAL_REMAIN_PRECENT (20)

extern u8 log_filter[XYLOG_MAX_BLOCK];
extern u8 primitive_filter[MAX_PS_BLOCK][MAX_MSG_ID]; 
extern u8 air_filter;
XY_MSGQUE xy_log_msg_q;
volatile int logview_alive = 1;//1 default,0 close log,2 receive tool command
volatile int log_fifo_full_cnt = 0;
osThreadId_t g_xylog_TskHandle = NULL;
osThreadId_t g_rxlog_TskHandle = NULL;
#else
#include "xy_log_send.h"
#include "ItemStruct.h"
#include "xy_log.h"
#include "diag_out.h"
#include "xy_utils.h"
#include "hw_memmap.h"
#include "replace.h"
#include "oss_nv.h"
#include "xy_system.h"
#include "inter_core_msg.h"
#include "TypeDefine.h"
#define XYLOG_STACK_SIZE 1024
#define XYLOG_MAX_SIZE 20480
#define XYLOG_MAXPRO_SIZE 20480 + 2048

osThreadId_t g_xylog_TskHandle = NULL;
int g_xylogtest_TskHandle = -1;
osMessageQueueId_t xy_log_msg_q = NULL;
volatile int xy_log_size = 0;
#endif



int xy_log_trace_msg_send(char *data, unsigned int buffer_len)
{ 

	if(buffer_len <= 0)
	{
		return LOG_FALSE;
	}

#if IS_DSP_CORE
    if (k_fifo_put(&xy_log_msg_q, &data) != XY_OK)
    {
    	log_fifo_full_cnt++;
		get_Seq_num();
    	return LOG_FALSE;
    }
#else
    if (osKernelGetState() != osKernelInactive && osMessageQueuePut(xy_log_msg_q, &data, 0, osNoWait) != osOK)
    {
       	return LOG_FALSE;
    }
#endif

	return LOG_TRUE;
}

void xy_log_trace(char *data, int item_len)
{
	int i = 0;
	char cData = 0x00;
	for(i = 0; i < item_len; i++)
	{
		cData ^= *(data + i);
	}

	data[item_len] = cData;
	data[item_len + 1] = 0xFA;
#if IS_DSP_CORE
	if((g_factory_nv->softap_fac_nv.open_log > 0 && g_factory_nv->softap_fac_nv.open_log < 4) || g_factory_nv->softap_fac_nv.open_log == 100)
	{
#else
	if((g_softap_fac_nv->open_log > 0 && g_softap_fac_nv->open_log < 4) || g_softap_fac_nv->open_log == 100)
	{
#endif
    	for (i = 0; i < item_len + DIAG_TAIL_LEN; i++)
		{
			CSPCharPut(CSP3_BASE, *(data + i));		
		}
	}

}



int Is_Log_For_DSP(ItemHeader_t * log_raw_msg)
{
	// 单核模式下由M3添加log头部信息，然后单核打印
	// 双核模式下由DSP添加log头部信息
	if(log_raw_msg->u32Header == 0xFEA55A)
		return 0;
	
	return 1;
}

void xy_log_entry(void)
{
#if IS_DSP_CORE
    long lRcvRet = XY_ERR;
    ItemHeader_t *xy_log_raw_msg = NULL;
	int item_len = 0;
	unsigned int remain_min_size = IPCMSG_NORMAL_REMAIN_PRECENT * ipcmsg_normal_size() / 100;

    while (1)
	{
		item_len = k_fifo_get(&xy_log_msg_q, (void *)(&xy_log_raw_msg), K_FOREVER);
		if(item_len > 0)
		{
        	item_len = xy_log_raw_msg->u16Len + HEADERSIZE + LENSIZE;
			xy_log_raw_msg->u16SeqNum = get_Seq_num();	
            xy_log_trace((char*)xy_log_raw_msg, item_len);
			xy_free(xy_log_raw_msg);
        }	
		/*让flash_dyn有机会运行*/
		if(icpmsg_ringbuf_write_remain_size(IpcMsg_Normal) < remain_min_size)
			osDelay(1);

		//if open_log == 100,do not check heart

		if(g_factory_nv->softap_fac_nv.open_log == 100)
		{
			continue;
		}

		if(g_factory_nv->softap_fac_nv.open_log == 1 && logview_alive == 1)
		{
			if(OSXY_GetTickCount() >= 30 * 1000)
			{
				logview_alive = 0;
			}
		}	
     
    }
#else
    ItemHeader_t *xy_log_raw_msg = NULL;
  	int item_len = 0;
     while (1)
  	{
     	osMessageQueueGet(xy_log_msg_q, (void *)(&xy_log_raw_msg), NULL, osWaitForever);

     	if((Is_Log_For_DSP(xy_log_raw_msg) == 1) && (g_softap_fac_nv->open_log != 2) && (g_softap_fac_nv->open_log != 3) && (g_softap_fac_nv->work_mode == 0 || (g_softap_fac_nv->work_mode == 1 && !DSP_IS_NOT_LOADED() ) || (g_softap_fac_nv->work_mode == 2 && !DSP_IS_NOT_LOADED() )))
     	{
     		if(shmmsg_write_channel(&xy_log_raw_msg,  4, ICM_AP_LOG_PRINT, IpcMsg_LOG) != XY_OK)
			 {
				 xy_free(xy_log_raw_msg);
			 }
     	}
     	else
     	{
       		if(xy_log_raw_msg != NULL && (1 != Is_Log_For_DSP(xy_log_raw_msg)))
       		{
			   item_len = xy_log_raw_msg->u16Len + HEADERSIZE + LENSIZE;
			   xy_log_trace((char*)xy_log_raw_msg, item_len);
            }
			xy_free(xy_log_raw_msg);
			xy_log_raw_msg = NULL;
     	}
  	}
#endif
}

unsigned int user_log_limit = 0;
int  xy_log_init_resource(void)
{
#if IS_DSP_CORE
    long ret = XY_ERR;
	
	memset(log_filter, 0xFF, XYLOG_MAX_BLOCK);	
	memset(primitive_filter, 0xFF, MAX_PS_BLOCK * MAX_MSG_ID);
	air_filter = 0xFF;
	
	
    ret = k_fifo_init(&xy_log_msg_q,XYLOG_MSG_Q_LENGTH);
    xy_log_size = 0;

    return ret;
#else
    long ret = XY_ERR;

	g_softap_fac_nv->log_length_limit = g_softap_fac_nv->log_length_limit < (31 - ucPortCountLeadingZeros(LOG_MAX_SIZE_LIMIT))? \
			g_softap_fac_nv->log_length_limit : 31 - ucPortCountLeadingZeros(LOG_MAX_SIZE_LIMIT);
	g_softap_fac_nv->log_length_limit = g_softap_fac_nv->log_length_limit > (31 - ucPortCountLeadingZeros(LOG_MIN_SIZE_LIMIT))? \
			g_softap_fac_nv->log_length_limit : 31 - ucPortCountLeadingZeros(LOG_MIN_SIZE_LIMIT);

	// 默认user_log_limit为50，可以使用NV设置，下次启动生效。
	user_log_limit = (g_softap_fac_nv->log_fifo_limit == 0) ?  50 : g_softap_fac_nv->log_fifo_limit;
	// xy_log_msg_q队列代表转发至log线程的消息数目
	//	M3侧的实际消息总数目为xy_log_msg_q队列长度加上核间log通道的长度，即74条消息（user_log_limit + 768 / 32）
   	xy_log_msg_q = osMessageQueueNew(user_log_limit, sizeof(void *), NULL);

   	xy_log_size = 0;
	

       return ret;
#endif
}

int xy_log_task()
{
#if IS_DSP_CORE
	if(is_print_log() != LOG_TRUE)
	{
		return 0;
	}
	
    U32 uwRet = 1;

    xy_log_init_resource();	

	//OSXY_TaskCreate_Index(tsk_xylog,NULL,NULL);
	g_xylog_TskHandle = osThreadNew(xy_log_entry, NULL, "xy_log_tk", 1*1024, osPriorityLow2);
	osThreadSetLowPowerFlag(g_xylog_TskHandle, osLowPowerProhibit);
	
	//OSXY_TaskCreate_Index(tsk_rxlog,NULL,NULL); 
	g_rxlog_TskHandle = osThreadNew(xy_diag_read, NULL, "diag_read", 2 * 1024, osPriorityLow1);
	osThreadSetLowPowerFlag(g_rxlog_TskHandle, osLowPowerProhibit);
#else
	osThreadAttr_t thread_attr = {0};

	if(g_softap_fac_nv->open_log!=0 && g_xylog_TskHandle == NULL)
	{
		xy_log_init_resource();
		thread_attr.name	   = "xy_log";
		thread_attr.priority   = osPriorityBelowNormal;
		thread_attr.stack_size = 0x180;
		g_xylog_TskHandle = osThreadNew ((osThreadFunc_t)(xy_log_entry),NULL,&thread_attr);
	}

#endif
    return 0;
}
