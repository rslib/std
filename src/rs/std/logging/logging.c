#define RS_STD_LOG_MODULE "logging"

#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/os/env.h>
#include <rs/std/string/string_view.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

// ============================================================================
// Platform-specific file locking
// ============================================================================

#if defined(__unix__) || defined(__APPLE__)
#define RS_LOCK_FILE(f) flockfile(f)
#define RS_UNLOCK_FILE(f) funlockfile(f)
#elif defined(_WIN32)
#define RS_LOCK_FILE(f) _lock_file(f)
#define RS_UNLOCK_FILE(f) _unlock_file(f)
#else
#define RS_LOCK_FILE(f) ((void)0)
#define RS_UNLOCK_FILE(f) ((void)0)
#endif

// ============================================================================
// Thread-local buffer
// ============================================================================

typedef struct {
    char buffer[RS_STD_LOG_THREAD_BUFFER_SIZE]; // Per-thread log buffer
    size_t used;                                // Bytes used in buffer
    const char *module;                         // Current module context
    rs_bool initialized;                        // Has this thread initialized?
} rs_log_thread_buffer_t;

static _Thread_local rs_log_thread_buffer_t rs_log_tls = {0};

// ============================================================================
// Global state
// ============================================================================

typedef struct {
    rs_log_level_t global_level;        // Global minimum log level
    rs_log_output_fn custom_output;     // Custom output callback
    void *custom_userdata;              // User data for callback
    FILE *output_file;                  // Output file (owned or not)
    rs_bool owns_file;                  // True if we opened the file
    rs_bool use_color;                  // ANSI color codes
    rs_bool use_timestamp;              // Timestamp prefix
    rs_bool buffered;                   // Use thread-local buffering
    rs_log_flush_policy_t flush_policy; // When to flush buffers
    uint32_t flush_interval_ms;         // Flush interval (for periodic)
    rs_bool initialized;                // Has init been called?
} rs_log_state_t;

static rs_log_state_t g_log_state = {
    .global_level = RS_STD_LOG_INFO,
    .custom_output = NULL,
    .custom_userdata = NULL,
    .output_file = NULL, // Will default to stderr
    .owns_file = false,
    .use_color = false,
    .use_timestamp = true,
    .buffered = true,
    .flush_policy = RS_STD_LOG_FLUSH_LINE,
    .flush_interval_ms = 0,
    .initialized = false,
};

// ============================================================================
// ANSI color codes
// ============================================================================

static const char *g_log_colors[] = {
    [RS_STD_LOG_TRACE] = "\033[36m",   // Cyan
    [RS_STD_LOG_DEBUG] = "\033[32m",   // Green
    [RS_STD_LOG_INFO] = "\033[37m",    // White
    [RS_STD_LOG_WARN] = "\033[33m",    // Yellow
    [RS_STD_LOG_ERROR] = "\033[31m",   // Red
    [RS_STD_LOG_FATAL] = "\033[1;31m", // Bold Red
    [RS_STD_LOG_OFF] = "",
};

static const char *g_log_reset = "\033[0m";

// ============================================================================
// Level names
// ============================================================================

static const char *g_log_level_names[] = {
    [RS_STD_LOG_TRACE] = "TRACE", [RS_STD_LOG_DEBUG] = "DEBUG", [RS_STD_LOG_INFO] = "INFO ",
    [RS_STD_LOG_WARN] = "WARN ",  [RS_STD_LOG_ERROR] = "ERROR", [RS_STD_LOG_FATAL] = "FATAL",
    [RS_STD_LOG_OFF] = "OFF  ",
};

// ============================================================================
// Helper functions
// ============================================================================

static inline FILE *rs_log_get_file(void)
{
    return g_log_state.output_file ? g_log_state.output_file : stderr;
}

static inline rs_bool rs_log_should_flush(void)
{
    if (!g_log_state.buffered) {
        return true; // Always flush if not buffered
    }

    switch (g_log_state.flush_policy) {
    case RS_STD_LOG_FLUSH_IMMEDIATE:
        return true;
    case RS_STD_LOG_FLUSH_LINE:
        // Check if buffer contains newline
        return rs_log_tls.used > 0 && rs_log_tls.buffer[rs_log_tls.used - 1] == '\n';
    case RS_STD_LOG_FLUSH_NONE:
        return false;
    case RS_STD_LOG_FLUSH_PERIODIC:
        // TODO: implement periodic flushing
        return false;
    default:
        return false;
    }
}

