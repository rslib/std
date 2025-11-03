#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/array.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Environment variable utilities.
 *
 * This module provides cross-platform environment variable operations.
 * Functions use output parameters for rs_string_t and accept rs_string_view_t
 * as input (works with both rs_string_t and C strings via rs_sv_from_cstr).
 *
 * Example:
 *   rs_allocator_t *a = rs_allocator_system();
 *
 *   // Get environment variable
 *   rs_string_t path = rs_string_create(a);
 *   if (rs_env_get(&path, "HOME") == RS_OK) {
 *       printf("HOME: %s\n", rs_string_cstr(&path));
 *   }
 *
 *   // Set environment variable
 *   rs_env_set("MY_VAR", "some_value");
 *
 *   // Expand variables in string
 *   rs_string_t expanded = rs_string_create(a);
 *   rs_env_expand(&expanded, rs_sv_from_cstr("$HOME/.config"));
 *
 *   // Cleanup
 *   rs_string_destroy(&path);
 *   rs_string_destroy(&expanded);
 */

// ============================================================================
// Basic Operations
// ============================================================================

/**
 * Get environment variable value.
 *
 * Copies the value of the environment variable 'name' into 'out'.
 * If the variable doesn't exist, 'out' is cleared and RS_ERR_NOTFOUND is returned.
 *
 * Note: 'out' must be an initialized rs_string_t (e.g., from rs_string_create()).
 * Returns RS_OK on success, RS_ERR_NOTFOUND if variable doesn't exist.
 */
RS_STD_API rs_result_t rs_env_get(rs_string_t *out, const char *name);

/**
 * Check if environment variable exists.
 *
 * Returns 1 if the environment variable 'name' is set, 0 otherwise.
 */
RS_STD_API int rs_env_exists(const char *name);

/**
 * Set environment variable.
 *
 * Sets the environment variable 'name' to 'value'.
 * If the variable already exists, it is overwritten.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_env_set(const char *name, const char *value);

/**
 * Unset/remove environment variable.
 *
 * Removes the environment variable 'name' from the environment.
 * If the variable doesn't exist, this is a no-op and returns RS_OK.
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_env_unset(const char *name);

/**
 * Expand environment variables in string.
 *
 * Expands environment variables in 'input' and writes the result to 'out'.
 * Supports both $VAR and ${VAR} syntax.
 *
 * Examples:
 *   "$HOME/.config" -> "/home/user/.config"
 *   "${USER}_backup" -> "john_backup"
 *   "Path: $PATH" -> "Path: /usr/bin:/usr/local/bin"
 *
 * If a variable is not found, it is replaced with an empty string.
 *
 * Note: 'out' must be an initialized rs_string_t (e.g., from rs_string_create()).
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_env_expand(rs_string_t *out, rs_string_view_t input);

// ============================================================================
// Advanced Operations
// ============================================================================

/**
 * Get environment variable as string_view (zero-copy).
 *
 * Returns a string_view pointing directly to the environment variable value.
 * This is faster than rs_env_get() as it doesn't allocate or copy.
 *
 * WARNING: The returned view is only valid until:
 * - The environment variable is modified (setenv/putenv)
 * - The program terminates
 * - On some platforms, until the next getenv() call
 *
 * Use rs_env_get() if you need a stable copy.
 *
 * Returns a string_view to the value, or empty view if not found.
 */
RS_STD_API rs_string_view_t rs_env_get_view(const char *name);

/**
 * Callback function type for environment variable iteration.
 *
 * Called for each environment variable during rs_env_foreach().
 *
 * Parameters:
 *   name - Environment variable name
 *   value - Environment variable value
 *   userdata - User-provided context pointer
 */
typedef void (*rs_env_foreach_fn)(const char *name, const char *value, void *userdata);

/**
 * Iterate over all environment variables.
 *
 * Calls 'callback' for each environment variable in the process environment.
 * The order of iteration is unspecified.
 *
 * Example:
 *   void print_env(const char *name, const char *value, void *userdata) {
 *       printf("%s=%s\n", name, value);
 *   }
 *   rs_env_foreach(print_env, NULL);
 *
 * Note: The callback should not modify the environment (setenv/unsetenv)
 * during iteration, as this may cause undefined behavior.
 */
RS_STD_API void rs_env_foreach(rs_env_foreach_fn callback, void *userdata);

/**
 * Environment variable name-value pair.
 */
typedef struct {
    rs_string_t name;
    rs_string_t value;
} rs_env_pair_t;

/**
 * Get all environment variables as an array of name-value pairs.
 *
 * Populates 'out_pairs' with all environment variables in the process.
 * Each pair contains allocated strings for both name and value.
 *
 * Note: 'out_pairs' must be an initialized rs_array_t with element_size = sizeof(rs_env_pair_t).
 * The caller is responsible for destroying the strings in each pair and the array itself.
 *
 * Example:
 *   rs_allocator_t *a = rs_allocator_system();
 *   rs_array_t pairs = rs_array_create(a, sizeof(rs_env_pair_t));
 *
 *   if (rs_env_get_all(&pairs, a) == RS_OK) {
 *       for (rs_size_t i = 0; i < rs_array_len(&pairs); i++) {
 *           rs_env_pair_t *pair = rs_array_at(&pairs, i);
 *           printf("%s=%s\n", rs_string_cstr(&pair->name), rs_string_cstr(&pair->value));
 *           rs_string_destroy(&pair->name);
 *           rs_string_destroy(&pair->value);
 *       }
 *   }
 *   rs_array_destroy(&pairs);
 *
 * Returns RS_OK on success, error code on failure.
 */
RS_STD_API rs_result_t rs_env_get_all(rs_array_t *out_pairs, rs_allocator_t *allocator);

RS_EXTERN_C_END
