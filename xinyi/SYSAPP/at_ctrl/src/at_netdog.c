/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_netdog.h"
#include "at_global_def.h"
#include "xy_utils.h"
#include "xy_system.h"

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
#define APP_LOG_ERR(a, b)    \
	if (b != 0)              \
	{                        \
		softap_printf(USER_LOG, WARN_LOG, a, b); \
	}

int at_NETDOG_req(char *at_buf, char **prsp_cmd)
{
	char cmd[10] = {0};

	if(g_req_type != AT_CMD_REQ)
    {
        *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
    }

	if (at_parse_param("%10s", at_buf, cmd) != AT_OK || strlen(cmd) == 0)
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	if (!strncmp(cmd, "AT", strlen(cmd)))
	{
		APP_LOG_ERR("dbg_drop_nearps_num=%d\n", at_netdog.dbg_drop_nearps_num);
		APP_LOG_ERR("dirty_sum=%d\n", at_netdog.dirty_sum);
		APP_LOG_ERR("tail_dirty_sum=%d\n", at_netdog.tail_dirty_sum);
		APP_LOG_ERR("all_dirty_sum=%d\n", at_netdog.all_dirty_sum);
	}
	else if (!strncmp(cmd, "NET", strlen(cmd)))
	{
		APP_LOG_ERR("drop_up_sum=%d\n", at_netdog.dbg_drop_up_sum);
		APP_LOG_ERR("drop_down_sum=%d\n", at_netdog.dbg_drop_down_sum);
	}
	else if (!strncmp(cmd, "PS", strlen(cmd)))
	{
		APP_LOG_ERR("pdp_act_succ=%d\n", at_netdog.dbg_pdp_act_succ);
		APP_LOG_ERR("pdp_act_fail=%d\n", at_netdog.dbg_pdp_act_fail);
		APP_LOG_ERR("pdp_deact_num=%d\n", at_netdog.dbg_pdp_deact_num);
	}
	else if (!strncmp(cmd, "DM", strlen(cmd)))
	{
		APP_LOG_ERR("dbg_dm_no_need_start_num=%d\n", at_netdog.dbg_dm_no_need_start_num);
		APP_LOG_ERR("dbg_dm_no_get_iccid_fail=%d\n", at_netdog.dbg_dm_no_get_iccid_fail);
		APP_LOG_ERR("dbg_dm_success_num=%d\n", at_netdog.dbg_dm_success_num);
		APP_LOG_ERR("dbg_dm_failed_num=%d\n", at_netdog.dbg_dm_failed_num);
	}
	else if (!strncmp(cmd, "PSM", strlen(cmd)))
	{
		APP_LOG_ERR("poweron_cause=%d\n", get_sys_up_stat()); //see  SYS_START_STAT
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
        return AT_END;
	}
	
	return AT_END;
}