#include "softap_macro.h"
#if !IS_DSP_CORE
#include "softap_macro.h"
#include "diag_string_print.h"
#include "diag_format.h"
#include "diag_out.h"
#include "ItemStruct.h"
#include "TypeDefine.h"
#include "xy_utils.h"


#define DYLOG_LEN 24

unsigned int diag_static_logM3(unsigned char src_id, unsigned char lev, unsigned int m3Time, const char*fmt, va_list args)
{

	int strLen = strlen(fmt) + 1;
	unsigned int ret = 0;
	int argLen = 0;
	char *log_message_auc = NULL;
	uint32_t log_length_limit = 1 << g_softap_fac_nv->log_length_limit;

	char *log_str = (char*)xy_malloc(log_length_limit);
	StaticLog_t * static_log = (StaticLog_t *)log_str;

	argLen = diag_str2memory(fmt, \
							 strLen - 1,
							 static_log->u8Payload, \
							 ARG_MEMORY_LEN_MAX, \
							 args);
	static_log->u8SrcId = src_id;
	static_log->u8MsgLev = lev;
	static_log->u8ParamSize = argLen;

	if(strLen + argLen + sizeof(StaticLog_t) > log_length_limit - 2)
	{
		xy_free(log_str);
		return LOG_FALSE;
	}

	if(strLen > 1)
	{
		strcpy((char*)static_log->u8Payload + argLen, fmt);
	}
	static_log->u8Payload[argLen + strLen - 1] = '\0';
	static_log->u16MsgSize = strLen + argLen;
	static_log->Pad = 1; // M3 directly output to UART1

	diag_packet_header_m3(log_str, sizeof(StaticLog_t) + static_log->u16MsgSize, XY_STATIC_LOG, 0, m3Time);

	log_message_auc = (char*)xy_malloc(sizeof(StaticLog_t) + static_log->u16MsgSize + DIAG_TAIL_LEN);
	memcpy(log_message_auc, log_str, sizeof(StaticLog_t) + static_log->u16MsgSize);
	xy_free(log_str);

	ret = diag_msg_out(log_message_auc, sizeof(StaticLog_t) + static_log->u16MsgSize, 1);

	if(ret == LOG_FALSE)
	{
		xy_free(log_message_auc);
	}

	return LOG_TRUE;

}

#else
#include "softap_macro.h"
#include "diag_string_print.h"
#include "diag_format.h"
#include "diag_out.h"
#include "itemstruct.h"
#include "Typedefine.h"
#include "os_adapt.h"
#include "softap_api.h"

#define MAX_LOG_SIZE 128
#define DYLOG_LEN 24
unsigned char log_filter[XYLOG_MAX_BLOCK];

unsigned int diag_log_filter(U32 src_id, u8 lev)
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


unsigned int diag_static_log(u8 src_id, u8 lev,const char*fmt,va_list args)
{
	char log_str[MAX_LOG_SIZE + DIAG_TAIL_LEN] = {0};
	int strLen = strlen(fmt) + 1;
	unsigned int ret = 0;
	int argLen = 0;	
		
	if(diag_log_filter(src_id, lev) == LOG_FALSE)
	{
		return LOG_FALSE;
	}
	
	StaticLog_t * static_log = (StaticLog_t *)log_str;
	
    argLen = diag_str2memory(fmt, \
							 strLen - 1,
                             static_log->u8Payload, \
                             ARG_MEMORY_LEN_MAX, \
                             args);
	static_log->u8SrcId = src_id;
	static_log->u8MsgLev = lev;
	static_log->u8ParamSize = argLen;
	if(strLen + argLen + sizeof(StaticLog_t) >= MAX_LOG_SIZE)
	{
		return LOG_FALSE;
	}
		
	if(strLen > 1)
	{
		strcpy(static_log->u8Payload + argLen, fmt);
	}
	static_log->u8Payload[argLen + strLen - 1] = '\0';
	static_log->u16MsgSize = strLen + argLen;

	
	int nAllSize = sizeof(StaticLog_t) + static_log->u16MsgSize;
	char *pOut = (char*)OSXY_Debug_Alloc(nAllSize + DIAG_TAIL_LEN, 1);
	if(pOut == NULL)
	{
		return LOG_FALSE;
	}
	
	memcpy(pOut, log_str, nAllSize);
	diag_packet_header(pOut, sizeof(StaticLog_t) + static_log->u16MsgSize, XY_STATIC_LOG, 0);
	ret = diag_msg_out(pOut, nAllSize, 1);	
	
	if(ret == LOG_FALSE)
	{
		xy_free(pOut);
	}
	
	return LOG_TRUE;
}

