#include "graphics.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "vulcan_graphics.h"

extern void yield(void);

struct framebuffer_info {
    u32 width;
    u32 height;
    u32 bpp;
    u8 available;
};

static struct graphics_info fb_cache = {0};
static bool fb_cached = false;

static const struct framebuffer_info *framebuffer_info(void)
{
    if (!fb_cached) {
        if (graphics_info(&fb_cache) != 0) {
            return NULL;
        }
        fb_cached = true;
    }
    
    if (!fb_cache.available) {
        return NULL;
    }
    
    return (const struct framebuffer_info *)&fb_cache;
}

static void draw_background(void)
{
    const struct framebuffer_info *fb = framebuffer_info();
    if (!fb || !fb->available) {
        return;
    }

    graphics_clear(GRAPHICS_RGB(12, 18, 34));
    graphics_draw_rect(0, 0, fb->width, 56, GRAPHICS_RGB(28, 44, 86));
    graphics_draw_rect(20, 72, fb->width - 40, fb->height - 92, GRAPHICS_RGB(18, 26, 46));
    graphics_draw_rect(32, 88, fb->width - 64, 120, GRAPHICS_RGB(50, 94, 164));
    graphics_draw_rect(40, 220, fb->width - 80, 72, GRAPHICS_RGB(92, 154, 220));
    graphics_draw_rect(40, 310, fb->width - 80, 32, GRAPHICS_RGB(122, 182, 246));
}

static void draw_status_text(void)
{
    const struct framebuffer_info *fb = framebuffer_info();
    if (!fb || !fb->available) {
        return;
    }

    printf("graphics_server: boot screen ready\n");
    printf("framebuffer: %ux%u %u bpp\n", fb->width, fb->height, fb->bpp);
}

void graphics_server_entry(void)
{
    const struct framebuffer_info *fb = framebuffer_info();
    if (!fb || !fb->available) {
        printf("graphics_server: framebuffer unavailable, leaving text mode alone\n");
        return;
    }

    draw_background();
    draw_status_text();

    for (;;) {
        yield();
    }
}

void graphics_server_main(void)
{
    graphics_server_entry();
}
