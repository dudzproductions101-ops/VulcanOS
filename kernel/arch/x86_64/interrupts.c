/*
 * interrupts.c - Interrupt dispatch policy
 *
 * Owns three things: remapping the legacy 8259 PIC so IRQs land on
 * vectors 32-47 instead of colliding with CPU exceptions, wiring
 * every ISR/IRQ assembly stub into the IDT, and routing each
 * incoming interrupt to either a registered IRQ handler or the
 * fault-reporting path for unhandled CPU exceptions.
 *
 * ROADMAP NOTE: the 8259 PIC is the simplest interrupt controller to
 * bring up (no ACPI/MADT parsing required) and is what every legacy-
 * compatible x86 platform still exposes at boot, which is why
 * VulcanOS starts here. Once ACPI table parsing exists, VulcanOS
 * should migrate to the I/O APIC + Local APIC, which is required
 * for any real multicore support (see the roadmap in the writeup).
 * That migration should replace this file's PIC calls, not sit
 * alongside them indefinitely.
 */

#include "arch/x86_64/interrupts.h"
#include "syscall.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/cpu.h"
#include "printk.h"
#include "panic.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define PIC_EOI   0x20

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

/* Every isrN/irqN symbol is defined in interrupts.asm. Declaring
 * them all here (rather than generating this list) is repetitive on
 * purpose: it keeps the ISR table below a single readable source of
 * truth for "which vectors exist" instead of hiding it behind
 * macro-generated symbol names a reader can't grep for. */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void irq0(void);  extern void irq1(void);
extern void isr128(void);

static isr_handler_t irq_handlers[16];

static const char *exception_names[32] = {
    "Divide-by-zero", "Debug", "Non-maskable Interrupt", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode",
    "Device Not Available", "Double Fault", "Coprocessor Segment Overrun",
    "Invalid TSS", "Segment Not Present", "Stack-Segment Fault",
    "General Protection Fault", "Page Fault", "Reserved",
    "x87 Floating-Point Exception", "Alignment Check", "Machine Check",
    "SIMD Floating-Point Exception", "Virtualization Exception",
    "Control Protection Exception", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Hypervisor Injection Exception",
    "VMM Communication Exception", "Security Exception", "Reserved",
};

static void pic_remap(void)
{
    u8 mask1 = inb(PIC1_DATA);
    u8 mask2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, IRQ_BASE);         /* master PIC vector offset */
    io_wait();
    outb(PIC2_DATA, IRQ_BASE + 8);     /* slave PIC vector offset */
    io_wait();

    outb(PIC1_DATA, 4);                /* tell master about slave on IRQ2 */
    io_wait();
    outb(PIC2_DATA, 2);                /* tell slave its cascade identity */
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, mask1);            /* restore saved masks rather than */
    outb(PIC2_DATA, mask2);            /* unmasking everything blindly */
}

void irq_send_eoi(u8 irq)
{
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

void irq_register_handler(u8 irq, isr_handler_t handler)
{
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void interrupts_install(void)
{
    idt_install();
    pic_remap();

    idt_set_gate(0,  (u64)isr0,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(1,  (u64)isr1,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(2,  (u64)isr2,  GDT_SEL_KERNEL_CODE, IDT_GATE_INTERRUPT);
    idt_set_gate(3,  (u64)isr3,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(4,  (u64)isr4,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(5,  (u64)isr5,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(6,  (u64)isr6,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(7,  (u64)isr7,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(8,  (u64)isr8,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(9,  (u64)isr9,  GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(10, (u64)isr10, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(11, (u64)isr11, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(12, (u64)isr12, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(13, (u64)isr13, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(14, (u64)isr14, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(15, (u64)isr15, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(16, (u64)isr16, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(17, (u64)isr17, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(18, (u64)isr18, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(19, (u64)isr19, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(20, (u64)isr20, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(21, (u64)isr21, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(22, (u64)isr22, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(23, (u64)isr23, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(24, (u64)isr24, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(25, (u64)isr25, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(26, (u64)isr26, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(27, (u64)isr27, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(28, (u64)isr28, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(29, (u64)isr29, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(30, (u64)isr30, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);
    idt_set_gate(31, (u64)isr31, GDT_SEL_KERNEL_CODE, IDT_GATE_TRAP);

    idt_set_gate(IRQ_TIMER,    (u64)irq0, GDT_SEL_KERNEL_CODE, IDT_GATE_INTERRUPT);
    idt_set_gate(IRQ_KEYBOARD, (u64)irq1, GDT_SEL_KERNEL_CODE, IDT_GATE_INTERRUPT);
    /* syscall trap vector: make user-callable by setting DPL=3 (0xEF) */
    idt_set_gate(128, (u64)isr128, GDT_SEL_KERNEL_CODE, 0xEF);

    sti();
}

/* Called by isr_common_stub for every vector. Vectors 0-31 are CPU
 * exceptions (currently fatal -- VulcanOS has no fault recovery yet,
 * see roadmap); vectors 32+ are hardware IRQs dispatched to whatever
 * driver registered a handler via irq_register_handler().
 *
 * EOI ORDERING IS DELIBERATE AND LOAD-BEARING: irq_send_eoi() is
 * called BEFORE the handler, not after. A handler like the timer's
 * may call into the scheduler and trigger a context switch that
 * never returns back to this function at all (context_switch's
 * `ret` lands in a completely different thread's call stack). If
 * EOI were sent after the handler call, a context-switching
 * handler would skip it entirely -- the 8259 PIC would then believe
 * that IRQ line is still "in service" and never raise it again,
 * silently freezing that IRQ forever after the very first switch.
 * This was a real bug caught during scheduler bring-up: exactly one
 * preemption occurred, then the timer appeared to stop firing
 * entirely, which is exactly this failure mode. Sending EOI first
 * means the PIC's bookkeeping is always correctly closed out
 * regardless of whether the handler beneath it ever returns. */
void isr_dispatch(struct interrupt_frame *frame)
{
    /* Special-case syscall vector first (not routed via PIC). */
    if (frame->vector == 128) {
        syscall_handle(frame);
        return;
    }

    if (frame->vector < 32) {
        printk_level(LOG_ERROR, "unhandled exception %llu (%s) error_code=0x%llx rip=0x%llx\n",
                     frame->vector, exception_names[frame->vector],
                     frame->error_code, frame->rip);
        panic("unrecoverable CPU exception");
        return;
    }

    u8 irq = (u8)(frame->vector - IRQ_BASE);

    irq_send_eoi(irq);

    if (irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](frame);
    }
}
