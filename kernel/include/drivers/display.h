







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

#endif 
