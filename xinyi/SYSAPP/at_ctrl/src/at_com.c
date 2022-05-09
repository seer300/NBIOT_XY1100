/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_ctl.h"
#include "at_global_def.h"
#include "at_utils.h"

/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/
osTimerId_t g_at_standby_timer = NULL;
int g_at_off_standby = 0;  //at_standby_set,++ or --,default into standby

/*******************************************************************************
 *                       Local function implementations	                   *
 ******************************************************************************/
//return 0:not at cmd,1:is at cmd
int check_atcmd(char *atcmd)
{
	if (atcmd == NULL)
		return 0;
	//char *str = find_first_print_char(atcmd);
	if (*atcmd != 'A' && *atcmd != 'a')
		return 0;
	atcmd++;
	if (*atcmd != 'T' && *atcmd != 't')
		return 0;
	atcmd++;
	if (!isalpha(*atcmd) && *atcmd != '\r' && *atcmd != '+' && *atcmd != '^' && *atcmd != '#' && *atcmd != '*')
		return 0;

	return 1;
}

static int parase_type_val(char *str, char *fmt, void **pval, char *parase_idx)
{
    int ret = AT_OK;
    int size = 0;
    char type;
    char len_str[6] = {0};

    while (*fmt == ' ')
        fmt++;

    while (*str == ' ')
        str++;
    if (*str == '"') //skip the front yinhao
        str++;

    //no valid param value
    if (strlen(str) == 0 && strlen(fmt) == 0)
        return ret;
    else if (strlen(str) == 0)
    {
        (*parase_idx)++;
        return ret;
    }
    else if (strlen(fmt) == 0)
        return ret;
    type = *(fmt + strlen(fmt) - 1);
    //have param length
    if (strlen(fmt) > 2)
    {
        strncpy(len_str, fmt + 1, strlen(fmt) - 2);
        size = (int)strtol(len_str,NULL,10);
    }
    if (type == 'd' || type == 'D')
    {
        if (size == 0 || size == 4)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, pval[(int)(*parase_idx)]) != XY_OK )
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
                }
                else
                {
                    *((long *)(pval[(int)(*parase_idx)])) = (long)strtol(str,NULL,10);
                }
            }
        }
        else if (size == 1)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)(pval[(int)(*parase_idx)])) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
                }
                else
                {
                    *((char *)(pval[(int)(*parase_idx)])) = (char)strtol(str,NULL,10);
                }
            }
        }
        else if (size == 2)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)(pval[(int)(*parase_idx)])) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
                }
                else
                {
                    *((short *)(pval[(int)(*parase_idx)])) = (short)strtol(str,NULL,10);
                }
            }
        }
        else
        {
            xy_assert(0);
        }
    }
    else if (type == 's' || type == 'S')
    {
        if (size == 0)
        {
            if (str != NULL)
            {
                strcpy((char *)(pval[(int)(*parase_idx)]), str);
            }
        }
        else
        {
            if (size < (int)(strlen(str)))
            {
                ret = ATERR_PARAM_INVALID;
            }
            else if (str != NULL)
            {
                int cpy_len = ((int)(strlen(str))) < size ? ((int)(strlen(str))) : (size - 1);
                strncpy((char *)(pval[(int)(*parase_idx)]), str, cpy_len);
            }
        }
    }
    else
    {
        xy_assert(0);
        return ret;
    }
    (*parase_idx)++;
    return ret;
}

