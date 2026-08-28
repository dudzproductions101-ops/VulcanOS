#include "init.h"
#include "shell.h"
#include "stdio.h"

void init_thread_entry(void)
{
    printf("init: VulcanOS userland starting.\n");

    char *argv[] = { "vulsh", NULL };
    int status = vulsh_main(1, argv);

    printf("init: shell exited with status %d.\n", status);

    for (;;) {
        /* Keep init alive. */
    }
}