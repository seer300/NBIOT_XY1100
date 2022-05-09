#include "ipcmsg.h"
#include "sema.h"

#include "xy_utils.h"
#include "xy_system.h"
#include "low_power.h"
#include "dsp_process.h"
#include "xy_flash.h"

#define min(x,y) ({ x < y ? x : y; })


T_IpcMsg_Resource IpcMsg_Resource_Tbl[IpcMsg_Channel_MAX] = 
{	
	{IpcMsg_LOG,sizeof(void*),96,2,0},
	{IpcMsg_Normal,sizeof(void*),64,3,0},
	{IpcMsg_Dump,sizeof(void*),4,3,0},
	{IpcMsg_Flash,sizeof(void*),8,2,0},
};

T_IpcMsg_ChInfo g_IpcMsg_ChInfo[IpcMsg_Channel_MAX] = {0};

T_RingBuf_Ctl g_ringbuf_ctl[IpcMsg_Channel_MAX] = {0};


volatile unsigned char g_icminitflag = 0;

static unsigned int get_ipcmsg_buf_len(unsigned int ch_id)
{
	int i;
	for(i=0;i<IpcMsg_Channel_MAX;i++)
	{
		if(ch_id == IpcMsg_Resource_Tbl[i].Ch_ID)
			return (IpcMsg_Resource_Tbl[i].block_cnt)*(IpcMsg_Resource_Tbl[i].block_size + sizeof(T_IpcMsg_Head));

	}
	xy_assert(!"[IPCMSG]get_ipcmsg_buf_len fail !");	
	return 0;
}
int Ipc_SetInt(char ch_id)
{

	//HWREG(ARM_DSP_MSG_STATE) |= (1 << ch_id);
	if(ch_id!=IpcMsg_LOG)
	{		
		HWREG(PRCM_BASE + 0x1c)|= 0x02;
	}

 	return 0;
 }
volatile unsigned long g_icm_flag = 0;
void IpcInthandler()
{

	HWREGB(PRCM_BASE + 0x1C) &= 0xFE;
#if 0	
	for(i=0;i<IpcMsg_Channel_MAX;i++)
	{

		IpcMsg_Semaphore_Give(&(g_IpcMsg_ChInfo[i].read_sema));

	}
#endif
	if(g_icm_flag==0)
		IpcMsg_Semaphore_Give(&(g_IpcMsg_ChInfo[IpcMsg_Normal].read_sema));
}

int IpcMsg_Read(T_IpcMsg_Msg *pMsg,unsigned long xTicksToWait)
{
	//unsigned int Recv_len = 0;
	T_RingBuf *ringbuf_rcv = g_IpcMsg_ChInfo[pMsg->chID].RingBuf_rcv;
	
	T_IpcMsg_Head tmpMsgHeader;
	

	unsigned int result_len = 0;	
	
	if(Is_Ringbuf_Empty(ringbuf_rcv) == true)
	{		
		g_icm_flag=0;
		IpcMsg_Semaphore_Take(&(g_IpcMsg_ChInfo[pMsg->chID].read_sema),xTicksToWait);
		g_icm_flag=1;
	}
	
	if(Is_Ringbuf_Empty(ringbuf_rcv) == true)
	{		
		return 0;
	}

	ringbuf_rcv = &g_ringbuf_ctl[pMsg->chID].rcv_crl;
	ringbuf_rcv->Read_Pos = g_IpcMsg_ChInfo[pMsg->chID].RingBuf_rcv->Read_Pos;
	ringbuf_rcv->Write_Pos = g_IpcMsg_ChInfo[pMsg->chID].RingBuf_rcv->Write_Pos;
	
	/* get msg header */
	ipcmsg_ringbuf_read(ringbuf_rcv,&tmpMsgHeader,sizeof(T_IpcMsg_Head),sizeof(T_IpcMsg_Head));
	//Recv_len = ALIGN_IPCMSG(tmpMsgHeader.data_len+sizeof(T_IpcMsg_Head),CACHE_LINE)-sizeof(T_IpcMsg_Head);
	if (pMsg->len < tmpMsgHeader.data_len)
	{		
		xy_assert(0);
		return -1;
	}
	pMsg->flag = tmpMsgHeader.data_flag;
	result_len = tmpMsgHeader.data_len;
	ipcmsg_ringbuf_read(ringbuf_rcv,pMsg->buf,tmpMsgHeader.data_len,ALIGN_IPCMSG(tmpMsgHeader.data_len+sizeof(T_IpcMsg_Head),CACHE_LINE)-sizeof(T_IpcMsg_Head));	

	g_IpcMsg_ChInfo[pMsg->chID].RingBuf_rcv->Read_Pos = ringbuf_rcv->Read_Pos;
	
	return result_len;


}

