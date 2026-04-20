# K-Shell

K-Shell is a POSIX-compliant Unix shell written in C11, developed as a course project for ICS 433 (Operating Systems) at KFUPM, Semester 252. It implements a read-eval-print loop with a dynamic prompt showing the current working directory, built-in commands (`cd`, `exit`, `help`, `history`), external command execution via `fork`/`execvp`/`waitpid`, in-memory command history, and SIGINT handling that keeps the shell alive while interrupting foreground commands.

## Build

```
make        # builds ./kshell
make test   # builds and runs the test suite
make asan   # builds ./kshell-asan with AddressSanitizer
make clean  # removes all build artifacts
```

## Run

```
./kshell
```

Type `help` to list available built-ins, `history` to see past commands, and `exit` to quit.
