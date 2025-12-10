#define RS_STD_LOG_MODULE "string"

#include <ctype.h>
#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// Internal helpers
// ============================================================================

// Mark string as large (set RS_STRING_LARGE_FLAG in cap)
// Note: Cannot use RS_STRING_ARG here because the flag hasn't been set yet,
// so rs_string_cstr would incorrectly treat this as a small string.
static inline void mark_as_large(rs_string_t *str)
{
    RS_TRACE_BEGIN_FMT("str=%p (marking as large)", (void *)str);
    str->u.large.cap |= RS_STRING_LARGE_FLAG;
    RS_TRACE_END();
}

// Set length for small string
static inline void set_small_len(rs_string_t *str, rs_size_t len)
{
    RS_TRACE_BEGIN_FMT("str=\"" RS_STRING_FMT "\", len=%zu", RS_STRING_ARG(str), len);
    str->u.small.len = (unsigned char)len; // MSB = 0 for small
    RS_TRACE_END();
}

// Set length for large string
static inline void set_large_len(rs_string_t *str, rs_size_t len)
{
    RS_TRACE_BEGIN_FMT("str=\"" RS_STRING_FMT "\", len=%zu", RS_STRING_ARG(str), len);
    str->u.large.len = len;
    RS_TRACE_END();
}

// ============================================================================
// Creation & Destruction
// ============================================================================

void rs_string_init_with_options(rs_string_t *str, rs_string_options_t opts)
{
    RS_TRACE_BEGIN_FMT("str=%p, allocator=%p, initial_capacity=%zu", (void *)str, (void *)opts.allocator,
                       opts.initial_capacity);

    rs_allocator_t *allocator = opts.allocator;
    if (!allocator) {
        allocator = rs_allocator_system();
    }

    memset(str, 0, sizeof(rs_string_t));

    // Use SSO if capacity fits
    if (opts.initial_capacity <= RS_STRING_SSO_CAP) {
        set_small_len(str, 0);
        str->u.small.buf[0] = '\0';
        str->allocator = allocator;
    } else {
        // Allocate on heap
        str->u.large.data = (char *)rs_alloc(allocator, opts.initial_capacity + 1);
        if (!str->u.large.data) {
            // Fall back to SSO on allocation failure
            set_small_len(str, 0);
            str->u.small.buf[0] = '\0';
            str->allocator = allocator;
            RS_TRACE_END();
            return;
        }
        str->u.large.data[0] = '\0';
        str->u.large.len = 0;
        str->u.large.cap = opts.initial_capacity;
        str->allocator = allocator;
        mark_as_large(str);
    }

    RS_TRACE_END();
}

rs_string_t rs_string_create_with_options(rs_string_options_t opts)
{
    RS_TRACE_BEGIN_FMT("allocator=%p, initial_capacity=%zu", (void *)opts.allocator, opts.initial_capacity);
    rs_string_t str;
    rs_string_init_with_options(&str, opts);
    RS_TRACE_END();
    return str;
}

void rs_string_init_from_cstr_with_options(rs_string_t *str, const char *cstr, rs_string_options_t opts)
{
    RS_TRACE_BEGIN_FMT("str=%p, cstr=%s, allocator=%p", (void *)str, cstr, (void *)opts.allocator);
    if (!cstr) {
        rs_string_init_with_options(str, opts);
        RS_TRACE_END();
        return;
    }
    rs_string_init_from_buf_with_options(str, cstr, strlen(cstr), opts);
    RS_TRACE_END();
}

rs_string_t rs_string_from_cstr_with_options(const char *cstr, rs_string_options_t opts)
{
    RS_TRACE_BEGIN_FMT("cstr=%s, allocator=%p", cstr, (void *)opts.allocator);
    rs_string_t str;
    rs_string_init_from_cstr_with_options(&str, cstr, opts);
    RS_TRACE_END();
    return str;
}

