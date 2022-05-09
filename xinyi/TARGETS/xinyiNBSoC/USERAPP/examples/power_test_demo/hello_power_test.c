/**
 * @file hello_power_test.c
 * @brief 这个demo是测试功耗的一个工作模型，这个模型的工作模式如下：
 * 
 * 1. 连接云平台
 * 2. 发送7包数据，每包数据150个字节。发送完全部数据后发送RAI。
 * 3. 进入standby 1分钟后重复步骤1。 
 */

#include "xy_api.h"
#include "xy_rtc_api.h"
#include "xy_utils.h"
#include "xy_demo.h"
#include "xy_cdp_api.h"

/**
 * @brief 任务重试间隔时间
 */	
#define  WAIT_AGAIN_TIME              (5*1000)
/**
 * @brief 任务重试次数上限
 */	
#define  DO_AGAIN_TIMES               3

/**
 * @brief 简化demo的任务句柄
 */	
osThreadId_t  hello_power_test_Handle =NULL;

/**
 *@brief 工作模型具体流程
 */	
void hello_module_task()
{
    int again_num = 0;

    uint8_t buffer[150] = {0};


    for(int i = 0; i < 150; i++)
    {
        buffer[i] = i;
    }

    xy_work_lock(0);

    //设置云平台的IP和PORT，云平台的IP和PORT会存储到NV中
    //CTWing:221.229.214.202;CDP:49.4.85.232
    if(cdp_cloud_setting("221.229.214.202", 5683) != XY_OK)
    {
       xy_assert(0);
    }

     //执行云平台注册流程
    while(cdp_register(86400, 30)  != XY_OK)
    { 
        again_num++;
        if(again_num > DO_AGAIN_TIMES)
        {
            xy_printf("register fail!!!");
            goto task_exit;
        }
        osDelay(WAIT_AGAIN_TIME);
    }

    while(1)
    {
        //注册成功后，发送上行数据
        for(int i = 0; i < 6; i++)
        {
            cdp_send_syn(buffer, sizeof(buffer), 1);
        }

        cdp_send_syn_with_rai(buffer, sizeof(buffer), 1);

        osDelay(60 * 1000 * 1);
    }

task_exit:
	osThreadExit();
}

/**
 * @brief demo初始化函数，把这个函数直接放入user_task_init即可运行demo
 */	
void hello_power_test_init()
{
    //创建CDP的demo线程
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "hello_module";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 0x400;
	hello_power_test_Handle = osThreadNew ((osThreadFunc_t)(hello_module_task),NULL,&thread_attr);
}
