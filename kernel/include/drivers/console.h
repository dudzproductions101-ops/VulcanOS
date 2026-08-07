#ifndef VULCAN_DRIVERS_CONSOLE_H
#define VULCAN_DRIVERS_CONSOLE_H

#include "types.h"

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
    VGA_LIGHT_BROWN = 14, 
    VGA_WHITE = 15,
};

void console_init(void);
void console_clear(void);
void console_putc(char c);
void console_write(const char *str);
void console_write_n(const char *str, usize len);
void console_set_color(enum vga_color fg, enum vga_color bg);

#endif 
