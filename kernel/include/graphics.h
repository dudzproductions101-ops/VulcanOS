/*
 * graphics.h - Minimal kernel graphics interface
 *
 * This is the first user-facing graphics layer beyond raw framebuffer
 * access. It draws a simple boot-time desktop-like UI and is intended
 * as a scaffold for future graphical userland work.
 */

#ifndef VULCAN_GRAPHICS_H
#define VULCAN_GRAPHICS_H

void graphics_init(void);
void graphics_draw_boot_ui(void);

#endif /* VULCAN_GRAPHICS_H */
