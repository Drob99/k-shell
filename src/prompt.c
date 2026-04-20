#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kshell.h"

int ks_render_prompt(char *buf, size_t buflen) {
    char cwd[KS_INPUT_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        snprintf(buf, buflen, "kshell:?$ ");
        return KS_ERR_EXEC;
    }

    const char *home = getenv("HOME");
    if (home != NULL) {
        size_t homelen = strlen(home);
        if (strncmp(cwd, home, homelen) == 0 &&
                (cwd[homelen] == '/' || cwd[homelen] == '\0')) {
            snprintf(buf, buflen, "kshell:~%s$ ", cwd + homelen);
            return KS_OK;
        }
    }

    snprintf(buf, buflen, "kshell:%s$ ", cwd);
    return KS_OK;
}
