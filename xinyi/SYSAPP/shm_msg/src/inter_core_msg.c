#include "flash_vendor.h"
#include "at_global_def.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "rtc_tmr.h"
#include "ps_netif_api.h"
#include "zero_copy_shm.h"
#include "inter_core_msg.h"
#include "shm_msg_api.h"
#include "flash_adapt.h"
#include "low_power.h"
#include "smartcard_proxy.h"
#include "xy_proxy.h"
#if XY_SOCKET_PROXY
#include "socket_proxy.h"
#endif

int g_flow_control_below_size = 256;

static int is_dropped_service(T_IpcMsg_Msg *pMsg)
{
	if(pMsg->flag == ICM_IPDATA || pMsg->flag == ICM_WIRESHARKDATA || pMsg->flag == ICM_AP_LOG_PRINT)
		return 1;

	return 0;
}

static int is_log_service(T_IpcMsg_Msg *pMsg)
{
	if(pMsg->flag == ICM_AP_LOG_PRINT)
		return 1;

	return 0;
}

static int flow_control_service(T_IpcMsg_Msg *pMsg)
{
	if((pMsg->flag == ICM_IPDATA || pMsg->flag == ICM_WIRESHARKDATA) && icpmsg_ringbuf_write_remain_size(pMsg->chID) <= g_flow_control_below_size)
		return 1;
	return 0;
}

//the caller need to free the memory--buf
volatile unsigned int time_begin = 0; //debug info
volatile unsigned int time_end = 0;   //debug info
volatile unsigned int log_send_num = 0;
int shmmsg_write_channel(void* buf, int len, unsigned int msg_type, int chID)
{
	unsigned int send_num = 0;
	//write  shm
	T_IpcMsg_Msg pMsg = {0};
	volatile unsigned int cycle = 0;
	static unsigned int dropped_num = 0;
	
	volatile unsigned int tb = 0; //begin write tick
	volatile unsigned int te = 0; //end write tick
	log_send_num = 0;

	if(buf == NULL || len <= 0 || chID < 0 || chID >= IpcMsg_Channel_MAX)
		xy_assert(0);
	
	if(DSP_IS_NOT_LOADED())
	{
		return XY_ERR;
	}
	
    pMsg.buf = buf;
    pMsg.chID = (unsigned char)chID;
    pMsg.flag = msg_type;
    pMsg.len = len;

	if(flow_control_service(&pMsg))
	{
		softap_printf(USER_LOG, WARN_LOG, "flow_control start!!");
		dropped_num++;
		return XY_ERR;
	}

	//must wait DSP core have inited
    while(IpcMsg_Write(&pMsg) < 0)
    {
    	if(is_dropped_service(&pMsg))
    	{
    		dropped_num++;
			return XY_ERR;
    	}
		//DSP发送来的AT命令采用零拷贝方式，由inter_core_msg线程处理后跨核通知DSP释放，进而当写通道满时会堵住inter_core_msg线程，无法进行读通道处理，进而造成DSP核的写通道满；
		//为了解决此情况，inter_core_msg线程不能执行写通道动作，交由xy_proxy处理。
		else if(strcmp(osThreadGetName(NULL),"inter_core_msg") == 0 && msg_type == ICM_FREE) //special operation for memory free of zero copy downlink
		{
			send_msg_2_proxy(PROXY_MSG_INTER_CORE_WAIT_FREE, pMsg.buf, 4);
			return XY_OK;
		}
		if(send_num == 0)  //debug info
		{
			tb = osKernelGetTickCount();			
			time_begin = tb;
		}

		// M3侧log请求不能轻易丢失，此处使用调试全局用以记录
		if(is_log_service(&pMsg))
		{
			log_send_num++;
		}
		else
		{
	    	send_num++;
		}

		if (send_num > 200) //debug info
		{
			te = osKernelGetTickCount();
			time_end = te;
			xy_assert(0); //10s assert
		}
		// 写flash操作时M3侧将锁调度，故此处使用for循环空跑等待，而非osdelay
		// delay about 50ms,have tested
		for(cycle = 0; cycle < 12500; cycle++);
    }
	return XY_OK;
}

