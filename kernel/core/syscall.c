#include "syscall.h"
#include "drivers/display.h"
#include "drivers/keyboard.h"
#include "proc/scheduler.h"
#include "proc/process.h"
#include "proc/ring3.h"
#include "arch/x86_64/cpu.h"
#include "printk.h"

struct graphics_info {
    u32 width;
    u32 height;
    u32 bpp;
    u8 available;
};

static isize syscall_read(u64 fd, char *buf, u64 len)
{
    if (fd != VULCAN_STDIN || !buf || len == 0)
        return -1;

    u64 count = 0;

    while (count < len) {
        char c;

        do {
            c = keyboard_read();
        } while (c == 0);

        buf[count++] = c;

        if (c == '\n')
            break;
    }

    return (isize)count;
}

static void syscall_graphics_info(struct interrupt_frame *frame)
{
    const struct framebuffer_info *fb = display_info();
    struct graphics_info *result = (struct graphics_info *)frame->rax;
    
    if (!result) {
        frame->rax = 1;
        return;
    }
    
    if (!fb || !fb->available) {
        result->available = 0;
        frame->rax = 0;
        return;
    }
    
    result->width = fb->width;
    result->height = fb->height;
    result->bpp = fb->bpp;
    result->available = 1;
    frame->rax = 0;
}

static void syscall_graphics_clear(struct interrupt_frame *frame)
{
    u32 color = frame->rdi;
    
    const struct framebuffer_info *fb = display_info();
    if (!fb || !fb->available) {
        frame->rax = 1;
        return;
    }
    
    display_clear(color);
    frame->rax = 0;
}

static void syscall_graphics_draw_rect(struct interrupt_frame *frame)
{
    u32 x = frame->rdi;
    u32 y = frame->rsi;
    u32 width = frame->rdx;
    u32 height = frame->rcx;
    u32 color = frame->r8;
    
    const struct framebuffer_info *fb = display_info();
    if (!fb || !fb->available) {
        frame->rax = 1;
        return;
    }
    
    display_draw_rect(x, y, width, height, color);
    frame->rax = 0;
}

static void syscall_exit(struct interrupt_frame *frame)
{
    i32 status = frame->rdi;
    printk_level(LOG_DEBUG, "process exiting with status %d\n", status);
    for (;;) {
        hlt();
    }
}

static void syscall_exec(struct interrupt_frame *frame)
{
    /* SYS_EXEC (syscall 20)
     * Purpose: Execute a new program in the current process
     * Arguments:
     *   RDI = entry point (pointer to function)
     * Returns:
     *   0 on success (never returns, jumps to user code)
     *   -1 on error
     *
     * This syscall sets up the process for ring-3 execution
     * and jumps to the entry point.
     */
    void (*entry_point)(void) = (void (*)(void))frame->rdi;
    struct thread *self = scheduler_current();

    if (!self || !self->owner) {
        frame->rax = (u64)-1;
        return;
    }

    struct process *p = self->owner;

    /* Set up user address space and privileges */
    ring3_setup_user_process(p, entry_point);

    /* Modify the interrupt frame to return to user mode */
    ring3_return_to_user(frame);

    /* iretq in interrupt handler will switch to ring 3 */
    printk_level(LOG_INFO, "syscall: executing process pid=%llu in ring 3\n", p->pid);
}

void syscall_handle(struct interrupt_frame *frame)
{
    u64 syscall_number = frame->rax;
    struct thread *self = scheduler_current();
    u64 ret = (u64)-1;

    switch (syscall_number) {

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

    case SYS_WRITE:
        ret = 0;
        break;

    case SYS_EXIT:
        syscall_exit(frame);
        break;

    case SYS_GRAPHICS_INFO:
        syscall_graphics_info(frame);
        return;

    case SYS_GRAPHICS_CLEAR:
        syscall_graphics_clear(frame);
        return;

    case SYS_GRAPHICS_DRAW_RECT:
        syscall_graphics_draw_rect(frame);
        return;

    case SYS_EXEC:
        syscall_exec(frame);
        return;

    default:
        printk_level(LOG_WARN, "unknown syscall: %llu\n", syscall_number);
        ret = (u64)-1;
        break;
    }

    frame->rax = ret;
}
