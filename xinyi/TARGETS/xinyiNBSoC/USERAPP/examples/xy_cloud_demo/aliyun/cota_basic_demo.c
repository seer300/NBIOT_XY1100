/*
 *  这个例子主要用于演示如何从设备端获取存储在云端的关于这个设备的远程配置
 *
 * 需要用户关注或修改的部分, 已用 `TODO` 在注释中标明
 *
 */
#include "xy_utils.h"
#include "xy_api.h"

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_ota_api.h"

/* TODO: 替换为自己设备的三元组 */
char *product_key		= "a10PBSzSCXi";
char *device_name		= "mqtt_basic_demo";
char *device_secret 	= "65f8022f49f43bc4fe67a187ba9ecc22";

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

void *g_mqtt_handle = NULL;
void *g_ota_handle = NULL;
void *g_dl_handle = NULL;

static osThreadId_t  g_mqtt_test_thread;

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1578463098.611][LK-0309] pub: /ota/device/upgrade/a13FN5TplKq/ota_demo
 *
 * 上面这条日志的code就是0309(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
int32_t demo_state_logcb(int32_t code, char *message)
{
    /* 下载远程配置的时候会有大量的HTTP收包日志, 通过code筛选出来关闭 */
    if (STATE_HTTP_LOG_RECV_CONTENT != code) {
        xy_printf("%s", message);
    }
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *const event, void *userdata)
{
	(void)handle;
	(void)userdata;
	
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            xy_printf("AIOT_MQTTEVT_CONNECT");
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            xy_printf("AIOT_MQTTEVT_RECONNECT");
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            xy_printf("AIOT_MQTTEVT_DISCONNECT: %s", cause);
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;
        default: {

        }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *const packet, void *userdata)
{
	(void)handle;
	(void)userdata;
	
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            xy_printf("heartbeat response");
            /* TODO: 处理服务器对心跳的回应, 一般不处理 */
        }
        break;
        case AIOT_MQTTRECV_SUB_ACK: {
            xy_printf("suback, res: -0x%04X, packet id: %d, max qos: %d",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
        }
        break;
        case AIOT_MQTTRECV_PUB: {
            xy_printf("pub, qos: %d, topic: %.*s", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            xy_printf("pub, payload: %.*s", packet->data.pub.payload_len, packet->data.pub.payload);
            /* TODO: 处理服务器下发的业务报文 */
        }
        break;
        case AIOT_MQTTRECV_PUB_ACK: {
            xy_printf("puback, packet id: %d", packet->data.pub_ack.packet_id);
            /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
        }
        break;
        default: {

        }
    }
}

/* 下载收包回调, 用户调用 aiot_download_recv() 后, SDK收到数据会进入这个函数, 把下载到的数据交给用户 */
/* TODO: 在用户收到远程配置的数据后, 需要自己决定如何处理 */
void demo_download_recv_handler(void *handle, const aiot_download_recv_t *packet, void *userdata)
{
	(void)userdata;
	
    /* 目前只支持 packet->type 为 AIOT_DLRECV_HTTPBODY 的情况 */
    if (!packet || AIOT_DLRECV_HTTPBODY != packet->type) {
        return;
    }
    int32_t percent = packet->data.percent;
    uint8_t *src_buffer = packet->data.buffer;
    uint32_t src_buffer_len = packet->data.len;
    /* 如果 percent 为负数, 说明发生了收包异常或者digest校验错误 */
    if (percent < 0) {
        /* 收包异常或者digest校验错误 */
        xy_printf("exception happend, percent is %d", percent);
        return;
    }
    aiot_download_report_progress(handle, percent);
    xy_printf("config len is %d, config content is %.*s", src_buffer_len, src_buffer_len, (char *)src_buffer);
}

/* 用户通过 aiot_ota_setopt() 注册的OTA消息处理回调, 如果SDK收到了OTA相关的MQTT消息, 会自动识别, 调用这个回调函数 */
void demo_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
	(void)ota_handle;
	(void)userdata;
	
    switch (ota_msg->type) {
        case AIOT_OTARECV_COTA: {

			uint32_t res = 0;
            uint16_t port = 443;
            uint32_t max_buffer_len = 2048;
            aiot_sysdep_network_cred_t cred;
            void *dl_handle = aiot_download_init();

            if (NULL == dl_handle || NULL == ota_msg->task_desc) {
                return;
            }

            xy_printf("configId: %s, configSize: %u Bytes", ota_msg->task_desc->version,
                   ota_msg->task_desc->size_total);

			cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
            cred.max_tls_fragment = 16384;
            cred.x509_server_cert = ali_ca_cert;
            cred.x509_server_cert_len = strlen(ali_ca_cert);
			
            /* 设置下载时为TLS下载 */
            if ((STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred))) ||
                /* 设置下载时访问的服务器端口号 */
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port))) ||
                /* 设置下载的任务信息, 通过输入参数 ota_msg 中的 task_desc 成员得到, 内含下载地址, 远程配置的大小和版本号*/
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc))) ||
                /* 设置下载内容到达时, SDK将调用的回调函数 */
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(demo_download_recv_handler))) ||
                /* 设置单次下载最大的buffer长度, 每当这个长度的内存读满了后会通知用户 */
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len))) ||
                /* 发送http的GET请求给http服务器 */
                (STATE_SUCCESS != aiot_download_send_request(dl_handle))) {
                if (res != STATE_SUCCESS) {
                    aiot_download_deinit(&dl_handle);
                    break;
                }
            }
            g_dl_handle = dl_handle;

            break;
        }
        default:
            break;
    }
}