//extern volatile unsigned long g_standby_dsp_ready;
#if INTERCORE_DEBUG
volatile unsigned int ipcdbg[50][2];
volatile unsigned int ipccnt=0;
volatile unsigned int ipcdbg2[50][2];
volatile unsigned int ipccnt2=0;
#define RINGALIGN(a) (((a)+ 31)&~(31))
#endif
int IpcMsg_Write(T_IpcMsg_Msg *pMsg)
{
	unsigned int size=0;
	T_RingBuf *ringbuf_send ;
		
	T_IpcMsg_Head tmpMsgHeader;
	
	xy_assert(pMsg->len != 0);
	//must wait DSP core have inited
	if(g_icminitflag!=1)
		return -1;

	size = pMsg->len+ sizeof(T_IpcMsg_Head);
	size = ALIGN_IPCMSG(size,CACHE_LINE);

	if(pMsg->flag != ICM_FLASHWRITE)
		IpcMsg_Mutex_Lock(&(g_IpcMsg_ChInfo[pMsg->chID].write_mutex),IPC_WAITFROEVER);	

	ringbuf_send = &g_ringbuf_ctl[pMsg->chID].send_crl;
	ringbuf_send->Read_Pos = g_IpcMsg_ChInfo[pMsg->chID].RingBuf_send->Read_Pos;
	ringbuf_send->Write_Pos = g_IpcMsg_ChInfo[pMsg->chID].RingBuf_send->Write_Pos;
	//Send_addr = ringbuf_send->base_addr+ringbuf_send->Write_Pos;
	
#if INTERCORE_DEBUG
	if(pMsg->chID == IpcMsg_LOG)
	{
		ipcdbg[ipccnt%50][0] = ringbuf_send->Write_Pos;
		ipcdbg[ipccnt%50][1] = pMsg->len;
		ipccnt++;
	}
#endif
/*
	// 防止DSP进入睡眠时，M3持续发送log消息，可能导致的看门狗断言
	if(pMsg->chID == IpcMsg_LOG)
	{
		// log的核间通道剩余小于4条消息时，需要唤醒DSP的睡眠
		// 目前log核间通道最多发送24条消息
		if (Is_Ringbuf_ChFreeSpace(ringbuf_send, 4 * CACHE_LINE) != true){
			HWREG(PRCM_BASE + 0x1c)|= 0x02;
		}
	}
*/
	if (Is_Ringbuf_ChFreeSpace(ringbuf_send, size) == true){
#if INTERCORE_DEBUG
		if(pMsg->chID == IpcMsg_LOG)
		{
			ipcdbg2[ipccnt2%50][0] = ringbuf_send->Write_Pos;
			ipcdbg2[ipccnt2%50][1] = pMsg->len;
			if(ipccnt2 >=1 )
			{
				if( (ipcdbg2[(ipccnt2 - 1)%50][0] + RINGALIGN(ipcdbg2[(ipccnt2 - 1)%50][1] + sizeof(T_IpcMsg_Head))) % (IpcMsg_Resource_Tbl[IpcMsg_LOG].block_cnt*(IpcMsg_Resource_Tbl[IpcMsg_LOG].block_size + sizeof(T_IpcMsg_Head))) != ipcdbg2[ipccnt2%50][0])
					xy_assert(0);
			}
			ipccnt2++;
		}
#endif
		tmpMsgHeader.data_flag = (unsigned short)(pMsg->flag);
		tmpMsgHeader.data_len = (unsigned short)(pMsg->len); 
	
		ipcmsg_ringbuf_write(ringbuf_send,(void*)&tmpMsgHeader,sizeof(T_IpcMsg_Head));
		if(pMsg->len)
			ipcmsg_ringbuf_write(ringbuf_send,pMsg->buf,size-sizeof(T_IpcMsg_Head));	
	}
	else{
		if(pMsg->flag != ICM_FLASHWRITE)
			IpcMsg_Mutex_Unlock(&(g_IpcMsg_ChInfo[pMsg->chID].write_mutex));
		return -1;
	}
	
	g_IpcMsg_ChInfo[pMsg->chID].RingBuf_send->Write_Pos = ringbuf_send->Write_Pos;

	// 除了LOG以外的所有核间消息都会唤醒DSP，以便消息及时处理
	Ipc_SetInt(pMsg->chID);
	if(pMsg->flag != ICM_FLASHWRITE)
		IpcMsg_Mutex_Unlock(&(g_IpcMsg_ChInfo[pMsg->chID].write_mutex));
	
	return (pMsg->len);
}

