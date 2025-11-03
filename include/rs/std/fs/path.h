#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Path manipulation utilities.
 *
 * This module provides cross-platform path manipulation functions.
 * Functions use output parameters for rs_string_t and accept rs_string_view_t
 * as input (works with both rs_string_t and C strings via rs_sv_from_cstr).
 *
 * Example:
 *   rs_allocator_t *a = rs_allocator_system();
 *
 *   // Create string once with allocator
 *   rs_string_t path = rs_string_create(a);
 *
 *   rs_path_get_home(&path);
 *   rs_path_append(&path, rs_sv_from_cstr(".config"));
 *   rs_path_append(&path, rs_sv_from_cstr("nvim"));
 *
 *   // Expand another path
 *   rs_string_t dotfiles = rs_string_create(a);
 *   rs_path_expand(&dotfiles, rs_sv_from_cstr("~/dotfiles"));
 *
 *   // Cleanup
 *   rs_string_destroy(&path);
 *   rs_string_destroy(&dotfiles);
 */

// ============================================================================
// Path Creators (write to existing string)
// ============================================================================

/**
 * Get home directory path.
 *
 * Note: 'out' must be an initialized rs_string_t (e.g., from rs_string_create()).
 * Returns RS_OK on success, error code on failure (check rs_last_error()).
 */
RS_STD_API rs_result_t rs_path_get_home(rs_string_t *out);

/**
 * Get temporary directory path.
 *
 * Returns the system's temporary directory:
 * - Unix/Linux: $TMPDIR, /tmp, or /var/tmp
 * - macOS: $TMPDIR or /tmp
 * - Windows: %TEMP%, %TMP%, or GetTempPath
 *
 * Note: 'out' must be an initialized rs_string_t (e.g., from rs_string_create()).
 * Returns RS_OK on success, error code on failure (check rs_last_error()).
 */
RS_STD_API rs_result_t rs_path_get_temp(rs_string_t *out);

/**
 * Expand path with ~ and environment variables.
 *
 * Expansions:
 * - "~" or "~/" -> user's home directory
 * - "~user" -> specified user's home directory
 * - "$VAR" or "${VAR}" -> environment variable
 *
 * Examples:
 *   "~/config" -> "/home/user/config"
 *   "$HOME/.vimrc" -> "/home/user/.vimrc"
 *   "~root/test" -> "/root/test"
 *
 * Note: 'out' must be an initialized rs_string_t (e.g., from rs_string_create()).
 * Returns RS_OK on success, error code on failure (check rs_last_error()).
 */
RS_STD_API rs_result_t rs_path_expand(rs_string_t *out, rs_string_view_t path);

/**
 * Get directory name from path (like dirname).
 *
 * Examples:
 *   "/foo/bar/baz" -> "/foo/bar"
 *   "/foo" -> "/"
 *   "foo/bar" -> "foo"
 *   "foo" -> "."
 *
 * Note: 'out' must be an initialized rs_string_t (e.g., from rs_string_create()).
 * Returns RS_OK on success, error code on failure (check rs_last_error()).
 */
RS_STD_API rs_result_t rs_path_dirname(rs_string_t *out, rs_string_view_t path);

/**
 * Get base name from path (like basename).
 *
 * Examples:
 *   "/foo/bar/baz" -> "baz"
 *   "/foo/" -> "foo"
 *   "foo" -> "foo"
 *   "/" -> "/"
 *
 * Note: 'out' must be an initialized rs_string_t (e.g., from rs_string_create()).
 * Returns RS_OK on success, error code on failure (check rs_last_error()).
 */
RS_STD_API rs_result_t rs_path_basename(rs_string_t *out, rs_string_view_t path);

// ============================================================================
// Path Modifiers (work on existing string, no allocator needed)
// ============================================================================

