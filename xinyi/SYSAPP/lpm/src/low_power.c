#include "low_power.h"
#include "lpm_config.h"
#include "lpm_adapt.h"
#include "watchdog.h"
#include "oss_nv.h"
extern unsigned int icm_buf_check();

void vApplicationIdleHook()
{
	//switch to external 32768 crystal

	Switch_32k_Xtal();

	icm_buf_check();

	//bug4505
	Check_ioldo_Bypass();
	
	#if (DSP_DYNAMIC_ON_OFF == 1)
	if(shut_down_dspcore_process() == SHUTDOWN_DSP_REQ)
		return;
	#endif
	
#if (XY_DEEPSLEEP==1)
	if (LPM_Deepsleep_Allow())
	{	
		//AT+FASTOFF=1：Soc not into DEEPSLEEP for slow discharge of capacitor,and user must poweroff directly
		if(LPM_Fastoff_Powerdown_Check())
		{
			LPM_Fastoff_Powerdown_Process();
		}
		//normal deepsleep or AT+FASTOFF=0(save NV and then deepsleep,could be wakeup as normal deepsleep)
		else
		{
			LPM_Deepsleep_Process();
		}
		return;
	} else
#endif

#if (XY_STANDBY==1)
	if(LPM_Standby_Allow())
	{
		//standby for dual-core mode
		LPM_Standby_Process();
		return;
				
	} else
#endif

	{
#if (XY_STANDBY==1)
		//M3 only mode,enter standby
		if(LPM_Standby_Allow_SingleCore())
		{
			//standby for single-core mode
			LPM_Standby_Process_SingleCore();
			return;
		}
#endif

#if  (XY_WFI==1)
		if(LPM_Wfi_Allow())
		{
			LPM_Wfi_Process();
			return;
		}
		
#endif
	}
}

