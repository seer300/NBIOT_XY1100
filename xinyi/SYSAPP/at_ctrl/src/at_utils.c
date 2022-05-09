/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ctl.h"
#include "at_global_def.h"
#include "xy_at_api.h"
#include "xy_sys_hook.h"
#include "xy_utils.h"

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
/*
 * 转义字符表，用于转义字符转换ASCII码
*/
esc_item_t esc_table[] =
{
	{'a', '\a'},
	{'b', '\b'},
	{'f', '\f'},
	{'n', '\n'},
	{'r', '\r'},
	{'t', '\t'},
	{'v', '\v'},
	{'?', '\?'},
	{'\\', '\\'},
	{'\'', '\''},
	{'\"', '\"'},
};

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
/*not  case  sensitive,see strstr*/
char * at_strstr(char * source, char * substr)
{
	return	strstr(source,substr);
}

/*skip "XXXX" influence,return next char*/
char * at_strnstr(char * source, char * substr, unsigned int n)
{
	char *ret;
	char *end = xy_zalloc(n+1);

	memcpy(end,source,n);
	ret = strstr(end,substr);
	if(ret == NULL)
	{
		xy_free(end);
		return NULL;
	}
	xy_free(end);
	return (char *)(source+(ret-end)+strlen(substr));
}

/*n is jump N match,see strnchr,only use to locate ,*/
char * at_strnchr(char *s, int c, int n)
{
	char *match = NULL;
	int i = 0;

	do {
		if (*s == (char)c) {
			i++;
			if (i == n) {
				match = (char *)s;
				break;
			}
		}
	} while (*s++);

	return match;
}

//find substr,and return next char
char *at_strstr2(char * source, char * substr)
{
	char *head=NULL;
	if((head=at_strstr(source,substr))!=NULL)
		head+=strlen(substr);
	return head;
}

int is_digit_str(char *str)
{
    unsigned digit_len = 0;

    while(*str == ' '){
        str++;
    }
	if(*str == '-'){
		str++;
	}
    while(isdigit(*str)){
        str++, digit_len++;
    }
    while(*str == ' '){
        str++;
    }
    return *str == '\0' && digit_len > 0;
}

/*"0XA358"--->0XA358 */  
int hexstr2int(char *hex,int *value)  
{  
    int len;   
    int temp;  
    int val=0;
    int i=0;  

	if(strncmp(hex,"0x",2)==0 || strncmp(hex,"0X",2)==0)
		hex+=2;
	else
		return XY_ERR;
	
	len = strlen(hex); 
	while(!(hex[i]<'0' || (hex[i] > '9'&&hex[i] < 'A') || (hex[i] > 'F' && hex[i] < 'a') || hex[i] > 'f'))
		i++;
    if(i < len)
		return XY_ERR;

    for (i=0, temp=0; (i<len && i<8); i++)  
    {  
			if(isdigit(hex[i]))  
            	temp = (hex[i] - 48);

    		if(isalpha(hex[i]))  
            	temp =  (isupper(hex[i]) ? hex[i] - 55 : hex[i] - 87);
    		val = val*16+temp;
    }   
    *value = val;
	return  XY_OK;
}  

//not care uppercase and lowercase,such as "AT+WORKLOCK" and "AT+worklock" is same
int at_strcasecmp(const char *s1, const char *s2)
{
    int ch1 = 0;
    int ch2 = 0;

    if(NULL == s1 || NULL == s2)
	{
        return -1;
    }

    do
	{
        if((ch1 = *((unsigned char *)s1++)) >= 'A' && ch1 <= 'Z')
		{
            ch1 += DIFF_VALUE;
        }
        if((ch2 = *((unsigned char *)s2++)) >= 'A' && ch2 <= 'Z')
		{
            ch2 += DIFF_VALUE;
        }
    }while(ch1 && ch1 == ch2);

    return ch1 - ch2;
}

