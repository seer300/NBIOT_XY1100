#pragma once

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef enum 
{
    BY_HTTP = 0,
    BY_HTTPS = 1
} xy_http_method_e;

typedef struct
{  
    unsigned int download_delta_size;
    unsigned short download_unit_index;
    unsigned short download_unit_num;
}download_info_struct;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
void http_fota_proc(char *url);
int at_http_fota(char *at_buf, char **prsp_cmd);

