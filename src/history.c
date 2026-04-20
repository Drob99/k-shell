#include <stdlib.h>
#include <string.h>
#include "kshell.h"

static char **g_entries     = NULL;
static size_t g_capacity    = 0;
static size_t g_count       = 0;
static size_t g_head        = 0;
static int    g_initialized = 0;

static int is_blank(const char *s) {
    while (*s) {
        if (*s != ' ' && *s != '\t') return 0;
        s++;
    }
    return 1;
}

int ks_history_init(size_t capacity) {
    if (g_initialized) return KS_ERR_HISTORY;
    if (capacity == 0) return KS_ERR_HISTORY;

    g_entries = calloc(capacity, sizeof(char *));
    if (g_entries == NULL) return KS_ERR_HISTORY;

    g_capacity    = capacity;
    g_count       = 0;
    g_head        = 0;
    g_initialized = 1;
    return KS_OK;
}

int ks_history_add(const char *line) {
    if (!g_initialized || line == NULL) return KS_ERR_HISTORY;
    if (is_blank(line))                 return KS_OK;

    /* Consecutive duplicate suppression. */
    if (g_count > 0) {
        const char *last = ks_history_get((int)(g_count - 1));
        if (last != NULL && strcmp(last, line) == 0) return KS_OK;
    }

    /* Evict oldest entry when slot is occupied (buffer full, wrapping). */
    if (g_entries[g_head] != NULL) {
        free(g_entries[g_head]);
        g_entries[g_head] = NULL;
    }

    g_entries[g_head] = strdup(line);
    if (g_entries[g_head] == NULL) return KS_ERR_HISTORY;

    g_head = (g_head + 1) % g_capacity;
    if (g_count < g_capacity) g_count++;
    return KS_OK;
}

const char *ks_history_get(int index) {
    if (!g_initialized || index < 0 || (size_t)index >= g_count) return NULL;

    size_t physical;
    if (g_count < g_capacity) {
        physical = (size_t)index;
    } else {
        physical = (g_head + (size_t)index) % g_capacity;
    }
    return g_entries[physical];
}

int ks_history_count(void) {
    if (!g_initialized) return 0;
    return (int)g_count; /* safe: count bounded by capacity, which fits int */
}

void ks_history_free(void) {
    if (!g_initialized) return;

    for (size_t i = 0; i < g_capacity; i++) {
        if (g_entries[i] != NULL) {
            free(g_entries[i]);
            g_entries[i] = NULL;
        }
    }
    free(g_entries);
    g_entries     = NULL;
    g_capacity    = 0;
    g_count       = 0;
    g_head        = 0;
    g_initialized = 0;
}
