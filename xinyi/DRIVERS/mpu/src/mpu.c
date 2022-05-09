#include "xy_mpu.h"
#include "core_cm3.h"
#include "sys_debug.h"
#include "string.h"
char region_hash[8]={0};
uint32_t watchpoint_background_enable(void)
{
	MPU_Type * mpu_reg = MPU;

	unsigned char region =0;

	mpu_reg->CTRL &= ~(MPU_CTRL_PRIVDEFENA_Msk|MPU_CTRL_ENABLE_Msk);

	mpu_reg->RBAR = 0x0 |(1<<MPU_RBAR_VALID_Pos)|(region<<MPU_RBAR_REGION_Pos);

	mpu_reg->RASR = MPU_RASR_XN_ENABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_WRITE_BACK_NOT_ALLOCATE|(0x1F<<MPU_RASR_SIZE_Pos)|1;

	region_hash[0]=1;

	return 0;
}


uint32_t enable_watchponit(uint32_t addr, uint32_t power, uint32_t rw, uint32_t region)
{
	MPU_Type * mpu_reg = MPU;

	Sys_Assert(region > 0 && region <8);
	Sys_Assert(region_hash[region] == 0);

	watchpoint_background_enable();

	addr&=~(1<<(power-1));

	mpu_reg->RBAR = addr |(1<<MPU_RBAR_VALID_Pos)|(region<<MPU_RBAR_REGION_Pos);

	switch (rw)
	{
		case 0:
			// 读写
			mpu_reg->RASR = MPU_RASR_XN_DISABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_WRITE_BACK_NOT_ALLOCATE|((power-1)<<MPU_RASR_SIZE_Pos)|1;
			break;
		case 1:
			// 只读
			mpu_reg->RASR = MPU_RASR_XN_DISABLE | MPU_AP_PRIV_RO_USR_RO | MPU_MEM_WRITE_BACK_NOT_ALLOCATE|((power-1)<<MPU_RASR_SIZE_Pos)|1;
			break;
		case 2:
			// 不可读不可写
			mpu_reg->RASR = MPU_RASR_XN_DISABLE | MPU_AP_ALL_PROHIBIT | MPU_MEM_WRITE_BACK_NOT_ALLOCATE|((power-1)<<MPU_RASR_SIZE_Pos)|1;
			break;
	}

	mpu_reg->CTRL |=MPU_CTRL_ENABLE_Msk;

	region_hash[region] = 1;

	__DSB();

	__ISB();

	return 0;

}

void disable_watchponit(void)
{
	MPU_Type * mpu_reg = MPU;

	memset(region_hash,0,8);

	mpu_reg->CTRL &= ~(MPU_CTRL_PRIVDEFENA_Msk|MPU_CTRL_ENABLE_Msk);

  	__DSB();

	__ISB();

}

unsigned char g_already_mpu = 0;
void Enable_MPU_Interrupt_Protect()
{
	if(!g_already_mpu)
	{
		// protect the whole flash before the XIP mode been set
		enable_watchponit(0x2711F000, 12, 2, 1);
		enable_watchponit(0x27120000, 17, 2, 2);
		enable_watchponit(0x27140000, 18, 2, 3);
		enable_watchponit(0x27180000, 19, 2, 4);
		g_already_mpu++;
	}
	else
	{
		g_already_mpu++;
	}

}
void Disable_MPU_Interrupt_protect()
{
	if(g_already_mpu == 1)
	{
		disable_watchponit();
	}
	else
	{
		g_already_mpu--;
	}
}

void MPU_config()
{
#if 0

#if defined (__MPU_PRESENT) && (__MPU_PRESENT == 1U)

	MPU_Type * mpu_reg = MPU;
	if(0 == (mpu_reg->TYPE&&MPU_TYPE_DREGION_Msk))
	{
		//MPU Not Support
		return ;
	}
	//disable MPU / privdefena /MPU in  NMI and hard fault
	mpu_reg->CTRL &= ~(MPU_CTRL_PRIVDEFENA_Msk|MPU_CTRL_ENABLE_Msk);

	mpu_reg->RBAR = SRAM_BASE |(1<<MPU_RBAR_VALID_Pos)|(0<<MPU_RBAR_REGION_Pos);
	mpu_reg->RASR = MPU_RASR_XN_ENABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_WRITE_BACK_NOT_ALLOCATE|MPU_RASR_SIZE_128K|(1<<MPU_RASR_ENABLE_Pos);


	mpu_reg->RBAR = RAM_ICM_BUF_START |(1<<MPU_RBAR_VALID_Pos)|(1<<MPU_RBAR_REGION_Pos);		//ICM_BUF
	mpu_reg->RASR = MPU_RASR_XN_DISABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_STRONGLY_ORDERED_SHARABLE|MPU_RASR_SIZE_8K|(1<<MPU_RASR_ENABLE_Pos);


	mpu_reg->RBAR = 0x25000000|(1<<MPU_RBAR_VALID_Pos)|(2<<MPU_RBAR_REGION_Pos);	//IPC and DMAC
	mpu_reg->RASR = MPU_RASR_XN_DISABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_STRONGLY_ORDERED_SHARABLE|MPU_RASR_SIZE_128K|(1<<MPU_RASR_ENABLE_Pos);

	mpu_reg->RBAR = 0x27000000|(1<<MPU_RBAR_VALID_Pos)|(3<<MPU_RBAR_REGION_Pos);	//FLASH
	mpu_reg->RASR = MPU_RASR_XN_DISABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_NON_CACHEABLE|MPU_RASR_SIZE_2M|(1<<MPU_RASR_ENABLE_Pos);


	mpu_reg->RBAR = 0xA0000000|(1<<MPU_RBAR_VALID_Pos)|(4<<MPU_RBAR_REGION_Pos);	//register
	mpu_reg->RASR = MPU_RASR_XN_DISABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_STRONGLY_ORDERED_SHARABLE|MPU_RASR_SIZE_2M|(1<<MPU_RASR_ENABLE_Pos);

	mpu_reg->RBAR = 0xE0000000|(1<<MPU_RBAR_VALID_Pos)|(5<<MPU_RBAR_REGION_Pos);	//register
	mpu_reg->RASR =	MPU_RASR_XN_DISABLE | MPU_AP_PRIV_RW_USR_RW | MPU_MEM_STRONGLY_ORDERED_SHARABLE|MPU_RASR_SIZE_512M|(1<<MPU_RASR_ENABLE_Pos);

	mpu_reg->CTRL |=MPU_CTRL_ENABLE_Msk;

#endif

#endif
	// protect the whole flash before the XIP mode been set
	enable_watchponit(0x2711F000, 12, 2, 1);
	enable_watchponit(0x27120000, 17, 2, 2);
	enable_watchponit(0x27140000, 18, 2, 3);
	enable_watchponit(0x27180000, 19, 2, 4);

}
