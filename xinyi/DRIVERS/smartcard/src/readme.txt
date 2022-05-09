版本切换：
默认版本SIM卡驱动在DSP核，如果需要在M3核实现SIM卡驱动功能，需要将出厂NV参数sim_vcc_ctrl设为128；
调试时，可以通过AT命令：AT+NV=SET,SIMVCC,128，进行配置，返回OK后AT+NRB重启设备即可；

文件介绍：
smartcard_proxy.c，为SIM卡驱动代理线程相关功能，负责打通DSP核的协议栈与M3核的SIM卡驱动之间的通信，简单说就是跨核消息传递机制；
smartcard.c，为真实的SIM卡驱动代码，实现与真实SIM卡的数据交互；
vsim_drv.c，为客户二次开发实现VSIM功能的填充框架代码，客户需要仔细阅读SC7816_command接口里的注释，并完善相关虚拟能力。


客户二次开发相关：
如果涉及到3GPP相关接口，请查阅xy_ps_api.h，如xy_get_IMEI
如果涉及到flash操作相关，请查阅xy_flash.h，如xy_flash_read、xy_flash_write、xy_flash_erase等接口；其中用户可用的flash空间为USER_FLASH_BASE，长度为USER_FLASH_LEN_MAX。

特别提醒：
smartcard.c与vsim_drv.c因为使用相关的接口，进而造成不能同时编译，请用户选择对应的源文件进行编译，另外源文件改为.bak即可。