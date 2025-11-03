#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * String view (non-owning, immutable).
 *
 * Properties:
 * - Does not own memory
 * - Immutable (read-only)
 * - Can point to substring
 * - Lightweight (just pointer + length)
 * - May not be null-terminated!
 *
 * Use cases:
 * - Function parameters (avoid copies)
 * - Substrings without allocation
 * - Parsing without copying
 *
 * Example:
 *   const char *text = "Hello, World!";
 *   rs_string_view_t sv = rs_sv_from_cstr(text);
 *   rs_string_view_t hello = rs_sv_slice_to(sv, 5);  // "Hello"
 *   printf("View: " RS_SV_FMT "\n", RS_SV_ARG(hello));  // "Hello"
 */
typedef struct {
    const char *data;
    rs_size_t len;
} rs_string_view_t;

// ============================================================================
// Format Specifiers
// ============================================================================

/**
 * Printf format specifier for rs_string_view_t.
 * Uses %.*s to handle non-null-terminated strings.
 *
 * Usage: printf("View: " RS_SV_FMT "\n", RS_SV_ARG(sv));
 */
#define RS_SV_FMT "%.*s"
#define RS_SV_ARG(sv) ((int)(sv).len), ((sv).data ? (sv).data : "")

// ============================================================================
// Creation
// ============================================================================

/**
 * Create view from C string.
 */
RS_STD_API rs_string_view_t rs_sv_from_cstr(const char *cstr);

/**
 * Create view from buffer with length.
 */
RS_STD_API rs_string_view_t rs_sv_from_buf(const char *buf, rs_size_t len);

/**
 * Create view from rs_string.
 */
RS_STD_API rs_string_view_t rs_sv_from_string(rs_string_t str);

/**
 * Empty view.
 */
static inline rs_string_view_t rs_sv_empty(void)
{
    rs_string_view_t sv = {NULL, 0};
    return sv;
}

// ============================================================================
// Properties
// ============================================================================

/**
 * Get data pointer.
 */
static inline const char *rs_sv_data(rs_string_view_t sv)
{
    return sv.data;
}

/**
 * Get length.
 */
static inline rs_size_t rs_sv_len(rs_string_view_t sv)
{
    return sv.len;
}

/**
 * Check if empty.
 */
static inline rs_bool rs_sv_is_empty(rs_string_view_t sv)
{
    return sv.len == 0;
}

/**
 * Get character at index (no bounds checking).
 */
static inline char rs_sv_at(rs_string_view_t sv, rs_size_t index)
{
    return sv.data[index];
}

// ============================================================================
// Slicing
// ============================================================================

/**
 * Slice view [start..end).
 * If end > len, clamps to len.
 */
RS_STD_API rs_string_view_t rs_sv_slice(rs_string_view_t sv, rs_size_t start, rs_size_t end);

/**
 * Slice from start to end.
 */
RS_STD_API rs_string_view_t rs_sv_slice_from(rs_string_view_t sv, rs_size_t start);

/**
 * Slice from start to end.
 */
RS_STD_API rs_string_view_t rs_sv_slice_to(rs_string_view_t sv, rs_size_t end);

/**
 * Remove prefix if present.
 */
RS_STD_API rs_string_view_t rs_sv_trim_prefix(rs_string_view_t sv, rs_string_view_t prefix);

/**
 * Remove prefix (C string convenience).
 */
#define rs_sv_trim_prefix_cstr(sv, prefix) rs_sv_trim_prefix(sv, rs_sv_from_cstr(prefix))

/**
 * Remove suffix if present.
 */
RS_STD_API rs_string_view_t rs_sv_trim_suffix(rs_string_view_t sv, rs_string_view_t suffix);

/**
 * Remove suffix (C string convenience).
 */
#define rs_sv_trim_suffix_cstr(sv, suffix) rs_sv_trim_suffix(sv, rs_sv_from_cstr(suffix))

/**
 * Trim whitespace from both ends.
 */
RS_STD_API rs_string_view_t rs_sv_trim(rs_string_view_t sv);

/**
 * Trim whitespace from left.
 */
RS_STD_API rs_string_view_t rs_sv_trim_left(rs_string_view_t sv);

/**
 * Trim whitespace from right.
 */
