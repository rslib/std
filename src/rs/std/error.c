#include <rs/std/error.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// Thread-local storage
// ============================================================================

// Thread-local keyword (C11 standard)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define RS_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
#define RS_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#define RS_THREAD_LOCAL __declspec(thread)
#else
#warning "Thread-local storage not supported, errors will not be thread-safe"
#define RS_THREAD_LOCAL
#endif

// Thread-local error context
static RS_THREAD_LOCAL rs_error_t g_error_ctx = {
    .code = RS_OK,
    .message = "",
    .file = NULL,
    .line = 0,
};

// Flag to track if an error has been set
static RS_THREAD_LOCAL rs_bool g_error_set = false;

// ============================================================================
// Public API
// ============================================================================

const rs_error_t *rs_last_error(void)
{
    if (!g_error_set) {
        return NULL;
    }
    return &g_error_ctx;
}

void rs_clear_error(void)
{
    g_error_set = false;
    g_error_ctx.code = RS_OK;
    g_error_ctx.message[0] = '\0';
    g_error_ctx.file = NULL;
    g_error_ctx.line = 0;
}

const char *rs_error_string(rs_result_t code)
{
    switch (code) {
    case RS_OK:
        return "Success";
    case RS_ERR_NOMEM:
        return "Out of memory";
    case RS_ERR_IO:
        return "I/O error";
    case RS_ERR_NOTFOUND:
        return "Not found";
    case RS_ERR_INVALID:
        return "Invalid argument";
    case RS_ERR_SYSTEM:
        return "System error";
    case RS_ERR_CRYPTO:
        return "Cryptography error";
    case RS_ERR_DB:
        return "Database error";
    case RS_ERR_OVERFLOW:
        return "Overflow/underflow";
    default:
        return "Unknown error";
    }
}

// ============================================================================
// Internal API
// ============================================================================

void rs_set_error_internal(rs_result_t code, const char *file, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    rs_set_error_va(code, file, line, fmt, args);
    va_end(args);
}

void rs_set_error_va(rs_result_t code, const char *file, int line, const char *fmt, va_list args)
{
    g_error_ctx.code = code;
    g_error_ctx.file = file;
    g_error_ctx.line = line;

    // Format message
    if (fmt && fmt[0] != '\0') {
        vsnprintf(g_error_ctx.message, sizeof(g_error_ctx.message), fmt, args);
    } else {
        // No custom message, use default error string
        snprintf(g_error_ctx.message, sizeof(g_error_ctx.message), "%s", rs_error_string(code));
    }

    g_error_set = true;
}