void ringbuf_ctl_init(T_RingBuf* pringbuf_ctl,T_RingBuf*pIpcMsg_ChInfo)
{
	pringbuf_ctl->base_addr = pIpcMsg_ChInfo->base_addr;
	pringbuf_ctl->size = pIpcMsg_ChInfo->size;
	pringbuf_ctl->Read_Pos = pIpcMsg_ChInfo->Read_Pos;
	pringbuf_ctl->Write_Pos = pIpcMsg_ChInfo->Write_Pos;
}
void IpcMsg_Buf_init(void)
{
	int i,size;
	unsigned long IpcMsg_Addr_tmp = (unsigned long)ALIGN_IPCMSG(IPCMSG_BUF_BASE_ADDR,CACHE_LINE);

	for(i=0;i<IpcMsg_Channel_MAX;i++)
	{
		size = get_ipcmsg_buf_len(i);
		
		if((IpcMsg_Resource_Tbl[i].direction&0x1) == 1)
		{
			g_IpcMsg_ChInfo[i].RingBuf_rcv = (T_RingBuf*)IpcMsg_Addr_tmp;
			g_IpcMsg_ChInfo[i].RingBuf_rcv->base_addr = IpcMsg_Addr_tmp + sizeof(T_RingBuf);
			g_IpcMsg_ChInfo[i].RingBuf_rcv->size = size;
			g_IpcMsg_ChInfo[i].RingBuf_rcv->Read_Pos = 0;		
			g_IpcMsg_ChInfo[i].RingBuf_rcv->Write_Pos = 0;

			ringbuf_ctl_init(&g_ringbuf_ctl[i].rcv_crl,g_IpcMsg_ChInfo[i].RingBuf_rcv);
				
			IpcMsg_Addr_tmp += ALIGN_IPCMSG((sizeof(T_RingBuf) + get_ipcmsg_buf_len(i)),CACHE_LINE);
			IpcMsg_Semaphore_Create(&(g_IpcMsg_ChInfo[i].read_sema));
		}

		if((IpcMsg_Resource_Tbl[i].direction&0x2) == 2)
		{
			g_IpcMsg_ChInfo[i].RingBuf_send = (T_RingBuf*)IpcMsg_Addr_tmp;
			g_IpcMsg_ChInfo[i].RingBuf_send->base_addr = IpcMsg_Addr_tmp + sizeof(T_RingBuf);
			g_IpcMsg_ChInfo[i].RingBuf_send->size = size;
			g_IpcMsg_ChInfo[i].RingBuf_send->Read_Pos = 0;
			g_IpcMsg_ChInfo[i].RingBuf_send->Write_Pos = 0;

			ringbuf_ctl_init(&g_ringbuf_ctl[i].send_crl,g_IpcMsg_ChInfo[i].RingBuf_send);
			
			IpcMsg_Addr_tmp += ALIGN_IPCMSG((sizeof(T_RingBuf) + get_ipcmsg_buf_len(i)),CACHE_LINE);
			IpcMsg_Mutex_Create(&(g_IpcMsg_ChInfo[i].write_mutex));
		}
		
		g_IpcMsg_ChInfo[i].flag = i;
		
		if(IpcMsg_Addr_tmp - IPCMSG_BUF_BASE_ADDR > IPCMSG_BUF_LEN)
			xy_assert(0);

		g_icminitflag = 1;

	}
	
}

