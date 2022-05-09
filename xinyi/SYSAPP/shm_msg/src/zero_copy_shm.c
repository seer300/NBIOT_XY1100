#include "ipcmsg.h"
#include "xy_utils.h"
#include "zero_copy_shm.h"
#include "inter_core_msg.h"

//will delete,not used
osSemaphoreId_t g_shmmem_alloc_sem = NULL;
osMutexId_t shm_mem_alloc_m = NULL;
void *shm_alloc_addr = NULL;
//will delete,not used


//will delete,not used
void *shm_mem_malloc(int ulSize)
{
	osMutexAcquire(shm_mem_alloc_m, osWaitForever);
	void *pheader = NULL;
	
	if(g_shmmem_alloc_sem != NULL)
		xy_assert(0);
	g_shmmem_alloc_sem = osSemaphoreNew(0xFFFF, 0, NULL);
	while (osSemaphoreAcquire(g_shmmem_alloc_sem, 0) == osOK)
	{
	}
	shm_msg_write((void *)&ulSize, sizeof(int), ICM_SHMMEM_ALLOC);
	//softap_printf(USER_LOG, WARN_LOG, "wait semaphone!!!");
	osSemaphoreAcquire(g_shmmem_alloc_sem, osWaitForever);
	pheader = shm_alloc_addr;
	shm_alloc_addr = NULL;
	osSemaphoreDelete(g_shmmem_alloc_sem);
	g_shmmem_alloc_sem = NULL;
	//memset(pheader,0,ulSize);
	osMutexRelease(shm_mem_alloc_m);
	return pheader;
}

//will delete,not used
void shmmem_give_msg_process(int addr)
{
	//softap_printf(USER_LOG, WARN_LOG, "get cp addr=0x%X",addr);
	shm_alloc_addr = (void *)(get_ARM_Addr_from_Dsp((unsigned int)(addr)));
	if(!((int)shm_alloc_addr>=SRAM_BASE && (int)shm_alloc_addr<=(SRAM_BASE+0x60000)))
		xy_assert(0);
	if(g_shmmem_alloc_sem == NULL)
		xy_assert(0);
	osSemaphoreRelease(g_shmmem_alloc_sem);
}

int shm_zero_copy(void* buf, int len, unsigned int msg_type)
{
	T_zero_copy temp = {0};

	if(buf == NULL || !((int)buf>=SRAM_BASE && (int)buf<=(SRAM_BASE+0x60000)))
		xy_assert(0);

	temp.buf = (void *)(get_Dsp_Addr_from_ARM((unsigned int)(buf)));
	temp.len = len;
	return shm_msg_write(&temp,sizeof(T_zero_copy),msg_type);
}

void shmmem_malloc_msg_process(int ulSize)
{
	void *shm_addr =  xy_zalloc_Align(ulSize);
	shm_addr = (void *)(get_Dsp_Addr_from_ARM((unsigned int)(shm_addr)));

	shm_msg_write((void *)&shm_addr, sizeof(int), ICM_SHMMEM_GIVE);
}

int cc_mem[3][2] = {{0,0},{0,0},{0,0}};
void shmmem_malloc_dspheap_msg_process(int ulSize)
{
	void *shm_addr =  xy_zalloc_Align(ulSize);

	for(int i = 0; i < 3;  i++)
	{
		vTaskSuspendAll();
		if(cc_mem[i][0] == 0)
		{
			cc_mem[i][0] = (int)(shm_addr);
			cc_mem[i][1] = ulSize;
			( void ) xTaskResumeAll();
			break;
		}
		( void ) xTaskResumeAll();
	}

	shm_addr = (void *)(get_Dsp_Addr_from_ARM((unsigned int)(shm_addr)));

	shm_msg_write((void *)&shm_addr, sizeof(int), ICM_SHMMEM_GIVE);
}


