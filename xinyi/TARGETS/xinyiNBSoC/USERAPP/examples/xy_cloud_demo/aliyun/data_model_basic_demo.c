/*
 * 该例程演示了用SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 接着演示了在MQTT连接上进行属性上报, 事件上报, 以及处理收到的属性设置, 服务调用, 取消这些代码段落的注释即可观察运行效果
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 *
 */
#include "xy_utils.h"
#include "xy_api.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_dm_api.h"

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
/* 物模型实例*/
void* g_dm_handle;

static osThreadId_t g_mqtt_test_thread;
static osThreadId_t g_mqtt_process_thread;
static osThreadId_t g_mqtt_recv_thread;

static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1577589489.033][LK-0317] mqtt_basic_demo&a13FN5TplKq
 *
 * 上面这条日志的code就是0317(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
int32_t demo_state_logcb(int32_t code, char *message)
{
	(void)code;
	
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

static void demo_dm_recv_generic_reply(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata){
	(void)dm_handle;
	(void)userdata;
		
    xy_printf("demo_dm_recv_generic_reply msg_id = %d, code = %d, data = %.*s, message = %.*s",
        recv->data.generic_reply.msg_id,
        recv->data.generic_reply.code,
        recv->data.generic_reply.data_len,
        recv->data.generic_reply.data,
        recv->data.generic_reply.message_len,
        recv->data.generic_reply.message);
}

static void demo_dm_recv_property_set(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    xy_printf("demo_dm_recv_property_set msg_id = %ld, params = %.*s",
            (unsigned long)recv->data.property_set.msg_id,
            recv->data.property_set.params_len,
            recv->data.property_set.params);

    /* TODO: 以下代码演示如何对来自云平台的属性设置指令进行应答, 用户可取消注释查看演示效果 */
	/*
	{
        aiot_dm_msg_t msg;

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_PROPERTY_SET_REPLY;
        msg.data.property_set_reply.msg_id = recv->data.property_set.msg_id;
        msg.data.property_set_reply.code = 200;
        msg.data.property_set_reply.data = "{}";
        int32_t res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            xy_printf("aiot_dm_send failed");
        }
    }
    */
}

static void demo_dm_recv_async_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    xy_printf("demo_dm_recv_async_service_invoke msg_id = %ld, service_id = %s, params = %.*s",
            (unsigned long)recv->data.async_service_invoke.msg_id,
            recv->data.async_service_invoke.service_id,
            recv->data.async_service_invoke.params_len,
            recv->data.async_service_invoke.params);

    /* TODO: 以下代码演示如何对来自云平台的异步服务调用进行应答, 用户可取消注释查看演示效果
        *
        * 注意: 如果用户在回调函数外进行应答, 需要自行保存msg_id, 因为回调函数入参在退出回调函数后将被SDK销毁, 不可以再访问到
        */

    /*
    {
        aiot_dm_msg_t msg;

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_ASYNC_SERVICE_REPLY;
        msg.data.async_service_reply.msg_id = recv->data.async_service_invoke.msg_id;
        msg.data.async_service_reply.code = 200;
        msg.data.async_service_reply.service_id = "ToggleLightSwitch";
        msg.data.async_service_reply.data = "{\"dataA\": 20}";
        int32_t res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            xy_printf("aiot_dm_send failed");
        }
    }
    */
}

static void demo_dm_recv_sync_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    xy_printf("demo_dm_recv_sync_service_invoke msg_id = %ld, rrpc_id = %s, service_id = %s, params = %.*s",
            (unsigned long)recv->data.sync_service_invoke.msg_id,
            recv->data.sync_service_invoke.rrpc_id,
            recv->data.sync_service_invoke.service_id,
            recv->data.sync_service_invoke.params_len,
            recv->data.sync_service_invoke.params);

    /* TODO: 以下代码演示如何对来自云平台的同步服务调用进行应答, 用户可取消注释查看演示效果
        *
        * 注意: 如果用户在回调函数外进行应答, 需要自行保存msg_id和rrpc_id字符串, 因为回调函数入参在退出回调函数后将被SDK销毁, 不可以再访问到
        */

    /*
    {
        aiot_dm_msg_t msg;

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_SYNC_SERVICE_REPLY;
        msg.data.sync_service_reply.rrpc_id = recv->data.sync_service_invoke.rrpc_id;
        msg.data.sync_service_reply.msg_id = recv->data.sync_service_invoke.msg_id;
        msg.data.sync_service_reply.code = 200;
        msg.data.sync_service_reply.service_id = "SetLightSwitchTimer";
        msg.data.sync_service_reply.data = "{}";
        int32_t res = aiot_dm_send(dm_handle, &msg);
        if (res < 0) {
            xy_printf("aiot_dm_send failed");
        }
    }
    */
}