static void get_val_limit(char *fmt, char *required_flag, char *type, int *size, unsigned int *min_limit, unsigned int *max_limit)
{
	char len_str[6] = {0};
	char *left_bracket = NULL;
	char *right_bracket = NULL;
	char *left_square_bracket = NULL;
	char *right_square_bracket = NULL;
	char *mid_line = NULL;

	if(strlen(fmt) > 2)
	{
		left_bracket = strchr(fmt, '(');
		right_bracket = strchr(fmt, ')');
		left_square_bracket = strchr(fmt, '[');
		right_square_bracket = strchr(fmt, ']');
		if((left_bracket == NULL && right_bracket == NULL) && (left_square_bracket == NULL && right_square_bracket== NULL))
		{
			*type = *(fmt + strlen(fmt) - 1);
			strncpy(len_str, fmt + 1, strlen(fmt) - 2);
			*size = (int)strtol(len_str,NULL,10);
		}
		else if(((left_bracket != NULL && right_bracket != NULL) && (left_square_bracket == NULL && right_square_bracket== NULL)) || \
			((left_bracket == NULL && right_bracket == NULL) && (left_square_bracket != NULL && right_square_bracket != NULL)))
		{
			if(left_bracket !=NULL)
			{
				*required_flag = 1;
			}
			else
			{
				left_bracket = left_square_bracket;
				right_bracket = right_square_bracket;
			}
			*left_bracket = '\0';
			*type = *(fmt + strlen(fmt) - 1);
			mid_line = strchr(left_bracket + 1, '-');
			if((*type != 'd' && *type != 'D') || (left_bracket > right_bracket))
			{
				xy_assert(0);
			}

			if(mid_line != NULL)
			{
				xy_assert(mid_line < right_bracket);
				*right_bracket = '\0';
				*min_limit = (int)strtol(left_bracket + 1,NULL,10);		//min_limit至少为0
				if(right_bracket - mid_line > 1)
					*max_limit = (int)strtol(mid_line + 1,NULL,10);		//不输上限时，max_limit为0x0fffffff,-)间必须输有数字，否则max_limit解析为0
			}
			else
				xy_assert((left_bracket + 1) == right_bracket);			//%d()表示必选参数没有范围限制，括号之间不能有任何字符

			if(*min_limit > *max_limit)
			{
				xy_assert(0);
			}
			if(strlen(fmt) > 2)
			{
				strncpy(len_str, fmt + 1, strlen(fmt) - 2);
				*size = (int)strtol(len_str,NULL,10);
			}
		}
		else
		{
			xy_assert(0);
		}
	}
	else
	{
		*type = *(fmt + strlen(fmt) - 1);
	}
}

static int parase_type_val_vp(char *str, char *fmt, va_list *ap)
{
    int ret = AT_OK;
    int size = 0;
    char type = 0;
	char required_flag = 0;
	unsigned int min_limit = 0;
	unsigned int max_limit = 0xffffffff;
	unsigned int val_trans = 0;

    while (*fmt == ' ')
        fmt++;

    while (*str == ' ')
    {
        str++;
    }

	//no valid param value
	if(strlen(fmt) == 0)
	{
		return ret;
	}
	else
	{
		get_val_limit(fmt, &required_flag, &type, &size, &min_limit, &max_limit);
		if (strlen(str) == 0)
		{
			va_arg(*ap, int);
			if(required_flag == 1)
				ret = ATERR_PARAM_INVALID;
			return ret;
		}
	}

    if (type == 'd' || type == 'D')
    {
        if (size == 0 || size == 4)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)va_arg(*ap, int)) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
					hexstr2int(str, (int *)&val_trans);
                }
                else
                {
                    *((long *)(va_arg(*ap, int))) = (long)strtol(str,NULL,10);
					val_trans = (unsigned int)strtol(str,NULL,10);
                }
            }
        }
        else if (size == 1)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)va_arg(*ap, int)) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
					hexstr2int(str, (int *)(&val_trans));
                }
                else
                {
                    *((char *)(va_arg(*ap, int))) = (char)strtol(str,NULL,10);
					val_trans = (unsigned int)strtol(str,NULL,10);
                }
            }
        }
        else if (size == 2)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)va_arg(*ap, int)) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
					hexstr2int(str, (int *)(&val_trans));
                }
                else
                {
                    *((short *)(va_arg(*ap, int))) = (short)strtol(str,NULL,10);
					val_trans = (unsigned int)strtol(str,NULL,10);
                }
            }
        }
        else
        {
            xy_assert(0);
        }
		if(val_trans < min_limit || val_trans > max_limit)
			ret = ATERR_PARAM_INVALID;
    }
    else if (type == 's' || type == 'S')
    {
        if (size == 0)
        {
            if (str != NULL)
            {
                strcpy((char *)(va_arg(*ap, int)), str);
            }
        }
        else
        {
            if (size < (int)(strlen(str)))
            {
                ret = ATERR_PARAM_INVALID;
            }
            else if (str != NULL)
            {
                int cpy_len = ((int)(strlen(str))) < size ? ((int)(strlen(str))) : (size - 1);
                strncpy((char *)(va_arg(*ap, int)), str, cpy_len);
            }
        }
    }
    else
    {
        xy_assert(0);
        return ret;
    }
    //(*parase_idx)++;
    return ret;
}

