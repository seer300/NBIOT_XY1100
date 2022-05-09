#include "xy_flash.h"
#include "flash_adapt.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "oss_nv.h"
#include "xy_at_api.h"
#include "xy_sys_hook.h"

#define  EFTL_MAGIC_NUM    0X5A5A5A5A


#define EFTL_PAGE_OFFSET_ADDR(ADDR,INDEX,OFFSET) (*((uint32_t *)(ADDR+EFTL_PAGE_SIZE*INDEX+OFFSET)))

struct xy_eftl_cfg {
	struct xy_eftl_cfg *next;
	uint32_t addr;//flash start addr
	uint32_t tpage_total;//true pages,such as 5
};

struct xy_eftl_cfg *user_nv_cfgs = NULL;


/**
 * @brief  少数客户有高频率写小数据到flash中的需求，如每两秒写4字节到flash中。由于flash寿命只有10万次，这样很容易造成坏块.\n
 * 芯翼平台复用4K retension mem来解决高频率写flash问题，用户仅需把私有小数据保存到该全局中即可，芯翼平台后台会保证不丢失数据。具体请参考user_task_volatile_demo.c
 * @attention   如果使用，必须联系芯翼的FAE确认 \n
 *  用户能保存的长度上限为USER_VOL_DATA_LEN，否则会造成系统各种异常
*/
void *g_user_vol_data=NULL;

/*指示是否由M3核强制通过寄存器关闭DSP，该全局用来指示是否可以本地擦写flash*/
uint8_t g_have_suspend_dsp = 0;

//指示是否为烧完版本第一次开机，当为1时表示没有工作态出厂NV
int  g_first_poweron = 0;
/*****************************************************************************
 Description : read data from flash
 Input       : addr: flash addr,if size > 1024,must 64 BYTES aligned
 			   data: ram addr,if size > 1024,must 64 BYTES aligned
 			   size:
 Output      : 
 Return      : 0,succ; -1,normal err
 note        :flash maybe bad if abnormal power off while writing or erasing,so must check flash context,such as magic num or checksum
 *****************************************************************************/
int xy_flash_read(uint32_t addr,void *data, uint32_t size)
{
	if(!(((addr >= FLASH_BASE) && (addr <= FLASH_BASE+FLASH_LENGTH)) 
		&& ((((uint32_t)data >= SRAM_BASE) && ((uint32_t)data <= SRAM_BASE+SRAM_LENGTH)) || ((uint32_t)data == BACKUP_MEM_BASE))))
	{
		return XY_ERR;
	}

	flash_read(addr,data, size);
	return  XY_OK;
}

uint32_t ftl_read_write_num(uint32_t addr)
{
	uint32_t addr_aligned = addr - (addr&(EFTL_PAGE_SIZE-1));

	return EFTL_PAGE_OFFSET_ADDR(addr_aligned,0,EFTL_WRITE_NUM_OFFSET);

}


/*****************************************************************************
 Description : write data to flash,and will erase before write.can not call often,else maybe bad block 
 Input       : addr: flash addr
 			   data: ram addr
 			   size:
 Output      : 
 Return      : 0,succ; others fail,such as low voltage(-7)
 note        :由于异常断电会造成flash内容损坏，所以客户必须自行添加头尾校验位来确保内容的有效性。 \n
              由于双核工作时写flash是在DSP执行的，为了解决cache脏数据问题，用户必须保证data是cache行对齐的
 *****************************************************************************/
