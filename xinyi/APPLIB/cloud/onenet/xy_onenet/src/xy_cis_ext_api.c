#include "softap_macro.h"
#if MOBILE_VER
#include "xy_cis_ext_api.h"
#include "xy_cis_api.h"
#include "oss_nv.h"


/**************************************************************/
/******************user not care,not modify*******************/
/**************************************************************/

static int  cis_ackid = 100;
static int  cis_work_state = 0;   //0,working;1,closing,2,closed

osThreadId_t g_cis_ext_api_Handle = NULL;
osMessageQueueId_t cis_event_msg_q = NULL; //事件消息队列
cis_write_type_downstream_callback g_cis_write_type_downstream_cb = NULL;
extern osMessageQueueId_t cis_pkt_msg_q;   //上下行报文消息队列
extern void cis_downstream_cb(et_callback_type_t type, void* context,cis_uri_t* uri,cis_mid_t mid, cis_evt_t eid, int valueType,char* value, int valueLen);

/*处理下行WRITE类型数据回调接口，不能阻塞*/
void cis_write_type_downstream_cb(char *data, int data_len)
{
    (char*)data;
    (int)data_len;
    /*process  write type CIS downstream pkt  */
    return;
}

//设置CIS处理write类型报文(下行数据)的回调
void  cis_set_write_type_downstream_cb(cis_write_type_downstream_callback downstream_cb)
{
    g_cis_write_type_downstream_cb = downstream_cb;
}

static int cis_ext_event_proc(int event_num)
{
    int ret = CIS_RET_OK;
    if(event_num == CIS_EVENT_UNREG_DONE)
    {
        cis_work_state=2;
        xy_printf("AP close success");
    }
    else if(event_num == CIS_EVENT_UPDATE_SUCCESS)
    	xy_printf("AP updata success");
    else if(event_num == CIS_EVENT_UPDATE_FAILED)
    	xy_printf("AP updata fail");
    else if(event_num == CIS_EVENT_REG_TIMEOUT)
    	xy_printf("AP register timeout");
    else if(event_num == CIS_EVENT_REG_SUCCESS)
    	xy_printf("AP register success");
    else if(event_num == CIS_EVENT_NOTIFY_SUCCESS)
    	xy_printf("AP notify success");
    else if(event_num == CIS_EVENT_NOTIFY_FAILED)
    	xy_printf("AP notify fail");
    else if(event_num == CIS_EVENT_UPDATE_NEED)
    {
    	xy_printf("AP need updata");
    	if ((ret=cis_updatelife(7200, 0)) != CIS_RET_OK)
    	{
            xy_assert(0);
    	}
    }

    if(cis_event_msg_q != NULL)
    {
        cis_event_msg *msg = NULL;
        msg = xy_zalloc(sizeof(cis_event_msg));
        msg->type = event_num;
		osMessageQueuePut(cis_event_msg_q, &msg, 0, osWaitForever);
    }
    return ret;
}

static  int cis_ext_init()
{
	int ret = CIS_RET_ERROR;
	//ret = cis_create(CIS_SERVER_BS_IP, CIS_SERVER_PORT, 1, NULL);
	if(strlen(g_softap_fac_nv->cloud_server_ip) == 0)
	{
		xy_printf("[CISDEMO]Err: server ip is empty. Can not create cis");
		return XY_ERR;
	}
	
	ret = cis_create(g_softap_fac_nv->cloud_server_ip, g_softap_fac_nv->cloud_server_port, 0,
	g_softap_fac_nv->cloud_server_auth);
	if(ret != CIS_RET_OK)
	{
		xy_printf("[CISDEMO]Err: Create cis failed");
		goto failed_out;
	}
    else if(ret ==CIS_RET_EXIST)
    {
        xy_printf("[CISDEMO]cis is running");
        return XY_OK;
    }

	ret = cis_addobj(CIS_API_OBJ_ID, 1, "1", 4, 0);
	if(ret != CIS_RET_OK)
	{
		xy_printf("[CISDEMO]Err: Add obj(%d) failed", CIS_API_OBJ_ID);
		goto failed_out;
	}

	ret = cis_reg(CIS_API_LIFT_TIME);
	if(ret != CIS_RET_OK)
	{
		xy_printf("[CISDEMO]Err: register cis failed");
		goto failed_out;
	}

	return XY_OK;

failed_out:
    cis_delete();
	return XY_ERR;
}

