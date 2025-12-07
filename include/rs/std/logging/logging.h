#pragma once

/**
 * High-performance logging system.
 *
 * Features:
 * - Thread-local buffers (zero contention, lock-free writes)
 * - File locking for safe concurrent writes
 * - Compile-time and runtime log level filtering
 * - Automatic function tracing with entry/exit tracking
 * - Variadic printf-style formatting
 * - Environment variable configuration
 * - Optional colorized output
 * - Optional timestamps
 *
 * Usage:
 *   // In your .c file, define the module name
 *   #define RS_STD_LOG_MODULE "mymodule"
 *   #include <rs/std/logging/logging.h>
 *
 *   void my_function(int x) {
 *       RS_TRACE_SCOPE();  // Auto-trace entry/exit
 *       RS_STD_LOG_DEBUG("Processing value: %d", x);
 *       if (x < 0) {
 *           RS_STD_LOG_ERROR("Invalid value: %d", x);
 *       }
 *   }
 *
 * Environment variables:
 *   RS_STD_LOG_LEVEL=TRACE|DEBUG|INFO|WARN|ERROR|FATAL|OFF
 *   RS_STD_LOG_COLOR=1|0
 *   RS_STD_LOG_FILE=/path/to/file.log
 *   RS_STD_LOG_BUFFERED=1|0
 */

#include <rs/std/internal/api.h>
#include <rs/std/logging/logging_config.h>
#include <rs/std/types.h>
#include <stdbool.h>
#include <stdio.h>

RS_EXTERN_C_BEGIN

// ============================================================================
// Log levels
// ============================================================================

typedef enum {
    RS_STD_LOG_TRACE = 0, // Function entry/exit, detailed execution flow
    RS_STD_LOG_DEBUG = 1, // Detailed diagnostic information
    RS_STD_LOG_INFO = 2,  // General informational messages
    RS_STD_LOG_WARN = 3,  // Warning conditions
    RS_STD_LOG_ERROR = 4, // Error conditions
    RS_STD_LOG_FATAL = 5, // Fatal errors (may terminate)
    RS_STD_LOG_OFF = 6    // Disable all logging
} rs_log_level_t;

// ============================================================================
// Flush policies
// ============================================================================

typedef enum {
    RS_STD_LOG_FLUSH_NONE,      // Manual flush only
    RS_STD_LOG_FLUSH_LINE,      // Flush on newline (default)
    RS_STD_LOG_FLUSH_IMMEDIATE, // Flush every log (slowest, safest)
    RS_STD_LOG_FLUSH_PERIODIC   // Flush every N ms (not yet implemented)
} rs_log_flush_policy_t;

// ============================================================================
// Custom output handler
// ============================================================================

/**
 * Custom log output callback.
 * Called for each log message that passes the level filter.
 *
 * @param level Log level of the message
 * @param module Module name (or NULL if not set)
 * @param file Source file name
 * @param line Source line number
 * @param func Function name
 * @param message Formatted log message
 * @param userdata User-provided context pointer
 */
typedef void (*rs_log_output_fn)(rs_log_level_t level, const char *module, const char *file, int line, const char *func,
                                 const char *message, void *userdata);

// ============================================================================
// Public API
// ============================================================================

/**
 * Initialize the logging system.
 * Reads environment variables and sets up defaults.
 * This is called automatically on first log, but can be called explicitly.
 *
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_log_init(void);

/**
 * Shutdown the logging system.
 * Flushes all pending logs and releases resources.
 */
RS_STD_API void rs_log_shutdown(void);

/**
 * Set the global minimum log level.
 * Messages below this level will be discarded.
 *
 * @param level Minimum log level
 * @return RS_OK on success
 */
RS_STD_API rs_result_t rs_log_set_level(rs_log_level_t level);

/**
 * Get the current global log level.
 *
 * @return Current minimum log level
 */
RS_STD_API rs_log_level_t rs_log_get_level(void);

/**
 * Set output to a FILE pointer.
 * The FILE* is not owned by the logging system - caller must close it.
 *
 * @param file FILE pointer (NULL to use stderr)
 * @param buffered Use thread-local buffering (true) or direct writes (false)
 * @return RS_OK on success
 */
RS_STD_API rs_result_t rs_log_set_output_file(FILE *file, rs_bool buffered);

/**
 * Set output to a file path.
 * The logging system will open and manage the file.
 *
 * @param path File path (NULL to use stderr)
 * @param append Append to existing file (true) or truncate (false)
 * @param buffered Use thread-local buffering
 * @return RS_OK on success, RS_ERR_IO on failure
 */
