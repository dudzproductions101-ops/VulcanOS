#include "drivers/driver.h"
#include "printk.h"

#define MAX_KERNEL_DRIVERS 16

static const struct driver *registered_drivers[MAX_KERNEL_DRIVERS];
static usize registered_driver_count = 0;

bool driver_register(const struct driver *driver)
{
    if (registered_driver_count >= MAX_KERNEL_DRIVERS) {
        printk_level(LOG_WARN, "driver: cannot register %s, registry full\n", driver->name);
        return false;
    }

    registered_drivers[registered_driver_count++] = driver;
    printk_level(LOG_INFO, "driver: registered %s\n", driver->name);
    return true;
}

void driver_init_all(void)
{
    for (usize i = 0; i < registered_driver_count; i++) {
        const struct driver *driver = registered_drivers[i];
        if (!driver->probe || driver->probe()) {
            if (driver->init) {
                printk_level(LOG_INFO, "driver: initializing %s\n", driver->name);
                driver->init();
            }
        } else {
            printk_level(LOG_INFO, "driver: probe failed for %s\n", driver->name);
        }
    }
}

const struct driver *driver_find(enum driver_class class)
{
    for (usize i = 0; i < registered_driver_count; i++) {
        if (registered_drivers[i]->class == class) {
            return registered_drivers[i];
        }
    }
    return NULL;
}
