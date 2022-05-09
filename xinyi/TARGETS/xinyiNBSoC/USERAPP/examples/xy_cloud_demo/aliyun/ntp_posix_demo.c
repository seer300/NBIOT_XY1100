/*
 * 该例程演示了用SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 接着在MQTT连接上发送NTP查询请求, 如果云平台的回应报文到达, 从接收线程会调用NTP消息处理的回调函数, 把对时后的本地时间打印出来
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 *
 */
#include "xy_utils.h"
#include "xy_api.h" 

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_ntp_api.h"

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
void* g_ntp_handle;

static osThreadId_t  g_mqtt_test_thread;
static osThreadId_t g_mqtt_process_thread;
static osThreadId_t g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1581501698.455][LK-0309] pub: /ext/ntp/a13FN5TplKq/ntp_basic_demo/request
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
    int8_t      public_instance = 1;  /* 用公共实例, 该参数要设置为1. 若用独享实例, 要将该参数设置为0 */
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

    /* TODO: 如果以下代码不被注释, 则例程会用TCP而不是TLS连接云平台 */
	/*
	{
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }
    */

    if (1 == public_instance) {
        snprintf(host, 100, "%s.%s", product_key, url);
    } else {
        snprintf(host, 100, "%s", url);
    }

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
        /* 尝试建立连接失败*/
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

/* 事件处理回调,  */
void demo_ntp_event_handler(void *handle, const aiot_ntp_event_t *event, void *userdata)
{
	(void)handle;
	(void)userdata;
	
    switch (event->type) {
        case AIOT_NTPEVT_INVALID_RESPONSE: {
            xy_printf("AIOT_NTPEVT_INVALID_RESPONSE");
        }
        break;
        case AIOT_NTPEVT_INVALID_TIME_FORMAT: {
            xy_printf("AIOT_NTPEVT_INVALID_TIME_FORMAT");
        }
        break;
        default: {

        }
    }
}

/* TODO: 数据处理回调, 当SDK从网络上收到ntp消息时被调用 */
void demo_ntp_recv_handler(void *handle, const aiot_ntp_recv_t *packet, void *userdata)
{
	(void)handle;
	(void)userdata;
    switch (packet->type) {
        /* TODO: 结构体 aiot_ntp_recv_t{} 中包含当前时区下, 年月日时分秒的数值, 可在这里把它们解析储存起来 */
        case AIOT_NTPRECV_LOCAL_TIME: {
            xy_printf("local time: %02d/%02d/%02d-%02d:%02d:%02d:%d",
                   packet->data.local_time.year,
                   packet->data.local_time.mon, packet->data.local_time.day, 
                   packet->data.local_time.hour, packet->data.local_time.min,
                   packet->data.local_time.sec, packet->data.local_time.msec);
        }
        break;

        default: {

		}
    }
}

void ntp_posix_demo_task()
{
    int32_t     res = STATE_SUCCESS;

    /*
     * 这里用中国所在的东8区演示功能, 因此例程运行时打印的是北京时间
     *
     * TODO: 若是其它时区, 可做相应调整, 如西3区则把8改为-3, time_zone的合理取值是-12到+12的整数
     *
     */
    int8_t      time_zone = 8;

    /* 建立MQTT连接, 并开启保活线程和接收线程 */
    res = demo_mqtt_start(&g_mqtt_handle, product_key, device_name, device_secret);
    if (res < 0) {
        xy_printf("demo_mqtt_start failed");
        goto exit;
    }

    /* 创建1个ntp客户端实例并内部初始化默认参数 */
    g_ntp_handle = aiot_ntp_init();
    if (g_ntp_handle == NULL) {
        xy_printf("aiot_ntp_init failed");
        goto exit;;
    }

    res = aiot_ntp_setopt(g_ntp_handle, AIOT_NTPOPT_MQTT_HANDLE, g_mqtt_handle);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_ntp_setopt AIOT_NTPOPT_MQTT_HANDLE failed, res: -0x%04X", -res);
        goto ntp_exit;
    }

    res = aiot_ntp_setopt(g_ntp_handle, AIOT_NTPOPT_TIME_ZONE, (int8_t *)&time_zone);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_ntp_setopt AIOT_NTPOPT_TIME_ZONE failed, res: -0x%04X", -res);
        goto ntp_exit;
    }

    /* TODO: NTP消息回应从云端到达设备时, 会进入此处设置的回调函数 */
    res = aiot_ntp_setopt(g_ntp_handle, AIOT_NTPOPT_RECV_HANDLER, (void *)demo_ntp_recv_handler);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_ntp_setopt AIOT_NTPOPT_RECV_HANDLER failed, res: -0x%04X", -res);
        goto ntp_exit;
    }

    res = aiot_ntp_setopt(g_ntp_handle, AIOT_NTPOPT_EVENT_HANDLER, (void *)demo_ntp_event_handler);
    if (res < STATE_SUCCESS) {
        xy_printf("aiot_ntp_setopt AIOT_NTPOPT_EVENT_HANDLER failed, res: -0x%04X", -res);
        goto ntp_exit;
    }

    /* 发送NTP查询请求给云平台 */
	//循环测试
	static bool g_loop_test = true;
    do{
    	res = aiot_ntp_send_request(g_ntp_handle);
    	if (res < STATE_SUCCESS) {
        	goto ntp_exit;
    	}
		osDelay(6000);
	}while (g_loop_test);

ntp_exit:
    /* 销毁NTP实例 */
	if (g_ntp_handle != NULL){
    	res = aiot_ntp_deinit(&g_ntp_handle);
    	if (res < STATE_SUCCESS) {
        	xy_printf("aiot_ntp_deinit failed: -0x%04X", -res);
			goto exit;
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

void ntp_posix_demo()
{
    //wait PDP active
    if (xy_wait_tcpip_ok(2*60) == XY_ERR)
        xy_assert(0);

    ntp_posix_demo_task();
  
	g_mqtt_test_thread = NULL;
	osThreadExit();
}

void ntp_posix_demo_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqtt_test_thread != NULL)
    {
        return;
    }

	thread_attr.name	   = "ntp_posix_demo";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0x2000;
    g_mqtt_test_thread = osThreadNew ((osThreadFunc_t)(ntp_posix_demo),NULL,&thread_attr);
}

