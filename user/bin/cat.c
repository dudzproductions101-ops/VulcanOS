
#include "stdio.h"

#define CAT_BUF_SIZE 512

static int cat_one_file(const char *path)
{
    int fd = open(path, VULCAN_O_READ);
    if (fd < 0) {
        printf("cat: cannot open '%s'\n", path);
        return 1;
    }

    char buf[CAT_BUF_SIZE];
    ssize_t n;
    while ((n = read(fd, buf, CAT_BUF_SIZE)) > 0) {
        write(VULCAN_STDOUT, buf, (usize)n);
    }

    close(fd);
    return 0;
}

int cat_main(int argc, char **argv)
{
    if (argc < 2) {
        printf("cat: missing file operand\n");
        return 1;
    }

    int exit_code = 0;
    for (int i = 1; i < argc; i++) {
        if (cat_one_file(argv[i]) != 0) {
            exit_code = 1;
        }
    }
    return exit_code;
}
