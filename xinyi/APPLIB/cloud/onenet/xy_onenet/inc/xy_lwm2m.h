#ifndef _XY_LWM2M_H
#define _XY_LWM2M_H

#define XY_LWM2M_SUCCESS 0
#define XY_LWM2M_TIMEOUT 1
#define XY_LWM2M_PKTNOTSENT 2
#define XY_LWM2M_RECOVERFAILED 3
#define XY_LWM2M_UPDATEFAILED 4
#define XY_LWM2M_RST 5

#define XY_LWM2M_ERROR 900

typedef struct xy_lwm2m_config_s
{
    int bootstrap_flag;
    char *server_host;
    char server_ip[16];
    int port;
    char *endpoint_name;
    int lifetime;
    int security_mode;
    char *psk_id;
    char *psk;
    int binding_mode;

    int ack_timeout;
    int retrans_max_times;
    int is_auto_ack;
    int access_mode;
    int access_mode_alternative;
    int platform;

    // TODO: cfg_res
#if VER_QUCTL260
	int cfg_res;   //add by cjh for QLACFG
#endif
    int recovery_mode;
    int lifetime_enable;
    int dtls_mode;
    int dtls_version;
} xy_lwm2m_config_t;

extern xy_lwm2m_config_t *xy_lwm2m_config;

typedef enum
{
    XY_LWM2M_CACHED_URC_TYPE_READ = 0,
    XY_LWM2M_CACHED_URC_TYPE_WRITE,
    XY_LWM2M_CACHED_URC_TYPE_EXECUTE,
    XY_LWM2M_CACHED_URC_TYPE_OBSERVE,
    XY_LWM2M_CACHED_URC_TYPE_LIFETIME_CHANGED,
    XY_LWM2M_CACHED_URC_TYPE_BS_FINISHED,
    XY_LWM2M_CACHED_URC_TYPE_FOTA_END
} xy_lwm2m_cached_urc_type_e;

typedef struct xy_lwm2m_cached_urc_common_s
{
    struct xy_lwm2m_cached_urc_common_s *next;
    char urc_type;
    char *urc_data;
} xy_lwm2m_cached_urc_common_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     short objectId;
//     short instanceId;
//     short resourceId;
// } xy_lwm2m_cached_urc_read_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     char value_type;
//     short objectId;
//     short instanceId;
//     short resourceId;

//     short len;
//     short index;
//     union
//     {
//         void *value_ptr;
//         int value_int;
//         double value_float;
//     };

// } xy_lwm2m_cached_urc_write_t;

// typedef xy_lwm2m_cached_urc_read_t xy_lwm2m_cached_urc_execute_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     char flag;
//     short objectId;
//     short instanceId;
//     short resourceId;
// } xy_lwm2m_cached_urc_observe_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     int value;
// } xy_lwm2m_cached_urc_other_t;

typedef struct
{
    xy_lwm2m_cached_urc_common_t *first;
    xy_lwm2m_cached_urc_common_t *last;
    int num;
} xy_lwm2m_cached_urc_head_t;

extern xy_lwm2m_cached_urc_head_t *xy_lwm2m_cached_urc_head;

#define MAX_RESOURCE_COUNT 14

typedef struct xy_lwm2m_object_info_s
{
    struct xy_lwm2m_object_info_s *next;
    int obj_id;
    int instance_id;
    int resource_count;
    int resouce_ids[MAX_RESOURCE_COUNT];
} xy_lwm2m_object_info_t;

typedef struct
{
    xy_lwm2m_object_info_t *first;
    int num;
} xy_lwm2m_object_info_head_t;

extern xy_lwm2m_object_info_head_t *xy_lwm2m_object_info_head;
#endif