#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kshell.h"

static int failures = 0;

static void pass(const char *name) {
    printf("PASS: %s\n", name);
}

static void fail(const char *name, const char *reason) {
    printf("FAIL: %s — %s\n", name, reason);
    failures++;
}

/* -------------------------------------------------------------------------
 * Setup helper — guarantees clean state regardless of prior test.
 * ---------------------------------------------------------------------- */
static void reset_history(size_t capacity) {
    ks_history_free(); /* safe no-op if not initialized */
    int ret = ks_history_init(capacity);
    if (ret != KS_OK) {
        fprintf(stderr, "FATAL: ks_history_init failed in test setup\n");
        exit(2);
    }
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

static void test_history_init_twice(void) {
    const char *name = "history_init_twice";
    reset_history(3);
    int ret = ks_history_init(3);
    if (ret != KS_ERR_HISTORY) { fail(name, "expected KS_ERR_HISTORY on second init"); goto done; }
    pass(name);
done:
    ks_history_free();
}

static void test_history_add_and_get(void) {
    const char *name = "history_add_and_get";
    reset_history(3);

    if (ks_history_add("a") != KS_OK)   { fail(name, "add 'a' failed"); goto done; }
    if (ks_history_add("b") != KS_OK)   { fail(name, "add 'b' failed"); goto done; }
    if (ks_history_count() != 2)        { fail(name, "expected count=2"); goto done; }
    if (strcmp(ks_history_get(0), "a") != 0) { fail(name, "expected get(0)='a'"); goto done; }
    if (strcmp(ks_history_get(1), "b") != 0) { fail(name, "expected get(1)='b'"); goto done; }
    pass(name);
done:
    ks_history_free();
}

static void test_history_empty_lines_skipped(void) {
    const char *name = "history_empty_lines_skipped";
    reset_history(3);

    if (ks_history_add("") != KS_OK)    { fail(name, "add '' returned error"); goto done; }
    if (ks_history_add("   ") != KS_OK) { fail(name, "add '   ' returned error"); goto done; }
    if (ks_history_count() != 0)        { fail(name, "expected count=0"); goto done; }
    pass(name);
done:
    ks_history_free();
}

static void test_history_consecutive_duplicates_skipped(void) {
    const char *name = "history_consecutive_duplicates_skipped";
    reset_history(3);

    if (ks_history_add("ls") != KS_OK) { fail(name, "first add failed"); goto done; }
    if (ks_history_add("ls") != KS_OK) { fail(name, "second add returned error"); goto done; }
    if (ks_history_count() != 1)       { fail(name, "expected count=1"); goto done; }
    pass(name);
done:
    ks_history_free();
}

static void test_history_wraparound(void) {
    const char *name = "history_wraparound";
    reset_history(3);

    ks_history_add("a");
    ks_history_add("b");
    ks_history_add("c");
    ks_history_add("d");
    ks_history_add("e");

    if (ks_history_count() != 3)               { fail(name, "expected count=3"); goto done; }
    if (strcmp(ks_history_get(0), "c") != 0)   { fail(name, "expected get(0)='c'"); goto done; }
    if (strcmp(ks_history_get(1), "d") != 0)   { fail(name, "expected get(1)='d'"); goto done; }
    if (strcmp(ks_history_get(2), "e") != 0)   { fail(name, "expected get(2)='e'"); goto done; }
    pass(name);
done:
    ks_history_free();
}

static void test_history_get_out_of_range(void) {
    const char *name = "history_get_out_of_range";
    reset_history(3);

    ks_history_add("x");

    if (ks_history_get(-1) != NULL)  { fail(name, "expected NULL for index -1"); goto done; }
    if (ks_history_get(1)  != NULL)  { fail(name, "expected NULL for index 1 (count=1)"); goto done; }
    pass(name);
done:
    ks_history_free();
}

/* -------------------------------------------------------------------------
 * Driver
 * ---------------------------------------------------------------------- */

int main(void) {
    test_history_init_twice();
    test_history_add_and_get();
    test_history_empty_lines_skipped();
    test_history_consecutive_duplicates_skipped();
    test_history_wraparound();
    test_history_get_out_of_range();

    printf("\n%s: %d test(s) failed.\n", failures == 0 ? "OK" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