static int parase_type_val_vp_esc(char *str, char *fmt, int* parse_num, va_list *ap)
{
    int ret = AT_OK;
    int size = 0;
    char type = 0;
	char required_flag = 0;
	unsigned int min_limit = 0;
	unsigned int max_limit = 0xffffffff;
	unsigned int val_trans = 0;

    while (*fmt == ' ')
        fmt++;

    while (*str == ' ')
    {
        str++;
    }

	//no valid param value
	if(strlen(fmt) == 0)
	{
		return ret;
	}
	else
	{
		get_val_limit(fmt, &required_flag, &type, &size, &min_limit, &max_limit);
		if (strlen(str) == 0)
		{
			va_arg(*ap, int);
			if(required_flag == 1)
				ret = ATERR_PARAM_INVALID;
			return ret;
		}
	}

    if (type == 'd' || type == 'D')
    {
        if (size == 0 || size == 4)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)va_arg(*ap, int)) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
					hexstr2int(str, (int *)&val_trans);
                }
                else
                {
                    *((long *)(va_arg(*ap, int))) = (long)strtol(str,NULL,10);
					*parse_num += 1;
					val_trans = (unsigned int)strtol(str,NULL,10);
				}
            }
        }
        else if (size == 1)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)va_arg(*ap, int)) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
					hexstr2int(str, (int *)(&val_trans));
                }
                else
                {
                    *((char *)(va_arg(*ap, int))) = (char)strtol(str,NULL,10);
					*parse_num += 1;
					val_trans = (unsigned int)strtol(str,NULL,10);
                }
            }
        }
        else if (size == 2)
        {
            if (str != NULL)
            {
                if (!is_digit_str(str))
                {
                    if (hexstr2int(str, (int *)va_arg(*ap, int)) != XY_OK)
                    {
                        ret = ATERR_PARAM_INVALID;
                    }
					hexstr2int(str, (int *)(&val_trans));
                }
                else
                {
                    *((short *)(va_arg(*ap, int))) = (short)strtol(str,NULL,10);
					*parse_num += 1;
					val_trans = (unsigned int)strtol(str,NULL,10);
                }
            }
        }
        else
        {
            xy_assert(0);
        }
		if(val_trans < min_limit || val_trans > max_limit)
			ret = ATERR_PARAM_INVALID;
    }
    else if (type == 's' || type == 'S')
    {
        if (size == 0)
        {
            if (str != NULL)
            {
				format_escape_char(str);
                strcpy((char *)(va_arg(*ap, int)), str);
				*parse_num += 1;
            }
        }
        else
        {
            if (size < (int)(strlen(str)))
            {
                ret = ATERR_PARAM_INVALID;
            }
            else if (str != NULL)
            {
				format_escape_char(str);
                int cpy_len = ((int)(strlen(str))) < size ? ((int)(strlen(str))) : (size - 1);
                strncpy((char *)(va_arg(*ap, int)), str, cpy_len);
				*parse_num += 1;
            }
        }
    }
    else
    {
        xy_assert(0);
        return ret;
    }
    //(*parase_idx)++;
    return ret;
}

//make sure get correct prefix,for example :AT+CGSN=1;+CIMI;+CGSN?, prefix should be AT+CGSN
static char* at_get_prefix_4_comb_cmd(char* start, char* end, char* type)
{
	xy_assert(start != NULL && end != NULL && start < end);
	char *tmp = start;
	while(tmp < end)
	{
		if (*tmp == '=')
		{
			if (*(tmp + 1) == '?')
			{
				*type = AT_CMD_TEST;
			}
			else
			{
				*type = AT_CMD_REQ;
			}
			return tmp;
		}
		else if (*tmp == '?')
		{
			*type = AT_CMD_REQ;
			return tmp;
		}
		tmp++;
	};

	if (*tmp == ';')
	{
		//example: AT+CIMI;+CGEQOS=0,9;+CGSN=1;+CGACT?
		*type = AT_CMD_ACTIVE;
		return tmp;
	}
	
	return NULL;
}

char *check_ati_req_custom(char *at_cmd, int* flag)
{
	char *param = NULL;
	*flag = 0; 
	if (*at_cmd == 'A' && *(at_cmd + 1) == 'T' && *(at_cmd + 2) == 'I' && *(at_cmd + 3) != '\r')
	{
		if (*(at_cmd + 3) == '=' && *(at_cmd + 4) == '?' && *(at_cmd + 5) == '\r')
		{
			g_atctl_req_type = AT_CMD_TEST;
			*flag = 1;
		}
		else if (*(at_cmd + 3) == '?' && *(at_cmd + 4) == '\r')
		{
			g_atctl_req_type = AT_CMD_QUERY;
			*flag = 1;
		}
		else if (*(at_cmd + 3) == '=')
		{
			g_atctl_req_type = AT_CMD_REQ;
			param = at_cmd + 4;
			*flag = 1;
		}
	}
	return param;
}

