#include "softap_macro.h"
#if LWM2M_COMMON_VER
#include "xy_utils.h"
#include "at_onenet.h"
#include "xy_cis_api.h"
#include "at_global_def.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "cloud_utils.h"
#include "xy_net_api.h"
#include "at_xy_lwm2m.h"
#include "net_app_resume.h"

#include "lwip/sockets.h"
#include "lwip/opt.h"
#include "lwip/netdb.h"

xy_lwm2m_config_t *xy_lwm2m_config = NULL;
xy_lwm2m_cached_urc_head_t *xy_lwm2m_cached_urc_head = NULL;
xy_lwm2m_object_info_head_t *xy_lwm2m_object_info_head = NULL;

extern onenet_context_config_t onenet_context_configs[CIS_REF_MAX_NUM];
extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];
extern osMutexId_t g_onenet_mutex;
extern osSemaphoreId_t cis_poll_sem;
extern osSemaphoreId_t g_cis_del_sem;
extern cis_ret_t onet_miplread_req(st_context_t *onenet_context, struct onenet_read *param);
extern char *cis_cfg_tool(char *ip, unsigned int port, char is_bs, char *authcode, char is_dtls, char *psk, int *cfg_out_len);

#define INIT_NULL_PTR(ptr) char *ptr = NULL

#define GOTOERRORIFNULL(param) \
    do                         \
    {                          \
        if (param == NULL)     \
            goto error;        \
    } while (0)

#define FREEIFNOTNULL(param) \
    do                       \
    {                        \
        if (param)           \
        {                    \
            xy_free(param);  \
            param = NULL;    \
        }                    \
    } while (0)

static char *gen_at_ok()
{
    char *at_result = xy_malloc(strlen(AT_RSP_OK) + 1);
    strcpy(at_result, AT_RSP_OK);

    return at_result;
}

int is_xy_lwm2m_running()
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_ref = &onenet_context_refs[0];
    //if (onenet_context_ref == NULL)
    //{
    //    return 0;
    //}
    if (onenet_context_ref->onenet_context == NULL || onenet_context_ref->onet_at_thread_id == NULL)
    {
        return 0;
    }
    return 1;
}

int xy_lwm2m_deinit(onenet_context_reference_t *onenet_context_ref)
{

    onenet_context_ref->thread_quit = 1;
    return 0;
}

int xy_lwm2m_init(onenet_context_reference_t *onenet_context_ref, onenet_context_config_t *onenet_context_config)
{
    int ret = 0;

    ret = cis_init_common((void **)&onenet_context_ref->onenet_context, onenet_context_config->config_hex, onenet_context_config->total_len, NULL);
    if (ret != CIS_RET_OK)
    {
        xy_lwm2m_deinit(onenet_context_ref);
        return -1;
    }
    onenet_context_ref->onenet_context_config = onenet_context_config;
    return 0;
}

void init_xy_lwm2m_cached_urc_list()
{
    if (xy_lwm2m_cached_urc_head != NULL)
    {
        clear_xy_lwm2m_cached_urc_list();
    }
    xy_lwm2m_cached_urc_head = xy_malloc(sizeof(xy_lwm2m_cached_urc_head_t));
    memset(xy_lwm2m_cached_urc_head, 0, sizeof(xy_lwm2m_cached_urc_head_t));
}

void clear_xy_lwm2m_cached_urc_list()
{
    if (xy_lwm2m_cached_urc_head == NULL)
    {
        return;
    }
    xy_lwm2m_cached_urc_common_t *node = xy_lwm2m_cached_urc_head->first;
    xy_lwm2m_cached_urc_common_t *temp_node;
    while (node != NULL)
    {
        temp_node = node->next;
        FREEIFNOTNULL(node->urc_data);
        FREEIFNOTNULL(node);
        node = temp_node;
    }

    FREEIFNOTNULL(xy_lwm2m_cached_urc_head);
}

