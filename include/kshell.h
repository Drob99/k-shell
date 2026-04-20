#ifndef KSHELL_H
#define KSHELL_H

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Status codes
 * -------------------------------------------------------------------------- */
#define KS_OK                0
#define KS_ERR_PARSE        -1
#define KS_ERR_EXEC         -2
#define KS_ERR_BUILTIN      -3
#define KS_ERR_HISTORY      -4
#define KS_ERR_TOO_MANY_ARGS -5
#define KS_CMD_NOT_FOUND    -6
#define KS_ERR_FORK         -7
/* KS_EXIT is a positive sentinel, not an error: signals clean shell termination. */
#define KS_EXIT              1

/* --------------------------------------------------------------------------
 * Limits
 * -------------------------------------------------------------------------- */
#define KS_INPUT_MAX   1024
#define KS_MAX_ARGS      64
#define KS_HISTORY_MAX  100

/* --------------------------------------------------------------------------
 * parser.c
 * -------------------------------------------------------------------------- */

/**
 * @brief Tokenize a shell input line into an argv-style array.
 *
 * Modifies @p line in-place by replacing delimiter characters with NUL bytes.
 * The pointers written into @p argv point into @p line; the caller must not
 * free them individually.
 *
 * @param line  NUL-terminated input string to tokenize (modified in-place).
 * @param argv  Caller-supplied array of at least KS_MAX_ARGS+1 pointers;
 *              on success argv[*argc] is set to NULL.
 * @param argc  Output: number of tokens found.
 * @return KS_OK on success; KS_ERR_PARSE if line, argv, or argc is NULL;
 *         KS_ERR_TOO_MANY_ARGS if the token count exceeds KS_MAX_ARGS.
 * @note Ownership of argv entries belongs to the caller's line buffer.
 */
int ks_parse_line(char *line, char **argv, int *argc);

/* --------------------------------------------------------------------------
 * dispatcher.c
 * -------------------------------------------------------------------------- */

/**
 * @brief Route a parsed command to the appropriate built-in or executor.
 *
 * Checks argv[0] against the known built-in names; if matched, calls the
 * corresponding ks_builtin_* function. Otherwise returns KS_CMD_NOT_FOUND.
 *
 * @param argc  Number of tokens (0 is valid: returns KS_OK immediately).
 * @param argv  NULL-terminated argument vector; argv[0] is the command name.
 * @return KS_OK if the command succeeded or input was empty;
 *         KS_EXIT if the built-in signals clean shell termination;
 *         KS_CMD_NOT_FOUND if argv[0] is not a known command (reserved);
 *         KS_ERR_BUILTIN if a built-in reports a usage or runtime error;
 *         KS_ERR_TOO_MANY_ARGS if argument limits were exceeded;
 *         KS_ERR_FORK if fork() failed when launching an external command.
 * @note Does not free argv; ownership remains with the caller.
 */
int ks_dispatch(int argc, char **argv);

/* --------------------------------------------------------------------------
 * executor.c
 * -------------------------------------------------------------------------- */

/**
 * @brief Execute an external command via fork/execvp/waitpid.
 *
 * @param argv  NULL-terminated argument vector; argv[0] is the program name.
 * @return KS_OK on success or non-zero child exit (child exit status is stored
 *         internally); KS_ERR_FORK if fork() fails; KS_ERR_EXEC if waitpid()
 *         fails. Child error messages are printed from within the child process.
 * @note Non-zero child exit is not treated as a shell error.
 */
int ks_execute(char **argv);

/* --------------------------------------------------------------------------
 * builtins.c
 * -------------------------------------------------------------------------- */

/**
 * @brief Change the shell's working directory.
 *
 * With no arguments changes to HOME. With one argument changes to that path.
 *
 * @param argc  Argument count (1 = cd to HOME, 2 = cd to argv[1]).
 * @param argv  Argument vector; argv[0] is "cd".
 * @return KS_OK on success, KS_ERR_BUILTIN on chdir failure.
 * @note Prints error via perror on failure.
 */
