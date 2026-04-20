#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kshell.h"

static int g_exit_code = 0;

int ks_builtin_exit_code(void) {
    return g_exit_code;
}

int ks_builtin_cd(int argc, char **argv) {
    (void)argv;

    if (argc > 2) {
        fprintf(stderr, "kshell: cd: too many arguments\n");
        return KS_ERR_BUILTIN;
    }

    const char *path;
    if (argc == 1) {
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "kshell: cd: HOME not set\n");
            return KS_ERR_BUILTIN;
        }
    } else {
        path = argv[1];
    }

    if (chdir(path) != 0) {
        perror("kshell: cd");
        return KS_ERR_BUILTIN;
    }
    return KS_OK;
}

int ks_builtin_exit(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "kshell: exit: too many arguments\n");
        return KS_ERR_BUILTIN;
    }
    g_exit_code = (argc == 2) ? atoi(argv[1]) : 0;
    return KS_EXIT;
}

int ks_builtin_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("K-Shell — lightweight POSIX shell\n");
    printf("\n");
    printf("Built-in commands:\n");
    printf("  cd [dir]     Change working directory (default: $HOME)\n");
    printf("  exit [n]     Exit the shell with optional status code n\n");
    printf("  help         Show this help message\n");
    printf("  history      Display command history\n");
    printf("\n");
    printf("External commands are executed via fork/execvp.\n");
    return KS_OK;
}

int ks_builtin_history(int argc, char **argv) {
    (void)argc;
    (void)argv;
    int n = ks_history_count();
    for (int i = 0; i < n; i++) {
        printf("%5d  %s\n", i + 1, ks_history_get(i));
    }
    return KS_OK;
}
