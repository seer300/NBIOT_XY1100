#include "softap_macro.h"
#if IS_DSP_CORE
#include "diag_string_print_phy.h"
#include "softap_macro.h"
#include "diag_string_print.h"
#include "diag_format.h"
#include "diag_out.h"
#include "itemstruct.h"
#include "Typedefine.h"
#include "os_adapt.h"
#include "softap_api.h"
#include "PhyConfigInterface.h"
#include "SubSysCnf.h"


#define MAX_LOG_SIZE 128
#define DYLOG_LEN 24

extern u8 log_filter[XYLOG_MAX_BLOCK];

U32 diag_log_filterEx(U32 src_id, u8 lev)
{

	u8 u8Filter = (1 << lev);
	if( src_id >= XYLOG_MAX_BLOCK)
	{
		return LOG_FALSE;
	}

	if( lev >= MAX_LEV_LOG)
	{
		return LOG_FALSE;
	}

	if((log_filter[src_id] & u8Filter) == 0)
	{
		return LOG_FALSE;
	}

	return LOG_TRUE;
}

U32 diag_pyh_printk4_ram(int dyn_id,
						 u8 src_id,
						 u8 lev,
						 char *fmt,
						 U32 v1,
						 U32 v2,
						 U32 v3,
						 U32 v4)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
		return LOG_FALSE;
	}

	return diag_pyh_printk4(dyn_id, src_id, lev, fmt, v1, v2, v3, v4);
	
}

U32 diag_pyh_printk8_ram(int dyn_id,
							 u8 src_id,
						 	 u8 lev,
						  	 char *fmt, 
						     U32 v1,
						  	 U32 v2,
						  	 U32 v3,
						     U32 v4,
						     U32 v5,
						     U32 v6,
						     U32 v7,
						     U32 v8)
{
	 if(is_printDsp_log() != LOG_TRUE)
	 {
		 return LOG_FALSE;
	 }

	return diag_pyh_printk8(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8);
}

 U32 diag_pyh_printk12_ram(int dyn_id,
							 u8 src_id,
						 	 u8 lev,
							 char *fmt, 
							 U32 v1,
							 U32 v2,
							 U32 v3,
							 U32 v4,
							 U32 v5,
							 U32 v6,
							 U32 v7,
							 U32 v8,
							 U32 v9,
							 U32 v10,
							 U32 v11,
							 U32 v12)
{
	if(is_printDsp_log() != LOG_TRUE)
	 {
		 return LOG_FALSE;
	 }

	return diag_pyh_printk12(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12);
}

 U32 diag_pyh_printk16_ram(int dyn_id,
								u8 src_id,
						 	 	u8 lev,
								char *fmt,
								U32 v1,
								U32 v2,
								U32 v3,
								U32 v4,
								U32 v5,
								U32 v6,
								U32 v7,
								U32 v8,
								U32 v9,
								U32 v10,
								U32 v11,
								U32 v12,
								U32 v13,
								U32 v14,
								U32 v15,
								U32 v16)
{
	if(is_printDsp_log() != LOG_TRUE)
	{
	 	return LOG_FALSE;
	}

	return diag_pyh_printk16(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16);
}


U32 diag_pyh_printk4(int dyn_id,
						 u8 src_id,
						 u8 lev,
						 char *fmt,
						 U32 v1,
						 U32 v2,
						 U32 v3,
						 U32 v4)
{
	char *log_str = 0;
	DynamicLog_t * dyn_log = 0;
	U32 *param = 0;
	U32 ret = 0;
	if(diag_log_filterEx(src_id, lev) == LOG_FALSE)
	{
		return LOG_TRUE;
	}

	log_str = (char*)OSXY_Debug_Alloc(DYLOG_LEN + sizeof(U32) * 4 + DIAG_TAIL_LEN, 1);
	if(log_str == NULL)
	{
		return 0;
	}
	dyn_log = (DynamicLog_t *)log_str;
	param = (U32 *)dyn_log->u8Payload;
	dyn_log->u8SrcId 	= src_id;
	dyn_log->u8MsgLev	= lev;
	dyn_log->u16DynId 	= dyn_id;
	dyn_log->u16MsgSize = sizeof(U32) * 4;
	param[0] = v1;
	param[1] = v2;
	param[2] = v3;
	param[3] = v4;

	diag_packet_header(log_str, DYLOG_LEN + dyn_log->u16MsgSize, XY_DYNAMIC_LOG, dyn_id);
	ret = diag_msg_out(log_str, DYLOG_LEN + dyn_log->u16MsgSize, 1);
	if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	
	return ret;
	
}

