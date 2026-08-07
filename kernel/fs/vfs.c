





#include "fs/vfs.h"
#include "printk.h"
#include "panic.h"

static struct mount mounts[VULCAN_MAX_MOUNTS];
static struct open_file open_files[VULCAN_MAX_OPEN_FILES];

void vfs_init(void)
{
    for (int i = 0; i < VULCAN_MAX_MOUNTS; i++) {
        mounts[i].in_use = false;
    }
    for (int i = 0; i < VULCAN_MAX_OPEN_FILES; i++) {
        open_files[i].in_use = false;
    }
    printk_level(LOG_INFO, "vfs: initialized (%d mount slots, %d fd slots)\n",
                 VULCAN_MAX_MOUNTS, VULCAN_MAX_OPEN_FILES);
}

static usize str_len(const char *s)
{
    usize n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

bool vfs_mount(const char *prefix, struct inode *root)
{
    for (int i = 0; i < VULCAN_MAX_MOUNTS; i++) {
        if (mounts[i].in_use && mounts[i].prefix[0] == prefix[0]) {
            




            usize i_len = str_len(mounts[i].prefix);
            usize p_len = str_len(prefix);
            if (i_len == p_len) {
                bool same = true;
                for (usize j = 0; j < i_len; j++) {
                    if (mounts[i].prefix[j] != prefix[j]) {
                        same = false;
                        break;
                    }
                }
                if (same) {
                    printk_level(LOG_WARN, "vfs: \"%s\" is already mounted\n", prefix);
                    return false;
                }
            }
        }
    }

    for (int i = 0; i < VULCAN_MAX_MOUNTS; i++) {
        if (!mounts[i].in_use) {
            usize len = str_len(prefix);
            if (len >= VULCAN_PATH_MAX) {
                printk_level(LOG_ERROR, "vfs: mount prefix too long\n");
                return false;
            }
            for (usize j = 0; j <= len; j++) {
                mounts[i].prefix[j] = prefix[j]; 
            }
            mounts[i].root = root;
            mounts[i].in_use = true;
            printk_level(LOG_INFO, "vfs: mounted at \"%s\" (root inode=%llu)\n",
                         prefix, root->inode_number);
            return true;
        }
    }

    printk_level(LOG_ERROR, "vfs: mount table full\n");
    return false;
}




static struct mount *find_mount_for_path(const char *path)
{
    struct mount *best = NULL;
    usize best_len = 0;

    for (int i = 0; i < VULCAN_MAX_MOUNTS; i++) {
        if (!mounts[i].in_use) {
            continue;
        }

        usize prefix_len = str_len(mounts[i].prefix);
        bool matches = true;
        for (usize j = 0; j < prefix_len; j++) {
            if (path[j] != mounts[i].prefix[j]) {
                matches = false;
                break;
            }
        }

        if (matches && prefix_len > best_len) {
            best = &mounts[i];
            best_len = prefix_len;
        }
    }

    return best;
}

struct inode *vfs_resolve(const char *path)
{
    if (path[0] != '/') {
        return NULL; 




    }

    struct mount *m = find_mount_for_path(path);
    if (!m) {
        return NULL;
    }

    struct inode *current = m->root;

    




    usize prefix_len = str_len(m->prefix);
    const char *remainder = path + prefix_len;

    char component[VULCAN_FILENAME_MAX];
    usize comp_len = 0;

    for (const char *p = remainder; ; p++) {
        if (*p == '/' || *p == '\0') {
            if (comp_len > 0) {
                component[comp_len] = '\0';

                if (current->type != INODE_DIRECTORY || !current->ops->lookup) {
                    return NULL;
                }

                current = current->ops->lookup(current, component);
                if (!current) {
                    return NULL;
                }

                comp_len = 0;
            }

            if (*p == '\0') {
                break;
            }
        } else {
            if (comp_len >= VULCAN_FILENAME_MAX - 1) {
                return NULL; 
            }
            component[comp_len++] = *p;
        }
    }

    return current;
}

vulcan_fd_t vfs_open(const char *path)
{
    struct inode *node = vfs_resolve(path);
    if (!node) {
        return VULCAN_FD_INVALID;
    }

    for (int i = 0; i < VULCAN_MAX_OPEN_FILES; i++) {
        if (!open_files[i].in_use) {
            open_files[i].node = node;
            open_files[i].offset = 0;
            open_files[i].in_use = true;
            node->refcount++;
            return i;
        }
    }

    printk_level(LOG_WARN, "vfs: open-file table full\n");
    return VULCAN_FD_INVALID;
}

static bool fd_valid(vulcan_fd_t fd)
{
    return fd >= 0 && fd < VULCAN_MAX_OPEN_FILES && open_files[fd].in_use;
}

void vfs_close(vulcan_fd_t fd)
{
    if (!fd_valid(fd)) {
        return;
    }

    if (open_files[fd].node->refcount > 0) {
        open_files[fd].node->refcount--;
    }
    open_files[fd].in_use = false;
}

isize vfs_read(vulcan_fd_t fd, void *buf, usize size)
{
    if (!fd_valid(fd)) {
        return VULCAN_FS_ERR_BAD_FD;
    }

    struct open_file *f = &open_files[fd];
    if (!f->node->ops->read) {
        return VULCAN_FS_ERR_NOT_SUPPORTED;
    }

    isize result = f->node->ops->read(f->node, buf, size, f->offset);
    if (result > 0) {
        f->offset += (usize)result;
    }
    return result;
}

isize vfs_write(vulcan_fd_t fd, const void *buf, usize size)
{
    if (!fd_valid(fd)) {
        return VULCAN_FS_ERR_BAD_FD;
    }

    struct open_file *f = &open_files[fd];
    if (!f->node->ops->write) {
        return VULCAN_FS_ERR_NOT_SUPPORTED;
    }

    isize result = f->node->ops->write(f->node, buf, size, f->offset);
    if (result > 0) {
        f->offset += (usize)result;
    }
    return result;
}





static bool split_parent_and_name(const char *path, char *parent_out, char *name_out)
{
    usize len = str_len(path);
    if (len == 0 || len >= VULCAN_PATH_MAX) {
        return false;
    }

    isize last_slash = -1;
    for (usize i = 0; i < len; i++) {
        if (path[i] == '/') {
            last_slash = (isize)i;
        }
    }

    if (last_slash < 0) {
        return false; 
    }

    usize name_len = len - (usize)last_slash - 1;
    if (name_len == 0 || name_len >= VULCAN_FILENAME_MAX) {
        return false; 
    }

    if (last_slash == 0) {
        parent_out[0] = '/';
        parent_out[1] = '\0';
    } else {
        for (isize i = 0; i < last_slash; i++) {
            parent_out[i] = path[i];
        }
        parent_out[last_slash] = '\0';
    }

    for (usize i = 0; i < name_len; i++) {
        name_out[i] = path[(usize)last_slash + 1 + i];
    }
    name_out[name_len] = '\0';

    return true;
}

bool vfs_create(const char *path, enum inode_type type)
{
    char parent_path[VULCAN_PATH_MAX];
    char name[VULCAN_FILENAME_MAX];

    if (!split_parent_and_name(path, parent_path, name)) {
        return false;
    }

    struct inode *parent = vfs_resolve(parent_path);
    if (!parent || parent->type != INODE_DIRECTORY || !parent->ops->create) {
        return false;
    }

    return parent->ops->create(parent, name, type) != NULL;
}

bool vfs_unlink(const char *path)
{
    char parent_path[VULCAN_PATH_MAX];
    char name[VULCAN_FILENAME_MAX];

    if (!split_parent_and_name(path, parent_path, name)) {
        return false;
    }

    struct inode *parent = vfs_resolve(parent_path);
    if (!parent || parent->type != INODE_DIRECTORY || !parent->ops->unlink) {
        return false;
    }

    return parent->ops->unlink(parent, name);
}

bool vfs_readdir(const char *path, usize index, struct dirent *out)
{
    struct inode *dir = vfs_resolve(path);
    if (!dir || dir->type != INODE_DIRECTORY || !dir->ops->readdir) {
        return false;
    }

    return dir->ops->readdir(dir, index, out);
}
