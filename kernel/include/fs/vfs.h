/*
 * vfs.h - VulcanOS Virtual Filesystem layer
 *
 * The VFS owns three things: the mount table (which filesystem
 * backend is responsible for which path prefix), path resolution
 * (walking a string like "/vulcan/bin/vulsh" down through
 * directory inodes to the target), and the open-file-descriptor
 * abstraction that userland-facing code (and, once syscalls exist,
 * actual user processes) uses instead of touching inodes directly.
 *
 * This bring-up milestone ships exactly one mount: a single
 * vulcanfs instance (fs/vulcanfs.c) mounted at "/". Multiple
 * simultaneous mounts (e.g. a future real disk filesystem mounted
 * under /media) are supported by the mount-table design below even
 * though only one entry is ever populated right now -- designing
 * the table to support N mounts from the start avoids a structural
 * rewrite later for what is a very likely near-term need.
 */

#ifndef VULCAN_FS_VFS_H
#define VULCAN_FS_VFS_H

#include "types.h"
#include "fs/inode.h"

#define VULCAN_PATH_MAX 256
#define VULCAN_MAX_MOUNTS 8
#define VULCAN_MAX_OPEN_FILES 64

/* Negative return values used throughout the VFS/inode_ops contract
 * in place of errno -- VulcanOS has no errno global yet (that's a
 * libc concept; the kernel-internal contract here is simpler:
 * negative means "failed, and this specific negative value says
 * why"). Chosen as small negative ints so a caller can safely check
 * `result < 0` without needing to know the specific constant, while
 * still having a real value to log/report when it matters. */
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
    char prefix[VULCAN_PATH_MAX];   /* path prefix this mount is responsible
                                      * for, e.g. "/" -- matched as the
                                      * LONGEST matching prefix across all
                                      * active mounts, so a future "/media"
                                      * mount correctly takes precedence over
                                      * the root mount for paths under it */
    struct inode *root;              /* this filesystem's root inode */
    bool in_use;
};

/* An open file descriptor: an inode plus a cursor position. Kept
 * deliberately separate from struct inode itself (rather than
 * storing the offset on the inode) because the same inode can be
 * open multiple times simultaneously, at different offsets, via
 * different file descriptors -- exactly the standard Unix
 * open-file-table semantics. */
struct open_file {
    struct inode *node;
    usize offset;
    bool in_use;
};

/* An opaque handle type distinct from a raw array index, even
 * though the current implementation IS just an array index -- kept
 * as its own type so a future change to how file descriptors are
 * represented (e.g. per-process fd tables, once real processes with
 * their own fd namespaces exist) doesn't require changing every
 * call site's parameter type, just this typedef and the functions
 * that use it. */
typedef int vulcan_fd_t;

#define VULCAN_FD_INVALID (-1)

void vfs_init(void);

/* Mounts filesystem `root` (its already-constructed root inode) at
 * `prefix`. Returns true on success, false if the mount table is
 * full or `prefix` is already mounted. */
bool vfs_mount(const char *prefix, struct inode *root);

/* Resolves `path` to its inode by walking the mount table and then
 * the matched filesystem's directory structure component by
 * component. Returns NULL if any component doesn't exist. Does not
 * open a file descriptor -- see vfs_open for that; this is the
 * lower-level primitive vfs_open (and directory-walking code like
 * vfs_mkdir) builds on. */
struct inode *vfs_resolve(const char *path);

/* Opens `path`, returning a file descriptor, or VULCAN_FD_INVALID on
 * failure (path doesn't exist, or the open-file table is full).
 * Increments the resolved inode's refcount -- see struct inode's
 * refcount field. */
vulcan_fd_t vfs_open(const char *path);

/* Closes `fd`, decrementing the underlying inode's refcount.
 * Does NOT free the inode even at refcount 0 -- vulcanfs (and any
 * future backend) keeps its own nodes alive independent of the VFS
 * layer's open-file bookkeeping; refcount 0 just means "no file
 * descriptor currently references this," not "this node no longer
 * exists." */
void vfs_close(vulcan_fd_t fd);

isize vfs_read(vulcan_fd_t fd, void *buf, usize size);
isize vfs_write(vulcan_fd_t fd, const void *buf, usize size);

/* Creates a new file or directory at `path`. The parent directory
 * of `path` must already exist and be resolvable; only the final
 * path component is created (no implicit "create intermediate
 * directories" behavior -- that's a userland `mkdir -p` policy, not
 * a VFS primitive, matching the same non-recursive philosophy as
 * inode_ops.unlink). */
bool vfs_create(const char *path, enum inode_type type);

bool vfs_unlink(const char *path);

/* Fills `out` with the `index`-th (0-based) directory entry of the
 * directory at `path`. Returns false once `index` is past the last
 * entry (the caller's iteration-done signal, matching
 * inode_ops.readdir's own contract in inode.h) or if `path` doesn't
 * resolve to a directory at all. This is the helper inode.h's own
 * readdir-related comment already referenced -- added here to
 * actually provide it, closing a real gap between that comment and
 * what existed, found while building the `ls` utility
 * (user/bin/ls.c), which is this function's first real consumer. */
bool vfs_readdir(const char *path, usize index, struct dirent *out);

#endif /* VULCAN_FS_VFS_H */
