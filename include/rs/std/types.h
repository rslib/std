#pragma once

#include <rs/std/macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

RS_EXTERN_C_BEGIN

// ============================================================================
// Integer type aliases (prefixed for safety)
// ============================================================================

// Unsigned integers
typedef uint8_t rs_u8;
typedef uint16_t rs_u16;
typedef uint32_t rs_u32;
typedef uint64_t rs_u64;

// Signed integers
typedef int8_t rs_i8;
typedef int16_t rs_i16;
typedef int32_t rs_i32;
typedef int64_t rs_i64;

// Floating point
typedef float rs_f32;
typedef double rs_f64;

// Character types
typedef char rs_byte;

// Boolean type
typedef bool rs_bool;

// Size types (architecture-dependent)
typedef size_t rs_usize;
typedef ptrdiff_t rs_isize;
typedef intptr_t rs_intptr;
typedef uintptr_t rs_uintptr;

typedef size_t rs_size_t;
typedef ptrdiff_t rs_ssize_t;
typedef uintptr_t rs_addr_t;
typedef intptr_t rs_saddr_t;

// ============================================================================
// Platform-specific types
// ============================================================================

#ifndef _WIN32
#include <sys/types.h>
typedef mode_t rs_file_mode_t;
#else
typedef unsigned int rs_file_mode_t;
#endif

// ============================================================================
// Result type for functions
// ============================================================================

/**
 * Result codes for function returns.
 * All error codes are negative, RS_OK is 0, positive values indicate special status.
 */
typedef enum {
    RS_OK = 0,            // Success
    RS_DONE = 1,          // Operation completed (no more data)
    RS_ERR_NOMEM = -1,    // Out of memory
    RS_ERR_IO = -2,       // I/O error
    RS_ERR_NOTFOUND = -3, // Not found
    RS_ERR_INVALID = -4,  // Invalid argument
    RS_ERR_SYSTEM = -5,   // System error
    RS_ERR_CRYPTO = -6,   // Cryptography error
    RS_ERR_DB = -7,       // Database error
    RS_ERR_OVERFLOW = -8, // Overflow/underflow
    RS_ERR_EOF = -9,      // End of file
    RS_ERR_TIMEOUT = -10  // Timeout
} rs_result_t;

/**
 * Rich error context with location and message.
 * Stored in thread-local storage.
 */
typedef struct {
    rs_result_t code;  // Error code
    char message[256]; // Error message
    const char *file;  // Source file where error occurred
    int line;          // Line number where error occurred
} rs_error_t;

RS_EXTERN_C_END
