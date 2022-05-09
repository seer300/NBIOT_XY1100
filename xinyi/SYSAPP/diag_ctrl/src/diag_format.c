#include <diag_format.h>
#include "ItemStruct.h"
#include "diag_out.h"
#include "softap_macro.h"
#if !IS_DSP_CORE
#include "xy_utils.h"
#include "flash_adapt.h"
#endif

#define ZEROPAD		    1		/* pad with zero */
#define SIGN			2		/* unsigned/signed long */
#define PLUS			4		/* show plus */
#define SPACE			8		/* space if plus */
#define LEFT			16		/* left justified */
#define SPECIAL		    32		/* 0x */
#define LARGE			64		/* use 'ABCDEF' instead of 'abcdef' */


#define QUALIFIER_SINGLE (1<<0)   /* qualifier flag ----'h','l' ,'q'   */
#define QUALIFIER_DOUBLE (1<<1)   /* qualifier flag ----'hh','ll','qq' */
#define is_digit(c) 	((c) >= '0' && (c) <= '9')
static void skip_atoi(const char **s);


/*! \fn      static UINT32 diag_print_ArgParser(const char*   pfmt,
 *                                              UINT32*       arg_num,
 *                                              UINT8*        pArgBuf,
 *                                              UINT32        arg_buf_sz
 *                                              )
 *  \brief  parser arguments through fomat sting and fetch argment data.
 *
 *  \param  [in]  pfmt       :pfmtpoint argment format string
 *  \param  [out] arg_num    :return argments number
 *  \param  [in]  pArgBuf    :point data buf for storing argments
 *  \param  [in]  arg_buf_sz :data buf size available
 *  \return [out]            :return argument data length.
 */
