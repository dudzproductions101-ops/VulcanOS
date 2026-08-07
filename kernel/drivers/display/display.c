#include "drivers/display.h"
#include "drivers/driver.h"
#include "printk.h"

static const struct display_driver_api *active_api = NULL;

void display_init(void)
{
    const struct driver *driver = driver_find(DRIVER_CLASS_DISPLAY);
    if (!driver || !driver->api) {
        printk_level(LOG_INFO, "display: no display driver available\n");
        return;
    }

    active_api = (const struct display_driver_api *)driver->api;
    printk_level(LOG_INFO, "display: using driver %s\n", driver->name);
}

bool display_available(void)
{
    return active_api && active_api->info && active_api->info()->available;
}

const struct framebuffer_info *display_info(void)
{
    if (!active_api || !active_api->info) {
        return NULL;
    }
    return active_api->info();
}

void display_clear(u32 color)
{
    if (active_api && active_api->clear) {
        active_api->clear(color);
    }
}

void display_draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color)
{
    if (active_api && active_api->draw_rect) {
        active_api->draw_rect(x, y, width, height, color);
    }
}

void display_draw_gradient(void)
{
    if (active_api && active_api->draw_gradient) {
        active_api->draw_gradient();
    }
}
