#include <string.h>
#include "kshell.h"

int ks_parse_line(char *line, char **argv, int *argc) {
    if (line == NULL || argv == NULL || argc == NULL) {
        return KS_ERR_PARSE;
    }

    *argc = 0;
    argv[0] = NULL;

    char *saveptr;
    char *tok = strtok_r(line, " \t", &saveptr);

    while (tok != NULL) {
        if (*argc >= KS_MAX_ARGS) {
            argv[*argc] = NULL;
            return KS_ERR_TOO_MANY_ARGS;
        }
        argv[(*argc)++] = tok;
        tok = strtok_r(NULL, " \t", &saveptr);
    }

    argv[*argc] = NULL;
    return KS_OK;
}
