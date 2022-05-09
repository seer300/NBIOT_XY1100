/*
 * 该例程演示了用SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 接着在MQTT连接上发送设备端日志到云端, 注意日志上云通道默认是关闭的, 用户需要到云端控制台打开设备端日志服务开关。
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 *
 */
 
#include "xy_utils.h"
#include "xy_api.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_logpost_api.h"

/* TODO: 替换为自己设备的三元组 */
char *product_key		= "a10PBSzSCXi";
char *device_name		= "mqtt_basic_demo";
char *device_secret 	= "65f8022f49f43bc4fe67a187ba9ecc22";

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

/* 客户端实例*/
void* g_mqtt_handle; 
/* logpost实例*/
void* g_logpost_handle = NULL;

static osThreadId_t g_mqtt_test_thread;
static osThreadId_t g_mqtt_process_thread;
static osThreadId_t g_mqtt_recv_thread;

static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1581501698.455][LK-0309] pub: /sys/a13FN5TplKq/logpost_basic_demo/thing/deviceinfo/update
 *
 * 上面这条日志的code就是0309(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出, 禁止在此函数中调用SDK API */
static int32_t demo_state_logcb(int32_t code, char *message)
{
	(void) code;
	
    xy_printf("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
	(void)handle;
	(void)userdata;
	
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            xy_printf("AIOT_MQTTEVT_CONNECT");
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            xy_printf("AIOT_MQTTEVT_RECONNECT");
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            xy_printf("AIOT_MQTTEVT_DISCONNECT: %s", cause);
        }
        break;

        default: {

        }
    }
}


/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
	(void)handle;
	(void)userdata;
	
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            xy_printf("heartbeat response");
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            xy_printf("suback, res: -0x%04X, packet id: %d, max qos: %d",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            xy_printf("pub, qos: %d, topic: %.*s", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            xy_printf("pub, payload: %.*s", packet->data.pub.payload_len, packet->data.pub.payload);
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            xy_printf("puback, packet id: %d", packet->data.pub_ack.packet_id);
        }
        break;

        default: {

        }
    }
}


/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void demo_mqtt_process_thread()
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(g_mqtt_handle);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        osDelay(1000);
    }
	
	g_mqtt_process_thread = NULL;
    osThreadExit();
}

/* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
void demo_mqtt_process_thread_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqtt_process_thread != NULL)
    {
        return;
    }

	thread_attr.name	   = "demo_mqtt_process_thread";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0x2000;
	
	g_mqtt_process_thread = osThreadNew ((osThreadFunc_t)(demo_mqtt_process_thread), NULL, &thread_attr);
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void demo_mqtt_recv_thread()
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(g_mqtt_handle);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            osDelay(1000);
        }
    }
    g_mqtt_recv_thread = NULL;
    osThreadExit();
}

/* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
void demo_mqtt_recv_thread_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqtt_recv_thread != NULL)
    {
        return;
    }

	thread_attr.name	   = "demo_mqtt_recv_thread";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0x2000;
	
	g_mqtt_recv_thread = osThreadNew ((osThreadFunc_t)(demo_mqtt_recv_thread), NULL, &thread_attr);
}

int32_t demo_mqtt_start(void **handle, char *product_key, char *device_name, char *device_secret)
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    char       *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com"; /* 阿里云平台上海站点的域名后缀 */
    char        host[100] = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */
    uint16_t    port = 443;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(demo_state_logcb);
	
    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 16384; /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                               /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        xy_printf("aiot_mqtt_init failed");
        return -1;
    }

    /* 如果以下代码不被注释, 则例程会用TCP而不是TLS连接云平台 */   
	/*
	{
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }
	*/
    snprintf(host, 100, "%s.%s", product_key, url);
    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        xy_printf("aiot_mqtt_connect failed: -0x%04X", -res);
        return -1;
    }

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_thread_running = 1;
   	demo_mqtt_process_thread_init();

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;
	demo_mqtt_recv_thread_init();

    *handle = mqtt_handle;

    return 0;
}

int32_t demo_mqtt_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;

    mqtt_handle = *handle;

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;

    /* 断开MQTT连接 */
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
		aiot_mqtt_deinit(&mqtt_handle);
        xy_printf("aiot_mqtt_disconnect failed: -0x%04X", -res);
        return -1;
    }

    /* 销毁MQTT实例 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_mqtt_deinit failed: -0x%04X", -res);
        return -1;
    }

    return 0;
}

/* 事件处理回调, 用户可通过此回调获取日志上报通道的开关状态 */
void demo_logpost_event_handler(void *handle, const aiot_logpost_event_t *event, void *userdata)
{
	(void)handle;
	(void)userdata;
	
    switch (event->type) {
        /* 日志配置事件, 当设备连云成功或者用户在控制台页面控制日志开关时会收到此事件 */
        case AIOT_LOGPOSTEVT_CONFIG_DATA: {
            xy_printf("user log switch state is: %d", event->data.config_data.on_off);
            xy_printf("toggle it using the switch in device detail page in https://iot.console.aliyun.com");
        }
		break;
        default:
            break;
    }
}