int xy_flash_write_ext(uint32_t addr,void *src, uint32_t size)
{	
	int ret = XY_OK;
	xy_assert((addr >= FLASH_BASE) && (addr <= FLASH_BASE+FLASH_LENGTH) && ((uint32_t)src >= SRAM_BASE) && ((uint32_t)src <= SRAM_BASE+SRAM_LENGTH));

	unsigned char * data = NULL;
	unsigned char * flash_data = NULL;
	if(xy_check_low_vBat(0) == XY_ERR)
		return  XY_Err_LowVbat;

	data = (unsigned char *)xy_zalloc(size);
	memcpy(data, src, size);
	
	if(DSP_IS_NOT_LOADED() || g_have_suspend_dsp)
	{
		flash_write_erase(addr,data,size);
	}
	else
	{
		flash_operate_ext(flash_op_writeErase,addr,data,size);
	}

	flash_data = (unsigned char *)xy_zalloc(size);
	flash_read(addr, (unsigned char *) flash_data, size);
	if(memcmp(flash_data, data, size) != 0)
	{
		if(g_softap_fac_nv->off_debug == 0)
			xy_assert(0);
		ret = XY_ERR;
	}

	if(data != NULL)
	{
		xy_free(data);
		data = NULL;
	}

	if(flash_data != NULL)
	{
		xy_free(flash_data);
		flash_data = NULL;
	}
	
	return  ret;
}

/*****************************************************************************
 Description : write data to flash,and will erase before write.can not call often,else maybe bad block 
 Input       : addr: flash addr
 			   data: ram addr
 			   size:
 Output      : 
 Return      : 0,succ; others fail,such as low voltage(-7)
 note        :由于异常断电会造成flash内容损坏，所以客户必须自行添加头尾校验位来确保内容的有效性。 \n
              由于双核工作时写flash是在DSP执行的，为了解决cache脏数据问题，用户必须保证data是cache行对齐的
              *此接口仅限芯翼内部使用，客户二次开发不得使用*
 *****************************************************************************/
int xy_flash_write(uint32_t addr,void *data, uint32_t size)
{	
	xy_assert((addr >= FLASH_BASE) && (addr <= FLASH_BASE+FLASH_LENGTH) && ((uint32_t)data >= SRAM_BASE) && ((uint32_t)data <= SRAM_BASE+SRAM_LENGTH));

	if(xy_check_low_vBat(0) == XY_ERR)
		return  XY_Err_LowVbat;
	if(DSP_IS_NOT_LOADED() || g_have_suspend_dsp)
	{
		flash_write_erase(addr,data,size);
	}
	else
	{
		flash_operate_ext(flash_op_writeErase,addr,data,size);
	}

	return  XY_OK;
}

/*****************************************************************************
 Description : write data to flash directly,and not need erase before write.can not call often,else maybe bad block  
 Input       : addr: flash addr
 			   data: ram addr
 			   size:
 Output      : 
 Return      : 0,succ; others fail,such as low voltage(-7)
 note        :flash maybe bad if abnormal power off while writing or erasing,so must check flash context,such as magic num or checksum
 *****************************************************************************/
int xy_flash_write_no_erase(uint32_t addr,void *data, uint32_t size)
{	
	xy_assert((addr >= FLASH_BASE) && (addr <= FLASH_BASE+FLASH_LENGTH) && ((uint32_t)data >= SRAM_BASE) && ((uint32_t)data <= SRAM_BASE+SRAM_LENGTH));

	if(xy_check_low_vBat(0) == XY_ERR)
		return XY_Err_LowVbat;
	if(DSP_IS_NOT_LOADED() || g_have_suspend_dsp)
	{
		flash_write_no_erase(addr,data,size);
	}
	else
	{
		flash_operate_ext(flash_op_writeNoErase,addr, data,size);
	}
	return  XY_OK;
}


/*****************************************************************************
 Description : erase  flash block,can not call often,else maybe bad block 
 Input       : addr: flash addr,must block aligned,such as 0x27008000
 			   size: blocks,must is multiple blocks, *1024
 Output      : 
 Return      : 0,succ; others fail,such as low voltage(-7)
 note        : flash maybe bad if abnormal power off while writing or erasing,so must check flash context,such as magic num or checksum
 *****************************************************************************/
int xy_flash_erase(uint32_t addr, uint32_t size)
{
	if(size < FLASH_SECTOR_LENGTH)
		size = FLASH_SECTOR_LENGTH;

	if(xy_check_low_vBat(0) == XY_ERR)
		return XY_Err_LowVbat;
	
	if(DSP_IS_NOT_LOADED() || g_have_suspend_dsp)
		flash_erase(addr,size);
	else
	{
		flash_operate_ext(flash_op_erase,addr,NULL, size);
	}
	return XY_OK;
}

