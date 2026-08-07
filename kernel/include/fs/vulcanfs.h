/*
 * vulcanfs.h - VulcanOS's RAM-resident filesystem backend
 *
 * The first concrete implementation of the inode_ops vtable
 * (inode.h). Not a placeholder or a toy: this is a real, complete,
 * fully-functional filesystem -- it simply keeps every file's data
 * in kernel heap memory rather than on a persistent block device,
 * because VulcanOS has no storage driver yet (see the interim-
 * decision note in PROJECT_STATUS.md). Everything vulcanfs does
 * (directories, files, read/write/create/unlink/readdir) works
 * correctly and is boot-verified; what's missing is persistence
 * across a reboot, which is a property of WHERE the bytes live, not
 * of vulcanfs's own logic.
 *
 * When a real storage driver exists, a second, disk-backed
 * filesystem can be added behind the exact same inode_ops interface
 * without changing vfs.c or any of vulcanfs's own logic -- that's
 * the entire point of the vtable design in inode.h.
 *
 * Named "vulcanfs" deliberately, not "ramfs" or "tmpfs": those
 * names describe an implementation detail (where bytes live) that
 * is explicitly NOT meant to be a permanent identity -- vulcanfs is
 * intended to grow a real persistent mode later while keeping its
 * name and on-disk-equivalent structure, the same way a filesystem
 * name doesn't usually change just because its backing storage
 * does.
 */

#ifndef VULCAN_FS_VULCANFS_H
#define VULCAN_FS_VULCANFS_H

#include "fs/inode.h"

/* Creates a fresh vulcanfs instance and returns its root directory
 * inode, ready to be passed to vfs_mount(). Each call creates an
 * independent, empty filesystem -- vulcanfs does not (yet) support
 * being instantiated from existing saved state, since there is no
 * persistent storage to load from at this bring-up milestone. */
struct inode *vulcanfs_create(void);

enum vulcanfs_device_kind {
    VULCANFS_DEVICE_NONE,
    VULCANFS_DEVICE_FRAMEBUFFER,
    VULCANFS_DEVICE_BLOCK,
};

struct inode *vulcanfs_create_device(struct inode *dir, const char *name,
                                     enum vulcanfs_device_kind kind);

#endif /* VULCAN_FS_VULCANFS_H */
