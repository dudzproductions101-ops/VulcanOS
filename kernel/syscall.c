#include "syscall.h"
#include "proc/scheduler.h"
#include "proc/process.h"
#include "printk.h"

/* Very small syscall dispatcher for initial bring-up. Syscall
 * arguments follow the AMD64 syscall convention: number in rax,
 * args in rdi, rsi, rdx, r10, r8, r9. The interrupt stub populates
 * a struct interrupt_frame so we can read/write rax/rdi/ rsi etc.
 */

void syscall_handle(struct interrupt_frame *frame)
{
    u64 num = frame->rax;
    struct thread *self = scheduler_current();
    u64 ret = (u64)-1;

    switch (num) {
    case SYS_GETPID: {
        if (self && self->owner) ret = self->owner->pid;
        else ret = 0;
        break;
    }
    case SYS_WRITE: {
        u64 fd = frame->rdi;
        const char *buf = (const char *)frame->rsi;
        u64 len = frame->rdx;
        /* For now, don't attempt to copy from user memory (no user
         * mode yet). Just log the syscall for testing and return
         * the requested length as if the write succeeded. */
        printk_level(LOG_INFO, "syscall: write fd=%llu len=%llu buf=%p\n", fd, len, buf);
        ret = len;
        break;
    }
    case SYS_EXIT: {
        int code = (int)frame->rdi;
        if (self && self->owner) {
            process_exit(self->owner, code);
        }
        ret = 0;
        break;
    }
    default:
        printk_level(LOG_WARN, "syscall: unknown number %llu\n", num);
        ret = (u64)-1;
    }

    frame->rax = ret;
}
