/*
 * display.h - Display abstraction for graphical drivers
 *
 * A small display API that sits on top of the generic driver manager.
 * It makes it easy for higher-level graphics code to render without
 * depending directly on a specific framebuffer implementation.
 */

#ifndef VULCAN_DRIVERS_DISPLAY_H
#define VULCAN_DRIVERS_DISPLAY_H

#include "types.h"
#include "drivers/framebuffer.h"
#include "drivers/driver.h"

struct display_driver_api {
    void (*clear)(u32 color);
    void (*draw_rect)(u32 x, u32 y, u32 width, u32 height, u32 color);
    void (*draw_gradient)(void);
    const struct framebuffer_info *(*info)(void);
};

bool display_available(void);
const struct framebuffer_info *display_info(void);
void display_init(void);
void display_clear(u32 color);
void display_draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color);
void display_draw_gradient(void);

#endif /* VULCAN_DRIVERS_DISPLAY_H */
