



























#ifndef VULCAN_FS_VULCANFS_H
#define VULCAN_FS_VULCANFS_H

#include "fs/inode.h"






struct inode *vulcanfs_create(void);

enum vulcanfs_device_kind {
    VULCANFS_DEVICE_NONE,
    VULCANFS_DEVICE_FRAMEBUFFER,
    VULCANFS_DEVICE_BLOCK,
};

struct inode *vulcanfs_create_device(struct inode *dir, const char *name,
                                     enum vulcanfs_device_kind kind);

#endif 