static void demo_dm_recv_raw_data(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    xy_printf("demo_dm_recv_raw_data raw data len = %d", recv->data.raw_data.data_len);
    /* TODO: 以下代码演示如何发送二进制格式数据, 若使用需要有相应的数据透传脚本部署在云端 */
    /*
    {
        aiot_dm_msg_t msg;
        uint8_t raw_data[] = {0x01, 0x02};

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_RAW_DATA;
        msg.data.raw_data.data = raw_data;
        msg.data.raw_data.data_len = sizeof(raw_data);
        aiot_dm_send(dm_handle, &msg);
    }
    */
}

static void demo_dm_recv_raw_sync_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    xy_printf("demo_dm_recv_raw_sync_service_invoke raw sync service rrpc_id = %s, data_len = %d",
            recv->data.raw_service_invoke.rrpc_id,
            recv->data.raw_service_invoke.data_len);
}

static void demo_dm_recv_raw_data_reply(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    xy_printf("demo_dm_recv_raw_data_reply receive reply for up_raw msg, data len = %d", recv->data.raw_data.data_len);
    /* TODO: 用户处理下行的二进制数据, 位于recv->data.raw_data.data中 */
}

/* 用户数据接收处理回调函数 */
static void demo_dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    xy_printf("demo_dm_recv_handler, type = %d", recv->type);

    switch (recv->type) {

        /* 属性上报, 事件上报, 获取期望属性值或者删除期望属性值的应答 */
        case AIOT_DMRECV_GENERIC_REPLY: {
            demo_dm_recv_generic_reply(dm_handle, recv, userdata);
        }
        break;

        /* 属性设置 */
        case AIOT_DMRECV_PROPERTY_SET: {
            demo_dm_recv_property_set(dm_handle, recv, userdata);
        }
        break;

        /* 异步服务调用 */
        case AIOT_DMRECV_ASYNC_SERVICE_INVOKE: {
            demo_dm_recv_async_service_invoke(dm_handle, recv, userdata);
        }
        break;

        /* 同步服务调用 */
        case AIOT_DMRECV_SYNC_SERVICE_INVOKE: {
            demo_dm_recv_sync_service_invoke(dm_handle, recv, userdata);
        }
        break;

        /* 下行二进制数据 */
        case AIOT_DMRECV_RAW_DATA: {
            demo_dm_recv_raw_data(dm_handle, recv, userdata);
        }
        break;

        /* 二进制格式的同步服务调用, 比单纯的二进制数据消息多了个rrpc_id */
        case AIOT_DMRECV_RAW_SYNC_SERVICE_INVOKE: {
            demo_dm_recv_raw_sync_service_invoke(dm_handle, recv, userdata);
        }
        break;

        /* 上行二进制数据后, 云端的回复报文 */
        case AIOT_DMRECV_RAW_DATA_REPLY: {
            demo_dm_recv_raw_data_reply(dm_handle, recv, userdata);
        }
        break;

        default:
            break;
    }
}

/* 属性上报函数演示 */
int32_t demo_send_property_post(void *dm_handle, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_PROPERTY_POST;
    msg.data.property_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

int32_t demo_send_property_batch_post(void *dm_handle, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_PROPERTY_BATCH_POST;
    msg.data.property_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

/* 事件上报函数演示 */
int32_t demo_send_event_post(void *dm_handle, char *event_id, char *params)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_EVENT_POST;
    msg.data.event_post.event_id = event_id;
    msg.data.event_post.params = params;

    return aiot_dm_send(dm_handle, &msg);
}

/* 演示了获取属性LightSwitch的期望值, 用户可将此函数取消注释以后进行运行演示 */
int32_t demo_send_get_desred_requset(void *dm_handle)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_GET_DESIRED;
    msg.data.get_desired.params = "[\"LightSwitch\"]";

    return aiot_dm_send(dm_handle, &msg);
}

