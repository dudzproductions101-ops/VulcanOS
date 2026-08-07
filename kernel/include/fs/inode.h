/*
 * inode.h - VulcanOS's in-memory filesystem node
 *
 * An inode represents one file, directory, or device node, backed
 * by whichever filesystem implementation actually owns it. Every
 * filesystem type (this bring-up milestone ships exactly one:
 * vulcanfs, a RAM-resident backend -- see fs/vulcanfs.c) implements
 * the same `struct inode_ops` vtable, so the VFS layer (vfs.h/c)
 * never needs to know which concrete filesystem it's talking to.
 *
 * This function-pointer-vtable-on-a-common-node-type shape is the
 * same pattern Linux's inode/file_operations and BSD's vnode use --
 * not because VulcanOS is copying either, but because it is simply
 * the correct answer to "many filesystem types behind one interface"
 * on any Unix-like system. What's original to VulcanOS is the
 * hierarchy this sits underneath (see docs/FILESYSTEM_HIERARCHY.md),
 * the naming, and vulcanfs's own on-disk-equivalent layout.
 */

#ifndef VULCAN_FS_INODE_H
#define VULCAN_FS_INODE_H

#include "types.h"

#define VULCAN_FILENAME_MAX 64

enum inode_type {
    INODE_FILE,
    INODE_DIRECTORY,
    INODE_DEVICE,        /* a /devices node; see drivers eventually
                           * registering device inodes here */
};

/* Permission bits. Deliberately a small, flat rwx-for-owner set for
 * this bring-up milestone rather than a full owner/group/other
 * matrix -- VulcanOS has no real multi-user login yet (see
 * docs/FILESYSTEM_HIERARCHY.md's "what this document does not yet
 * cover"), so designing a full permission matrix now would be
 * guessing at requirements nothing exercises. Extending this to
 * owner/group/other is expected future work once there's a real
 * user-identity system to check permissions against. */
#define VULCAN_PERM_READ    (1 << 0)
#define VULCAN_PERM_WRITE   (1 << 1)
#define VULCAN_PERM_EXEC    (1 << 2)

struct inode;
struct dirent;

/* Every filesystem backend fills in the operations it supports;
 * unsupported operations are left NULL and the VFS layer checks
 * before calling (see vfs.h's error-handling contract). A
 * directory-only backend, for instance, may leave `write` NULL. */
struct inode_ops {
    /* Reads up to `size` bytes starting at `offset` into `buf`.
     * Returns bytes actually read, 0 at end-of-file, or a negative
     * error code (see fs/vfs.h's VULCAN_FS_ERR_* constants). */
    isize (*read)(struct inode *node, void *buf, usize size, usize offset);

    /* Writes up to `size` bytes from `buf` starting at `offset`.
     * Returns bytes actually written or a negative error code. A
     * write past the current end of file is expected to grow the
     * file (this bring-up milestone's vulcanfs backend does), not
     * fail -- sparse-file semantics (holes) are not required. */
    isize (*write)(struct inode *node, const void *buf, usize size, usize offset);

    /* Looks up `name` as a direct child of directory node `dir`.
     * Returns the child inode, or NULL if no such child exists.
     * Only meaningful when dir->type == INODE_DIRECTORY. */
    struct inode *(*lookup)(struct inode *dir, const char *name);

    /* Creates a new child inode named `name` of type `type` inside
     * directory `dir`. Returns the new inode, or NULL on failure
     * (e.g. name already exists, or the backend is read-only). */
    struct inode *(*create)(struct inode *dir, const char *name, enum inode_type type);

    /* Removes the child named `name` from directory `dir`. Returns
     * true on success. Does not follow directories recursively --
     * removing a non-empty directory is expected to fail; recursive
     * removal is a userland (`rm -r`) policy, not a VFS primitive. */
    bool (*unlink)(struct inode *dir, const char *name);

    /* Fills `out` with the name of the `index`-th directory entry
     * (0-based) inside `dir`. Returns false once `index` is past the
     * last entry, which callers use as the iteration-done signal --
     * see fs/vfs.h's readdir helper for the typical loop shape. */
    bool (*readdir)(struct inode *dir, usize index, struct dirent *out);
};

struct inode {
    u64 inode_number;              /* unique within its owning filesystem */
    enum inode_type type;
    u32 permissions;                /* VULCAN_PERM_* bitmask */
    usize size;                     /* bytes, for INODE_FILE; entry count for
                                      * INODE_DIRECTORY is tracked by the
                                      * backend's own private_data instead */

    const struct inode_ops *ops;

    void *private_data;             /* backend-owned; vulcanfs uses this to
                                      * point at its own node structure (see
                                      * fs/vulcanfs.c) */

    u32 refcount;                    /* number of open file descriptors +
                                      * VFS-internal references currently
                                      * pointing at this inode; see vfs.h's
                                      * vfs_open/vfs_close contract */
};

struct dirent {
    char name[VULCAN_FILENAME_MAX];
    u64 inode_number;
    enum inode_type type;
};

/* Generic helpers implemented in fs/inode.c, built purely in terms
 * of the inode_ops vtable above -- correct for any backend, not
 * just vulcanfs (fs/vulcanfs.c). */
bool inode_has_permission(const struct inode *node, u32 required);
usize inode_count_children(struct inode *dir);

#endif /* VULCAN_FS_INODE_H */
