#pragma once

#include <rs/std/containers/array.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN
/**
 * Directory operations.
 *
 * This module provides directory manipulation and traversal operations.
 *
 * Example:
 *   // Create directory
 *   rs_dir_create(rs_sv_from_cstr("mydir"), 0755);
 *
 *   // List directory contents
 *   rs_array_t entries = rs_array_create(sizeof(rs_string_t), allocator);
 *   rs_dir_read(rs_sv_from_cstr("mydir"), &entries);
 *
 *   // Remove directory
 *   rs_dir_remove(rs_sv_from_cstr("mydir"));
 */

// ============================================================================
// Directory Metadata
// ============================================================================

/**
 * Check if directory exists.
 *
 * Returns true if the path exists and is a directory.
 */
RS_STD_API rs_bool rs_dir_exists(rs_string_view_t path);

/**
 * Check if path is a directory.
 *
 * Returns true if the path exists and is a directory.
 */
RS_STD_API rs_bool rs_dir_is_dir(rs_string_view_t path);

// ============================================================================
// Directory Operations
// ============================================================================

/**
 * Create directory.
 *
 * Creates a single directory. Parent directories must already exist.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_dir_create(rs_string_view_t path, rs_file_mode_t mode);

/**
 * Create directory and all parent directories.
 *
 * Like mkdir -p, creates all necessary parent directories.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_dir_create_all(rs_string_view_t path, rs_file_mode_t mode);

/**
 * Remove empty directory.
 *
 * Removes a directory. The directory must be empty.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_dir_remove(rs_string_view_t path);

/**
 * Remove directory and all contents recursively.
 *
 * Like rm -rf, removes directory and all files/subdirectories.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_dir_remove_all(rs_string_view_t path);

// ============================================================================
// Directory Reading
// ============================================================================

/**
 * Read directory entries.
 *
 * Reads all entries in a directory (excluding "." and "..").
 * The output array should be initialized with sizeof(rs_string_t).
 * Each entry is a string containing just the filename (not full path).
 * Returns RS_OK on success, error code on failure.
 *
 * Example:
 *   rs_array_t entries = rs_array_create(sizeof(rs_string_t), allocator);
 *   rs_dir_read(rs_sv_from_cstr("/tmp"), &entries);
 *   for (size_t i = 0; i < rs_array_len(&entries); i++) {
 *       rs_string_t *entry = (rs_string_t*)rs_array_get(&entries, i);
 *       printf("%s\n", rs_string_cstr(entry));
 *       rs_string_destroy(entry);
 *   }
 *   rs_array_destroy(&entries);
 */
RS_STD_API rs_result_t rs_dir_read(rs_string_view_t path, rs_array_t *out);

// ============================================================================
// Current Working Directory
// ============================================================================

/**
 * Get current working directory.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_dir_get_current(rs_string_t *out);

/**
 * Set current working directory.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_dir_set_current(rs_string_view_t path);

RS_EXTERN_C_END
