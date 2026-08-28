#include "syscall.h"
#include "drivers/keyboard.h"
#include "proc/scheduler.h"
#include "proc/process.h"
#include "printk.h"

static isize syscall_read(u64 fd, char *buf, u64 len)
{
    if (fd != VULCAN_STDIN || !buf || len == 0)
        return -1;

    u64 count = 0;

    while (count < len) {
        char c;

        /* Wait for keyboard input. */
        do {
            c = keyboard_read();
        } while (c == 0);

        buf[count++] = c;

        /* Normal terminal read: return one line at newline. */
        if (c == '\n')
            break;
    }

    return (isize)count;
}

void syscall_handle(struct interrupt_frame *frame)
{
    u64 num = frame->rax;
    struct thread *self = scheduler_current();
    u64 ret = (u64)-1;

    switch (num) {

    case SYS_GETPID:
        if (self && self->owner)
            ret = self->owner->pid;
        else
            ret = 0;
        break;

    case SYS_READ:
        ret = (u64)syscall_read(
            frame->rdi,
            (char *)frame->rsi,
            frame->rdx
        );
        break;

    case SYS_WRITE: {
        u64 fd = frame->rdi;
        const char *buf = (const char *)frame->rsi;
        u64 len = frame->rdx;

        if (fd != VULCAN_STDOUT && fd != VULCAN_STDERR) {
            ret = (u64)-1;
            break;
        }

        if (!buf || len == 0) {
            ret = 0;
            break;
        }

        /*
         * Your existing printk path can be used here.
         * For now write character-by-character so arbitrary
         * user buffers don't need to be NUL terminated.
         */
        for (u64 i = 0; i < len; i++)
            printk_level(LOG_INFO, "%c", buf[i]);

        ret = len;
        break;
    }

    case SYS_EXIT:
        if (self && self->owner)
            process_exit(self->owner, (int)frame->rdi);

        ret = 0;
        break;

    default:
        printk_level(
            LOG_WARN,
            "syscall: unknown number %llu\n",
            num
        );
        ret = (u64)-1;
        break;
    }

    frame->rax = ret;
}