RS_STD_API rs_result_t rs_log_set_output_path(const char *path, rs_bool append, rs_bool buffered);

/**
 * Set a custom output handler.
 * When set, the default file output is bypassed.
 *
 * @param output_fn Callback function (NULL to restore default)
 * @param userdata User data passed to callback
 * @return RS_OK on success
 */
RS_STD_API rs_result_t rs_log_set_output_handler(rs_log_output_fn output_fn, void *userdata);

/**
 * Set the flush policy.
 *
 * @param policy Flush policy
 * @param interval_ms Interval in milliseconds (for RS_STD_LOG_FLUSH_PERIODIC)
 * @return RS_OK on success
 */
RS_STD_API rs_result_t rs_log_set_flush_policy(rs_log_flush_policy_t policy, uint32_t interval_ms);

/**
 * Enable or disable colored output.
 *
 * @param enable True to enable colors, false to disable
 */
RS_STD_API void rs_log_set_color(rs_bool enable);

/**
 * Enable or disable timestamps in log output.
 *
 * @param enable True to enable timestamps, false to disable
 */
RS_STD_API void rs_log_set_timestamp(rs_bool enable);

/**
 * Flush the current thread's log buffer.
 * This is called automatically based on flush policy.
 *
 * @return RS_OK on success
 */
RS_STD_API rs_result_t rs_log_flush(void);

/**
 * Flush all thread buffers (if possible).
 * Note: Only flushes the calling thread's buffer in current implementation.
 *
 * @return RS_OK on success
 */
RS_STD_API rs_result_t rs_log_flush_all(void);

/**
 * Set the current thread's module name.
 * This is used for filtering and display.
 *
 * @param module Module name (lifetime must exceed usage, typically use string literal)
 */
RS_STD_API void rs_log_set_module(const char *module);

/**
 * Get the current thread's module name.
 *
 * @return Module name or NULL if not set
 */
RS_STD_API const char *rs_log_get_module(void);

// ============================================================================
// Internal API (do not call directly - use macros below)
// ============================================================================

RS_STD_API void rs_log_write_internal(rs_log_level_t level, const char *module, const char *file, int line,
                                      const char *func, const char *fmt, ...);

RS_STD_API void rs_log_trace_begin_internal(const char *file, int line, const char *func, const char *fmt, ...);

RS_STD_API void rs_log_trace_end_internal(const char *file, int line, const char *func);

// ============================================================================
// Logging macros
// ============================================================================

// Helper to get module name (either from RS_STD_LOG_MODULE define or thread-local)
#ifdef RS_STD_LOG_MODULE
#define RS_STD_LOG_MODULE_NAME RS_STD_LOG_MODULE
#else
#define RS_STD_LOG_MODULE_NAME rs_log_get_module()
#endif

// Conditional logging based on compile-time level
#ifdef RS_STD_ENABLE_LOGGING

