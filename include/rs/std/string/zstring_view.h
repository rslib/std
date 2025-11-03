#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Zero-terminated string view (non-owning, immutable, guaranteed null-terminated).
 *
 * Properties:
 * - Does not own memory
 * - Immutable (read-only)
 * - ALWAYS null-terminated (safe for C APIs)
 * - Lightweight (pointer + length)
 * - Cannot be a substring (would break null-termination)
 *
 * Differences from rs_string_view_t:
 * - rs_string_view_t: May not be null-terminated, can be substring
 * - rs_zstring_view_t: Always null-terminated, safe for C API use
 *
 * Use cases:
 * - Passing to C APIs that require const char*
 * - Function parameters when null-termination is required
 * - String literals
 *
 * Example:
 *   rs_zstring_view_t path = rs_zsv_from_cstr("/tmp/file.txt");
 *   FILE *f = fopen(rs_zsv_cstr(path), "r");  // Safe! Guaranteed null-terminated
 */
typedef struct {
    const char *data; // Pointer to null-terminated string
    rs_size_t len;    // Length (cached, excludes null terminator)
} rs_zstring_view_t;

// ============================================================================
// Format Specifiers
// ============================================================================

/**
 * Printf format specifier for rs_zstring_view_t.
 * Can use either %s (since it's null-terminated) or %.*s for consistency.
 *
 * Usage: printf("Path: " RS_ZSV_FMT "\n", RS_ZSV_ARG(zsv));
 */
#define RS_ZSV_FMT "%s"
#define RS_ZSV_ARG(zsv) ((zsv).data ? (zsv).data : "")

// Alternative: for consistency with string_view
#define RS_ZSV_FMT_LEN "%.*s"
#define RS_ZSV_ARG_LEN(zsv) ((int)(zsv).len), ((zsv).data ? (zsv).data : "")

// ============================================================================
// Creation
// ============================================================================

/**
 * Create zstring_view from C string (null-terminated).
 *
 * @param cstr Null-terminated C string
 * @return Zero-terminated string view
 */
RS_STD_API rs_zstring_view_t rs_zsv_from_cstr(const char *cstr);

/**
 * Create zstring_view from rs_string_t.
 * rs_string_t is always null-terminated, so this is safe.
 *
 * @param str Pointer to rs_string_t
 * @return Zero-terminated string view
 */
RS_STD_API rs_zstring_view_t rs_zsv_from_string(const rs_string_t *str);

/**
 * Empty zstring_view.
 * Points to static empty string "".
 */
RS_STD_API rs_zstring_view_t rs_zsv_empty(void);

// ============================================================================
// Conversion
// ============================================================================

/**
 * Convert to rs_string_view_t (zero-cost).
 * Safe because zstring_view is always null-terminated.
 *
 * @param zsv Zero-terminated string view
 * @return String view pointing to same data
 */
static inline rs_string_view_t rs_zsv_to_sv(rs_zstring_view_t zsv)
{
    return (rs_string_view_t){zsv.data, zsv.len};
}

/**
 * Get C string pointer (zero-cost).
 * Safe because zstring_view is always null-terminated.
 *
 * @param zsv Zero-terminated string view
 * @return Pointer to null-terminated C string
 */
static inline const char *rs_zsv_cstr(rs_zstring_view_t zsv)
{
    return zsv.data ? zsv.data : "";
}

// ============================================================================
// Properties
// ============================================================================

/**
 * Get data pointer.
 */
static inline const char *rs_zsv_data(rs_zstring_view_t zsv)
{
    return zsv.data;
}

/**
 * Get length (excludes null terminator).
 */
static inline rs_size_t rs_zsv_len(rs_zstring_view_t zsv)
{
    return zsv.len;
}

/**
 * Check if empty.
 */
static inline rs_bool rs_zsv_is_empty(rs_zstring_view_t zsv)
{
    return zsv.len == 0;
}

/**
 * Get character at index (no bounds checking).
 */
static inline char rs_zsv_at(rs_zstring_view_t zsv, rs_size_t index)
{
    return zsv.data[index];
}

// ============================================================================
// Comparison (delegates to string_view)
// ============================================================================

/**
 * Compare zstring_views (returns <0, 0, >0 like memcmp).
 */
static inline int rs_zsv_cmp(rs_zstring_view_t a, rs_zstring_view_t b)
{
    return rs_sv_cmp(rs_zsv_to_sv(a), rs_zsv_to_sv(b));
}

/**
 * Check equality.
 */
static inline rs_bool rs_zsv_eq(rs_zstring_view_t a, rs_zstring_view_t b)
{
    return rs_sv_eq(rs_zsv_to_sv(a), rs_zsv_to_sv(b));
}

/**
 * Compare with C string (convenience).
 */
#define rs_zsv_cmp_cstr(zsv, cstr) rs_zsv_cmp(zsv, rs_zsv_from_cstr(cstr))

/**
 * Check equality with C string (convenience).
 */
#define rs_zsv_eq_cstr(zsv, cstr) rs_zsv_eq(zsv, rs_zsv_from_cstr(cstr))

// ============================================================================
// Searching (delegates to string_view)
// ============================================================================

/**
 * Find character (returns index or -1).
 */
static inline rs_ssize_t rs_zsv_find_char(rs_zstring_view_t zsv, char c)
{
    return rs_sv_find_char(rs_zsv_to_sv(zsv), c);
}

/**
 * Check if starts with.
 */
static inline rs_bool rs_zsv_starts_with(rs_zstring_view_t zsv, rs_string_view_t prefix)
{
    return rs_sv_starts_with(rs_zsv_to_sv(zsv), prefix);
}

/**
 * Check if starts with (C string convenience).
 */
#define rs_zsv_starts_with_cstr(zsv, prefix) rs_zsv_starts_with(zsv, rs_sv_from_cstr(prefix))

/**
 * Check if ends with.
 */
static inline rs_bool rs_zsv_ends_with(rs_zstring_view_t zsv, rs_string_view_t suffix)
{
    return rs_sv_ends_with(rs_zsv_to_sv(zsv), suffix);
}

/**
 * Check if ends with (C string convenience).
 */
#define rs_zsv_ends_with_cstr(zsv, suffix) rs_zsv_ends_with(zsv, rs_sv_from_cstr(suffix))

/**
 * Check if contains substring.
 */
static inline rs_bool rs_zsv_contains(rs_zstring_view_t zsv, rs_string_view_t needle)
{
    return rs_sv_contains(rs_zsv_to_sv(zsv), needle);
}

/**
 * Check if contains substring (C string convenience).
 */
#define rs_zsv_contains_cstr(zsv, needle) rs_zsv_contains(zsv, rs_sv_from_cstr(needle))

// ============================================================================
// Note on Slicing
// ============================================================================

/*
 * IMPORTANT: Slicing is NOT provided for rs_zstring_view_t!
 *
 * Slicing would break the null-termination guarantee.
 * If you need to slice, convert to rs_string_view_t first:
 *
 *   rs_zstring_view_t zstr = rs_zsv_from_cstr("Hello, World!");
 *   rs_string_view_t sv = rs_zsv_to_sv(zstr);
 *   rs_string_view_t hello = rs_sv_slice_to(sv, 5);  // "Hello" (not null-terminated)
 *
 * If you need a null-terminated slice, allocate a new string:
 *   rs_string_t hello_str = rs_sv_to_string(hello);
 *   rs_zstring_view_t hello_zstr = rs_zsv_from_string(&hello_str);
 */

RS_EXTERN_C_END