/*******************************************************************************
 *                       Global function implementations	                   *
 ******************************************************************************/
//if is_all==1,at_prefix="AT+WORKLOCK"/"AT+XX"/"AT+NRB",else if is_all==0,prefix is only letter,e.g "+WORKLOCK"/"+NRB"
char *at_get_prefix_and_param(char *at_cmd, char *at_prefix, char is_all)
{
	char *head;
	char *end;
	char *param = NULL;
	char atcmd_type = AT_CMD_REQ;
	int at_prefix_len = 0; 
	memset(at_prefix, 0, AT_CMD_PREFIX);
	while (*at_cmd == '\r' || *at_cmd == '\n')
		at_cmd++;
	if (at_is_rsp_err(at_cmd, NULL) || !at_strcasecmp(at_cmd, "OK\r\n")) //if the result is "ERROR" or "\r\nOK\r\n",do not look for prefixe.
		return NULL;

	//deal with custom ati command:eg；ATI=1,"adbc", ATI?, ATI=?
	int is_ati_custom = 0;
	char *ati_param = check_ati_req_custom(at_cmd, &is_ati_custom);
	if (is_ati_custom)
	{
		strncpy(at_prefix, "ATI", strlen("ATI"));
		return ati_param;
	}

	//include param
	if (*at_cmd == 'A' && *(at_cmd + 1) == 'T')
	{
		char *temp = at_cmd + 2;
		if(*temp == '+' || *temp == '^' || *temp == '&' || *temp == '+' || *temp == '#' || *temp == '*')
		{
			head = temp + 1;
			if ((end = strchr(head, '=')) != NULL && *(end+1) == '?' && *(end+2) == '\r')
			{				
				atcmd_type = AT_CMD_TEST;				
				param = end + 1;
			}
			else if((end = strchr(head, '?')) != NULL && *(end+1) == '\r')
			{
				char *tmp = NULL;
				if ((tmp = strchr(head, ';')) != NULL && tmp < end)
				{
					//example:AT+CGEQOS=0,9;+CGSN=1;+CGACT?
					end = at_get_prefix_4_comb_cmd(at_cmd, tmp, &atcmd_type);
					param = end + 1;
				}
				else
				{
					atcmd_type = AT_CMD_QUERY;
					param = end + 1;
				}
				
			}
			else if((end = strchr(head, '=')) != NULL)
			{
				char *tmp = NULL;
				if ((tmp = strchr(head, ';')) != NULL && tmp < end)
				{
					//example:AT+CIMI;+CGCMOD=0;
					end = at_get_prefix_4_comb_cmd(at_cmd, tmp, &atcmd_type);
					param = end + 1;
				}
				else
				{
					atcmd_type = AT_CMD_REQ;
					param = end + 1;
				}
			}
			else if ((end = strchr(head, '\r')) != NULL)
			{
				char *tmp = NULL;
				if ((tmp = strchr(head, ';')) != NULL && tmp < end)
				{
					//example:AT+CGSN;+CIMI
					end = at_get_prefix_4_comb_cmd(at_cmd, tmp, &atcmd_type);
					param = end + 1;
				}
				else
				{
					atcmd_type = AT_CMD_ACTIVE;
					param = end;
				}
			}
			else
				xy_assert(0);

			if (is_all == 1)
			{
				g_atctl_req_type = atcmd_type;
				at_prefix_len = end - head + 3;
				if(at_prefix_len < AT_CMD_PREFIX)
				{
					strncpy(at_prefix, head - 3, at_prefix_len);
				}
				else
				{
					param = NULL;
					softap_printf(USER_LOG, WARN_LOG, "AT_CMD error!!!");
				}
			}
			else
			{
				at_prefix_len = end - head + 1;
				if(at_prefix_len < AT_CMD_PREFIX)
				{
					strncpy(at_prefix, head - 1, at_prefix_len);
				}
				else
				{
					param = NULL;
					softap_printf(USER_LOG, WARN_LOG, "AT_CMD error!!!");
				}	
			}
		}
		//ATD
		else if (isalpha(*temp))
		{
			g_atctl_req_type = AT_CMD_ACTIVE;
			at_prefix_len = 3;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, at_cmd, at_prefix_len);
				param = at_cmd + 3;
			}
			else
			{
				softap_printf(USER_LOG, WARN_LOG, "AT_CMD error!!!");
			}			
		}
		else if (*temp == '\r')
		{
			/*eg: AT/r */
			g_atctl_req_type = AT_CMD_ACTIVE;
			at_prefix_len = 2;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, at_cmd, at_prefix_len);
				param = at_cmd + 3;
			}
			else
			{
				softap_printf(USER_LOG, WARN_LOG, "AT_CMD error!!!");
			}
		}
		else
			param = NULL;
	}
	//主动上报
	else if (*at_cmd == '+' || *at_cmd == '^' || *at_cmd == '&')
	{
		head = at_cmd;
		if ((end = strchr(head, ':')) != NULL)
		{
			at_prefix_len = end - head + 1;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, head, at_prefix_len);
				param = end + 1;
			}
			else
			{
				softap_printf(USER_LOG, WARN_LOG, "AT_CMD error!!!");
			}
		}
		else if ((end = strchr(head, '\r')) != NULL)
		{
			at_prefix_len = end - head;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, head, at_prefix_len);
				param = end;
			}
			else
			{
				softap_printf(USER_LOG, WARN_LOG, "AT_CMD error!!!");
			}
		}
		else
		{
			xy_assert(0);
			param = NULL;
		}
	}
	else
	{
		param = NULL;
	}
	//xy_assert(strlen(at_prefix) < AT_CMD_PREFIX);
	return param;
}