int  is_ftl_flash_addr(uint32_t addr)
{
	struct xy_eftl_cfg *user_nv_cfgs_temp = user_nv_cfgs;	
	while(user_nv_cfgs_temp != NULL)
	{
		if(user_nv_cfgs_temp->addr<=addr && user_nv_cfgs_temp->addr+user_nv_cfgs_temp->tpage_total*FLASH_SECTOR_LENGTH >addr)
		{
			return XY_OK;
		}
		user_nv_cfgs_temp = user_nv_cfgs_temp->next;
	}
	return XY_ERR;
}

struct xy_eftl_cfg *find_valid_ftl_cfg(uint32_t addr)
{
	struct xy_eftl_cfg *user_nv_cfgs_temp = user_nv_cfgs;	
	while(user_nv_cfgs_temp != NULL)
	{
		if(user_nv_cfgs_temp->addr == addr)
		{
			return user_nv_cfgs_temp;
		}
		user_nv_cfgs_temp = user_nv_cfgs_temp->next;
	}
	return NULL;
}

/**
 * @brief   根据头尾，选择有效的块，并可选返回下一个代写的块;对于芯翼使用的NV，为了减少memcpy，做了定制，保证头部预留4字节用于5A5A5A5A
 * 
 * @param addr              [IN]flash  addr
 * @param p_cur_ftl_idx     [OUT]当前头尾有正常值的索引，若未找到，返回-1
 * @param p_next_ftl_idx    [OUT]下次准备写入的索引，第一次从0开始
 * @return int              bool
 */
int xy_choose_valid_ftl(uint32_t addr, uint8_t *data, int check_csm, int *p_cur_ftl_idx, int *p_next_ftl_idx)
{
	int valid_page;
	uint32_t max_writenum = 0;
	uint32_t i = 0;
	struct xy_eftl_cfg *nv_cfgs_temp = NULL;
	uint32_t write_num;
	char  *str=NULL;

	nv_cfgs_temp = find_valid_ftl_cfg(addr);
	xy_assert(nv_cfgs_temp != NULL);
	
CHOOSE_AGAIN:

	valid_page = -1;

	for(i = 0; i<nv_cfgs_temp->tpage_total; i++)
	{
		write_num = EFTL_PAGE_OFFSET_ADDR(addr,i,EFTL_WRITE_NUM_OFFSET);
		if(EFTL_PAGE_OFFSET_ADDR(addr,i,0) == EFTL_MAGIC_NUM 
			&& write_num != 0XFFFFFFFF 
			&& write_num != 0)
		{
			if(max_writenum <= write_num)
			{
				max_writenum = write_num;
				valid_page = i;
			}
		}
		else
		{
			if(addr == NV_FLASH_FACTORY_BASE)
			{
				str = xy_malloc(64);
				sprintf(str,"+DBGINFO:find index err %lX m3\r\n", addr+EFTL_PAGE_SIZE*i);
				send_debug_str_to_at_uart(str);
				xy_free(str);
			}
		}
	}
	
	if(valid_page != -1)
	{
		xy_flash_read(nv_cfgs_temp->addr+FLASH_SECTOR_LENGTH*valid_page, data, FLASH_SECTOR_LENGTH);
		if(check_csm==1 && (*((uint32_t *)(data+EFTL_CSM_OFFSET)) != xy_chksum((void*)data,EFTL_CSM_OFFSET)))
		{
			str = xy_malloc(64);
			sprintf(str,"+DBGINFO:checksum err %lX m3\r\n", nv_cfgs_temp->addr+FLASH_SECTOR_LENGTH*valid_page);
			send_debug_str_to_at_uart(str);
			xy_free(str);

			xy_flash_erase(nv_cfgs_temp->addr+FLASH_SECTOR_LENGTH*valid_page,FLASH_SECTOR_LENGTH);
			goto CHOOSE_AGAIN;
		}

		*p_cur_ftl_idx = valid_page;
		if(p_next_ftl_idx != NULL)
			*p_next_ftl_idx = (valid_page+1) % nv_cfgs_temp->tpage_total;
	}
	else
	{
		*p_cur_ftl_idx = -1;
		if(p_next_ftl_idx != NULL)
			*p_next_ftl_idx = 0;
	}

	return XY_OK;
}

