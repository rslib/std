#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Length-based string (owned, mutable) with Small String Optimization (SSO).
 *
 * Properties:
 * - Stores length explicitly (no strlen needed)
 * - Can contain null bytes
 * - Always null-terminated for C compatibility
 * - Owns its memory
 * - Uses custom allocator (preserved even for small strings)
 * - Small strings (≤22 bytes on 64-bit) stored inline (no allocation)
 *
 * Memory layout (32 bytes total):
 *   Large: [data:8][len:8][cap:8][allocator:8]
 *   Small: [buf:23][len:1][allocator:8]
 *   The allocator is always at offset 24, accessible in both modes.
 *   The MSB of cap (offset 23) doubles as the large flag, overlapping with small.len.
 *
 * Example:
 *   rs_arena_t *arena = rs_arena_create(1024);
 *   rs_string_t str = rs_string_from_cstr("Hello", .allocator = rs_arena_allocator(arena));
 *   rs_string_push_cstr(&str, " World");
 *   printf("String: " RS_STRING_FMT "\n", RS_STRING_ARG(&str));  // "Hello World"
 *   // No need to free str, arena owns the memory
 */
typedef struct {
    union {
        // Large string: heap-allocated
        // The MSB of cap (at offset 23 on little-endian) is the large flag (1 = large)
        struct {
            char *data;                // offset 0-7: Heap-allocated data
            rs_size_t len;             // offset 8-15: Length
            rs_size_t cap;             // offset 16-23: Capacity (MSB is large flag)
            rs_allocator_t *allocator; // offset 24-31: Allocator
        } large;

        // Small string: inline storage
        // Allocator is at same offset as large.allocator for unified access
        struct {
            char buf[23];              // offset 0-22: Inline buffer (22 chars + null)
            unsigned char len;         // offset 23: Length (MSB=0 means small string)
            rs_allocator_t *allocator; // offset 24-31: Allocator (same position as large)
        } small;
    } u;
} rs_string_t;

// SSO threshold: 22 characters (23-byte buffer minus null terminator)
#define RS_STRING_SSO_CAP 22

// Flag stored in MSB of cap to indicate large string
#define RS_STRING_LARGE_FLAG ((rs_size_t)1 << (sizeof(rs_size_t) * 8 - 1))
#define RS_STRING_CAP_MASK (~RS_STRING_LARGE_FLAG)

// ============================================================================
// Format Specifiers
// ============================================================================

/**
 * Printf format specifier for rs_string_t.
 * Usage: printf("String: " RS_STRING_FMT "\n", RS_STRING_ARG(&str));
 */
#define RS_STRING_FMT "%s"
#define RS_STRING_ARG(str) rs_string_cstr(str)

// ============================================================================
// Creation & Destruction
// ============================================================================

/**
 * Options for string creation.
 */
typedef struct {
    rs_allocator_t *allocator;  // Allocator to use (default: system allocator)
    rs_size_t initial_capacity; // Initial capacity (default: 0 = use SSO)
    rs_u32 reserved;            // Reserved for future use
} rs_string_options_t;

/**
 * Initialize an empty string with options.
 *
 * @param str Pointer to uninitialized string
 * @param opts String options
 */
RS_STD_API void rs_string_init_with_options(rs_string_t *str, rs_string_options_t opts);

/**
 * Initialize an empty string with optional parameters.
 *
 * @param str Pointer to uninitialized string
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @param .initial_capacity (optional): rs_size_t (default: 0 = use SSO)
 *
 * Example:
 *   rs_string_t str = {0};
 *   rs_string_init(&str);  // Uses system allocator
 *   rs_string_init(&str, .allocator = my_allocator);
 *   rs_string_init(&str, .initial_capacity = 100);  // Pre-allocate capacity
 *   // ... use str ...
 *   rs_string_destroy(&str);
 */
#define rs_string_init(str, ...) rs_string_init_with_options(str, (rs_string_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Create empty string with options.
 */
RS_STD_API rs_string_t rs_string_create_with_options(rs_string_options_t opts);

/**
 * Create empty string with optional parameters.
 *
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @param .initial_capacity (optional): rs_size_t (default: 0 = use SSO)
 * @return New empty string
 *
 * Example:
 *   rs_string_t str1 = rs_string_create();  // Uses system allocator
 *   rs_string_t str2 = rs_string_create(.allocator = my_allocator);
 *   rs_string_t str3 = rs_string_create(.initial_capacity = 100);  // Pre-allocate capacity
 */
