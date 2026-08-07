/*
 * cpu.h - Low-level x86_64 CPU primitives
 *
 * Thin wrappers around instructions that C cannot express directly
 * (port I/O, flag control, halting). Every other kernel subsystem
 * should go through these rather than embedding inline asm inline
 * in unrelated files, so that the "touches raw hardware" surface
 * stays in one place.
 */

#ifndef VULCAN_ARCH_CPU_H
#define VULCAN_ARCH_CPU_H

#include "types.h"

/* --- Port I/O --------------------------------------------------- */

static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline u8 inb(u16 port)
{
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(u16 port, u16 val)
{
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline u16 inw(u16 port)
{
    u16 ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Small delay by writing to an unused POST diagnostic port (0x80).
 * A standard trick for pacing legacy hardware (e.g. the 8259 PIC or
 * PS/2 controller) that needs a few cycles to settle between
 * successive port writes. */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

/* --- Interrupt flag control --------------------------------------*/

static inline void cli(void)
{
    __asm__ volatile ("cli");
}

static inline void sti(void)
{
    __asm__ volatile ("sti");
}

/* Halt the CPU until the next interrupt. Used by the idle loop so
 * that an idle core burns no power/cycles spinning. */
static inline void hlt(void)
{
    __asm__ volatile ("hlt");
}

/* Halt permanently, interrupts masked first. Used for unrecoverable
 * conditions (panic, double fault) where continuing execution would
 * be unsafe. Marked noreturn so callers like panic() -- itself
 * noreturn -- can have that guarantee verified by the compiler
 * instead of just asserted; without this, GCC cannot prove the
 * static inline's infinite loop never falls through, and callers
 * declared noreturn get a spurious "function does return" warning. */
__attribute__((noreturn)) static inline void halt_forever(void)
{
    cli();
    for (;;) {
        hlt();
    }
}

/* --- Control registers ------------------------------------------ */

static inline u64 read_cr2(void)
{
    u64 val;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(val));
    return val;
}

static inline u64 read_cr3(void)
{
    u64 val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void write_cr3(u64 val)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(val) : "memory");
}

/* --- CPU identification (implemented in cpu.c) ------------------- */

struct cpu_vendor {
    char id[13]; /* 12-character CPUID vendor string + NUL */
};

void cpu_get_vendor(struct cpu_vendor *out);
bool cpu_supports_long_mode(void);
void cpu_print_info(void);

#endif /* VULCAN_ARCH_CPU_H */
