/*
 * inode.c - Generic inode helpers shared across filesystem backends
 *
 * Deliberately does NOT contain vulcanfs's (or any future backend's)
 * own inode allocation/lifecycle logic -- that stays backend-owned
 * in fs/vulcanfs.c, since only the backend knows how to correctly
 * free its own private_data. This file holds only the small set of
 * generic conveniences built purely in terms of the inode_ops
 * vtable, usable identically regardless of which backend an inode
 * belongs to.
 */

#include "fs/inode.h"

bool inode_has_permission(const struct inode *node, u32 required)
{
    return (node->permissions & required) == required;
}

/* Counts a directory's entries by walking readdir until it returns
 * false, per inode_ops.readdir's index-based iteration contract
 * (inode.h). A generic convenience built purely on the vtable --
 * correct for any backend, not just vulcanfs. */
usize inode_count_children(struct inode *dir)
{
    if (dir->type != INODE_DIRECTORY || !dir->ops->readdir) {
        return 0;
    }

    usize count = 0;
    struct dirent tmp;
    while (dir->ops->readdir(dir, count, &tmp)) {
        count++;
    }
    return count;
}
