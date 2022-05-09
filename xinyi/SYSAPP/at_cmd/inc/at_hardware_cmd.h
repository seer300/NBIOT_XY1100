#pragma once

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/

/**
  * @brief  引脚测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+XYCNNT=<bit map>
  * @arg 查询类AT命令：AT+XYCNNT?
  * @arg 测试类AT命令：AT+XYCNNT=?
  */
int at_XYCNNT_req(char *at_buf, char **prsp_cmd);

/**
  * @brief  ADC测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 查询类AT命令：AT+ZADC?
  * @arg 测试类AT命令：AT+ZADC=?  
  * @arg 设置类AT命令：AT+ZADC=mode
  */
int at_ZADC_req(char *at_buf, char **prsp_cmd);

/**
  * @brief  VBAT电压测试AT命令处理函数，显示结果为VBAT电压（单位mV）
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 测试AT命令：AT+VBAT=? 
  */
int at_VBAT_req(char *at_buf, char **prsp_cmd);

/**
  * @brief  AT串口配置AT命令（可配置波特率（必选），超时时间（可选），是否存储在NV中（可选），同步模式（可选），停止位（可选））
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+NATSPEED=<baud_rate>[,<timeout>[,<store>[,<sync_mode>[,<stopbits>]]]]
  *      @arg <baud_rate>,可配置波特率（必选）：2400,4800,9600,57600,115200,230400,460800
  *      @arg <timeout>,超时时间（可选）
  *      @arg <store>,是否存储在NV中（可选），1为是，0为否
  *      @arg <sync_mode>,同步模式（可选）
  *      @arg <stopbits>,停止位（可选）:1,1.5,2
  * @arg 查询类AT命令：AT+NATSPEED?
  * @arg 测试类AT命令：AT+NATSPEED=?
  */
int at_NATSPEED_req(char *at_buf, char **prsp_cmd);

/**
  * @brief  wakeup引脚控制选择
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+RESETCTL=<mode>
  *      @arg <mode>:0，表示按键小于20ms唤醒，超过20ms复位，默认值
  *      @arg <mode>:1，表示按键小于6s唤醒，超过6s复位
  * @arg 查询类AT命令：AT+RESETCTL?
  */
int at_RESETCTL_req(char *at_buf, char **prsp_cmd);

/**
  * @brief  AT串口配置(波特率（必选），是否存储在NV中（可选），是否打开standby（可选）)
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+UARTSET=<baud_rate>[,<store>[,<open standby>]]
  *      @arg <baud_rate>,可配置波特率（必选）：2400,4800,9600,57600,115200,230400,460800
  *      @arg <store>,是否存储在NV中（可选），1为是，0为否
  *      @arg <open standby>,是否打开standby，1为是，0为否
  * @arg 查询类AT命令：AT+UARTSET?
  * @arg 测试类AT命令：AT+UARTSET=?
  */
int at_UARTSET_req(char *at_buf, char **prsp_cmd);

/**
  * @brief  AT串口配置(波特率)
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+IPR=<baud_rate>
  *      @arg <baud_rate>,可配置波特率：0,4800,9600,19200,38400,57600,115200,230400,460800,921600;0表示启用波特率自适应
  *		 若设置的波特率高于9600，则同时修改NV关闭standby
  * @arg 查询类AT命令：AT+IPR?
  * @arg 测试类AT命令：AT+IPR=?
  */
int at_IPR_req(char *at_buf, char **prsp_cmd);
/**
  * @brief 芯片当前温度和电池电压
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval int
  * @arg 请求类AT命令：AT+QCHIPINFO=<cmdadc_channel>
  *      @arg <cmd>, ALL:所有数据；TEMP当前温度（摄氏度）；VBAT:电池电压（mv）
  * @arg 查询类AT命令：AT+QCHIPINFO=?
  */
int at_QCHIPINFO_req(char *at_buf,char **prsp_cmd);
/**
 * @brief 看门狗喂狗
 * 
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 * @arg 请求类AT命令：AT+WTD=<reset timer>[,<>]
 * @arg <reset timer> 看门狗下次进行重启的时间，单位秒，最高不得高于131071秒
 * @note 此指令已废弃，不建议客户使用
 */
int at_WTD_req(char *at_buf, char **prsp_cmd);

#if  WAKEUP_MCU_BY_URC
void wakeup_MCU_by_URC();
int at_CMSRI_req(char *at_buf, char **prsp_cmd);
#endif

/**
  * @brief  ADC测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 执行类AT命令：AT+QADC  +QADC: 0,<voltage>
  * @arg 测试类AT命令：AT+QADC=? +QADC: 支持的channel列表
  * @arg 设置类AT命令：AT+QADC=<channel>    +QADC: <channel>,<voltage>
    */
int at_QADC_req(char *at_buf, char **prsp_cmd);
/**
  * @brief  查询模块的供电电压值，显示结果为VBAT接口的电压值（单位mV）
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 测试类AT命令：AT+CBC=?  
  * @arg 执行类AT命令：AT+CBC  
  */
int at_CBC_req(char *at_buf, char **prsp_cmd);