/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "at_global_def.h"
#include "at_com.h"
#include "factory_nv.h"
#include "rtc_tmr.h"
#include "xy_at_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
int g_remoteat_isenable = 0;
char *g_Remote_AT_Rsp = NULL;

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
int g_FOTAing_flag = 0;
#if WAKEUP_MCU_BY_URC
int g_ring_enable = 0;
int g_duration = 120;
#endif
struct at_debug_stat at_netdog = {0};
at_ctrl_callback g_at_ctrl_cbk = {0};

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
static const at_err_msg_t at_err_msg_tbl[] = {
    ERR_AT_IT(ATERR_XY_ERR),
    ERR_AT_IT(ATERR_PARAM_INVALID),
    ERR_AT_IT(ATERR_NOT_ALLOWED),
    ERR_AT_IT(ATERR_DROP_MORE),
    ERR_AT_IT(ATERR_DOING_FOTA),
    ERR_AT_IT(ATERR_MORE_PARAM),
    ERR_AT_IT(ATERR_WAIT_RSP_TIMEOUT),
    ERR_AT_IT(ATERR_CHANNEL_BUSY),
    ERR_AT_IT(ATERR_DSP_NOT_RUN),
    ERR_AT_IT(ATERR_NOT_NET_CONNECT),
    ERR_AT_IT(ATERR_NEED_OFFTIME),
    ERR_AT_IT(ATERR_CONN_NOT_CONNECTED),
    ERR_AT_IT(ATERR_INVALID_PREFIX),

};

static const at_err_num_t at_error_num_table[] = {
	{ATERR_XY_ERR				, 308},
	{ATERR_PARAM_INVALID		, 50},
	{ATERR_NOT_ALLOWED			, 3},
	{ATERR_DROP_MORE			, 25},
	{ATERR_DOING_FOTA			, 126},
	{ATERR_MORE_PARAM			, 301},
	{ATERR_WAIT_RSP_TIMEOUT		, 301},
	{ATERR_CHANNEL_BUSY			, 302},
	{ATERR_DSP_NOT_RUN			, 303},
	{ATERR_NOT_NET_CONNECT		, 30},
	{ATERR_NEED_OFFTIME			, 301},
	{ATERR_CONN_NOT_CONNECTED	, 30},
	{ATERR_INVALID_PREFIX		, 301},
	{ATERR_LOW_VOL				, 60},
};

char *get_at_err_string(int error_number)
{
    int i;
    for (i = 0; i < (int)(sizeof(at_err_msg_tbl) / sizeof(at_err_msg_tbl[0])); ++i)
    {
        if (at_err_msg_tbl[i].error_number == error_number)
            return at_err_msg_tbl[i].errname;
    }
    return NULL;
}

int get_at_err_num(int error_number)
{
    int i;
    for (i = 0; i < (int)(sizeof(at_error_num_table) / sizeof(at_error_num_table[0])); ++i)
    {
        if (at_error_num_table[i].xy_err == error_number)
            return at_error_num_table[i].quectel_err;
    }
    
    return -1;
}

extern int at_ReqAndRsp_to_ps(char *req_at, char *info_fmt, int timeout, ...);
/*先在一个下行报文中发送AT+REMOTECTL打开AT远程调试功能g_remoteat_isenable开关；再在下一个下行报文中发送正常的AT+xxx命令，*/
/*被当前函数截获后，调用at_ReqAndRsp_to_ps接口执行AT命令，并将应答结果保存到g_Remote_AT_Rsp全局中，最后由各种云封装后发送给云服务器*/
int xy_Remote_AT_Req(char *req_at)
{
    int ret = 0;
    char *new_at_req;
    char *temp = req_at;
    //char *rsp = NULL;

	//avoid dirty user data
    if (strstr(req_at, "AT+") == NULL)
    {
    	g_remoteat_isenable = 0;
        return 0;
    }

    if (g_remoteat_isenable == 0)
    {
        if (strstr(req_at, REMOTE_AT_CTRL) != NULL)
        {
            g_remoteat_isenable = 1;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    if (g_Remote_AT_Rsp != NULL)
        xy_free(g_Remote_AT_Rsp);

    g_Remote_AT_Rsp = NULL;

    //find all AT string
    while (is_at_char(*temp))
    {
        temp++;
    }

    new_at_req = xy_zalloc(temp - req_at + 3);
    memcpy(new_at_req, req_at, temp - req_at);
    *(new_at_req + (int)(temp - req_at)) = '\r';
    *(new_at_req + (int)(temp - req_at) + 1) = '\n';

    g_Remote_AT_Rsp = xy_zalloc(1000);
    ret = at_ReqAndRsp_to_ps(new_at_req, "%s", 60, &g_Remote_AT_Rsp);

    if (strlen(new_at_req) < 100)
        softap_printf(USER_LOG, WARN_LOG, "Remote AT Req:%s", new_at_req);
    else
        softap_printf(USER_LOG, WARN_LOG, "Remote AT Req for test!");

    xy_free(new_at_req);
    if (ret == AT_OK)
    {
        return 1;
    }
    else
    {
        memset(g_Remote_AT_Rsp, 0, 1000);
        sprintf(g_Remote_AT_Rsp, "AT_ERROR:%d", ret);
        return 0;
    }
}

//if have remote AT Req,will replace data by AT response for TEST
int xy_Remote_AT_Rsp(char **rsp_data, int *len)
{
    if (g_Remote_AT_Rsp != NULL)
    {
        xy_assert(*rsp_data != NULL);

        xy_free(*rsp_data);

        if (strlen(g_Remote_AT_Rsp) < 100)
            softap_printf(USER_LOG, WARN_LOG, "Remote AT Response for test:%s", g_Remote_AT_Rsp);
        else
            softap_printf(USER_LOG, WARN_LOG, "Remote AT Response for test!");

        *rsp_data = g_Remote_AT_Rsp;
        g_Remote_AT_Rsp = NULL;
        *len = strlen(*rsp_data);
    }
    return 0;
}