void rs_string_init_from_buf_with_options(rs_string_t *str, const char *buf, rs_size_t len, rs_string_options_t opts)
{
    RS_TRACE_BEGIN_FMT("str=%p, buf=%p, len=%zu, allocator=%p", (void *)str, (void *)buf, len, (void *)opts.allocator);
    rs_allocator_t *allocator = opts.allocator;
    if (!allocator) {
        allocator = rs_allocator_system();
    }

    memset(str, 0, sizeof(rs_string_t));

    // Use SSO if fits
    if (len <= RS_STRING_SSO_CAP) {
        set_small_len(str, len);
        if (buf && len > 0) {
            memcpy(str->u.small.buf, buf, len);
        }
        str->u.small.buf[len] = '\0';
        str->allocator = allocator;
    } else {
        // Allocate on heap
        str->u.large.data = (char *)rs_alloc(allocator, len + 1);
        if (!str->u.large.data) {
            rs_string_init_with_options(str, opts);
            RS_TRACE_END();
            return;
        }
        if (buf && len > 0) {
            memcpy(str->u.large.data, buf, len);
        }
        str->u.large.data[len] = '\0';
        str->u.large.len = len;
        str->u.large.cap = len;
        str->allocator = allocator;
        mark_as_large(str);
    }

    RS_TRACE_END();
}

rs_string_t rs_string_from_buf_with_options(const char *buf, rs_size_t len, rs_string_options_t opts)
{
    RS_TRACE_BEGIN_FMT("buf=%p, len=%zu, allocator=%p", (void *)buf, len, (void *)opts.allocator);
    rs_string_t str;
    rs_string_init_from_buf_with_options(&str, buf, len, opts);
    RS_TRACE_END();
    return str;
}

rs_string_t rs_string_clone(const rs_string_t *str)
{
    RS_TRACE_BEGIN_FMT("str=\"" RS_STRING_FMT "\"", RS_STRING_ARG(str));
    if (rs_string_is_small(str)) {
        RS_TRACE_END();
        // Small string: just copy the struct
        return *str;
    } else {
        // Large string: deep copy
        rs_string_t str2 = rs_string_from_buf(str->u.large.data, str->u.large.len, .allocator = str->allocator);
        RS_TRACE_END();
        return str2;
    }
}

void rs_string_destroy(rs_string_t *str)
{
    RS_TRACE_BEGIN_FMT("str=\"" RS_STRING_FMT "\"", RS_STRING_ARG(str));
    if (!rs_string_is_small(str) && str->u.large.data) {
        // Mask off the large flag when reading cap for free
        rs_size_t cap = str->u.large.cap & RS_STRING_CAP_MASK;
        rs_free(str->allocator, str->u.large.data, cap + 1);
        str->u.large.data = NULL;
        str->u.large.len = 0;
        str->u.large.cap = 0;
    }
    RS_TRACE_END();
}

// ============================================================================
// Modification
// ============================================================================

rs_result_t rs_string_reserve(rs_string_t *str, rs_size_t additional)
{
    RS_TRACE_BEGIN_FMT("str=\"" RS_STRING_FMT "\", additional=%zu", RS_STRING_ARG(str), additional);
    rs_size_t current_len = rs_string_len(str);
    rs_size_t current_cap = rs_string_cap(str);
    rs_size_t required = current_len + additional;

    if (required <= current_cap) {
        RS_TRACE_END();
        return RS_OK; // Already have enough capacity
    }

    // Calculate new capacity (grow by 1.5x or to required, whichever is larger)
    rs_size_t new_cap = current_cap + (current_cap / 2);
    if (new_cap < required) {
        new_cap = required;
    }

    if (rs_string_is_small(str)) {
        // Transitioning from small to large
        rs_allocator_t *allocator = str->allocator;
        char *new_data = (char *)rs_alloc(allocator, new_cap + 1);
        if (!new_data) {
            RS_TRACE_END();
            return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate %zu bytes for string (SSO -> heap)", new_cap + 1);
        }

        // Copy small string data
        memcpy(new_data, str->u.small.buf, current_len);
        new_data[current_len] = '\0';

        // Convert to large string
        str->u.large.data = new_data;
        str->u.large.len = current_len;
        str->u.large.cap = new_cap;
        mark_as_large(str);
    } else {
        // Already large, just realloc
        // Mask off the large flag when reading cap for realloc
        rs_size_t old_cap = str->u.large.cap & RS_STRING_CAP_MASK;
        char *new_data = (char *)rs_realloc(str->allocator, str->u.large.data, old_cap + 1, new_cap + 1);
        if (!new_data) {
            RS_TRACE_END();
            return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to reallocate string to %zu bytes (cap: %zu -> %zu)", new_cap + 1,
                                old_cap, new_cap);
        }

        str->u.large.data = new_data;
        // Preserve the large flag when setting new capacity
        str->u.large.cap = new_cap | RS_STRING_LARGE_FLAG;
    }

    RS_TRACE_END();
    return RS_OK;
}

rs_result_t rs_string_push_cstr(rs_string_t *str, const char *cstr)
{
    if (!cstr) {
        return RS_OK;
    }
    return rs_string_push_buf(str, cstr, strlen(cstr));
}