int urc_exists(xy_lwm2m_cached_urc_common_t *node)
{
    xy_lwm2m_cached_urc_common_t *temp = xy_lwm2m_cached_urc_head->first;
    while (temp)
    {
        if (temp->urc_type == node->urc_type && strcmp(temp->urc_data, node->urc_data) == 0)
        {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

int insert_cached_urc_node(xy_lwm2m_cached_urc_common_t *node)
{
    if (xy_lwm2m_cached_urc_head == NULL)
    {
        return -1;
    }
    if (xy_lwm2m_cached_urc_head->last)
    {
        if (urc_exists(node))
            return 0;
        xy_lwm2m_cached_urc_head->last->next = node;
        xy_lwm2m_cached_urc_head->last = node;
        xy_lwm2m_cached_urc_head->num++;
    }
    else
    {
        xy_lwm2m_cached_urc_head->first = xy_lwm2m_cached_urc_head->last = node;
        xy_lwm2m_cached_urc_head->num = 1;
        char *at_str = xy_malloc(32);
        snprintf(at_str, 32, "\r\n+QLAURC: \"buffer\"\r\n");
        send_rsp_str_to_ext(at_str);
        xy_free(at_str);
    }
    return 0;
}

xy_lwm2m_cached_urc_common_t *get_and_remove_cached_urc_first_node()
{
    if (xy_lwm2m_cached_urc_head->num == 0)
    {
        return NULL;
    }
    xy_lwm2m_cached_urc_common_t *node = xy_lwm2m_cached_urc_head->first;

    if (xy_lwm2m_cached_urc_head->num == 1)
    {
        xy_lwm2m_cached_urc_head->first = xy_lwm2m_cached_urc_head->last = NULL;
    }
    else
    {
        xy_lwm2m_cached_urc_head->first = xy_lwm2m_cached_urc_head->first->next;
    }
    xy_lwm2m_cached_urc_head->num--;
    return node;
}

void init_xy_lwm2m_object_info_list()
{
    if (xy_lwm2m_object_info_head != NULL)
    {
        clear_xy_lwm2m_object_info_list();
    }
    xy_lwm2m_object_info_head = xy_malloc(sizeof(xy_lwm2m_object_info_head_t));
    memset(xy_lwm2m_object_info_head, 0, sizeof(xy_lwm2m_object_info_head_t));
}

void clear_xy_lwm2m_object_info_list()
{
    if (xy_lwm2m_object_info_head == NULL)
    {
        return;
    }
    xy_lwm2m_object_info_t *node = xy_lwm2m_object_info_head->first;
    xy_lwm2m_object_info_t *temp_node;
    while (node != NULL)
    {
        temp_node = node->next;
        FREEIFNOTNULL(node);
        node = temp_node;
    }

    FREEIFNOTNULL(xy_lwm2m_object_info_head);
}

int insert_object_info_node(xy_lwm2m_object_info_t *node)
{
    if (xy_lwm2m_object_info_head == NULL)
    {
        return -1;
    }
    node->next = xy_lwm2m_object_info_head->first;
    xy_lwm2m_object_info_head->first = node;
    xy_lwm2m_object_info_head->num++;
    return 0;
}

int remove_object_info_node(int obj_id)
{
    if (xy_lwm2m_object_info_head->num == 0)
    {
        return -1;
    }
    xy_lwm2m_object_info_t *node = xy_lwm2m_object_info_head->first;
    xy_lwm2m_object_info_t *prev = NULL;

    while (node)
    {
        if (node->obj_id == obj_id)
        {
            if (prev)
                prev->next = node->next;
            else
                xy_lwm2m_object_info_head->first = node->next;
            xy_free(node);
            xy_lwm2m_object_info_head->num--;
        }
        prev = node;
        node = node->next;
    }

    return -1;
}

void report_recover_result(int result)
{
    if(result == RESUME_SUCCEED)
        send_urc_to_ext("\r\n+QLAURC: \"recovered\",0\r\n");
    else if(result == RESUME_FLAG_ERROR)
        (void) result;
    else
        send_urc_to_ext("\r\n+QLAURC: \"recovered\",3\r\n");
}

void init_xy_lwm2m_config()
{
    init_xy_lwm2m_cached_urc_list();
    init_xy_lwm2m_object_info_list();
    if (xy_lwm2m_config != NULL)
    {
        clear_xy_lwm2m_config();
    }
    xy_lwm2m_config = xy_malloc(sizeof(xy_lwm2m_config_t));
    memset(xy_lwm2m_config, 0, sizeof(xy_lwm2m_config_t));
    xy_lwm2m_config->lifetime = 86400;
    xy_lwm2m_config->binding_mode = 1;

    xy_lwm2m_config->ack_timeout = 2;
    xy_lwm2m_config->retrans_max_times = 5;

    xy_lwm2m_config->lifetime_enable = 1;
}

void clear_xy_lwm2m_config()
{
    clear_xy_lwm2m_cached_urc_list();
    clear_xy_lwm2m_object_info_list();
    if (xy_lwm2m_config == NULL)
    {
        return;
    }
    FREEIFNOTNULL(xy_lwm2m_config->server_host);
    // FREEIFNOTNULL(xy_lwm2m_config->server_ip);
    FREEIFNOTNULL(xy_lwm2m_config->endpoint_name);
    FREEIFNOTNULL(xy_lwm2m_config->psk_id);
    FREEIFNOTNULL(xy_lwm2m_config->psk);

    FREEIFNOTNULL(xy_lwm2m_config);
}

int generate_config_hex()
{
    onenet_context_config_t *onenet_context_config = &onenet_context_configs[0];
    int ret;
    char *config = NULL;
    int cfg_len = 0;

    // TODO: create_cisnet_sock does query server ip, so we may not have to do this here. Delete it later.
    struct addrinfo hints;
    struct addrinfo *addr_list;
    struct addrinfo *cur;
    struct sockaddr_in *addr;
    char port[6];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    sprintf(port, "%d", xy_lwm2m_config->port);
    ret = getaddrinfo(xy_lwm2m_config->server_host, port, &hints, &addr_list);
    if (ret != 0)
    {
        return -1;
    }
    for (cur = addr_list; cur != NULL; cur = cur->ai_next)
    {
        if (cur->ai_family == AF_INET)
        {
            addr = (struct sockaddr_in *)cur->ai_addr;
            memset(xy_lwm2m_config->server_ip, 0x0, sizeof(xy_lwm2m_config->server_ip));
            inet_ntop(AF_INET, &addr->sin_addr, xy_lwm2m_config->server_ip, 16);
        }
    }
    freeaddrinfo(addr_list);

    config = cis_cfg_tool(xy_lwm2m_config->server_ip, xy_lwm2m_config->port, xy_lwm2m_config->bootstrap_flag, NULL,
                          xy_lwm2m_config->security_mode == 0 ? 1 : 0, xy_lwm2m_config->security_mode == 0 ? xy_lwm2m_config->psk : NULL, &cfg_len);

    FREEIFNOTNULL(onenet_context_config->config_hex);
    memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    onenet_context_config->config_hex = xy_malloc(cfg_len);
    onenet_context_config->total_len = cfg_len;
    memcpy(onenet_context_config->config_hex, config, cfg_len);
    onenet_context_config->index = 0;

    FREEIFNOTNULL(config);
    return 0;
}

int convert_cis_event_to_common_urc(char *at_str, int max_len, int cis_eid, void *param)
{
    switch (cis_eid)
    {
    case CIS_EVENT_REG_SUCCESS:
        snprintf(at_str, max_len, "\r\n+QLAREG: %d", XY_LWM2M_SUCCESS);
        break;
    case CIS_EVENT_REG_FAILED:
        snprintf(at_str, max_len, "\r\n+QLAREG: %d", XY_LWM2M_ERROR);
        break;
    case CIS_EVENT_REG_TIMEOUT:
        snprintf(at_str, max_len, "\r\n+QLAREG: %d", XY_LWM2M_TIMEOUT);
        break;
    case CIS_EVENT_UPDATE_SUCCESS:
        if ((int)param == -1)
            snprintf(at_str, max_len, "\r\n+QLAURC: \"ping\",%d", XY_LWM2M_SUCCESS);
        else if ((int)param == -2)
        {
            if (xy_lwm2m_config->access_mode == 0)
                snprintf(at_str, max_len, "\r\n+QLAURC: \"lifetime_changed\",%d", onenet_context_refs[0].onenet_context->lifetime);
            else
            {
                xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
                memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
                xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_OBSERVE;
                xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
                snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"lifetime_changed\",%d",
                         onenet_context_refs[0].onenet_context->lifetime);
                insert_cached_urc_node(xy_lwm2m_cached_urc_common);
                return -1;
            }
        }
        else if ((int)param == -3)
            snprintf(at_str, max_len, "\r\n+QLAURC: \"binding_changed\",%s", onenet_context_refs[0].onenet_context->server->binding == 1 ? "U" : "UQ");
        else
            snprintf(at_str, max_len, "\r\n+QLAUPDATE: %d,%d", XY_LWM2M_SUCCESS, (int)param);
        break;
    case CIS_EVENT_UPDATE_FAILED:
        if ((int)param != -1)
            snprintf(at_str, max_len, "\r\n+QLAUPDATE: %d,%d", XY_LWM2M_UPDATEFAILED, (int)param);
        else
            snprintf(at_str, max_len, "\r\n+QLAURC: \"ping\",%d", XY_LWM2M_UPDATEFAILED);
        break;
    case CIS_EVENT_UPDATE_TIMEOUT:
        if ((int)param != -1)
            snprintf(at_str, max_len, "\r\n+QLAUPDATE: %d,%d", XY_LWM2M_TIMEOUT, (int)param);
        else
            snprintf(at_str, max_len, "\r\n+QLAURC: \"ping\",%d", XY_LWM2M_TIMEOUT);
        break;
    case CIS_EVENT_UNREG_DONE:
        snprintf(at_str, max_len, "\r\n+QLADEREG: %d", XY_LWM2M_SUCCESS);
        xy_lwm2m_deinit(&onenet_context_refs[0]);
        clear_xy_lwm2m_config();
        break;
    case CIS_EVENT_NOTIFY_SUCCESS:
        snprintf(at_str, max_len, "\r\n+QLAURC: \"report_ack\",%d,%d", XY_LWM2M_SUCCESS, (int)param);
        break;
    case CIS_EVENT_NOTIFY_FAILED:
        snprintf(at_str, max_len, "\r\n+QLANOTIFY: %d", XY_LWM2M_ERROR);
        break;
    case CIS_EVENT_BOOTSTRAP_SUCCESS:
        if (xy_lwm2m_config->access_mode == 0)
            snprintf(at_str, max_len, "\r\n+QLAURC: \"bs_finished\"");
        else
        {
            xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
            memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
            xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_OBSERVE;
            xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
            snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"bs_finished\"");
            insert_cached_urc_node(xy_lwm2m_cached_urc_common);
            return -1;
        }
        break;
    default:
        return -1;
        break;
    }

    return 0;
}

int convert_coap_code_to_commond_result_code(int coap_code)
{
    int result_code;
    switch (coap_code)
    {
    case COAP_NO_ERROR:
        result_code = 0;
        break;
    case COAP_231_CONTINUE:
        result_code = 231;
        break;
    case COAP_400_BAD_REQUEST:
        result_code = 400;
        break;
    case COAP_404_NOT_FOUND:
        result_code = 404;
        break;
    case COAP_500_INTERNAL_SERVER_ERROR:
        result_code = 500;
        break;
    default:
        result_code = XY_LWM2M_ERROR;
        break;
    }
    return result_code;
}

int xy_lwm2m_at_get_read_value(char *at_buf, struct onenet_read *param)
{
    int ret = -1;
    char *src_data = NULL;

    if (param->value_type == cis_data_type_integer)
    {
        if (param->len != 2 && param->len != 4 && param->len != 8)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_float)
    {
        if (param->len != 4 && param->len != 8)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_bool)
    {
        if (param->len != 1)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_string)
    {
        if (param->len > STR_VALUE_LEN)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_opaque)
    {
        if ((param->len * 2) > OPAQUE_VALUE_LEN)
            goto ERR_PROC;
        //param->len = param->len * 2;
    }
    else
    {
        goto ERR_PROC;
    }

    param->value = xy_zalloc(strlen(at_buf));

    if (param->value_type == cis_data_type_string)
    {
        if (get_ascii_data(",,,,,,,%s", (char *)at_buf, param->len, param->value) != AT_OK)
            goto ERR_PROC;

        if (strlen(param->value) != param->len)
            goto ERR_PROC;
    }
    else
    {
        if (param->value_type == cis_data_type_opaque)
        {
            src_data = xy_zalloc(strlen(at_buf));
            if (at_parse_param_2(",,,,,,,%s", (char *)at_buf, (void *)&src_data) != AT_OK)
            {
                goto ERR_PROC;
            }

            if (param->len * 2 != strlen(src_data) || strlen(src_data) == 0)
            {
                goto ERR_PROC;
            }
            if (hexstr2bytes(src_data, param->len * 2, param->value, param->len) == -1)
            {
                goto ERR_PROC;
            }
        }
        else
        {
            if (at_parse_param(",,,,,,,%s", (char *)at_buf, param->value) != AT_OK)
                goto ERR_PROC;

            if (strlen(param->value) == 0)
                goto ERR_PROC;

            if (param->value_type == cis_data_type_integer)
            {
                if ((param->len == 2) && (((int)strtol(param->value, NULL, 10) < INT16_MIN) || ((int)strtol(param->value, NULL, 10) > INT16_MAX)))
                {
                    goto ERR_PROC;
                }
                else if ((param->len == 4) && (((int)strtol(param->value, NULL, 10) < INT32_MIN) || ((int)strtol(param->value, NULL, 10) > INT32_MAX)))
                {
                    goto ERR_PROC;
                }
                else if ((param->len == 8) && (((int)strtol(param->value, NULL, 10) < INT64_MIN) || ((int)strtol(param->value, NULL, 10) > INT64_MAX)))
                {
                    goto ERR_PROC;
                }
            }
            else if (param->value_type == cis_data_type_float)
            {
                if ((param->len == 4) && (atof(param->value) > FLT_MAX))
                {
                    goto ERR_PROC;
                }
                else if ((param->len == 8) && (atof(param->value) > DBL_MAX))
                {
                    goto ERR_PROC;
                }
            }
            else if (param->value_type == cis_data_type_bool)
            {
                if ((int)strtol(param->value, NULL, 10) > 1 || (int)strtol(param->value, NULL, 10) < 0)
                    goto ERR_PROC;
            }
        }
    }
    ret = 0;
ERR_PROC:
    if (src_data != NULL)
        xy_free(src_data);
    return ret;
}

int xy_lwm2m_at_get_notify_value(char *at_buf, struct onenet_notify *param)
{
    int ret = -1;
    char *src_data = NULL;


    if (param->value_type == cis_data_type_integer)
    {
        if (param->len != 2 && param->len != 4 && param->len != 8)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_float)
    {
        if (param->len != 4 && param->len != 8)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_bool)
    {
        if (param->len != 1)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_string)
    {
        if (param->len > STR_VALUE_LEN)
            goto ERR_PROC;
    }
    else if (param->value_type == cis_data_type_opaque)
    {
        if ((param->len * 2) > OPAQUE_VALUE_LEN)
            goto ERR_PROC;
        //param->len = param->len * 2;
    }
    else
    {
        goto ERR_PROC;
    }

    param->value = xy_zalloc(strlen(at_buf));

    if (param->value_type == cis_data_type_string)
    {
        if (get_ascii_data(",,,,,%s,", (char *)at_buf, param->len, param->value) != AT_OK)
            goto ERR_PROC;

        if (strlen(param->value) != param->len)
            goto ERR_PROC;
    }
    else
    {
        if (param->value_type == cis_data_type_opaque)
        {
            src_data = xy_zalloc(strlen(at_buf));
            if (at_parse_param_2(",,,,,%s", (char *)at_buf, (void *)&src_data) != AT_OK)
            {
                goto ERR_PROC;
            }

            if (param->len * 2 != strlen(src_data) || strlen(src_data) == 0)
            {
                goto ERR_PROC;
            }
            if (hexstr2bytes(src_data, param->len * 2, param->value, param->len) == -1)
            {
                goto ERR_PROC;
            }
        }
        else
        {
            if (at_parse_param_2(",,,,,%s", (char *)at_buf, (void *)&param->value) != AT_OK)
                goto ERR_PROC;
            if (param->value_type == cis_data_type_integer)
            {
                if ((param->len == 2) && (((int)strtol(param->value, NULL, 10) < INT16_MIN) || ((int)strtol(param->value, NULL, 10) > INT16_MAX)))
                {
                    goto ERR_PROC;
                }
                else if ((param->len == 4) && (((int)strtol(param->value, NULL, 10) < INT32_MIN) || ((int)strtol(param->value, NULL, 10) > INT32_MAX)))
                {
                    goto ERR_PROC;
                }
                else if ((param->len == 8) && (((int)strtol(param->value, NULL, 10) < INT64_MIN) || ((int)strtol(param->value, NULL, 10) > INT64_MAX)))
                {
                    goto ERR_PROC;
                }
            }
            else if (param->value_type == cis_data_type_float)
            {
                if ((param->len == 4) && (atof(param->value) > FLT_MAX))
                {
                    goto ERR_PROC;
                }
                else if ((param->len == 8) && (atof(param->value) > DBL_MAX))
                {
                    goto ERR_PROC;
                }
            }
            else if (param->value_type == cis_data_type_bool)
            {
                if ((int)strtol(param->value, NULL, 10) > 1 || (int)strtol(param->value, NULL, 10) < 0)
                    goto ERR_PROC;
            }
        }
    }

    ret = 0;
ERR_PROC:
    if (src_data != NULL)
        xy_free(src_data);
    return ret;
}

cis_ret_t xy_lwm2m_notify_data(st_context_t *onenet_context, struct onenet_notify *param)
{
    cis_data_t tmpdata = {0};
    cis_uri_t uri = {0};
    cis_ret_t ret = 0;
    cis_coapret_t result = CIS_NOTIFY_CONTINUE;

    if (param->flag != 0 && param->index == 0)
    {
        return CIS_RET_PARAMETER_ERR;
    }

    if (param->ackid != 0 && param->ackid != 1)
        return CIS_RET_PARAMETER_ERR;

    uri.objectId = param->objId;
    uri.instanceId = param->insId;
    uri.resourceId = param->resId;
    cis_uri_update(&uri);

    if ((ret = onet_read_param((struct onenet_read *)param, &tmpdata)) != CIS_RET_OK)
    {
        return ret;
    }

    if (param->flag == 0 && param->index == 0)
    {
        result = CIS_NOTIFY_CONTENT;
    }
    // if (param->ackid == 0)
    ret = cis_notify(onenet_context, &uri, &tmpdata, param->msgId, result, 0, param->ackid, param->raiflag);

    return ret;
}

extern cis_data_t* prv_dataDup(const cis_data_t* src);
cis_ret_t xy_lwm2m_send_data(st_context_t *onenet_context, struct onenet_notify *param)
{
    cis_data_t tmpdata = {0};
    cis_uri_t uri = {0};
    st_notify_t* notify = NULL;

    if (onenet_context == NULL)
    {
        return CIS_RET_PARAMETER_ERR;
    }
    st_context_t* ctx = (st_context_t*)onenet_context;

    uri.objectId = param->objId;
    uri.instanceId = param->insId;
    uri.resourceId = param->resId;
    cis_uri_update(&uri);

    if (onet_read_param((struct onenet_read *)param, &tmpdata) != CIS_RET_OK)
    {
        return CIS_CALLBACK_NOT_FOUND;
    }

    if (ctx->stateStep != PUMP_STATE_READY)
    {
        return CIS_RET_INVILID;
    }

    if(CIS_LIST_COUNT(ctx->notifyList) >= 10)
        return CIS_RET_MEMORY_ERR;

    notify = (st_notify_t*)cis_malloc(sizeof(st_notify_t));
    cissys_assert(notify != NULL);
    if (notify == NULL)
    {
        return CIS_RET_MEMORY_ERR;
    }
    memset(notify, 0, sizeof(st_notify_t));
    notify->isResponse = false;
    notify->next = NULL;
    notify->id = ++ctx->nextNotifyId;
    notify->mid = param->msgId;
    notify->result = CIS_NOTIFY_CONTENT;
    notify->value = NULL;
    notify->ackID = param->ackid;
    notify->raiflag = param->raiflag;
    notify->report_type = 1;

    notify->value = prv_dataDup(&tmpdata);
    if (notify->value == NULL)
    {
        cis_free(notify);
        return CIS_RET_MEMORY_ERR;
    }
    notify->uri = uri;

    cloud_mutex_lock(&ctx->lockNotify, MUTEX_LOCK_INFINITY);
    ctx->notifyList = (st_notify_t*)CIS_LIST_ADD(ctx->notifyList, notify);
    cloud_mutex_unlock(&ctx->lockNotify);
    //[XY]Add for onenet loop
    if(cis_poll_sem != NULL)
        osSemaphoreRelease(cis_poll_sem);
    return CIS_RET_OK;
}

/*****************************************************************************
  Function	  : at_proc_qlaconfig_req
  Description : set or query the registration parameters for the client
  Input 	  : at_buf	---data buf
				rsp_cmd ---response cmd
  Output	  : None
  Return	  : AT_END
  Eg		  : AT+QLACONFIG=<bootstrap_flag>,<severIP>,<port>,<endpoint_name>,
  					<lifetime>,<security_mode>,[<PSK_ID>,<PSK>][,binding_mode]
*****************************************************************************/
int at_proc_qlaconfig_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        INIT_NULL_PTR(server_host);
        INIT_NULL_PTR(endpoint_name);
        INIT_NULL_PTR(psk_id);
        INIT_NULL_PTR(psk);
        onenet_context_reference_t *onenet_context_ref = &onenet_context_refs[0];
        onenet_context_config_t *onenet_context_config = &onenet_context_configs[0];
        unsigned int err_num = ATERR_XY_ERR;

        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
        	goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (is_xy_lwm2m_running())
        {
            *rsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            return AT_END;
        }
        if (xy_lwm2m_config == NULL)
            init_xy_lwm2m_config();

        server_host = xy_malloc(150);
        GOTOERRORIFNULL(server_host);
        endpoint_name = xy_malloc(150);
        GOTOERRORIFNULL(endpoint_name);

        if (at_parse_param("%d,%s,%d,%s,%d,%d,,,%d", at_buf, &xy_lwm2m_config->bootstrap_flag,
                           server_host, &xy_lwm2m_config->port, endpoint_name,
                           &xy_lwm2m_config->lifetime, &xy_lwm2m_config->security_mode, &xy_lwm2m_config->binding_mode) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }
        if (xy_lwm2m_config->port < 0 || xy_lwm2m_config->port > 65535)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }
        if (xy_lwm2m_config->lifetime < 20 || xy_lwm2m_config->lifetime > 31536000)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }
        if (xy_lwm2m_config->binding_mode != 0 && xy_lwm2m_config->binding_mode != 1)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }
        xy_lwm2m_config->server_host = xy_malloc(strlen(server_host) + 1);
        GOTOERRORIFNULL(xy_lwm2m_config->server_host);
        strcpy(xy_lwm2m_config->server_host, server_host);
        // query server_ip when handling AT+QLAREG, we don't need ps network to be active here.
        xy_lwm2m_config->endpoint_name = xy_malloc(strlen(endpoint_name) + 1);
        GOTOERRORIFNULL(xy_lwm2m_config->endpoint_name);
        strcpy(xy_lwm2m_config->endpoint_name, endpoint_name);
        FREEIFNOTNULL(server_host);
        FREEIFNOTNULL(endpoint_name);
        if (xy_lwm2m_config->security_mode == 0)
        {
            psk_id = xy_malloc(strlen(at_buf));
            GOTOERRORIFNULL(psk_id);
            psk = xy_malloc(strlen(at_buf));
            GOTOERRORIFNULL(psk);
            if (at_parse_param(",,,,,,%s,%s,", at_buf, psk_id, psk) != AT_OK || strlen(psk_id) > 150 || strlen(psk) > 256)
            {
            	err_num = ATERR_PARAM_INVALID;
            	goto error;
            }
            xy_lwm2m_config->psk_id = xy_malloc(strlen(psk_id) + 1);
            GOTOERRORIFNULL(xy_lwm2m_config->psk_id);
            strcpy(xy_lwm2m_config->psk_id, psk_id);
            xy_lwm2m_config->psk = xy_malloc(strlen(psk) + 1);
            GOTOERRORIFNULL(xy_lwm2m_config->psk);
            strcpy(xy_lwm2m_config->psk, psk);
            FREEIFNOTNULL(psk_id);
            FREEIFNOTNULL(psk);
        }

        if (generate_config_hex() < 0)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        osThreadAttr_t thread_attr = {0};

        if (xy_lwm2m_init(onenet_context_ref, onenet_context_config) < 0)
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }
        else
        {
            thread_attr.name = "xy_lwm2m_tk";
            thread_attr.priority = XY_OS_PRIO_NORMAL1;
            thread_attr.stack_size = 0x1000;
            onenet_context_ref->onet_at_thread_id = osThreadNew((osThreadFunc_t)(onet_at_pump), onenet_context_ref, &thread_attr);

            //store factory_nv
            g_softap_fac_nv->onenet_config_len = onenet_context_config->total_len;
            memset(g_softap_fac_nv->onenet_config_hex, 0, sizeof(g_softap_fac_nv->onenet_config_hex));
            memcpy(g_softap_fac_nv->onenet_config_hex, onenet_context_config->config_hex, onenet_context_config->total_len);
            SAVE_FAC_PARAM(onenet_config_hex);
            SAVE_FAC_PARAM(onenet_config_len);
        }

        *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        clear_xy_lwm2m_config();
        FREEIFNOTNULL(server_host);
        FREEIFNOTNULL(endpoint_name);
        FREEIFNOTNULL(psk_id);
        FREEIFNOTNULL(psk);
        if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
        {
            xy_free(onenet_context_config->config_hex);
            memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
        }
        if (onenet_context_ref != NULL)
        {
            if (onenet_context_ref->onenet_context != NULL)
                cis_deinit((void **)&onenet_context_ref->onenet_context);

            if (onenet_context_ref->onet_at_thread_id != NULL)
            {
                osThreadTerminate(onenet_context_ref->onet_at_thread_id);
                onenet_context_ref->onet_at_thread_id = NULL;
            }
            free_onenet_context_ref(onenet_context_ref);
        }
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if(xy_lwm2m_config == NULL || (xy_lwm2m_config->bootstrap_flag == 0 && xy_lwm2m_config->server_host == NULL) ||
                (xy_lwm2m_config->bootstrap_flag == 1 && (onenet_context_refs[0].onenet_context->server == NULL || onenet_context_refs[0].onenet_context->server->sessionH == NULL)))
        {
            *rsp_cmd = gen_at_ok();
        }
        else
        {
            *rsp_cmd = xy_malloc(150);
            sprintf(*rsp_cmd, "\r\n+QLACONFIG: %d,%s,%d,%s,%d,%d", xy_lwm2m_config->bootstrap_flag, xy_lwm2m_config->bootstrap_flag == 1 ? ((cisnet_t)onenet_context_refs[0].onenet_context->server->sessionH)->host : xy_lwm2m_config->server_host,
                    xy_lwm2m_config->bootstrap_flag == 1 ? ((cisnet_t)onenet_context_refs[0].onenet_context->server->sessionH)->port : xy_lwm2m_config->port, xy_lwm2m_config->endpoint_name, xy_lwm2m_config->lifetime, xy_lwm2m_config->security_mode);
            if (xy_lwm2m_config->security_mode == 0)
            {
                sprintf(*rsp_cmd + strlen(*rsp_cmd), ",%s,%s", xy_lwm2m_config->psk_id, xy_lwm2m_config->psk);
            }
            else
            {
                strcat(*rsp_cmd, ",,");
            }
            sprintf(*rsp_cmd + strlen(*rsp_cmd), ",%d\r\n\r\nOK\r\n", xy_lwm2m_config->binding_mode);
        }
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}

