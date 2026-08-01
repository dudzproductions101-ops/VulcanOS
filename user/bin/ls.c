
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