static void rs_log_flush_buffer(void)
{
    if (rs_log_tls.used == 0) {
        return;
    }

    FILE *file = rs_log_get_file();
    RS_LOCK_FILE(file);
    fwrite(rs_log_tls.buffer, 1, rs_log_tls.used, file);
    RS_UNLOCK_FILE(file);

    rs_log_tls.used = 0; // Reset buffer
}

static rs_log_level_t rs_log_parse_level(const char *str)
{
    if (!str)
        return RS_STD_LOG_INFO;

    if (strcmp(str, "TRACE") == 0)
        return RS_STD_LOG_TRACE;
    if (strcmp(str, "DEBUG") == 0)
        return RS_STD_LOG_DEBUG;
    if (strcmp(str, "INFO") == 0)
        return RS_STD_LOG_INFO;
    if (strcmp(str, "WARN") == 0)
        return RS_STD_LOG_WARN;
    if (strcmp(str, "ERROR") == 0)
        return RS_STD_LOG_ERROR;
    if (strcmp(str, "FATAL") == 0)
        return RS_STD_LOG_FATAL;
    if (strcmp(str, "OFF") == 0)
        return RS_STD_LOG_OFF;

    return RS_STD_LOG_INFO; // Default
}

static void rs_log_get_timestamp(char *buf, size_t size)
{
#if defined(__unix__) || defined(__APPLE__)
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    // Get microseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d.%06ld", tm_info->tm_year + 1900, tm_info->tm_mon + 1,
             tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, (long)tv.tv_usec);
#elif defined(_WIN32)
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buf, size, "%04d-%02d-%02d %02d:%02d:%02d.%06d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
             st.wSecond, st.wMilliseconds * 1000);
#else
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S.000000", tm_info);
#endif
}

// ============================================================================
// Thread cleanup (POSIX)
// ============================================================================

#if defined(__unix__) || defined(__APPLE__)
static pthread_key_t rs_log_cleanup_key;
static pthread_once_t rs_log_cleanup_once = PTHREAD_ONCE_INIT;

static void rs_log_thread_cleanup(void *arg)
{
    (void)arg;
    // Flush thread buffer on exit
    rs_log_flush_buffer();
}

static void rs_log_create_cleanup_key(void)
{
    pthread_key_create(&rs_log_cleanup_key, rs_log_thread_cleanup);
}

static void rs_log_register_thread_cleanup(void)
{
    pthread_once(&rs_log_cleanup_once, rs_log_create_cleanup_key);
    pthread_setspecific(rs_log_cleanup_key, &rs_log_tls);
}
#else
static void rs_log_register_thread_cleanup(void)
{
    // No cleanup on non-POSIX platforms
}
#endif

// ============================================================================
// Initialization
// ============================================================================

static void rs_log_init_thread_local(void)
{
    if (!rs_log_tls.initialized) {
        rs_log_tls.used = 0;
        rs_log_tls.module = NULL;
        rs_log_tls.initialized = true;
        rs_log_register_thread_cleanup();
    }
}

RS_STD_API rs_result_t rs_log_init(void)
{
    if (g_log_state.initialized) {
        return RS_OK; // Already initialized
    }

    // Read environment variables
    rs_string_view_t level_str = rs_env_get_view("RS_STD_LOG_LEVEL");
    if (level_str.len > 0) {
        // Convert string_view to null-terminated string
        char level_buf[32];
        size_t copy_len = level_str.len < sizeof(level_buf) - 1 ? level_str.len : sizeof(level_buf) - 1;
        memcpy(level_buf, level_str.data, copy_len);
        level_buf[copy_len] = '\0';
        g_log_state.global_level = rs_log_parse_level(level_buf);
    }

    rs_string_view_t color_str = rs_env_get_view("RS_STD_LOG_COLOR");
    if (color_str.len > 0 && color_str.data[0] == '1') {
        g_log_state.use_color = true;
    }

    rs_string_view_t buffered_str = rs_env_get_view("RS_STD_LOG_BUFFERED");
    if (buffered_str.len > 0 && buffered_str.data[0] == '0') {
        g_log_state.buffered = false;
    }

    rs_string_view_t file_str = rs_env_get_view("RS_STD_LOG_FILE");
    if (file_str.len > 0) {
        // Convert to null-terminated string
        char file_path[1024];
        size_t copy_len = file_str.len < sizeof(file_path) - 1 ? file_str.len : sizeof(file_path) - 1;
        memcpy(file_path, file_str.data, copy_len);
        file_path[copy_len] = '\0';

        rs_log_set_output_path(file_path, true, g_log_state.buffered);
    }

    g_log_state.initialized = true;
    return RS_OK;
}