unsigned int diag_static_logM3(u8 src_id,u8 lev, u32 m3Time, const char* log_buf)
{
	unsigned int ret = LOG_TRUE;
	short strLen = strlen(log_buf) + 1;
	if(diag_log_filter(src_id, lev) == LOG_FALSE)
	{
		return LOG_FALSE;
	}

	char *log_str = (char*)OSXY_Debug_Alloc(sizeof(StaticLog_t) + strLen + DIAG_TAIL_LEN, 1);
	if(log_str == NULL)
	{
		return 0;
	}	
	StaticLog_t * static_log = (StaticLog_t *)log_str;
	static_log->u8SrcId = src_id;
	static_log->u8MsgLev = lev;
	static_log->u8ParamSize = 0;
		
	if(strLen > 1)
	{
		xy_assert(strLen >= strlen(log_buf));
		strcpy(static_log->u8Payload, log_buf);
	}
	
	xy_assert(strLen >= strlen(log_buf));
	static_log->u8Payload[strLen - 1] = '\0';
	xy_assert(strLen >= strlen(log_buf));
	static_log->u16MsgSize = strLen;

	diag_packet_header_m3(log_str, sizeof(StaticLog_t) + static_log->u16MsgSize, XY_STATIC_LOG, 0, m3Time);
	ret = diag_msg_out(log_str, sizeof(StaticLog_t) + static_log->u16MsgSize, 1);	
	
	if(ret == LOG_FALSE)
	{
		xy_free(log_str);
	}
	
	return LOG_TRUE;

}

unsigned int diag_dynamic_log(int dyn_id, u8 src_id,u8 lev, char *fmt, va_list args)
{
	char log_str[MAX_LOG_SIZE + DIAG_TAIL_LEN] = {0};
	DynamicLog_t * dyn_log = 0;

	unsigned int ret = 0;
	int argLen = 0;
	int str_len = strlen(fmt);
	
	if(diag_log_filter(src_id, lev) == LOG_FALSE)
	{
		return LOG_TRUE;
	}
	
	dyn_log = (DynamicLog_t *)log_str;
	dyn_log->u8SrcId 	= src_id;
	dyn_log->u8MsgLev	= lev;
	dyn_log->u16DynId 	= dyn_id;
    argLen = diag_str2memory(fmt,
							str_len,
    						 dyn_log->u8Payload,
                             ARG_MEMORY_LEN_MAX,
                             args);
	
	if(argLen >= MAX_LOG_SIZE)
	{
		return LOG_FALSE;
	}

    dyn_log->u16MsgSize = argLen;
	int nAllSize = sizeof(DynamicLog_t) + dyn_log->u16MsgSize;
	char *pOut = (char*)OSXY_Debug_Alloc(nAllSize + DIAG_TAIL_LEN, 1);
	if(pOut == NULL)
	{
		return LOG_FALSE;
	}
			
	memcpy(pOut, log_str, nAllSize);
	diag_packet_header(pOut, sizeof(DynamicLog_t) + dyn_log->u16MsgSize, XY_DYNAMIC_LOG, dyn_id);
	ret = diag_msg_out(pOut, nAllSize, 1);		
	if(ret == LOG_FALSE)
	{
		xy_free(pOut);
	}
	
	return ret;
}	
#endif


