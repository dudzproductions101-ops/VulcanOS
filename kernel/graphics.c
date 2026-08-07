#include "graphics.h"
#include "drivers/display.h"
#include "drivers/framebuffer.h"
#include "printk.h"

void graphics_init(void)
{
    display_init();
    if (!display_available()) {
        printk_level(LOG_INFO, "graphics: no display available, staying in text mode\n");
        return;
    }
    graphics_draw_boot_ui();
}

void graphics_draw_boot_ui(void)
{
    const struct framebuffer_info *info = display_info();
    if (!info) {
        return;
    }

    display_clear(FRAMEBUFFER_RGB(10, 16, 36));

    u32 title_height = 64;
    display_draw_rect(0, 0, info->width, title_height, FRAMEBUFFER_RGB(32, 48, 96));
    display_draw_rect(16, title_height + 16, info->width - 32, 160, FRAMEBUFFER_RGB(24, 32, 72));
    display_draw_rect(32, title_height + 40, info->width - 64, 96, FRAMEBUFFER_RGB(54, 102, 164));

    u32 window_width = 320;
    u32 window_height = 180;
    u32 window_x = (info->width - window_width) / 2;
    u32 window_y = title_height + 220;
    display_draw_rect(window_x, window_y, window_width, window_height, FRAMEBUFFER_RGB(40, 48, 72));
    display_draw_rect(window_x + 12, window_y + 12, window_width - 24, window_height - 24, FRAMEBUFFER_RGB(20, 28, 56));

    /* Status band */
    display_draw_rect(window_x + 16, window_y + 28, window_width - 32, 24, FRAMEBUFFER_RGB(92, 159, 222));
    display_draw_rect(window_x + 16, window_y + 64, 100, 10, FRAMEBUFFER_RGB(112, 186, 255));
    display_draw_rect(window_x + 16, window_y + 84, window_width - 48, 10, FRAMEBUFFER_RGB(72, 122, 194));
}
