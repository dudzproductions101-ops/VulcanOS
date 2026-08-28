#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define VULSH_LINE_MAX 256
#define VULSH_MAX_ARGS 16
#define VULSH_PROMPT "vulsh> "

extern int ls_main(int argc, char **argv);
extern int cat_main(int argc, char **argv);
extern int echo_main(int argc, char **argv);
extern int vpkg_main(int argc, char **argv);

static void print_prompt(void)
{
    printf("%s", VULSH_PROMPT);
}

static void clear_line(char *buf, usize max_len)
{
    if (max_len == 0) {
        return;
    }

    memset(buf, 0, max_len);
}

static void write_char(char c)
{
    printf("%c", c);
}

static void erase_char(void)
{
    printf("\b \b");
}

static void read_line(char *buf, usize max_len)
{
    usize len = 0;

    clear_line(buf, max_len);

    if (max_len == 0) {
        return;
    }

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
                buf[len] = '\0';
                erase_char();
            }
            continue;
        }

        if (c < 32) {
            continue;
        }

        if (len < max_len - 1) {
            buf[len++] = c;
            buf[len] = '\0';
            write_char(c);
        }
    }
}

static int is_space(char c)
{
    return c == ' ' || c == '\t';
}

static int tokenize(char *line, char **argv, int max_args)
{
    int argc = 0;
    char *p = line;

    if (line == NULL || argv == NULL || max_args <= 0) {
        return 0;
    }

    while (*p && argc < max_args) {
        while (*p && is_space(*p)) {
            p++;
        }

        if (!*p) {
            break;
        }

        argv[argc++] = p;

        while (*p && !is_space(*p)) {
            p++;
        }

        if (*p) {
            *p = '\0';
            p++;
        }
    }

    return argc;
}

static void print_help(void)
{
    printf("VulcanOS shell (vulsh) -- built-in commands:\n");
    printf("  ls [path]       list directory contents\n");
    printf("  cat file...     print file contents\n");
    printf("  echo [args]     print arguments\n");
    printf("  vpkg ...        package manager\n");
    printf("  help            show this message\n");
    printf("  clear           clear the terminal\n");
    printf("  version         show shell version\n");
    printf("  exit [code]     exit the shell\n");
}

static void print_version(void)
{
    printf("vulsh 0.1.0\n");
    printf("VulcanOS userspace shell\n");
}

static void clear_terminal(void)
{
    printf("\033[2J\033[H");
}

static int command_exit(int argc, char **argv)
{
    int code = 0;

    if (argc >= 2) {
        code = atoi(argv[1]);
    }

    exit(code);
    return 0;
}

static int command_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    print_help();
    return 0;
}

static int command_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    clear_terminal();
    return 0;
}

static int command_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    print_version();
    return 0;
}

static int dispatch(int argc, char **argv)
{
    if (argc == 0 || argv == NULL) {
        return 0;
    }

    if (strcmp(argv[0], "exit") == 0) {
        return command_exit(argc, argv);
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

    if (strcmp(argv[0], "vpkg") == 0) {
        return vpkg_main(argc, argv);
    }

    if (strcmp(argv[0], "help") == 0) {
        return command_help(argc, argv);
    }

    if (strcmp(argv[0], "clear") == 0) {
        return command_clear(argc, argv);
    }

    if (strcmp(argv[0], "version") == 0) {
        return command_version(argc, argv);
    }

    printf("vulsh: unknown command: %s\n", argv[0]);
    return 1;
}

static int execute_line(char *line, char **argv)
{
    int argc;

    argc = tokenize(line, argv, VULSH_MAX_ARGS);

    if (argc == 0) {
        return 0;
    }

    return dispatch(argc, argv);
}

static void print_banner(void)
{
    printf("\n");
    printf("VulcanOS shell (vulsh)\n");
    printf("Type 'help' for available commands.\n");
    printf("\n");
}

int vulsh_main(int argc, char **argv)
{
    char line[VULSH_LINE_MAX];
    char *args[VULSH_MAX_ARGS];

    (void)argc;
    (void)argv;

    print_banner();

    for (;;) {
        print_prompt();
        read_line(line, VULSH_LINE_MAX);
        execute_line(line, args);
    }

    return 0;
}
