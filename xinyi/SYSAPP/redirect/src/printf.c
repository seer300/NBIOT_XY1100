#include "print.h"


#if __CC_ARM

// 使用MDK编译器时，GCC库的printf实现
// 加入以下代码,支持printf函数,而不需要选择use MicroLIB
#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE
{
	int handle;
	/* Whatever you require here. If the only file you are using is */
	/* standard output using printf() for debugging, no file handling */
	/* is required. */
};
/* FILE is typedef in stdio.h. */
FILE __stdout;
// 定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
	(void)x;
}
// printf implementation of the MDK library
int fputc(int ch, FILE *f)
{
	(void)f;

	PRINT_CHAR(ch);

	return ch;
}

#endif


#ifdef __GNUC__

// printf implementation of the GCC library
int _write (int fd, char *pBuffer, int size)
{
	int i=0;

	(void)fd;	// Prevents compiler warnings

	for (i = 0; i < size; i++)
	{
		PRINT_CHAR(pBuffer[i]);
	}

	return size;
}

#endif
