/*
 * keyboard.c - PS/2 keyboard driver (scancode set 1)
 *
 * See drivers/keyboard.h for why PS/2 is the correct first target.
 * This driver deliberately only handles the printable-ASCII subset
 * of scancode set 1 plus a few control keys; a full scancode table
 * (function keys, modifiers-as-state, extended E0-prefixed codes)
 * is future work once something (a shell) actually consumes it.
 */

#include "drivers/keyboard.h"
#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/cpu.h"

#define PS2_DATA_PORT 0x60

#define KBD_BUFFER_SIZE 256

static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile usize kbd_head = 0;
static volatile usize kbd_tail = 0;

static bool shift_held = false;

/* Scancode set 1, unshifted. Index = scancode, value = ASCII (0 =
 * no mapping, e.g. modifier keys or unmapped codes). Covers the
 * standard US QWERTY main block; a real layout system (and non-US
 * layouts) is userland/config scope, not this driver's job. */
static const char scancode_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ', 0,
};

static const char scancode_ascii_shifted[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ', 0,
};

#define SCANCODE_LSHIFT_PRESS   0x2A
#define SCANCODE_RSHIFT_PRESS   0x36
#define SCANCODE_LSHIFT_RELEASE 0xAA
#define SCANCODE_RSHIFT_RELEASE 0xB6
#define SCANCODE_RELEASE_BIT    0x80

static void kbd_buffer_push(char c)
{
    usize next = (kbd_head + 1) % KBD_BUFFER_SIZE;
    if (next == kbd_tail) {
        return; /* buffer full: drop the keystroke rather than corrupt state */
    }
    kbd_buffer[kbd_head] = c;
    kbd_head = next;
}

static void keyboard_irq_handler(struct interrupt_frame *frame)
{
    (void)frame;
    u8 scancode = inb(PS2_DATA_PORT);

    if (scancode == SCANCODE_LSHIFT_PRESS || scancode == SCANCODE_RSHIFT_PRESS) {
        shift_held = true;
        return;
    }
    if (scancode == SCANCODE_LSHIFT_RELEASE || scancode == SCANCODE_RSHIFT_RELEASE) {
        shift_held = false;
        return;
    }
    if (scancode & SCANCODE_RELEASE_BIT) {
        return; /* other key-release events: ignored at this layer */
    }
    if (scancode >= 128) {
        return;
    }

    char c = shift_held ? scancode_ascii_shifted[scancode] : scancode_ascii[scancode];
    if (c != 0) {
        kbd_buffer_push(c);
    }
}

void keyboard_init(void)
{
    irq_register_handler(1, keyboard_irq_handler);
}

char keyboard_read(void)
{
    if (kbd_head == kbd_tail) {
        return 0;
    }
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}