RS_STD_API void rs_log_shutdown(void)
{
    if (!g_log_state.initialized) {
        return;
    }

    // Flush current thread's buffer
    rs_log_flush_buffer();

    // Close file if we own it
    if (g_log_state.owns_file && g_log_state.output_file) {
        fclose(g_log_state.output_file);
        g_log_state.output_file = NULL;
        g_log_state.owns_file = false;
    }

    g_log_state.initialized = false;
}

// ============================================================================
// Configuration API
// ============================================================================

RS_STD_API rs_result_t rs_log_set_level(rs_log_level_t level)
{
    g_log_state.global_level = level;
    return RS_OK;
}

RS_STD_API rs_log_level_t rs_log_get_level(void)
{
    return g_log_state.global_level;
}

RS_STD_API rs_result_t rs_log_set_output_file(FILE *file, rs_bool buffered)
{
    // Close old file if we own it
    if (g_log_state.owns_file && g_log_state.output_file) {
        fclose(g_log_state.output_file);
    }

    g_log_state.output_file = file;
    g_log_state.owns_file = false;
    g_log_state.buffered = buffered;
    return RS_OK;
}

RS_STD_API rs_result_t rs_log_set_output_path(const char *path, rs_bool append, rs_bool buffered)
{
    if (!path) {
        return rs_log_set_output_file(NULL, buffered);
    }

    FILE *file = fopen(path, append ? "a" : "w");
    if (!file) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to open log file: %s", path);
    }

    // Close old file if we own it
    if (g_log_state.owns_file && g_log_state.output_file) {
        fclose(g_log_state.output_file);
    }

    g_log_state.output_file = file;
    g_log_state.owns_file = true;
    g_log_state.buffered = buffered;
    return RS_OK;
}

RS_STD_API rs_result_t rs_log_set_output_handler(rs_log_output_fn output_fn, void *userdata)
{
    g_log_state.custom_output = output_fn;
    g_log_state.custom_userdata = userdata;
    return RS_OK;
}

RS_STD_API rs_result_t rs_log_set_flush_policy(rs_log_flush_policy_t policy, uint32_t interval_ms)
{
    g_log_state.flush_policy = policy;
    g_log_state.flush_interval_ms = interval_ms;
    return RS_OK;
}

RS_STD_API void rs_log_set_color(rs_bool enable)
{
    g_log_state.use_color = enable;
}

RS_STD_API void rs_log_set_timestamp(rs_bool enable)
{
    g_log_state.use_timestamp = enable;
}

RS_STD_API rs_result_t rs_log_flush(void)
{
    rs_log_flush_buffer();
    return RS_OK;
}

RS_STD_API rs_result_t rs_log_flush_all(void)
{
    // In current implementation, we can only flush current thread
    // TODO: implement global flush tracking
    rs_log_flush_buffer();
    return RS_OK;
}

RS_STD_API void rs_log_set_module(const char *module)
{
    rs_log_init_thread_local();
    rs_log_tls.module = module;
}

RS_STD_API const char *rs_log_get_module(void)
{
    rs_log_init_thread_local();
    return rs_log_tls.module;
}

// ============================================================================
// Logging implementation
// ============================================================================