void zero_cp_transfer(void ** pbuf, int len)
{
	char *buf = *(char **)pbuf;
	char *tmp_buf;

	if(((unsigned long)buf) >= DSP_HEAP_DRAM_START && ((unsigned long)buf) <= (DSP_HEAP_DRAM_START + DSP_HEAP_DRAM_LEN))
	{
		/* dsp传过来的字符串len不包括'\0',在这里需要添加'\0' */
		tmp_buf = xy_zalloc(len + 1);
		DMAChannelTransfer_withMutex((unsigned long)tmp_buf,(unsigned long)buf, len, DMA_CHANNEL_2);
		tmp_buf[len] = '\0';
		/* notify DSP to free mem */
		xy_free(buf);
		*pbuf = tmp_buf;
	}
	else
	{
		*pbuf = (void *)(get_ARM_Addr_from_Dsp((unsigned int)buf));
	}
}



extern unsigned int g_log_size;
void inter_core_msg_entry()
{
	int rcv_len = SHM_DATA_MAX;
	int ret = -1;
	T_IpcMsg_Msg Msg;
	char *rcv_buf;
	Msg.chID = IpcMsg_Normal;
	void * buf;
	int len;

	while(1)
	{
		rcv_buf = xy_zalloc(rcv_len);
		Msg.len = rcv_len;
		Msg.buf = rcv_buf;
		Msg.flag = 0;
		ret = IpcMsg_Read(&Msg, IPC_WAITFROEVER);

		if( ret > 0 )
		{
			T_zero_copy *zero_msg = (T_zero_copy *)rcv_buf;
			switch(Msg.flag)
			{
				case ICM_LPM:
					lpm_msg_process(Msg.buf);
				break;
				
				case ICM_AT:
					xy_assert(g_at_ctrl_cbk.recv_from_nearps_cbk != NULL);
					zero_cp_transfer((void**)&(zero_msg->buf), zero_msg->len);
					g_at_ctrl_cbk.recv_from_nearps_cbk((void*)zero_msg->buf, zero_msg->len);
				break;
				
				case ICM_FREE:
				{
					xy_assert(ret==4);
					xy_free((void *)(get_ARM_Addr_from_Dsp(*(int *)rcv_buf)));
				}
				break;	

				case ICM_SHMMEM_ALLOC:
					shmmem_malloc_msg_process(*(int *)rcv_buf);
				break;

				case ICM_SHMMEM_GIVE:
					shmmem_give_msg_process(*(int *)rcv_buf);
				break;
				
				case ICM_IPDATA:
					buf = ((struct ipdata_info *)rcv_buf)->data;
					len = ((struct ipdata_info *)rcv_buf)->data_len;
					zero_cp_transfer(&buf, len);
					((struct ipdata_info *)rcv_buf)->data = buf;
					send_packet_to_user(0,sizeof(struct ipdata_info), rcv_buf);
				break;
#if XY_SOCKET_PROXY				
				case ICM_SOCKPROXY:
					zero_cp_transfer((void**)&(zero_msg->buf), zero_msg->len);
					send_msg_to_sockproxy((void *)zero_msg->buf, zero_msg->len);
				break;
#endif
				case ICM_SMARTCARDPROXY:
					softap_printf(USER_LOG, WARN_LOG, "recv smartcardproxy from dsp!!");
					zero_cp_transfer((void**)&(zero_msg->buf), zero_msg->len);
					send_msg_to_smartcardproxy((void *)zero_msg->buf,zero_msg->len);
					xy_free((void*)zero_msg->buf);
				break;
				
				case ICM_FLASHWRT_NOTICE:
					diffcore_flashwrt_msg_proc();
				break;	
				
				case ICM_XINYI_SHM_MSG:
					rcv_shm_msg_process(Msg.buf);
				break;

				case ICM_PS_SHM_MSG:  //see  send_ps_shm_msg
					zero_cp_transfer((void**)&(zero_msg->buf), zero_msg->len);
					SendShmMsg2AtcAp((void*)zero_msg->buf, zero_msg->len);
				break;
				
				default:
					xy_assert(0);
				break;
			}	
		}
		xy_free(rcv_buf);
	}
}

void icm_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "inter_core_msg";
	thread_attr.priority   = osPriorityNormal3;
	thread_attr.stack_size = 0x400;
	osThreadNew ((osThreadFunc_t)(inter_core_msg_entry),NULL,&thread_attr);
}


