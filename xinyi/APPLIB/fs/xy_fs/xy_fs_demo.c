#include "xy_fs.h"
#include "fs_al.h"
#include "xy_system.h"
#include "xy_utils.h"

#define DEMO_NAME  "file"

void fs_demo(void)
{
    xy_file fp = NULL;
	int readCount = XY_ERR;
	int writeCount = XY_ERR;
	int pos = XY_ERR;
	int fp_size = XY_ERR;
	int offset = XY_ERR;

	char writebuf[64] = {0};
	char readbuf[64] = {0};

  //使用文件系统之前，先挂载文件系统
   fs_mount();
  //如果文件已存在，直接打开，如果不存在，创建文件
  fp = xy_fopen(DEMO_NAME, "w+");

    if(fp!=NULL)
    {
		sprintf(writebuf, "xy_fs demo test");
		writeCount = xy_fwrite(writebuf, strlen(writebuf), fp);
        //查询文件大小
		fp_size = xy_fsize(fp);
        //文件位置指针移动到文件头
		offset = xy_fseek(fp,0,0);
        //查询文件位置指针
        //读文件，返回读取的字节数
		pos = xy_ftell(fp);
		readCount = xy_fread(readbuf, 15, fp);
        xy_log_print(USER_LOG, WARN_LOG,"%s%d", readbuf, pos);
		sprintf(writebuf, "!!!");
        //文件位置指针移动到原字符串结束处，修改原文件的数据
		offset = xy_fseek(fp, 0, 1);
		writeCount = xy_fwrite(writebuf, strlen(writebuf), fp);
        //查询文件大小
		fp_size  = xy_fsize(fp);
        //文件位置指针移动到文件头，读取现在文件中的数据
        offset = xy_fseek(fp,0,0);
		readCount = xy_fread(readbuf, 18, fp);
		//通过打印的Log检验是否正确
		xy_log_print(USER_LOG, WARN_LOG,"%s%d", readbuf, pos);
        pos = xy_ftell(fp);
        //读写完毕，关闭文件，数据同步到flash
		xy_fclose(fp);

    }


}



