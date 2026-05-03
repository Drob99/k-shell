#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "kshell.h"

static int g_last_status = 0;

/* Decode wait status into bash-style exit code (128 + signal if signalled). */
static int decode_status(int status) {
    if (WIFEXITED(status))   return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 0;
}

/* Standard fork+execvp+waitpid path, used when --inspect is NOT set. */
static int execute_plain(char **argv) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("kshell: fork");
        return KS_ERR_FORK;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "kshell: command not found: %s\n", argv[0]);
                _exit(127);
            case EACCES:
                fprintf(stderr, "kshell: permission denied: %s\n", argv[0]);
                _exit(126);
            default:
                fprintf(stderr, "kshell: %s: %s\n", argv[0], strerror(errno));
                _exit(126);
        }
    }

    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("kshell: waitpid");
            return KS_ERR_EXEC;
        }
    }

    g_last_status = decode_status(status);
    return KS_OK;
}

/* Inspect path: same fork/exec/wait shape but instrumented with a pipe
 * for child->parent FD-snapshot transfer, wait4() for per-child rusage,
 * and CLOCK_MONOTONIC wall-clock measurement. wait4() is BSD-derived but
 * available on both Darwin and Linux; it's strictly more precise than
 * getrusage(RUSAGE_CHILDREN) deltas, which aggregate across all children. */
static int execute_with_inspect(char **argv) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("kshell: pipe");
        return KS_ERR_EXEC;
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    pid_t pid = fork();

    if (pid == -1) {
        perror("kshell: fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return KS_ERR_FORK;
    }

    if (pid == 0) {
        close(pipefd[0]);                       /* child: no need for read end */
        ks_introspect_child_snapshot(pipefd[1]); /* writes FD list, closes pipefd[1] */
        execvp(argv[0], argv);
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "kshell: command not found: %s\n", argv[0]);
                _exit(127);
            case EACCES:
                fprintf(stderr, "kshell: permission denied: %s\n", argv[0]);
                _exit(126);
            default:
                fprintf(stderr, "kshell: %s: %s\n", argv[0], strerror(errno));
                _exit(126);
        }
    }

    /* Parent: close write end so child's close causes EOF on our read. */
    close(pipefd[1]);

    char fd_list[KS_INSPECT_FD_BUF];
    ks_introspect_read_fds(pipefd[0], fd_list, sizeof(fd_list));

    int status;
    struct rusage child_usage;
    while (wait4(pid, &status, 0, &child_usage) == -1) {
        if (errno != EINTR) {
            perror("kshell: wait4");
            return KS_ERR_EXEC;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long wallclock_us = (t1.tv_sec - t0.tv_sec) * 1000000L
                      + (t1.tv_nsec - t0.tv_nsec) / 1000L;

    int decoded = decode_status(status);
    g_last_status = decoded;

    char out[KS_INSPECT_OUT_BUF];
    if (ks_introspect_format(&child_usage, wallclock_us, decoded, fd_list, out, sizeof(out)) == KS_OK) {
        fputs(out, stdout);
        fflush(stdout);
    }
    return KS_OK;
}

int ks_execute(char **argv) {
    if (ks_introspect_consume()) {
        return execute_with_inspect(argv);
    }
    return execute_plain(argv);
}
