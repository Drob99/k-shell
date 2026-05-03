#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
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
 * --inspect token stripping
 * ---------------------------------------------------------------------- */

static void test_strip_single_inspect(void) {
    const char *name = "strip_single_inspect";
    char *argv[5] = { "ls", "--inspect", "/tmp", NULL, NULL };
    int argc = 3;
    int n = ks_introspect_strip(&argc, argv);
    if (n != 1)                        { fail(name, "expected 1 stripped"); return; }
    if (argc != 2)                     { fail(name, "expected argc==2"); return; }
    if (strcmp(argv[0], "ls") != 0)    { fail(name, "expected argv[0]=ls"); return; }
    if (strcmp(argv[1], "/tmp") != 0)  { fail(name, "expected argv[1]=/tmp"); return; }
    if (argv[2] != NULL)               { fail(name, "expected argv[2]=NULL"); return; }
    if (!ks_introspect_consume())      { fail(name, "expected flag set"); return; }
    pass(name);
}

static void test_strip_no_inspect(void) {
    const char *name = "strip_no_inspect";
    char *argv[4] = { "ls", "-la", "/tmp", NULL };
    int argc = 3;
    int n = ks_introspect_strip(&argc, argv);
    if (n != 0)                        { fail(name, "expected 0 stripped"); return; }
    if (argc != 3)                     { fail(name, "expected argc==3"); return; }
    if (ks_introspect_consume())       { fail(name, "expected flag NOT set"); return; }
    pass(name);
}

static void test_strip_multiple_inspects(void) {
    const char *name = "strip_multiple_inspects";
    char *argv[6] = { "--inspect", "ls", "--inspect", "/tmp", NULL, NULL };
    int argc = 4;
    int n = ks_introspect_strip(&argc, argv);
    if (n != 2)                        { fail(name, "expected 2 stripped"); return; }
    if (argc != 2)                     { fail(name, "expected argc==2"); return; }
    if (strcmp(argv[0], "ls") != 0)    { fail(name, "expected argv[0]=ls"); return; }
    if (argv[2] != NULL)               { fail(name, "expected argv[2]=NULL"); return; }
    if (!ks_introspect_consume())      { fail(name, "expected flag set"); return; }
    pass(name);
}

static void test_strip_inspect_at_end(void) {
    const char *name = "strip_inspect_at_end";
    char *argv[4] = { "ls", "/tmp", "--inspect", NULL };
    int argc = 3;
    ks_introspect_strip(&argc, argv);
    if (argc != 2)                     { fail(name, "expected argc==2"); return; }
    if (argv[2] != NULL)               { fail(name, "expected argv[2]=NULL"); return; }
    if (!ks_introspect_consume())      { fail(name, "expected flag set"); return; }
    pass(name);
}

static void test_consume_clears_flag(void) {
    const char *name = "consume_clears_flag";
    ks_introspect_set(1);
    if (!ks_introspect_consume())  { fail(name, "first consume should be 1"); return; }
    if (ks_introspect_consume())   { fail(name, "second consume should be 0"); return; }
    pass(name);
}

/* -------------------------------------------------------------------------
 * Format function
 * ---------------------------------------------------------------------- */

static void test_format_basic(void) {
    const char *name = "format_basic";
    struct rusage ru = {0};
    ru.ru_utime.tv_sec = 0; ru.ru_utime.tv_usec = 1234;
    ru.ru_stime.tv_sec = 0; ru.ru_stime.tv_usec = 5678;
    ru.ru_maxrss = 4096;        /* KB on Linux, bytes on macOS */
    ru.ru_minflt = 12;
    ru.ru_majflt = 1;
    ru.ru_nvcsw  = 7;
    ru.ru_nivcsw = 3;
    char buf[KS_INSPECT_OUT_BUF];
    int rc = ks_introspect_format(&ru, 9999L, 0, "0 1 2", buf, sizeof(buf));
    if (rc != KS_OK)                              { fail(name, "expected KS_OK"); return; }
    if (strstr(buf, "kshell --inspect") == NULL)  { fail(name, "missing header"); return; }
    if (strstr(buf, "wall-clock") == NULL)        { fail(name, "missing wall-clock label"); return; }
    if (strstr(buf, "9999 us") == NULL)           { fail(name, "missing wall-clock value"); return; }
    if (strstr(buf, "1234 us") == NULL)           { fail(name, "missing user CPU value"); return; }
    if (strstr(buf, "5678 us") == NULL)           { fail(name, "missing system CPU value"); return; }
    if (strstr(buf, "0 1 2") == NULL)             { fail(name, "missing inherited FDs"); return; }
    pass(name);
}

static void test_format_empty_fds(void) {
    const char *name = "format_empty_fds";
    struct rusage ru = {0};
    char buf[KS_INSPECT_OUT_BUF];
    int rc = ks_introspect_format(&ru, 0, 0, "", buf, sizeof(buf));
    if (rc != KS_OK)                          { fail(name, "expected KS_OK"); return; }
    if (strstr(buf, "(none)") == NULL)        { fail(name, "expected (none) for empty fd list"); return; }
    pass(name);
}

static void test_format_null_fd_list(void) {
    const char *name = "format_null_fd_list";
    struct rusage ru = {0};
    char buf[KS_INSPECT_OUT_BUF];
    int rc = ks_introspect_format(&ru, 0, 0, NULL, buf, sizeof(buf));
    if (rc != KS_OK)                          { fail(name, "expected KS_OK"); return; }
    if (strstr(buf, "(none)") == NULL)        { fail(name, "expected (none) for NULL fd list"); return; }
    pass(name);
}

static void test_format_null_args_rejected(void) {
    const char *name = "format_null_args_rejected";
    struct rusage ru = {0};
    char buf[8];
    if (ks_introspect_format(NULL, 0, 0, "", buf, sizeof(buf)) != KS_ERR_PARSE) {
        fail(name, "expected KS_ERR_PARSE for NULL delta"); return;
    }
    if (ks_introspect_format(&ru, 0, 0, "", NULL, sizeof(buf)) != KS_ERR_PARSE) {
        fail(name, "expected KS_ERR_PARSE for NULL out_buf"); return;
    }
    if (ks_introspect_format(&ru, 0, 0, "", buf, 0) != KS_ERR_PARSE) {
        fail(name, "expected KS_ERR_PARSE for buflen=0"); return;
    }
    pass(name);
}

static void test_format_buf_too_small(void) {
    const char *name = "format_buf_too_small";
    struct rusage ru = {0};
    char buf[16]; /* far too small for the full table */
    int rc = ks_introspect_format(&ru, 0, 0, "", buf, sizeof(buf));
    if (rc != KS_ERR_EXEC) { fail(name, "expected KS_ERR_EXEC for tiny buffer"); return; }
    pass(name);
}

/* -------------------------------------------------------------------------
 * Driver
 * ---------------------------------------------------------------------- */

int main(void) {
    test_strip_single_inspect();
    test_strip_no_inspect();
    test_strip_multiple_inspects();
    test_strip_inspect_at_end();
    test_consume_clears_flag();
    test_format_basic();
    test_format_empty_fds();
    test_format_null_fd_list();
    test_format_null_args_rejected();
    test_format_buf_too_small();

    printf("\n%s: %d test(s) failed.\n", failures == 0 ? "OK" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
