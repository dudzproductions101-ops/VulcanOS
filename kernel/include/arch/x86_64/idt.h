#ifndef VULCAN_ARCH_IDT_H
#define VULCAN_ARCH_IDT_H

#include "types.h"

#define IDT_ENTRIES 256

#define IDT_GATE_INTERRUPT 0x8E  
#define IDT_GATE_TRAP      0x8F  

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8  ist;         
    u8  type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

void idt_install(void);
void idt_set_gate(u8 vector, u64 handler, u16 selector, u8 type_attr);

#endif 