/*****************************************************************************
   Function    : at_proc_qlacfg_req
   Description : qurey or config registration parameters 
   Input	   : at_buf  ---data buf
				 rsp_cmd ---response cmd
   Output	   : None
   Return	   : AT_END
   Eg		   : AT+QLACFG="retransmit"[,<ACK_timeout>,<retrans_max_times>]
*****************************************************************************/
int at_proc_qlacfg_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (xy_lwm2m_config == NULL)
        {
            init_xy_lwm2m_config();
        }
        char operation[16] = {0};

        if (at_parse_param("%16s", at_buf, operation) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }
        if (at_strcasecmp(operation, "retransmit") == 0)
        {
            int ack_timeout = -1;
            int retrans_max_times = -1;
            if (at_parse_param(",%d,%d", at_buf, &ack_timeout, &retrans_max_times) != AT_OK)
            {
            	err_num = ATERR_PARAM_INVALID;
            	goto error;
            }
            if (ack_timeout == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\r\n+QLACFG: \"retransmit\",%d,%d\r\n\r\nOK\r\n", xy_lwm2m_config->ack_timeout, xy_lwm2m_config->retrans_max_times);
                goto out;
            }
            else
            {
                if (ack_timeout < 2 || ack_timeout > 20 || retrans_max_times < 0 || retrans_max_times > 8)
                {
                	err_num = ATERR_PARAM_INVALID;
                    goto error;
                }
                xy_lwm2m_config->ack_timeout = ack_timeout;
                xy_lwm2m_config->retrans_max_times = retrans_max_times;
            }
        }
        else if (at_strcasecmp(operation, "auto_ack") == 0)
        {
            int is_auto_ack = -1;
            if (at_parse_param(",%d", at_buf, &is_auto_ack) != AT_OK)
            {
            	err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            if (is_auto_ack == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\r\n+QLACFG: \"auto_ack\",%d\r\n\r\nOK\r\n", xy_lwm2m_config->is_auto_ack);
                goto out;
            }
            else
            {
                if (is_auto_ack != 0 && is_auto_ack != 1)
                {
                	err_num = ATERR_PARAM_INVALID;
                	goto error;
                }
                xy_lwm2m_config->is_auto_ack = is_auto_ack;
            }
        }
        else if (at_strcasecmp(operation, "access_mode") == 0)
        {
            int access_mode = -1;
            if (at_parse_param(",%d", at_buf, &access_mode) != AT_OK)
            {
            	err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            if (access_mode == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\r\n+QLACFG: \"access_mode\",%d\r\n\r\nOK\r\n", xy_lwm2m_config->access_mode);
                goto out;
            }
            else
            {
                if (access_mode != 0 && access_mode != 1)
                {
                	err_num = ATERR_PARAM_INVALID;
                	goto error;
                }
                // xy_lwm2m_config->access_mode = access_mode;
                xy_lwm2m_config->access_mode_alternative = access_mode;
            }
        }
        else if (at_strcasecmp(operation, "platform") == 0)
        {
            int platform = -1;
            if (at_parse_param(",%d", at_buf, &platform) != AT_OK)
            {
            	err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            if (platform == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\r\n+QLACFG: \"platform\",%d\r\n\r\nOK\r\n", xy_lwm2m_config->platform);
                goto out;
            }
            else
            {
                if (platform != 0 && platform != 1 && platform != 2)
                {
                	err_num = ATERR_PARAM_INVALID;
                	goto error;
                }
                xy_lwm2m_config->platform = platform;
            }
        }
        else if (at_strcasecmp(operation, "cfg_res") == 0)
        {
            int device_value = -1;
            if (at_parse_param(",,,,%d", at_buf, &device_value) != AT_OK)
            {
                err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            if (device_value == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\r\n+QLACFG: \"cfg_res\",3,0,17,%d\r\n\r\nOK\r\n", onenet_context_refs[0].onenet_user_config.device_type);
                goto out;
            }
            else
            {
                onenet_context_refs[0].onenet_user_config.device_type = device_value;
            }
        }
        else if (at_strcasecmp(operation, "recovery_mode") == 0)
        {
            int recovery_mode = -1;
            if (at_parse_param(",%d", at_buf, &recovery_mode) != AT_OK)
            {
            	err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            if (recovery_mode == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\r\n+QLACFG: \"recovery_mode\",%d\r\n\r\nOK\r\n", g_softap_var_nv->lwm2m_recovery_mode);
                goto out;
            }
            else
            {
                if (recovery_mode != 0 && recovery_mode != 1)
                {
                	err_num = ATERR_PARAM_INVALID;
                	goto error;
                }
                g_softap_var_nv->lwm2m_recovery_mode = recovery_mode;
            }
        }
        else if (at_strcasecmp(operation, "lifetime_enable") == 0)
        {
            int lifetime_enable = -1;
            if (at_parse_param(",%d", at_buf, &lifetime_enable) != AT_OK)
            {
            	err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            if (lifetime_enable == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\r\n+QLACFG: \"lifetime_enable\",%d\r\n\r\nOK\r\n", xy_lwm2m_config->lifetime_enable);
                goto out;
            }
            else
            {
                if (lifetime_enable != 0 && lifetime_enable != 1)
                {
                	err_num = ATERR_PARAM_INVALID;
                	goto error;
                }
                xy_lwm2m_config->lifetime_enable = lifetime_enable;
            }
        }
        else
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        *rsp_cmd = gen_at_ok();
    out:
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (xy_lwm2m_config == NULL)
        {
            init_xy_lwm2m_config();
        }

        *rsp_cmd = xy_malloc(256);
        snprintf(*rsp_cmd, 256, "\r\n+QLACFG: \"retransmit\",%d,%d\r\n"
                                "+QLACFG: \"auto_ack\",%d\r\n"
                                "+QLACFG: \"access_mode\",%d\r\n"
                                "+QLACFG: \"platform\",%d\r\n"
                                "+QLACFG: \"recovery_mode\",%d\r\n"
                                "+QLACFG: \"lifetime_enable\",%d\r\n"
                                "\r\nOK\r\n",
                 xy_lwm2m_config->ack_timeout, xy_lwm2m_config->retrans_max_times, xy_lwm2m_config->is_auto_ack,
                 /* xy_lwm2m_config->access_mode */ xy_lwm2m_config->access_mode_alternative, xy_lwm2m_config->platform, g_softap_var_nv->lwm2m_recovery_mode,
                 xy_lwm2m_config->lifetime_enable);
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}

