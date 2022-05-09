#include "at_wireshark.h"
#include "xy_at_api.h"
#include "xy_utils.h"
#include "xy_wireshark.h"

#if XY_WIRESHARK

//AT+WIRESHARK=<capture_type>
//capture_type:1-将M3接收到的数据以wireshark形式输出，0-取消M3输出数据
//用于M3判断是否接收到协议栈上报的数据
int at_WIRESHARK_req(char *at_buf, char **prsp_cmd)
{
	int capture_type = -1;
	struct common_msg *msg = NULL;
	osThreadAttr_t thread_attr = {0};
	
	if (at_parse_param("%d", at_buf, &capture_type) != AT_OK || (capture_type != 1 && capture_type != 0))
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
		return AT_END;
	}

	softap_printf(USER_LOG, WARN_LOG, "wireshark origin: %d, new: %d", g_wireshark_set, capture_type);
	g_wireshark_set = capture_type;

	if (capture_type == 1)
	{
		if (g_wireshark_TskHandle == NULL)
		{
			thread_attr.name	   = "wireshark_task";
			thread_attr.priority   = XY_OS_PRIO_NORMAL1;
			thread_attr.stack_size = 0x800;
			g_wireshark_TskHandle = osThreadNew((osThreadFunc_t)(wireshark_task), NULL, &thread_attr);
		}
	}
	else
	{
		if (g_wireshark_TskHandle != NULL)
		{
			msg = xy_zalloc(sizeof(struct common_msg));
			msg->msg_id = WIRESHARK_END;
			osMessageQueuePut(wireshark_msg_q, &msg, 0, osWaitForever);
		}
	}
	return AT_END;
}
#endif //XY_WIRESHARK
