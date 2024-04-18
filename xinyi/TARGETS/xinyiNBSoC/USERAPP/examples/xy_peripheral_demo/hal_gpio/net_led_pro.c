/**
* @file        net_led_pro.c
* @ingroup     peripheral
* @brief       LED指示灯，指示当前驻网情况
* @attention   1，由于硬件上有些限制，如果要使用，请咨询我们的硬件FAE;
*              2，目前默认未使用该源文件;
*              3，如用户使用该源文件，需要修改NV参数g_softap_fac_nv->peri_remap_enable & 0x0100为0时，用户方可自行控制LED灯
* @par	点灯规则：
* @par							亮                        灭
* @par			上电默认注网：	   64ms    800ms
* @par			注网成功		   64ms    2000ms
* @par			idle		   0ms	   ~ms
* @par			PSM			   0ms	   ~ms
* @par			PDP deact      64ms    800ms
* @par			PDP act		   64ms    2000ms
***********************************************************************************/

#include "xy_api.h"
#include "xy_atc_interface.h"
#include "at_xy_cmd.h"

/*
0: off
1: on
2: flash	
*/
int led_type = 0,led_on = 0;

void timer_led_callback(void)
{
	if (led_type == 0) {
		HAL_GPIO_WritePin(LED_NUM,HAL_GPIO_PIN_RESET);
	} else if(led_type == 1) {
		HAL_GPIO_WritePin(LED_NUM,HAL_GPIO_PIN_SET);
	} else {
		if (led_on == 1) {
			HAL_GPIO_WritePin(LED_NUM,HAL_GPIO_PIN_RESET);
			led_on = 0;
		} else {
			HAL_GPIO_WritePin(LED_NUM,HAL_GPIO_PIN_SET);
			led_on = 1;
		} 
	}
}

osTimerId_t timer_led = NULL;
void init_timer_led(void)
{
	osTimerAttr_t timer_attr = {0};
	
	timer_attr.name = "led_tmr";
	timer_led = osTimerNew((osTimerFunc_t)(timer_led_callback), osTimerPeriodic, NULL, &timer_attr);

	//timer maybe delay
	osTimerSetLowPowerFlag(timer_led, osLowPowerProhibit);
	osTimerStart(timer_led, 500);
	//osTimerStop(timer_led);
}

int at_CGEV_led_info(char *at_buf)
{
	char *at_paras = NULL;
	
	if ((at_paras = strstr(at_buf, "PDN ACT")) != NULL)
	{
		led_type = 1;
	}
	else if ((at_paras = strstr(at_buf, "PDN DEACT")) != NULL)
	{
		led_type = 2;
	}
	else if ((at_paras = strstr(at_buf, "OOS")) != NULL)
	{
		led_type = 2;
	}
	else if ((at_paras = strstr(at_buf, "IS")) != NULL)
	{
		led_type = 2;
	}
	return AT_END;
}

int at_CSCON_led_info(char *at_buf)
{
	int con_state = -1;

//	printf("%d/%s entry %s \r\n",__LINE__,__FUNCTION__,at_buf);

	if(strstr(at_buf,",") || strstr(at_buf,"-") || strstr(at_buf,")"))
		return AT_END;
	
	if (at_parse_param("%d", at_buf,&con_state) != AT_OK)
	{
		return AT_END;
	}
	if(con_state == 1)
		led_type = 1;
	else if(con_state == 0)
		led_type = 2;
	return AT_END;
}

/**
 * @brief 用户任务初始化函数，在user_task_init中添加
 * @attention   
 */	
void user_led_demo_init(void)
{
	HAL_GPIO_InitTypeDef gpio_init;

//	printf("%d/%s entry.\r\n",__LINE__,__FUNCTION__);
	if (g_softap_fac_nv->peri_remap_enable & 0x0100)
	{
		printf("peri_remap_enable  ERROR!!!");
		return ;
	}
	
	if((g_softap_fac_nv != NULL) && (g_softap_fac_nv->led_pin <= HAL_GPIO_PIN_NUM_63))
	{
		gpio_init.Pin		= LED_NUM;
		gpio_init.Mode		= GPIO_MODE_OUTPUT_PP;
		gpio_init.Pull		= GPIO_NOPULL;
		HAL_GPIO_Init(&gpio_init);
	}
	register_app_at_urc("+CGEV:",at_CGEV_led_info);
	register_app_at_urc("+CSCON:", at_CSCON_led_info);
	
	led_type = 2;
	init_timer_led();
}