void reset_factory_param(softap_fac_nv_t *data)
{
#if VER_QUECTEL
	data->ack_timeout = 4;
	data->obs_autoack = 1;
	data->write_format = 0;
#else
	data->ack_timeout = 4;
	data->obs_autoack = 0;
	data->write_format = 0;
#endif
}

//对于芯翼使用的NV，为了减少memcpy，做了定制，保证头部预留4字节用于5A5A5A5A
int xy_ftl_read(uint32_t addr, uint8_t *data, uint32_t size)
{
	uint32_t offset = addr&(FLASH_SECTOR_LENGTH-1);
	uint32_t addr_align = addr - offset;
	uint8_t *temp = NULL;
	char *str = NULL;
	int next_ftl_idx = -1;
	int cur_ftl_idx = -1;

	//开启offtime时,不保存云的数据，也无需读取
	if(g_softap_fac_nv->offtime == 1 && addr >= NV_FLASH_NET_BASE && addr < NV_FLASH_NET_BASE + XY_FTL_AVAILABLE_SIZE)
		return XY_OK;
	
	xy_assert(size <= XY_FTL_AVAILABLE_SIZE);
	xy_assert((addr+size) <= (addr_align+EFTL_CSM_OFFSET)); 
	/*NV整块读取时为了减少一次拷贝，目的内存包含了头部4字节魔术数字*/
	if((addr==NV_FLASH_FACTORY_BASE || addr==NV_FLASH_DSP_VOLATILE_BASE) && (size == XY_FTL_AVAILABLE_SIZE))
	{
		temp = data;
		size += 4;
	}
	else
	{
		temp = xy_malloc_Align(FLASH_SECTOR_LENGTH);
	}
	
	xy_choose_valid_ftl(addr_align,temp,1,&cur_ftl_idx, &next_ftl_idx);

	if(cur_ftl_idx == -1)
	{
		if((temp != NULL)&&(temp != data))
		{
			xy_free(temp);
		}
		/* first poweron, use main factory nv */
		if(addr_align==NV_FLASH_FACTORY_BASE)
		{
			g_first_poweron = 1;
			send_debug_str_to_at_uart("+DBGINFO:factory use main factory base M3\r\n");
			flash_read(NV_FLASH_MAIN_FACTORY_BASE + offset, data, size);
			reset_factory_param((softap_fac_nv_t *)data);
			return XY_OK;
		}
		else
		{
			str = xy_malloc(100);
			sprintf(str,"+DBGINFO:0x%x can not find valid ftl M3\r\n",(int)addr);
			send_debug_str_to_at_uart(str);
			xy_free(str);
			return XY_ERR;
		}
	}

	/*芯翼自己的NV读取，不执行此处的拷贝，以减少一次大拷贝，目的内存包含了头部4字节魔术数字*/
	if(temp != data)
	{
		memcpy(data,temp+4+offset,size);
		xy_free(temp);
	}
	
	return XY_OK;
}