/* 上报日志到云端 */
void demo_send_log(void *handle, char *log)
{
    int32_t res = 0;
    aiot_logpost_msg_t msg;

    memset(&msg, 0, sizeof(aiot_logpost_msg_t));
    msg.timestamp = 0;                          /* 单位为ms的时间戳, 填写0则SDK将使用当前的时间戳 */
    msg.loglevel = AIOT_LOGPOST_LEVEL_DEBUG;    /* 日志级别 */
    msg.module_name = "APP";                    /* 日志对应的模块 */
    msg.code = 200;                             /* 状态码 */
    msg.msg_id = 0;                             /* 云端下行报文的消息标示符, 若无对应消息可直接填0 */
    msg.content = log;                          /* 日志内容 */

    res = aiot_logpost_send(handle, &msg);
    if (res < 0) {
        xy_printf("aiot_logpost_send failed: -0x%04X", -res);
    }
}

void logpost_basic_demo_task()
{
    int32_t res = STATE_SUCCESS;
    uint8_t sys_log_switch = 1;

    /* 建立MQTT连接, 并开启保活线程和接收线程 */
    res = demo_mqtt_start(&g_mqtt_handle, product_key, device_name, device_secret);
    if (res < 0) {
        xy_printf("demo_mqtt_start failed");
        goto exit;
    }

    /* 创建1个logpost客户端实例并内部初始化默认参数 */
    g_logpost_handle = aiot_logpost_init();
    if (g_logpost_handle == NULL) {
        xy_printf("aiot_logpost_init failed");
        goto logpost_exit;
    }
	
    /* 配置logpost的系统日志开关, 打开后将上报网络延时信息 */
    res = aiot_logpost_setopt(g_logpost_handle, AIOT_LOGPOSTOPT_SYS_LOG, (void *)&sys_log_switch);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_logpost_setopt AIOT_LOGPOSTOPT_SYS_LOG failed, res: -0x%04X", -res);
        goto logpost_exit;
    }   

    /* 配置logpost会话, 把它和MQTT会话的句柄关联起来 */
    res = aiot_logpost_setopt(g_logpost_handle, AIOT_LOGPOSTOPT_MQTT_HANDLE, g_mqtt_handle);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_logpost_setopt AIOT_LOGPOSTOPT_MQTT_HANDLE failed, res: -0x%04X", -res);
        goto logpost_exit;
    }  
      
    res = aiot_logpost_setopt(g_logpost_handle, AIOT_LOGPOSTOPT_EVENT_HANDLER, (void *)demo_logpost_event_handler);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_logpost_setopt AIOT_LOGPOSTOPT_EVENT_HANDLER failed, res: -0x%04X", -res);
        goto logpost_exit;
    }
                           
	//循环测试
	static bool g_loop_test = true;
    do{
        /* TODO: 用户可取消注释上报日志到云端, 注意: 日志模块完成初始化后上报通道默认为关闭状态, 日志模块会在收到设备建连内部事件后立即同步云端控制台的开关状态。*/
        demo_send_log(g_logpost_handle, "log in while");
        osDelay(6000);
    }while (g_loop_test);

logpost_exit:
    /* 销毁logpost实例*/
	if (g_logpost_handle != NULL){
    	res = aiot_logpost_deinit(&g_logpost_handle);
    	if (res < STATE_SUCCESS) { 
        	xy_printf("aiot_logpost_deinit failed: -0x%04X", -res);
       	 	goto exit;;
    	}
	}
exit:
    /* 销毁MQTT实例, 退出保活线程和接收线程 */
	if (g_mqtt_handle != NULL){
    	res = demo_mqtt_stop(&g_mqtt_handle);
    	if (res < 0) {
        	xy_printf("demo_start_stop failed");
    	}
	}

}

void logpost_basic_demo(void)
{
    //wait PDP active
    if (xy_wait_tcpip_ok(2*60) == XY_ERR)
        xy_assert(0);

    logpost_basic_demo_task();
  
	g_mqtt_test_thread = NULL;
	osThreadExit();
}

void logpost_basic_demo_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqtt_test_thread != NULL)
    {
        return;
    }

	thread_attr.name	   = "logpost_basic_demo";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0x2000;
    g_mqtt_test_thread = osThreadNew ((osThreadFunc_t)(logpost_basic_demo),NULL,&thread_attr);
}