RS_STD_API rs_string_view_t rs_sv_trim_right(rs_string_view_t sv);

// ============================================================================
// Comparison
// ============================================================================

/**
 * Compare views (returns <0, 0, >0 like memcmp).
 */
RS_STD_API int rs_sv_cmp(rs_string_view_t a, rs_string_view_t b);

/**
 * Check equality.
 */
RS_STD_API rs_bool rs_sv_eq(rs_string_view_t a, rs_string_view_t b);

/**
 * Compare with C string (convenience).
 */
#define rs_sv_cmp_cstr(sv, cstr) rs_sv_cmp(sv, rs_sv_from_cstr(cstr))

/**
 * Check equality with C string (convenience).
 */
#define rs_sv_eq_cstr(sv, cstr) rs_sv_eq(sv, rs_sv_from_cstr(cstr))

// ============================================================================
// Searching
// ============================================================================

/**
 * Find character (returns index or -1).
 */
RS_STD_API rs_ssize_t rs_sv_find_char(rs_string_view_t sv, char c);

/**
 * Find substring (returns index or -1).
 */
RS_STD_API rs_ssize_t rs_sv_find(rs_string_view_t sv, rs_string_view_t needle);

/**
 * Find substring (C string convenience).
 */
#define rs_sv_find_cstr(sv, needle) rs_sv_find(sv, rs_sv_from_cstr(needle))

/**
 * Check if starts with.
 */
RS_STD_API rs_bool rs_sv_starts_with(rs_string_view_t sv, rs_string_view_t prefix);

/**
 * Check if starts with (C string convenience).
 */
#define rs_sv_starts_with_cstr(sv, prefix) rs_sv_starts_with(sv, rs_sv_from_cstr(prefix))

/**
 * Check if ends with.
 */
RS_STD_API rs_bool rs_sv_ends_with(rs_string_view_t sv, rs_string_view_t suffix);

/**
 * Check if ends with (C string convenience).
 */
#define rs_sv_ends_with_cstr(sv, suffix) rs_sv_ends_with(sv, rs_sv_from_cstr(suffix))

/**
 * Check if string view contains substring.
 */
static inline rs_bool rs_sv_contains(rs_string_view_t sv, rs_string_view_t needle)
{
    return rs_sv_find(sv, needle) >= 0;
}

/**
 * Check if string view contains substring (C string convenience).
 */
#define rs_sv_contains_cstr(sv, needle) rs_sv_contains(sv, rs_sv_from_cstr(needle))

// ============================================================================
// Conversion
// ============================================================================

/**
 * Options for string_view conversion.
 */
typedef struct {
    rs_allocator_t *allocator; // Allocator to use (default: system allocator)
    rs_u32 reserved;           // Reserved for future use
} rs_sv_options_t;

/**
 * Convert to rs_string (allocates) with options.
 */
RS_STD_API rs_string_t rs_sv_to_string_with_options(rs_string_view_t sv, rs_sv_options_t opts);

/**
 * Convert to rs_string (allocates) with optional parameters.
 *
 * @param sv String view to convert
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @return New string
 *
 * Example:
 *   rs_string_t str1 = rs_sv_to_string(sv);  // Uses system allocator
 *   rs_string_t str2 = rs_sv_to_string(sv, .allocator = my_allocator);
 */
#define rs_sv_to_string(sv, ...) rs_sv_to_string_with_options(sv, (rs_sv_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Convert to C string with allocator (allocates, null-terminates) with options.
 * Caller must free using the same allocator.
 */
RS_STD_API char *rs_sv_to_cstr_with_options(rs_string_view_t sv, rs_sv_options_t opts);

/**
 * Convert to C string (allocates, null-terminates) with optional parameters.
 *
 * @param sv String view to convert
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @return Null-terminated C string (caller must free)
 *
 * Example:
 *   char *str1 = rs_sv_to_cstr(sv);  // Uses system allocator
 *   char *str2 = rs_sv_to_cstr(sv, .allocator = my_allocator);
 */
#define rs_sv_to_cstr(sv, ...) rs_sv_to_cstr_with_options(sv, (rs_sv_options_t){.reserved = 0, ##__VA_ARGS__})

RS_EXTERN_C_END
