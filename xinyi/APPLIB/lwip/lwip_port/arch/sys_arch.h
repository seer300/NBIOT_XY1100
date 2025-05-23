/* Copyright (C) 2012 mbed.org, MIT License
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**********************************************************************************
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which
 * might include those applicable to Huawei LiteOS of U.S. and the country in which you
 * are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance
 * with such applicable export control laws and regulations.
 **********************************************************************************/

#ifndef __ARCH_SYS_ARCH_H__
#define __ARCH_SYS_ARCH_H__

#include "osal.h"
#include "lwip/opt.h"
#include "lwip/err.h"

// #define SYS_MBOX_NULL (xQueueHandle)0
#define SYS_MBOX_NULL (osMessageQueueId_t)0
// #define SYS_SEM_NULL  (xSemaphoreHandle)0
#define SYS_SEM_NULL  (osSemaphoreId_t)0
#define SYS_DEFAULT_THREAD_STACK_DEPTH configMINIMAL_STACK_SIZE

// typedef xSemaphoreHandle sys_sem_t;
typedef osSemaphoreId_t sys_sem_t;
// typedef xQueueHandle sys_mbox_t;
typedef osMessageQueueId_t sys_mbox_t;
// typedef xTaskHandle sys_thread_t;
typedef osThreadId_t sys_thread_t;
// typedef u32_t sys_mutex_t;
typedef osMutexId_t sys_mutex_t;

typedef struct _sys_arch_state_t
{
	// Task creation data.
	char cTaskName[configMAX_TASK_NAME_LEN];
	unsigned short nStackDepth;
	unsigned short nTaskCount;
} sys_arch_state_t;

//#define sys_sem_valid(x)        (((*x).sem == NULL) ? 0 : 1)
//#define sys_sem_set_invalid(x)  ( (*x).sem = NULL)
//
//#define sys_arch_mbox_tryfetch(mbox,msg)   sys_arch_mbox_fetch(mbox,msg,1)
//#define SYS_MBOX_NULL               (NULL)
//#define sys_mbox_valid(mbox)  (*(mbox) != NULL)
//#define sys_mbox_set_invalid(mbox) do { if((mbox) != NULL) { *(mbox) = NULL; }}while(0)
//
//// === PROTECTION ===
//typedef int sys_prot_t;


/** \brief  Get the current systick time in milliSeconds
 *
 *  Returns the current systick time in milliSeconds. This function is only
 *  used in standalone systems.
 *
 *  /returns current systick time in milliSeconds
 */
u32_t sys_now(void);


#endif /* __ARCH_SYS_ARCH_H__ */
