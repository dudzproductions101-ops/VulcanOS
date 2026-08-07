#ifndef VULCAN_ARCH_INTERRUPTS_H
#define VULCAN_ARCH_INTERRUPTS_H

#include "types.h"

#define VEC_DIVIDE_ERROR        0
#define VEC_DEBUG                1
#define VEC_NMI                  2
#define VEC_BREAKPOINT            3
#define VEC_OVERFLOW              4
#define VEC_BOUND_RANGE           5
#define VEC_INVALID_OPCODE        6
#define VEC_DEVICE_NOT_AVAIL      7
#define VEC_DOUBLE_FAULT          8
#define VEC_INVALID_TSS          10
#define VEC_SEGMENT_NOT_PRESENT  11
#define VEC_STACK_FAULT          12
#define VEC_GENERAL_PROTECTION   13
#define VEC_PAGE_FAULT           14
#define VEC_FPU_ERROR            16
#define VEC_ALIGNMENT_CHECK      17
#define VEC_MACHINE_CHECK        18
#define VEC_SIMD_FP              19

#define IRQ_BASE       32
#define IRQ_TIMER      (IRQ_BASE + 0)
#define IRQ_KEYBOARD   (IRQ_BASE + 1)
#define IRQ_CASCADE    (IRQ_BASE + 2)  
#define IRQ_RTC        (IRQ_BASE + 8)

struct interrupt_frame {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 vector;
    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};

typedef void (*isr_handler_t)(struct interrupt_frame *frame);

void interrupts_install(void);
void isr_dispatch(struct interrupt_frame *frame);
void irq_register_handler(u8 irq, isr_handler_t handler);
void irq_send_eoi(u8 irq);

#endif 
