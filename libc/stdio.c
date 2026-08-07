#include "stdio.h"
#include "string.h"
#include "fs/vfs.h"
#include "drivers/console.h"

#define VULCAN_FD_OFFSET 3

static char current_working_directory[VULCAN_PATH_MAX] = "/";
#define MAX_CWD_COMPONENTS 64

static usize str_len(const char *s)
{
    usize n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static bool canonicalize_absolute_path(const char *path, char *out)
{
    char components[MAX_CWD_COMPONENTS][VULCAN_FILENAME_MAX];
    usize comp_count = 0;
    usize i = 0;

    if (path[0] != '/') {
        return false;
    }

    while (path[i]) {
        while (path[i] == '/') {
            i++;
        }
        if (!path[i]) {
            break;
        }

        usize comp_len = 0;
        while (path[i] && path[i] != '/') {
            if (comp_len >= VULCAN_FILENAME_MAX - 1) {
                return false;
            }
            components[comp_count][comp_len++] = path[i++];
        }
        components[comp_count][comp_len] = '\0';

        if (comp_len == 0) {
            continue;
        }
        if (comp_len == 1 && components[comp_count][0] == '.') {
            continue;
        }
        if (comp_len == 2 && components[comp_count][0] == '.' &&
            components[comp_count][1] == '.') {
            if (comp_count > 0) {
                comp_count--;
            }
            continue;
        }

        if (comp_count >= MAX_CWD_COMPONENTS) {
            return false;
        }
        comp_count++;
    }

    if (comp_count == 0) {
        out[0] = '/';
        out[1] = '\0';
        return true;
    }

    usize pos = 0;
    for (usize j = 0; j < comp_count; j++) {
        out[pos++] = '/';
        for (usize k = 0; components[j][k]; k++) {
            if (pos >= VULCAN_PATH_MAX - 1) {
                return false;
            }
            out[pos++] = components[j][k];
        }
    }
    out[pos] = '\0';
    return true;
}

static bool resolve_path(const char *path, char *out)
{
    if (path[0] == '/') {
        return canonicalize_absolute_path(path, out);
    }

    char joined[VULCAN_PATH_MAX];
    usize cwd_len = str_len(current_working_directory);
    usize path_len = str_len(path);

    if (cwd_len == 1 && current_working_directory[0] == '/') {
        if (path_len + 1 >= VULCAN_PATH_MAX) {
            return false;
        }
        joined[0] = '/';
        for (usize i = 0; i <= path_len; i++) {
            joined[1 + i] = path[i];
        }
    } else {
        if (cwd_len + 1 + path_len >= VULCAN_PATH_MAX) {
            return false;
        }
        for (usize i = 0; i < cwd_len; i++) {
            joined[i] = current_working_directory[i];
        }
        joined[cwd_len] = '/';
        for (usize i = 0; i <= path_len; i++) {
            joined[cwd_len + 1 + i] = path[i];
        }
    }

    return canonicalize_absolute_path(joined, out);
}

int open(const char *path, int flags)
{
    (void)flags; 

    char abs_path[VULCAN_PATH_MAX];
    if (!resolve_path(path, abs_path)) {
        return -1;
    }

    struct inode *existing = vfs_resolve(abs_path);
    if (!existing && (flags & VULCAN_O_CREATE)) {
        if (!vfs_create(abs_path, INODE_FILE)) {
            return -1;
        }
    }

    vulcan_fd_t fd = vfs_open(abs_path);
    if (fd == VULCAN_FD_INVALID) {
        return -1;
    }
    return (int)fd + VULCAN_FD_OFFSET;
}

void close(int fd)
{
    if (fd == VULCAN_STDIN || fd == VULCAN_STDOUT || fd == VULCAN_STDERR) {
        return; 
    }
    if (fd < VULCAN_FD_OFFSET) {
        return; 
    }
    vfs_close((vulcan_fd_t)(fd - VULCAN_FD_OFFSET));
}

int chdir(const char *path)
{
    char abs_path[VULCAN_PATH_MAX];
    if (!resolve_path(path, abs_path)) {
        return -1;
    }

    struct inode *node = vfs_resolve(abs_path);
    if (!node || node->type != INODE_DIRECTORY) {
        return -1;
    }

    usize len = str_len(abs_path);
    for (usize i = 0; i <= len; i++) {
        current_working_directory[i] = abs_path[i];
    }
    return 0;
}

char *getcwd(char *buf, usize size)
{
    usize len = str_len(current_working_directory);
    if (size <= len) {
        return NULL;
    }
    for (usize i = 0; i <= len; i++) {
        buf[i] = current_working_directory[i];
    }
    return buf;
}

int mkdir(const char *path)
{
    char abs_path[VULCAN_PATH_MAX];
    if (!resolve_path(path, abs_path)) {
        return -1;
    }
    if (!vfs_create(abs_path, INODE_DIRECTORY)) {
        return -1;
    }
    return 0;
}

int unlink(const char *path)
{
    char abs_path[VULCAN_PATH_MAX];
    if (!resolve_path(path, abs_path)) {
        return -1;
    }

    struct inode *node = vfs_resolve(abs_path);
    if (!node || node->type == INODE_DIRECTORY) {
        return -1;
    }

    if (!vfs_unlink(abs_path)) {
        return -1;
    }
    return 0;
}

int rmdir(const char *path)
{
    char abs_path[VULCAN_PATH_MAX];
    if (!resolve_path(path, abs_path)) {
        return -1;
    }

    struct inode *node = vfs_resolve(abs_path);
    if (!node || node->type != INODE_DIRECTORY) {
        return -1;
    }

    if (!vfs_unlink(abs_path)) {
        return -1;
    }
    return 0;
}

ssize_t read(int fd, void *buf, usize count)
{
    if (fd == VULCAN_STDIN) {
        extern char keyboard_read(void);
        u8 *out = buf;
        usize got = 0;

        while (got < count) {
            char c = keyboard_read();
            if (c == 0) {
                break;
            }
            out[got++] = (u8)c;
        }
        return (ssize_t)got;
    }

    if (fd == VULCAN_STDOUT || fd == VULCAN_STDERR) {
        return -1; 
    }

    if (fd < VULCAN_FD_OFFSET) {
        return -1; 
    }

    return vfs_read((vulcan_fd_t)(fd - VULCAN_FD_OFFSET), buf, count);
}

ssize_t write(int fd, const void *buf, usize count)
{
    if (fd == VULCAN_STDOUT || fd == VULCAN_STDERR) {
        console_write_n(buf, count);
        return (ssize_t)count;
    }

    if (fd == VULCAN_STDIN) {
        return -1; 
    }

    if (fd < VULCAN_FD_OFFSET) {
        return -1; 
    }

    return vfs_write((vulcan_fd_t)(fd - VULCAN_FD_OFFSET), buf, count);
}

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

#define PRINTF_BUF_MAX 512

struct printf_sink {
    char buf[PRINTF_BUF_MAX];
    usize len;
};

static void sink_putc(struct printf_sink *sink, char c)
{
    if (sink->len < PRINTF_BUF_MAX - 1) {
        sink->buf[sink->len++] = c;
    }

}

static void sink_write_uint(struct printf_sink *sink, u64 value, int base, bool uppercase)
{
    char tmp[32];
    int i = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (value == 0) {
        sink_putc(sink, '0');
        return;
    }

    while (value > 0) {
        tmp[i++] = digits[value % (u64)base];
        value /= (u64)base;
    }
    while (i > 0) {
        sink_putc(sink, tmp[--i]);
    }
}

static void sink_write_int(struct printf_sink *sink, i64 value)
{
    if (value < 0) {
        sink_putc(sink, '-');
        sink_write_uint(sink, (u64)(-(value + 1)) + 1, 10, false);
    } else {
        sink_write_uint(sink, (u64)value, 10, false);
    }
}

static void sink_write_str(struct printf_sink *sink, const char *s)
{
    if (!s) {
        s = "(null)";
    }
    while (*s) {
        sink_putc(sink, *s++);
    }
}

int printf(const char *fmt, ...)
{
    struct printf_sink sink;
    sink.len = 0;

    va_list ap;
    va_start(ap, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            sink_putc(&sink, *p);
            continue;
        }

        p++;
        bool is_long_long = false;
        if (p[0] == 'l' && p[1] == 'l') {
            is_long_long = true;
            p += 2;
        }

        switch (*p) {
        case 'd':
        case 'i':
            if (is_long_long) {
                sink_write_int(&sink, va_arg(ap, i64));
            } else {
                sink_write_int(&sink, (i64)va_arg(ap, int));
            }
            break;
        case 'u':
            if (is_long_long) {
                sink_write_uint(&sink, va_arg(ap, u64), 10, false);
            } else {
                sink_write_uint(&sink, (u64)va_arg(ap, unsigned int), 10, false);
            }
            break;
        case 'x':
            if (is_long_long) {
                sink_write_uint(&sink, va_arg(ap, u64), 16, false);
            } else {
                sink_write_uint(&sink, (u64)va_arg(ap, unsigned int), 16, false);
            }
            break;
        case 'p':
            sink_write_str(&sink, "0x");
            sink_write_uint(&sink, (u64)(uptr)va_arg(ap, void *), 16, false);
            break;
        case 's':
            sink_write_str(&sink, va_arg(ap, const char *));
            break;
        case 'c':
            sink_putc(&sink, (char)va_arg(ap, int));
            break;
        case '%':
            sink_putc(&sink, '%');
            break;
        default:
            sink_putc(&sink, '%');
            sink_putc(&sink, *p);
            break;
        }
    }

    va_end(ap);

    write(VULCAN_STDOUT, sink.buf, sink.len);
    return (int)sink.len;
}

int vulcan_readdir(const char *path, unsigned long index, struct vulcan_dirent *out)
{
    struct dirent kernel_entry;

    if (!vfs_readdir(path, (usize)index, &kernel_entry)) {
        return 0;
    }

    usize i = 0;
    for (; i < sizeof(out->name) - 1 && kernel_entry.name[i]; i++) {
        out->name[i] = kernel_entry.name[i];
    }
    out->name[i] = '\0';
    out->is_directory = (kernel_entry.type == INODE_DIRECTORY) ? 1 : 0;

    return 1;
}
