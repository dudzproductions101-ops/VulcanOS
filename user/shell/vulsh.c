/*
 * vulsh.c - VulcanOS's shell
 *
 * Reads a line, tokenizes it, and dispatches to a built-in command.
 * DISPATCH MECHANISM NOTE: VulcanOS has no exec()-from-disk yet (no
 * ring-3 execution -- see the interim-design note in
 * libc/stdlib.c), so vulsh dispatches by calling the linked-in
 * utility functions (ls_main, cat_main, echo_main) directly as
 * ordinary C function calls, not by forking and executing separate
 * binaries. This is an honest reflection of the current
 * architecture: vulsh, ls, cat, and echo are all compiled into the
 * same kernel image and this function call IS the real dispatch
 * mechanism right now, not a placeholder for a "real" one. When
 * VulcanOS gains fork/exec, this dispatch table becomes the natural
 * place to instead build an argv[0] -> on-disk-path lookup and
 * fork+exec each command as a genuinely separate process.
 */

#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define VULSH_LINE_MAX 256
#define VULSH_MAX_ARGS 16

/* Forward declarations for the linked-in utilities' real entry
 * points -- see this file's top comment for why a direct call is
 * the honest current dispatch mechanism. */
extern int ls_main(int argc, char **argv);
extern int cat_main(int argc, char **argv);
extern int echo_main(int argc, char **argv);
extern int vpkg_main(int argc, char **argv);

static void print_prompt(void)
{
    printf("vulsh> ");
}

/* Reads one line from stdin into `buf` (NUL-terminated, trailing
 * newline stripped), handling backspace ('\b') so line editing
 * actually works, not just raw character accumulation. Polls
 * read() (see stdio.c's read() design note on why stdin is
 * currently non-blocking) and yields between polls so an idle shell
 * waiting for a keypress doesn't burn its entire time slice
 * busy-spinning and starve other threads. */
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

        if (c == '\b' || c == 127) { /* backspace or DEL */
            if (len > 0) {
                len--;
                printf("\b \b"); /* move back, erase, move back again --
                                   * the standard terminal trick for
                                   * visually erasing the last character */
            }
            continue;
        }

        if (len < max_len - 1) {
            buf[len++] = c;
            printf("%c", c); /* local echo -- VulcanOS's console driver
                               * doesn't echo keystrokes on its own, so
                               * the shell must do it explicitly for
                               * typed input to be visible at all */
        }
    }

    buf[len] = '\0';
}

/* Splits `line` into whitespace-separated tokens, in place (writes
 * NUL bytes into `line` itself at each boundary, matching the
 * standard strtok-style approach) and fills `argv` with pointers
 * into it. Returns the resulting argc. Handles multiple consecutive
 * spaces correctly (does not produce empty-string tokens between
 * them). */
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
        return 0; /* empty line, nothing to do */
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
    if (strcmp(argv[0], "vpkg") == 0) {
        return vpkg_main(argc, argv);
    }
    if (strcmp(argv[0], "help") == 0) {
        printf("VulcanOS shell (vulsh) -- built-in commands:\n");
        printf("  ls [path]     list directory contents\n");
        printf("  cat file...   print file contents\n");
        printf("  echo [args]   print arguments\n");
        printf("  vpkg ...      package manager (try 'vpkg' with no args for its own help)\n");
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

    return 0; /* unreachable under normal operation (the loop above
               * never breaks except via exit()'s own noreturn path),
               * but kept for a well-formed function signature */
}