static int cis_ext_pro()
{
	int ret = CIS_RET_OK;
	cis_pkt_msg *rcv_msg = NULL;
	int evtType = XY_ERR;
	while(cis_work_state != 2)
	{	
		osMessageQueueGet(cis_pkt_msg_q, &rcv_msg, NULL, osWaitForever);
		evtType	= (int)rcv_msg->type;

		xy_printf("[CIS_OPENCPU]work_state(%d),evtType(%d)", cis_work_state, evtType);

		//when do closing,only can proc CALLBACK_TYPE_EVENT type  
		if(cis_work_state == 1 && evtType != CALLBACK_TYPE_EVENT)
		{
			xy_free(rcv_msg);
			continue;
		}

		switch(evtType)
		{
			case CALLBACK_TYPE_DISCOVER:
				xy_printf("[CIS_OPENCPU]discover flag(%d)", rcv_msg->flag);
				ret = cis_discover_rsp(-1, rcv_msg->objId, 1, 19, "5850;5851;5706;5805");
				break;
			case CALLBACK_TYPE_OBSERVE:
				xy_printf("[CIS_OPENCPU]observe flag(%d),objId(%d),insId(%d),resId(%d)", rcv_msg->flag,rcv_msg->objId, rcv_msg->insId, rcv_msg->resId);
				ret = cis_observe_rsp(-1, 1);
				break;
			case CALLBACK_TYPE_OBSERVE_CANCEL:
				//Process observe cancel event
				xy_printf("[CIS_OPENCPU]Read flag(%d),objId(%d),insId(%d),resId(%d)", rcv_msg->flag, rcv_msg->objId, rcv_msg->insId, rcv_msg->resId);
				cis_observe_rsp(-1, 1);
				break;
			case CALLBACK_TYPE_READ:
				//Process read event and send readRsp
				xy_printf("[CIS_OPENCPU]Read flag(%d),objId(%d),insId(%d),resId(%d)", rcv_msg->flag, rcv_msg->objId, rcv_msg->insId, rcv_msg->resId);
				break;
			case CALLBACK_TYPE_WRITE:
				//Process write event and send writeRsp
				xy_printf("[CIS_OPENCPU]Write ");
				ret = cis_write_rsp(-1, 2);
                if(rcv_msg->data != NULL && rcv_msg->valueType == cis_data_type_string && g_cis_write_type_downstream_cb != NULL)
                {
                    g_cis_write_type_downstream_cb(rcv_msg->data, rcv_msg->data_len);
                }
				break;
			case CALLBACK_TYPE_EXECUTE:
				//Process execute event and send executeRsp
				xy_printf("[CIS_OPENCPU]Execute ");
				ret = cis_execute_rsp(-1, 2);
				break;
			case CALLBACK_TYPE_OBSERVE_PARAMS:
				//Process parameter event and send parameterRsp
				xy_printf("[CIS_OPENCPU]Set Param ");
				ret = cis_rsp_withparam(-1, 2);
				break;
			case CALLBACK_TYPE_EVENT:
				xy_printf("[CIS_OPENCPU]Event(%d) ", rcv_msg->evtId);
				//Process event inform and try to update
				ret = cis_ext_event_proc(rcv_msg->evtId);
				break;
			case CALLBACK_TYPE_NOTIFY:
				xy_printf("[CIS_OPENCPU]Notify ");
				//Process send notify event
				ret = cis_notify_sync(-1, rcv_msg->objId, rcv_msg->insId, rcv_msg->resId, rcv_msg->valueType,
				rcv_msg->data_len,rcv_msg->data, rcv_msg->index, rcv_msg->flag, cis_ackid);	
				break;
			case CALLBACK_TYPE_QUIT:
				xy_printf("[CIS_OPENCPU]Quit ");
				//Process 
				cis_work_state = 1;
				//do delete
				if (cis_delete() != CIS_RET_OK)
				{
					xy_assert(0);
				}
				break;
			default:
				break;			

		}
			
		xy_free(rcv_msg);

		if(ret != CIS_RET_OK)
			xy_printf("[CIS_OPENCPU]err: process ret(%d)", ret);
		
	}
	cis_work_state = 0;
	return XY_OK;
}

static void cis_ext_task(void *args)
{
	(void) args;

	//wait PDP active
	if(xy_wait_tcpip_ok(2*60) == XY_ERR)
		xy_assert(0);

	xy_printf("[CIS_OPENCPU]cis simplified api task start");

    //注册下行报文回调函数
    cis_set_downstream_cb(cis_downstream_cb);

	//start onenet task
	if(cis_ext_init() != XY_OK)
	{
        goto out;
	}

	//onenet inform event process
	cis_ext_pro();

	xy_printf("[CIS_OPENCPU]cis simplified api task end");

out:
	g_cis_ext_api_Handle = NULL;
	osThreadExit();
}

void cis_ext_task_init()
{
	osThreadAttr_t thread_attr = {0};

	xy_printf("cis simplified api task create");

	if(cis_pkt_msg_q == NULL)
		cis_pkt_msg_q = osMessageQueueNew(16, sizeof(void *), NULL);
    if(g_cis_ext_api_Handle == NULL)
    {
		thread_attr.name	   = "cis_ext_api_task";
		thread_attr.priority   = XY_OS_PRIO_NORMAL1;
		thread_attr.stack_size = 0x800;
		g_cis_ext_api_Handle = osThreadNew ((osThreadFunc_t)(cis_ext_task),NULL,&thread_attr);
    }

    else
        xy_printf("cis simplified api task running");
}

