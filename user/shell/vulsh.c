#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stddef.h"

#ifndef VULCAN_STDIN
#define VULCAN_STDIN 0
#endif

extern ssize_t read(int fd, void *buf, size_t count);
extern void yield(void);

#define VULSH_LINE_MAX       256
#define VULSH_MAX_ARGS       16
#define VULSH_HISTORY_MAX    32
#define VULSH_PAGE_LINES     18
#define VULSH_PROMPT         "vulsh> "

extern int ls_main(int argc, char **argv);
extern int cat_main(int argc, char **argv);
extern int echo_main(int argc, char **argv);
extern int vpkg_main(int argc, char **argv);

static char history[VULSH_HISTORY_MAX][VULSH_LINE_MAX];
static int history_count = 0;

static void history_add(const char *line)
{
    if (line == NULL || line[0] == '\0') {
        return;
    }

    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
        return;
    }

    if (history_count < VULSH_HISTORY_MAX) {
        strncpy(history[history_count], line, VULSH_LINE_MAX - 1);
        history[history_count][VULSH_LINE_MAX - 1] = '\0';
        history_count++;
        return;
    }

    memmove(history[0], history[1], (VULSH_HISTORY_MAX - 1) * VULSH_LINE_MAX);
    strncpy(history[VULSH_HISTORY_MAX - 1], line, VULSH_LINE_MAX - 1);
    history[VULSH_HISTORY_MAX - 1][VULSH_LINE_MAX - 1] = '\0';
}

static void print_history(void)
{
    if (history_count == 0) {
        printf("history: empty\n");
        return;
    }

    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i]);
    }
}

static void print_prompt(void)
{
    printf("%s", VULSH_PROMPT);
}

static void clear_line(char *buf, size_t max_len)
{
    if (buf != NULL && max_len > 0) {
        memset(buf, 0, max_len);
    }
}

static void write_char(char c)
{
    printf("%c", c);
}

static void erase_char(void)
{
    printf("\b \b");
}

static void clear_terminal(void)
{
    printf("\033[2J\033[H");
}

static void redraw_input(const char *buf)
{
    printf("\r\033[K");
    print_prompt();
    if (buf != NULL) {
        printf("%s", buf);
    }
}

