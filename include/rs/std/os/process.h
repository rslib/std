#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

/**
 * @file process.h
 * @brief Cross-platform process execution and management
 *
 * Provides both high-level command execution and low-level process control.
 */

RS_EXTERN_C_BEGIN

// =============================================================================
// High-Level API - Simple Command Execution
// =============================================================================

/**
 * @brief Check if a command exists in PATH
 *
 * Uses platform-specific method (command -v on Unix, where on Windows)
 *
 * @param cmd Command name to check (as string view)
 * @return 1 if exists, 0 if not, -1 on error
 */
RS_STD_API int rs_process_command_exists(rs_string_view_t cmd);

/**
 * @brief Execute a shell command (no output capture)
 *
 * Output goes to stdout/stderr as normal.
 *
 * @param cmd Command to execute (as string view)
 * @return Exit code of command, or -1 on error
 */
RS_STD_API int rs_process_exec(rs_string_view_t cmd);

/**
 * @brief Execute command and capture stdout
 *
 * The output string must be initialized before calling this function.
 * The captured output will be appended to the string.
 *
 * @param cmd Command to execute (as string view)
 * @param output Initialized output string (output will be appended)
 * @param exit_code Pointer to receive exit code (optional, can be NULL)
 * @return RS_OK on success, RS_ERR_* on error
 */
RS_STD_API rs_result_t rs_process_exec_capture(rs_string_view_t cmd, rs_string_t *output, int *exit_code);

/**
 * @brief Execute command and capture both stdout and stderr separately
 *
 * Both output strings must be initialized before calling this function.
 * The captured output will be appended to the strings.
 *
 * @param cmd Command to execute (as string view)
 * @param stdout_out Initialized stdout string (output will be appended)
 * @param stderr_out Initialized stderr string (output will be appended)
 * @param exit_code Pointer to receive exit code (optional, can be NULL)
 * @return RS_OK on success, RS_ERR_* on error
 */
RS_STD_API rs_result_t rs_process_exec_capture_split(rs_string_view_t cmd, rs_string_t *stdout_out,
                                                     rs_string_t *stderr_out, int *exit_code);

// =============================================================================
// Low-Level API - Process Management
// =============================================================================

/**
 * @brief Process handle (opaque type)
 */
typedef struct rs_process_t rs_process_t;

/**
 * @brief Process spawn options
 */
typedef struct {
    rs_string_view_t working_dir; /**< Working directory (empty = inherit) */
    rs_allocator_t *allocator;    /**< Allocator for process resources */
    rs_bool capture_stdout;       /**< Capture stdout? */
    rs_bool capture_stderr;       /**< Capture stderr? */
    rs_bool redirect_stdin;       /**< Allow writing to stdin? */
    rs_u32 timeout_ms;            /**< Timeout in milliseconds (0 = no timeout) */
} rs_process_opts_t;

/**
 * @brief Create default process options
 */
RS_STD_API rs_process_opts_t rs_process_opts_default(void);

/**
 * @brief Spawn a new process
 *
 * @param argv Command and arguments (array of string views, last element should be empty)
 * @param argc Number of arguments in argv
 * @param opts Spawn options (NULL = defaults)
 * @return Process handle, or NULL on error
 */
RS_STD_API rs_process_t *rs_process_spawn(const rs_string_view_t *argv, rs_size_t argc, const rs_process_opts_t *opts);

/**
 * @brief Wait for process to complete
 *
 * @param proc Process handle
 * @param exit_code Pointer to receive exit code (optional, can be NULL)
 * @param timeout_ms Timeout in milliseconds (0 = wait indefinitely)
 * @return RS_OK on success, RS_ERR_TIMEOUT on timeout, RS_ERR_* on error
 */
RS_STD_API rs_result_t rs_process_wait(rs_process_t *proc, int *exit_code, uint32_t timeout_ms);

/**
 * @brief Check if process is still running
 *
 * @param proc Process handle
 * @return 1 if running, 0 if exited, -1 on error
 */
RS_STD_API int rs_process_is_running(rs_process_t *proc);

/**
 * @brief Read from process stdout
 *
 * Only works if capture_stdout was enabled in spawn options.
 *
 * @param proc Process handle
 * @param buffer Buffer to read into
 * @param size Buffer size
 * @return Number of bytes read, 0 on EOF, -1 on error
 */
RS_STD_API rs_ssize_t rs_process_read_stdout(rs_process_t *proc, void *buffer, rs_size_t size);

/**
 * @brief Read from process stderr
 *
 * Only works if capture_stderr was enabled in spawn options.
 *
 * @param proc Process handle
 * @param buffer Buffer to read into
 * @param size Buffer size
 * @return Number of bytes read, 0 on EOF, -1 on error
 */
RS_STD_API rs_ssize_t rs_process_read_stderr(rs_process_t *proc, void *buffer, rs_size_t size);

/**
 * @brief Write to process stdin
 *
 * Only works if redirect_stdin was enabled in spawn options.
 *
 * @param proc Process handle
 * @param buffer Data to write
 * @param size Number of bytes to write
 * @return Number of bytes written, -1 on error
 */
RS_STD_API rs_ssize_t rs_process_write(rs_process_t *proc, const void *buffer, rs_size_t size);

/**
 * @brief Close process stdin (signal EOF to child)
 *
 * @param proc Process handle
 * @return RS_OK on success, RS_ERR_* on error
 */
RS_STD_API rs_result_t rs_process_close_stdin(rs_process_t *proc);

/**
 * @brief Terminate process gracefully
 *
 * Sends SIGTERM on Unix, TerminateProcess on Windows.
 *
 * @param proc Process handle
 * @return RS_OK on success, RS_ERR_* on error
 */
RS_STD_API rs_result_t rs_process_terminate(rs_process_t *proc);

/**
 * @brief Kill process forcefully
 *
 * Sends SIGKILL on Unix, TerminateProcess with force on Windows.
 *
 * @param proc Process handle
 * @return RS_OK on success, RS_ERR_* on error
 */
RS_STD_API rs_result_t rs_process_kill(rs_process_t *proc);

/**
 * @brief Get process ID
 *
 * @param proc Process handle
 * @return Process ID, or -1 on error
 */
RS_STD_API int rs_process_get_pid(rs_process_t *proc);

/**
 * @brief Close process handle and free resources
 *
 * If process is still running, this will wait for it to complete.
 * Use rs_process_terminate() or rs_process_kill() first if you want to stop it.
 *
 * @param proc Process handle
 */
RS_STD_API void rs_process_close(rs_process_t *proc);

// =============================================================================
// Convenience Macros for C String Literals
// =============================================================================

/**
 * Execute command from C string literal
 * Usage: rs_process_exec_cstr("ls -la")
 */
#define rs_process_exec_cstr(cmd_literal) rs_process_exec(rs_string_view_from_cstr(cmd_literal))

/**
 * Check command exists from C string literal
 * Usage: rs_process_command_exists_cstr("git")
 */
#define rs_process_command_exists_cstr(cmd_literal) rs_process_command_exists(rs_string_view_from_cstr(cmd_literal))

RS_EXTERN_C_END
