#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "kshell.h"

/* SIGINT is delivered to the whole foreground process group. The child's
 * default handler kills it; this handler lets the shell survive by just
 * re-prompting. No setpgid needed because we don't require full job-control
 * groups. */
static void sigint_handler(int sig) {
    (void)sig;
    write(1, "\n", 1); /* async-signal-safe; printf/fprintf are not */
}

int main(void) {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* NOT SA_RESTART: fgets must return EINTR so we can re-prompt */
    sigaction(SIGINT, &sa, NULL);

    if (ks_history_init(KS_HISTORY_MAX) != KS_OK) {
        fprintf(stderr, "kshell: failed to initialize history\n");
        return 1;
    }

    char prompt[KS_INPUT_MAX];
    char line[KS_INPUT_MAX];
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    while (1) {
        ks_render_prompt(prompt, sizeof(prompt));
        fputs(prompt, stdout);
        fflush(stdout);

        errno = 0;
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (feof(stdin)) {
                putchar('\n');
                break;          /* Ctrl+D: exit cleanly */
            }
            if (errno == EINTR) {
                clearerr(stdin);
                continue;       /* Ctrl+C: re-prompt */
            }
            perror("kshell: fgets");
            break;
        }

        char *nl = strchr(line, '\n');
        if (nl == NULL) {
            /* Input exceeded KS_INPUT_MAX-1: warn, drain, and re-prompt. */
            fprintf(stderr, "kshell: input too long (max %d chars), truncated\n",
                    KS_INPUT_MAX - 1);
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            continue;
        } else {
            *nl = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

        ks_history_add(line);

        if (ks_parse_line(line, argv, &argc) == KS_ERR_TOO_MANY_ARGS) {
            fprintf(stderr, "kshell: too many arguments (max %d)\n", KS_MAX_ARGS);
            continue;
        }

        int ret = ks_dispatch(argc, argv);

        if (ret == KS_EXIT) {
            break;
        } else if (ret == KS_CMD_NOT_FOUND) {
            fprintf(stderr, "kshell: command not found: %s\n", argv[0]);
        }
        /* ret < 0 (other errors): builtin already printed its message; re-prompt. */
    }

    ks_history_free();
    return ks_builtin_exit_code();
}
