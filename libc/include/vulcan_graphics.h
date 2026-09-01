#ifndef VULCAN_LIBC_GRAPHICS_H
#define VULCAN_LIBC_GRAPHICS_H

#include "vulcan_types.h"

/* Graphics syscall numbers */
#define SYS_GRAPHICS_INFO       10
#define SYS_GRAPHICS_CLEAR      11
#define SYS_GRAPHICS_DRAW_RECT  12

struct graphics_info {
    u32 width;
    u32 height;
    u32 bpp;
    u8 available;
};

u32 graphics_info(struct graphics_info *info);
void graphics_clear(u32 color);
void graphics_draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color);

#define GRAPHICS_RGB(r, g, b) ((((u32)(r) & 0xFF) << 16) | (((u32)(g) & 0xFF) << 8) | ((u32)(b) & 0xFF))

#endif
