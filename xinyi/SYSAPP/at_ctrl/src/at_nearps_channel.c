/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ctl.h"
#include "at_global_def.h"
#include "inter_core_msg.h"
#include "xy_utils.h"
#include "zero_copy_shm.h"

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
void at_rcv_from_nearps(void *buf, unsigned int len)
{
	is_SRAM_addr((unsigned int)(buf));
	xy_assert(strlen(buf) != 0 && len == strlen(buf));

	send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, buf, len, NEARPS_DSP_FD);
	softap_printf(USER_LOG, WARN_LOG, "recv_shm_from_ps:%s", buf);
	xy_free((void*)buf);
}

//the caller need to free the memory--buf
int at_nearps_fd_write(int nearps_fd, void *buf, int size)
{
	UNUSED_ARG(nearps_fd);
	UNUSED_ARG(size);
	int result = XY_OK;

	void *at_cmd = xy_zalloc_Align(strlen(buf) + 1);
	memcpy(at_cmd, buf, strlen(buf));

	result = shm_zero_copy(at_cmd, strlen(buf) + 1, ICM_AT);

	if (result == XY_ERR)
	{
		char *ret = AT_ERR_BUILD(ATERR_DSP_NOT_RUN);
		send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, ret, strlen(ret), NEARPS_DSP_FD);
		xy_free(ret);
		return result;
	}
	softap_printf(USER_LOG, WARN_LOG, "send_shm:%s", buf);
	return result;
}