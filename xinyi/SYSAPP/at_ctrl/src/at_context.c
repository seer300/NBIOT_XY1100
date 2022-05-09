/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_context.h"
#include "at_com.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "xy_utils.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
at_context_t nearps_ctx = {0};       //dsp context
at_context_t uart_ctx = {0};         //uart context
at_context_t user_rsp_ctx = {0};     //user response to farps context
at_context_dict_t *g_at_context_dict = NULL;  //a dictionary record system used at_contexts 
osMutexId_t at_ctx_dict_m = NULL;  //used for protect register or deregister at_context progress

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
void reset_ctx(at_context_t *ctx)
{
	memset(ctx->at_cmd_prefix, 0, AT_CMD_PREFIX);
	ctx->at_proc = NULL;
	ctx->fwd_ctx = NULL;
	ctx->state = REQ_IDLE;
	ctx->retrans_count = 0;
}

//m3 start up will register at context,should notice kernel may not running!!
int register_at_context(at_context_t *ctx)
{
	xy_assert(ctx != NULL);

	if(osKernelGetState() != osKernelInactive) //only kernel running can use os interface!!
		osMutexAcquire(at_ctx_dict_m, osWaitForever);

	at_context_dict_t *temp = NULL;
	if (g_at_context_dict == NULL)
	{
		temp = xy_malloc(sizeof(at_context_dict_t));
		temp->pre = NULL;
		temp->node = ctx;
		temp->next = NULL;
		g_at_context_dict = temp;
		if(osKernelGetState() != osKernelInactive)
			osMutexRelease(at_ctx_dict_m);
		return XY_OK;
	}

	at_context_dict_t *dict = g_at_context_dict;
	if (dict->node->fd == ctx->fd)
	{
		if (osKernelGetState() != osKernelInactive)
			osMutexRelease(at_ctx_dict_m);
		return XY_ERR;
	}
	while (dict->next != NULL)
	{
		if (dict->node->fd == ctx->fd)
		{
			if(osKernelGetState() != osKernelInactive)
				osMutexRelease(at_ctx_dict_m);
			return XY_ERR;
		}
		dict = dict->next;
	};
	temp = xy_malloc(sizeof(at_context_dict_t));
	temp->node = ctx;
	temp->pre = dict;
	temp->next = NULL;
	dict->next = temp;
	if (osKernelGetState() != osKernelInactive)
		osMutexRelease(at_ctx_dict_m);
	return XY_OK;
}

int deregister_at_context(int fd)
{
	xy_assert(g_at_context_dict != NULL &&
			  ((fd >= PS_SYS_MIN && fd < PS_SYS_MAX) || (fd >= FARPS_USER_MIN && fd <= FARPS_USER_MAX)));

	if (osKernelGetState() != osKernelInactive)
		osMutexAcquire(at_ctx_dict_m, osWaitForever);
	at_context_dict_t *temp = NULL;
	temp = g_at_context_dict;
	do
	{
		if (temp->node->fd == fd)
		{
			softap_printf(USER_LOG, WARN_LOG, "deregister at context fd[%d] from at dict", fd);
			if (temp->next != NULL)
			{
				temp->next->pre = temp->pre;
			}
			temp->pre->next = temp->next;
			xy_free(temp);
			temp = NULL;
			if (osKernelGetState() != osKernelInactive)
				osMutexRelease(at_ctx_dict_m);
			return XY_OK;
		}
		else
		{
			temp = temp->next;
		}

	} while (temp != NULL);

	if (osKernelGetState() != osKernelInactive)
		osMutexRelease(at_ctx_dict_m);
	return XY_ERR;
}

at_context_t *search_at_context(int fd)
{
	if(osKernelGetState() != osKernelInactive) //only kernel running can use os interface!!
		osMutexAcquire(at_ctx_dict_m, osWaitForever);

	xy_assert(g_at_context_dict != NULL &&
			  ((fd >= PS_SYS_MIN && fd < PS_SYS_MAX) || (fd >= FARPS_USER_MIN && fd <= FARPS_USER_MAX)));

	at_context_dict_t *temp = NULL;
	temp = g_at_context_dict;
	do
	{
		if (temp->node->fd == fd)
		{
			if (osKernelGetState() != osKernelInactive)
				osMutexRelease(at_ctx_dict_m);
			return temp->node;
		}
		else
		{
			temp = temp->next;
		}
	} while (temp != NULL);

	if (osKernelGetState() != osKernelInactive)
		osMutexRelease(at_ctx_dict_m);
	return NULL;
}

at_context_t* get_avail_atctx_4_user(int from_proxy)
{
	if(osKernelGetState() != osKernelInactive)
		osMutexAcquire(at_ctx_dict_m, osWaitForever);

	at_context_t *ctx = NULL;
	int fd;

	if (from_proxy == 1)
	{
		if (search_at_context(FARPS_USER_PROXY) == NULL)
		{
			ctx = xy_zalloc(sizeof(at_context_t));
			ctx->position = FAR_PS;
			ctx->fd = FARPS_USER_PROXY;
			xy_assert(register_at_context(ctx) != XY_ERR);
		}
		goto END_GET_CTX;
	}

	for (fd = FARPS_USER_PROXY + 1; fd <= FARPS_USER_MAX; fd++)
	{
		if (search_at_context(fd) == NULL)
		{
			ctx = xy_zalloc(sizeof(at_context_t));
			ctx->position = FAR_PS;
			ctx->fd = fd;
			xy_assert(register_at_context(ctx) != XY_ERR);
			goto END_GET_CTX;
		}
	}
END_GET_CTX:
	if (osKernelGetState() != osKernelInactive)
		osMutexRelease(at_ctx_dict_m);
	return ctx;
}

osMessageQueueId_t at_related_queue_4_user(osTimerId_t timerId)
{
	if (osKernelGetState() != osKernelInactive) //only kernel running can use os interface!!
		osMutexAcquire(at_ctx_dict_m, osWaitForever);

	at_context_dict_t *temp = NULL;
	temp = g_at_context_dict;
	do
	{
		if (temp->node->user_ctx_tmr == timerId)
		{
			if (osKernelGetState() != osKernelInactive)
				osMutexRelease(at_ctx_dict_m);
			return temp->node->user_queue_Id;
		}
		else
		{
			temp = temp->next;
		}
	} while (temp != NULL);

	if (osKernelGetState() != osKernelInactive)
		osMutexRelease(at_ctx_dict_m);
	return NULL;
}

int at_deepsleep_Check()
{
	//临界区里调用，不需要加锁
	at_context_dict_t *temp = NULL;
	temp = g_at_context_dict;

	do
	{
		if (temp->node->process_state == AT_PROCESSING)
		{
			return 0;
		}
		else
		{
			temp = temp->next;
		}
	} while (temp != NULL);

	return 1;
}
