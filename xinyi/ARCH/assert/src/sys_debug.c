#include "sys_debug.h"

#include "hardware.h"
#include "dump.h"
#include "print.h"

#include <string.h>

#define ASSERT_MAX_FILE_LENGTH          20
#define ASSERT_MAX_LINE_BITS            5
#define ASSERT_MEMCPY(dst, src, len)    memcpy(dst, src, len)
#define ASSERT_STRLEN(str)              strlen(str)

#ifdef __SOC_XINYI_1100__
#define ASSERT_PRINT_STRING             "\r\nAssert AP: "
#endif

#ifdef __SOC_XINYI_2100__

#ifdef __XINYI_2100_CORE_AP__
#define ASSERT_PRINT_STRING             "\r\nAssert AP: "
#endif

#ifdef __XINYI_2100_CORE_CP__
#define ASSERT_PRINT_STRING             "\r\nAssert CP: "
#endif

#endif


volatile char     *assert_file = NULL;
volatile int       assert_line = 0;
volatile uint16_t  assert_primask = 0;
volatile char      assert_print = 0;


void print_assert_message(char *file, int line)
{
	/*
	 * sizeof(ASSERT_PRINT_STRING) - 1: the length of string ASSERT_PRINT_STRING, not include '\0'
	 * ASSERT_MAX_FILE_LENGTH: the max length of file name
	 * 1: it is the comma ','
	 * ASSERT_MAX_LINE_BITS: the max length of line number
	 * 2: it is the new line "\r\n"
	 * 1: it is the string end character '\0'
	 */
	char assert_buf[sizeof(ASSERT_PRINT_STRING) - 1 + ASSERT_MAX_FILE_LENGTH + 1 + ASSERT_MAX_LINE_BITS + 2 + 1];
	/* the index write to buffer */
	int  buf_index = 0;
	/* save erver bit of line number */
	char line_num[ASSERT_MAX_LINE_BITS];
	/* save line number valid bits */
	uint16_t line_bits;
	/* the filename length to copy to buffer */
	uint16_t copy_file_len;

	/* sava assert message to global */
	assert_file = file;
	assert_line = line;

	/* copy assert string to buffer */
	ASSERT_MEMCPY(&assert_buf[buf_index], ASSERT_PRINT_STRING, sizeof(ASSERT_PRINT_STRING) - 1);
	buf_index += sizeof(ASSERT_PRINT_STRING) - 1;

	/* calculate the line number of decimalism string */
//	line_num[9] = line % 10000000000 / 1000000000;
//	line_num[8] = line % 1000000000 / 100000000;
//	line_num[7] = line % 100000000 / 10000000;
//	line_num[6] = line % 10000000 / 1000000;
//	line_num[5] = line % 1000000 / 100000;
	line_num[4] = line % 100000 / 10000;
	line_num[3] = line % 10000 / 1000;
	line_num[2] = line % 1000 / 100;
	line_num[1] = line % 100 / 10;
	line_num[0] = line % 10 / 1;
	/* han many bits the line number has */
	for (line_bits=0; line_bits<sizeof(line_num); line_bits++)
	{
		if(line_num[sizeof(line_num) - 1 - line_bits] != 0)
		{
			break;
		}
	}

	/* get file name length */
	copy_file_len = ASSERT_STRLEN(file);
	/* if file name length over the buffer, just save a part of it */
	if ((copy_file_len + line_bits) > (ASSERT_MAX_FILE_LENGTH + ASSERT_MAX_LINE_BITS))
	{
		copy_file_len = ASSERT_MAX_FILE_LENGTH + ASSERT_MAX_LINE_BITS - line_bits;
	}

	/* copy file name to buffer */
	ASSERT_MEMCPY(&assert_buf[buf_index], file, copy_file_len);
	buf_index += copy_file_len;

	/* copy comma to buffer */
	assert_buf[buf_index] = ',';
	buf_index += 1;

	/* copy line number string to buffer */
	for ( ; line_bits<sizeof(line_num); line_bits++)
	{
		assert_buf[buf_index] = line_num[sizeof(line_num) - 1 - line_bits] + 0x30;
		buf_index += 1;
	}

	/* copy new line and set string end */
	assert_buf[buf_index] = '\r';
	buf_index += 1;
	assert_buf[buf_index] = '\n';
	buf_index += 1;
	assert_buf[buf_index] = '\0';

	/* print assert message */
	PRINT_BUF_WITH_LEN(assert_buf, buf_index);
}


/******************************************************************************
 * @brief  : This function take the initiative to assert
 * @param  : file: assert file's name
 *           line: assert file's line
 * @retval : None
 *****************************************************************************/
void sys_assert_proc(char *file, int line)
{
	assert_primask = __get_PRIMASK();

	__set_PRIMASK(1);

	DumpRegister_from_Normal();

	print_assert_message(file, line);

	Dump_Memory();

	while(1);
}