#define OPENPRINT 1
unsigned int diag_str2memory(const char* pfmt,
							unsigned char strLen,
							unsigned char*      pArgBuf,
                            int      arg_buf_sz,
                            va_list     args
                           )
{
	unsigned int       qualifier_flag = 0;
	unsigned int       arg_buf_len    = 0;
	const char*  pstr;
	int        align_flags = 0;
	int        field_flag = 0;
	int        precision_flag = 0;
	unsigned int         ret = 1;
	unsigned char*       psrc = pArgBuf;
	int str_len = 0;
	int i = 0;

	unsigned int arg_num = 0;

    memset(pArgBuf, 0, arg_buf_sz);


    for(i = 0; (*pfmt) && (arg_buf_sz >= 8) && i < strLen ; ++pfmt, i++)
    {
        if(*pfmt != '%')
        {
            continue;
        }

	
        /*1: process aligen flags */

        align_flags = 0;

repeat:
        ++pfmt;

        /* skips first '%' */

        switch(*pfmt)
        {
        case '-':
            align_flags |= LEFT;
            goto repeat;

        case '+':
            align_flags |= PLUS;
            goto repeat;

        case ' ':
            align_flags |= SPACE;
            goto repeat;

        case '#':
            align_flags |= SPECIAL;
            goto repeat;

        case '0':
            align_flags |= ZEROPAD;
            goto repeat;

        }

        /*2: get field width */

        field_flag = 0;
        if(is_digit(*pfmt))
        {
            skip_atoi(&pfmt);
            precision_flag = 1;

        }
        else if(*pfmt == '*')
        {
            ret = 0;
            goto END;
        }


        /*3: get the precision */

        precision_flag = 0;
        if(*pfmt == '.')
        {
            ++pfmt;

            if (is_digit(*pfmt))
            {
                precision_flag = 1;
                skip_atoi(&pfmt);

            }
            else if(*pfmt == '*')
            {
                ret = 0;
                goto END;
            }


        }

        /*4: get the conversion qualifier */

        qualifier_flag = 0;
        if(*pfmt == 'h' || *pfmt == 'q')
        {
            ret = 0;
            goto END;
        }
        else if(*pfmt == 'l')
        {
            ++pfmt;
            qualifier_flag |= QUALIFIER_SINGLE;
            if(*pfmt == 'l')
            {
                qualifier_flag = 0;
                qualifier_flag |= QUALIFIER_DOUBLE;
                ++pfmt;
            }
        }

        if((align_flags & SPACE) && (precision_flag || field_flag))
        {
            ret = 0;
            break;

        }

        switch (*pfmt)
        {
        /* integer number formats  */

        case 'o':
        case 'X':
        case 'x':
        case 'd':
        case 'i':
        case 'u':
        case 'c':
        {           
             if(!(qualifier_flag & QUALIFIER_DOUBLE))
            {
                *((volatile unsigned int*)pArgBuf) = va_arg(args, unsigned int);

                arg_buf_sz -= 4;
                pArgBuf += 4;

            }
            else
            {
                *((volatile unsigned long long*)pArgBuf) = va_arg(args, unsigned long long);


                arg_buf_sz -= 8;
                pArgBuf += 8;

            }
            arg_num++;
        }
        break;

        /* point number formats */

        case 'p':
        {
            *((volatile unsigned int*)pArgBuf) = (unsigned int)va_arg(args, void *);
            arg_buf_sz -= 4;
            pArgBuf += 4;
            arg_num++;
        }
        break;

        /* float  notation */

        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G':

            if(qualifier_flag & QUALIFIER_SINGLE)
            {
                ret = 0;
                goto END;
            }


            *((volatile double *)pArgBuf) = va_arg(args, double);

            arg_buf_sz -= 8;
            pArgBuf += 8;
            arg_num++;

            break;

        /* string format */

        case 's':

            pstr = va_arg(args, char *);
            if (!pstr)
            {
                *((volatile unsigned int*)pArgBuf) = 0;
            }


            str_len = strlen(pstr);
            if(str_len > arg_buf_sz - 1)
            {
                str_len = arg_buf_sz - 1;

            }
            memcpy(pArgBuf, pstr, str_len);
            *((volatile unsigned char*)(pArgBuf + str_len)) = '\0';
            /* word align */
            pArgBuf = (unsigned char*)((unsigned int)(pArgBuf + (str_len + 1) + (sizeof(int) - 1)) & (~(sizeof(int) - 1)));
            arg_buf_sz = arg_buf_sz - ((str_len + 1 + (sizeof(int) - 1)) & (~(sizeof(int) - 1)));

            arg_num++;


            break;

        case 'a':
        case 'A':
        case 'n':

            /* un-support format */
            ret = 0;
            goto END;
            //break;

        case '%':
            /* ignore */

            break;

        default:

            break;

        }


    }

END:

    arg_buf_len = ret ? (unsigned int)(pArgBuf - psrc) : 0;
    return arg_buf_len;

}
static void skip_atoi(const char **s)
{

    while (is_digit(**s))  (*s)++;

}

static unsigned short seq_no = 0;

unsigned short get_Seq_num()
{
	return 	seq_no++;
}

void diag_packet_header(char *pItemHeader, unsigned int buf_sz, unsigned int type_id, unsigned int item_id)
{

	ItemHeader_t *itemHeader = (ItemHeader_t *)pItemHeader;
	pItemHeader[0] = 0x5A;
	pItemHeader[1] = 0xA5;
	pItemHeader[2] = 0xFE;
	pItemHeader[3] = Chip_1100;
	itemHeader->u16Len = buf_sz - HEADERSIZE - LENSIZE;
	itemHeader->u4TraceId = type_id;
	itemHeader->u28ClassId = item_id;
	itemHeader->u16SeqNum = 0;
#if IS_DSP_CORE
	itemHeader->u32Time = OSXY_GetTickCount();	
#else
	itemHeader->u32Time = osKernelGetTickCount();
#endif
}

void diag_packet_header_m3(char *pItemHeader,unsigned int buf_sz, unsigned int type_id, unsigned int item_id, unsigned int m3Time)
{
	ItemHeader_t *itemHeader = (ItemHeader_t *)pItemHeader;
	pItemHeader[0] = 0x5A;
	pItemHeader[1] = 0xA5;
	pItemHeader[2] = 0xFE;
	pItemHeader[3] = Chip_1100;
	itemHeader->u16Len = buf_sz - HEADERSIZE - LENSIZE;
	itemHeader->u4TraceId = type_id;
	itemHeader->u28ClassId = item_id;
	itemHeader->u16SeqNum = 0;
	itemHeader->u32Time = m3Time;	
}



