xy_fota_demo
芯翼FOTA升级demo相关源码，该文件中的代码做为客户二次开发FOTA的示例，方便客户私有云的FOTA开发！
===========================

###########目录结构描述
├── readme.txt                  // help
├── by_http(s)                  // 基于HTTP(S)协议下载差分包，并使用xy_fota.c中的API接口的FOTA升级实例
└── by_coap                     // 基于COAP协议下载差分包，并使用xy_fota.c中的API接口的FOTA升级实例
	├── xy_fota_demo.c          // 获取差分包数据后，使用XY_FOTA.c中的API接口进行FOTA升级的实例
	└── xy_fota_by_coap.c       // 基于COAP协议，使用客户私有通讯协议下载差分包！

by_http(s):
	源文件是通过HTTP（S）协议下载差分包进行FOTA升级。