//对于芯翼使用的NV，为了减少memcpy，做了定制，保证头部预留4字节用于5A5A5A5A
int xy_ftl_write(uint32_t page_addr_base,int offset, uint8_t *data, uint32_t size)
{
	uint32_t write_num = 0;
	int next_ftl_idx = -1;
	int cur_ftl_idx = -1;
	uint8_t *p_page_buf = NULL;
	char *str=NULL;
	int   ret = -1;
	int  check_csm = 1;

	if(g_softap_fac_nv->offtime==1 && page_addr_base==NV_FLASH_NET_BASE)
		return XY_OK;
	
	xy_assert((offset+size) <= XY_FTL_AVAILABLE_SIZE);
	p_page_buf = xy_malloc_Align(FLASH_SECTOR_LENGTH);
	
	/* 对于芯翼的NV，为了避免读时的一次大拷贝，目的地址预留了头部4字节保存魔术数字，此处的入参需要跳过该4字节*/
	if(((page_addr_base==NV_FLASH_FACTORY_BASE) && ((uint32_t)data == RAM_FACTORY_NV_BASE))
		|| ((page_addr_base==NV_FLASH_DSP_VOLATILE_BASE) && ((uint32_t)data == BACKUP_MEM_BASE))
		|| ((page_addr_base==NV_FLASH_DSP_NON_VOLATILE_BASE) && ((uint32_t)data == RAM_INVAR_NV_BASE)))
	{
		data += 4;
		check_csm = 0;
	}
	
	xy_choose_valid_ftl(page_addr_base,p_page_buf,check_csm,&cur_ftl_idx, &next_ftl_idx);

	//flash中没有有效NV，一般是首次写入
	if(cur_ftl_idx == -1) 
	{
		/* 出厂NV尚未保存，用main factory nv */
		if(page_addr_base == NV_FLASH_FACTORY_BASE)
		{
			send_debug_str_to_at_uart("+DBGINFO:factory nv have not writed,and first write! M3\r\n");
			xy_flash_read(NV_FLASH_MAIN_FACTORY_BASE,p_page_buf, FLASH_SECTOR_LENGTH);
		}
		else
		{
			/* buf清零防止首次写入时带入非零数据 */
			memset(p_page_buf,0,FLASH_SECTOR_LENGTH);
			str = xy_malloc(100);
			snprintf(str,100,"+DBGINFO:addr 0x%x have not writed,and first write! M3\r\n",(int)page_addr_base);
			send_debug_str_to_at_uart(str);
			xy_free(str);
		}
	}

	//if new data is equal old data, Don't write flash
	if(cur_ftl_idx != -1 && memcmp((p_page_buf+4+offset),data,size) == 0)
	{
		xy_free(p_page_buf);
		return XY_OK;
	}
	else
	{
		if(page_addr_base == NV_FLASH_FACTORY_BASE)
		{
			send_debug_str_to_at_uart("+DBGINFO:factory nv write! M3\r\n");
		}
		else if(page_addr_base == NV_FLASH_DSP_VOLATILE_BASE)
		{
			send_debug_str_to_at_uart("+DBGINFO:var nv write! M3\r\n");	
		}
		else if(page_addr_base == NV_FLASH_NET_BASE)
		{
			send_debug_str_to_at_uart("+DBGINFO:NET nv write! M3\r\n");
		}		
		else if(page_addr_base == NV_FLASH_DSP_NON_VOLATILE_BASE)
		{
			send_debug_str_to_at_uart("+DBGINFO:NON_VOLATILE nv write! M3\r\n");	
		}
		else
		{
			send_debug_str_to_at_uart("+DBGINFO:other nv write! M3\r\n");
		}
	}
	
	*((uint32_t *)(p_page_buf)) = 0X5A5A5A5A;

	memcpy((p_page_buf+4+offset),data,size);
	*((uint32_t*)(p_page_buf+EFTL_CSM_OFFSET)) = xy_chksum((void*)p_page_buf,EFTL_CSM_OFFSET);

	write_num = *((uint32_t *)(p_page_buf+EFTL_WRITE_NUM_OFFSET));
	if(write_num == 0XFFFFFFFF)
		write_num = 0;
	write_num++;
	*((uint32_t *)(p_page_buf+EFTL_WRITE_NUM_OFFSET)) = write_num;

	ret = xy_flash_write(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx,p_page_buf,FLASH_SECTOR_LENGTH);

	if(ret != XY_OK)
	{
		xy_free(p_page_buf);
		return ret;
	}
	
	xy_assert(*(uint32_t*)(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx) == 0X5A5A5A5A);
	
	xy_free(p_page_buf);

	return XY_OK;
}

