下述几个task实例，是从客户产品形态角度定义的参考代码，不涉及具体的数据通信，客户参考开发时，具体的数据通信请参阅“xy_cloud_demo”文件夹下的demo，在user_process中插入具体的数据通信接口即可


user_task_demo.c	
测试方法：
AT+NV=SET,DEMOTEST,160
AT+NRB
功能：
双核启动的用户周期性RTC动作的参考任务代码。包括第一次上电及深睡唤醒的参考建议，用户在user_process中实现产品需要做的事务，如云通信等。双核启动的OPENCPU产品推荐使用。


user_task_demo2.c	
测试方法：
AT+NV=SET,DEMOTEST,161
AT+NRB
功能：
长供电场景下，用户周期性RTC数据传输的参考代码。增加了对用户任务是否正常运行的软看门狗机制，当未能超时喂狗，则建议设备软重启。OPENCPU推荐使用。


user_task_demo3.c	
测试方法：
AT+NV=SET,DEMOTEST,162
AT+NRB
功能：
单核启动的周期性RTC动作的参考任务代码，如烟感火警报警器。包含单核数据采样及阈值满足后的动态使用NB等行为。用户在user_process中实现产品需要做的事务，如云通信等。单核启动的OPENCPU推荐使用。



user_task_volatile_demo.c	
测试方法：
AT+NV=SET,HIGHFREQ,1
AT+NV=SET,BACKUPKEEP,1
AT+NV=SET,WORKMODE,1
AT+NV=SET,DEMOTEST,163
AT+NRB	
功能：
用户高频率周期性数据采集的参考任务代码。用户高频率数据保存在g_user_vol_data全局中。OPENCPU推荐使用。如需使用，请联系芯翼的FAE。

