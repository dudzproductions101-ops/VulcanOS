/*
 * console.h - Early VGA text-mode console driver
 *
 * VulcanOS's first-boot console targets VGA text mode (0xB8000)
 * because it needs no mode-setting, no framebuffer negotiation with
 * the bootloader, and works identically under BIOS and UEFI/CSM.
 * This is deliberately the *simplest possible* output path so
 * printk() has somewhere to write before any real driver framework
 * exists. A framebuffer/graphics console arrives once VulcanOS's
 * driver model (Phase 2) is in place; this module is expected to be
 * superseded, not extended indefinitely.
 */

#ifndef VULCAN_DRIVERS_CONSOLE_H
#define VULCAN_DRIVERS_CONSOLE_H

#include "types.h"

/* VGA text-mode standard 16-color palette. Named rather than
 * numbered so callers read as intent ("amber on black"), matching
 * VulcanOS's branding palette described in the identity guide. */
enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14, /* commonly rendered as yellow */
    VGA_WHITE = 15,
};

void console_init(void);
void console_clear(void);
void console_putc(char c);
void console_write(const char *str);
void console_write_n(const char *str, usize len);
void console_set_color(enum vga_color fg, enum vga_color bg);

#endif /* VULCAN_DRIVERS_CONSOLE_H */
