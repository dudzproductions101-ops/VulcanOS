
#include "printk.h"
#include "drivers/console.h"

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

static void print_uint(u64 value, int base, bool uppercase)
{
    char buf[32];
    int i = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (value == 0) {
        console_putc('0');
        return;
    }

    while (value > 0) {
        buf[i++] = digits[value % (u64)base];
        value /= (u64)base;
    }

    while (i > 0) {
        console_putc(buf[--i]);
    }
}

static void print_int(i64 value)
{
    if (value < 0) {
        console_putc('-');

        print_uint((u64)(-(value + 1)) + 1, 10, false);
    } else {
        print_uint((u64)value, 10, false);
    }
}

static void vprintk(const char *fmt, va_list ap)
{
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            console_putc(*p);
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
                print_int(va_arg(ap, i64));
            } else {
                print_int((i64)va_arg(ap, int));
            }
            break;
        case 'u':
            if (is_long_long) {
                print_uint(va_arg(ap, u64), 10, false);
            } else {
                print_uint((u64)va_arg(ap, unsigned int), 10, false);
            }
            break;
        case 'x':
            if (is_long_long) {
                print_uint(va_arg(ap, u64), 16, false);
            } else {
                print_uint((u64)va_arg(ap, unsigned int), 16, false);
            }
            break;
        case 'X':
            if (is_long_long) {
                print_uint(va_arg(ap, u64), 16, true);
            } else {
                print_uint((u64)va_arg(ap, unsigned int), 16, true);
            }
            break;
        case 'p':
            console_write("0x");
            print_uint((u64)(uptr)va_arg(ap, void *), 16, false);
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            console_write(s ? s : "(null)");
            break;
        }
        case 'c':
            console_putc((char)va_arg(ap, int));
            break;
        case '%':
            console_putc('%');
            break;
        default:

            console_putc('%');
            console_putc(*p);
            break;
        }
    }
}

void printk(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintk(fmt, ap);
    va_end(ap);
}

void printk_level(enum log_level level, const char *fmt, ...)
{
    static const char *prefixes[] = {
        "[debug] ", "[info]  ", "[warn]  ", "[error] ",
    };

    console_write(prefixes[level]);

    va_list ap;
    va_start(ap, fmt);
    vprintk(fmt, ap);
    va_end(ap);
}
