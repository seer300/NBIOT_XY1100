#include <stdio.h>
#include "xy_fs.h"
#include "xy_utils.h"

xy_file xy_fopen(const char * fileName, const char * mode)
{
	int32_t open_mode;
	fs_file_t * pf;

	/* Parse the specified mode. */
	open_mode = LFS_O_RDONLY;
	if (*mode != 'r') {			/* Not read... */
		open_mode = (LFS_O_WRONLY | LFS_O_CREAT);
		if (*mode != 'w') {		/* Not write (create or truncate)... */
			open_mode = (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
			if (*mode != 'a') {	/* Not write (create or append)... */
				return NULL;
			}
		}
	}

	if ((mode[1] == 'b')) {		/* Binary mode (NO-OP currently). */
		++mode;
	}

	if (mode[1] == '+') {			/* Read and Write. */
		++mode;
		open_mode |= (LFS_O_RDONLY | LFS_O_WRONLY);
		open_mode += (LFS_O_RDWR - (LFS_O_RDONLY | LFS_O_WRONLY));
	}

    pf = xy_malloc(sizeof(fs_file_t));

	if(fs_open(pf, fileName, open_mode))
	{
		xy_free(pf);
		pf = NULL;
	}
	
    return pf;
}

int32_t xy_fclose(xy_file fp)
{
	int ret;

	ret = fs_close(fp);
	xy_free(fp);
	
    return ret;
}

uint32_t xy_fread(void * buf, uint32_t len, xy_file fp)
{

	if(len == 0)
	return 0;
	
	len = (uint32_t)fs_read(fp, buf, len);
	return len;
}

uint32_t xy_fwrite(void *buf, uint32_t len ,xy_file fp)
{

	if(len == 0)
	return 0;
	
	len = fs_write(fp, buf, len);
	return len;
}

uint32_t xy_fwrite_safe(void *buf, uint32_t len, xy_file fp)
{

	uint32_t len_r = 0;
	uint32_t len_w = 0;
	uint8_t * buf_r = NULL;

	if(len == 0)
		return 0;

	buf_r = xy_malloc(len);
	len_r = (uint32_t)fs_read(fp, buf_r, len);

    if(len == len_r&&!memcmp(buf_r, buf,len_r))
    {
    	len_w = len;
    }
    else
    {
    	fs_seek(fp, 0, 0);
    	len_w = fs_write(fp, buf, len);

    }

	xy_free(buf_r);
	return len_w;
}

int32_t xy_ftell(xy_file fp)
{
	return fs_tell(fp);
}

int32_t xy_fseek(xy_file fp, int32_t offset, int32_t seekType)
{
	return fs_seek(fp, offset, seekType);
}

int32_t xy_fsync(xy_file fp)
{
	return fs_sync(fp);
}

int32_t xy_fsize(xy_file fp)
{
	return fs_size(fp);
}

int32_t xy_fremove(const char *fileName)
{
	return fs_remove(fileName);
}