U32 diag_pyh_printk8(int dyn_id,
							 u8 src_id,
						 	 u8 lev,
						  	 char *fmt, 
						     U32 v1,
						  	 U32 v2,
						  	 U32 v3,
						     U32 v4,
						     U32 v5,
						     U32 v6,
						     U32 v7,
						     U32 v8)
{

	char *log_str = 0;
	DynamicLog_t * dyn_log = 0;
	U32 *param = 0;
	U32 ret = 0;

	 if(diag_log_filterEx(src_id, lev) == LOG_FALSE)
	 {
	    return LOG_TRUE;
	 }

	 log_str = (char*)OSXY_Debug_Alloc(DYLOG_LEN + sizeof(U32) * 8 + DIAG_TAIL_LEN, 1);
	 if(log_str == NULL)
	 {
		 return 0;
	 }

	 
	 dyn_log = (DynamicLog_t *)log_str;
	 param = (U32 *)dyn_log->u8Payload;
	 dyn_log->u8SrcId	 = src_id;
	 dyn_log->u8MsgLev	 = lev;
	 dyn_log->u16DynId	 = dyn_id;
	 dyn_log->u16MsgSize = sizeof(U32) * 8;
	 
	 param[0] = v1;
	 param[1] = v2;
	 param[2] = v3;
	 param[3] = v4;
	 param[4] = v5;
	 param[5] = v6;
	 param[6] = v7;
	 param[7] = v8;
	 diag_packet_header(log_str, DYLOG_LEN + dyn_log->u16MsgSize, XY_DYNAMIC_LOG, dyn_id);
	 ret = diag_msg_out(log_str, DYLOG_LEN + dyn_log->u16MsgSize, 1);
	 
	 if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	
	 return ret;
}

 U32 diag_pyh_printk12(int dyn_id,
							 u8 src_id,
						 	 u8 lev,
							 char *fmt, 
							 U32 v1,
							 U32 v2,
							 U32 v3,
							 U32 v4,
							 U32 v5,
							 U32 v6,
							 U32 v7,
							 U32 v8,
							 U32 v9,
							 U32 v10,
							 U32 v11,
							 U32 v12)
{
	char *log_str = 0;
	DynamicLog_t * dyn_log = 0;
	U32 *param = 0;
	U32 ret = 0;

	 if(diag_log_filterEx(src_id, lev) == LOG_FALSE)
	 {
	    return LOG_TRUE;
	 }

	 log_str = (char*)OSXY_Debug_Alloc(DYLOG_LEN + sizeof(U32) * 12  + DIAG_TAIL_LEN, 1);
	 if(log_str == NULL)
	 {
		 return 0;
	 }
	 
	 dyn_log = (DynamicLog_t *)log_str;
	 param = (U32 *)dyn_log->u8Payload;
	 dyn_log->u8SrcId	 = src_id;
	 dyn_log->u16DynId	 = dyn_id;
	 dyn_log->u8MsgLev	 = lev;
	 dyn_log->u16MsgSize = sizeof(U32) * 12;
	 
	 param[0] = v1;
	 param[1] = v2;
	 param[2] = v3;
	 param[3] = v4;
	 param[4] = v5;
	 param[5] = v6;
	 param[6] = v7;
	 param[7] = v8;
	 param[8] = v9;
	 param[9] = v10;
	 param[10] = v11;
	 param[11] = v12;
	 diag_packet_header(log_str, DYLOG_LEN + dyn_log->u16MsgSize, XY_DYNAMIC_LOG, dyn_id);
	 ret = diag_msg_out(log_str, DYLOG_LEN + dyn_log->u16MsgSize, 1);
	 if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	
	 return ret;
}

 U32 diag_pyh_printk16(int dyn_id,
								u8 src_id,
						 	 	u8 lev,
								char *fmt,
								U32 v1,
								U32 v2,
								U32 v3,
								U32 v4,
								U32 v5,
								U32 v6,
								U32 v7,
								U32 v8,
								U32 v9,
								U32 v10,
								U32 v11,
								U32 v12,
								U32 v13,
								U32 v14,
								U32 v15,
								U32 v16)
{

	char *log_str = 0;
	DynamicLog_t * dyn_log = 0;
	U32 *param = 0;
	U32 ret = 0;

	 if(diag_log_filterEx(src_id, lev) == LOG_FALSE)
	 {
	    return LOG_TRUE;
	 }

	 log_str = (char*)OSXY_Debug_Alloc(DYLOG_LEN + sizeof(U32) * 16 + DIAG_TAIL_LEN, 1);
	 if(log_str == NULL)
	 {
		 return 0;
	 }
	 dyn_log = (DynamicLog_t *)log_str;
	 param = (U32 *)dyn_log->u8Payload;
	 dyn_log->u8SrcId	 = src_id;
	 dyn_log->u8MsgLev	 = lev;
	 dyn_log->u16DynId	 = dyn_id;
	 dyn_log->u16MsgSize = sizeof(U32) * 16;
	 
	 param[0] = v1;
	 param[1] = v2;
	 param[2] = v3;
	 param[3] = v4;
	 param[4] = v5;
	 param[5] = v6;
	 param[6] = v7;
	 param[7] = v8;
	 param[8] = v9;
	 param[9] = v10;
	 param[10] = v11;
	 param[11] = v12;
	 param[12] = v13;
	 param[13] = v14;
	 param[14] = v15;
	 param[15] = v16;
	 diag_packet_header(log_str, DYLOG_LEN + dyn_log->u16MsgSize, XY_DYNAMIC_LOG, dyn_id);
	 ret = diag_msg_out(log_str, DYLOG_LEN + dyn_log->u16MsgSize, 1);
	 if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	
	 return ret;
}								

