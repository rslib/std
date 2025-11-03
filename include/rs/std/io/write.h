#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/io/types.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Low-level writing operations.
 *
 * This module provides cross-platform writing primitives for files and file descriptors.
 *
 * Example:
 *   // Write entire buffer to file
 *   const char *data = "Hello, world!";
 *   rs_io_write_all(rs_sv_from_cstr("output.txt"), data, strlen(data));
 *
 *   // Append to file
 *   rs_io_append_str(rs_sv_from_cstr("log.txt"), rs_sv_from_cstr("New entry\n"));
 *
 *   // Write at specific offset (pwrite-style)
 *   rs_io_pwrite(rs_sv_from_cstr("data.bin"), "PATCH", 5, 1024);
 */

// ============================================================================
// File Handle
// ============================================================================

/**
 * Opaque file handle for writing.
 */
typedef struct rs_io_writer_t rs_io_writer_t;

/**
 * Open file for writing.
 *
 * mode: File permissions (Unix) - use 0644 for regular files
 * create_mode: How to handle existing files
 *
 * Returns NULL on failure (check rs_last_error()).
 */
RS_STD_API rs_io_writer_t *rs_io_writer_open(rs_string_view_t path, rs_file_mode_t mode,
                                             rs_io_create_mode_t create_mode);

/**
 * Close writer handle.
 */
RS_STD_API void rs_io_writer_close(rs_io_writer_t *writer);

// ============================================================================
// Writing Operations
// ============================================================================

/**
 * Write entire buffer to file (create or truncate).
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_io_write_all(rs_string_view_t path, const void *buf, rs_size_t count);

/**
 * Append buffer to file (create if doesn't exist).
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_io_append_all(rs_string_view_t path, const void *buf, rs_size_t count);

/**
 * Write string_view to file (convenience wrapper).
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_io_write_str(rs_string_view_t path, rs_string_view_t content);

/**
 * Append string_view to file (convenience wrapper).
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_io_append_str(rs_string_view_t path, rs_string_view_t content);

/**
 * Write up to 'count' bytes from buffer to writer.
 *
 * Returns number of bytes written, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_writer_write(rs_io_writer_t *writer, const void *buf, rs_size_t count);

/**
 * Write exactly 'count' bytes from buffer (or fail).
 *
 * Unlike rs_io_writer_write(), this ensures exactly 'count' bytes are written.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_io_writer_write_exact(rs_io_writer_t *writer, const void *buf, rs_size_t count);

/**
 * Flush writer buffers to disk.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_io_writer_flush(rs_io_writer_t *writer);

// ============================================================================
// Positional Writing (pwrite-style)
// ============================================================================

/**
 * Write to file at specific offset without changing position.
 *
 * Cross-platform abstraction over pwrite (Unix) and WriteFile with OVERLAPPED (Windows).
 * Does not modify the file position.
 *
 * Returns number of bytes written, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_pwrite(rs_string_view_t path, const void *buf, rs_size_t count, rs_size_t offset);

/**
 * Write to writer at specific offset without changing position.
 *
 * Returns number of bytes written, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_writer_write_at(rs_io_writer_t *writer, const void *buf, rs_size_t count, rs_size_t offset);

// ============================================================================
// Seeking
// ============================================================================

/**
 * Seek to position in file.
 *
 * Returns new position from beginning of file, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_writer_seek(rs_io_writer_t *writer, rs_ssize_t offset, rs_io_seek_mode_t mode);

/**
 * Get current position in file.
 *
 * Returns current position, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_writer_tell(rs_io_writer_t *writer);

RS_EXTERN_C_END