/*****************************************************************************
   Function	 	: at_proc_qlareg_req
   Description 	: send registration request to platform
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLAREG
*****************************************************************************/
int at_proc_qlareg_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_ACTIVE)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }
        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        onenet_context_reference_t *onenet_context_ref = &onenet_context_refs[0];
        int ret = CIS_RET_ERROR;

        if (onenet_context_refs[0].onenet_context->registerEnabled == true)
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }

        onenet_context_refs[0].onenet_context->platform_common_type = xy_lwm2m_config->platform;

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_register(onenet_context_ref->onenet_context, xy_lwm2m_config->lifetime);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }

        *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }
}


/*****************************************************************************
   Function    : at_proc_qlaupdate_req
   Description : send update request to platform
   Input	   : at_buf  ---data buf
				 rsp_cmd ---response cmd
   Output	   : None
   Return	   : AT_END
   Eg		   : AT+QLAUPDATE=<mode>,<lifetime/binding_mode>
*****************************************************************************/
int at_proc_qlaupdate_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }
        int ret;
        int mode;
        int value;
        unsigned short mid;

        if (at_parse_param("%d,%d", at_buf, &mode, &value) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        if (mode == 0)
        {
            if (value < 20 || value > 31536000)
            {
            	err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            xy_lwm2m_config->lifetime = value;
        }
        else
        {
            if (value != 0 && value != 1)
            {
            	err_num = ATERR_PARAM_INVALID;
            	goto error;
            }
            xy_lwm2m_config->binding_mode = value;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_update_reg_common(onenet_context_refs[0].onenet_context, xy_lwm2m_config->lifetime,
                                    xy_lwm2m_config->binding_mode, 0, &mid, mode == 0 ? 1 : 0, PS_SOCK_RAI_NO_INFO);
        osMutexRelease(g_onenet_mutex);
        //[XY]Add for onenet loop
        if (cis_poll_sem != NULL)
            osSemaphoreRelease(cis_poll_sem);
        if (ret != CIS_RET_OK)
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }

    out_ok:
        *rsp_cmd = xy_zalloc(40);
        sprintf(*rsp_cmd, "\r\n+QLAUPDATE: %d\r\n", mid);
        strcat(*rsp_cmd, "\r\nOK\r\n");
        // *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }
}


/*****************************************************************************
   Function	 	: at_proc_qladereg_req
   Description 	: send deregisteration request to platform
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLADEREG
*****************************************************************************/
int at_proc_qladereg_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_ACTIVE)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
            err_num = ATERR_NOT_ALLOWED;
            goto error;
        }
        int ret;
        st_context_t *onenet_context = onenet_context_refs[0].onenet_context;
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        if (onenet_context->registerEnabled == true)
            ret = cis_unregister(onenet_context);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }

        *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }
}


