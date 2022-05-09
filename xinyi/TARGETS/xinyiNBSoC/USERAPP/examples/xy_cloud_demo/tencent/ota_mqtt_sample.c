/*
 * 该例程演示了用SDK配置MQTT参数并建立连接, 以及使用相关MQTT API实现与MQTT服务器进行基本通信,并实现OTA的基本功能。
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 */
#include "xy_utils.h"
#include "xy_api.h"
#include "osal.h"
  
#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

#define KEY_VER  "version"
#define KEY_SIZE "downloaded_size"

#define FW_VERSION_MAX_LEN    32
#define DOWNLOAD_UNIT_BYTES   1024

static osThreadId_t g_tencent_mqtt_handle = NULL;

typedef struct OTAContextData {
    void *ota_handle;
    void *mqtt_client;
} OTAContextData;

static DeviceInfo sg_devInfo;

static void _event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg)
{
	(void)pclient;
	
	uintptr_t       packet_id = (uintptr_t)msg->msg;
    OTAContextData *ota_ctx   = (OTAContextData *)handle_context;

    switch (msg->event_type) {
        case MQTT_EVENT_UNDEF:
            Log_i("undefined event occur.");
            break;

        case MQTT_EVENT_DISCONNECT:
            Log_i("MQTT disconnect.");
            break;

        case MQTT_EVENT_RECONNECT:
            Log_i("MQTT reconnect.");
            break;

        case MQTT_EVENT_SUBCRIBE_SUCCESS:
            Log_i("subscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBCRIBE_TIMEOUT:
            Log_i("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBCRIBE_NACK:
            Log_i("subscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
            break;  

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);      
            break;
        default:
            Log_i("Should NOT arrive here.");
            break;
    }
}

static int _setup_connect_init_params(MQTTInitParams *initParams, void *ota_ctx, DeviceInfo *device_info)
{
    initParams->region      = device_info->region;
    initParams->product_id  = device_info->product_id;
    initParams->device_name = device_info->device_name;
    initParams->device_secret = device_info->device_secret;

    initParams->command_timeout        = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;

    initParams->auto_connect_enable  = 1;
    initParams->event_handle.h_fp    = _event_handler;
    initParams->event_handle.context = ota_ctx;

    return QCLOUD_RET_SUCCESS;
}

bool process_ota_data(OTAContextData *ota_ctx)
{
	int rc;
	char buf_ota[DOWNLOAD_UNIT_BYTES] = {0};
	int recv_len = 0;
    unsigned int downloaded_size = 0;
	bool  download_finished     = false;
    bool  upgrade_fetch_success = true;
	
	char  remote_fw_version[FW_VERSION_MAX_LEN] = {0};
	unsigned int remote_fw_size = 0;

	void *h_ota = ota_ctx->ota_handle;
	void *pmqtt_client = ota_ctx->mqtt_client;

	if (0 > IOT_OTA_ReportVersion(h_ota, (char *)g_softap_fac_nv->versionExt)) {
        Log_e("report OTA version failed");
        return false;
    }

	do {
		IOT_MQTT_Yield(ota_ctx->mqtt_client, 200);
        Log_i("wait for ota upgrade command...");

		// recv the upgrade cmd
        if (IOT_OTA_IsFetching(h_ota)) {
			IOT_OTA_Ioctl(h_ota, IOT_OTAG_FILE_SIZE, &remote_fw_size, 4);
    		IOT_OTA_Ioctl(h_ota, IOT_OTAG_VERSION, remote_fw_version, FW_VERSION_MAX_LEN);
			Log_i( "[MQTT OTA] remote version:%s, remote_fw_size:%d", remote_fw_version, remote_fw_size);

			/*set offset and start http connect*/
	 		rc = IOT_OTA_StartDownload(h_ota, 0, remote_fw_size);
			if (QCLOUD_RET_SUCCESS != rc) {
				Log_i("[MQTT OTA]OTA download start err,rc:%d", rc);
				upgrade_fetch_success = false;
				break;
			}

			do {
      	        // download and save the fw  
				recv_len = IOT_OTA_FetchYield(h_ota, buf_ota, DOWNLOAD_UNIT_BYTES, 60);
				if (recv_len > 0) {
					Log_i("[MQTT OTA]recv ota data len:%d", recv_len);

					if (ota_write_to_flash(downloaded_size, buf_ota, recv_len)){
						upgrade_fetch_success = false;
						break;	
					}	
					downloaded_size += recv_len;
				}     
				else if (recv_len < 0){
           				Log_i("[MQTT OTA]OTA download start err,rc:%d", recv_len);
		  				upgrade_fetch_success = false;
						break;
        		}
				// quit ota process as something wrong with mqtt
				rc = IOT_MQTT_Yield(pmqtt_client, 500);
				if (rc != QCLOUD_RET_SUCCESS && rc != QCLOUD_RET_MQTT_RECONNECTED){
					Log_i("[MQTT OTA]MQTT error: %d", rc);
					return false;
				}
			}while (!IOT_OTA_IsFetchFinish(h_ota));

			/* Must check MD5 match or not */
            if (upgrade_fetch_success) {
          		int firmware_valid;
                IOT_OTA_Ioctl(h_ota, IOT_OTAG_CHECK_FIRMWARE, &firmware_valid, 4);
                if (0 == firmware_valid) {
                    Log_e("The firmware is invalid");
                    upgrade_fetch_success = false;
                } 
            }
			
            download_finished = true;
        }
			  
	} while (!download_finished);

	
	if (upgrade_fetch_success) {
		// do some post-download stuff for your need	
		if (ota_delta_check()){
			Log_i("[MQTT OTA]ota delta check failed");

			IOT_OTA_ReportUpgradeFail(h_ota, NULL);
			return false;
		}

		IOT_OTA_ReportUpgradeSuccess(h_ota, NULL); 
    	ota_update_start();
    	return true;
	}
	else {
			IOT_OTA_ReportUpgradeFail(h_ota, NULL);
			return false;
	}
}	

static int ota_mqtt_demo_task(void)
{
    int             rc;
    OTAContextData *ota_ctx     = NULL;
    void *          mqtt_client = NULL;
    void *          h_ota       = NULL;

    IOT_Log_Set_Level(eLOG_DEBUG);
	
    ota_ctx = (OTAContextData *)HAL_Malloc(sizeof(OTAContextData));
    if (ota_ctx == NULL) {
        Log_e("malloc failed");
        goto exit;
    }
    memset(ota_ctx, 0, sizeof(OTAContextData));

    rc = HAL_GetDevInfo(&sg_devInfo);
    if (QCLOUD_RET_SUCCESS != rc) {
        Log_e("get device info failed: %d", rc);
        goto exit;
    }

    // setup MQTT init params
    MQTTInitParams init_params = DEFAULT_MQTTINIT_PARAMS;
    rc                         = _setup_connect_init_params(&init_params, ota_ctx, &sg_devInfo);
    if (rc != QCLOUD_RET_SUCCESS) {
        Log_e("init params err,rc=%d", rc);
        return rc;
    }

    // create MQTT mqtt_client and connect to server
    mqtt_client = IOT_MQTT_Construct(&init_params);
    if (mqtt_client != NULL) {
        Log_i("Cloud Device Construct Success");
    } else {
        Log_e("Cloud Device Construct Failed");
        return QCLOUD_ERR_FAILURE;
    }

    // init OTA handle
    h_ota = IOT_OTA_Init(sg_devInfo.product_id, sg_devInfo.device_name, mqtt_client); 
    if (NULL == h_ota) {
        Log_e("initialize OTA failed");
        goto exit;
    }

    ota_ctx->ota_handle  = h_ota;
    ota_ctx->mqtt_client = mqtt_client;
	
    bool ota_success;  
    do {
        // mqtt should be ready first
        rc = IOT_MQTT_Yield(mqtt_client, 500);   
        if (rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT) {
            HAL_SleepMs(1000);
            continue;
        } else if (rc != QCLOUD_RET_SUCCESS && rc != QCLOUD_RET_MQTT_RECONNECTED) {
            Log_e("exit with error: %d", rc);
            break;
        }

        // OTA process
        ota_success = process_ota_data(ota_ctx);
        if (!ota_success) {
            Log_e("OTA failed! Do it again");
            HAL_SleepMs(2000);
        }
     } while (!ota_success);

exit:

    if (NULL != ota_ctx)
        HAL_Free(ota_ctx);
   
    if (NULL != h_ota)
        IOT_OTA_Destroy(h_ota);
  
    IOT_MQTT_Destroy(&mqtt_client);

    return 0;
}

void ota_mqtt_demo(void)
{
    //wait PDP active
    if(xy_wait_tcpip_ok(2*60) == XY_ERR)
        xy_assert(0);

    ota_mqtt_demo_task();

	g_tencent_mqtt_handle = NULL;

	osThreadExit();
}

void ota_mqtt_demo_init()
{
	osThreadAttr_t thread_attr = {0};
	thread_attr.name	   = "ota_mqtt_task_demo";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0x2000;
	g_tencent_mqtt_handle = osThreadNew((osThreadFunc_t)(ota_mqtt_demo),NULL,&thread_attr);
}
