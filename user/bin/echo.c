/*
 * echo.c - VulcanOS's echo utility
 *
 * Prints its arguments, space-separated, followed by a newline --
 * the standard, minimal Unix echo behavior. No flags (-n, -e, ...)
 * in this bring-up milestone; real POSIX echo's flag handling is
 * straightforward future work once there's a compelling reason
 * (some script/utility that actually needs -n) to add it, not
 * before.
 */

#include "stdio.h"

int echo_main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}
