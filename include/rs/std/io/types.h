#pragma once

#include <rs/std/internal/api.h>

RS_EXTERN_C_BEGIN

/**
 * Shared I/O types.
 *
 * Common types used across I/O modules.
 */

// ============================================================================
// Seek Mode
// ============================================================================

/**
 * Seek mode for file positioning operations.
 */
typedef enum {
    RS_IO_SEEK_SET = 0, // Absolute offset from beginning
    RS_IO_SEEK_CUR = 1, // Relative to current position
    RS_IO_SEEK_END = 2  // Relative to end of file
} rs_io_seek_mode_t;

// ============================================================================
// File Creation Mode
// ============================================================================

/**
 * File creation mode flags.
 */
typedef enum {
    RS_IO_CREATE_TRUNCATE = 0, // Create new or truncate existing (default)
    RS_IO_CREATE_APPEND = 1,   // Create new or append to existing
    RS_IO_CREATE_EXCL = 2,     // Create new, fail if exists
    RS_IO_CREATE_OPEN = 3      // Open existing or create new (no truncate)
} rs_io_create_mode_t;

RS_EXTERN_C_END
