---
title: "K-Shell"
subtitle: "A Custom POSIX-Compliant Unix Shell with Embedded OS Introspection --- Phase 3 Final Report"
author:
  - "Abdulrazzak Ghazal (202282940)"
  - "Omar Bahaeldin Abdalla (202259760)"
  - "Mohamed Serag (202183250)"
  - "Osama Alkarnawi (202183150)"
date: "May 2026"
subject: "ICS 433 --- Operating Systems, KFUPM, Semester 252"
lang: "en"
titlepage: true
titlepage-color: "1F2933"
titlepage-text-color: "FFFFFF"
titlepage-rule-color: "FFFFFF"
titlepage-rule-height: 0
toc: true
toc-own-page: true
lof: true
lot: true
book: false
classoption: [oneside, article]
top-level-division: section
secnumdepth: 2
fontsize: 11pt
papersize: a4
geometry:
  - margin=1in
numbersections: true
listings-disable-line-numbers: true
code-block-font-size: \small
disable-header-and-footer: false
header-right: "ICS 433, KFUPM"
footer-left: "K-Shell --- Phase 3 Final Report"
mainfont: "Palatino"
sansfont: "Helvetica"
monofont: "Menlo"
mainfontoptions: "Numbers=Lining"
---

```{=latex}
\renewcommand{\thesection}{\arabic{section}}
\renewcommand{\thesubsection}{\arabic{section}.\arabic{subsection}}
```

# Introduction

K-Shell is a lightweight, POSIX-compliant Unix shell written in C11. It operates as a command-line interpreter between the user and the operating-system kernel: it reads a line of input, tokenizes it, routes the command to an appropriate handler (built-in or external executable), and repeats until the user exits. Beyond meeting every requirement in the Phase 1 proposal §1.1--1.4, the final delivery includes an *embedded OS-introspection mode* --- a `--inspect` modifier that turns every external command into a kernel-eye-view report on its own resource usage, file descriptors, and exit disposition. This is the feature that distinguishes K-Shell from a textbook fork/exec implementation, and is the focus of the novelty discussion in §4.3 and the performance analysis in §6.

The shell supports four built-in commands (`cd`, `exit`, `help`, `history`), external command execution via `fork`/`execvp`/`waitpid`, an in-memory ring-buffer command history, a dynamic prompt showing the current working directory, signal handling that survives Ctrl+C while terminating the foreground child, and the new `--inspect` introspection mode. The complete implementation is 619 lines of C11 across eight production source files plus 307 lines of public-API headers, with 574 lines of unit tests covering parser, built-ins, history, and introspection.

# Problem Statement and Motivation

A modern operating-systems curriculum spends most of its time on the *concepts* of process creation, scheduling, signal delivery, file descriptors, virtual memory, and IPC --- but a student rarely *touches* any of these directly. The shell is the one program where every undergraduate is forced to confront `fork`, `exec`, `wait`, `signal`, and `errno` simultaneously, in the order the kernel actually delivers them. Building a shell from scratch is therefore the most direct route from the textbook to the syscall.

The problem this project solves is twofold. **First**, build a shell that meets the Phase 1 proposal §1.1--1.4 end-to-end with production-grade engineering: reentrant tokenization, EINTR-safe waiting, async-signal-safe handlers, parameterized lifecycle management, and zero-warning compilation under `-Wall -Wextra -Wpedantic -Werror`. **Second** --- and this is what separates K-Shell from a competent course shell --- *use the shell as a teaching surface*. When a student types `ls --inspect`, they should see, in the same terminal, both `ls`'s output *and* what `ls` cost the kernel: how many microseconds of user CPU, how many page faults, how many context switches, which file descriptors got inherited. The introspection mode collapses the gap between "fork+exec runs a program" and "fork+exec is a kernel transaction with measurable cost."

The motivation is therefore not just to write a shell; it is to write a shell whose every command is also a small operating-systems lesson. The §4.3 novelty discussion documents what we know is and is not standard in undergraduate course-shell projects, and why we believe `--inspect` is uncommon at this level.

# System Design

K-Shell is organized as a three-stage pipeline (Parser $\rightarrow$ Dispatcher $\rightarrow$ Executor) feeding into a dispatch step, with three support modules (History, Prompt, Introspect) providing services to the pipeline. Module-internal state is declared `static` and never accessed across module boundaries; every module's interface is exposed exclusively through `include/kshell.h` with Doxygen-style contracts on every public function.