//0 is equal.not care uppercase and lowercase,such as "AT+WORKLOCK=1" and "AT+worklock" is same before strlen(AT+worklock) bytes
int at_strncasecmp(const char *s1, const char *s2, int n)
{
    int ch1 = 0;
    int ch2 = 0;

    if(NULL == s1 || NULL == s2 || 0 > n)
	{
        return -1;
    }

    if(0 == n)
	{
        return 0;
    }

    do
	{
        if((ch1 = *(unsigned char *)s1++) >= 'A' && (ch1 <= 'Z'))
		{
            ch1 += DIFF_VALUE;
        }
        if((ch2 = *(unsigned char *)s2++) >= 'A' && (ch2 <= 'Z'))
		{
            ch2 += DIFF_VALUE;
        }
    }while(--n && ch1 && (ch1 == ch2));

    return ch1 - ch2;
}

int at_is_rsp_err(char *str, int *err_no)
{
	char *err_head = NULL;
	char *temp_char = NULL;
	char *head_tag = NULL;
	char *chr = NULL;
	xy_assert(str != NULL);

	while (*str == '\r' || *str == '\n')
		str++;
	if (at_strncasecmp(str, "+CME ERROR:", strlen("+CME ERROR:")) && at_strncasecmp(str, "+CIS ERROR:", strlen("+CIS ERROR:")) && at_strncasecmp(str, "+CMS ERROR:", strlen("+CMS ERROR:")) && at_strncasecmp(str, "ERROR\r\n", strlen("ERROR\r\n"))&& at_strncasecmp(str, "+CTLW ERROR:", strlen("+CTLW ERROR:")))
		return 0;

	//"\r\n+CME ERROR:xxx\r\n""\r\n+CMS ERROR:xxx\r\n" or "\r\nERROR\r\n"
	if ((err_head = at_strstr(str, "ERROR")) != NULL)
	{
		//ERRORxxxxxxxxxxxxxxx
		if (g_softap_fac_nv->cmee_mode == 2)
			return 1;

		if (strchr(err_head, '\r') == NULL || strchr(err_head, '\r') - err_head > 40)
			return 0;

		head_tag = (err_head - str > 10 ? (err_head - 10) : str);
		head_tag = strchr(head_tag, '\r');

		//"\r\nERROR\r\n"    "ERROR\r\n"    " +CME ERROR:"
		if (head_tag > err_head && err_head - str > 7)
			return 0;

		temp_char = err_head + strlen("ERROR");
		if (err_no != NULL && temp_char < str + strlen(str) && *temp_char != '\r')
		{
			char *num_first = temp_char + 1;
			while (*num_first == ' ')
			{
				num_first++;
			}
			if ((chr = strchr(num_first, '\r')) != NULL)
			{
				*chr = '\0';
			}
			if (is_digit_str(num_first))
			{
				*err_no = (int)strtol(num_first,NULL,10);
#if VER_QUCTL260
				if (chr != NULL)
					*chr = '\r';
				if(*err_no >= ATERR_XY_ERR && *err_no <= ATERR_LOW_VOL)
					sprintf(num_first, "%d\r\n", get_at_err_num(*err_no));
				return 1;
#endif
			}
			if (chr != NULL)
				*chr = '\r';
		}
		return 1;
	}
	return 0;
}

int is_ok_at(char *str)
{
	char *ok_head = at_strstr(str, "OK\r");
	if (ok_head)
	{
		if (ok_head == str)
			return 1;
		else if (ok_head - str >= 2 && *(ok_head - 1) == '\n' && *(ok_head - 2) == '\r')
			return 1;
	}
	return 0;
}

int is_result_at(char *str,int *at_rsp_no)
{
	if (is_ok_at(str))
	{
		if(at_rsp_no != NULL)
			*at_rsp_no = AT_OK;
		
		return 1;
	}
	else if (at_is_rsp_err(str,at_rsp_no))
		return 1;
	else if (is_user_result_hook(str,at_rsp_no))
		return 1;
	else
		return 0;
}

char *at_ok_build()
{
	char *at_str;

	at_str = xy_malloc(7);
	snprintf(at_str, 7, AT_RSP_OK);
	return at_str;
}

