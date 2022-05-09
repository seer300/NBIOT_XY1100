#include "diag_mem_dump.h"
#include "diag_out.h"
#include "sysappcnf.h"
#include "ItemStruct.h"
#include "softap_macro.h"
#include <string.h>
#include "diag_format.h"
#include "diag_read_msg.h"
#if IS_DSP_CORE
#include "DataTypes.h"
#include "softap_macro.h"
#include "os_adapt.h"
#include "softap_api.h"
#else
#include "flash_adapt.h"
#endif

unsigned char mem_is_ready = 0;
void send_mem_ready()
{
	ResetMem();
	diag_send_header(XY_SYSAPPCNF_LOG, XY_SYSAPP_MEMHAVE_IND, 0);
}

void set_pc_info(unsigned char ready)
{
	
	diag_send_header(XY_SYSAPPCNF_LOG, XY_SYSAPP_MEMREADY_CNF, 0);
	mem_is_ready = ready;
}

//return 1 pc is ready,otherewise 0
unsigned char  query_pc_info()
{
	return mem_is_ready;
}

MemInfo_t *mem_info;
char szOutPut[600];
char szFileName[128] = {0};
void mem_dump_info(char *mem_file, unsigned int mem_addr, unsigned int mem_size)
{
	mem_is_ready = 0;
	mem_info = (MemInfo_t *)(szOutPut + sizeof(ItemHeader_t));
#define ONE_PACKET_SIZE 512
	char *send_data = (char*)mem_addr;
	int nSize = mem_size / ONE_PACKET_SIZE;
	int last_size = mem_size % ONE_PACKET_SIZE;
	int i = 0;
	int offset = 0;
	int name_len = strlen(mem_file);
	int copy_len = 0;
	for(i = 0; i < nSize; i++)
	{
		mem_info->u8Len = ONE_PACKET_SIZE;
		mem_info->u8Seq = i;
		mem_info->u8PEnd = 0;
		if(i == nSize - 1)
		{
			mem_info->u8PEnd = 1;
		}

		//first packet add file name
		if(i == 0)
		{
			//copy file name
			memcpy(szFileName, mem_file, name_len);
			szFileName[name_len] = '\0';
			copy_len= name_len + 1;
			mem_info->u8Len += copy_len;
		}
		else
		{
			copy_len = 0;
		}

		//send header
		diag_packet_header_m3(szOutPut,sizeof(ItemHeader_t) + sizeof(MemInfo_t) + ONE_PACKET_SIZE + copy_len, XY_MEMORY_LOG, 0, 0);
		
		if(i == 0)
		{
			//send file name
			memcpy(mem_info->u8Payload,szFileName , copy_len);
		}

		//send memory information
		memcpy(mem_info->u8Payload + copy_len, send_data + offset , ONE_PACKET_SIZE);	
		diag_send_data(szOutPut, sizeof(MemInfo_t) + ONE_PACKET_SIZE + copy_len + sizeof(ItemHeader_t));
		offset += ONE_PACKET_SIZE;
	}

	copy_len = 0;

	//the last item
	if(last_size != 0)
	{
		mem_info->u8Len = last_size;
		mem_info->u8Seq = i;
		mem_info->u8PEnd = 1;

		// if the dump size less ONE_PACKET_SIZE. add file name
		if(nSize == 0)
		{
			memcpy(szFileName, mem_file, name_len);
			szFileName[name_len] = '\0';
			mem_info->u8Len += name_len + 1;
			copy_len = name_len + 1;
		}

		int nDataLen = sizeof(ItemHeader_t) + sizeof(MemInfo_t) + last_size + name_len + 1;
		diag_packet_header_m3(szOutPut,nDataLen, XY_MEMORY_LOG, 0, 0);
		if(nSize == 0)
		{
			memcpy(mem_info->u8Payload,szFileName , copy_len);
	
		}

		memcpy(mem_info->u8Payload + copy_len,send_data + offset , last_size);
		diag_send_data(szOutPut, sizeof(MemInfo_t) + last_size + copy_len + sizeof(ItemHeader_t));
	}	
	
}

#if !IS_DSP_CORE
void mem_dump_info_flash(char *mem_file, unsigned int mem_addr, unsigned int mem_size)
{
	mem_info = (MemInfo_t *)(szOutPut + sizeof(ItemHeader_t));
#define ONE_PACKET_SIZE 512
	char *send_data = (char*)mem_addr;
	int nSize = mem_size / ONE_PACKET_SIZE;
	int last_size = mem_size % ONE_PACKET_SIZE;
	int i = 0;
	int offset = 0;
	int name_len = strlen(mem_file);
	int copy_len = 0;
	mem_is_ready = 0;
	for(i = 0; i < nSize; i++)
	{
		mem_info->u8Len = ONE_PACKET_SIZE;
		mem_info->u8Seq = i;
		mem_info->u8PEnd = 0;
		if(i == nSize - 1)
		{
			mem_info->u8PEnd = 1;
		}

		//first packet add file name
		if(i == 0)
		{
			//copy file name
			memcpy(szFileName, mem_file, name_len);
			szFileName[name_len] = '\0';
			copy_len= name_len + 1;
			mem_info->u8Len += copy_len;
		}
		else
		{
			copy_len = 0;
		}

		//send header
		diag_packet_header_m3(szOutPut,sizeof(ItemHeader_t) + sizeof(MemInfo_t) + ONE_PACKET_SIZE + copy_len, XY_MEMORY_LOG, 0, 0);
		
		if(i == 0)
		{
			//send file name
			memcpy(mem_info->u8Payload,szFileName , copy_len);
		}

		//send memory information
		flash_read((unsigned int)(send_data + offset) , mem_info->u8Payload + copy_len, ONE_PACKET_SIZE);

		int i;
		char checksum = 0x00;

		for(i=0; i<(int)(sizeof(MemInfo_t) + ONE_PACKET_SIZE + copy_len + sizeof(ItemHeader_t)); i++)
		{
			checksum ^= szOutPut[i];
		}

		szOutPut[i]   = checksum;
		szOutPut[i+1] = 0xFA;

		diag_send_data(szOutPut, sizeof(MemInfo_t) + ONE_PACKET_SIZE + copy_len + sizeof(ItemHeader_t));
		offset += ONE_PACKET_SIZE;
	}

	copy_len = 0;

	//the last item
	if(last_size != 0)
	{
		mem_info->u8Len = last_size;
		mem_info->u8Seq = i;
		mem_info->u8PEnd = 1;

		// if the dump size less ONE_PACKET_SIZE. add file name
		if(nSize == 0)
		{
			memcpy(szFileName, mem_file, name_len);
			szFileName[name_len] = '\0';
			mem_info->u8Len += name_len + 1;
			copy_len = name_len + 1;
		}

		int nDataLen = sizeof(ItemHeader_t) + sizeof(MemInfo_t) + last_size + name_len + 1;
		diag_packet_header_m3(szOutPut,nDataLen, XY_MEMORY_LOG, 0, 0);
		if(nSize == 0)
		{
			memcpy(mem_info->u8Payload,szFileName , copy_len);
	
		}

		memcpy(mem_info->u8Payload + copy_len,send_data + offset , last_size);
		diag_send_data(szOutPut, sizeof(MemInfo_t) + last_size + copy_len + sizeof(ItemHeader_t));
	}
	
}

#endif

