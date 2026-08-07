#include "drivers/display.h"
#include "drivers/framebuffer.h"
#include "drivers/driver.h"

static const struct display_driver_api fb_display_api = {
    .clear = framebuffer_clear,
    .draw_rect = framebuffer_draw_rect,
    .draw_gradient = framebuffer_draw_gradient,
    .info = framebuffer_info,
};

static bool framebuffer_probe(void)
{
    return framebuffer_available();
}

static void framebuffer_init_driver(void)
{
}

static const struct driver framebuffer_driver = {
    .name = "framebuffer",
    .class = DRIVER_CLASS_DISPLAY,
    .probe = framebuffer_probe,
    .init = framebuffer_init_driver,
    .api = &fb_display_api,
};

bool framebuffer_driver_register(void)
{
    return driver_register(&framebuffer_driver);
}
