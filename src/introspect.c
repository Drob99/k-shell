/* introspect.c — embedded OS-introspection mode (--inspect).
 *
 * Captures a kernel-eye view of every external command:
 *   - getrusage deltas: user/sys CPU, max RSS, page faults, ctx switches, FS I/O
 *   - wall-clock time (CLOCK_MONOTONIC)
 *   - inherited file descriptors (snapshotted via /dev/fd in the child between
 *     fork and exec, piped back to the parent)
 *   - decoded child exit status
 *
 * Default execution path (no --inspect token) is byte-for-byte unchanged: the
 * single static flag is only read by ks_execute when set, and is consumed
 * single-shot so a second non-inspected command runs the original code path.
 */

#define _DARWIN_C_SOURCE  /* dirfd, /dev/fd on Darwin */
#define _GNU_SOURCE       /* dirfd on glibc          */

#include "kshell.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

/* Single-shot flag: dispatcher sets it on --inspect, executor consumes. */
static int g_inspect_enabled = 0;

void ks_introspect_set(int enabled) {
    g_inspect_enabled = enabled ? 1 : 0;
}

int ks_introspect_consume(void) {
    int v = g_inspect_enabled;
    g_inspect_enabled = 0;
    return v;
}

int ks_introspect_strip(int *argc, char **argv) {
    if (argc == NULL || argv == NULL) return 0;
    int stripped = 0;
    int read_idx = 0;
    int write_idx = 0;
    while (read_idx < *argc) {
        if (argv[read_idx] != NULL && strcmp(argv[read_idx], "--inspect") == 0) {
            stripped++;
            read_idx++;
            continue;
        }
        argv[write_idx++] = argv[read_idx++];
    }
    *argc = write_idx;
    argv[write_idx] = NULL;  /* preserve execvp NULL-termination invariant */
    if (stripped > 0) ks_introspect_set(1);
    return stripped;
}

void ks_introspect_child_snapshot(int pipe_write_fd) {
    /* /dev/fd is provided by both Darwin and Linux (procfs symlink there).
     * If it can't be opened, write a sentinel so the parent doesn't block. */
    DIR *d = opendir("/dev/fd");
    if (d == NULL) {
        const char msg[] = "?";
        (void)write(pipe_write_fd, msg, sizeof(msg) - 1);
        close(pipe_write_fd);
        return;
    }

    int dir_fd = dirfd(d);
    char buf[KS_INSPECT_FD_BUF];
    size_t off = 0;
    struct dirent *ent;

    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        int fd = atoi(ent->d_name);
        /* Don't report the pipe write end or the directory's own fd —
         * those are bookkeeping the user didn't ask for. */
        if (fd == pipe_write_fd) continue;
        if (fd == dir_fd) continue;
        if (off + 8 >= sizeof(buf)) break;  /* leave room for one more + NUL */
        int n = snprintf(buf + off, sizeof(buf) - off, "%d ", fd);
        if (n < 0) break;
        if ((size_t)n >= sizeof(buf) - off) break;
        off += (size_t)n;
    }
    closedir(d);

    /* Trim trailing space if any. */
    if (off > 0 && buf[off - 1] == ' ') off--;
    (void)write(pipe_write_fd, buf, off);
    close(pipe_write_fd);
}

int ks_introspect_read_fds(int pipe_read_fd, char *fd_buf, size_t fd_buflen) {
    if (fd_buf == NULL || fd_buflen == 0) return KS_ERR_PARSE;
    fd_buf[0] = '\0';

    size_t total = 0;
    while (total < fd_buflen - 1) {
        ssize_t n = read(pipe_read_fd, fd_buf + total, fd_buflen - 1 - total);
        if (n == 0) break;                    /* EOF: child closed write end */
        if (n < 0) {
            if (errno == EINTR) continue;
            close(pipe_read_fd);
            return KS_ERR_EXEC;
        }
        total += (size_t)n;
    }
    fd_buf[total] = '\0';
    close(pipe_read_fd);
    return KS_OK;
}

static long timeval_to_us(const struct timeval *tv) {
    return (long)tv->tv_sec * 1000000L + (long)tv->tv_usec;
}

int ks_introspect_format(const struct rusage *delta,
                         long wallclock_us,
                         int exit_status,
                         const char *fd_list,
                         char *out_buf,
                         size_t buflen) {
    if (delta == NULL || out_buf == NULL || buflen == 0) return KS_ERR_PARSE;

    long user_us = timeval_to_us(&delta->ru_utime);
    long sys_us  = timeval_to_us(&delta->ru_stime);
    /* Darwin reports ru_maxrss in bytes; Linux reports it in kilobytes.
     * Normalize to KB so the column is meaningful across platforms. */
    long max_rss_kb = delta->ru_maxrss;
#ifdef __APPLE__
    max_rss_kb = (max_rss_kb + 1023) / 1024;
#endif
    const char *fds = (fd_list != NULL && fd_list[0] != '\0') ? fd_list : "(none)";

    int n = snprintf(out_buf, buflen,
        "  ----- kshell --inspect ------------------------------\n"
        "   wall-clock          : %10ld us\n"
        "   user CPU time       : %10ld us\n"
        "   system CPU time     : %10ld us\n"
        "   max RSS             : %10ld KB\n"
        "   minor page faults   : %10ld\n"
        "   major page faults   : %10ld\n"
        "   vol. ctx switches   : %10ld\n"
        "   invol. ctx switches : %10ld\n"
        "   FS block reads      : %10ld\n"
        "   FS block writes     : %10ld\n"
        "   child exit status   : %10d\n"
        "   inherited FDs       : %s\n"
        "  -----------------------------------------------------\n",
        wallclock_us, user_us, sys_us,
        max_rss_kb,
        delta->ru_minflt, delta->ru_majflt,
        delta->ru_nvcsw, delta->ru_nivcsw,
        delta->ru_inblock, delta->ru_oublock,
        exit_status,
        fds);

    if (n < 0 || (size_t)n >= buflen) return KS_ERR_EXEC;
    return KS_OK;
}