void dsp_suspend()
{
	T_RingBuf *ringbuf_send = &g_ringbuf_ctl[IpcMsg_Dump].send_crl;
	//unsigned int intsave;
	if(!DSP_IS_NOT_LOADED())
	{

		vTaskSuspendAll();
		HWREG(DSP_SUSPEND)=0x66776677;

		ringbuf_send->Write_Pos++;

		g_IpcMsg_ChInfo[IpcMsg_Dump].RingBuf_send->Write_Pos = ringbuf_send->Write_Pos;
		HWREG(PRCM_BASE + 0x1c)|= 0x02;

		do{
		}while(HWREG(DSP_SUSPEND)!=0x88998899);
		//g_DSP_state = DSP_STATE_STOPPED;
		g_have_suspend_dsp = 1;
		( void ) xTaskResumeAll();
	}

}

#define Align_IPCMSG(size , align)  (((unsigned int )size + align - 1) & (~(align - 1)))
unsigned long Current_Stack_Addr;
void Dump_Notice()
{
	extern uint32_t _Flash_Used;
	extern uint32_t _Ram_Text;
	extern uint32_t _Ram_Data;
	extern uint32_t _Ram_Bss;
	extern uint32_t __Main_Stack_Size;

	T_RingBuf *ringbuf_send = &g_ringbuf_ctl[IpcMsg_Dump].send_crl;

	if(g_icminitflag!=1)
		return;

	dynamic_load_up_dsp(1);
			
	if(g_softap_fac_nv->dump_mem_into_flash== 1)
	{
		// flashdumpsize: flash中可用的剩余空间
		uint32_t flashdumpsize = ARM_FLASH_BASE_LEN + FOTA_BACKUP_LEN_MAX - (uint32_t)&_Flash_Used -  (uint32_t)&_Ram_Text - (uint32_t)&_Ram_Data - 0x1000;

		// 判断剩余空间能够满足dump flash功能所需
		if(flashdumpsize >= (uint32_t)&_Ram_Text + (uint32_t)&_Ram_Data + (uint32_t)&_Ram_Bss + (uint32_t)&__Main_Stack_Size + RAM_ICM_BUF_LEN + RAM_XTEND_SBUF_LEN +0x3000)
		{
			TaskStatus_t StatusArray = {0};

			vTaskGetInfo(NULL, &StatusArray, pdTRUE, eInvalid );
			Current_Stack_Addr = (unsigned long)(StatusArray.pxStackBase);

			// 记录ARM data/BSS总大小
			*(unsigned int *)g_IpcMsg_ChInfo[IpcMsg_Dump].RingBuf_send->base_addr 	 	= (unsigned int)&_Ram_Text + (uint32_t)&_Ram_Data + (uint32_t)&_Ram_Bss;
			// 记录主栈大小
			*((unsigned int *)g_IpcMsg_ChInfo[IpcMsg_Dump].RingBuf_send->base_addr + 1) = (unsigned int)&__Main_Stack_Size;
			// 记录当前线程栈起始地址
			*((unsigned int *)g_IpcMsg_ChInfo[IpcMsg_Dump].RingBuf_send->base_addr + 2) = (unsigned int)(StatusArray.pxStackBase);
			//记录dump flash的起始地址
			*((unsigned int *)g_IpcMsg_ChInfo[IpcMsg_Dump].RingBuf_send->base_addr + 3) = Align_IPCMSG((Align_IPCMSG((uint32_t)&_Flash_Used, 0x1000) + (uint32_t)&_Ram_Text + (uint32_t)&_Ram_Data),0x1000);
			ringbuf_send->Write_Pos += 16;
		}	
	}
	else
	{
		ringbuf_send->Write_Pos++;			// Notice without dumping into flash
	}
	
	g_IpcMsg_ChInfo[IpcMsg_Dump].RingBuf_send->Write_Pos = ringbuf_send->Write_Pos;
	HWREGB(0xA0110002) = 0x03;
	HWREG(PRCM_BASE + 0x1c)|= 0x02;
}

int icpmsg_ringbuf_write_remain_size(int ch_id)
{
	unsigned int Write_Pos = g_IpcMsg_ChInfo[ch_id].RingBuf_send->Write_Pos;
	unsigned int Read_Pos = g_IpcMsg_ChInfo[ch_id].RingBuf_send->Read_Pos;
	unsigned int ringbuf_size = g_IpcMsg_ChInfo[ch_id].RingBuf_send->size;

	if (Write_Pos < Read_Pos)
    	return (Read_Pos - Write_Pos);
	else
		return (ringbuf_size - Write_Pos + Read_Pos);
}



