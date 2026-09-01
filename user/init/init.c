#include "init.h"
#include "shell.h"
#include "stdio.h"
#include "syscall.h"
#include "vulcan_types.h"

extern void graphics_server_main(void);
extern void test_graphics_main(void);

void init_thread_entry(void)
{
    printf("init: VulcanOS userland starting.\n");
    graphics_server_main();

    printf("init: spawning ring-3 test_graphics process\n");
    
    /* Execute test_graphics in ring 3
     * SYS_EXEC (20): entry point in RDI
     */
    u64 result;
    asm volatile (
        "mov $20, %%rax\n"              /* SYS_EXEC = 20 */
        "mov %1, %%rdi\n"               /* RDI = test_graphics_main */
        "syscall\n"
        "mov %%rax, %0\n"
        : "=r" (result)
        : "r" ((u64)(void*)test_graphics_main)
        : "rax", "rcx", "r11", "memory"
    );

    if (result != 0) {
        printf("init: exec failed, result=%llu\n", result);
    }

    char *argv[] = { "vulsh", NULL };
    int status = vulsh_main(1, argv);

    printf("init: shell exited with status %d.\n", status);

    for (;;) {
        
    }
}