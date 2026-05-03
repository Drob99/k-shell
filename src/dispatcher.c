#include <string.h>
#include "kshell.h"

int ks_dispatch(int argc, char **argv) {
    if (argc == 0 || argv[0] == NULL) {
        return KS_OK;
    }

    /* Strip --inspect tokens before any routing. The flag applies only to
     * external commands; on builtins it's silently ignored. */
    ks_introspect_strip(&argc, argv);
    if (argc == 0 || argv[0] == NULL) {
        return KS_OK;
    }

    if (strcmp(argv[0], "cd") == 0)      { ks_introspect_consume(); return ks_builtin_cd(argc, argv); }
    if (strcmp(argv[0], "exit") == 0)    { ks_introspect_consume(); return ks_builtin_exit(argc, argv); }
    if (strcmp(argv[0], "help") == 0)    { ks_introspect_consume(); return ks_builtin_help(argc, argv); }
    if (strcmp(argv[0], "history") == 0) { ks_introspect_consume(); return ks_builtin_history(argc, argv); }

    return ks_execute(argv);
}