RS_STD_API void rs_log_write_internal(rs_log_level_t level, const char *module, const char *file, int line,
                                      const char *func, const char *fmt, ...)
{
    // Lazy init
    if (RS_UNLIKELY(!g_log_state.initialized)) {
        rs_log_init();
    }

    rs_log_init_thread_local();

    // Check level filter
    if (level < g_log_state.global_level) {
        return;
    }

    // Format message
    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // Custom output handler takes priority
    if (g_log_state.custom_output) {
        g_log_state.custom_output(level, module, file, line, func, message, g_log_state.custom_userdata);
        return;
    }

    // Build log line
    char log_line[2048];
    size_t offset = 0;

    // Timestamp
    if (g_log_state.use_timestamp) {
        char timestamp[64];
        rs_log_get_timestamp(timestamp, sizeof(timestamp));
        offset += snprintf(log_line + offset, sizeof(log_line) - offset, "[%s] ", timestamp);
    }

    // Color prefix
    if (g_log_state.use_color && level < RS_STD_LOG_OFF) {
        offset += snprintf(log_line + offset, sizeof(log_line) - offset, "%s", g_log_colors[level]);
    }

    // Level
    if (level < RS_STD_LOG_OFF) {
        offset += snprintf(log_line + offset, sizeof(log_line) - offset, "[%s] ", g_log_level_names[level]);
    }

    // Module
    if (module) {
        offset += snprintf(log_line + offset, sizeof(log_line) - offset, "[%s] ", module);
    }

    // Message
    offset += snprintf(log_line + offset, sizeof(log_line) - offset, "%s", message);

    // Location (file:line)
    if (file && line > 0) {
        // Extract just the filename (not full path)
        const char *filename = strrchr(file, '/');
        if (!filename)
            filename = strrchr(file, '\\');
        if (!filename)
            filename = file;
        else
            filename++; // Skip the slash

        offset += snprintf(log_line + offset, sizeof(log_line) - offset, " (%s:%d)", filename, line);
    }

    // Color reset
    if (g_log_state.use_color && level < RS_STD_LOG_OFF) {
        offset += snprintf(log_line + offset, sizeof(log_line) - offset, "%s", g_log_reset);
    }

    // Newline
    offset += snprintf(log_line + offset, sizeof(log_line) - offset, "\n");

    // Write to buffer or directly to file
    if (g_log_state.buffered) {
        // Append to thread-local buffer
        size_t space_left = RS_STD_LOG_THREAD_BUFFER_SIZE - rs_log_tls.used;

        // If message doesn't fit, flush first
        if (offset > space_left) {
            rs_log_flush_buffer();
        }

        // Copy to buffer (truncate if still too large)
        size_t copy_size = offset < RS_STD_LOG_THREAD_BUFFER_SIZE ? offset : RS_STD_LOG_THREAD_BUFFER_SIZE;
        memcpy(rs_log_tls.buffer + rs_log_tls.used, log_line, copy_size);
        rs_log_tls.used += copy_size;

        // Flush if needed
        if (rs_log_should_flush()) {
            rs_log_flush_buffer();
        }
    } else {
        // Direct write with file locking
        FILE *file_out = rs_log_get_file();
        RS_LOCK_FILE(file_out);
        fwrite(log_line, 1, offset, file_out);
        fflush(file_out);
        RS_UNLOCK_FILE(file_out);
    }
}

// ============================================================================
// Trace logging
// ============================================================================

RS_STD_API void rs_log_trace_begin_internal(const char *file, int line, const char *func, const char *fmt, ...)
{
    if (!g_log_state.initialized) {
        rs_log_init();
    }

    if (RS_STD_LOG_TRACE < g_log_state.global_level) {
        return;
    }

    if (fmt) {
        // Format parameters
        char params[512];
        va_list args;
        va_start(args, fmt);
        vsnprintf(params, sizeof(params), fmt, args);
        va_end(args);

        rs_log_write_internal(RS_STD_LOG_TRACE, rs_log_tls.module, file, line, func, "[-->] %s(%s)", func, params);
    } else {
        rs_log_write_internal(RS_STD_LOG_TRACE, rs_log_tls.module, file, line, func, "[-->] %s()", func);
    }
}

RS_STD_API void rs_log_trace_end_internal(const char *file, int line, const char *func)
{
    if (!g_log_state.initialized) {
        rs_log_init();
    }

    if (RS_STD_LOG_TRACE < g_log_state.global_level) {
        return;
    }

    rs_log_write_internal(RS_STD_LOG_TRACE, rs_log_tls.module, file, line, func, "[<--] %s()", func);
}