/* 演示了删除属性LightSwitch的期望值, 用户可将此函数取消注释以后进行运行演示 */
int32_t demo_send_delete_desred_requset(void *dm_handle)
{
    aiot_dm_msg_t msg;

    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_DELETE_DESIRED;
    msg.data.get_desired.params = "{\"LightSwitch\":{}}";

    return aiot_dm_send(dm_handle, &msg);
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
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败 */
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

void data_model_basic_demo_task()
{
    int32_t     res = STATE_SUCCESS;
    uint8_t     post_reply = 1;

	/* 建立MQTT连接, 并开启保活线程和接收线程 */
    res = demo_mqtt_start(&g_mqtt_handle, product_key, device_name, device_secret);
    if (res < 0) {
        xy_printf("demo_mqtt_start failed");
        goto exit;
    }
	
    /* 创建DATA-MODEL实例 */
    g_dm_handle = aiot_dm_init();
    if (g_dm_handle == NULL) {
        xy_printf("aiot_dm_init failed");
        goto dm_exit;
    }
	
    /* 配置MQTT实例句柄 */
    aiot_dm_setopt(g_dm_handle, AIOT_DMOPT_MQTT_HANDLE, g_mqtt_handle);
    /* 配置消息接收处理回调函数 */
    aiot_dm_setopt(g_dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)demo_dm_recv_handler);

    /* 配置是云端否需要回复post_reply给设备. 如果为1, 表示需要云端回复, 否则表示不回复 */
    aiot_dm_setopt(g_dm_handle, AIOT_DMOPT_POST_REPLY, (void *)&post_reply);

    /* 向服务器订阅property/batch/post_reply这个topic */
    aiot_mqtt_sub(g_mqtt_handle, "/sys/a10PBSzSCXi/mqtt_basic_demo/thing/event/property/batch/post_reply", NULL, 1, NULL);

	//循环测试
	static bool g_loop_test = true;
    do{
        /* TODO: 以下代码演示了简单的属性上报和事件上报, 用户可取消注释观察演示效果 */
        demo_send_property_post(g_dm_handle, "{\"LightSwitch\": 1}");
        /*
        demo_send_event_post(g_dm_handle, "Error", "{\"ErrorCode\": 0}");
        */

        /* TODO: 以下代码演示了基于模块的物模型的上报, 用户可取消注释观察演示效果
         * 本例需要用户在产品的功能定义的页面中, 点击"编辑草稿", 增加一个名为demo_extra_block的模块,
         * 再到该模块中, 通过添加标准功能, 选择一个名为NightLightSwitch的物模型属性, 再点击"发布上线".
         * 有关模块化的物模型的概念, 请见 https://help.aliyun.com/document_detail/73727.html
        */
        /*
        demo_send_property_post(g_dm_handle, "{\"demo_extra_block:NightLightSwitch\": 1}");
        */

        /* TODO: 以下代码显示批量上报用户数据, 用户可取消注释观察演示效果
         * 具体数据格式请见https://help.aliyun.com/document_detail/89301.html 的"设备批量上报属性、事件"一节
        */
        /*
        demo_send_property_batch_post(g_dm_handle,
                                      "{\"properties\":{\"Power\": [ {\"value\":\"on\",\"time\":1612684518}],\"WF\": [{\"value\": 3,\"time\":1612684518}]}}");
        */
        
        osDelay(6000);
    }while (g_loop_test);

    /* 停止收发动作 */
    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;

dm_exit:
    /* 销毁DATA-MODEL实例, 一般不会运行到这里 */
	if (g_dm_handle != NULL){
    	res = aiot_dm_deinit(&g_dm_handle);
    	if (res < STATE_SUCCESS) {
        	xy_printf("aiot_dm_deinit failed: -0x%04X", -res);
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

void data_model_basic_demo(void)
{
    //wait PDP active
    if(xy_wait_tcpip_ok(2*60) == XY_ERR)
        xy_assert(0);

   	data_model_basic_demo_task();

	g_mqtt_test_thread = NULL;
	osThreadExit();
}

void data_model_basic_demo_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqtt_test_thread != NULL)
    {
        return;
    }

	thread_attr.name	   = "data_model_basic_demo";
	thread_attr.priority   = XY_OS_PRIO_NORMAL1;
	thread_attr.stack_size = 0x2000;
    g_mqtt_test_thread = osThreadNew ((osThreadFunc_t)(data_model_basic_demo), NULL, &thread_attr);
}