#ifdef PHY_RAW_DATA_PRINT
U32 print_phy_primitive(char*pMsg, int nLen, u8 ucSN)
{
    U32 ret = 0;
    CommonCnf_t * msgHeader = 0;
    PHY_RAW_DATA_t * pstPhyRawData;

    if(is_printDsp_log() != LOG_TRUE)
    {
     return LOG_FALSE;
    }

    
    msgHeader = (CommonCnf_t*)OSXY_Debug_Alloc(sizeof(CommonCnf_t) +sizeof(PHY_RAW_DATA_t) + nLen + DIAG_TAIL_LEN, 1);
    if(NULL == msgHeader)
    {
        return 0;
    }

    msgHeader->u8SrcId = LPHY;
    msgHeader->u16Len  = sizeof(PHY_RAW_DATA_t) + nLen;


    pstPhyRawData = (PHY_RAW_DATA_t *)(msgHeader+1);

    pstPhyRawData->usHSN     = 0;
    pstPhyRawData->usSFN     = 0;
    pstPhyRawData->ucSubf    = 0;
    pstPhyRawData->ucSN      = ucSN;
    pstPhyRawData->usLen     = nLen;

    
    
    if(nLen > 0)
    {
        memcpy(pstPhyRawData->aucRawData, pMsg, nLen);
    }

    diag_packet_header((char*)msgHeader, sizeof(CommonCnf_t) +sizeof(PHY_RAW_DATA_t) + nLen, XY_SUBSYSCNF_LOG, PHY_RAW_DATA/*u32MsgId*/);
    ret = diag_msg_out((char*)msgHeader, sizeof(CommonCnf_t) +sizeof(PHY_RAW_DATA_t) + nLen,  1);

    if(ret == LOG_FALSE)
    {
        OSXY_Mem_Free(msgHeader);
    }

    return LOG_TRUE;
}
#endif

#endif
