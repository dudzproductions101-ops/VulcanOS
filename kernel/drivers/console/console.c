
#include "drivers/console.h"
#include "arch/x86_64/cpu.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile u16 *)0xB8000)

static usize cursor_row = 0;
static usize cursor_col = 0;
static u8 current_color = 0;

static inline u16 vga_entry(char c, u8 color)
{
    return (u16)c | ((u16)color << 8);
}

static inline u8 vga_color(enum vga_color fg, enum vga_color bg)
{
    return (u8)fg | ((u8)bg << 4);
}

static void console_move_cursor(void)
{
    u16 pos = (u16)(cursor_row * VGA_WIDTH + cursor_col);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

static void console_scroll(void)
{
    for (usize row = 1; row < VGA_HEIGHT; row++) {
        for (usize col = 0; col < VGA_WIDTH; col++) {
            VGA_MEMORY[(row - 1) * VGA_WIDTH + col] = VGA_MEMORY[row * VGA_WIDTH + col];
        }
    }

    for (usize col = 0; col < VGA_WIDTH; col++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = vga_entry(' ', current_color);
    }

    cursor_row = VGA_HEIGHT - 1;
}

void console_init(void)
{
    current_color = vga_color(VGA_LIGHT_GREY, VGA_BLACK);
    console_clear();
}

void console_clear(void)
{
    for (usize row = 0; row < VGA_HEIGHT; row++) {
        for (usize col = 0; col < VGA_WIDTH; col++) {
            VGA_MEMORY[row * VGA_WIDTH + col] = vga_entry(' ', current_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
    console_move_cursor();
}

void console_set_color(enum vga_color fg, enum vga_color bg)
{
    current_color = vga_color(fg, bg);
}

void console_putc(char c)
{
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 4) & ~(usize)3;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(' ', current_color);
        }
    } else {
        VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(c, current_color);
        cursor_col++;
    }

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
    if (cursor_row >= VGA_HEIGHT) {
        console_scroll();
    }

    console_move_cursor();
}

void console_write(const char *str)
{
    while (*str) {
        console_putc(*str++);
    }
}

void console_write_n(const char *str, usize len)
{
    for (usize i = 0; i < len; i++) {
        console_putc(str[i]);
    }
}
