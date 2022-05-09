/**
  ******************************************************************************
  * @file    osal_statistics.c
  * @brief   This file contains memory function prototype.
  ******************************************************************************
  * @attention
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at

  * http://www.apache.org/licenses/LICENSE-2.0

  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#include "osal_statistics.h"
#include "xy_utils.h"
#include "oss_nv.h"

stackPeak_t stackPeakTbl[TOTAL_STACK_PEAK_TBL_LEN];

/** @brief  获取线程栈的最大使用量，并更新stackPeakTbl
  * @param  Tskname是线程的名称
  * @param  peaksize是该线程栈的当前使用量
  * @retval 该线程栈的最大使用量
  */
uint16_t TaskStackMatchHandle(char * Tskname, uint16_t peakUsed)
{
	uint32_t i;

	for(i = 0; i < GetStackPeakTblUsed() ; i++)
	{	
		if(strcmp(Tskname, stackPeakTbl[i].name) == 0)
		{
			if(stackPeakTbl[i].peakUsed < peakUsed)
				stackPeakTbl[i].peakUsed = peakUsed;
			return stackPeakTbl[i].peakUsed;
		}
	}
	
	return peakUsed;
}

/** @brief  获取stackPeakTbl数组已使用的数量
  */
uint32_t GetStackPeakTblUsed(void)
{
	uint32_t len = 0;

	for(len = 0; len < TOTAL_STACK_PEAK_TBL_LEN; len ++)
	{
		if(stackPeakTbl[len].name == NULL)
		{
			break;
		}
	}

	return len;
}

/** @brief  将线程的名称注册进stackPeakTbl
  * @param  线程的名称
  */
void RegTaskNameInStackTbl(const char * name)
{
	uint32_t len = 0;

	for(len = 0; len < TOTAL_STACK_PEAK_TBL_LEN; len ++)
	{
		/* skip used */
		if(stackPeakTbl[len].name == NULL)
		{
			stackPeakTbl[len].name = name;
			return;
		}
		else
		{
			if(strcmp(stackPeakTbl[len].name, name) == 0)
			{
				/* registered already */
				return;
			}
		}
	}
}

/** @brief  获取主栈的最大使用量
  */
uint32_t GetMainStackPeakUsed(void)
{
	extern uint32_t __Main_Stack_Limit;
	extern uint32_t __stack;
	extern uint32_t __Main_Stack_Size;
	uint32_t * sys_stack_start = (uint32_t *)&__Main_Stack_Limit;
	uint32_t * sys_stack_end = (uint32_t *)&__stack;
	uint32_t sys_stack_size = (uint32_t)&__Main_Stack_Size;
	uint32_t cnt = 0;

	while((*sys_stack_start == MAIN_STACK_WATER_MARK) && (sys_stack_start < sys_stack_end))
	{
		sys_stack_start ++;
		cnt += 4;
	}

	return (sys_stack_size - cnt);
}

/** @brief  获取M3核FLash和Ram的统计信息
  * @param  MemStatus是存放统计信息结构体指针
  *            @arg flash_total: flash总大小
  *            @arg flash_used: flash已经试用的大小
  *            @arg ram_total: ram总大小
  *            @arg text: ram中text段的大小
  *            @arg data: ram中data段的大小
  *            @arg bss: ram中bss段的大小
  */
void GetMemStatistics(memStatus_t * MemStatus)
{
	extern uint32_t _Flash_Total;
	extern uint32_t _Flash_Used;
	extern uint32_t _Ram_Total;
	extern uint32_t _Ram_Text;
	extern uint32_t _Ram_Data;
	extern uint32_t _Ram_Bss;

	MemStatus->flash_total = (uint32_t)&_Flash_Total;
	MemStatus->flash_used = (uint32_t)&_Flash_Used;
	MemStatus->ram_total = (uint32_t)&_Ram_Total;
	MemStatus->text = (uint32_t)&_Ram_Text;
	MemStatus->data = (uint32_t)&_Ram_Data;
	MemStatus->bss = (uint32_t)&_Ram_Bss;
}

/** @brief  获取当前运行线程栈统计信息
  * @param  Status是存放统计信息结构体指针
  *            @arg stackSize: 线程栈的总大小
  *            @arg currentUseSize: 线程栈目前的使用大小
  *            @arg peakUsedSize: 线程栈历史使用最大值
  */
void GetStackStatistics(osStackStatus_t * Status, UBaseType_t ArraySize)
{
	TaskStatus_t *StatusArray = NULL;
	osStackStatus_t * pstatus = Status;
	StaticTask_t * pTCB;
	uint32_t index = 0;

	StatusArray = xy_zalloc(ArraySize * sizeof(TaskStatus_t));
	uxTaskGetSystemState((TaskStatus_t *)StatusArray, ArraySize, NULL);

	for(index = 0; index < ArraySize; index ++)
	{
		pTCB = (StaticTask_t *)StatusArray[index].xHandle;
		pstatus->name = StatusArray[index].pcTaskName;
		pstatus->stackSize = ((StackType_t *)pTCB->pxDummy8 - (StackType_t *)pTCB->pxDummy6) * sizeof(StackType_t);
		pstatus->currentUseSize = ((StackType_t *)pTCB->pxDummy8 - (StackType_t *)pTCB->pxDummy1) * sizeof(StackType_t);
		pstatus->peakUsedSize = pstatus->stackSize - (StatusArray[index].usStackHighWaterMark * sizeof(StackType_t));
		pstatus->peakUsedSize = TaskStackMatchHandle((char *)(StatusArray[index].pcTaskName), pstatus->peakUsedSize);
		pstatus ++;
	}

	xy_free(StatusArray);
}

/** 获取heap的总大小
  */
uint32_t GetHeapSize(void)
{
	extern uint32_t __Heap_Size;
	uint32_t heap_size = (uint32_t)&__Heap_Size;
	heap_size += ARM_RAM_HEAP2_LEN;

	return heap_size;
}

uint32_t GetHeapPeakUsed(void)
{
	uint32_t min_free_size;
	uint32_t heap_size;
	uint32_t heap_peak_used;

	heap_size = GetHeapSize();
	min_free_size = xPortGetMinimumEverFreeHeapSize();

	heap_peak_used = heap_size - min_free_size;

	return heap_peak_used;
}

extern void xPortResetMinimumEverFreeHeapSize( void );
void ResetHeapPeakUsed(void)
{
	xPortResetMinimumEverFreeHeapSize();
}

#define traceTask_ARRAY_LEN 20

typedef struct
{
	char * pcTaskName;
	unsigned long time_in;
	unsigned long time_out;
}trace_task_node_T;

typedef struct
{
	unsigned int offset;
	trace_task_node_T trace_task_array[traceTask_ARRAY_LEN];
}trace_task_info_T;

trace_task_info_T trace_task_info = {0};
char *pxTCB_name;

void task_switch_in_hook(void)
{
	pxTCB_name = pcTaskGetName(NULL);
	trace_task_info.trace_task_array[trace_task_info.offset].pcTaskName = pxTCB_name;
	trace_task_info.trace_task_array[trace_task_info.offset].time_in = xTaskGetTickCount();
}
void task_switch_out_hook(void)
{
	if(trace_task_info.trace_task_array[0].pcTaskName == 0)
		return;
	if(trace_task_info.trace_task_array[trace_task_info.offset].pcTaskName == pxTCB_name)
	{
		trace_task_info.trace_task_array[trace_task_info.offset].time_out = xTaskGetTickCount();
		trace_task_info.offset = (trace_task_info.offset + 1) % traceTask_ARRAY_LEN;
	}
	else
	{
	}
	
}