int xy_ftl_erase(uint32_t addr, uint32_t size)
{
	uint32_t i;
	uint32_t sectors;
	uint32_t addr_tmp;
	
	xy_assert(((addr%FLASH_SECTOR_LENGTH) == 0) && ((size%FLASH_SECTOR_LENGTH) == 0));

	sectors = size/FLASH_SECTOR_LENGTH;
	
	for(i = 0; i < sectors; i ++)
	{
		addr_tmp = addr + i * FLASH_SECTOR_LENGTH;
		
		if((EFTL_PAGE_OFFSET_ADDR(addr_tmp,0,0) != 0xFFFFFFFF) 
			|| (EFTL_PAGE_OFFSET_ADDR(addr_tmp,0,EFTL_WRITE_NUM_OFFSET) != 0xFFFFFFFF))
		{
			xy_flash_erase(addr_tmp, FLASH_SECTOR_LENGTH);
		}
	}
	
	return XY_OK;

}

void xy_regist_ftl_req(uint32_t addr,uint32_t size)
{
    struct xy_eftl_cfg *new_user_nv_cfgs = (struct xy_eftl_cfg*)xy_malloc(sizeof(struct xy_eftl_cfg));

	xy_assert(addr%FLASH_SECTOR_LENGTH==0 && size%FLASH_SECTOR_LENGTH==0);
    new_user_nv_cfgs->next = NULL;
    new_user_nv_cfgs->addr = addr;
	new_user_nv_cfgs->tpage_total = size/FLASH_SECTOR_LENGTH;
	
    if (user_nv_cfgs == NULL)
    {
        user_nv_cfgs = new_user_nv_cfgs;
    }
	else
	{
		new_user_nv_cfgs->next = user_nv_cfgs;
		user_nv_cfgs = new_user_nv_cfgs;
	}
}
/* main函数前调用，查找factory nv的当前块 */  
uint32_t CheckFactoryNvValid(void)
{
	uint32_t max_writenum = 0;
	uint32_t writenum;
	uint32_t i;
	int valid_page = -1;
	uint32_t cur_fac_addr = 0;
	
	for(i = 0; i<NV_FLASH_FACTORY_LEN/EFTL_PAGE_SIZE; i++)
	{
		writenum = EFTL_PAGE_OFFSET_ADDR(NV_FLASH_FACTORY_BASE,i,EFTL_WRITE_NUM_OFFSET);
		if(EFTL_PAGE_OFFSET_ADDR(NV_FLASH_FACTORY_BASE,i,0) == (int)(0x5A5A5A5A) && writenum != 0XFFFFFFFF && writenum != 0)
		{
			if(max_writenum <= writenum)
			{
				max_writenum = writenum;
				valid_page = i;
			}
		}
	}
	if(valid_page != -1)
	{
		cur_fac_addr = NV_FLASH_FACTORY_BASE+EFTL_PAGE_SIZE*valid_page;
	}
	
	return cur_fac_addr;
}

#if FLASH_PROTECT
/* flash保护，根据需要保护的flash范围设置保护模式，详见FLASH_PROTECT_MODE的定义，处于保护范围内的flash只允许读不允许写和擦 */ 
void xy_flash_protect(FLASH_PROTECT_MODE protect_mode, uint8_t cmp)
{
	xy_assert((cmp == 0) || (cmp == 1));
	
	if(g_DSP_state==DSP_STATE_NOT_WORK || g_DSP_state==DSP_STATE_STOPPED)
	{
		flash_protect(protect_mode, cmp);
	}
	else
	{
		flash_protect_ext(protect_mode, cmp);
	}
}
#endif

void xy_nv_ftl_init()
{
	xy_regist_ftl_req(NV_FLASH_FACTORY_BASE, 			NV_FLASH_FACTORY_LEN);
	xy_regist_ftl_req(NV_FLASH_DSP_VOLATILE_BASE, 		NV_FLASH_DSP_VOLATILE_LEN);
	xy_regist_ftl_req(NV_FLASH_DSP_NON_VOLATILE_BASE, 	NV_FLASH_DSP_NON_VOLATILE_LEN);
	xy_regist_ftl_req(NV_FLASH_NET_BASE, 				NV_FLASH_NET_LEN);
}


