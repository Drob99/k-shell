#include <stdio.h>
#include <string.h>
#include "kshell.h"

int main(void) {
    char prompt[KS_INPUT_MAX];
    char line[KS_INPUT_MAX];
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    while (1) {
        ks_render_prompt(prompt, sizeof(prompt));
        fputs(prompt, stdout);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            putchar('\n');
            break;
        }

        char *nl = strchr(line, '\n');
        if (nl == NULL) {
            /* Input exceeded KS_INPUT_MAX-1: drain the rest silently. */
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
        } else {
            *nl = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

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
