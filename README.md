# K-Shell

K-Shell is a lightweight, POSIX-compliant Unix shell written in C11, developed as a course project for ICS 433 (Operating Systems) at KFUPM, Semester 252. It implements a read-eval-print loop with a dynamic prompt showing the current working directory, four built-in commands, external command execution via `fork`/`execvp`/`waitpid`, an in-memory ring-buffer command history with consecutive-duplicate suppression, and SIGINT handling that keeps the shell alive while terminating the foreground child process.

## Build

```
make          # build ./kshell
make test     # build and run all unit tests (parser, builtins, history)
make asan     # build ./kshell-asan with AddressSanitizer (-fsanitize=address)
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
