
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "smartcard.h"
#include "smartcard_proxy.h"
#include "osal.h"
#include "xy_utils.h"
#include "low_power.h"
#include "ipcmsg.h"
#include "xy_system.h"
#include "zero_copy_shm.h"

osMessageQueueId_t proxy_smartcard_msg_q = NULL;
unsigned int proxy_smartcard_task_handler;

int smartcard_write_to_cp(struct sc_response_msg * p_response_msg, int msglen)
{
	void *res_pkg = NULL;
	int ret = 0;
	if(DSP_IS_NOT_LOADED())
	{
		softap_printf(USER_LOG, WARN_LOG, "dsp not runned and packet dropped!!!");
		return 1;
	}
	res_pkg = xy_zalloc_Align(msglen);
	if(res_pkg == NULL)
		xy_assert(0);
	memcpy(res_pkg,p_response_msg,msglen);
	ret = shm_zero_copy(res_pkg,msglen,ICM_SMARTCARDPROXY);
	
	if(ret == -1)
		softap_printf(USER_LOG, WARN_LOG, "smartcard_write_to_cp fail");
	return 1;
}




void proc_sc7816cmd_msg(uint8_t *pApduBuf,uint32_t buf_len)
{

	volatile uint32_t expected_rsp_len;



	T_Command_APDU TxAPDU	=	{0};

	if(buf_len != 1)
	{
		TxAPDU.cla				=	pApduBuf[0];
		TxAPDU.ins				=	pApduBuf[1];
		TxAPDU.p1				=	pApduBuf[2];
		TxAPDU.p2				=	pApduBuf[3];
		
		if(buf_len==5)
		{
			TxAPDU.Lc		=	0;
			if(0==pApduBuf[4])
			{
				TxAPDU.Le	=	256;
			}
			else
			{
				TxAPDU.Le	=	(uint16_t)pApduBuf[4];
			}
		}
		else
		{
			TxAPDU.Lc		=	pApduBuf[4];
			TxAPDU.Le		=	0;
			TxAPDU.data		=	(uint8_t *)(pApduBuf+5);
		}
	}
	

	expected_rsp_len = TxAPDU.Le+2;
	
	struct sc_response_msg *p_response_msg =  xy_zalloc(sizeof(struct  sc_response_msg) + expected_rsp_len);
	
	xy_printf("sim proxy,cmd is=[%x,%x,%x,%x,%x,],cmd len=[%d]", pApduBuf[0],pApduBuf[1],pApduBuf[2],pApduBuf[3],pApduBuf[4], buf_len);
	
	SC7816_command(pApduBuf,p_response_msg->data,&buf_len);
	
	p_response_msg->rsp_len = buf_len;
	
	smartcard_write_to_cp( p_response_msg, sizeof(struct sc_response_msg)+ p_response_msg->rsp_len);

	//xy_printf("sim proxy,response expected_rsp_len=[%d],response len=[%d]", TxAPDU.Le+2, p_response_msg->rsp_len);
	
	xy_free(p_response_msg);
}


void proxy_smartcard_task(void)
{
	struct  sc_request_msg *req_msg = NULL;
	//uint8_t *pRxBuffer;
	//uint32_t sc7816_cmd_len;

	while (1) 
	{	
		osMessageQueueGet(proxy_smartcard_msg_q, &req_msg, NULL, osWaitForever);

		if(req_msg == NULL)
		{
            xy_assert(0);
            continue;
		}

		proc_sc7816cmd_msg(req_msg->data,req_msg->req_len);
		
		xy_free(req_msg);
	}
}

int send_msg_to_smartcardproxy(void *buf,unsigned int len)
{
	

	if(len < sizeof(struct  sc_request_msg))
		xy_assert(0);
	
	struct  sc_request_msg *req_msg = xy_zalloc(len );
	
	proxy_smartcard_init();

	if (req_msg != NULL) {
		memcpy(req_msg, buf, len);
	}
	else{
		xy_assert(0);
		return -1;
	}
	osMessageQueuePut(proxy_smartcard_msg_q, &req_msg, 0, osWaitForever);
	return 1;

}

void proxy_smartcard_init()
{
	static  int s_socksmartcard_working=0;
	osThreadAttr_t thread_attr = {0};
	if(s_socksmartcard_working == 1)
		return;
	s_socksmartcard_working = 1;
	proxy_smartcard_msg_q = osMessageQueueNew(10, sizeof(void *), NULL);
	thread_attr.name	   = "proxy_smartcard";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0x1000;
	osThreadNew ((osThreadFunc_t)(proxy_smartcard_task),NULL,&thread_attr);
}


