#if XY_FOTA
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <lwip/netdb.h>

#include "xy_utils.h"
#include "xy_at_api.h"
#include "xy_fota.h"
#include "at_global_def.h"
#include "at_com.h"
typedef enum
{
    AT_FOTA_IDLE,
    AT_FOTA_DOWNING,
    AT_FOTA_DOWNED,
    AT_FOTA_UPDATE
} at_fota_cmd_e;


static at_fota_cmd_e fota_status = AT_FOTA_IDLE;
static int block_num = 0;
static uint32_t at_fota_recv_size = 0;

static uint8_t chk_sum(const void*data,uint32_t length)
{
	uint8_t ret = 0;
	int   val = 0;
	unsigned char *tmp_ptr = (unsigned char *)data;
	unsigned char *tail;

	xy_assert(((int)data%4) == 0);
	while(length>0)
	{
		val ^= *tmp_ptr;
		tmp_ptr++;
		length--;
	}
	ret ^= *((unsigned char *)&val);
	ret ^= *((unsigned char *)&val+1);
	ret ^= *((unsigned char *)&val+2);
	ret ^= *((unsigned char *)&val+3);

	tail = (unsigned char *)tmp_ptr;
	while((uint32_t)tail < (int)data+length)
	{
		ret ^= *tail;
		tail++;
	}
	return ret;
}


int at_fota_Flash_Erase(void)
{
    uint32_t addr, size;
    xy_get_OTA_flash_info(&(addr), &(size));

    xy_flash_erase(addr, size);
    block_num = 0;
    at_fota_recv_size = 0;
    fota_status = AT_FOTA_DOWNING;
    return 0;
}

int at_fota_downing(char *at_buf)
{
    if(fota_status != AT_FOTA_DOWNING)
        return -1;

    int len = -1;
    int sn = -1;
    unsigned char crc_sum = 0;
    char *crc_data = xy_zalloc(3);
    char *save_data = NULL;
    char *src_data = xy_zalloc(strlen(at_buf));
    
    if (at_parse_param(",%d,%d,%s,%s", at_buf, &sn,&len,src_data,crc_data) != AT_OK)
        goto ERR_PROC;

    if(sn != block_num)
        goto ERR_PROC;
        
    if(len>512 || len<=0 || strlen(src_data)!=len * 2)
        goto ERR_PROC;
    
    save_data = xy_zalloc(len);
    if (hexstr2bytes(src_data, len * 2, save_data, len) == -1)
        goto ERR_PROC;
    
    if (hexstr2bytes(crc_data, 2, (char *)(&crc_sum), 1) == -1)
        goto ERR_PROC;

    int sum = chk_sum(save_data, len);
    if(sum != crc_sum)
        goto ERR_PROC;
      
    if(ota_write_to_flash(at_fota_recv_size, save_data, len))
        goto ERR_PROC;

    block_num++;
    at_fota_recv_size += len;

    if(save_data)
        xy_free(save_data);
    if(src_data)
        xy_free(src_data);
    if(crc_data)
        xy_free(crc_data);

    return 0;

ERR_PROC:
    if(save_data)
        xy_free(save_data);
    if(src_data)
        xy_free(src_data);
    if(crc_data)
        xy_free(crc_data);

    return -1;


}

char *at_fota_check(void)
{
    char *at_str = xy_zalloc(50);
    if((fota_status != AT_FOTA_DOWNING) || (block_num == 0))
    {
        snprintf(at_str, 50, "\r\n+NFWUPD:PACKAGE_NOT_DOWNLOADED\r\n\r\nOK\r\n");
        return at_str;
    }
        
    if(ota_delta_check())
    {
        snprintf(at_str, 50, "\r\n+NFWUPD:PACKAGE_VALIDATION_FAILURE\r\n\r\nOK\r\n");
        return at_str;
    }
        
    fota_status = AT_FOTA_DOWNED;
    snprintf(at_str, 50, "\r\n+NFWUPD:PACKAGE_OK \r\n\r\nOK\r\n");
    return at_str;
}

int at_fota_update(void)
{
    char *at_str = NULL;
    
    if(fota_status != AT_FOTA_DOWNED)
        return -1;
    
    fota_status = AT_FOTA_UPDATE;

        
    at_str = xy_zalloc(64);
    snprintf(at_str, 64, "\r\n+FIRMWARE UPDATING\r\n");
    send_rsp_str_to_ext(at_str);
    osDelay(50);
	xy_free(at_str);
    
    ota_update_start();
    return 0;
}

/*****************************************************************************
 Function    : at_NFWUPD_req
 Description :   
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+NFWUPD=<cmd>[,<sn>,<len>,<data>,<crc>]
               AT+NFWUPD=?
 *****************************************************************************/
int at_NFWUPD_req(char *at_buf, char **prsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int cmd = -1;

        if (at_parse_param("%d", at_buf, &cmd) != AT_OK)
        {
            *prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
            return AT_END;
        }

        switch(cmd)
        {
        case 0:
            if(at_fota_Flash_Erase())
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            break;
        case 1:
            if(at_fota_downing(at_buf))
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            break;
        case 2:
                *prsp_cmd = at_fota_check();
            break;
        case 3:
        {
            if(fota_status != AT_FOTA_DOWNED)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
                break;
            }
            *prsp_cmd = xy_zalloc(40);
            snprintf(*prsp_cmd, 40, "\r\n+NFWUPD:xyDelta\r\n\r\nOK\r\n");
            break;
        }
        case 4:
        {
            if(fota_status != AT_FOTA_DOWNED)
            {
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
                break;
            }
            *prsp_cmd = xy_zalloc(40);
            snprintf(*prsp_cmd, 40, "\r\n+NFWUPD:V1.0\r\n\r\nOK\r\n");
             break;
        }
        case 5:
            if(at_fota_update())
                *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            break;
        default:
            *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
        }

    }
    else if(g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_zalloc(40);
        snprintf(*prsp_cmd, 40, "\r\n+NFWUPD:(0-5)\r\n\r\nOK\r\n");
    }
    else
        *prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);

    return AT_END;
}


//20230413 MG FOTA AT command
//输入完整url，解析得到host/port/path，三项均不能缺少，否则解析失败
//如AT+QFOTADL="http://iotsvr.meigsmart.com:9090/download/xyDelta"得到
//host:iotsvr.meigsmart.com port:9090 path:/download/xyDelta
//处理从AT口输入的命令，需要特别注意结尾处的'\r'字符
#include "fota_by_http.h"

char *g_http_host = NULL;
char *g_http_path = NULL;
short unsigned int g_http_port = 0;

int at_QFOTADL_req(char *at_buf, char **prsp_cmd)
{
    if(AT_CMD_TEST == g_req_type)
    {
		*prsp_cmd = xy_zalloc(48);
		sprintf(*prsp_cmd, "\r\n+QFOTADL=<http url>\r\n\r\nOK\r\n");
    }

	else if(AT_CMD_REQ == g_req_type)
	{
	    char *http_addr = NULL;
		http_addr = xy_zalloc(256);
		
		if (at_parse_param("%256s", at_buf, http_addr) != AT_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}

		if(httpurl_parse(at_buf, &g_http_host, &g_http_port, &g_http_path) != XY_OK)
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		
		fota_by_http();
		
		xy_free(http_addr);
	}

	else
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return AT_END;	
}
//add end
#endif

