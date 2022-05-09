#include "ipcmsg_hal.h"
#include <stddef.h>
#include "FreeRTOS.h"
#include "semphr.h"

#define IpcMsg_FAILURE -1
#define IpcMsg_SUCCESS 0

long IpcMsg_Mutex_Create(IpcMsg_MUTEX *pMutexId)
{
    long OSAdp_ret = IpcMsg_FAILURE;

	if( NULL == pMutexId )
	{		
		return IpcMsg_FAILURE;
	}

	pMutexId->pMutex = osMutexNew(NULL);
	
	if(NULL != pMutexId->pMutex)
    {
        OSAdp_ret = IpcMsg_SUCCESS;
    }

    return OSAdp_ret;
}

long IpcMsg_Mutex_Destroy(IpcMsg_MUTEX *pMutexId)
{
	if( NULL == pMutexId )
	{
		
		return IpcMsg_FAILURE;
	}

	osMutexDelete(pMutexId->pMutex);
	pMutexId->pMutex = NULL;

	return IpcMsg_SUCCESS;
}

//m3 core start up will execute,should notice kernel may not running!!!
long IpcMsg_Mutex_Lock(IpcMsg_MUTEX *pMutexId,unsigned long xTicksToWait)
{
	long OSAdp_ret = IpcMsg_FAILURE;
	
	if( NULL == pMutexId )
		return OSAdp_ret;

	if (osKernelGetState() != osKernelInactive)
		OSAdp_ret = osMutexAcquire(pMutexId->pMutex,xTicksToWait);
	else
		OSAdp_ret = osOK;

	
	if(OSAdp_ret == osOK)
		OSAdp_ret = IpcMsg_SUCCESS;
	else
		OSAdp_ret = IpcMsg_FAILURE;	

    return OSAdp_ret;
}

//m3 core start up will execute,should notice kernel may not running!!!
long IpcMsg_Mutex_Unlock(IpcMsg_MUTEX *pMutexId)
{
	if( NULL == pMutexId )
	{
		return IpcMsg_FAILURE;
	}

	if (osKernelGetState() != osKernelInactive)
		osMutexRelease(pMutexId->pMutex);

    return IpcMsg_SUCCESS;
}

long IpcMsg_Semaphore_Create(IpcMsg_SEMAPHORE *pSemaphoreId)
{
    long OSAdp_ret = IpcMsg_FAILURE;

	if( NULL == pSemaphoreId )
	{
		
		return IpcMsg_FAILURE;
	}

	pSemaphoreId->pSemaphore = osSemaphoreNew(1, 0, NULL);
	if (NULL != pSemaphoreId->pSemaphore)
	{
		OSAdp_ret = IpcMsg_SUCCESS;
	}
    
    return OSAdp_ret;
}


long IpcMsg_Semaphore_Destroy(IpcMsg_SEMAPHORE *pSemaphoreId)
{
	if( NULL == pSemaphoreId )
	{
		
		return IpcMsg_FAILURE;
	}

	osSemaphoreDelete(pSemaphoreId->pSemaphore);
	pSemaphoreId->pSemaphore = NULL;

	return IpcMsg_SUCCESS;
}

long IpcMsg_Semaphore_Take(IpcMsg_SEMAPHORE *pSemaphoreId,unsigned long xTicksToWait)
{
    long OSAdp_ret = IpcMsg_FAILURE;

	if( NULL == pSemaphoreId )
	{		
		return IpcMsg_FAILURE;
	}

	if (osSemaphoreAcquire(pSemaphoreId->pSemaphore, xTicksToWait) == osOK)
		OSAdp_ret = IpcMsg_SUCCESS;
	else
		OSAdp_ret = IpcMsg_FAILURE;	

    return OSAdp_ret;
}

long IpcMsg_Semaphore_Give(IpcMsg_SEMAPHORE *pSemaphoreId)
{
	if( NULL == pSemaphoreId )
	{
		
		return IpcMsg_FAILURE;
	}

	osSemaphoreRelease(pSemaphoreId->pSemaphore);

    return IpcMsg_SUCCESS;
}

void IpcMsg_Semaphore_Give_ISR(IpcMsg_SEMAPHORE *pSemaphoreId)
{
	BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
	if (NULL == pSemaphoreId)
	{
		return;
	}
	xSemaphoreGiveFromISR(pSemaphoreId->pSemaphore, &pxHigherPriorityTaskWoken);
}