/**
 * Append path component to existing path.
 *
 * Handles:
 * - Adds separator if needed
 * - Skips leading separators in component
 * - If component is absolute, replaces entire path
 *
 * Examples:
 *   path="/foo", component="bar" -> "/foo/bar"
 *   path="/foo/", component="/bar" -> "/foo/bar"
 *   path="/foo", component="/bar" -> "/bar" (absolute wins)
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_path_append(rs_string_t *path, rs_string_view_t component);

/**
 * Normalize path in-place (resolve . and .., remove duplicate separators).
 *
 * Examples:
 *   "/foo//bar" -> "/foo/bar"
 *   "/foo/./bar" -> "/foo/bar"
 *   "/foo/bar/../baz" -> "/foo/baz"
 *   "foo/../bar" -> "bar"
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_path_normalize(rs_string_t *path);

// ============================================================================
// Path Queries
// ============================================================================

/**
 * Check if path is absolute.
 *
 * Unix: starts with '/'
 * Windows: starts with drive letter (C:) or UNC path (\\)
 *
 * Returns 1 if absolute, 0 if relative.
 */
RS_STD_API int rs_path_is_absolute(rs_string_view_t path);

/**
 * Get file extension from path.
 *
 * Examples:
 *   "foo.txt" -> ".txt"
 *   "foo.tar.gz" -> ".gz"
 *   "foo" -> ""
 *   ".vimrc" -> ""
 *
 * Returns a string_view pointing to the extension (including the dot).
 * Returns empty string_view if no extension.
 */
RS_STD_API rs_string_view_t rs_path_extension(rs_string_view_t path);

/**
 * Check if path has extension.
 *
 * Example:
 *   rs_path_has_extension(rs_sv_from_cstr("foo.txt"), ".txt") -> 1
 *   rs_path_has_extension(rs_sv_from_cstr("foo.txt"), ".c") -> 0
 *
 * Returns 1 if path ends with extension, 0 otherwise.
 */
RS_STD_API int rs_path_has_extension(rs_string_view_t path, const char *ext);

/**
 * Check if path exists on filesystem.
 *
 * Returns 1 if path exists (file or directory), 0 if not, -1 on error.
 */
RS_STD_API int rs_path_exists(rs_string_view_t path);

/**
 * Check if path is a regular file.
 *
 * Returns 1 if path exists and is a regular file, 0 otherwise.
 */
RS_STD_API int rs_path_is_file(rs_string_view_t path);

/**
 * Check if path is a directory.
 *
 * Returns 1 if path exists and is a directory, 0 otherwise.
 */
RS_STD_API int rs_path_is_dir(rs_string_view_t path);

/**
 * Check if path is a symbolic link.
 *
 * Returns 1 if path exists and is a symlink, 0 otherwise.
 */
RS_STD_API int rs_path_is_symlink(rs_string_view_t path);

/**
 * Get relative path from 'from' to 'to'.
 *
 * Examples:
 *   from="/foo/bar", to="/foo/baz" -> "../baz"
 *   from="/foo/bar", to="/foo/bar/baz" -> "baz"
 *   from="/foo", to="/bar" -> "../bar"
 *
 * Note: 'out' must be an initialized rs_string_t.
 * Both paths must be absolute or both must be relative.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_path_relative(rs_string_t *out, rs_string_view_t from, rs_string_view_t to);

/**
 * Get proximate path (shortest of relative or absolute).
 *
 * Returns the shorter of:
 * - The relative path from 'from' to 'to'
 * - The absolute path 'to'
 *
 * Examples:
 *   from="/foo/bar", to="/foo/baz" -> "../baz"
 *   from="/foo", to="/very/long/path" -> "/very/long/path" (absolute is shorter)
 *
 * Note: 'out' must be an initialized rs_string_t.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_path_proximate(rs_string_t *out, rs_string_view_t from, rs_string_view_t to);

// ============================================================================
// Filesystem Operations
// ============================================================================

/**
 * Remove file or empty directory.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_path_remove(rs_string_view_t path);

/**
 * Create symbolic link.
 *
 * Creates a symlink at 'link_path' pointing to 'target'.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_path_symlink(rs_string_view_t target, rs_string_view_t link_path);

/**
 * Read symbolic link target.
 *
 * Reads the target of a symbolic link.
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_path_read_symlink(rs_string_view_t link_path, rs_string_t *out);

// ============================================================================
// Platform-specific
// ============================================================================

/**
 * Get path separator for current platform.
 * Returns '/' on Unix, '\\' on Windows.
 */
RS_STD_API char rs_path_separator(void);

/**
 * Get path list separator for current platform.
 * Returns ':' on Unix (for PATH), ';' on Windows.
 */
RS_STD_API char rs_path_list_separator(void);

RS_EXTERN_C_END