#if RS_STD_LOG_COMPILE_LEVEL <= RS_STD_LOG_LEVEL_TRACE && defined(RS_STD_ENABLE_TRACE_LOGS)
#define RS_STD_LOG_TRACE(...)                                                                                          \
    rs_log_write_internal(RS_STD_LOG_TRACE, RS_STD_LOG_MODULE_NAME, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define RS_STD_LOG_TRACE(...) ((void)0)
#endif

#if RS_STD_LOG_COMPILE_LEVEL <= RS_STD_LOG_LEVEL_DEBUG
#define RS_STD_LOG_DEBUG(...)                                                                                          \
    rs_log_write_internal(RS_STD_LOG_DEBUG, RS_STD_LOG_MODULE_NAME, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define RS_STD_LOG_DEBUG(...) ((void)0)
#endif

#if RS_STD_LOG_COMPILE_LEVEL <= RS_STD_LOG_LEVEL_INFO
#define RS_STD_LOG_INFO(...)                                                                                           \
    rs_log_write_internal(RS_STD_LOG_INFO, RS_STD_LOG_MODULE_NAME, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define RS_STD_LOG_INFO(...) ((void)0)
#endif

#if RS_STD_LOG_COMPILE_LEVEL <= RS_STD_LOG_LEVEL_WARN
#define RS_STD_LOG_WARN(...)                                                                                           \
    rs_log_write_internal(RS_STD_LOG_WARN, RS_STD_LOG_MODULE_NAME, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define RS_STD_LOG_WARN(...) ((void)0)
#endif

#if RS_STD_LOG_COMPILE_LEVEL <= RS_STD_LOG_LEVEL_ERROR
#define RS_STD_LOG_ERROR(...)                                                                                          \
    rs_log_write_internal(RS_STD_LOG_ERROR, RS_STD_LOG_MODULE_NAME, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define RS_STD_LOG_ERROR(...) ((void)0)
#endif

#if RS_STD_LOG_COMPILE_LEVEL <= RS_STD_LOG_LEVEL_FATAL
#define RS_STD_LOG_FATAL(...)                                                                                          \
    rs_log_write_internal(RS_STD_LOG_FATAL, RS_STD_LOG_MODULE_NAME, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define RS_STD_LOG_FATAL(...) ((void)0)
#endif

#else
// Logging disabled at compile time - all macros are no-ops
#define RS_STD_LOG_TRACE(...) ((void)0)
#define RS_STD_LOG_DEBUG(...) ((void)0)
#define RS_STD_LOG_INFO(...) ((void)0)
#define RS_STD_LOG_WARN(...) ((void)0)
#define RS_STD_LOG_ERROR(...) ((void)0)
#define RS_STD_LOG_FATAL(...) ((void)0)
#endif

// ============================================================================
// Trace helpers (automatic function entry/exit tracking)
// ============================================================================

#if defined(RS_STD_ENABLE_LOGGING) && defined(RS_STD_ENABLE_TRACE_LOGS) &&                                             \
    RS_STD_LOG_COMPILE_LEVEL <= RS_STD_LOG_LEVEL_TRACE

/**
 * Simple trace - just function name.
 * Usage: RS_TRACE_BEGIN();
 */
#define RS_TRACE_BEGIN() rs_log_trace_begin_internal(__FILE__, __LINE__, __func__, NULL)

/**
 * Trace with parameters.
 * Usage: RS_TRACE_BEGIN("x=%d, y=%d", x, y);
 */
#define RS_TRACE_BEGIN_FMT(...) rs_log_trace_begin_internal(__FILE__, __LINE__, __func__, __VA_ARGS__)

/**
 * Trace function exit.
 * Usage: RS_TRACE_END();
 */
#define RS_TRACE_END() rs_log_trace_end_internal(__FILE__, __LINE__, __func__)

/**
 * RAII-style trace scope using cleanup attribute.
 * Automatically logs entry and exit (even on early return).
 *
 * Usage:
 *   void my_function(int x) {
 *       RS_TRACE_SCOPE();  // Logs entry and exit
 *       if (x < 0) return; // Exit is still logged!
 *   }
 */
#if defined(__GNUC__) || defined(__clang__)

typedef struct {
    const char *file;
    int line;
    const char *func;
} rs_trace_scope_t;

static inline void rs_trace_scope_cleanup(rs_trace_scope_t *scope)
{
    rs_log_trace_end_internal(scope->file, scope->line, scope->func);
}

#define RS_TRACE_SCOPE()                                                                                               \
    rs_trace_scope_t __rs_trace_scope                                                                                  \
        __attribute__((cleanup(rs_trace_scope_cleanup))) = {__FILE__, __LINE__, __func__};                             \
    RS_TRACE_BEGIN()

#define RS_TRACE_SCOPE_FMT(...)                                                                                        \
    rs_trace_scope_t __rs_trace_scope                                                                                  \
        __attribute__((cleanup(rs_trace_scope_cleanup))) = {__FILE__, __LINE__, __func__};                             \
    RS_TRACE_BEGIN_FMT(__VA_ARGS__)

#else
// Compiler doesn't support cleanup attribute - fallback to manual tracing
// WARNING: On compilers without cleanup attribute support (MSVC, etc.),
// RS_TRACE_SCOPE only logs entry, NOT exit. Use RS_TRACE_BEGIN/END manually.
#define RS_TRACE_SCOPE()                                                                                               \
    do {                                                                                                               \
        RS_TRACE_BEGIN();                                                                                              \
    } while (0)

#define RS_TRACE_SCOPE_FMT(...)                                                                                        \
    do {                                                                                                               \
        RS_TRACE_BEGIN_FMT(__VA_ARGS__);                                                                               \
    } while (0)
#endif

#else
// Trace logging disabled
#define RS_TRACE_BEGIN() ((void)0)
#define RS_TRACE_BEGIN_FMT(...) ((void)0)
#define RS_TRACE_END() ((void)0)
#define RS_TRACE_SCOPE() ((void)0)
#define RS_TRACE_SCOPE_FMT(...) ((void)0)
#endif

RS_EXTERN_C_END
