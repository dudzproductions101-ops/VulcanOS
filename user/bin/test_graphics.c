#include "syscall.h"
#include "vulcan_graphics.h"
#include "stdio.h"

/*
 * test_graphics: Ring-3 User Application
 *
 * This program demonstrates ring-3 user code execution.
 * It runs in a separate address space with limited privileges.
 * All hardware access goes through syscalls.
 *
 * Execution model:
 * 1. Started by kernel via SYS_EXEC syscall
 * 2. Executes at privilege level 3 (ring 3)
 * 3. Can only access own address space
 * 4. Hardware access via syscalls (graphics, input, etc.)
 * 5. Cannot access kernel memory or I/O ports
 */

void test_graphics_main(void)
{
    struct graphics_info info = {0};

    printf("[ring3] test_graphics: starting in user mode\n");
    
    /* Query framebuffer via syscall */
    u32 result = graphics_info(&info);
    if (result != 0 || !info.available) {
        printf("[ring3] test_graphics: framebuffer not available\n");
        return;
    }

    printf("[ring3] test_graphics: framebuffer %ux%u@%ubpp\n",
           info.width, info.height, info.bpp);

    /* Clear to dark blue */
    printf("[ring3] test_graphics: clearing screen\n");
    graphics_clear(GRAPHICS_RGB(0, 0, 128));

    /* Draw test rectangles at different positions */
    printf("[ring3] test_graphics: drawing rectangles\n");

    /* Red rectangle */
    graphics_draw_rect(100, 100, 200, 150, GRAPHICS_RGB(255, 0, 0));

    /* Green rectangle */
    graphics_draw_rect(400, 100, 200, 150, GRAPHICS_RGB(0, 255, 0));

    /* Yellow rectangle */
    graphics_draw_rect(250, 300, 200, 150, GRAPHICS_RGB(255, 255, 0));

    printf("[ring3] test_graphics: ring-3 execution complete!\n");
    printf("[ring3] test_graphics: exiting\n");
}
