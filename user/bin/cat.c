/*
 * cat.c - VulcanOS's cat utility
 *
 * Prints the contents of each named file to stdout. With no
 * arguments, real POSIX cat reads from stdin instead -- not
 * implemented here yet, since VulcanOS's stdin (libc/stdio.c's
 * read() for VULCAN_STDIN) is a non-blocking, poll-only keyboard
 * read at this bring-up milestone, which would make "cat with no
 * args" busy-loop rather than block sensibly waiting for input.
 * Worth adding once a real blocking stdin read exists (see the note
 * in libc/stdio.c's read() implementation).
 */

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
