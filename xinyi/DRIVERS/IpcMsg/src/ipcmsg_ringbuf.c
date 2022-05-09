#include "ipcmsg.h"
#include "sema.h"
#include "xy_utils.h"

/*void memcpy_patch(void* dest,void* src,unsigned long len)
{
			volatile unsigned long tmp_len = (unsigned long)len;
			volatile unsigned long tmp_dest = (unsigned long)dest;
			volatile unsigned long tmp_src = (unsigned long)src;
	
			while(tmp_len)
			{
					memcpy_patch_b(tmp_dest,tmp_src);
					tmp_len--;
					tmp_dest++;
					tmp_src++;
			}		


}*/


int Is_Ringbuf_Empty(T_RingBuf *ringbuf)
{
	unsigned int Write_Pos = ringbuf->Write_Pos;
	unsigned int Read_Pos = ringbuf->Read_Pos;
	
	if (Write_Pos == Read_Pos)
		return true;
	else
		return false;

}


int Is_Ringbuf_ChFreeSpace(T_RingBuf *ringbuf_send, unsigned int size)
{
	unsigned int Write_Pos = ringbuf_send->Write_Pos;
	unsigned int Read_Pos = ringbuf_send->Read_Pos;
	unsigned int ringbuf_size = ringbuf_send->size;
	/* |+++wp-----rp+++| */
	if (Write_Pos < Read_Pos)
	{

    	if ((Read_Pos - Write_Pos) > size){			
			return true;
		}
		else{
			return false;
		}
	}
	else{

		/* |---rp+++++wp---| */
		if ((ringbuf_size - Write_Pos + Read_Pos ) > size){
			return true;
		}
		else{
			return false;
		}
	}
}

void ipcmsg_ringbuf_read(T_RingBuf* RingBuf_rcv, void *__dest, unsigned int __data_len,unsigned int __buf_len)
{
	unsigned int read_pos = RingBuf_rcv->Read_Pos;
	unsigned int write_pos = RingBuf_rcv->Write_Pos;
	unsigned int Base_Addr = RingBuf_rcv->base_addr;
	unsigned int tmp;
	/* |+++wp-----rp+++| */
	if((write_pos < read_pos)&&(__buf_len > RingBuf_rcv->size - read_pos))
	{
		if(__data_len > RingBuf_rcv->size - read_pos)
		{
			memcpy(__dest, (void *)(Base_Addr + read_pos), RingBuf_rcv->size - read_pos);
			memcpy((void *)((unsigned int)__dest + RingBuf_rcv->size - read_pos), (void *)Base_Addr, __data_len - RingBuf_rcv->size + read_pos);
		}
		else
			memcpy(__dest, (void*)(Base_Addr + read_pos), __data_len);
		//RingBuf_rcv->Read_Pos = ALIGN_IPCMSG(__n - RingBuf_rcv->size + read_pos ,IPCMSG_ALIGN);
		tmp = ALIGN_IPCMSG(__buf_len - RingBuf_rcv->size + read_pos ,IPCMSG_ALIGN);
		memcpy(&RingBuf_rcv->Read_Pos,&tmp,4);
	}
	/* |---rp+++++wp---| */
	else
	{
		memcpy(__dest, (void*)(Base_Addr + read_pos), __data_len);
		//RingBuf_rcv->Read_Pos += ALIGN_IPCMSG(__n,IPCMSG_ALIGN);//(__n+__n%2);
		tmp = RingBuf_rcv->Read_Pos + ALIGN_IPCMSG(__buf_len,IPCMSG_ALIGN);
		memcpy(&RingBuf_rcv->Read_Pos,&tmp,4);
	}

	if(RingBuf_rcv->Read_Pos >= RingBuf_rcv->size)
	{
		//RingBuf_rcv->Read_Pos = (RingBuf_rcv->Read_Pos % RingBuf_rcv->size);
		tmp = (RingBuf_rcv->Read_Pos % RingBuf_rcv->size);
		memcpy(&RingBuf_rcv->Read_Pos,&tmp,4);
	}

}



void ipcmsg_ringbuf_write(T_RingBuf* RingBuf_send, void *__src, unsigned int __n)
{
	unsigned int read_pos = RingBuf_send->Read_Pos;
	unsigned int write_pos = RingBuf_send->Write_Pos;
	unsigned int Base_Addr = RingBuf_send->base_addr;
	unsigned int tmp;
	/* |---rp+++++wp---| */
	if((write_pos >= read_pos) && (__n > RingBuf_send->size - write_pos) )
	{
		memcpy((void*)(Base_Addr + write_pos), __src, RingBuf_send->size - write_pos);
		memcpy((void*)(Base_Addr), (void*)((unsigned int)__src + RingBuf_send->size - write_pos), __n - RingBuf_send->size + write_pos);
		//RingBuf_send->Write_Pos = ALIGN_IPCMSG(__n- RingBuf_send->size + write_pos,IPCMSG_ALIGN);
		tmp = ALIGN_IPCMSG(__n- RingBuf_send->size + write_pos,IPCMSG_ALIGN);
		memcpy(&RingBuf_send->Write_Pos,&tmp,4);
	}
	else/* |+++wp-----rp+++| */
	{
		memcpy((void*)(Base_Addr + write_pos), __src, __n);
		//RingBuf_send->Write_Pos += ALIGN_IPCMSG(__n,IPCMSG_ALIGN);//(__n+__n%2);
		tmp = RingBuf_send->Write_Pos + ALIGN_IPCMSG(__n,IPCMSG_ALIGN);
		memcpy(&RingBuf_send->Write_Pos,&tmp,4);
	}

	if(RingBuf_send->Write_Pos >= RingBuf_send->size)
	{
		//RingBuf_send->Write_Pos = (RingBuf_send->Write_Pos % RingBuf_send->size);
		tmp = (RingBuf_send->Write_Pos % RingBuf_send->size);
		memcpy(&RingBuf_send->Write_Pos,&tmp,4);
	}
}








