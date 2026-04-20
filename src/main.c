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
            return 0;
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

        int ret = ks_parse_line(line, argv, &argc);
        if (ret == KS_ERR_TOO_MANY_ARGS) {
            fprintf(stderr, "kshell: too many arguments (max %d)\n", KS_MAX_ARGS);
            continue;
        }

        if (argc == 0) {
            continue;
        }

        printf("parsed %d token%s:", argc, argc == 1 ? "" : "s");
        for (int i = 0; i < argc; i++) {
            printf(" [%s]", argv[i]);
        }
        putchar('\n');
    }
}