//find print string,but not include blank space, \r, \n...
char *find_first_print_char(char *at_str, int len)
{
	char *str = at_str;
	while ((*str < '!' || *str > '}') && str < at_str + len)
	{
		str++;
	}
	return (str < at_str + len ? str : NULL);
}
#if 0
//auto info is not  include  +XXX:,add it here
char *shape_special_mid_rsp(char *prefix, char *at_cmd)
{
	char *shaped = NULL;
	char *chr_chr = NULL;
	while (*at_cmd == '\r' || *at_cmd == '\n')
		at_cmd++;
	if (at_cmd == NULL || *at_cmd == '\0')
	{
		return shaped;
	}
	if (!strcmp(at_cmd, "OK\r\n") || at_is_rsp_err(at_cmd, NULL) || (chr_chr=strchr(at_cmd,':') && chr_chr-at_cmd<AT_CMD_PREFIX))
	{
		return shaped;
	}
	else if (at_strstr(prefix, "NSOCR") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+NSOCR:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+NSOCR:");
		strcat(shaped, at_cmd);
	}
	else if (at_strstr(prefix, "NSOST") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+NSOST:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+NSOST:");
		strcat(shaped, at_cmd);
	}
	else if (at_strstr(prefix, "NSOSTF") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+NSOSTF:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+NSOSTF:");
		strcat(shaped, at_cmd);
	}
	else if (at_strstr(prefix, "NSORF") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+NSORF:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+NSORF:");
		strcat(shaped, at_cmd);
	}
	else if (at_strstr(prefix, "NMGR") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+NMGR:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+NMGR:");
		strcat(shaped, at_cmd);
	}
	//BUFFERED=0,RECEIVED=34,DROPPED=2
	else if (at_strstr(prefix, "NQMGR") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+NQMGR:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+NQMGR:");
		strcat(shaped, at_cmd);
	}
	//PENDING=1,SENT=34,ERROR=0
	else if (at_strstr(prefix, "NQMGS") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+NQMGS:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+NQMGS:");
		strcat(shaped, at_cmd);
	}
	//AT+CIMI       321542673
	else if (at_strstr(prefix, "CIMI") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+CIMI:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+CIMI:");
		strcat(shaped, at_cmd);
	}
	else if (at_strstr(prefix, "CGSN") != NULL)
	{
		shaped = xy_zalloc(strlen("\r\n+CGSN:") + strlen(at_cmd) + 1);
		strcpy(shaped, "\r\n+CGSN:");
		strcat(shaped, at_cmd);
	}
	return shaped;
}
#endif
int at_check_sms_fun(char *at_cmd_prefix)
{
	char *sem_chr_list[] = {
		"CMGS",
		"CMGC",
		"CNMA",
	};
	unsigned int i = 0;
	for (i = 0; i < sizeof(sem_chr_list) / sizeof(sem_chr_list[0]); i++)
	{
		if (at_strstr(at_cmd_prefix, sem_chr_list[i]))
			return 1;
	}
	return 0;
}