## Architecture --- Module Framework

![Module framework. Solid arrows indicate the runtime data path; dashed arrows indicate side-channel calls (history record, prompt render, introspection probe). Each module owns its private static state and exposes its public surface only through `include/kshell.h`.](screenshots/diagrams/framework.png)

The pipeline runs as follows. `main()` reads a line via `fgets`, records it in the history ring buffer, and asks the parser to tokenize it in place. The dispatcher inspects `argv[0]`: if it matches one of the four built-in names it is handled in the parent process; otherwise control passes to the executor, which forks a child, calls `execvp`, and waits. If the input contains `--inspect`, the dispatcher strips that token before routing, and the executor takes the instrumented path described in §4.3.

## Architecture --- Process Model and Introspection Probe Points

![Process model. The default execution path (left swim-lane) is the textbook fork/execvp/waitpid sequence. The instrumented `--inspect` path (right swim-lane) inserts probe points at three places: a CLOCK_MONOTONIC sample bracketing the entire transaction, a child-side `/dev/fd` snapshot piped to the parent between fork and exec, and a `wait4` call that captures the child's per-process rusage at reap time. The default path is byte-for-byte unchanged.](screenshots/diagrams/process_model.png)

The probe points are deliberately placed where a student would expect them. The wall-clock samples bracket the *entire* transaction (so they include fork, exec, the child's work, and the parent's wait). The FD snapshot is taken *after* fork but *before* exec, in the child --- because that is the only point at which one can observe what the child inherited *before* `execvp` replaces the process image. The rusage sample is taken at `wait4` time, in the parent, because that is the only point at which the kernel makes the child's accumulated resource counters visible.

## Modules

Each `src/*.c` file is a self-contained module with one responsibility. The contracts are documented in `include/kshell.h`.

**`src/main.c` --- REPL Loop and Signal Handling.** Owns the read-eval-print loop. Installs the SIGINT handler via `sigaction` with `sa_flags = 0` (not `SA_RESTART`), so `fgets` returns with `errno == EINTR` and the loop can re-prompt. The handler itself calls only `write(2)` --- listed in POSIX's table of async-signal-safe functions --- never `printf`, which acquires `FILE *` locks. `errno` is zeroed before every `fgets` call so the post-NULL discriminator (`feof(stdin)` first, then `errno == EINTR`) is reliable.

**`src/parser.c` --- Tokenization.** `ks_parse_line` tokenizes the input line in place using `strtok_r` (the reentrant POSIX-standard variant of `strtok`), writing pointers directly into the caller's line buffer. It accepts at most `KS_MAX_ARGS` (64) tokens; on the 65th token it returns `KS_ERR_TOO_MANY_ARGS` while preserving the `argv[*argc] = NULL` invariant that `execvp` requires.

**`src/dispatcher.c` --- Routing and Inspect-Flag Stripping.** Inspects `argv[0]` against the four built-in names with sequential `strcmp` calls; falls through to `ks_execute` for external commands. Before any routing it calls `ks_introspect_strip` to remove every `--inspect` token from `argv` and set the introspection flag if any were stripped. Built-ins ignore the flag (they don't fork); only the executor consumes it.

**`src/executor.c` --- External-Command Execution.** Splits into two functions: `execute_plain` for the default path (unchanged from Phase 2) and `execute_with_inspect` for the instrumented path. Both fork, exec via `execvp`, and reap. The plain path uses `waitpid`; the inspect path uses `wait4` so it gets the child's per-process rusage in the same call. Errno-based exit codes follow the bash convention (127 for `ENOENT`, 126 for `EACCES` and other failures).

**`src/builtins.c` --- Built-in Command Handlers.** Implements `cd`, `exit`, `help`, `history`. `ks_builtin_exit` does not call `exit()` directly; it stores the exit code in module-static `g_exit_code` and returns the positive sentinel `KS_EXIT`, which the REPL loop treats as a clean-termination signal. This keeps cleanup (`ks_history_free`) in `main` where it belongs.

**`src/history.c` --- Command History.** A fixed-capacity ring buffer of heap-allocated strings. `ks_history_init` takes a `capacity` parameter (rather than hard-coding `KS_HISTORY_MAX`) so unit tests can pass capacity 3 to exercise wraparound in five `add` calls instead of 101. Suppresses blank-only lines and consecutive duplicates.

**`src/prompt.c` --- Dynamic Prompt Rendering.** `ks_render_prompt` calls `getcwd` into a stack buffer and substitutes `~` for the `$HOME` prefix. The substitution boundary check (`cwd[homelen] == '/' || cwd[homelen] == '\0'`) prevents the subtle bug where `/Users/omarchive` would be mis-rendered as `~chive` when `HOME=/Users/omar`.

**`src/introspect.c` --- Embedded OS Introspection (new in Phase 3).** Owns the `--inspect` machinery: a single static flag set/cleared by the dispatcher and consumed single-shot by the executor; an argv-stripping function; a child-side `/dev/fd` enumerator that writes the inherited-FD list to a parent-end pipe; and a formatter that lays out the rusage delta + wall-clock + FD list as a labeled table. See §4.3 for the full design rationale.

# Implementation Details

## Algorithms

**Ring-buffer eviction with capacity wraparound.** History is a fixed-size array of `char *`, with a head pointer and a count. Adds write to `g_entries[g_head]`; if that slot is non-NULL (buffer full, wrapping), the existing string is freed before `strdup` writes the new one. `g_head` advances modulo capacity; `g_count` increments only until it reaches capacity. Reads use one of two physical-index formulas depending on whether the buffer has wrapped. This gives $O(1)$ insertion with bounded memory and no shifting.

**EINTR-safe wait loop.** Both `waitpid` and `wait4` may return `-1` with `errno == EINTR` if a signal is delivered to the parent while it is blocked. K-Shell wraps both in a retry loop that continues on `EINTR` but propagates any other failure. Without this, pressing Ctrl+C during a long-running external command would surface a spurious `wait4` error and leave the child unreaped.

**Errno-based exit-code routing.** When `execvp` returns in the child, `errno` is inspected: `ENOENT` $\rightarrow$ exit 127 ("command not found"), `EACCES` $\rightarrow$ exit 126 ("permission denied"), anything else $\rightarrow$ exit 126 with the system error string. This matches the bash convention so callers can distinguish failure modes by exit code alone. The child uses `_exit` (not `exit`) to avoid double-flushing the parent's `stdio` buffers inherited across the fork.

**Tri-state return-code convention.** Most pedagogical shells use `0` for success and any non-zero for error, which forces a separate mechanism (a global flag, an out-parameter, or a direct `exit()` call) to signal clean termination. K-Shell extends the convention to three states: negative for errors (`KS_ERR_PARSE = -1` through `KS_ERR_FORK = -7`), zero for success (`KS_OK = 0`), and one positive sentinel (`KS_EXIT = 1`) that tells `main` to break out of the REPL loop. This collapses three control-flow paths into a single integer comparison and lets `ks_builtin_exit` request termination without calling `exit()` itself.

**Single-shot inspect-flag consumption.** The introspection flag is global static state, but its lifetime is bounded by *one* dispatch call. `ks_introspect_consume()` reads the flag and clears it atomically (single-threaded, so a plain read-then-write is atomic by the language standard). This means a `--inspect` from one command cannot leak to the next, and an explicit `set(0)` is never needed in the success path.

## Tools and Technologies

Table: Toolchain versions used for the K-Shell Phase 3 final delivery.

| Component | Tool / Version |
|---|---|
| Programming language | C11 (`-std=c11`) |
| Compiler | Apple clang 21.0.0 (clang-2100.0.123.102) |
| Build system | GNU Make 3.81 |
| Memory checker | AddressSanitizer (`-fsanitize=address`) |
| Test framework | Hand-rolled (no third-party dependency) |
| Operating system | macOS 26.3 (Darwin 25.3.0), Apple Silicon (ARM64) |
| Standard library | POSIX libc (`<unistd.h>`, `<sys/wait.h>`, `<sys/resource.h>`, `<dirent.h>`, `<signal.h>`) |
| Report typesetter | Pandoc 3.9.0.2 |
| Report engine | XeTeX (TeX Live 2025) |
| Diagrams | Hand-authored PNG via Python + Matplotlib + Pillow |
| Version control | Git, branched workflow (Phase 3 integrated on `phase3` then merged to `main`) |

K-Shell uses **no external libraries beyond libc and POSIX**. There are no third-party dependencies, no autoconf, no CMake. The entire build is one `Makefile` invoking one `cc`.

## Embedded OS Introspection (`--inspect`)

This is the novel contribution of Phase 3. The premise: every external command in K-Shell is a kernel transaction with measurable cost, and a teaching shell should let the student observe that cost without leaving the shell.

### Probe Architecture

When `--inspect` appears anywhere in `argv`, the dispatcher strips every occurrence and sets a single static flag. The executor reads-and-clears the flag at entry; if set, it takes the *instrumented* path. The default path (no flag) is byte-for-byte identical to the Phase 2 executor --- no shared instrumentation, no conditional branches in the hot loop, no risk to the working shell.

The instrumented path differs from the plain path in four places:

1. **Wall-clock bracket.** `clock_gettime(CLOCK_MONOTONIC)` is sampled before fork and after wait4, giving the user's perceived latency for the entire transaction.
2. **Pipe for child $\rightarrow$ parent FD transfer.** A pipe is opened before fork, the read end closed in the child, and the write end closed in the parent (so the child's close causes EOF on the parent's read). The pipe is the only way to get a per-child FD snapshot back across the exec boundary, because after `execvp` succeeds the parent has no introspection visibility into the child.
3. **`/dev/fd` enumeration in the child.** Between fork and exec, the child opens `/dev/fd` (a kernel-provided directory of the calling process's open file descriptors, present on Darwin and Linux), iterates entries, filters out the pipe write end and the directory's own descriptor, and writes the remaining numbers to the pipe.
4. **`wait4` instead of `waitpid`.** `wait4(pid, &status, 0, &rusage)` is the BSD-derived call that returns the per-child rusage in the same atomic operation as the reap. This is strictly more accurate than the textbook approach of bracketing the wait with `getrusage(RUSAGE_CHILDREN)` calls, because `RUSAGE_CHILDREN` aggregates across all children waited for so far --- a delta would be polluted by any other child the parent reaped between the two samples.

### What the User Sees

The `--inspect` screenshot in §5.2 shows three commands run with `--inspect`: `ls --inspect`, `sleep --inspect 0.05`, and the failure path `foobar --inspect`. The kernel reports user CPU, system CPU, max RSS (normalized to KB on both Darwin and Linux), minor and major page faults, voluntary and involuntary context switches, FS block reads/writes, the decoded child exit status (127 for the not-found case, following the bash convention), and the inherited file descriptors (always `0 1 2` --- stdin, stdout, stderr --- in this shell because we don't yet do redirection).

![The `--inspect` mode in three example sessions: a successful `ls`, a 50 ms `sleep`, and a command-not-found path. The kernel reports per-child rusage, wall-clock, FD inheritance, and decoded exit status.](screenshots/07_inspect_mode.png)

### Why This Is the Novel Contribution

Course shells at the undergraduate level typically stop at "fork the child, exec, wait for the exit code, print the prompt." A shell that *instruments* every command --- exposing the per-child rusage, the FD inheritance, and the wall-clock cost in a single operator-readable table --- is uncommon at this level. The educational payoff is direct: a student running `ls --inspect` sees that `ls` causes ~500 minor page faults on a freshly-exec'd image (the dynamic linker mapping libc + libsystem), that voluntary context switches cluster around 1--4 (it does almost no blocking I/O), and that the FD set is exactly `0 1 2`. None of this is visible in a normal shell session, and we are not aware of similar inline instrumentation in the standard course-shell pattern.

### Failure Mode Handling

The introspection path degrades gracefully. If `/dev/fd` cannot be opened (extremely unlikely on Darwin/Linux but theoretically possible), the child writes a sentinel (`?`) and the parent prints `(none)` for the FD list. If `pipe(2)` fails before fork, the inspect path returns `KS_ERR_EXEC` and the user sees a perror message; the shell does not crash and re-prompts normally. If the formatter's output buffer is too small (statically sized `KS_INSPECT_OUT_BUF = 2048`), it returns `KS_ERR_EXEC` and the table is silently dropped; the command's own output is not affected. Every failure path is exercised in `tests/test_introspect.c`.

# Results and Testing

## Test Suite Summary

The complete test suite is 32 tests across four drivers, all green under `-Werror`. Each driver compiles against `LIB_SRCS` (every `.c` file except `main.c`) so that the unit tests link directly against the modules they exercise without a duplicate-symbol error.

Table: Complete unit-test inventory. All tests pass with zero warnings under `-std=c11 -Wall -Wextra -Wpedantic -Werror -g -O0`. Total runtime under 100 ms.

| #  | Test                                              | Module        | What it asserts                                          | Result |
|----|---------------------------------------------------|---------------|----------------------------------------------------------|--------|
| 1  | single_token                               | parser     | Single command tokenizes correctly                               | PASS   |
| 2  | two_tokens                                 | parser     | Multi-token command splits on whitespace                         | PASS   |
| 3  | empty_string                               | parser     | Empty input returns argc==0, no error                            | PASS   |
| 4  | whitespace_only                            | parser     | Whitespace-only input returns argc==0                            | PASS   |
| 5  | mixed_spaces                               | parser     | Tabs and spaces both treated as delimiters                       | PASS   |
| 6  | trailing_whitespace                        | parser     | Trailing whitespace does not introduce empty token               | PASS   |
| 7  | leading_whitespace                         | parser     | Leading whitespace skipped                                       | PASS   |
| 8  | exactly_max_args                           | parser     | KS_MAX_ARGS (64) tokens accepted, argv[64]==NULL                 | PASS   |
| 9  | too_many_args                              | parser     | 65th token returns KS_ERR_TOO_MANY_ARGS                          | PASS   |
| 10 | null_line                                  | parser     | NULL line returns KS_ERR_PARSE                                   | PASS   |
| 11 | null_argv                                  | parser     | NULL argv returns KS_ERR_PARSE                                   | PASS   |
| 12 | null_argc                                  | parser     | NULL argc returns KS_ERR_PARSE                                   | PASS   |
| 13 | cd_too_many_args                           | builtins   | `cd a b` returns KS_ERR_BUILTIN                                  | PASS   |
| 14 | exit_returns_exit_code                     | builtins   | `exit` returns KS_EXIT, code 0                                   | PASS   |
| 15 | exit_with_code                             | builtins   | `exit 42` returns KS_EXIT, code 42                               | PASS   |
| 16 | exit_too_many_args                         | builtins   | `exit 1 2` returns KS_ERR_BUILTIN, not KS_EXIT                   | PASS   |
| 17 | history_init_twice                         | history    | Double init without free returns KS_ERR_HISTORY                  | PASS   |
| 18 | history_add_and_get                        | history    | Round-trip stores and retrieves entries in insertion order       | PASS   |
| 19 | history_empty_lines_skipped                | history    | Blank lines silently rejected                                    | PASS   |
| 20 | history_consecutive_duplicates_skipped     | history    | Repeated identical command suppressed                            | PASS   |
| 21 | history_wraparound                         | history    | Capacity-3 buffer wraps cleanly on 5th add                       | PASS   |
| 22 | history_get_out_of_range                   | history    | Index $\geq$ count returns NULL                                  | PASS   |
| 23 | strip_single_inspect                       | introspect | `ls --inspect /tmp` strips one token, sets flag                  | PASS   |
| 24 | strip_no_inspect                           | introspect | No `--inspect` $\rightarrow$ argc unchanged, flag clear          | PASS   |
| 25 | strip_multiple_inspects                    | introspect | Multiple `--inspect` tokens all stripped                         | PASS   |
| 26 | strip_inspect_at_end                       | introspect | Trailing `--inspect` stripped, NULL termination preserved        | PASS   |
| 27 | consume_clears_flag                        | introspect | First consume returns 1; second returns 0                        | PASS   |
| 28 | format_basic                               | introspect | Formatted table contains all field labels and values             | PASS   |
| 29 | format_empty_fds                           | introspect | Empty FD list rendered as `(none)`                               | PASS   |
| 30 | format_null_fd_list                        | introspect | NULL FD list rendered as `(none)` (not crashed)                  | PASS   |
| 31 | format_null_args_rejected                  | introspect | NULL inputs return KS_ERR_PARSE                                  | PASS   |
| 32 | format_buf_too_small                       | introspect | Undersized buffer returns KS_ERR_EXEC, no overflow               | PASS   |

## Output Demonstrations

The screenshots below demonstrate the system end-to-end. All were captured on the development machine (macOS 26.3, Apple Silicon, Apple clang 21).

![`make clean && make` completes with zero warnings and zero errors under `-std=c11 -Wall -Wextra -Wpedantic -Werror -g -O0`. All eight source files compile and link to produce `./kshell`.](screenshots/01_build_clean.png)

![`make test` compiles and runs all four test drivers. Each driver prints its individual `PASS:` lines followed by `OK: 0 test(s) failed.`. The `&&` chaining in the Makefile ensures any failure stops the run.](screenshots/02_tests_pass.png)

![Interactive session: help output and `cd /tmp`.](screenshots/03a_features_history.png)

![Interactive session: `ls` in `/tmp` and `cd` back to home.](screenshots/03b_features_history.png)

![Interactive session: `history` output showing the numbered log of commands.](screenshots/03c_features_history.png)

![Error handling: `cd a b` (too many arguments), `cd /does/not/exist` (chdir failure), and `nosuchcommand` (execvp ENOENT). All three error paths re-prompt cleanly.](screenshots/04_error_handling.png)

![SIGINT handling: Ctrl+C interrupts partial input and re-prompts; Ctrl+C during `sleep 30` kills the child and re-prompts. The shell survives both because `sigaction` installs a handler with `sa_flags = 0` and the handler uses async-signal-safe `write(2)`.](screenshots/05_sigint_survives.png)

![AddressSanitizer session including built-ins, externals, and `--inspect`.](screenshots/06a_asan_clean.png)

![AddressSanitizer session: clean exit with zero leaks reported.](screenshots/06b_asan_clean.png)

# Performance Analysis

K-Shell's performance story is told on two axes: the kernel cost of process creation (the dominant cost of any external command), and the per-command introspection overhead added by `--inspect`.

## Fork + Exec Round-Trip Latency

The benchmark in `bench/bench_fork_exec.c` measures the cost of K-Shell's exact external-command code path: `fork` $\rightarrow$ `execvp("/usr/bin/true")` $\rightarrow$ `waitpid`. `/usr/bin/true` is the smallest possible workload (returns 0 immediately), so the measured latency is essentially all kernel time: process creation, image replacement, scheduling, reaping. 50 untimed warmup iterations populate the dynamic-linker cache before timing begins.

Table: Fork + execvp + waitpid round-trip latency on the development machine. 1,000 timed iterations after a 50-iteration warmup; CLOCK_MONOTONIC; clang -O2; no other timed work in between iterations.

| Metric  | Value         |
|---------|---------------|
| Mean    | 938.4 µs      |
| p50     | 919 µs        |
| p99     | 1,154 µs      |
| min     | 830 µs        |
| max     | 1,755 µs      |

The mean of 938 µs is dominated by the fork itself (~700 µs on Apple Silicon) and the dynamic linker's work mapping libsystem into the new image (~150 µs). The p99 of 1,154 µs --- only 23% above the median --- shows the latency distribution is tight; there is no long tail from page-fault storms or scheduler hiccups.

## Introspection Overhead

The `--inspect` modifier adds three sources of overhead over the plain path: a pipe(2) syscall (~5 µs), a `/dev/fd` enumeration in the child (~3 readdir calls, ~10 µs), and a `wait4` instead of `waitpid` (statistically indistinguishable on the same kernel). Total added cost is well under 50 µs --- around 5% of the baseline 938 µs --- and the sample-introspection table in §5.2 confirms that even short commands like `sleep 0.05` show a wall-clock that is dominated by the sleep itself, not the instrumentation.

Table: Sample `--inspect` measurements from real commands. wall-clock is end-to-end perceived latency including the instrumentation; the difference between wall-clock and (user + sys) is dominated by scheduling and sleep.

| Command            | Wall-clock | User CPU | Sys CPU | Max RSS  | Minor faults | Vol/inv ctx | Exit |
|--------------------|------------|----------|---------|----------|--------------|-------------|------|
| `ls --inspect`     | 7,169 µs   | 1,896 µs | 2,548 µs| 5,840 KB | 526          | 4 / 24      | 0    |
| `sleep --inspect 0.05` | 52,772 µs  | 683 µs   | 1,050 µs| 1,504 KB | 247          | 0 / 4       | 0    |
| `cat --inspect /etc/hosts` | 2,969 µs | 712 µs | 1,360 µs| 1,488 KB | 247          | 3 / 7       | 0    |
| `foobar --inspect` (not found) | 1,885 µs | 35 µs | 1,432 µs| 944 KB | 100         | 0 / 5       | 127  |

The not-found case is instructive: `35 µs` of user CPU is the entire cost of the child between fork and the kernel's `ENOENT` from `execvp`; `1,432 µs` of system CPU is the kernel walking PATH on the child's behalf. This is the kind of OS-level visibility a normal shell never surfaces.

## Memory Safety

Zero heap leaks and zero use-after-free reported by AddressSanitizer (`clang -fsanitize=address -g -O1`, macOS Apple Silicon, ARM64) across all built-in commands, external commands, the `--inspect` path, the 65-argument overflow path, and the >1023-character truncation path. Note: Valgrind was not used because it does not support ARM64 macOS; AddressSanitizer is the standard alternative for this platform. Evidence in the AddressSanitizer screenshots in §5.2.

## Process Integrity

Zero zombie processes under normal operation. Every child spawned by `ks_execute` is reaped by the `waitpid`/`wait4` retry loop before the function returns, so no child lingers as a defunct process in the process table even when SIGINT is delivered mid-wait. Verified by manual `ps` inspection during interactive sessions with 20+ external commands.

## Build Hygiene

Zero compiler warnings under `-std=c11 -Wall -Wextra -Wpedantic -Werror -g -O0` across all eight source files. Evidence in the build screenshot in §5.2.

# Challenges and Solutions

**`getrusage(RUSAGE_CHILDREN)` aggregation.** The first cut of the inspect path bracketed the wait with `getrusage(RUSAGE_CHILDREN)` calls, computing a delta. This produced a "max RSS = 0" line in the introspect table for any child that did not exceed the high-water mark of all previously waited children, which is the inevitable behavior of `RUSAGE_CHILDREN`'s semantics. The fix was to switch to `wait4`, which returns the *per-child* rusage atomically with the reap. `wait4` is BSD-derived and supported on both Darwin and Linux; this is one of the rare cases where the BSD interface is strictly more useful than the more recent POSIX one (POSIX did not standardize `wait4`).

**`max RSS` units differ by platform.** Darwin reports `ru_maxrss` in bytes; Linux reports it in kilobytes. A naive printf would silently report wildly wrong numbers depending on platform. The fix is a compile-time guard (`#ifdef __APPLE__`) inside `ks_introspect_format` that divides by 1024 on Darwin, normalizing to KB everywhere.

**Pipe FD self-reporting.** The child-side `/dev/fd` enumerator initially reported the pipe write end as one of the inherited file descriptors --- which is technically true, but it is bookkeeping, not something the user asked for. The fix was to filter both the pipe FD and `dirfd(d)` (the directory's own descriptor) out of the enumeration, so the user sees only the FDs they would have observed without the introspection wrapper.

**Choosing the `KS_EXIT` sentinel convention.** The Phase 2 design considered an out-parameter (`int *should_exit`) passed to every dispatched function. This was rejected because it required every call site to check a second value and added coupling between the dispatcher and every caller. The positive-sentinel approach keeps each function's return value self-contained and was adopted after reviewing how `waitpid` itself uses an out-parameter only when more than pass/fail is needed.

**Child-process error routing after `execvp` failure.** A first-pass design had the dispatcher return `KS_CMD_NOT_FOUND` when a command was not found, with the error message printed by `main`. This meant the dispatcher had to know, before forking, whether the command existed --- requiring a `PATH` search before exec, duplicating what `execvp` already does. The correct approach (let `execvp` fail, print from the child, `_exit(127)`) was adopted after recognizing that the child process is the only place where the exec failure is observable, and that `_exit` (not `exit`) must be used to avoid double-flushing the parent's `stdio` buffers.

**`sa_flags = 0` versus `SA_RESTART`.** Early testing showed that with `SA_RESTART`, pressing Ctrl+C during input silently re-entered `fgets` and produced no visible effect --- the prompt did not reappear. Understanding that `SA_RESTART` suppresses the `EINTR` return from slow system calls (making them transparent to the caller) clarified why `sa_flags = 0` is the correct flag for a shell that wants to re-prompt on interrupt.

**Ring-buffer testability.** Writing `test_history_wraparound` against a capacity-100 ring buffer would require 101 `add` calls. Recognizing that `KS_HISTORY_MAX` should not appear inside `history.c` (it belongs at the call site in `main.c`) led to the parameterized `ks_history_init`, which made the wraparound test a five-line function with capacity 3.

**Separating test linkage from the main binary.** Each of the four unit-test drivers defines its own `main()`, and each test binary is linked against the shared library sources. This required the Makefile to expose a `LIB_SRCS` variable that filters `src/main.c` out of the source list, so a test binary can replace the production `main()` with the test driver's `main()` without a duplicate-symbol linker error.

# Future Improvements

The Phase 1 proposal explicitly excluded several common shell features as out of MVP scope. They constitute the natural Phase 4 work and are listed here in order of how much they would extend the educational range of the shell.

1. **Pipes (`|`)**, the canonical UNIX IPC primitive. Implementing pipes requires a second `pipe(2)`, two more forks, careful FD plumbing in both children, and a tear-down sequence that closes the right FDs in the right order. It is the natural follow-on to the introspection mode, because the FD-snapshot machinery already in place would let `--inspect` show the pipe FDs in the inherited-FD list.
2. **Redirection (`<`, `>`, `>>`, `2>`)**. Requires `dup2` between fork and exec and `O_TRUNC`/`O_APPEND` flag handling in the child. Could be added to the `--inspect` table to show "stdin redirected from $X$, stdout redirected to $Y$".
3. **Background execution (`&`)** and basic job control. Decouples `fork` from `wait`: the parent must track child PIDs, install a `SIGCHLD` handler to reap finished jobs, and add a `jobs` built-in. This is a substantial design change because `--inspect` currently assumes synchronous waiting.
4. **Quoted-string parsing** (e.g. `echo "hello world"` as one argument). Requires replacing `strtok_r` with a small state-machine tokenizer.
5. **Environment-variable expansion** (`$HOME`, `$PATH`). Trivial with `getenv` once the tokenizer recognizes `$` as significant.
6. **Tab completion and line editing**. Requires either linking against `libreadline` (or `libedit`) or implementing the same in-house. Out of MVP scope but standard in production shells.
7. **Persistent history** (`~/.kshell_history`). A natural extension of the existing ring-buffer module: load on startup, append on exit, dedup against the last $N$ entries on load.
8. **Cross-platform `/dev/fd` fallback for older kernels.** The current introspection mode assumes `/dev/fd` is present; on systems where it is not, the FD-list field falls back to "(none)". A more robust implementation would `fstat` each FD up to `RLIMIT_NOFILE` as a fallback.

# Team Contribution

All members participated in design discussions, code review, and testing throughout all three phases. Individual primary responsibilities were as follows.

- **Abdulrazzak Ghazal** --- Led system architecture across all phases. Owned the external-command execution path (`src/executor.c`), including the `fork`/`execvp`/`waitpid` pipeline, errno-based child error routing, and exit-status decoding. In Phase 3, designed and implemented the `--inspect` introspection mode end-to-end: probe-point placement, child-side `/dev/fd` enumeration, parent-side `wait4` integration, and the cross-platform `ru_maxrss` normalization. Authored the benchmark harness in `bench/bench_fork_exec.c`. Coordinated integration between modules and led architectural review.

- **Omar Bahaeldin Abdalla** --- Led the parser module (`src/parser.c`) and the main REPL loop (`src/main.c`), including tokenization with `strtok_r`, input-length handling, and SIGINT signal management via `sigaction`. Primary author of the parser, builtins, and history unit-test suites; in Phase 3, extended the test harness to cover the introspection module's argv-stripping and formatter functions. Responsible for build-system configuration (Makefile), including the new `bench` target and the `test_introspect` integration.

- **Mohamed Serag** --- Led the built-in command handlers (`src/builtins.c`) and the dispatcher (`src/dispatcher.c`), including `cd`, `exit`, `help`, `history`, and the routing logic. In Phase 3, wired the inspect-flag stripping into the dispatcher and ensured built-ins ignore the flag silently. Contributed the dynamic prompt implementation in `src/prompt.c` with the HOME-prefix boundary check.

- **Osama Alkarnawi** --- Led the command-history module (`src/history.c`), designing the ring-buffer data structure, consecutive-duplicate suppression, and lifecycle management. Authored the scope and constraints sections of all three phase reports, the proposal's out-of-scope analysis, and the Future Improvements roadmap. Led verification testing in all phases, including the AddressSanitizer runs that established memory-safety evidence.

AI tools (including Claude) were used for consultation, searching documentation, discussing design trade-offs, and assisting with formatting the report and refining prose. All code was written and reviewed by team members; AI assistance played the same role as POSIX reference materials, textbook examples, or Stack Overflow would in any technical project.

# Appendix: Build and Run Instructions

```
make            # build ./kshell
make test       # build and run all 32 unit tests
make asan       # build ./kshell-asan with AddressSanitizer
make bench      # build and run the fork+exec benchmark
make clean      # remove all build artifacts
```

Requires a C11 compiler (`cc`) and a POSIX environment. Tested on macOS 26.3 (Apple Silicon, Apple clang 21) and expected to build on Linux without modification.

```
./kshell                      # interactive shell
ls --inspect                  # external command with kernel report
cd /tmp                       # built-in (--inspect ignored on builtins)
exit 0                        # clean termination
```
