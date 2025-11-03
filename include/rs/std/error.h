#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/types.h>
#include <stdarg.h>

RS_EXTERN_C_BEGIN

/**
 * Thread-local error context for rs_std.
 *
 * This module provides rich error reporting with context (file, line, message).
 * Error state is stored in thread-local storage, similar to errno but better.
 *
 * Usage:
 *   // Simple error checking
 *   if (rs_array_push(&arr, &val) != RS_OK) {
 *       return RS_ERR_NOMEM;
 *   }
 *
 *   // Detailed error reporting
 *   if (rs_string_format(&str, "%d", x) != RS_OK) {
 *       const rs_error_t *err = rs_last_error();
 *       fprintf(stderr, "Error: %s at %s:%d\n",
 *               err->message, err->file, err->line);
 *   }
 */

// ============================================================================
// Public API
// ============================================================================

/**
 * Get the last error that occurred in the current thread.
 * Returns NULL if no error has occurred (last operation was RS_OK).
 *
 * The returned pointer is valid until the next error occurs or
 * rs_clear_error() is called in this thread.
 */
RS_STD_API const rs_error_t *rs_last_error(void);

/**
 * Clear the error state for the current thread.
 * After calling this, rs_last_error() will return NULL.
 */
RS_STD_API void rs_clear_error(void);

/**
 * Get error message for a given error code.
 * Returns a static string describing the error.
 */
RS_STD_API const char *rs_error_string(rs_result_t code);

// ============================================================================
// Internal API (used by library implementation)
// ============================================================================

/**
 * Set error context (internal use only).
 * Use the RS_ERROR macro instead.
 */
RS_STD_API void rs_set_error_internal(rs_result_t code, const char *file, int line, const char *fmt, ...);

/**
 * Set error context with va_list (internal use only).
 */
RS_STD_API void rs_set_error_va(rs_result_t code, const char *file, int line, const char *fmt, va_list args);

// ============================================================================
// Convenience macros
// ============================================================================

/**
 * Set error with formatted message.
 * Usage: RS_ERROR(RS_ERR_NOMEM, "Failed to allocate %zu bytes", size);
 */
#define RS_ERROR(code, ...) rs_set_error_internal((code), __FILE__, __LINE__, __VA_ARGS__)

/**
 * Set error and return the error code.
 * Usage: return RS_ERROR_RET(RS_ERR_INVALID, "Index %zu out of bounds", idx);
 */
#define RS_ERROR_RET(code, ...) (RS_ERROR((code), __VA_ARGS__), (code))

/**
 * Check condition, set error and return if false.
 * Usage: RS_CHECK(ptr != NULL, RS_ERR_NOMEM, "Allocation failed");
 */
#define RS_CHECK(cond, code, ...)                                                                                      \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            RS_ERROR((code), __VA_ARGS__);                                                                             \
            return (code);                                                                                             \
        }                                                                                                              \
    } while (0)

/**
 * Propagate error from function call.
 * Usage: RS_TRY(rs_array_reserve(&arr, 10));
 */
#define RS_TRY(expr)                                                                                                   \
    do {                                                                                                               \
        rs_result_t _rs_result = (expr);                                                                               \
        if (_rs_result != RS_OK) {                                                                                     \
            return _rs_result;                                                                                         \
        }                                                                                                              \
    } while (0)

RS_EXTERN_C_END
