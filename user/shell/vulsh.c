
#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define VULSH_LINE_MAX 256
#define VULSH_MAX_ARGS 16

extern int ls_main(int argc, char **argv);
extern int cat_main(int argc, char **argv);
extern int echo_main(int argc, char **argv);

static void print_prompt(void)
{
    printf("vulsh> ");
}

static void read_line(char *buf, usize max_len)
{
    usize len = 0;

    for (;;) {
        char c;
        ssize_t n = read(VULCAN_STDIN, &c, 1);

        if (n <= 0) {
            yield();
            continue;
        }

        if (c == '\n' || c == '\r') {
            printf("\n");
            break;
        }

        if (c == '\b' || c == 127) {
            if (len > 0) {
                len--;
                printf("\b \b");

            }
            continue;
        }

        if (len < max_len - 1) {
            buf[len++] = c;
            printf("%c", c);

        }
    }

    buf[len] = '\0';
}

static int tokenize(char *line, char **argv, int max_args)
{
    int argc = 0;
    char *p = line;

    while (*p && argc < max_args) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (!*p) {
            break;
        }

        argv[argc++] = p;

        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p) {
            *p = '\0';
            p++;
        }
    }

    return argc;
}

static int dispatch(int argc, char **argv)
{
    if (argc == 0) {
        return 0;
    }

    if (strcmp(argv[0], "exit") == 0) {
        int code = (argc >= 2) ? atoi(argv[1]) : 0;
        exit(code);
    }

    if (strcmp(argv[0], "ls") == 0) {
        return ls_main(argc, argv);
    }
    if (strcmp(argv[0], "cat") == 0) {
        return cat_main(argc, argv);
    }
    if (strcmp(argv[0], "echo") == 0) {
        return echo_main(argc, argv);
    }
    if (strcmp(argv[0], "help") == 0) {
        printf("VulcanOS shell (vulsh) -- built-in commands:\n");
        printf("  ls [path]     list directory contents\n");
        printf("  cat file...   print file contents\n");
        printf("  echo [args]   print arguments\n");
        printf("  help          show this message\n");
        printf("  exit [code]   exit the shell\n");
        return 0;
    }

    printf("vulsh: unknown command: %s\n", argv[0]);
    return 1;
}

int vulsh_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("\nVulcanOS shell (vulsh). Type 'help' for available commands.\n\n");

    char line[VULSH_LINE_MAX];
    char *args[VULSH_MAX_ARGS];

    for (;;) {
        print_prompt();
        read_line(line, VULSH_LINE_MAX);

        int line_argc = tokenize(line, args, VULSH_MAX_ARGS);
        dispatch(line_argc, args);
    }

    return 0;

}
