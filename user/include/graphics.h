#ifndef VULCAN_USER_GRAPHICS_H
#define VULCAN_USER_GRAPHICS_H

#include "vulcan_types.h"
#include "vulcan_graphics.h"

#define FRAMEBUFFER_RGB(r, g, b) GRAPHICS_RGB(r, g, b)

void graphics_server_main(void);
void graphics_server_entry(void);

#endif
