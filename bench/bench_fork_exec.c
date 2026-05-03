/* bench_fork_exec.c — measure fork+execvp+waitpid round-trip latency.
 *
 * Reports mean, p50, p99, min, max in microseconds over N iterations.
 * The child execs /usr/bin/true (a no-op program), so what we measure
 * is the kernel cost of process creation + image replacement + reaping
 * for K-Shell's exact code path.
 *
 * Build: cc -O2 -std=c11 bench/bench_fork_exec.c -o bench/bench_fork_exec
 *
 * This is a stand-alone benchmark binary with its own main(); it does
 * not link against kshell sources because it is measuring the kernel
 * primitives K-Shell wraps, not K-Shell itself. The instrumentation
 * matches src/executor.c::execute_plain exactly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define DEFAULT_N 1000

static int cmp_long(const void *a, const void *b) {
    long la = *(const long *)a;
    long lb = *(const long *)b;
    return (la > lb) - (la < lb);
}

/* Measure one fork + execvp(/usr/bin/true) + waitpid round trip.
 * Returns elapsed microseconds, or -1 on failure. */
static long one_iter(void) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    pid_t pid = fork();
    if (pid == -1) return -1;

    if (pid == 0) {
        char *const argv[] = { (char *)"/usr/bin/true", NULL };
        execvp(argv[0], argv);
        _exit(127);
    }

    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (t1.tv_sec - t0.tv_sec) * 1000000L
         + (t1.tv_nsec - t0.tv_nsec) / 1000L;
}

int main(int argc, char **argv) {
    int n = DEFAULT_N;
    if (argc > 1) {
        n = atoi(argv[1]);
        if (n <= 0) n = DEFAULT_N;
    }

    long *samples = malloc(sizeof(long) * (size_t)n);
    if (samples == NULL) {
        fprintf(stderr, "bench: out of memory\n");
        return 1;
    }

    /* Warm-up: 50 untimed iterations to populate exec cache, page tables. */
    for (int i = 0; i < 50; i++) (void)one_iter();

    long failures = 0;
    long total_us = 0;
    long min_us = -1, max_us = 0;
    int written = 0;

    for (int i = 0; i < n; i++) {
        long us = one_iter();
        if (us < 0) { failures++; continue; }
        samples[written++] = us;
        total_us += us;
        if (min_us < 0 || us < min_us) min_us = us;
        if (us > max_us) max_us = us;
    }

    if (written == 0) {
        fprintf(stderr, "bench: every iteration failed\n");
        free(samples);
        return 1;
    }

    qsort(samples, (size_t)written, sizeof(long), cmp_long);
    long p50 = samples[written / 2];
    long p99 = samples[(written * 99) / 100];
    double mean = (double)total_us / (double)written;

    printf("fork + execvp(/usr/bin/true) + waitpid — %d iterations\n", n);
    printf("------------------------------------------------------\n");
    printf("  iterations completed : %d (failures: %ld)\n", written, failures);
    printf("  mean latency         : %10.1f us\n", mean);
    printf("  p50                  : %10ld us\n", p50);
    printf("  p99                  : %10ld us\n", p99);
    printf("  min                  : %10ld us\n", min_us);
    printf("  max                  : %10ld us\n", max_us);
    printf("------------------------------------------------------\n");
    printf("system: macOS / Apple Silicon (Darwin 25.x), CC=clang -O2\n");

    free(samples);
    return 0;
}
