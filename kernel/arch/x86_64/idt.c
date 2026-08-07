#include "arch/x86_64/idt.h"

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr ip;

extern void idt_flush(u64 idt_ptr_addr);

void idt_set_gate(u8 vector, u64 handler, u16 selector, u8 type_attr)
{
    idt[vector].offset_low  = (u16)(handler & 0xFFFF);
    idt[vector].offset_mid  = (u16)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (u32)((handler >> 32) & 0xFFFFFFFF);
    idt[vector].selector    = selector;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = type_attr;
    idt[vector].reserved    = 0;
}

void idt_install(void)
{
    ip.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    ip.base  = (u64)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_flush((u64)&ip);
}
