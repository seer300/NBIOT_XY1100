#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fs_al.h"
#include "lfs_util.h"
#include "xy_flash.h"
#include "xy_memmap.h"
#include "xy_utils.h"

#define LFS_BLOCK_DEVICE_READ_SIZE      (16)
#define LFS_BLOCK_DEVICE_PROG_SIZE      (16)
#define LFS_BLOCK_DEVICE_CACHE_SIZE     (256)
#define LFS_BLOCK_DEVICE_ERASE_SIZE     (4096)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD     (8)

#if FLASH_GIGA_4M
#define LFS_BLOCK_DEVICE_BASE     		(FS_FLASH_BASE)
#define LFS_BLOCK_DEVICE_TOTAL_SIZE     (FS_FLASH_LEN)
#else
#define LFS_BLOCK_DEVICE_BASE     		(USER_FLASH_BASE)
#define LFS_BLOCK_DEVICE_TOTAL_SIZE     (USER_FLASH_LEN_MAX)
#endif

static lfs_t lfs;
#ifdef LFS_THREADSAFE
static osMutexId_t lfs_mutex = NULL;
#endif
static int lfs_flash_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

static int lfs_flash_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

static int lfs_flash_erase(const struct lfs_config *cfg, lfs_block_t block);

static int lfs_flash_sync(const struct lfs_config *cfg);
#ifdef LFS_THREADSAFE
static int lfs_lock(const struct lfs_config *cfg);
static int lfs_unlock(const struct lfs_config *cfg);
#endif

// configuration of the filesystem is provided by this struct
static struct lfs_config lfs_cfg =
{
    .context = NULL,
    // block device operations
    .read  = lfs_flash_read,
    .prog  = lfs_flash_prog,
    .erase = lfs_flash_erase,
    .sync  = lfs_flash_sync,
#ifdef LFS_THREADSAFE
    .lock   = lfs_lock,
    .unlock = lfs_unlock,
#endif
    // block device configuration
    .read_size = LFS_BLOCK_DEVICE_READ_SIZE,
    //写flash的时候按prog_size对齐去写，这个值设置小些可以提高flash的利用率特别是小文件（小于cache_size）较多时
    .prog_size = LFS_BLOCK_DEVICE_PROG_SIZE,
    //一次性擦除的大小也是分配块的大小
    .block_size = LFS_BLOCK_DEVICE_ERASE_SIZE,
    .block_count = LFS_BLOCK_DEVICE_TOTAL_SIZE / LFS_BLOCK_DEVICE_ERASE_SIZE,
    //存放目录的block前四个字节记录revision，当revision大于block_cycles换新块
    .block_cycles = 200,
    //小于等于cache_size的文件直接存放到entry中，当小文件较多时可以适当增大这个值来提高flash的利用率
    //文件系统挂载后会为读写各分配一个cache_size大小的buffer，所以这个值增大需要更多的ram
    .cache_size = LFS_BLOCK_DEVICE_CACHE_SIZE,
    //块分配器bitmap的大小lookahead_size*8，滑窗大小
    .lookahead_size = LFS_BLOCK_DEVICE_LOOK_AHEAD, //fixed-size bitmap for allocator

	//文件名最大长度
    .name_max = LFS_NAME_MAX,
    //文件最大大小
    .file_max = LFS_FILE_MAX,  
    .attr_max = 0
};

static int lfs_flash_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    LFS_ASSERT(off  % cfg->read_size == 0);
    LFS_ASSERT(size % cfg->read_size == 0);
    LFS_ASSERT(block < cfg->block_count);

	xy_flash_read(LFS_BLOCK_DEVICE_BASE + block * cfg->block_size + off, buffer, size);

    return LFS_ERR_OK;
}

static int lfs_flash_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{

    LFS_ASSERT(off  % cfg->prog_size == 0);
    LFS_ASSERT(size % cfg->prog_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    int ret;
    ret = xy_flash_write_no_erase(LFS_BLOCK_DEVICE_BASE + block * cfg->block_size + off, buffer, size);

    return (ret == 0) ? LFS_ERR_OK: LFS_ERR_IO;
}

static int lfs_flash_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    LFS_ASSERT(block < cfg->block_count);

    int ret;
    ret = xy_flash_erase(LFS_BLOCK_DEVICE_BASE + block * cfg->block_size, LFS_BLOCK_DEVICE_ERASE_SIZE);

    return (ret == 0) ? LFS_ERR_OK: LFS_ERR_IO;
}
/* 如果底层写flash有cache需要用这个函数将cache中的数据同步到flash,
   xy1100写flash没有cache, 这里直接返回0
*/
static int lfs_flash_sync(const struct lfs_config *cfg)
{
	(void)cfg;
    return 0;
}