rs_result_t rs_string_push_buf(rs_string_t *str, const char *buf, rs_size_t len)
{
    if (!buf || len == 0) {
        return RS_OK;
    }

    rs_size_t current_len = rs_string_len(str);
    rs_size_t new_len = current_len + len;

    // Check if we need to grow
    if (new_len > rs_string_cap(str)) {
        rs_result_t result = rs_string_reserve(str, len);
        if (result != RS_OK) {
            return result;
        }
    }

    // Append data
    if (rs_string_is_small(str)) {
        memcpy(str->u.small.buf + current_len, buf, len);
        str->u.small.buf[new_len] = '\0';
        set_small_len(str, new_len);
    } else {
        memcpy(str->u.large.data + current_len, buf, len);
        str->u.large.data[new_len] = '\0';
        set_large_len(str, new_len);
    }

    return RS_OK;
}

rs_result_t rs_string_push_char(rs_string_t *str, char c)
{
    return rs_string_push_buf(str, &c, 1);
}

rs_result_t rs_string_push_string(rs_string_t *str, const rs_string_t *other)
{
    const char *data = rs_string_cstr(other);
    rs_size_t len = rs_string_len(other);
    return rs_string_push_buf(str, data, len);
}

void rs_string_clear(rs_string_t *str)
{
    if (rs_string_is_small(str)) {
        set_small_len(str, 0);
        str->u.small.buf[0] = '\0';
    } else {
        set_large_len(str, 0);
        if (str->u.large.data) {
            str->u.large.data[0] = '\0';
        }
    }
}

void rs_string_truncate(rs_string_t *str, rs_size_t len)
{
    rs_size_t current_len = rs_string_len(str);
    if (len < current_len) {
        if (rs_string_is_small(str)) {
            set_small_len(str, len);
            str->u.small.buf[len] = '\0';
        } else {
            set_large_len(str, len);
            if (str->u.large.data) {
                str->u.large.data[len] = '\0';
            }
        }
    }
}

rs_result_t rs_string_format(rs_string_t *str, const char *fmt, ...)
{
    va_list args, args_copy;
    va_start(args, fmt);
    va_copy(args_copy, args);

    // Calculate required size
    int size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (size < 0) {
        va_end(args_copy);
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid format string");
    }

    // Clear and reserve space
    rs_string_clear(str);
    if ((rs_size_t)size > rs_string_cap(str)) {
        rs_result_t result = rs_string_reserve(str, (rs_size_t)size);
        if (result != RS_OK) {
            va_end(args_copy);
            return result;
        }
    }

    // Format into buffer
    char *data = rs_string_data_mut(str);
    vsnprintf(data, size + 1, fmt, args_copy);
    va_end(args_copy);

    // Set length
    if (rs_string_is_small(str)) {
        set_small_len(str, (rs_size_t)size);
    } else {
        set_large_len(str, (rs_size_t)size);
    }

    return RS_OK;
}

// ============================================================================
// Comparison
// ============================================================================

int rs_string_cmp(const rs_string_t *a, const rs_string_t *b)
{
    rs_size_t len_a = rs_string_len(a);
    rs_size_t len_b = rs_string_len(b);
    rs_size_t min_len = len_a < len_b ? len_a : len_b;

    const char *data_a = rs_string_cstr(a);
    const char *data_b = rs_string_cstr(b);

    int cmp = memcmp(data_a, data_b, min_len);
    if (cmp != 0) {
        return cmp;
    }

    // If equal up to min_len, longer string is greater
    if (len_a < len_b)
        return -1;
    if (len_a > len_b)
        return 1;
    return 0;
}

rs_bool rs_string_eq(const rs_string_t *a, const rs_string_t *b)
{
    rs_size_t len_a = rs_string_len(a);
    rs_size_t len_b = rs_string_len(b);

    if (len_a != len_b) {
        return false;
    }

    const char *data_a = rs_string_cstr(a);
    const char *data_b = rs_string_cstr(b);

    return memcmp(data_a, data_b, len_a) == 0;
}

int rs_string_cmp_cstr(const rs_string_t *str, const char *cstr)
{
    return strcmp(rs_string_cstr(str), cstr);
}

rs_bool rs_string_eq_cstr(const rs_string_t *str, const char *cstr)
{
    if (!cstr) {
        return rs_string_is_empty(str);
    }
    return strcmp(rs_string_cstr(str), cstr) == 0;
}

