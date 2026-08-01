
#include "init.h"
#include "shell.h"
#include "stdio.h"

void init_thread_entry(void)
{
    printf("init: VulcanOS userland starting.\n");

    char *argv[] = { "vulsh" };
    vulsh_main(1, argv);

    printf("init: shell exited unexpectedly.\n");
}