static void read_line(char *buf, size_t max_len)
{
    size_t len = 0;
    int hist_pos;
    char c;

    clear_line(buf, max_len);

    if (buf == NULL || max_len < 2) {
        return;
    }

    hist_pos = history_count;

    for (;;) {
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
            hist_pos = history_count;
            continue;
        }

        if ((unsigned char)c == 27) {
            char seq1, seq2;

            if (read(VULCAN_STDIN, &seq1, 1) <= 0) continue;
            if (seq1 != '[') continue;
            if (read(VULCAN_STDIN, &seq2, 1) <= 0) continue;

            if (seq2 == 'A') {
                if (history_count == 0) continue;
                if (hist_pos > 0) hist_pos--;

                strncpy(buf, history[hist_pos], max_len - 1);
                buf[max_len - 1] = '\0';
                len = strlen(buf);

                redraw_input(buf);
                continue;
            }

            if (seq2 == 'B') {
                if (history_count == 0) continue;

                if (hist_pos < history_count - 1) {
                    hist_pos++;
                    strncpy(buf, history[hist_pos], max_len - 1);
                    buf[max_len - 1] = '\0';
                    len = strlen(buf);
                } else {
                    hist_pos = history_count;
                    len = 0;
                    buf[0] = '\0';
                }

                redraw_input(buf);
                continue;
            }
            continue;
        }

        if ((unsigned char)c < 32) {
            continue;
        }

        if (len < max_len - 1) {
            buf[len++] = c;
            buf[len] = '\0';
            write_char(c);
            hist_pos = history_count;
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

        if (!*p) break;

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

static int pager_wait(void)
{
    char c;

    printf("\n-- MORE -- [Enter: next  Space: page  q: quit] ");

    for (;;) {
        ssize_t n = read(VULCAN_STDIN, &c, 1);

        if (n <= 0) {
            yield();
            continue;
        }

        if (c == 'q' || c == 'Q') {
            printf("\n");
            return 0;
        }

        if (c == ' ') {
            printf("\r\033[K");
            return 1;
        }

        if (c == '\n' || c == '\r') {
            printf("\r\033[K");
            return 2;
        }
    }
}

static int pager_line(const char *line, int *line_count)
{
    if (line == NULL || line_count == NULL) {
        return 1;
    }

    printf("%s\n", line);
    (*line_count)++;

    if (*line_count >= VULSH_PAGE_LINES) {
        int result = pager_wait();

        if (result == 0) return 0;
        
        if (result == 1) {
            *line_count = 0;
        } else {
            *line_count = VULSH_PAGE_LINES - 1;
        }
    }

    return 1;
}

static void print_fastfetch(void)
{
    printf("\n");
    printf("                 xl\"\"``\"\"lx\n");
    printf("                 X8Xxx..xxX8X\n");
    printf("                 8X8bdX8bd8X8\n");
    printf("                dX8XbdX8XbdX8Xb\n");
    printf("               dX8XbdX8XbdX8Xb\n");
    printf("              dX8XbdX8X8XbdX8Xb\n");
    printf("            .dX8XbdX8X8X8XbdX8Xb.\n");
    printf("          .d8X8XbdX8X8X8X8XbdX8X8b.\n");
    printf("      _.-dX8X8XbdX8X8X8X8X8XdbX8X8Xb-._\n");
    printf("   .-d8X8X8X8bdX8X8X8X8X8X8X8X8db8X8X8X8b-.\n");
    printf(".-d8X8X8X8X8bdX8X8X8X8X8X8X8X8X8db8X8X8-RG-b-.\n");
    printf("\n");
    printf("  OS           VulcanOS\n");
    printf("  Kernel       VulcanOS Kernel Build 3.3.2\n");
    printf("  Shell        vulsh 1.2\n");
    printf("  Architecture x86_64\n");
    printf("  Hardware     shi idk\n");
    printf("  Terminal     vulsh\n");
    printf("  User         dudas@vulcan\n");
    printf("\n");
}

static void print_fetch(void)
{
    print_fastfetch();
}

static void print_logo(void)
{
    printf("\n");
    printf("              _             _\n");
    printf(" __   __    _| |_ __ _  ___| |__\n");
    printf(" \\ \\ / /   / _` |/ _` |/ __| '_ \\\n");
    printf("  \\ V /   | (_| | (_| | (__| | | |\n");
    printf("   \\_/     \\__,_|\\__,_|\\___|_| |_|\n");
    printf("\n");
}

static void print_help(void)
{
    int lines = 0;

#define HELP(x)                                      \
    do {                                             \
        if (!pager_line((x), &lines)) {              \
            return;                                  \
        }                                            \
    } while (0)

    HELP("VulcanOS shell (vulsh)");
    HELP("");
    HELP("USAGE");
    HELP("  command [arguments]");
    HELP("");
    HELP("FILE COMMANDS");
    HELP("  ls [path]       list directory contents");
    HELP("  dir [path]      alias for ls");
    HELP("  cat file...     print file contents");
    HELP("");
    HELP("SHELL COMMANDS");
    HELP("  help            show this help");
    HELP("  help short      compact help");
    HELP("  history         show command history");
    HELP("  clear           clear terminal");
    HELP("  cls             alias for clear");
    HELP("  version         show shell version");
    HELP("  fastfetch       show system information");
    HELP("  fetch           alias for fastfetch");
    HELP("  logo            show VulcanOS logo");
    HELP("  about           show VulcanOS information");
    HELP("  true            return success");
    HELP("  false           return failure");
    HELP("  exit [code]     exit the shell");
    HELP("");
    HELP("OTHER");
    HELP("  echo [args]     print arguments");
    HELP("  print [args]    alias for echo");
    HELP("");
    HELP("PACKAGE MANAGER");
    HELP("  vpkg ...        package manager");
    HELP("  pkg ...         alias for vpkg");
    HELP("");
    HELP("LINE EDITING");
    HELP("  Up/Down         command history");
    HELP("  Backspace       delete character");
    HELP("");
    HELP("OUTPUT");
    HELP("  help is paged automatically so output");
    HELP("  does not disappear below the screen.");
    HELP("");
    HELP("No external packages are required by vulsh.");

#undef HELP
}

static void print_help_short(void)
{
    printf("Vulsh commands:\n");
    printf("  ls, dir       files\n");
    printf("  cat           display files\n");
    printf("  echo, print   print text\n");
    printf("  fastfetch     system information\n");
    printf("  history       command history\n");
    printf("  clear, cls    clear terminal\n");
    printf("  version       shell version\n");
    printf("  about         VulcanOS information\n");
    printf("  help          full command list\n");
    printf("  exit          leave shell\n");
    printf("\n");
    printf("Use Up/Down for command history.\n");
}

static void print_version(void)
{
    printf("vulsh 0.1.0\n");
    printf("VulcanOS userspace shell\n");
}

static void print_about(void)
{
    printf("VulcanOS\n");
    printf("A small operating system written for the Vulcan project.\n");
    printf("Userspace shell: vulsh\n");
    printf("Shell version:   0.1.0\n");
}

static int command_exit(int argc, char **argv)
{
    int code = 0;
    if (argc >= 2) code = atoi(argv[1]);
    exit(code);
    return 0;
}

static int command_help(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "short") == 0) {
        print_help_short();
    } else {
        print_help();
    }
    return 0;
}