#ifdef LFS_THREADSAFE
static int lfs_lock(const struct lfs_config *cfg)
{
	(void)cfg;

	if(!lfs_mutex)
	{
		lfs_mutex = osMutexNew(NULL);
	}

	if(osKernelGetState() >= osKernelRunning)
	{
		osMutexAcquire(lfs_mutex, osWaitForever);
	}
	
    return 0;
}

static int lfs_unlock(const struct lfs_config *cfg)
{
	(void)cfg;

	if(osKernelGetState() >= osKernelRunning)
	{
		osMutexRelease(lfs_mutex);
	}
    return 0;
}
#endif

int fs_mount(void)
{
    int err;

	//已经挂载过
	if(lfs.cfg != NULL)
	{
		return LFS_ERR_OK;
	}
	
	err = lfs_mount(&lfs, &lfs_cfg);

    //首次启动时flash全FF挂载会失败，需要格式化后再重新挂载
    if (err)
    {
        err = lfs_format(&lfs, &lfs_cfg);
        if(err)
            return err;

        err = lfs_mount(&lfs, &lfs_cfg);
        if(err)
            return err;
    }

    return LFS_ERR_OK;
}

int fs_unmount(void)
{
    int err = lfs_unmount(&lfs);
	lfs.cfg = NULL;

	return err;
}

int fs_open(fs_file_t *file, const char *path, int flags)
{
	int ret;
	uint32_t name_len;

	ret = lfs_file_open(&lfs, &file->file, path, flags);
	if(ret == LFS_ERR_OK)
	{
		name_len = strlen(path) + 1;
		file->filename = xy_malloc(name_len);
		strncpy(file->filename, path, name_len);
	}
    return ret;
}

int fs_close(fs_file_t *file)
{
	if(file->filename)
	{
		xy_free(file->filename);
		file->filename = NULL;
	}
	
    return lfs_file_close(&lfs, &file->file);
}

int fs_read(fs_file_t *file, void *buffer, uint32_t size)
{
    return lfs_file_read(&lfs, &file->file, buffer, size);
}

int fs_write(fs_file_t *file, const void *buffer, uint32_t size)
{
    return lfs_file_write(&lfs, &file->file, buffer, size);
}

int fs_sync(fs_file_t *file)
{
	return lfs_file_sync(&lfs, &file->file);
}

int fs_seek(fs_file_t *file, int off, int whence)
{
    return lfs_file_seek(&lfs, &file->file, off, whence);
}

int fs_tell(fs_file_t *file)
{
    return lfs_file_tell(&lfs, &file->file);
}

int fs_size(fs_file_t *file)
{
    return lfs_file_size(&lfs, &file->file);
}

int fs_truncate(fs_file_t *file, uint32_t size)
{
    return lfs_file_truncate(&lfs, &file->file, size);
}

int fs_remove(const char *path)
{
    return  lfs_remove(&lfs, path);
}

int fs_statfs(fs_stat_t *stat)
{
    int ret;
	
	stat->block_size = lfs_cfg.block_size;
	stat->total_block = lfs_cfg.block_count;
	ret = lfs_fs_size(&lfs);
	if(ret > 0)
	{
		stat->block_used = ret;
		ret = 0;
	}
	
    return ret;
}

int fs_rename(const char *oldpath, const char *newpath)
{
	return lfs_rename(&lfs, oldpath, newpath);
}

int fs_mkdir(const char *path)
{
	return lfs_mkdir(&lfs, path);
}

int fs_dir_open(fs_dir_t *dir, const char *path)
{
	return lfs_dir_open(&lfs, dir, path);
}

int fs_dir_read(fs_dir_t *dir, fs_info_t *info)
{
	return lfs_dir_read(&lfs, dir, info);
}

int fs_dir_close(fs_dir_t *dir)
{
	return lfs_dir_close(&lfs, dir);
}

