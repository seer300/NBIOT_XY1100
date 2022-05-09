1、若始终在线场景，根据NV参数fota_period开设软timer，如一周，超时后执行FOTA升级检查；
2、若支持深睡模式，则在易变NV中添加上一次FOTA检查时刻点，每次上电连上网后，检测是否已超过fota_period时长，如一周，若超过，则执行FOTA升级检查；

abup_init-》abup_check_update_timeout流程看不懂