// ============================================================================
// Searching
// ============================================================================

rs_ssize_t rs_string_find_char(const rs_string_t *str, char c)
{
    const char *data = rs_string_cstr(str);
    rs_size_t len = rs_string_len(str);

    for (rs_size_t i = 0; i < len; i++) {
        if (data[i] == c) {
            return (rs_ssize_t)i;
        }
    }
    return -1;
}

rs_ssize_t rs_string_find(const rs_string_t *str, const char *needle)
{
    if (!needle || !needle[0]) {
        return 0;
    }

    rs_size_t needle_len = strlen(needle);
    rs_size_t str_len = rs_string_len(str);

    if (needle_len > str_len) {
        return -1;
    }

    const char *data = rs_string_cstr(str);

    for (rs_size_t i = 0; i <= str_len - needle_len; i++) {
        if (memcmp(data + i, needle, needle_len) == 0) {
            return (rs_ssize_t)i;
        }
    }
    return -1;
}

rs_bool rs_string_starts_with(const rs_string_t *str, const char *prefix)
{
    if (!prefix) {
        return true;
    }

    rs_size_t prefix_len = strlen(prefix);
    rs_size_t str_len = rs_string_len(str);

    if (prefix_len > str_len) {
        return false;
    }

    const char *data = rs_string_cstr(str);
    return memcmp(data, prefix, prefix_len) == 0;
}

rs_bool rs_string_ends_with(const rs_string_t *str, const char *suffix)
{
    if (!suffix) {
        return true;
    }

    rs_size_t suffix_len = strlen(suffix);
    rs_size_t str_len = rs_string_len(str);

    if (suffix_len > str_len) {
        return false;
    }

    const char *data = rs_string_cstr(str);
    return memcmp(data + str_len - suffix_len, suffix, suffix_len) == 0;
}

// ============================================================================
// String Manipulation
// ============================================================================

// Helper: Check if character is whitespace
static inline rs_bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

void rs_string_trim_start(rs_string_t *str)
{
    if (!str) {
        return;
    }

    rs_size_t len = rs_string_len(str);
    if (len == 0) {
        return;
    }

    char *data = rs_string_data_mut(str);
    rs_size_t start = 0;

    // Find first non-whitespace
    while (start < len && is_whitespace(data[start])) {
        start++;
    }

    if (start == 0) {
        return; // Nothing to trim
    }

    // Shift data left
    rs_size_t new_len = len - start;
    memmove(data, data + start, new_len);
    data[new_len] = '\0';

    // Update length
    if (rs_string_is_small(str)) {
        set_small_len(str, new_len);
    } else {
        set_large_len(str, new_len);
    }
}

void rs_string_trim_end(rs_string_t *str)
{
    if (!str) {
        return;
    }

    rs_size_t len = rs_string_len(str);
    if (len == 0) {
        return;
    }

    char *data = rs_string_data_mut(str);
    rs_size_t end = len;

    // Find last non-whitespace
    while (end > 0 && is_whitespace(data[end - 1])) {
        end--;
    }

    if (end == len) {
        return; // Nothing to trim
    }

    // Update length and null terminate
    data[end] = '\0';
    if (rs_string_is_small(str)) {
        set_small_len(str, end);
    } else {
        set_large_len(str, end);
    }
}

void rs_string_trim(rs_string_t *str)
{
    rs_string_trim_end(str);
    rs_string_trim_start(str);
}

void rs_string_to_lower(rs_string_t *str)
{
    if (!str) {
        return;
    }

    rs_size_t len = rs_string_len(str);
    char *data = rs_string_data_mut(str);

    for (rs_size_t i = 0; i < len; i++) {
        data[i] = (char)tolower((unsigned char)data[i]);
    }
}

void rs_string_to_upper(rs_string_t *str)
{
    if (!str) {
        return;
    }

    rs_size_t len = rs_string_len(str);
    char *data = rs_string_data_mut(str);

    for (rs_size_t i = 0; i < len; i++) {
        data[i] = (char)toupper((unsigned char)data[i]);
    }
}

void rs_string_reverse(rs_string_t *str)
{
    if (!str) {
        return;
    }

    rs_size_t len = rs_string_len(str);
    if (len <= 1) {
        return;
    }

    char *data = rs_string_data_mut(str);
    rs_size_t left = 0;
    rs_size_t right = len - 1;

    while (left < right) {
        char temp = data[left];
        data[left] = data[right];
        data[right] = temp;
        left++;
        right--;
    }
}