/*****************************************************************************
   Function	 	: at_proc_qlaaddobj_req
   Description 	: add object
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLAADDOBJ=<objectID>,<instantID>,<resource_number>,<resourceID>
*****************************************************************************/
int at_proc_qlaaddobj_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        int *res_ids = NULL;
        char *res_ids_fmt = NULL;
        void **params = NULL;
        unsigned int err_num = ATERR_XY_ERR;

        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }

        int ret;
        int obj_id;
        int inst_id;
        int res_num;
        int i;

        if (at_parse_param("%d,%d,%d", at_buf, &obj_id, &inst_id, &res_num) != AT_OK || inst_id > 7)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        res_ids_fmt = xy_malloc(100);
        GOTOERRORIFNULL(res_ids_fmt);
        memset(res_ids_fmt, 0, 100);
        strcpy(res_ids_fmt, ",,");
        res_ids = xy_malloc(res_num * sizeof(int));
        GOTOERRORIFNULL(res_ids);
        memset(res_ids, 0, res_num * sizeof(int));
        params = xy_malloc(res_num * sizeof(void *));
        GOTOERRORIFNULL(params);
        memset(params, 0, res_num * sizeof(void *));
        for (i = 0; i < res_num; i++)
        {
            strcat(res_ids_fmt, ",%d");
            params[i] = &res_ids[i];
        }
        res_ids_fmt[strlen(res_ids_fmt)] = '\0';
        if (at_parse_param_2(res_ids_fmt, at_buf, params) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        cis_inst_bitmap_t bitmap = {0};
        uint8_t instPtr[1] = {0};
        uint16_t instBytes;
        cis_res_count_t rescount;

        bitmap.instanceBitmap = instPtr;
        bitmap.instanceCount = 8;
        bitmap.instanceBytes = 1;
        rescount.attrCount = res_num;
        rescount.actCount = res_num;

        instPtr[0] = 0x80 >> inst_id;
        st_object_t *objectP = prv_findObject(onenet_context_refs[0].onenet_context, obj_id);

        if(objectP != NULL)
        {
            if(instPtr[0] & *objectP->instBitmapPtr)    //obj_id和ins_id 已存在直接报错
            {
                err_num = ATERR_PARAM_INVALID;
                goto error;
            }
            else
            {
                //obj_id存在，ins_id不存在，需先删除obj，再重新添加
                instPtr[0] |= *objectP->instBitmapPtr;
                if(res_num < objectP->attributeCount)
                {
                    rescount.attrCount = objectP->attributeCount;
                    rescount.actCount = objectP->actionCount;
                }
                osMutexAcquire(g_onenet_mutex, osWaitForever);
                cis_delobject(onenet_context_refs[0].onenet_context, obj_id);
                osMutexRelease(g_onenet_mutex);
            }
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_addobject(onenet_context_refs[0].onenet_context, obj_id, &bitmap, &rescount);
        osMutexRelease(g_onenet_mutex);

        if (ret != CIS_RET_OK)
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }
        else
        {
            xy_lwm2m_object_info_t *xy_lwm2m_object_info = xy_malloc(sizeof(xy_lwm2m_object_info_t));
            xy_lwm2m_object_info->obj_id = obj_id;
            xy_lwm2m_object_info->instance_id = inst_id;
            xy_lwm2m_object_info->resource_count = res_num;
            for (i = 0; i < res_num; i++)
            {
                xy_lwm2m_object_info->resouce_ids[i] = res_ids[i];
            }
            if (insert_object_info_node(xy_lwm2m_object_info) < 0)
            {
                xy_free(xy_lwm2m_object_info);
            }
        }

        *rsp_cmd = gen_at_ok();
        if (res_ids_fmt != NULL)
            xy_free(res_ids_fmt);
        if (res_ids != NULL)
            xy_free(res_ids);
        if (params != NULL)
            xy_free(params);
        return AT_END;
    error:
        if (res_ids_fmt != NULL)
            xy_free(res_ids_fmt);
        if (res_ids != NULL)
            xy_free(res_ids);
        if (params != NULL)
            xy_free(params);
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        if (!is_xy_lwm2m_running())
        {
            *rsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            return AT_END;
        }
        if (xy_lwm2m_object_info_head == NULL || xy_lwm2m_object_info_head->num == 0)
        {
            *rsp_cmd = xy_malloc(32);
            strcpy(*rsp_cmd, "\r\n+QLAADDOBJ: \r\n\r\nOK\r\n");
        }
        else
        {
            st_context_t *context = onenet_context_refs[0].onenet_context;
            int i, len, temp_len;
            len = 0;
            *rsp_cmd = xy_malloc(90 * xy_lwm2m_object_info_head->num);
            xy_lwm2m_object_info_t *node = xy_lwm2m_object_info_head->first;
            while (node)
            {
                temp_len = sprintf(*rsp_cmd + len, "\r\n+QLAADDOBJ: %d,%d,%d", node->obj_id, node->instance_id, node->resource_count);
                len += temp_len;
                for (i = 0; i < node->resource_count; i++)
                {
                    temp_len = sprintf(*rsp_cmd + len, ",%d", node->resouce_ids[i]);
                    len += temp_len;
                }
                node = node->next;
            }
            strcat(*rsp_cmd, "\r\n\r\nOK\r\n");
        }
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}

