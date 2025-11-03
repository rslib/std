#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>
#include <rs/std/io/types.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Low-level reading operations.
 *
 * This module provides cross-platform reading primitives for files and file descriptors.
 *
 * Example:
 *   rs_allocator_t *a = rs_allocator_system();
 *   rs_string_t content = rs_string_create(a);
 *
 *   // Read entire file
 *   if (rs_io_read_all(rs_sv_from_cstr("config.txt"), &content) == RS_OK) {
 *       printf("Content: %s\n", rs_string_cstr(&content));
 *   }
 *
 *   // Read at specific offset (pread-style)
 *   char buf[256];
 *   rs_ssize_t n = rs_io_pread(rs_sv_from_cstr("data.bin"), buf, 256, 1024);
 *
 *   rs_string_destroy(&content);
 */

// ============================================================================
// File Handle
// ============================================================================

/**
 * Opaque file handle for reading.
 */
typedef struct rs_io_reader_t rs_io_reader_t;

/**
 * Open file for reading.
 *
 * Returns NULL on failure (check rs_last_error()).
 */
RS_STD_API rs_io_reader_t *rs_io_reader_open(rs_string_view_t path);

/**
 * Close reader handle.
 */
RS_STD_API void rs_io_reader_close(rs_io_reader_t *reader);

// ============================================================================
// Reading Operations
// ============================================================================

/**
 * Read entire file into string.
 *
 * Reads all bytes from file and appends to 'out'.
 * Note: 'out' must be an initialized rs_string_t.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_io_read_all(rs_string_view_t path, rs_string_t *out);

/**
 * Read up to 'count' bytes from reader into buffer.
 *
 * Returns number of bytes read, 0 on EOF, -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_reader_read(rs_io_reader_t *reader, void *buf, rs_size_t count);

/**
 * Read exactly 'count' bytes from reader (or fail).
 *
 * Unlike rs_io_reader_read(), this ensures exactly 'count' bytes are read
 * unless EOF or error occurs.
 *
 * Returns RS_OK on success, RS_ERR_EOF if EOF before reading count bytes,
 * error code on other failures.
 */
RS_STD_API rs_result_t rs_io_reader_read_exact(rs_io_reader_t *reader, void *buf, rs_size_t count);

// ============================================================================
// Positional Reading (pread-style)
// ============================================================================

/**
 * Read from file at specific offset without changing position.
 *
 * Cross-platform abstraction over pread (Unix) and ReadFile with OVERLAPPED (Windows).
 * Does not modify the file position.
 *
 * Returns number of bytes read, 0 on EOF, -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_pread(rs_string_view_t path, void *buf, rs_size_t count, rs_size_t offset);

/**
 * Read from reader at specific offset without changing position.
 *
 * Returns number of bytes read, 0 on EOF, -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_reader_read_at(rs_io_reader_t *reader, void *buf, rs_size_t count, rs_size_t offset);

// ============================================================================
// Seeking
// ============================================================================

/**
 * Seek to position in file.
 *
 * Returns new position from beginning of file, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_reader_seek(rs_io_reader_t *reader, rs_ssize_t offset, rs_io_seek_mode_t mode);

/**
 * Get current position in file.
 *
 * Returns current position, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_reader_tell(rs_io_reader_t *reader);

/**
 * Get size of file.
 *
 * Returns file size in bytes, or -1 on error.
 */
RS_STD_API rs_ssize_t rs_io_reader_size(rs_io_reader_t *reader);

RS_EXTERN_C_END