static int command_clear(int argc, char **argv)
{
    (void)argc; (void)argv;
    clear_terminal();
    return 0;
}

static int command_version(int argc, char **argv)
{
    (void)argc; (void)argv;
    print_version();
    return 0;
}

static int command_logo(int argc, char **argv)
{
    (void)argc; (void)argv;
    print_logo();
    return 0;
}

static int command_fetch(int argc, char **argv)
{
    (void)argc; (void)argv;
    print_fetch();
    return 0;
}

static int command_about(int argc, char **argv)
{
    (void)argc; (void)argv;
    print_about();
    return 0;
}

static int command_history(int argc, char **argv)
{
    (void)argc; (void)argv;
    print_history();
    return 0;
}

static int command_true(int argc, char **argv)
{
    (void)argc; (void)argv;
    return 0;
}

static int command_false(int argc, char **argv)
{
    (void)argc; (void)argv;
    return 1;
}

static int dispatch(int argc, char **argv)
{
    if (argc == 0 || argv == NULL) return 0;

    if (strcmp(argv[0], "exit") == 0) return command_exit(argc, argv);
    if (strcmp(argv[0], "help") == 0) return command_help(argc, argv);
    if (strcmp(argv[0], "clear") == 0 || strcmp(argv[0], "cls") == 0) return command_clear(argc, argv);
    if (strcmp(argv[0], "version") == 0) return command_version(argc, argv);
    if (strcmp(argv[0], "logo") == 0) return command_logo(argc, argv);
    if (strcmp(argv[0], "fetch") == 0 || strcmp(argv[0], "fastfetch") == 0) return command_fetch(argc, argv);
    if (strcmp(argv[0], "about") == 0) return command_about(argc, argv);
    if (strcmp(argv[0], "history") == 0) return command_history(argc, argv);
    if (strcmp(argv[0], "true") == 0) return command_true(argc, argv);
    if (strcmp(argv[0], "false") == 0) return command_false(argc, argv);

    if (strcmp(argv[0], "ls") == 0 || strcmp(argv[0], "dir") == 0) return ls_main(argc, argv);
    if (strcmp(argv[0], "cat") == 0) return cat_main(argc, argv);
    if (strcmp(argv[0], "echo") == 0 || strcmp(argv[0], "print") == 0) return echo_main(argc, argv);

    if (strcmp(argv[0], "vpkg") == 0 || strcmp(argv[0], "pkg") == 0) return vpkg_main(argc, argv);

    printf("vulsh: unknown command: %s\nTry 'help'.\n", argv[0]);
    return 1;
}

static int execute_line(char *line, char **argv)
{
    int argc = tokenize(line, argv, VULSH_MAX_ARGS);
    if (argc == 0) return 0;
    return dispatch(argc, argv);
}

static void print_banner(void)
{
    printf("\n");
    printf("VulcanOS shell (vulsh) 0.1.0\n");
    printf("Type 'help' for commands.\n");
    printf("Use Up/Down for command history.\n");
    printf("\n");
}

int vulsh_main(int argc, char **argv)
{
    char line[VULSH_LINE_MAX];
    char *args[VULSH_MAX_ARGS];

    (void)argc; (void)argv;

    clear_terminal();
    print_banner();

    for (;;) {
        print_prompt();
        read_line(line, VULSH_LINE_MAX);
        history_add(line);
        execute_line(line, args);
    }

    return 0;
}