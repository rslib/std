#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/array.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

// ============================================================================
// String Utilities - Operations on rs_string_t with rs_string_view_t
// ============================================================================

/**
 * Replace first occurrence of old with new.
 * Returns RS_OK if replaced, RS_ERR_NOT_FOUND if old not found.
 *
 * @param str String to modify
 * @param old Substring to find
 * @param new_str Replacement substring
 * @return RS_OK on success, RS_ERR_NOT_FOUND if old not found, RS_ERR_NOMEM on allocation failure
 */
RS_STD_API rs_result_t rs_string_replace(rs_string_t *str, rs_string_view_t old, rs_string_view_t new_str);

/**
 * Replace all occurrences of old with new.
 * Returns number of replacements made.
 *
 * @param str String to modify
 * @param old Substring to find
 * @param new_str Replacement substring
 * @return Number of replacements made (0 if none found)
 */
RS_STD_API rs_size_t rs_string_replace_all(rs_string_t *str, rs_string_view_t old, rs_string_view_t new_str);

/**
 * Split string by delimiter into array of string views.
 * Array must be initialized with rs_array_init() with elem_size = sizeof(rs_string_view_t).
 *
 * Note: The returned string views point into the original string's data.
 * They remain valid as long as the original string is not modified or destroyed.
 *
 * @param str String to split
 * @param delim Delimiter to split on
 * @param skip_empty If true, skip empty parts between delimiters
 * @param out_array Output array (must be initialized with elem_size = sizeof(rs_string_view_t))
 * @return RS_OK on success, RS_ERR_INVALID if parameters invalid
 */
RS_STD_API rs_result_t rs_string_split(const rs_string_t *str, rs_string_view_t delim, rs_bool skip_empty,
                                       rs_array_t *out_array);

typedef struct {
    rs_allocator_t *allocator; // Allocator (NULL = system allocator)
    rs_u64 reserved;           // Reserved for future use
} rs_string_join_options_t;

/**
 * Join array of string views with delimiter.
 *
 * @param parts Array of string views to join
 * @param count Number of parts
 * @param delim Delimiter to insert between parts
 * @param options Join options
 * @return Joined string (use rs_string_destroy() to free)
 */
RS_STD_API rs_string_t rs_string_join_with_options(const rs_string_view_t *parts, rs_size_t count,
                                                   rs_string_view_t delim, rs_string_join_options_t options);

/**
 * Join array of string views with delimiter.
 *
 * @param parts Array of string views to join
 * @param count Number of parts
 * @param delim Delimiter to insert between parts
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @return Joined string (use rs_string_destroy() to free)
 */
#define rs_string_join(parts, count, delim, ...)                                                                       \
    rs_string_join_with_options(parts, count, delim, (rs_string_join_options_t){.reserved = 0, ##__VA_ARGS__})

RS_EXTERN_C_END
