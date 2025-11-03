#pragma once

#include <rs/std/containers/array.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN
/**
 * High-level file operations.
 *
 * This module provides convenient file operations built on top of the I/O primitives.
 * Unlike io/read.h and io/write.h which provide low-level operations, this module
 * focuses on common file manipulation tasks.
 *
 * Example:
 *   // Read entire file
 *   rs_string_t content = rs_string_create(allocator);
 *   rs_file_read(rs_sv_from_cstr("config.txt"), &content);
 *
 *   // Write file
 *   rs_file_write(rs_sv_from_cstr("output.txt"), rs_sv_from_cstr("Hello!"));
 *
 *   // Check if file exists
 *   if (rs_file_exists(rs_sv_from_cstr("data.bin"))) {
 *       // ...
 *   }
 */

// ============================================================================
// File Metadata
// ============================================================================

/**
 * Check if file exists.
 *
 * Returns true if the path exists and is a regular file.
 */
RS_STD_API rs_bool rs_file_exists(rs_string_view_t path);

/**
 * Check if path is a regular file.
 *
 * Returns true if the path exists and is a regular file (not a directory, symlink, etc.).
 */
RS_STD_API rs_bool rs_file_is_file(rs_string_view_t path);

/**
 * Get file size in bytes.
 *
 * Returns file size, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_file_size(rs_string_view_t path);

// ============================================================================
// Reading
// ============================================================================

/**
 * Read entire file into string.
 *
 * Convenience wrapper around rs_io_read_all().
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_file_read(rs_string_view_t path, rs_string_t *out);

/**
 * Read file as lines.
 *
 * Reads entire file and splits into lines (without line endings).
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_file_read_lines(rs_string_view_t path, rs_array_t *out);

// ============================================================================
// Writing
// ============================================================================

/**
 * Write string to file (create or truncate).
 *
 * Convenience wrapper around rs_io_write_str().
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_file_write(rs_string_view_t path, rs_string_view_t content);

/**
 * Append string to file.
 *
 * Convenience wrapper around rs_io_append_str().
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_file_append(rs_string_view_t path, rs_string_view_t content);

// ============================================================================
// File Operations
// ============================================================================

/**
 * Copy file from src to dst.
 *
 * If dst exists, it will be overwritten.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_file_copy(rs_string_view_t src, rs_string_view_t dst);

/**
 * Move/rename file from src to dst.
 *
 * If dst exists, it will be overwritten.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_file_move(rs_string_view_t src, rs_string_view_t dst);

/**
 * Delete file.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_file_remove(rs_string_view_t path);

RS_EXTERN_C_END
