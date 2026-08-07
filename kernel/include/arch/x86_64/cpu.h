









#ifndef VULCAN_ARCH_CPU_H
#define VULCAN_ARCH_CPU_H

#include "types.h"



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





static inline void io_wait(void)
{
    outb(0x80, 0);
}



static inline void cli(void)
{
    __asm__ volatile ("cli");
}

static inline void sti(void)
{
    __asm__ volatile ("sti");
}



static inline void hlt(void)
{
    __asm__ volatile ("hlt");
}








__attribute__((noreturn)) static inline void halt_forever(void)
{
    cli();
    for (;;) {
        hlt();
    }
}



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



struct cpu_vendor {
    char id[13]; 
};

void cpu_get_vendor(struct cpu_vendor *out);
bool cpu_supports_long_mode(void);
void cpu_print_info(void);

#endif 
