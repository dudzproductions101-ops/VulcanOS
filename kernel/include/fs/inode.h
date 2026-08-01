
#ifndef VULCAN_FS_INODE_H
#define VULCAN_FS_INODE_H

#include "types.h"

#define VULCAN_FILENAME_MAX 64

enum inode_type {
    INODE_FILE,
    INODE_DIRECTORY,
    INODE_DEVICE,

};

#define VULCAN_PERM_READ    (1 << 0)
#define VULCAN_PERM_WRITE   (1 << 1)
#define VULCAN_PERM_EXEC    (1 << 2)

struct inode;
struct dirent;

struct inode_ops {

    isize (*read)(struct inode *node, void *buf, usize size, usize offset);

    isize (*write)(struct inode *node, const void *buf, usize size, usize offset);

    struct inode *(*lookup)(struct inode *dir, const char *name);

    struct inode *(*create)(struct inode *dir, const char *name, enum inode_type type);

    bool (*unlink)(struct inode *dir, const char *name);

    bool (*readdir)(struct inode *dir, usize index, struct dirent *out);
};

struct inode {
    u64 inode_number;
    enum inode_type type;
    u32 permissions;
    usize size;

    const struct inode_ops *ops;

    void *private_data;

    u32 refcount;

};

struct dirent {
    char name[VULCAN_FILENAME_MAX];
    u64 inode_number;
    enum inode_type type;
};

bool inode_has_permission(const struct inode *node, u32 required);
usize inode_count_children(struct inode *dir);

#endif