int ks_builtin_cd(int argc, char **argv);

/**
 * @brief Signal the shell to terminate.
 *
 * Stores the optional exit code via the module-static g_exit_code
 * (retrievable with ks_builtin_exit_code()). Does NOT call exit() itself;
 * the caller (main) is responsible for cleanup and the actual exit call.
 *
 * @param argc  1 = exit with code 0; 2 = exit with atoi(argv[1]);
 *              > 2 prints an error to stderr and returns KS_ERR_BUILTIN.
 * @param argv  Argument vector; argv[0] is "exit".
 * @return KS_EXIT on success, KS_ERR_BUILTIN if argc > 2.
 */
int ks_builtin_exit(int argc, char **argv);

/**
 * @brief Return the exit code set by the most recent ks_builtin_exit call.
 *
 * @return Exit code (default 0 if ks_builtin_exit was never called).
 */
int ks_builtin_exit_code(void);

/**
 * @brief Print a summary of available built-in commands to stdout.
 *
 * @param argc  Argument count (ignored).
 * @param argv  Argument vector (ignored beyond argv[0]).
 * @return KS_OK always.
 */
int ks_builtin_help(int argc, char **argv);

/**
 * @brief Print the command history to stdout.
 *
 * Each line is printed as "  N  command" where N is 1-based.
 *
 * @param argc  Argument count (ignored).
 * @param argv  Argument vector (ignored beyond argv[0]).
 * @return KS_OK always.
 */
int ks_builtin_history(int argc, char **argv);

/* --------------------------------------------------------------------------
 * history.c
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialize the in-memory history ring buffer.
 *
 * Must be called exactly once before any other ks_history_* function.
 * Calling a second time without an intervening ks_history_free() is an
 * error and returns KS_ERR_HISTORY.
 *
 * @param capacity  Maximum number of entries to store (must be > 0).
 * @return KS_OK on success; KS_ERR_HISTORY if already initialized,
 *         capacity is 0, or memory allocation fails.
 */
int ks_history_init(size_t capacity);

/**
 * @brief Append a command line to the history buffer.
 *
 * Copies @p line; the caller retains ownership of the original string.
 * When the buffer is full the oldest entry is evicted.
 *
 * @param line  NUL-terminated command string to store.
 * @return KS_OK on success, KS_ERR_HISTORY if line is NULL or empty.
 * @note Allocates memory internally; freed by ks_history_free().
 */
int ks_history_add(const char *line);

/**
 * @brief Retrieve a history entry by index.
 *
 * @param index  0-based index where 0 is the oldest stored entry.
 * @return Pointer to the stored string (valid until next ks_history_add or
 *         ks_history_free call), or NULL if index is out of range.
 * @note Caller must not free or modify the returned pointer.
 */
const char *ks_history_get(int index);

/**
 * @brief Return the number of entries currently in the history buffer.
 *
 * @return Count of stored entries (0 to KS_HISTORY_MAX).
 */
int ks_history_count(void);

/**
 * @brief Release all resources held by the history buffer.
 *
 * After this call ks_history_init() must be called again before using
 * any other ks_history_* function.
 *
 * @note Safe to call even if ks_history_init() was never called.
 */
void ks_history_free(void);

/* --------------------------------------------------------------------------
 * prompt.c
 * -------------------------------------------------------------------------- */

/**
 * @brief Render the shell prompt into @p buf as "kshell:<cwd>$ ".
 *
 * Uses getcwd to obtain the current directory. If the path begins with
 * $HOME (and the next character is '/' or '\0'), that prefix is replaced
 * with '~'. If getenv("HOME") returns NULL the raw cwd is used. If getcwd
 * fails the buffer is set to "kshell:?$ ".
 *
 * @param buf     Destination buffer for the rendered prompt string.
 * @param buflen  Size of @p buf in bytes.
 * @return KS_OK on success, KS_ERR_EXEC if getcwd fails.
 * @note Caller owns @p buf; no allocation is performed.
 */
int ks_render_prompt(char *buf, size_t buflen);

#endif /* KSHELL_H */