int is_critical_req(char *at_str)
{
	char *at_critical_req[] = {
	"AT+NRB",
	"AT+RB",
	"AT+RESET",
	"AT+SWVER",
	"AT+CGMR",
	"AT+CGMM",
	"AT+CGMI",
	"AT+NV=SAVE",
	"AT+NETDOG",
	};
	unsigned int i = 0;

	for (i = 0; i < sizeof(at_critical_req) / sizeof(at_critical_req[0]); i++)
	{
		if (!at_strncasecmp(at_str, at_critical_req[i], strlen(at_critical_req[i])) && \
				at_strncasecmp(at_str, "AT+RESETCTL", strlen("AT+RESETCTL")))
			return 1;
	}
	return 0;
}


/*待废弃，请使用at_parse_param接口*/
int parse_param(char *fmt_parm, char *buf, void **pval, int is_strict)
{
	xy_assert(fmt_parm != NULL && buf != NULL);
	int ret = AT_OK;
    char *p_douhao = NULL;
    char *p_yinhao1 = NULL;
    char *p_yinhao2 = NULL;
    char *q_douhao = NULL;
    char parase_idx = 0;
    char *p_end = NULL;
	//fmt指针指向随时会变化，释放内存时必须从起始释放，zallco首地址也必须有记录(fmt_original变量)
	int fmt_len = strlen(fmt_parm);
	char *fmt_original = xy_zalloc(fmt_len + 1);
    char *fmt = fmt_original; 
	strncpy(fmt, fmt_parm, fmt_len);

    while (*buf == ' ')
        buf++;

    p_end = strchr(buf, '\r');
    if (p_end != NULL)
        *p_end = '\0';

    if (strcmp(fmt, "%a") == 0 || strcmp(fmt, "%A") == 0)
    {
        strcpy((char *)pval[0], buf);
        goto END;
    }

    while (*buf != '\0' && *fmt != '\0')
    {
        p_douhao = strchr(buf, ',');
        q_douhao = strchr(fmt, ',');
        p_yinhao1 = strchr(buf, '"');
        p_yinhao2 = NULL;
        //"",delete ""
        if (p_yinhao1 && (p_yinhao1 < p_douhao || p_douhao == NULL))
        {
            p_yinhao2 = strchr(p_yinhao1 + 1, '"');
            //lose you yinhao
            if (p_yinhao2 == NULL)
            {
                ret = ATERR_PARAM_INVALID;
                break;
            }

            p_douhao = strchr(p_yinhao2 + 1, ',');
            buf = p_yinhao1;
            *p_yinhao2 = '\0';
        }

        if (p_douhao && q_douhao)
        {
            *p_douhao = '\0';
            *q_douhao = '\0';

            if ((ret = parase_type_val(buf, fmt, pval, &parase_idx)) != AT_OK)
                break;
            *p_douhao = ',';
            *q_douhao = ',';
            if (p_yinhao2)
                *p_yinhao2 = '"';
            buf = p_douhao + 1;
            fmt = q_douhao + 1;
        }
        else if (p_douhao)
        {
            *p_douhao = '\0';
            ret = parase_type_val(buf, fmt, pval, &parase_idx);

            //too many param
            if (ret == AT_OK && is_strict)
                ret = ATERR_MORE_PARAM;
            break;
        }
        else if (q_douhao)
        {
            *q_douhao = '\0';
            ret = parase_type_val(buf, fmt, pval, &parase_idx);
            break;
        }
        else
        {
            ret = parase_type_val(buf, fmt, pval, &parase_idx);
            break;
        }
    }
END:
    if (p_end)
        *p_end = '\r';
    if (p_douhao)
        *p_douhao = ',';
    if (p_yinhao2)
        *p_yinhao2 = '"';
	xy_free(fmt_original);
    return ret;
}

