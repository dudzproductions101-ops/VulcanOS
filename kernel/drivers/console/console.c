/*
 * console.c - VGA text-mode console
 *
 * See drivers/console.h for why VGA text mode is the deliberate
 * choice for VulcanOS's first-boot output path.
 */

#include "drivers/console.h"
#include "arch/x86_64/cpu.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile u16 *)0xB8000)

/* MINIMAL SERIAL MIRROR (kept permanently, not temporary): a raw
 * COM1 (0x3F8) writer, originally added to capture a diagnostic
 * trace longer than VGA text mode's 25-line screen during vpkg
 * bring-up debugging (see PROJECT_STATUS.md's vpkg bring-up section
 * for the real stack-overflow bug this was essential to finding --
 * the corruption's evidence was spread across 80+ lines of boot
 * output, far more than VGA's screen could ever show at once). This
 * is NOT VulcanOS's real serial driver -- a proper one (with IRQ-
 * driven RX, configurable baud, and a real driver header under
 * drivers/) is legitimate future work, scoped on its own merits,
 * not squeezed in here. This block writes every character
 * console_putc receives to COM1 as well as VGA, so a full boot's
 * output can be captured via QEMU's `-serial file:...` with no
 * scrollback limit. Kept in permanently now that it's proven its
 * worth (harmless if no serial port is present -- the status-
 * register check below simply means the loop never spins waiting
 * for a UART that isn't there), but should be superseded by a real
 * driver rather than extended further. */
#define COM1_PORT 0x3F8

static void debug_serial_putc(char c)
{
    /* Poll the Line Status Register (COM1 + 5) bit 5 (Transmit
     * Holding Register Empty) before writing, per the standard
     * 16550 UART protocol -- bounded retry count so a genuinely
     * absent serial port (LSR reads back 0xFF, all bits set,
     * including one that looks like "ready") can't hang this
     * function forever. */
    for (int i = 0; i < 10000; i++) {
        if (inb(COM1_PORT + 5) & 0x20) {
            break;
        }
    }
    outb(COM1_PORT, (u8)c);
}

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

    /* CRT controller index register: select cursor location
     * low/high byte registers and write the new position. This is
     * the standard VGA hardware cursor protocol, present since the
     * original IBM CGA/VGA spec. */
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
    debug_serial_putc(c); /* mirrors every character to COM1 -- see
                            * the SERIAL MIRROR comment above. Kept
                            * as a permanent feature after proving
                            * genuinely necessary during vpkg bring-
                            * up debugging: VGA's 25-line screen
                            * could not show enough scrollback to
                            * diagnose a bug whose evidence was
                            * spread across 80+ lines of boot output,
                            * while `-serial file:...` captured the
                            * complete, ordered trace with no limit.
                            * See PROJECT_STATUS.md's vpkg bring-up
                            * section for the full story this
                            * capability was essential to solving. */

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
