
！！！具体使用参阅《XY1100 OPENCPU驱动接口说明》和《XY1100 外设使用说明》

	adc_demo.c			ADC外设使用参考代码
	csp_uart_demo1.c	将CSP4配置成uart的参考代码，简单实现数据的回写，仅供用户测试硬件是否能正常通信。
	csp_uart_demo2.c	将CSP4配置成uart，使用阈值和timeout，供用户需要收到完整一包数据进行处理的需求。
	gpio_int_demo.c		GPIO中断配置参考代码，中断的具体动作由用户自己实现。
	i2c_demo.c			i2c作master的参考代码。
	pwm_demo.c			简单pwm输出，输出周期为1ms的占空比为1:2的方波。
	spi_slave_demo.c	SPI做slave的参考代码，slave的具体动作未提供，需要用户自己实现功能。
	spi_master_demo.c	SPI做master的参考代码，测试数据的发送功能。
	timer_demo.c		TIMER硬定时器参考代码，若客户用来做定时功能，建议统一用RTC定时器，参见user_task_demo.c。
	uart_demo1.c		uart的参考代码，简单实现数据的回写，使用中断方式实时读取数据。
	uart_demo2.c		uart的参考代码，简单实现数据的回写，使用查询方式读取数据。
	