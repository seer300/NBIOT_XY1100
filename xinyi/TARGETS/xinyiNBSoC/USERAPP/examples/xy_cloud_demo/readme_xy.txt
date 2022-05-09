该文件夹是做什么的？？
====
cloud_demo文件夹中包含芯翼XY100芯片支持的公有云的示例程序

demo支持华为云、CTWing和OneNet云

cdp_opencpu_demo.c：该文件是cdp公有云的示例程序，在芯片作为主控时有多任务的情况下可以参考该示例。

cdp_opencpu_simple_demo.c：该文件是cdp公有云的简单的示例程序，实现了数据收发功能。可配合user_task_demo示例程序使用，能够实现RTC定时唤醒发送数据给云平台。

onenet_opencpu_demo.c：该文件是onenet公有云的示例程序，该示例可以满足OneNet公有云所有的功能。当用户有多个应用实例的时候可以参考该示例代码。

onenet_opencpu_simple_demo.c：该文件是对onenet公有云简化的示例程序，该示例仅有一个实例，实现了OneNet平台的数据收发功能。如果只是做数据收发可以参考此示例。



这些demo怎么用？？
===
首先，需要在userapp/module.mk文件中添加要使用的demo文件的路径，才能编译demo文件。
	eg:
		###C soruce file for OPENCPU cloud demo
		C_FILES_FLASH+=$(TOP_DIR)/userapp/demo/cloud_demo/onenet_opencpu_demo.c \
				$(TOP_DIR)/userapp/demo/cloud_demo/cdp_opencpu_demo.c

然后，需要在系统初始化的时候启动周期性RTC定时器任务，在该任务中调用公有云的demo。这样才能实现周期唤醒发送数据到云平台。
	在userapp/Src/user_hook_func.c中的user_app_init()函数中添加周期性RTC定时器任务初始化函数。
	eg:
		void user_app_init()
		{
				user_task_demo_init();
		}
		
最后，一般demo配合周期性的RTC定时器任务使用，可以参考userapp\examples\xy_opencpu_demo文件夹下的user_task_demo.c文件。
	eg:
		//user can do any work,such as cdp_opencpu_demo
		static void user_process()
		{
			//need user do
			// do something but that shouldn't last long
			xy_printf("user work start\n");
			
			cdp_register(30);
			
			cdp_send_data("A1B1C1D1",strlen("A1B1C1D1"));

			//cdp can save register into when into deepsleep,so not must deregister
			cdp_deregister();
		}




What it is
===
These c sources files are demo for OPENCPU related interfaces.These demos can be used as a reference for using the cloud

Supported:

- CDP Cloud
	cdp_opencpu_demo.c
- CTWing Cloud
	cdp_opencpu_demo.c
- OneNet Cloud
	onenet_opencpu_demo.c
	onenet_opencpu_simple_demo.c



File introduced
===
cdp_opencpu_demo.c:
		This file contains examples of the use of the opencpu interface between telecom and unicom public cloud

onenet_opencpu_demo.c:
		This file contains examples of using the mobile public cloud opencpu interface


How to use
===
1、First,you must add the demo file path to the build file(userapp/moudle.mk)
	eg:
		###C soruce file for OPENCPU cloud demo
		C_FILES_FLASH+=$(TOP_DIR)/userapp/demo/cloud_demo/onenet_opencpu_demo.c \
				$(TOP_DIR)/userapp/demo/cloud_demo/cdp_opencpu_demo.c
				
2、Second,opens the RTC periodic timer startup function(user_task_demo_init()) during user task initialization(userapp/Src/user_hook_func.c)
	eg:
		void user_app_init()
		{
				user_task_demo_init();
		}

3、Third,the demo initialization function is called in the RTC periodic timing startup function
	eg:
		//user can do any work,such as cdp_opencpu_demo
		static void user_process()
		{
			//need user do
			// do something but that shouldn't last long
			xy_printf("user work start\n");
			
			cdp_register(30);
			
			cdp_send_data("A1B1C1D1",strlen("A1B1C1D1"));

			//cdp can save register into when into deepsleep,so not must deregister
			cdp_deregister();
		}
		