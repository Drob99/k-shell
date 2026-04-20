#include "kshell.h"

void ks_history_init(void) {
}

int ks_history_add(const char *line) {
    (void)line;
    return KS_OK;
}

const char *ks_history_get(int index) {
    (void)index;
    return (const char *)0;
}

int ks_history_count(void) {
    return 0;
}

void ks_history_free(void) {
}
