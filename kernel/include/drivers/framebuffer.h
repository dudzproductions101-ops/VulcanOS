










#ifndef VULCAN_DRIVERS_FRAMEBUFFER_H
#define VULCAN_DRIVERS_FRAMEBUFFER_H

#include "types.h"

struct framebuffer_info {
    void *address;
    u32 pitch;
    u32 width;
    u32 height;
    u8 bpp;
    u8 framebuffer_type;
    u8 red_field_position;
    u8 red_mask_size;
    u8 green_field_position;
    u8 green_mask_size;
    u8 blue_field_position;
    u8 blue_mask_size;
    bool available;
};

void framebuffer_init(u64 mb2_info_addr);
bool framebuffer_available(void);
const struct framebuffer_info *framebuffer_info(void);
bool framebuffer_driver_register(void);
void framebuffer_clear(u32 color);
void framebuffer_put_pixel(u32 x, u32 y, u32 color);
void framebuffer_draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color);
void framebuffer_draw_gradient(void);

#define FRAMEBUFFER_RGB(r, g, b) ((((u32)(r) & 0xFF) << 16) | (((u32)(g) & 0xFF) << 8) | ((u32)(b) & 0xFF))

#endif 
