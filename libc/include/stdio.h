/*
 * stdio.h - VulcanOS libc standard I/O
 *
 * See stdlib.c's file comment for the full interim-design
 * explanation of why open/read/write/close here call directly into
 * VFS kernel functions rather than crossing a real syscall
 * boundary.
 *
 * VulcanOS's own printf-family implementation, independent of the
 * kernel's own printk (kernel/core/printk.c) even though both
 * currently render through the same underlying console -- kept
 * separate because a real userland/kernel split will eventually mean
 * these run in genuinely different contexts (unprivileged process
 * vs. kernel code) and should not share an implementation that
 * assumes kernel-only capabilities.
 */

#ifndef VULCAN_LIBC_STDIO_H
#define VULCAN_LIBC_STDIO_H

#include "vulcan_types.h"

#define VULCAN_STDIN  0
#define VULCAN_STDOUT 1
#define VULCAN_STDERR 2

int open(const char *path, int flags);
void close(int fd);
ssize_t read(int fd, void *buf, usize count);
ssize_t write(int fd, const void *buf, usize count);

int chdir(const char *path);
char *getcwd(char *buf, usize size);
int mkdir(const char *path);
int rmdir(const char *path);
int unlink(const char *path);

/* Open flags. A small set for this bring-up milestone -- real POSIX
 * has many more (O_APPEND, O_TRUNC, O_EXCL, ...); these are the ones
 * VulcanOS's current VFS (vfs_open/vfs_create) can actually honor.
 * Extending this set is straightforward once the VFS grows the
 * corresponding capability, not a structural change. */
#define VULCAN_O_READ    (1 << 0)
#define VULCAN_O_WRITE   (1 << 1)
#define VULCAN_O_CREATE  (1 << 2)

int printf(const char *fmt, ...);

/* Directory listing. Kept in stdio.h rather than a separate
 * dirent.h for this bring-up milestone -- three functions don't yet
 * justify their own header; splitting into a real <dirent.h> is
 * reasonable once directory-handling surface grows beyond this. */
struct vulcan_dirent {
    char name[64]; /* matches VULCAN_FILENAME_MAX, kernel/include/fs/inode.h --
                    * duplicated as a literal rather than included, since this
                    * header must stay usable without pulling in kernel
                    * headers for the common case (a program that only wants
                    * to list a directory shouldn't need fs/inode.h) */
    int is_directory;
};

/* Returns true and fills `out` with the `index`-th (0-based) entry
 * of the directory at `path`, or false once index is past the last
 * entry -- the caller's iteration-done signal, matching the
 * underlying vfs_readdir contract this wraps. No real DIR* handle
 * or opendir/closedir pairing yet (unlike POSIX's actual dirent.h
 * API) since VulcanOS's VFS has no per-directory iteration state to
 * hold open across calls -- each call re-resolves `path` fresh. A
 * real stateful DIR* handle is reasonable future work once that
 * becomes a real cost worth avoiding. */
int vulcan_readdir(const char *path, unsigned long index, struct vulcan_dirent *out);

#endif /* VULCAN_LIBC_STDIO_H */
