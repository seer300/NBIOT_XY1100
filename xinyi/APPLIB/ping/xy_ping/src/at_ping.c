#include "xy_at_api.h"
#include "at_ping.h"
#include "at_ps_cmd.h"
#include "ping.h"
#include "lwip/sockets.h"
#include "ps_netif_api.h"
#include "xy_utils.h"

#if XY_PING

/**
 * @brief AT+NPING=<host>,<data_len>,<ping_num>,<time_out>,<interval_time>[,<rai>]
 */
int at_NPING_req(char *at_buf,char **prsp_cmd)
{
	char *host = xy_zalloc(REMOTE_SERVER_LEN);
	int data_len = 0;
	int ping_num = 0;
	int time_out = 0;
	int interval_time = 0;
	int rai_val = -1;

    if (at_parse_param("%100s,%d,%d,%d(1-),%d,%d[0-2]", at_buf, host, &data_len, &ping_num, &time_out, &interval_time, &rai_val) != AT_OK || strlen(host) == 0 || data_len > XY_PING_MAX_DATA_LENGTH)
    {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto out;
	}
	if (xy_wait_tcpip_ok(0) != XY_OK)
    {
        softap_printf(USER_LOG, WARN_LOG, "ps netif is not ok!");
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		goto out;
    }
	if (g_OOS_flag == 1)
	{
    	softap_printf(USER_LOG, WARN_LOG, "ps netif is OOS!");
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		goto out;
    }
    /**
     * ping第一包成功后，网侧一般等15~20s 下发链路释放；而在ping后面包时容易和前面的链路释放碰撞，导致上行ping发出后得不到回包。
     * 如果采用单包发送的方式ping包或者两次ping包间隔超过20s，可以设置rai=2, 告知网络ping包收到回包后就没有后续数据了，网络会快速释放链路。
     */
    if (rai_val == -1)
    {
        if (ping_num == 1 || interval_time > 20)
            rai_val = 2;
        else
            rai_val = 0;
    }

    if (start_ping(AF_INET, host, data_len, ping_num, time_out, interval_time, rai_val) == XY_ERR)
	{
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
    }

out:
    xy_free(host);
	return AT_END;
}

int at_NPINGSTOP_req(char *at_buf,char **prsp_cmd)
{
    UNUSED_ARG(at_buf);
    UNUSED_ARG(prsp_cmd);
    stop_ping();
	return AT_END;
}

/**
 * @brief AT++NPING6=<host>,<data_len>,<ping_num>,<time_out>,<interval_time>[,<rai>]
 */
int at_NPING6_req(char *at_buf,char **prsp_cmd)
{
	char *host = xy_zalloc(REMOTE_SERVER_LEN);
	int data_len = 0;
	int ping_num = 0;
	int time_out = 0;
	int interval_time = 0;
	int rai_val = -1;

	if (at_parse_param("%100s,%d,%d,%d(1-),%d,%d[0-2]", at_buf, host, &data_len, &ping_num, &time_out, &interval_time, &rai_val) != AT_OK || strlen(host) == 0 || data_len > XY_PING_MAX_DATA_LENGTH)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		goto out;
	}
	if (xy_wait_tcpip_ok(0) != XY_OK)
    {
        softap_printf(USER_LOG, WARN_LOG, "ps netif is not ok!");
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		goto out;
    }
	if (g_OOS_flag == 1)
	{
    	softap_printf(USER_LOG, WARN_LOG, "ps netif is OOS!");
		*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		goto out;
    }
    /**
     * ping第一包成功后，网侧一般等15~20s 下发链路释放；而在ping后面包时容易和前面的链路释放碰撞，导致上行ping发出后得不到回包。
     * 如果采用单包发送的方式ping包或者两次ping包间隔超过20s，可以设置rai=2, 告知网络ping包收回包后就没有后续数据了，网络会快速释放链路。
     */
    if (rai_val == -1)
    {
        if (ping_num == 1 || interval_time > 20)
            rai_val = 2;
        else
            rai_val = 0;
    }

	if (start_ping(AF_INET6, host, data_len, ping_num, time_out, interval_time, rai_val) == XY_ERR)
	{
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
    }

out:
	xy_free(host);
	return AT_END;
}

#endif //XY_PING
