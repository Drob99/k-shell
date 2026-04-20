#include <stdio.h>
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
 * Helpers
 * ---------------------------------------------------------------------- */

/* Build a space-separated string of n single-char tokens into buf. */
static void build_tokens(char *buf, size_t buflen, int n) {
    int pos = 0;
    for (int i = 0; i < n && (size_t)pos < buflen - 2; i++) {
        if (i > 0) buf[pos++] = ' ';
        buf[pos++] = 'x';
    }
    buf[pos] = '\0';
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

static void test_empty_string(void) {
    const char *name = "empty_string";
    char line[] = "";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)       { fail(name, "expected KS_OK"); return; }
    if (argc != 0)          { fail(name, "expected argc=0"); return; }
    if (argv[0] != NULL)    { fail(name, "expected argv[0]=NULL"); return; }
    pass(name);
}

static void test_whitespace_only(void) {
    const char *name = "whitespace_only";
    char line[] = "   \t  ";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)       { fail(name, "expected KS_OK"); return; }
    if (argc != 0)          { fail(name, "expected argc=0"); return; }
    if (argv[0] != NULL)    { fail(name, "expected argv[0]=NULL"); return; }
    pass(name);
}

static void test_single_token(void) {
    const char *name = "single_token";
    char line[] = "ls";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)                       { fail(name, "expected KS_OK"); return; }
    if (argc != 1)                          { fail(name, "expected argc=1"); return; }
    if (strcmp(argv[0], "ls") != 0)         { fail(name, "expected argv[0]=\"ls\""); return; }
    if (argv[argc] != NULL)                 { fail(name, "expected argv[argc]=NULL"); return; }
    pass(name);
}

static void test_two_tokens(void) {
    const char *name = "two_tokens";
    char line[] = "ls -la";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)                       { fail(name, "expected KS_OK"); return; }
    if (argc != 2)                          { fail(name, "expected argc=2"); return; }
    if (strcmp(argv[0], "ls") != 0)         { fail(name, "expected argv[0]=\"ls\""); return; }
    if (strcmp(argv[1], "-la") != 0)        { fail(name, "expected argv[1]=\"-la\""); return; }
    if (argv[argc] != NULL)                 { fail(name, "expected argv[argc]=NULL"); return; }
    pass(name);
}

static void test_mixed_spaces(void) {
    const char *name = "mixed_spaces";
    char line[] = "a  b   c \t d";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)                   { fail(name, "expected KS_OK"); return; }
    if (argc != 4)                      { fail(name, "expected argc=4"); return; }
    if (strcmp(argv[0], "a") != 0)      { fail(name, "expected argv[0]=\"a\""); return; }
    if (strcmp(argv[1], "b") != 0)      { fail(name, "expected argv[1]=\"b\""); return; }
    if (strcmp(argv[2], "c") != 0)      { fail(name, "expected argv[2]=\"c\""); return; }
    if (strcmp(argv[3], "d") != 0)      { fail(name, "expected argv[3]=\"d\""); return; }
    if (argv[argc] != NULL)             { fail(name, "expected argv[argc]=NULL"); return; }
    pass(name);
}

static void test_trailing_whitespace(void) {
    const char *name = "trailing_whitespace";
    char line[] = "ls -la   ";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)                       { fail(name, "expected KS_OK"); return; }
    if (argc != 2)                          { fail(name, "expected argc=2"); return; }
    if (strcmp(argv[0], "ls") != 0)         { fail(name, "expected argv[0]=\"ls\""); return; }
    if (strcmp(argv[1], "-la") != 0)        { fail(name, "expected argv[1]=\"-la\""); return; }
    if (argv[argc] != NULL)                 { fail(name, "expected argv[argc]=NULL"); return; }
    pass(name);
}

static void test_leading_whitespace(void) {
    const char *name = "leading_whitespace";
    char line[] = "   ls -la";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)                       { fail(name, "expected KS_OK"); return; }
    if (argc != 2)                          { fail(name, "expected argc=2"); return; }
    if (strcmp(argv[0], "ls") != 0)         { fail(name, "expected argv[0]=\"ls\""); return; }
    if (strcmp(argv[1], "-la") != 0)        { fail(name, "expected argv[1]=\"-la\""); return; }
    if (argv[argc] != NULL)                 { fail(name, "expected argv[argc]=NULL"); return; }
    pass(name);
}

static void test_exactly_max_args(void) {
    const char *name = "exactly_max_args";
    /* KS_MAX_ARGS single-char tokens separated by spaces: needs 2*N-1 chars + NUL */
    char line[KS_MAX_ARGS * 2];
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    build_tokens(line, sizeof(line), KS_MAX_ARGS);

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_OK)               { fail(name, "expected KS_OK"); return; }
    if (argc != KS_MAX_ARGS)        { fail(name, "expected argc=KS_MAX_ARGS"); return; }
    if (argv[argc] != NULL)         { fail(name, "expected argv[argc]=NULL"); return; }
    pass(name);
}

static void test_too_many_args(void) {
    const char *name = "too_many_args";
    char line[(KS_MAX_ARGS + 1) * 2];
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    build_tokens(line, sizeof(line), KS_MAX_ARGS + 1);

    int ret = ks_parse_line(line, argv, &argc);
    if (ret != KS_ERR_TOO_MANY_ARGS) { fail(name, "expected KS_ERR_TOO_MANY_ARGS"); return; }
    pass(name);
}

static void test_null_line(void) {
    const char *name = "null_line";
    char *argv[KS_MAX_ARGS + 1];
    int argc;

    int ret = ks_parse_line(NULL, argv, &argc);
    if (ret != KS_ERR_PARSE) { fail(name, "expected KS_ERR_PARSE"); return; }
    pass(name);
}

static void test_null_argv(void) {
    const char *name = "null_argv";
    char line[] = "ls";
    int argc;

    int ret = ks_parse_line(line, NULL, &argc);
    if (ret != KS_ERR_PARSE) { fail(name, "expected KS_ERR_PARSE"); return; }
    pass(name);
}

static void test_null_argc(void) {
    const char *name = "null_argc";
    char line[] = "ls";
    char *argv[KS_MAX_ARGS + 1];

    int ret = ks_parse_line(line, argv, NULL);
    if (ret != KS_ERR_PARSE) { fail(name, "expected KS_ERR_PARSE"); return; }
    pass(name);
}

/* -------------------------------------------------------------------------
 * Driver
 * ---------------------------------------------------------------------- */

int main(void) {
    test_empty_string();
    test_whitespace_only();
    test_single_token();
    test_two_tokens();
    test_mixed_spaces();
    test_trailing_whitespace();
    test_leading_whitespace();
    test_exactly_max_args();
    test_too_many_args();
    test_null_line();
    test_null_argv();
    test_null_argc();

    printf("\n%s: %d test(s) failed.\n", failures == 0 ? "OK" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
