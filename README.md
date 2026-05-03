# K-Shell

K-Shell is a lightweight, POSIX-compliant Unix shell written in C11, developed as a course project for ICS 433 (Operating Systems) at KFUPM, Semester 252. It implements a read-eval-print loop with a dynamic prompt showing the current working directory, four built-in commands, external command execution via `fork`/`execvp`/`waitpid`, an in-memory ring-buffer command history with consecutive-duplicate suppression, SIGINT handling that keeps the shell alive while terminating the foreground child process, and an embedded OS-introspection mode (`--inspect`) that turns every external command into a kernel-eye-view report.

## Build

```
make          # build ./kshell
make test     # build and run all 32 unit tests (parser, builtins, history, introspect)
make asan     # build ./kshell-asan with AddressSanitizer (-fsanitize=address)
make bench    # build and run the fork+exec round-trip latency benchmark
make clean    # remove all build artifacts
```

Requires a C11 compiler (`cc`) and a POSIX environment. Tested on macOS (Apple Silicon, clang). No external libraries beyond libc and POSIX are used.

## Usage

```
./kshell
```

The prompt shows the current working directory with `$HOME` abbreviated to `~`:

```
kshell:~/projects/k-shell$
```

Type any command and press Enter. Use `Ctrl+C` to cancel the current input and get a new prompt (the shell does not exit). Use `Ctrl+D` on an empty line to exit cleanly.

## Built-in Commands

| Command | Description |
|---|---|
| `cd [dir]` | Change working directory. With no argument, changes to `$HOME`. |
| `exit [n]` | Exit the shell with optional status code `n` (default 0). |
| `help` | Print a summary of built-in commands. |
| `history` | Display numbered command history (most recent entries last). |

## External Commands

Any executable found on `PATH` is supported. K-Shell forks a child process, calls `execvp`, and waits for it to finish. Examples:

```
kshell:~$ ls -la
kshell:~$ grep -r "main" src/
kshell:~$ /usr/bin/python3 script.py
```

## `--inspect` (OS Introspection Mode)

Add `--inspect` anywhere in the argv of an external command and K-Shell prints a kernel-eye-view report after the child finishes: per-child rusage (user/sys CPU, max RSS, page faults, context switches, FS I/O), wall-clock time, the inherited file descriptors at exec time, and the decoded child exit status. The instrumented path uses `wait4` for per-child rusage and `/dev/fd` enumeration in the child between fork and exec; the default path (no flag) is byte-for-byte unchanged.

```
kshell:~$ ls --inspect
PHASE3_PLAN.md  README.md  include  kshell  Makefile  report  src  tests
  ----- kshell --inspect ------------------------------
   wall-clock          :       7169 us
   user CPU time       :       1896 us
   system CPU time     :       2548 us
   max RSS             :       5840 KB
   minor page faults   :        526
   major page faults   :         19
   vol. ctx switches   :          4
   invol. ctx switches :         24
   FS block reads      :          0
   FS block writes     :          0
   child exit status   :          0
   inherited FDs       : 0 1 2
  -----------------------------------------------------
```

`--inspect` is silently ignored on built-in commands (they don't fork).

## Not Supported

The following features are explicitly out of scope for this project:

- Piping (`|`)
- Backgrounding (`&`)
- Redirection (`<`, `>`, `>>`)
- Quoted-string parsing (e.g. `echo "hello world"` passes two tokens)
- Escape characters
- Environment-variable expansion (e.g. `$PATH`)

## Limitations

- Maximum input line length: 1023 characters (lines longer than this are discarded with a warning).
- Maximum arguments per command: 64.
- History capacity: 100 entries (oldest entries are evicted when full).

## Team

- Abdulrazzak Ghazal
- Omar Bahaeldin Abdalla
- Mohamed Serag
- Osama Alkarnawi

## Course

ICS 433 — Operating Systems, KFUPM, Semester 252