int cis_ext_reg(int timeout)
{
    cis_event_msg *rcv_msg = NULL;
    int evtType = XY_ERR;
	
    //wait PDP active
	if(xy_wait_tcpip_ok(timeout) == XY_ERR)
		return XY_ERR;

    if(cis_event_msg_q == NULL)
		cis_event_msg_q = osMessageQueueNew(16, sizeof(void *), NULL);

	cis_ext_task_init();

    while(evtType != CIS_EVENT_UPDATE_SUCCESS)
    {
        if(osMessageQueueGet(cis_event_msg_q, &rcv_msg, NULL, timeout*1000) != osOK)
    	{
    		xy_printf("cis simplified api register timeout");
			cis_ext_dereg();
			return XY_ERR;
    	}
        evtType = rcv_msg->type;
        xy_free(rcv_msg);

        if(evtType == CIS_EVENT_REG_TIMEOUT || evtType == CIS_EVENT_BOOTSTRAP_FAILED
                || evtType == CIS_EVENT_CONNECT_FAILED || evtType == CIS_EVENT_REG_FAILED)
        {
            xy_printf("cis simplified api register failed event[%d]", evtType);
            cis_ext_dereg();
            return XY_ERR;
        }
        else
            xy_printf("cis simplified api registering event[%d]", evtType);
    }
    
    xy_printf("cis simplified api register success");
    return XY_OK;
}

int cis_ext_dereg()
{
    cis_event_msg *rcv_msg = NULL;
    int evtType = XY_ERR;
    
    cis_pkt_msg *msg = NULL;
    msg = xy_zalloc(sizeof(cis_pkt_msg));
    msg->type = CALLBACK_TYPE_QUIT;
    
    if(cis_pkt_msg_q == NULL)
		cis_pkt_msg_q = osMessageQueueNew(16, sizeof(void *), NULL);
	xy_assert(cis_pkt_msg_q != NULL);
	//Put data into cis_pkt_msg_q, the task will read and notify it.
	osMessageQueuePut(cis_pkt_msg_q, &msg, 0, osWaitForever);

    while(evtType != CIS_EVENT_UNREG_DONE)
    {
		osMessageQueueGet(cis_event_msg_q, &rcv_msg, NULL, osWaitForever);
		evtType = rcv_msg->type;
        xy_free(rcv_msg);
    }
    
    xy_printf("cis simplified api deregister success");
    return XY_OK;
}

int cis_ext_send(char *data, int data_len, int timeout)
{
    cis_event_msg *rcv_msg = NULL;
    int evtType = XY_ERR;

    //Send upload data must be set notify type; and unlock to sleep until the hook evtid=26
    cis_pkt_msg *msg = NULL;
    msg = xy_zalloc(sizeof(cis_pkt_msg) + data_len + 1);
    msg->type = CALLBACK_TYPE_NOTIFY;
    msg->valueType = cis_data_type_string;
    msg->data_len = data_len;
    memcpy(msg->data, data, data_len);
    msg->objId = CIS_API_OBJ_ID;
    msg->insId = 0;
    msg->resId = CIS_API_RESOURCE_ID;

    if(cis_pkt_msg_q == NULL)
		cis_pkt_msg_q = osMessageQueueNew(16, sizeof(void *), NULL);
	xy_assert(cis_pkt_msg_q != NULL);
    //Put data into apidemo_onenet_q, the task will read and notify it.
	osMessageQueuePut(cis_pkt_msg_q, &msg, 0, osWaitForever);

    while(evtType != CIS_EVENT_NOTIFY_SUCCESS)
    {
		if (osMessageQueueGet(cis_event_msg_q, &rcv_msg, NULL, timeout * 1000) != osOK)
		{
			xy_printf("cis simplified api send data timeout");
			return XY_ERR;
		}

		evtType = rcv_msg->type;
		xy_free(rcv_msg);

        if(evtType == CIS_EVENT_NOTIFY_FAILED)
        {
            xy_printf("cis simplified api send data error");
            return XY_ERR;
        }

    }
    
    xy_printf("cis simplified api send date success");
    return XY_OK;
}

int cis_ext_send_with_rai(char *data, int data_len, int timeout)
{
    int ret = XY_ERR;
    cis_set_rai_flag(PS_SOCK_ONLY_DL_FOLLOWED);
    ret = cis_ext_send(data, data_len, timeout);
    cis_set_rai_flag(PS_SOCK_RAI_NO_INFO);
    return ret;
}

#endif