#define rs_string_create(...) rs_string_create_with_options((rs_string_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Initialize string from C string with options.
 *
 * @param str Pointer to uninitialized string
 * @param cstr C string to copy from
 * @param opts String options
 */
RS_STD_API void rs_string_init_from_cstr_with_options(rs_string_t *str, const char *cstr, rs_string_options_t opts);

/**
 * Initialize string from C string with optional parameters.
 *
 * @param str Pointer to uninitialized string
 * @param cstr C string to copy from
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 *
 * Example:
 *   rs_string_t str = {0};
 *   rs_string_init_from_cstr(&str, "Hello");
 */
#define rs_string_init_from_cstr(str, cstr, ...)                                                                       \
    rs_string_init_from_cstr_with_options(str, cstr, (rs_string_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Create string from C string with options.
 */
RS_STD_API rs_string_t rs_string_from_cstr_with_options(const char *cstr, rs_string_options_t opts);

/**
 * Create string from C string with optional parameters.
 *
 * @param cstr C string to copy from
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @return New string
 *
 * Example:
 *   rs_string_t str1 = rs_string_from_cstr("Hello");  // Uses system allocator
 *   rs_string_t str2 = rs_string_from_cstr("World", .allocator = my_allocator);
 */
#define rs_string_from_cstr(cstr, ...)                                                                                 \
    rs_string_from_cstr_with_options(cstr, (rs_string_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Initialize string from buffer with length and options.
 *
 * @param str Pointer to uninitialized string
 * @param buf Buffer to copy from
 * @param len Length of buffer
 * @param opts String options
 */
RS_STD_API void rs_string_init_from_buf_with_options(rs_string_t *str, const char *buf, rs_size_t len,
                                                     rs_string_options_t opts);

/**
 * Initialize string from buffer with length and optional parameters.
 *
 * @param str Pointer to uninitialized string
 * @param buf Buffer to copy from
 * @param len Length of buffer
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 *
 * Example:
 *   rs_string_t str = {0};
 *   rs_string_init_from_buf(&str, "Hello", 5);
 */
#define rs_string_init_from_buf(str, buf, len, ...)                                                                    \
    rs_string_init_from_buf_with_options(str, buf, len, (rs_string_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Create string from buffer with length and options.
 */
RS_STD_API rs_string_t rs_string_from_buf_with_options(const char *buf, rs_size_t len, rs_string_options_t opts);

/**
 * Create string from buffer with length and optional parameters.
 *
 * @param buf Buffer to copy from
 * @param len Length of buffer
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @return New string
 */
#define rs_string_from_buf(buf, len, ...)                                                                              \
    rs_string_from_buf_with_options(buf, len, (rs_string_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Clone string (deep copy, uses same allocator).
 */
RS_STD_API rs_string_t rs_string_clone(const rs_string_t *str);

/**
 * Destroy string and free memory.
 * Note: If using arena allocator, this is a no-op (arena owns memory).
 */
RS_STD_API void rs_string_destroy(rs_string_t *str);

// ============================================================================
// Properties
// ============================================================================

/**
 * Check if string is using small string optimization.
 * Large strings have the MSB of cap set (RS_STRING_LARGE_FLAG).
 */
static inline rs_bool rs_string_is_small(const rs_string_t *str)
{
    // Large strings have RS_STRING_LARGE_FLAG set in cap
    // For small strings, cap overlaps with small.buf which is typically 0 or low values
    return (str->u.large.cap & RS_STRING_LARGE_FLAG) == 0;
}

/**
 * Get length.
 */
static inline rs_size_t rs_string_len(const rs_string_t *str)
{
    if (rs_string_is_small(str)) {
        return str->u.small.len;
    }
    return str->u.large.len;
}

/**
 * Get capacity.
 */
static inline rs_size_t rs_string_cap(const rs_string_t *str)
{
    if (rs_string_is_small(str)) {
        return RS_STRING_SSO_CAP;
    }
    // Mask off the large flag to get the actual capacity
    return str->u.large.cap & RS_STRING_CAP_MASK;
}

/**
 * Get C string pointer (null-terminated).
 */
static inline const char *rs_string_cstr(const rs_string_t *str)
{
    if (rs_string_is_small(str)) {
        return str->u.small.buf;
    }
    return str->u.large.data ? str->u.large.data : "";
}

/**
 * Get mutable data pointer.
 */
static inline char *rs_string_data_mut(rs_string_t *str)
{
    if (rs_string_is_small(str)) {
        return str->u.small.buf;
    }
    return str->u.large.data;
}

/**
 * Check if empty.
 */
static inline rs_bool rs_string_is_empty(const rs_string_t *str)
{
    return rs_string_len(str) == 0;
}

/**
 * Get allocator from string.
 * The allocator is always stored at offset 24, accessible in both SSO and large modes.
 */
static inline rs_allocator_t *rs_string_get_allocator(const rs_string_t *str)
{
    // Allocator is at the same offset in both small and large layouts
    return str->u.small.allocator;
}

// ============================================================================
// Modification
// ============================================================================

/**
 * Reserve capacity.
 */
RS_STD_API rs_result_t rs_string_reserve(rs_string_t *str, rs_size_t additional);

/**
 * Append C string.
 */
RS_STD_API rs_result_t rs_string_push_cstr(rs_string_t *str, const char *cstr);

/**
 * Append buffer with length.
 */
RS_STD_API rs_result_t rs_string_push_buf(rs_string_t *str, const char *buf, rs_size_t len);

/**
 * Append single character.
 */
RS_STD_API rs_result_t rs_string_push_char(rs_string_t *str, char c);

/**
 * Append another string.
 */
RS_STD_API rs_result_t rs_string_push_string(rs_string_t *str, const rs_string_t *other);

/**
 * Clear string (set length to 0, keep capacity).
 */
RS_STD_API void rs_string_clear(rs_string_t *str);

/**
 * Truncate to length.
 */
RS_STD_API void rs_string_truncate(rs_string_t *str, rs_size_t len);

/**
 * Format string (printf-style).
 * Replaces current content.
 */
RS_STD_API rs_result_t rs_string_format(rs_string_t *str, const char *fmt, ...);

// ============================================================================
// Comparison
// ============================================================================

/**
 * Compare strings (returns <0, 0, >0 like strcmp).
 */
RS_STD_API int rs_string_cmp(const rs_string_t *a, const rs_string_t *b);

/**
 * Check equality.
 */
RS_STD_API rs_bool rs_string_eq(const rs_string_t *a, const rs_string_t *b);

/**
 * Compare with C string.
 */
RS_STD_API int rs_string_cmp_cstr(const rs_string_t *str, const char *cstr);

/**
 * Check equality with C string.
 */
RS_STD_API rs_bool rs_string_eq_cstr(const rs_string_t *str, const char *cstr);

// ============================================================================
// Searching
// ============================================================================

/**
 * Find character (returns index or -1).
 */
RS_STD_API rs_ssize_t rs_string_find_char(const rs_string_t *str, char c);

/**
 * Find substring (returns index or -1).
 */
RS_STD_API rs_ssize_t rs_string_find(const rs_string_t *str, const char *needle);

/**
 * Check if starts with.
 */
RS_STD_API rs_bool rs_string_starts_with(const rs_string_t *str, const char *prefix);

/**
 * Check if ends with.
 */
RS_STD_API rs_bool rs_string_ends_with(const rs_string_t *str, const char *suffix);

/**
 * Check if string contains substring.
 */
static inline rs_bool rs_string_contains(const rs_string_t *str, const char *needle)
{
    return rs_string_find(str, needle) >= 0;
}

// ============================================================================
// String Manipulation
// ============================================================================

/**
 * Trim whitespace from both ends (mutates string).
 */
RS_STD_API void rs_string_trim(rs_string_t *str);

/**
 * Trim whitespace from start (mutates string).
 */
RS_STD_API void rs_string_trim_start(rs_string_t *str);

/**
 * Trim whitespace from end (mutates string).
 */
RS_STD_API void rs_string_trim_end(rs_string_t *str);

/**
 * Convert string to lowercase (mutates string).
 */
RS_STD_API void rs_string_to_lower(rs_string_t *str);

/**
 * Convert string to uppercase (mutates string).
 */
RS_STD_API void rs_string_to_upper(rs_string_t *str);

/**
 * Reverse string (mutates string).
 */
RS_STD_API void rs_string_reverse(rs_string_t *str);

RS_EXTERN_C_END
