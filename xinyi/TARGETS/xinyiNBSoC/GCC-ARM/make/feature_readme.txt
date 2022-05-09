芯翼平台提供了业界主流的业务功能，每个功能皆可以通过y/n方式进行开关，以解决客户自己功能与芯翼平台功能冲突的问题。
另外，若用户二次开发的flash空间紧张，也可以关闭不需要的功能模块来腾出flash空间。
下面对每个宏功能进行中文注释，如下：

AT_SOCKET_SUPPORT=y  //对于业界主流的socket扩展AT命令功能，具体AT命令参看《芯翼XY1100平台扩展AT命令_V1.2》

XY_PING_SUPPORT=y      //PING包功能是否打开，具体使用参看《芯翼XY1100平台扩展AT命令_V1.2》中的NPING

XY_PERF_SUPPORT=y     //IPERF功能是否打开，具体使用参看《芯翼XY1100平台扩展AT命令_V1.2》中的XYPERF

XY_SOCKET_PROXY_SUPPORT=y   //socket代理功能，以方便DSP核调用socket套接字，用户无需关心

TELECOM_VER_SUPPORT=y   //电信云平台是否打开，目前仅实现了CDP，且与联通云复用，所以如果客户选择CDP连云，该宏值不能关闭

MOBILE_VER_SUPPORT=y    //移动云平台是否打开，即onenet

HTTP_VER_SUPPORT=n    //HTTP\HTTPS是否打开，该模块依赖dtls库，请与dtls库一起打开

WAKAAMA_SUPPORT=y       //lwm2m开源库是否打开

LIBCOAP_SUPPORT=y       //coap开源库是否打开

MQTT_SUPPORT=y         //mqtt开源库是否打开

XY_DTLS_SUPPORT=y      //DTLS开源库是否打开

CJSON_SUPPORT=y        //cJSON开源库是否打开

DEMO_SUPPORT=y       //芯翼提供的很多个DEMO实例是否打开，建议客户关闭，否则占用flash空间太多

DM_SUPPORT=y         //电信、联通、移动三大运营商的DM自注册功能是否打开，该模块的移动DM依赖于onenet模块，如使用onenet的DM，需打开onenet模块；
电信DM依赖于lwm2m开源库，需要打开WAKAAMA_SUPPORT

BLE_SUPPORT=y         //蓝牙功能的相关代码是否打开

XY_ASYNC_SOCKET_SUPPORT=y   //异步socket api接口的相关代码是否打开

XY_FOTA_SUPPORT=y          //芯翼自研FOTA库是否打开

ABUP_FOTA_SUPPORT=n		  //第三方艾拉比FOTA是否打开，该模块依赖HTTP和DTLS模块，请与http和dtls库一起打开

CTWING_VER_SUPPORT=y  //电信云新CTWING平台是否打开

ALIYUN_SUPPORT=n    //阿里云平台是否打开

TENCENT_SUPPORT=n    //腾讯云平台是否打开

注意：每行编译宏结束不能出现空格，否则会上报编译错误