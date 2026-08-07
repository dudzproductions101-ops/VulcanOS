#include "pkg/embedded_packages.h"

extern const u8 g_pkg_hello_vulcan[];
extern const usize g_pkg_hello_vulcan_size;

static const struct embedded_package registry[] = {
    { "hello-vulcan", g_pkg_hello_vulcan, 0 

    },
};

#define REGISTRY_COUNT (sizeof(registry) / sizeof(registry[0]))

static void ensure_sizes_patched(void)
{
    static bool patched = false;
    if (patched) {
        return;
    }

    ((struct embedded_package *)&registry[0])->size = g_pkg_hello_vulcan_size;
    patched = true;
}

const struct embedded_package *embedded_package_find(const char *install_name)
{
    ensure_sizes_patched();

    for (usize i = 0; i < REGISTRY_COUNT; i++) {
        const char *a = registry[i].install_name;
        const char *b = install_name;
        while (*a && *a == *b) {
            a++;
            b++;
        }
        if (*a == *b) {
            return &registry[i];
        }
    }
    return NULL;
}

u32 embedded_package_count(void)
{
    return (u32)REGISTRY_COUNT;
}

const struct embedded_package *embedded_package_at(u32 index)
{
    ensure_sizes_patched();

    if (index >= REGISTRY_COUNT) {
        return NULL;
    }
    return &registry[index];
}