int is_at_char(char c)
{
    return (((((unsigned char)c) >= ' ') && (((unsigned char)c) <= '~')) || c == '\r' || c == '\n');
}

int vary_to_uppercase(char *at_cmd)
{
	if (at_cmd == NULL)
		return 0;

	char *pchar = at_cmd;

	while (*pchar != '=' && *pchar != '?' && *pchar != ':' && *pchar != '\r' && *pchar != '\0')
	{
		if (*pchar >= 'a' && *pchar <= 'z')
			*pchar -= DIFF_VALUE;
		pchar++;
	}

	return 1;
}

bool isHexChar(char ch)
{
	if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
		return true;
	else
		return false;
}

char covertHextoNum(char ch)
{
	char temp=0;
	if (isdigit(ch))
		temp = (ch - '0');

	if (isalpha(ch))
		temp = (isupper(ch) ? ch - 55 : ch - 87);
	return (char)temp;
}

bool isOctChar(char ch)
{
	if ((ch >= '0' && ch <= '7'))
		return true;
	else 
		return false;
}

int covertEscToAscII(char* p)
{
	int len = -1;
	int index_val = *(p + 1);
	int i = 0;
	for (; i < (int)(sizeof(esc_table) / sizeof(esc_item_t)); i++)
	{
		if (index_val == esc_table[i].ch)
		{
			*p = esc_table[i].esc_ch;
			softap_printf(USER_LOG, WARN_LOG, "match esc table and changed:%d", *p);
			return 1;
		}
	}

	//Hex
	if(index_val == 'x')
	{
		if(isHexChar(*(p + 2)) && isHexChar(*(p + 3)))
		{
			*p = (covertHextoNum(*(p + 2)) << 4) + covertHextoNum(*(p + 3));
			softap_printf(USER_LOG, WARN_LOG, "hex changed:%d", *p);
			return 3;
		}
		else if(isHexChar(*(p + 2)))
		{
			*p = covertHextoNum(*(p + 2));
			softap_printf(USER_LOG, WARN_LOG, "hex changed:%d", *p);
			return 2;			
		}
	}

	//Octal eg:/101
	if (isOctChar(*(p + 1)) && isOctChar(*(p + 2)) && isOctChar(*(p + 3)))
	{
		*p = (char)(((*(p + 1) - '0') << 6) + ((*(p + 2) - '0') << 3) + (*(p + 3) - '0'));
		softap_printf(USER_LOG, WARN_LOG, "oct changed:%d", *p);
		return 3;
	}
	else if (isOctChar(*(p + 1)) && isOctChar(*(p + 2)))
	{
		*p = (char)(((*(p + 1) - '0') << 3) + (*(p + 2) - '0'));
		softap_printf(USER_LOG, WARN_LOG, "oct changed:%d", *p);
		return 2;
	}
	else if (isOctChar(*(p + 1)))
	{
		*p = (char)(*(p + 1) - '0');
		softap_printf(USER_LOG, WARN_LOG, "oct changed:%d", *p);
		return 1;
	}

	return len;
}

void format_escape_char(char *input)
{
	xy_assert(input != NULL);
	char *p = input;
	int offset = 0;
	unsigned int i = 0;

	while ((p = strchr(p, '\\')) != NULL && *(p + 1) != '\0')
	{
		if ((offset = covertEscToAscII(p)) != -1)
		{
			if(strlen(p + offset + 1) + 1 < (unsigned int)(offset)) //eg: \101a --> Aa,  原先1a位置必须置0
			{
				memset(p + 1, '\0', (unsigned int)(offset));
			}	
			for (i = 0; i <= strlen(p + offset + 1); i++)
			{
				*(p + 1 + i) = *(p + offset + 1 + i);
			}
		}
		p = p + 1;
	};
}

char* find_next_double_quato(char* data)
{
	xy_assert(data != NULL);
	char *tmp = NULL;

	while(*data != '\0')
	{
		if(*data == '"' && *(data - 1) != '\\')
		{
			tmp = data;
			break;
		}
		data++;
	}

	return tmp;
}
