/**
 * @file
 * @brief Implementation of FAT driver
 *
 * @date 28.03.2012
 * @author Andrey Gazukin
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>

#include <util/err.h>

#include <fs/file_desc.h>
#include <fs/super_block.h>
#include <fs/fs_driver.h>
#include <fs/inode_operation.h>
#include <fs/inode.h>

#include <drivers/block_dev.h>

#include "fat.h"

extern struct file_operations fat_fops;

extern int fat_fill_sb(struct super_block *sb, const char *source);
extern int fat_clean_sb(struct super_block *sb);
extern int fat_create(struct inode *i_new, struct inode *i_dir, int mode);
extern int fat_format(struct block_dev *dev, void *priv);
extern struct inode *fat_ilookup(char const *name, struct inode const *dir);

struct inode_operations fat_iops = {
	.ino_create   = fat_create,
	.ino_lookup   = fat_ilookup,
	.ino_remove   = fat_delete,
	.ino_iterate  = fat_iterate,
	.ino_truncate = fat_truncate,
};

static const struct fs_driver fatfs_driver = {
	.name     = "vfat",
	.format = fat_format,
	.fill_sb  = fat_fill_sb,
	.clean_sb = fat_clean_sb,
};

DECLARE_FILE_SYSTEM_DRIVER(fatfs_driver);
