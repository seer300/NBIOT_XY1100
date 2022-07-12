/**
* @file        user_flash_demo.c
* @ingroup     peripheral
* @brief       用户使用带有磨损功能和内容保护机制算法的接口实例。
* @attention	凡是使用该接口进行磨损和容错机制的，用户可使用的空间大小皆为XY_FTL_AVAILABLE_SIZE，其中头和尾部添加了保护字段。
* @par			用户根据写flash的频繁度，决定size的大小，若size=FLASH_SECTOR_LENGTH，则代表放弃磨损均衡，仅使用芯翼的头尾保护机制，以解决异常断电的flash容错。
* @par			客户完全可以自行设计磨损和内容保护机制，可以调用xy_Flash_Read和xy_flash_write接口。
* @par			目前芯翼的flash寿命为10万次，当写频率小于一小时，必须考虑磨损机制，否则无法达到10年使用期限。
*/

#if 1//DEMO_TEST

#include "xy_api.h"
#include "at_xy_cmd.h"

#define  USER_DATA_FLASH_BASE1		USER_FLASH_BASE
#define  USER_DATA_FLASH_LEN1		2*FLASH_SECTOR_LENGTH

#define  USER_DATA_FLASH_BASE2		(USER_DATA_FLASH_BASE1+USER_DATA_FLASH_LEN1)
#define  USER_DATA_FLASH_LEN2		2*FLASH_SECTOR_LENGTH

/*
typedef struct
{	
    unsigned char    atWakeup;   
}user_nv_data_t;
*/
typedef struct
{
    unsigned char    param1;
    unsigned char    param2;
    unsigned char    param3;
    unsigned char    param4;
	unsigned char    param5;
}user_cloud_data_t;

//user_nv_data_t *g_user_nv = NULL;
user_cloud_data_t *g_user_cloud_data_demo = NULL;
extern user_nv_data_t *g_user_nv;


/*
为了保证flash的使用年限，用户应尽量减小写flash的次数，如果不同的数据在多处发生变化，可以在需要保存时在同一个接口一次性写入。
*/
void  write_user_nv_demo()
{
	xy_assert(sizeof(user_nv_data_t) <= PAGE_AVAILABLE_SIZE);
	if(xy_ftl_write((unsigned int)(USER_DATA_FLASH_BASE1),0,(unsigned char *)g_user_nv,sizeof(user_nv_data_t)) != XY_OK)
	{
		xy_assert(0);
	}
	
}

void  read_user_nv_demo()
{
	char temp[64]={0};
	g_user_nv = (user_nv_data_t *)xy_malloc(sizeof(user_nv_data_t));

	if(xy_ftl_read((unsigned int)(USER_DATA_FLASH_BASE1),(unsigned char*)(g_user_nv),sizeof(user_nv_data_t)) != XY_OK)
	{
		xy_printf("not have valid user NV!!!maybe init here!!!");
		g_user_nv->atWakeup = 0;
		sprintf(temp,"g_user_nv_data_demo.atWakeup000=%d",g_user_nv->atWakeup);
		xy_printf(temp);	
		write_user_nv_demo();		
	}
	else
	{		
		sprintf(temp,"g_user_nv_data_demo.atWakeup111=%d",g_user_nv->atWakeup);
		xy_printf(temp);	
	}
	
	
}

void  init_user_flash()
{
	
	//数据长度为2*FLASH_SECTOR_LENGTH,表示两块0x1000的flash参与磨损，交替写入。
	xy_regist_ftl_req(USER_DATA_FLASH_BASE1,USER_DATA_FLASH_LEN1);
	//xy_regist_ftl_req(USER_DATA_FLASH_BASE2,USER_DATA_FLASH_LEN2);
	xy_printf("init_user_flash");
}
#endif



