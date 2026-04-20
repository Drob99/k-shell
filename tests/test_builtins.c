#include <stdio.h>
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
 * Tests
 * ---------------------------------------------------------------------- */

static void test_cd_too_many_args(void) {
    const char *name = "cd_too_many_args";
    char *argv[] = { "cd", "a", "b", NULL };
    int ret = ks_builtin_cd(3, argv);
    if (ret != KS_ERR_BUILTIN) { fail(name, "expected KS_ERR_BUILTIN"); return; }
    pass(name);
}

static void test_exit_returns_exit_code(void) {
    const char *name = "exit_returns_exit_code";
    char *argv[] = { "exit", NULL };
    int ret = ks_builtin_exit(1, argv);
    if (ret != KS_EXIT)             { fail(name, "expected KS_EXIT"); return; }
    if (ks_builtin_exit_code() != 0) { fail(name, "expected exit code 0"); return; }
    pass(name);
}

static void test_exit_with_code(void) {
    const char *name = "exit_with_code";
    char *argv[] = { "exit", "42", NULL };
    int ret = ks_builtin_exit(2, argv);
    if (ret != KS_EXIT)              { fail(name, "expected KS_EXIT"); return; }
    if (ks_builtin_exit_code() != 42) { fail(name, "expected exit code 42"); return; }
    pass(name);
}

static void test_exit_too_many_args(void) {
    const char *name = "exit_too_many_args";
    char *argv[] = { "exit", "1", "2", NULL };
    int ret = ks_builtin_exit(3, argv);
    if (ret != KS_ERR_BUILTIN) { fail(name, "expected KS_ERR_BUILTIN, not KS_EXIT"); return; }
    pass(name);
}

/* -------------------------------------------------------------------------
 * Driver
 * ---------------------------------------------------------------------- */

int main(void) {
    test_cd_too_many_args();
    test_exit_returns_exit_code();
    test_exit_with_code();
    test_exit_too_many_args();

    printf("\n%s: %d test(s) failed.\n", failures == 0 ? "OK" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
