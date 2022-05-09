/** 
* @file        xy_fota_demo.c
* @ingroup     cloud
* @brief       通过coap协议与私有服务器进行FOTA下载升级
* @attention  
* @note         !!!该源文件默认不编译，用户不得直接在该文件中修改！！！\n
*               而应该是自行创建新的私有的源文件，将demo示例代码拷贝过去后再做修改。
*/

/**
 * @defgroup cloud Cloud
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <lwip/netdb.h>

#if XY_FOTA
#include "xy_fota.h"
#include "osal.h"

/**
 * @brief fota超时时间
 */	
#define FOTA_TIMEOUT 96

/**
* @brief message queue for fota demo
*/
typedef struct _fota_demo_msg {
    int     last_flag;  ///< last fota package flag;0:not,1:yes
    int     data_len;	///< fota package len
    uint8_t    *data;	///< fota package data
}fota_demo_msg;


/**
 * @brief fota任务句柄
 */	
static osThreadId_t g_xy_fota_Handle = NULL;

/**
 * @brief fota任务消息队列
 */	
osMessageQueueId_t fota_demo_q = NULL;


/**
 * @brief fota的任务入口
 *@return 0
 *@note 使用ota_update_Init进行fota开始前的初始化；
 使用chat_with_cloud_start进行与fota服务器的交互，并开始下载差分升级包；
 使用ota_update_Hmac进行数据包的Hmac计算；
 使用ota_check_FwValidate进行数据包的校验；
 使用ota_update_start重启设备进行fota升级操作。
 */	
int xy_fota_task()
{
    int last_flag = 0;
    int recv_size = 0;
    fota_demo_msg *msg;


    if(chat_with_cloud_start())
    {
        xy_printf("[king][xy_fota_task] chat_with_cloud error!");
        goto end;
    }


    while(!last_flag)
    {
        if(osMessageQueueGet(fota_demo_q, &msg, NULL, FOTA_TIMEOUT) != osOK)
        {
            xy_printf("[king][xy_fota_task] get data timeout!");
            goto end;
        }

        if(1 == msg->last_flag)
            last_flag = 1;
        
        if(ota_write_to_flash(recv_size, msg->data, msg->data_len))
        {
            xy_printf("[king][xy_fota_task] write error!");
            chat_with_cloud_end(1);
            goto end;
        }    
        recv_size += msg->data_len;
        
        if(msg != NULL)
        {  
            if(msg->data != NULL)
                xy_free(msg->data);

            xy_free(msg);
            msg = NULL;
        }    
    }
	
    if(ota_delta_check())
    {
        xy_printf("[king][xy_fota_task] flie error!");
        chat_with_cloud_end(1);
        goto end;
    }

 
    if(chat_with_cloud_end(0))
    {
        xy_printf("[king][xy_fota_task] chat_with_cloud error!");
        goto end;
    }
    

    ota_update_start();

    
end: 
    return 0;    
}

/**
 * @brief fota的任务初始化，fota任务的创建和消息队列初始化
 */	
void xy_fota_init()
{
	osThreadAttr_t thread_attr = {0};

    // init demo queue
    if(NULL == fota_demo_q)
        fota_demo_q = osMessageQueueNew(16, sizeof(void *), NULL);
    else
        xy_queue_clear(fota_demo_q);
    
    //Create xy_fota demo
	thread_attr.name	   = "xy_fota_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 0x800;
	g_xy_fota_Handle = osThreadNew ((osThreadFunc_t)(xy_fota_task),NULL,&thread_attr); 
}

#endif