/*该接口仅内部函数调用，参数解析以可变入参方式提供，类似scanf*/
int parse_param_vp(char *fmt_parm, char *buf, int is_strict, va_list *ap)
{
	xy_assert(fmt_parm != NULL && buf != NULL);
	int ret = AT_OK;
	char *param_comma = NULL;
	char *param_quotes_begin = NULL;
	char *param_quotes_end = NULL;
	char *fmt_comma = NULL;
	char *param_end = NULL;
	//fmt指针指向随时会变化，释放内存时必须从起始释放，zallco首地址也必须有记录(fmt_original变量)
	int fmt_len = strlen(fmt_parm);
	char *fmt_original = xy_malloc(fmt_len + 1);
	*(fmt_original + fmt_len) = '\0';
	char *fmt = fmt_original; 
	strncpy(fmt, fmt_parm, fmt_len);

	while (*buf == ' ')
		buf++;

	param_end = strchr(buf, '\r');
	if (param_end != NULL)
		*param_end = '\0';

	if (strcmp(fmt, "%a") == 0 || strcmp(fmt, "%A") == 0)
	{
		strcpy((char *)va_arg(*ap, int), buf);
		goto END;
	}

	while (*buf != '\0' || *fmt != '\0')
	{
		param_comma = strchr(buf, ',');
		fmt_comma = strchr(fmt, ',');
		param_quotes_begin = strchr(buf, '"');
		param_quotes_end = NULL;

		//如果参数带引号，则去掉引号
		if (param_quotes_begin && (param_quotes_begin < param_comma || param_comma == NULL))
		{
			param_quotes_end = strchr(param_quotes_begin + 1, '"');
			//只有左引号，没有右引号
			if (param_quotes_end == NULL)
			{
				ret = ATERR_PARAM_INVALID;
				break;
			}

			param_comma = strchr(param_quotes_end + 1, ',');
			buf = param_quotes_begin + 1;
			*param_quotes_end = '\0';
		}

		if (param_comma && fmt_comma)
		{
			*param_comma = '\0';
			*fmt_comma = '\0';

			if ((ret = parase_type_val_vp(buf, fmt, ap)) != AT_OK)
				break;
			*param_comma = ',';
			*fmt_comma = ',';
			if (param_quotes_end)
				*param_quotes_end = '"';
			buf = param_comma + 1;
			fmt = fmt_comma + 1;
		}
		else if (param_comma)
		{
			*param_comma = '\0';
			ret = parase_type_val_vp(buf, fmt, ap);

			//too many param
			if (ret == AT_OK && is_strict)
				ret = ATERR_MORE_PARAM;
			break;
		}
		else if (fmt_comma)
		{
			*fmt_comma = '\0';
			ret = parase_type_val_vp(buf, fmt, ap);
			if(ret == AT_OK)
			{
				if (param_quotes_end)
					*param_quotes_end = '"';
				buf = buf + strlen(buf);
				*fmt_comma = ',';
				fmt = fmt_comma + 1;
			}
			else
				break;
		}
		else
		{
			ret = parase_type_val_vp(buf, fmt, ap);
			break;
		}
	}
END:
	if (param_end)
		*param_end = '\r';
	if (param_comma)
		*param_comma = ',';
	if (param_quotes_end)
		*param_quotes_end = '"';
	xy_free(fmt_original);
	return ret;
}

//仅用于含转义字符串参数的解析，通常用于http等特殊命令中
int parse_param_vp_3(char *fmt_parm, char *buf, int *parse_num, va_list *ap)
{
	xy_assert(fmt_parm != NULL && buf != NULL);
	int ret = AT_OK;
	char *param_comma = NULL;
	char *param_quotes_begin = NULL;
	char *param_quotes_end = NULL;
	char *fmt_comma = NULL;
	char *param_end = NULL;
	//fmt指针指向随时会变化，释放内存时必须从起始释放，zallco首地址也必须有记录(fmt_original变量)
	int fmt_len = strlen(fmt_parm);
	char *fmt_original = xy_malloc(fmt_len + 1);
	*(fmt_original + fmt_len) = '\0';
    char *fmt = fmt_original; 
	strncpy(fmt, fmt_parm, fmt_len);

	while (*buf == ' ')
		buf++;

	param_end = strchr(buf, '\r');
	if (param_end != NULL)
		*param_end = '\0';

	if (strcmp(fmt, "%a") == 0 || strcmp(fmt, "%A") == 0)
	{
		strcpy((char *)va_arg(*ap, int), buf);
		goto END;
	}

	while (*buf != '\0' || *fmt != '\0')
	{
		param_comma = strchr(buf, ',');
		fmt_comma = strchr(fmt, ',');
		param_quotes_begin = strchr(buf, '"');
		param_quotes_end = NULL;
		//"",delete ""
		if (param_quotes_begin && (param_quotes_begin < param_comma || param_comma == NULL))
		{
			// param_quotes_end = strchr(param_quotes_begin + 1, '"');
			param_quotes_end = find_next_double_quato(param_quotes_begin + 1);
			//lose you yinhao
			if (param_quotes_end == NULL)
			{
				ret = ATERR_PARAM_INVALID;
				break;
			}

			param_comma = strchr(param_quotes_end + 1, ',');
			buf = param_quotes_begin + 1;
			*param_quotes_end = '\0';
		}

		if (param_comma && fmt_comma)
		{
			*param_comma = '\0';
			*fmt_comma = '\0';

			if ((ret = parase_type_val_vp_esc(buf, fmt, parse_num, ap)) != AT_OK)
				break;
			*param_comma = ',';
			*fmt_comma = ',';
			if (param_quotes_end)
				*param_quotes_end = '"';
			buf = param_comma + 1;
			fmt = fmt_comma + 1;

		}
		else if (param_comma)
		{
			//eg: fmt: "%d,%s"  buf: 1,"2",3
			*param_comma = '\0';
			ret = parase_type_val_vp_esc(buf, fmt, parse_num, ap);
			break;
		}
		else if (fmt_comma)
		{
			//eg: fmt: "%d,%s"  buf: 1
			*fmt_comma = '\0';
			ret = parase_type_val_vp_esc(buf, fmt, parse_num, ap);
			if(ret == AT_OK)
			{
				if (param_quotes_end)
					*param_quotes_end = '"';
				buf = buf + strlen(buf);
				*fmt_comma = ',';
				fmt = fmt_comma + 1;
			}
			else
				break;
		}
		else
		{
			ret = parase_type_val_vp_esc(buf, fmt, parse_num, ap);
			break;
		}
	}
END:
	if (param_end)
		*param_end = '\r';
	if (param_comma)
		*param_comma = ',';
	if (param_quotes_end)
		*param_quotes_end = '"';
	xy_free(fmt_original);
	return ret;
}

