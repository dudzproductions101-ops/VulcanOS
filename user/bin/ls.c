/*
 * ls.c - VulcanOS's ls utility
 *
 * Lists the contents of a directory, one entry per line, with a
 * trailing "/" on directory entries -- the minimal, information-
 * dense output this bring-up milestone's utilities aim for (see the
 * "developer-focused, not beginner-oversized" goal in the project's
 * design docs). No -l/-a/-h flag handling yet; real POSIX ls's rich
 * flag set is straightforward future work once something actually
 * needs it, matching the same scoping call made in echo.c.
 */

#include "stdio.h"

int ls_main(int argc, char **argv)
{
    const char *path = (argc >= 2) ? argv[1] : "/";

    unsigned long index = 0;
    struct vulcan_dirent entry;
    int found_any = 0;

    while (vulcan_readdir(path, index, &entry)) {
        found_any = 1;
        if (entry.is_directory) {
            printf("%s/\n", entry.name);
        } else {
            printf("%s\n", entry.name);
        }
        index++;
    }

    if (!found_any) {
        printf("ls: %s: no such directory or empty\n", path);
        return 1;
    }

    return 0;
}