/*****************************************************************************
   Function	 	: at_proc_qladelobj_req
   Description 	: delete object
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLADELOBJ=<objectID>   
 *****************************************************************************/
int at_proc_qladelobj_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        int ret;
        int obj_id;

        if (at_parse_param("%d,%d,%d", at_buf, &obj_id) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_delobject(onenet_context_refs[0].onenet_context, obj_id);
        osMutexRelease(g_onenet_mutex);

        if (ret != CIS_RET_OK)
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }
        else
        {
            remove_object_info_node(obj_id);
        }

        *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlardrsp_req
   Description 	: response for the read request from platform
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLARDRSP=<messageID>,<result>,<objectID>,<instantID>,
   							  <resourceID>,<value_type>,<len>,<value>,<index>  
 *****************************************************************************/
int at_proc_qlardrsp_req(char *at_src_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        struct onenet_read param = {0};
        cis_coapret_t result;
		char *at_buf = NULL;
        int ret = -1;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }

		//因为该命令内部含明文字符串参数，为了避免该参数内部含双引号或逗号的特殊字符，进而进行内存拷贝，以保证get_ascii_data执行转义不影响原始AT命令内容，从而也保证后续的参数能够使用at_parse_param来解析
		at_buf = xy_malloc(strlen(at_src_buf) + 1);
		strcpy(at_buf, at_src_buf);

        if (at_parse_param("%d,%d,%d,%d,%d,%d,%d", at_buf, &param.msgId,
                           &param.result, &param.objId, &param.insId, &param.resId, &param.value_type, &param.len) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        if (param.len > NET_MAX_USER_DATA_LEN)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        if (param.msgId < 0 || param.insId < 0 || param.resId < 0 || param.len < 0)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }


        if (0 != check_coap_result_code(param.result, RSP_READ))
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        if (onet_check_reqType(CALLBACK_TYPE_READ, param.msgId) != 0)
        {
			err_num = ATERR_PARAM_INVALID;
			goto error;
		}

        if ((result = get_coap_result_code(param.result)) != CIS_COAP_205_CONTENT)
        {
            osMutexAcquire(g_onenet_mutex, osWaitForever);
            cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msgId, result, param.raiflag);
            osMutexRelease(g_onenet_mutex);
            goto out_ok;
        }

        //value
        if (0 != xy_lwm2m_at_get_read_value(at_buf, &param))
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        if (at_parse_param(",,,,,,,,%d", at_buf, &param.index) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        // if (onet_set_rai(raiType, &param.raiflag) != 0)
        //     goto param_error;

        if (param.index == 0)
        {
            param.flag = 0;
        }
        else
        {
            param.flag = 2;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_miplread_req(onenet_context_refs[0].onenet_context, &param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
        	err_num = ATERR_NOT_ALLOWED;
        	goto error;
        }
    out_ok:
        *rsp_cmd = gen_at_ok();
        if (param.value != NULL)
            xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
        return AT_END;
    error:
        if (param.value != NULL)
            xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlawrrsp_req
   Description 	: response for the write request from platform
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLAWRRSP=<messageID>,<result>   
 *****************************************************************************/
int at_proc_qlawrrsp_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        int msg_id, result, ret;

        if (at_parse_param("%d,%d", at_buf, &msg_id, &result) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        if (0 != check_coap_result_code(result, RSP_WRITE))
        {
			err_num = ATERR_PARAM_INVALID;
			goto error;
		}

        if (onet_check_reqType(CALLBACK_TYPE_WRITE, msg_id) != 0)
        {
			err_num = ATERR_PARAM_INVALID;
			goto error;
		}

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, msg_id, get_coap_result_code(result), PS_SOCK_RAI_NO_INFO);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            goto error;

        *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlaexersp_req
   Description 	: response for the execute request from platform
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLAEXERSP=<messageID>,<result>   
 *****************************************************************************/
int at_proc_qlaexersp_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        int msg_id, result, ret;

        if (at_parse_param("%d,%d", at_buf, &msg_id, &result) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        if (0 != check_coap_result_code(result, RSP_EXECUTE))
        {
			err_num = ATERR_PARAM_INVALID;
			goto error;
		}

        if (onet_check_reqType(CALLBACK_TYPE_EXECUTE, msg_id) != 0)
        {
			err_num = ATERR_PARAM_INVALID;
			goto error;
		}

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, msg_id, get_coap_result_code(result), PS_SOCK_RAI_NO_INFO);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
			err_num = ATERR_NOT_ALLOWED;
			goto error;
		}

        *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlaobsrsp_req
   Description 	: response for the observe request from platform
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLAOBSRSP=<messageID>,<result>,<objectID>,<instantID>,
   							   <resourceID>,<value_type>,<len>,<value>,<index> 
 *****************************************************************************/
int at_proc_qlaobsrsp_req(char *at_src_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        struct onenet_read param = {0};
		char *at_buf = NULL;
        int ret;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        cis_mid_t temp_observeMid;

		//因为该命令内部含明文字符串参数，为了避免该参数内部含双引号或逗号的特殊字符，进而进行内存拷贝，以保证get_ascii_data执行转义不影响原始AT命令内容，从而也保证后续的参数能够使用at_parse_param来解析
		at_buf = xy_malloc(strlen(at_src_buf) + 1);
		strcpy(at_buf, at_src_buf);

        if (at_parse_param("%d,%d,%d,%d,%d,%d,%d,,%d", at_buf, &param.msgId, &param.result,
                           &param.objId, &param.insId, &param.resId, &param.value_type, &param.len, &param.index) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        if (0 != check_coap_result_code(param.result, RSP_OBSERVE))
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }
        if (onet_check_reqType(CALLBACK_TYPE_OBSERVE, param.msgId) != 0 && onet_check_reqType(CALLBACK_TYPE_OBSERVE_CANCEL, param.msgId) != 0)
        {
        	err_num = ATERR_PARAM_INVALID;
        	goto error;
        }

        //value
        if (0 != xy_lwm2m_at_get_read_value(at_buf, &param))
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        if (param.index == 0)
        {
            param.flag = 0;
        }
        else
        {
            param.flag = 2;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        if (observe_findByMsgid(onenet_context_refs[0].onenet_context, param.msgId) == NULL)
        {
            if (!packet_asynFindObserveRequest(onenet_context_refs[param.ref].onenet_context, param.msgId, &temp_observeMid))
            {
                osMutexRelease(g_onenet_mutex);
                err_num = ATERR_NOT_ALLOWED;
                goto error;
            }
        }

        ret = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, param.msgId, get_coap_result_code(param.result), param.raiflag);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
        	 err_num = ATERR_NOT_ALLOWED;
        	 goto error;
        }

        *rsp_cmd = gen_at_ok();
        if (param.value != NULL)
            xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        if (param.value != NULL)
            xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }
    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlanotify_req
   Description 	: response for the nofity request from platform
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLANOTIFY=<objectID>,<instantID>,<resourceID>,<value_type>,
   							   <len>,<value>,<index>[,<ACK>]
 *****************************************************************************/
