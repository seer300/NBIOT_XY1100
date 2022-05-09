/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "xy_utils.h"
#include "xy_system.h"
#include "shm_msg_api.h"
#include "inter_core_msg.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
osMutexId_t g_shmmsg_mux = NULL;
osSemaphoreId_t g_shmmsg_sem = NULL;
void *g_shmmsg_buf = NULL;
int g_shmmsg_buf_len = 0;
int g_sync_shm_msg_id = SHM_MSG_INVALID;
struct shmmsg_proc_e *g_shmmsg_proc_base = NULL;

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/

int send_ps_shm_msg(void *msg, int msg_len)
{
	void *shm_data = xy_malloc_Align(msg_len+1);

	if(shm_data == NULL)
		xy_assert(0);

	memcpy(shm_data,msg,msg_len);
	return shm_zero_copy(shm_data,msg_len,ICM_PS_SHM_MSG);
}

//send req,and not wait rsp
int send_shm_msg(int msg_id, void *msg, int msg_len)
{
	int ret = XY_OK;
	char *user_param_temp = NULL;
	
	T_SHM_MSG_TYPE user_shm_param_req = {0};

	if (msg != NULL)
	{
		user_param_temp = xy_zalloc_Align(msg_len);
		memcpy(user_param_temp, msg, msg_len);
	}
	user_shm_param_req.buf = (void *)(get_Dsp_Addr_from_ARM((unsigned int)(user_param_temp)));
	user_shm_param_req.msg_id = msg_id;
	user_shm_param_req.len = msg_len;

	if (shm_msg_write((void *)&user_shm_param_req, sizeof(T_SHM_MSG_TYPE), ICM_XINYI_SHM_MSG) != XY_OK)
	{
		if(user_param_temp != NULL)
			xy_free(user_param_temp);
		ret = XY_ERR;
	}

	return ret;
}

//send req and wait rsp forever
int send_shm_msg_sync(int msg_id, void *req_param, int req_param_len, void *rsp_buf, int rsp_buf_len)
{
	int ret = XY_ERR;
	char *user_param_temp = NULL;

	if (DSP_IS_NOT_LOADED())
	{
		return ret;
	}
	
	osMutexAcquire(g_shmmsg_mux, osWaitForever);

	T_SHM_MSG_TYPE user_shm_param_req = {0};
	
	if(req_param != NULL)
	{
		user_param_temp = xy_zalloc_Align(req_param_len);
		memcpy(user_param_temp,req_param,req_param_len);
	}
	user_shm_param_req.buf = (char *)(get_Dsp_Addr_from_ARM((unsigned int)(user_param_temp)));
	user_shm_param_req.msg_id = msg_id;
	user_shm_param_req.len = req_param_len;
	g_sync_shm_msg_id = msg_id;  //记录msg id

	if (shm_msg_write((void *)&user_shm_param_req, sizeof(T_SHM_MSG_TYPE), ICM_XINYI_SHM_MSG) != XY_OK)
	{		
		if(user_param_temp != NULL)
			xy_free(user_param_temp);
		g_sync_shm_msg_id = SHM_MSG_INVALID;
		osMutexRelease(g_shmmsg_mux);
		return XY_ERR;
	}

	osSemaphoreAcquire(g_shmmsg_sem, osWaitForever);
	g_sync_shm_msg_id = SHM_MSG_INVALID; //重置msg id

	if(g_shmmsg_buf_len > rsp_buf_len)
	{
		xy_assert(0);
	}
	else if(g_shmmsg_buf_len != 0)
	{
		memcpy(rsp_buf, g_shmmsg_buf, g_shmmsg_buf_len);
		ret = XY_OK;
	}
	else
	{
		//内容为空的确认消息
		ret = XY_OK;
	}
	if(g_shmmsg_buf != NULL)
		xy_free(g_shmmsg_buf);
	
	g_shmmsg_buf = NULL;
	g_shmmsg_buf_len = 0;
	osMutexRelease(g_shmmsg_mux);
	return ret;
}

//recv rsp and copy to local,proc rsp by send_shm_msg_sync
int rcv_shm_msg_process(void *rcv_buf)
{
	int ret = XY_OK;
	T_SHM_MSG_TYPE *app_msg_buf = (T_SHM_MSG_TYPE *)rcv_buf;
	void* buf = app_msg_buf->buf;
	int len = app_msg_buf->len;
	zero_cp_transfer(&buf, len);
	app_msg_buf->buf = buf;

	if (app_msg_buf == NULL)
		xy_assert(0);

	//有M3核触发的请求，DSP核回复对应消息ID的应答，释放信号量，解阻塞send_shm_msg_sync接口
	if(g_sync_shm_msg_id == app_msg_buf->msg_id)
	{
		is_SRAM_addr((unsigned int)(app_msg_buf->buf));
		g_shmmsg_buf = app_msg_buf->buf;
		g_shmmsg_buf_len = app_msg_buf->len;
		osSemaphoreRelease(g_shmmsg_sem);
	}
	//DSP发送来的消息，与本地注册的回调ID进行匹配，成功后执行回调
	else
	{
		struct shmmsg_proc_e *app_shmmsg_proc = NULL;
		void *data_buf = (void *)app_msg_buf->buf;
		app_shmmsg_proc = g_shmmsg_proc_base;
		while (app_shmmsg_proc != NULL)
		{
			if (app_shmmsg_proc->shmmsg_id == app_msg_buf->msg_id)
			{
				ret = app_shmmsg_proc->proc(app_msg_buf->msg_id, data_buf, app_msg_buf->len);
				if (data_buf != NULL)
					xy_free(data_buf);
				return ret;
			}
			app_shmmsg_proc = app_shmmsg_proc->next;
		}
		//找不到回调处理函数断言
		if (data_buf != NULL)
			xy_free(data_buf);
	}
	return ret;
}

void regist_rcved_msg_callback(int msg_id, shmmsg_req_func func)
{
    struct shmmsg_proc_e *new_proc = (struct shmmsg_proc_e *)xy_zalloc(sizeof(struct shmmsg_proc_e));
    new_proc->proc = func;
    new_proc->next = NULL;
	new_proc->shmmsg_id = msg_id;
	
    if (g_shmmsg_proc_base == NULL)
    {
        g_shmmsg_proc_base = new_proc;
        return;
    }
	struct shmmsg_proc_e *temp, *pre;
    pre = g_shmmsg_proc_base;
    temp = g_shmmsg_proc_base->next;
    while (temp != NULL)
    {
        pre = temp;
        temp = temp->next;
    }
    pre->next = new_proc;
}

void shm_msg_init()
{
	if (g_shmmsg_mux == NULL)
		g_shmmsg_mux = osMutexNew(NULL);
	if (g_shmmsg_sem == NULL)
		g_shmmsg_sem = osSemaphoreNew(0xFFFF, 0, NULL);
}
