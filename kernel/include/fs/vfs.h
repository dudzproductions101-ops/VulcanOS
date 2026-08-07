


















#ifndef VULCAN_FS_VFS_H
#define VULCAN_FS_VFS_H

#include "types.h"
#include "fs/inode.h"

#define VULCAN_PATH_MAX 256
#define VULCAN_MAX_MOUNTS 8
#define VULCAN_MAX_OPEN_FILES 64








#define VULCAN_FS_OK             0
#define VULCAN_FS_ERR_NOT_FOUND  (-1)
#define VULCAN_FS_ERR_EXISTS     (-2)
#define VULCAN_FS_ERR_NOT_DIR    (-3)
#define VULCAN_FS_ERR_IS_DIR     (-4)
#define VULCAN_FS_ERR_NO_SPACE   (-5)
#define VULCAN_FS_ERR_BAD_FD     (-6)
#define VULCAN_FS_ERR_TOO_MANY_FDS (-7)
#define VULCAN_FS_ERR_NOT_SUPPORTED (-8)
#define VULCAN_FS_ERR_INVALID_PATH (-9)

struct mount {
    char prefix[VULCAN_PATH_MAX];   





    struct inode *root;              
    bool in_use;
};







struct open_file {
    struct inode *node;
    usize offset;
    bool in_use;
};








typedef int vulcan_fd_t;

#define VULCAN_FD_INVALID (-1)

void vfs_init(void);




bool vfs_mount(const char *prefix, struct inode *root);







struct inode *vfs_resolve(const char *path);





vulcan_fd_t vfs_open(const char *path);







void vfs_close(vulcan_fd_t fd);

isize vfs_read(vulcan_fd_t fd, void *buf, usize size);
isize vfs_write(vulcan_fd_t fd, const void *buf, usize size);







bool vfs_create(const char *path, enum inode_type type);

bool vfs_unlink(const char *path);










bool vfs_readdir(const char *path, usize index, struct dirent *out);

#endif 
