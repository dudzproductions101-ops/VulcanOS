#include "fs/inode.h"

bool inode_has_permission(const struct inode *node, u32 required)
{
    return (node->permissions & required) == required;
}

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
