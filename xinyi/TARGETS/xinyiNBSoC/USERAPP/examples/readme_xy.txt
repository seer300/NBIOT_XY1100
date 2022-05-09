具体使用请阅读《XY1100 SDK平台软件开发指南》第五章节，DEMO使用说明


用户任务	

	user_task_demo.c	 双核启动的用户周期性RTC动作的参考任务代码，如表计类产品。包括第一次上电及深睡唤醒的参考建议，用户在user_process中实现产品需要做的事务，如云通信等，具体参考cdp_opencpu_demo.c、onenet_opencpu_simple_demo.c。双核启动的OPENCPU产品推荐使用。
	user_task_demo2.c    长供电场景下，用户周期性RTC数据传输的参考代码。增加了对用户任务是否正常运行的软看门狗机制，当未能超时喂狗，则建议设备软重启。OPENCPU推荐使用。
	user_task_demo3.c    单核启动的周期性RTC动作的参考任务代码，如烟感火警报警器。包含单核数据采样及阈值满足后的动态使用NB等行为。用户在user_process中实现产品需要做的事务，如云通信等。单核启动的OPENCPU推荐使用。
	user_task_volatile_demo.c	用户高频率周期性数据采集的参考任务代码，用户高频率数据保存在g_user_vol_data全局中。OPENCPU推荐使用。
	user_task_mcu_demo.c	底板MCU控制芯翼芯片的参考任务代码，包括工作锁机制、断电模式、省电控制等参考流程。外接MCU客户参考使用
	cloud_demo.c	        仅用于芯翼内部云测试，无参考价值
	
	
	
	
AT机制用户业务	

	如果是非3GPP扩展AT命令，参考at_user_req.c和at_user_req1.c
	at_NFTCIMEI_demo.c	IMEI号的用户扩展AT命令示例，内部通过标准3GPP的AT命令获取IMEI
	at_NUESTATS_demo.c	通过NUESTATS扩展AT命令，获取3GPP实时信息的解析示例，以指导用户对非标准AT应答结果的解析
	
！！！以下AT参考为外接MCU底板客户参考使用，OPENCPU客户严禁使用！！！
	user_task_mcu_demo.c	底板MCU控制芯翼芯片的参考任务代码，包括工作锁机制、断电模式、省电控制等参考流程。外接MCU客户参考使用
	at_socket_ext_demo.c	移远的socket客户的扩展AT参考代码，仅支持UDP，单路。外接MCU客户参考使用
	at_cdp_demo.c	移远的CDP客户端参考代码，外接MCU客户参考使用
	at_onenet_demo.c	中移的onenet客户端参考代码，用户自行根据需求进行功能宏的开关，外接MCU客户参考使用
	at_server_demo.c	AT服务端参考代码，供客户实现外部MCU与芯翼芯片间的扩展AT命令信息交互，不建议使用。
	at_encode_demo.c	mbedtls加解密接口示例，重点关注xy_encrypt、xy_decrypt两个接口的使用
	
	
	
OPENCPU接口机制用户业务	！！！具体参阅《XY1100 OPENCPU平台业务接口说明》
	cdp_opencpu_demo.c	API机制的CDP用户应用参考代码，OPENCPU推荐使用，可与user_task_demo.c配套使用
	onenet_opencpu_demo.c	API机制的onenet用户应用参考代码，支持多object多resource。OPENCPU推荐使用，可与user_task_demo.c配套使用
	onenet_opencpu_simple_demo.c	简化版API机制的onenet用户应用参考代码，仅支持一个object，一个resource。OPENCPU推荐使用，可与user_task_demo.c配套使用
	user_flash_demo.c	用户数据深睡断电相关的flash操作，如果用户需要运行过程中操作flash，请参阅《xy_flash.h》。
	
	
	
驱动相关	

！！！具体使用参阅《XY1100 OPENCPU驱动接口说明》和《XY1100 外设使用说明》

	adc_demo.c			ADC外设使用参考代码
	csp_uart_demo1.c	将CSP4配置成uart的参考代码，简单实现数据的回写，仅供用户测试硬件是否能正常通信。
	csp_uart_demo2.c	将CSP4配置成uart，使用阈值和timeout，供用户需要收到完整一包数据进行处理的需求。
	gpio_int_demo.c		GPIO中断配置参考代码，中断的具体动作由用户自己实现，如触发信号量通知用户线程执行数据发送等。
	i2c_demo.c			i2c作master的参考代码。
	pwm_demo.c			简单pwm输出，输出周期为1ms的占空比为1:2的方波。
	spi_slave_demo.c	SPI做slave的参考代码，slave的具体动作未提供，需要用户自己实现功能。
	spi_master_demo.c	SPI做master的参考代码，测试数据的发送功能。
	timer_demo.c		TIMER硬定时器参考代码，若客户用来做定时功能，建议统一用RTC定时器，参见user_task_demo.c。
	uart_demo1.c		uart的参考代码，简单实现数据的回写，使用中断方式实时读取数据。
	uart_demo2.c		uart的参考代码，简单实现数据的回写，使用查询方式读取数据。