int at_proc_qlanotify_req(char *at_src_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        struct onenet_notify param = {0};
        int ret;
        st_observed_t *observe = NULL;
		char *at_buf = NULL;
		
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }
        param.ref = -1;
        param.msgId = -1;
        param.objId = -1;
        param.insId = -1;
        param.resId = -1;
        param.index = -1;
        param.flag = -1;
        void *params[] = {&param.ref, &param.msgId, &param.objId, &param.insId, &param.resId, &param.value_type, &param.len};
        void *params1[] = {&param.index, &param.flag, &param.ackid};
		
		//因为该命令内部含明文字符串参数，为了避免该参数内部含双引号或逗号的特殊字符，进而进行内存拷贝，以保证get_ascii_data执行转义不影响原始AT命令内容，从而也保证后续的参数能够使用at_parse_param来解析
		at_buf = xy_malloc(strlen(at_src_buf) + 1);
		strcpy(at_buf, at_src_buf);

        if (at_parse_param("%d,%d,%d,%d,%d,,%d,%d", at_buf, &param.objId, &param.insId,
                           &param.resId, &param.value_type, &param.len, &param.index, &param.ackid) != AT_OK)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        if (param.len > NET_MAX_USER_DATA_LEN)
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        //value
        if (0 != xy_lwm2m_at_get_notify_value(at_buf, &param))
        {
        	err_num = ATERR_PARAM_INVALID;
            goto error;
        }

        if (param.index == 0)
        {
            param.flag = 0;
        }
        else
        {
            param.flag = 2;
        }

        st_uri_t uriP = {0};
        uri_make(param.objId, param.insId, param.resId, &uriP);
        observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
        if (observe == NULL)
        {
            uri_make(param.objId, param.insId, URI_INVALID, &uriP);
            observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
            if (observe == NULL)
            {
            	err_num = ATERR_NOT_ALLOWED;
                goto error;
            }
        }
        param.msgId = observe->msgid;
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = xy_lwm2m_notify_data(onenet_context_refs[0].onenet_context, &param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK && ret != COAP_205_CONTENT)
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }
        *rsp_cmd = gen_at_ok();
        if (param.value != NULL)
            xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        if (param.value != NULL)
            xy_free(param.value);
		if(at_buf!=NULL)
			xy_free(at_buf);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }

    return AT_END;
}

