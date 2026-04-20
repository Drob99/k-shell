#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "kshell.h"

static int g_last_status = 0;

int ks_execute(char **argv) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("kshell: fork");
        return KS_ERR_FORK;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        /* execvp returned — it failed. */
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

    /* Parent */
    int status;
    while (waitpid(pid, &status, 0) == -1)
    {
        if (errno != EINTR)
        {
            perror("kshell: waitpid");
            return KS_ERR_EXEC;
        }
    }

    if (WIFEXITED(status)) {
        g_last_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        g_last_status = 128 + WTERMSIG(status);
    }

    return KS_OK;
}
