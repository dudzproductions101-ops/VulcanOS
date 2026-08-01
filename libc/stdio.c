
#include "stdio.h"
#include "string.h"
#include "fs/vfs.h"
#include "drivers/console.h"

#define VULCAN_FD_OFFSET 3

int open(const char *path, int flags)
{
    (void)flags;

    struct inode *existing = vfs_resolve(path);
    if (!existing && (flags & VULCAN_O_CREATE)) {
        if (!vfs_create(path, INODE_FILE)) {
            return -1;
        }
    }

    vulcan_fd_t fd = vfs_open(path);
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
