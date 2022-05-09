#include "patch.h"
#include "stdarg.h"
#include <stddef.h>

void PATCH_GPIOConflictStatusClear(unsigned char ucPins)
{
    unsigned long ulOffsetReg;
    unsigned long ulOffsetBit;
    
    if(ucPins < 32)
    {
        ulOffsetReg = 0;
        ulOffsetBit = 1 << ucPins;
    }
    else
    {
        ulOffsetReg = 4;
        ulOffsetBit = 1 << (ucPins & 0x1F);
    }
    
    HWREG(GPIO_BASE + GPIO_PAD_CFT0 + ulOffsetReg) = ulOffsetBit;
}


void PATCH_SEMARequest(unsigned long ulBase, unsigned char ucSemaMaster, unsigned char ucSemaSlave)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    // Request
    //HWREG(ulSemaReg) |= (SEMA_SEMA_REQ | 0xFF);
    
    // wait until actually granted
    while((HWREG(ulSemaReg) & SEMA_SEMA_OWNER_Msk) != (((unsigned long)(ucSemaMaster)) << SEMA_SEMA_OWNER_Pos))
    {
		// Request
    	HWREG(ulSemaReg) = (SEMA_SEMA_REQ | (1<<(ucSemaMaster-1)));
    }
}

void PATCH_SEMARelease(unsigned long ulBase, unsigned char ucSemaMaster, unsigned char ucSemaSlave)
{
    unsigned long ulSemaReg;
    
    ulSemaReg = ulBase + REG_SEMA_SEMA0 + (ucSemaSlave << 2);
    
    // Release
    //HWREG(ulSemaReg) = (HWREG(ulSemaReg) & ~(SEMA_SEMA_REQ)) | 0xFF;
    do
	{
    	HWREG(ulSemaReg) = HWREG(ulSemaReg) & ~(SEMA_SEMA_REQ| (1<<(ucSemaMaster-1))) ;
    }while((HWREG(ulSemaReg) & SEMA_SEMA_OWNER_Msk) == (((unsigned long)(ucSemaMaster)) << SEMA_SEMA_OWNER_Pos));
}

static void printch(char ch)
{
	ROM_UARTCharPut(UART1_BASE, ch);
}

static void printdec(int dec)
{
    if(dec==0)
    {
       return;
    }
    printdec(dec/10);
    printch( (char)(dec%10 + '0'));
}

static void printflt(double flt)
{
    //int icnt = 0;
    int tmpint = 0;
    
    tmpint = (int)flt;
    printdec(tmpint);
    printch('.');
    flt = flt - tmpint;
    tmpint = (int)(flt * 1000000);
    printdec(tmpint);
}

static void printstr(char* str)
{
    while(*str)
    {
        printch(*str++);
    }
}

static void printbin(int bin)
{
    if(bin == 0)
    {
        printstr("0b");
        return;
    }
    printbin(bin/2);
    printch( (char)(bin%2 + '0'));
}

static void printhex(int hex)
{
    if(hex==0)
    {
        printstr("0x");
        return;
    }
    printhex(hex/16);
    if(hex%16 < 10)
    {
        printch((char)(hex%16 + '0'));
    }
    else
    {
        printch((char)(hex%16 - 10 + 'a' ));
    }
}


void PATCH_UARTPrint(char* fmt, ...)
{
    double vargflt = 0;
    int  vargint = 0;
    char* vargpch = NULL;
    char vargch = 0;
    char* pfmt = NULL;
    va_list vp;
    va_start(vp, fmt);
    pfmt = fmt;

    while(*pfmt)
    {
        if(*pfmt == '%')
        {
            switch(*(++pfmt))
            {

                case 'c':
                    vargch = va_arg(vp, int);
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                        mode, or a warning otherwise.In non-strict ANSI mode, 'type' is allowed to be any expression. */
                    printch(vargch);
                    break;
                case 'd':
                case 'i':

                	vargint = va_arg(vp, int);
                    if(vargint ==0)
                    	printch(vargint);
                    else
                    	printdec(vargint);

                    break;
                case 'f':
                    vargflt = va_arg(vp, double);
                    /*    va_arg(ap, type), if type is narrow type (char, short, float) an error is given in strict ANSI
                        mode, or a warning otherwise.In non-strict ANSI mode, 'type' is allowed to be any expression. */
                    printflt(vargflt);
                    break;
                case 's':
                    vargpch = va_arg(vp, char*);
                    printstr(vargpch);
                    break;
                case 'b':
                case 'B':
                    vargint = va_arg(vp, int);
                    printbin(vargint);
                    break;
                case 'x':
                case 'X':
                    vargint = va_arg(vp, int);
                    printhex(vargint);
                    break;
                case '%':
                    printch('%');
                    break;
                default:
                    break;
            }
            pfmt++;
        }
        else
        {
            printch(*pfmt++);
        }
    }
    va_end(vp);
}