/*****************************************************************************
 Function    : +ALNOTIFY=<ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,<len>,<value>[,<ackid>[,<raimode>]]
 Description : notify the basic communication suite of a numerical change
 Input       : at_buf  ---data buf
               rsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QLASENDDATA=<value_type>,<len>,<value>,<ACK>
 *****************************************************************************/
int at_proc_qlasend_req(char *at_buf, char **prsp_cmd)
{
    struct onenet_notify param = {0};
    cis_ret_t ret = CIS_RET_INVILID;
    char *hex_data = xy_zalloc(strlen(at_buf));
    uint32_t hex_data_len = 0;
    param.value = NULL;
    st_observed_t *observe = NULL;
    void *params[]  = {&param.value_type, &hex_data_len,hex_data,&param.ackid};

    param.objId = 19;
    param.insId = 1;
    param.resId = 0;

    if (!ps_netif_is_ok())
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
        goto error;
    }

    if(g_softap_var_nv->lwm2m_recovery_mode == 0)
        report_recover_result(resume_net_app(ONENET_TASK));

    if (!is_xy_lwm2m_running())
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        goto error;
    }

    if (g_req_type != AT_CMD_REQ ||  at_parse_param_2("%d,%d,%s,%d", at_buf, params) != AT_OK )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto error;
    }

    if(((param.value_type == 1) && ((hex_data_len > 1024) || (strlen(hex_data) != hex_data_len))) \
        || ((param.value_type == 2) && ((hex_data_len > 512) || (strlen(hex_data) != hex_data_len*2))))
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto error;
    }

    if(param.len > NET_MAX_USER_DATA_LEN ||param.msgId < 0 || param.insId < -1 || param.resId < -1 )
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto error;
    }

    param.value = xy_zalloc(hex_data_len + 1);
    param.len = hex_data_len;
    if (hexstr2bytes(hex_data, hex_data_len * 2, param.value, hex_data_len) == XY_ERR)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        goto error;
    }
    param.value[param.len]= '\0';

    st_uri_t uriP = {0};
    uri_make(param.objId, param.insId, param.resId, &uriP);
    observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
    if (observe == NULL)
    {
        uri_make(param.objId, param.insId, URI_INVALID, &uriP);
        observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
        if (observe == NULL)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            goto error;
        }
    }
    param.msgId = observe->msgid;
    osMutexAcquire(g_onenet_mutex, osWaitForever);
    ret = xy_lwm2m_send_data(onenet_context_refs[0].onenet_context, &param);
    osMutexRelease(g_onenet_mutex);
    if (ret != CIS_RET_OK && ret != COAP_205_CONTENT)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
    }

error:
    if (param.value != NULL)
        xy_free(param.value);
    if (hex_data != NULL)
        xy_free(hex_data);
    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlard_req
   Description 	: read or query the data in the buffer
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLARD?   
 *****************************************************************************/
int at_proc_qlard_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_ACTIVE)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 0)
            report_recover_result(resume_net_app(ONENET_TASK));

        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        if (xy_lwm2m_cached_urc_head->num == 0)
        {
            *rsp_cmd = xy_malloc(32);
            snprintf(*rsp_cmd, 32, "\r\n+QLARD: %d\r\n\r\nOK\r\n", xy_lwm2m_cached_urc_head->num);
        }
        else
        {
            xy_lwm2m_cached_urc_common_t *node = get_and_remove_cached_urc_first_node();
            if (node == NULL)
            {
            	err_num = ATERR_NOT_ALLOWED;
                goto error;
            }
            int temp_len = 32 + strlen(node->urc_data);
            *rsp_cmd = xy_malloc(temp_len);
            snprintf(*rsp_cmd, temp_len, "\r\n+QLARD: %d,%s\r\n\r\nOK\r\n", xy_lwm2m_cached_urc_head->num, node->urc_data);
            FREEIFNOTNULL(node->urc_data);
            FREEIFNOTNULL(node);
        }

        // *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        *rsp_cmd = xy_malloc(32);
        snprintf(*rsp_cmd, 32, "\r\n+QLARD: %d\r\n\r\nOK\r\n", xy_lwm2m_cached_urc_head->num);
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }

    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlstatus_req
   Description 	: query the LwM2M status
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLASTATUS?
 *****************************************************************************/
int at_proc_qlstatus_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_QUERY)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        int status;
        if (!ps_netif_is_ok())
        {
        	err_num = ATERR_NOT_NET_CONNECT;
            goto error;
        }

        if (!is_xy_lwm2m_running())
        {
            if(NET_NEED_RECOVERY(ONENET_TASK))
                status = 7;
            else
                status = 5;
        }
        else
        {
            st_context_t *onenet_context = onenet_context_refs[0].onenet_context;
            if (onenet_context->registerEnabled == true)
            {
                switch (registration_getStatus(onenet_context))
                {
                case STATE_REGISTERED:
                {
                    status = 2;
                    break;
                }

                case STATE_REG_FAILED:
                {
                    status = 0;
                    break;
                }

                case STATE_REG_PENDING:
                default:
                        status = 1;
                    break;
                }
            }
            else
            {
                status = 0;
            }
        }
        *rsp_cmd = xy_malloc(32);
        snprintf(*rsp_cmd, 32, "\r\n+QLASTATUS: %d\r\n\r\nOK\r\n", status);

        // *rsp_cmd = gen_at_ok();
        return AT_END;
    error:
        *rsp_cmd = AT_ERR_BUILD(err_num);
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }

    return AT_END;
}


/*****************************************************************************
   Function	 	: at_proc_qlarecover_req
   Description 	: recover the LwM2M session manually
   Input		: at_buf  ---data buf
  			   	  rsp_cmd ---response cmd
   Output 	 	: None
   Return 	 	: AT_END
   Eg 		 	: AT+QLARECOVER
 *****************************************************************************/
int at_proc_qlarecover_req(char *at_buf, char **rsp_cmd)
{
    if (g_req_type == AT_CMD_ACTIVE)
    {
		unsigned int err_num = ATERR_XY_ERR;
        *rsp_cmd = xy_malloc(30);
        if (!ps_netif_is_ok() || is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        if(g_softap_var_nv->lwm2m_recovery_mode == 1)
            report_recover_result(resume_net_app(ONENET_TASK));
        else
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }
        if (!is_xy_lwm2m_running())
        {
        	err_num = ATERR_NOT_ALLOWED;
            goto error;
        }

        snprintf(*rsp_cmd, 30, "\r\n+QLARECOVER:0\r\n\r\nOK\r\n");
        return AT_END;
    error:
        snprintf(*rsp_cmd, 30, "\r\n+QLARECOVER:3\r\n\r\nOK\r\n");
        return AT_END;
    }
    else
    {
        *rsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
    }

    return AT_END;
}

#endif