int32_t demo_mqtt_start(void **handle, char *product_key, char *device_name, char *device_secret)
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    char       *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com"; /* 阿里云平台上海站点的域名后缀 */
    char        host[100] = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */
    uint16_t    port = 1883;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
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
        xy_printf("aiot_mqtt_init failed\r\n");
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
        xy_printf("aiot_mqtt_connect failed: -0x%04X", -res);
        return -1;
    }

    *handle = mqtt_handle;

    return 0;
}

int32_t demo_mqtt_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;

	if (handle == NULL || *handle == NULL){
		return -1;
	}

    mqtt_handle = *handle;	

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

void cota_basic_demo_task()
{
    int32_t  res = STATE_SUCCESS;
    uint32_t timeout_ms = 0;

	/* 建立MQTT连接  */
    res = demo_mqtt_start(&g_mqtt_handle, product_key, device_name, device_secret);
    if (res < 0) {
        xy_printf("demo_mqtt_start failed");
        goto exit;
    }
	
    /* 与MQTT例程不同的是, 这里需要增加创建OTA会话实例的语句 */
    g_ota_handle = aiot_ota_init();
    if (NULL == g_ota_handle) {
        goto exit;
    }

    /* 用以下语句, 把OTA会话和MQTT会话关联起来 */
    aiot_ota_setopt(g_ota_handle, AIOT_OTAOPT_MQTT_HANDLE, g_mqtt_handle);
    /* 用以下语句, 设置OTA会话的数据接收回调, SDK收到OTA相关推送时, 会进入这个回调函数 */
    aiot_ota_setopt(g_ota_handle, AIOT_OTAOPT_RECV_HANDLER, demo_ota_recv_handler);


    /* 发送请求, 获取云端的远程配置 */
    {
        char *topic_string = "/sys/a10PBSzSCXi/mqtt_basic_demo/thing/config/get";
        char *payload_string = "{\"id\":\"123\",\"params\":{\"configScope\":\"product\",\"getType\":\"file\"}}";

        res = aiot_mqtt_pub(g_mqtt_handle, topic_string, (uint8_t *)payload_string, strlen(payload_string), 0);
        if (res < STATE_SUCCESS) {
            xy_printf("aiot_mqtt_pub failed: -0x%04X", -res);
            /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
            goto ota_exit;
        }
    }

	while (1){
        aiot_mqtt_process(g_mqtt_handle);
        res = aiot_mqtt_recv(g_mqtt_handle);
        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            osDelay(1000);
        }
		
        if (NULL != g_dl_handle) {
            /* 完成远程配置的接收前, 将mqtt的收包超时调整到100ms, 以减少两次远程配置的下载间隔*/
            int32_t ret = aiot_download_recv(g_dl_handle);
            timeout_ms = 100;

            if (STATE_DOWNLOAD_FINISHED == ret) {
                aiot_download_deinit(&g_dl_handle);
                /* 完成远程配置的接收后, 将mqtt的收包超时调整回到默认值5000ms */
                timeout_ms = 5000;
                break;
            }
            aiot_mqtt_setopt(g_mqtt_handle, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
        }
    }

ota_exit:
	/* 销毁OTA实例 */
	if (g_ota_handle != NULL){
		res = aiot_ota_deinit(&g_ota_handle);
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

void cota_basic_demo(void)
{
    //wait PDP active
    if(xy_wait_tcpip_ok(2*60) == XY_ERR)
        xy_assert(0);

    cota_basic_demo_task();

	g_mqtt_test_thread = NULL;
	osThreadExit();
}

void cota_basic_demo_init()
{
	osThreadAttr_t thread_attr = {0};

    if (g_mqtt_handle != NULL)
    {
        return;
    }

	thread_attr.name	   = "cota_basic_demo";
	thread_attr.priority   = XY_OS_PRIO_NORMAL1;
	thread_attr.stack_size = 0x2000;
    g_mqtt_test_thread = osThreadNew ((osThreadFunc_t)(cota_basic_demo),NULL,&thread_attr);
}

