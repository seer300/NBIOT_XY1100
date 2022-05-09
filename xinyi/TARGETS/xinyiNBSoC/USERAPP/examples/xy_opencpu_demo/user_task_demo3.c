/** 
* @file        user_task_demo3.c
* @ingroup     opencpu
* @brief       单核启动的周期性RTC动作的参考任务代码，包含单核数据采样及阈值满足后的动态使用NB等行为，如烟感火警报警器。
用户通过修改USER_RTC_OFFSET宏值来定义周期性时长，在user_process中实现用户数据的处理，如云通信等。
为了保证芯片在任何状态下都能正常处理RTC事件，请不要修改参考代码的主框架。
* @attention   必须设置g_softap_fac_nv->deepsleep_enable==1 && g_softap_fac_nv->work_mode==1! \n
*  如果不对芯翼芯片执行断电操作，为了防止软硬件异常、无线环境异常等造成功耗过高，用户必须设置好user_dog_time、hard_dog_time两个软硬看门狗时长！
* @note         !!!该源文件默认不编译，用户不得直接在该文件中修改！！！\n
*               而应该是自行创建新的私有的源文件，将demo示例代码拷贝过去后再做修改。
*/


#include "xy_api.h"
#include "dsp_process.h"
#if XY_PING
#include "ping.h"
#endif

#if DEMO_TEST

/**
 * @brief user task period lenth,user must vary it by self production
 */	
#define   USER_RTC_OFFSET   (40+osKernelGetTickCount()%60)


osThreadId_t  g_user_task_Handle3 = NULL;
osSemaphoreId_t user_rtc_sem3 = NULL;

/**
 * @brief 用户周期性RTC超时回调接口，不建议用户修改
 */	
static void user_rtc_timeout_cb(void *para)
{
	(void) para;

	if(user_rtc_sem3 == NULL)
	{
		user_rtc_sem3 = osSemaphoreNew(0xFFFF, 0, NULL);
	}
	xy_printf("user RTC callback\n");
	
	osSemaphoreRelease(user_rtc_sem3);
	
}

#if 0
/**
 * @brief       用户根据自己的产品特点，完成软看门狗异常的容错方案
 * @attention   用户也可以不使用芯翼平台的软看门狗机制，自行设计容错方案，但不管采用哪种策略，必须保护到位 
 */	
void  user_dog_timeout_hook()
{
#if 0
	xy_rtc_set_by_day(RTC_TIMER_USER1,user_rtc_timeout_cb,NULL,15,3*60*60,0,0);
#else
	//for to reduce BSS stress,and improve communication success rate,user task must set rand UTC,see  xy_rtc_set_by_day
	xy_rtc_timer_create(RTC_TIMER_USER1,USER_RTC_OFFSET,user_rtc_timeout_cb,NULL);
#endif	
	//xy_reboot(0,0);
	xy_fast_power_off(0);
}
#endif


/**
 * @brief 用户周期性的数据处理入口，内部可以进行ONENET/CDP/SOCKET/MQTT等数据通信，也可以执行用户私有行为
 * @attention   如果是进行远程数据通信，请参考云通信的demo实例，在该函数中调用相关接口即可
 */	
static void user_process()
{
	//need user do
    // do something but that shouldn't last long
    xy_printf("user work start\n");

#if XY_PING
	//ping test
	start_ping(AF_INET, "139.224.112.6", 32, 1, 30, 1, -1);
#endif
   	osDelay(2000);
	
	//CDP api,see:cdp_regsiter/cdp_senddata

	//onenet api,see cis_simple_api_demo_task/onenet_api_demo_task
}

static void user_task_demo(void *args)
{
	int ret;

	(void) args;

	xy_assert(g_softap_fac_nv->deepsleep_enable==1 && g_softap_fac_nv->work_mode!=0);

	while(1)
	{		
		//wait user RTC timeout,maybe happen after PS RTC wakeup
		if(user_rtc_sem3 != NULL)
		{	
			ret = osSemaphoreAcquire(user_rtc_sem3, osWaitForever);

			//user RTC timeout
			if(ret == osOK)
			{
				xy_printf("user_task_demo RTC timeout\n");	

				//only M3 core loaded when power on,do local data proc,and maybe load DSP core dynamicly to send burst event to server,such as fire			
				//not need load DSP core,and do local work
				xy_work_lock(0);

				//Users decide whether to connect NBiot according to the current situation,such as threshold/time...
				if(osKernelGetTickCount()%4 == 0)
				{
					//load DSP dynamicly,and send IP packet to server
					xy_work_lock(0);

					xy_Dynamic_load_DSP();
					
					//wait PDP active
					if(xy_wait_tcpip_ok(2*60) == XY_ERR)
						xy_assert(0);
					
					xy_printf("tcpip is ok,start IP packet send\n");			

					//Data communication with server through NB 
					user_process();
					xy_work_unlock(0);
				}
				else
				{
					//maybe  call  xy_Flash_Write or xy_Flash_Write_no_erase to save user slef data in flash
				}

				//free user data until finished current communication,because maybe resend.

				xy_printf("set next user rtc:%d\n",USER_RTC_OFFSET);
#if 0
					xy_rtc_set_by_day(RTC_TIMER_USER1,user_rtc_timeout_cb,NULL,15,3*60*60,0,0);
#else
				//for to reduce BSS stress,and improve communication success rate,user task must set rand UTC,see  xy_rtc_set_by_day
				xy_rtc_timer_create(RTC_TIMER_USER1,USER_RTC_OFFSET,user_rtc_timeout_cb,NULL);
#endif

				xy_printf("user task end,goto deep sleep\n");	
							
				xy_work_unlock(0);
				
			}
			else
				xy_assert(0);
		}
		//first POWENON,only set next user RTC,not use NB PS
		else
		{
			user_rtc_sem3 = osSemaphoreNew(0xFFFF, 0, NULL);
			//wakeup by other cause,such as PS RTC/PIN
			if(xy_rtc_next_offset_by_ID(RTC_TIMER_USER1))
			{
				xy_printf("user RTC have setted\n");			
				continue;
			}
			
			xy_printf("first set user RTC:%d\n",USER_RTC_OFFSET);	

			//not need load DSP core
			xy_work_lock(0);	
#if 0
			xy_rtc_set_by_day(RTC_TIMER_USER1,user_rtc_timeout_cb,NULL,15,3*60*60,0,0);
#else
			//for to reduce BSS stress,and improve communication success rate,user task must set rand UTC,see  xy_rtc_set_by_day
			xy_rtc_timer_create(RTC_TIMER_USER1,USER_RTC_OFFSET,user_rtc_timeout_cb,NULL);
#endif			
						
			xy_work_unlock(0);
		}
	}
}

/**
 * @brief 用户任务初始化函数，在user_task_init中添加
 * @attention   
 */	
void user_task_demo3_init()
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "user_task_demo";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 0x400;
	g_user_task_Handle3 = osThreadNew ((osThreadFunc_t)(user_task_demo),NULL,&thread_attr); 
}
#endif