/*待废弃，请使用at_parse_param接口*/
int at_parse_param_2(char *fmt_parm, char *buf, void **pval)
{
    return parse_param(fmt_parm, buf, pval, 0);
}
//仅用于含转义字符串参数的解析，通常用于http等特殊命令中
int at_parse_param_3(char *fmt, char *buf, int* parse_num, ...)
{
    int ret = AT_OK;
    va_list vl;
    va_start(vl,parse_num);
    *parse_num = 0;
    ret = parse_param_vp_3(fmt, buf, parse_num, &vl);
    va_end(vl);
    return ret;
}

void at_standby_timer_callback(void *arg)
{
	UNUSED_ARG(arg);
	osCoreEnterCritical();
	/* standby timer超时但当前命令仍未返回结果,保持关闭standby状态直到命令结果返回 */
	if (uart_ctx.process_state == AT_PROCESS_DONE)
	{
		g_at_off_standby = 0;
	}
	osCoreExitCritical();

	osTimerDelete(g_at_standby_timer);
	g_at_standby_timer = NULL;
}

void start_at_standby_timer()
{
    /* standby关闭或者at_standby_delay配置为0不做处理  */
    if (g_softap_fac_nv->at_standby_delay == 0 || g_softap_fac_nv->lpm_standby_enable == 0)
        return;
    if (uart_ctx.error_no == ATERR_DROP_MORE)
    {
        /* 出现8003错误启动standby定时器 */
        uart_ctx.error_no = AT_OK;
        if (g_at_standby_timer == NULL)
        {
            osTimerAttr_t timer_attr = {0};
            timer_attr.name = "at_standby";
            g_at_standby_timer = osTimerNew((osTimerFunc_t)(at_standby_timer_callback), osTimerOnce, NULL, &timer_attr);
            xy_assert(g_at_standby_timer != NULL);
        }
        osTimerStart(g_at_standby_timer, g_softap_fac_nv->at_standby_delay * 1000);
        softap_printf(USER_LOG, WARN_LOG, "8003 cause standby timer start.");
    }
    else if (g_at_standby_timer != NULL)
    {
        /* standby定时器有效时间内有at输入，重置standby定时器时间 */
        osTimerStart(g_at_standby_timer, g_softap_fac_nv->at_standby_delay * 1000);
        softap_printf(USER_LOG, WARN_LOG, "standby timer start.");
    }
    return;
}

void at_standby_set()
{
	osCoreEnterCritical();
    g_at_off_standby++;
	osCoreExitCritical();

    start_at_standby_timer();
}

void at_standby_unset()
{
	osCoreEnterCritical();
    if(g_at_off_standby != 0)
    {
    	g_at_off_standby--;
    }
	osCoreExitCritical();
}

void at_standby_clear()
{
	/* if standby timer is running, do not open standby util timer timeout */
	if (g_at_standby_timer != NULL && osTimerIsRunning(g_at_standby_timer) == 1)
	{
		softap_printf(USER_LOG, WARN_LOG, "standby timer is running,keep close standby");
		return;
	}
	osCoreEnterCritical();
   	g_at_off_standby = 0;
	osCoreExitCritical();
}
