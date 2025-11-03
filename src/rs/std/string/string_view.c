#define RS_STD_LOG_MODULE "string_view"

#include <ctype.h>
#include <rs/std/allocators/allocator.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <string.h>

// ============================================================================
// Creation
// ============================================================================

rs_string_view_t rs_sv_from_cstr(const char *cstr)
{
    RS_TRACE_BEGIN_FMT("cstr=%p", (void *)cstr);
    if (!cstr) {
        RS_TRACE_END();
        return rs_sv_empty();
    }
    rs_string_view_t sv = rs_sv_from_buf(cstr, strlen(cstr));
    RS_TRACE_END();
    return sv;
}

rs_string_view_t rs_sv_from_buf(const char *buf, rs_size_t len)
{
    RS_TRACE_BEGIN_FMT("buf=%p, len=%zu", (void *)buf, len);
    rs_string_view_t sv = {buf, len};
    RS_TRACE_END();
    return sv;
}

rs_string_view_t rs_sv_from_string(rs_string_t str)
{
    RS_TRACE_BEGIN_FMT("str=%p", (void *)&str);
    rs_string_view_t sv = rs_sv_from_buf(rs_string_cstr(&str), rs_string_len(&str));
    RS_TRACE_END();
    return sv;
}

// ============================================================================
// Slicing
// ============================================================================

rs_string_view_t rs_sv_slice(rs_string_view_t sv, rs_size_t start, rs_size_t end)
{
    RS_TRACE_BEGIN_FMT("sv=%p, start=%zu, end=%zu", (void *)&sv, start, end);
    if (start >= sv.len) {
        RS_TRACE_END();
        return rs_sv_empty();
    }
    if (end > sv.len) {
        end = sv.len;
    }
    if (start >= end) {
        RS_TRACE_END();
        return rs_sv_empty();
    }
    rs_string_view_t result = rs_sv_from_buf(sv.data + start, end - start);
    RS_TRACE_END();
    return result;
}

rs_string_view_t rs_sv_slice_from(rs_string_view_t sv, rs_size_t start)
{
    RS_TRACE_BEGIN_FMT("sv=%p, start=%zu", (void *)&sv, start);
    rs_string_view_t result = rs_sv_slice(sv, start, sv.len);
    RS_TRACE_END();
    return result;
}

rs_string_view_t rs_sv_slice_to(rs_string_view_t sv, rs_size_t end)
{
    RS_TRACE_BEGIN_FMT("sv=%p, end=%zu", (void *)&sv, end);
    rs_string_view_t result = rs_sv_slice(sv, 0, end);
    RS_TRACE_END();
    return result;
}

rs_string_view_t rs_sv_trim_prefix(rs_string_view_t sv, rs_string_view_t prefix)
{
    RS_TRACE_BEGIN_FMT("sv=%p, prefix=%p", (void *)&sv, (void *)&prefix);
    if (rs_sv_is_empty(prefix) || !rs_sv_starts_with(sv, prefix)) {
        RS_TRACE_END();
        return sv;
    }
    rs_string_view_t result = rs_sv_slice_from(sv, prefix.len);
    RS_TRACE_END();
    return result;
}

rs_string_view_t rs_sv_trim_suffix(rs_string_view_t sv, rs_string_view_t suffix)
{
    RS_TRACE_BEGIN_FMT("sv=%p, suffix=%p", (void *)&sv, (void *)&suffix);
    if (rs_sv_is_empty(suffix) || !rs_sv_ends_with(sv, suffix)) {
        RS_TRACE_END();
        return sv;
    }
    rs_string_view_t result = rs_sv_slice_to(sv, sv.len - suffix.len);
    RS_TRACE_END();
    return result;
}

rs_string_view_t rs_sv_trim_left(rs_string_view_t sv)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT, RS_SV_ARG(sv));
    rs_size_t start = 0;
    while (start < sv.len && isspace((unsigned char)sv.data[start])) {
        start++;
    }
    rs_string_view_t result = rs_sv_slice_from(sv, start);
    RS_TRACE_END();
    return result;
}

rs_string_view_t rs_sv_trim_right(rs_string_view_t sv)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT, RS_SV_ARG(sv));
    rs_size_t end = sv.len;
    while (end > 0 && isspace((unsigned char)sv.data[end - 1])) {
        end--;
    }
    rs_string_view_t result = rs_sv_slice_to(sv, end);
    RS_TRACE_END();
    return result;
}

rs_string_view_t rs_sv_trim(rs_string_view_t sv)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT, RS_SV_ARG(sv));
    rs_string_view_t result = rs_sv_trim_right(rs_sv_trim_left(sv));
    RS_TRACE_END();
    return result;
}

// ============================================================================
// Comparison
// ============================================================================

int rs_sv_cmp(rs_string_view_t a, rs_string_view_t b)
{
    RS_TRACE_BEGIN_FMT("a=" RS_SV_FMT ", b=" RS_SV_FMT, RS_SV_ARG(a), RS_SV_ARG(b));
    rs_size_t min_len = a.len < b.len ? a.len : b.len;
    int cmp = memcmp(a.data, b.data, min_len);
    if (cmp != 0) {
        RS_TRACE_END();
        return cmp;
    }
    // If equal up to min_len, longer string is greater
    if (a.len < b.len) {
        RS_TRACE_END();
        return -1;
    }
    if (a.len > b.len) {
        RS_TRACE_END();
        return 1;
    }
    RS_TRACE_END();
    return 0;
}

rs_bool rs_sv_eq(rs_string_view_t a, rs_string_view_t b)
{
    RS_TRACE_BEGIN_FMT("a=" RS_SV_FMT ", b=" RS_SV_FMT, RS_SV_ARG(a), RS_SV_ARG(b));
    if (a.len != b.len) {
        RS_TRACE_END();
        return false;
    }
    RS_TRACE_END();
    return memcmp(a.data, b.data, a.len) == 0;
}

// ============================================================================
// Searching
// ============================================================================

rs_ssize_t rs_sv_find_char(rs_string_view_t sv, char c)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT ", c=%c", RS_SV_ARG(sv), c);
    for (rs_size_t i = 0; i < sv.len; i++) {
        if (sv.data[i] == c) {
            RS_TRACE_END();
            return (rs_ssize_t)i;
        }
    }
    RS_TRACE_END();
    return -1;
}

rs_ssize_t rs_sv_find(rs_string_view_t sv, rs_string_view_t needle)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT ", needle=" RS_SV_FMT, RS_SV_ARG(sv), RS_SV_ARG(needle));
    if (rs_sv_is_empty(needle)) {
        RS_TRACE_END();
        return 0;
    }

    if (needle.len > sv.len) {
        RS_TRACE_END();
        return -1;
    }

    for (rs_size_t i = 0; i <= sv.len - needle.len; i++) {
        if (memcmp(sv.data + i, needle.data, needle.len) == 0) {
            RS_TRACE_END();
            return (rs_ssize_t)i;
        }
    }
    RS_TRACE_END();
    return -1;
}

rs_bool rs_sv_starts_with(rs_string_view_t sv, rs_string_view_t prefix)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT ", prefix=" RS_SV_FMT, RS_SV_ARG(sv), RS_SV_ARG(prefix));
    if (rs_sv_is_empty(prefix)) {
        RS_TRACE_END();
        return true;
    }
    if (prefix.len > sv.len) {
        RS_TRACE_END();
        return false;
    }
    RS_TRACE_END();
    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

rs_bool rs_sv_ends_with(rs_string_view_t sv, rs_string_view_t suffix)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT ", suffix=" RS_SV_FMT, RS_SV_ARG(sv), RS_SV_ARG(suffix));
    if (rs_sv_is_empty(suffix)) {
        RS_TRACE_END();
        return true;
    }
    if (suffix.len > sv.len) {
        RS_TRACE_END();
        return false;
    }
    RS_TRACE_END();
    return memcmp(sv.data + sv.len - suffix.len, suffix.data, suffix.len) == 0;
}

// ============================================================================
// Conversion
// ============================================================================

rs_string_t rs_sv_to_string_with_options(rs_string_view_t sv, rs_sv_options_t opts)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT ", allocator=%p", RS_SV_ARG(sv), (void *)opts.allocator);
    rs_string_t trimmed_sv = rs_string_from_buf(sv.data, sv.len, .allocator = opts.allocator);
    RS_TRACE_END();
    return trimmed_sv;
}

char *rs_sv_to_cstr_with_options(rs_string_view_t sv, rs_sv_options_t opts)
{
    RS_TRACE_BEGIN_FMT("sv=" RS_SV_FMT ", allocator=%p", RS_SV_ARG(sv), (void *)opts.allocator);
    rs_allocator_t *allocator = opts.allocator;
    if (!allocator) {
        allocator = rs_allocator_system();
    }

    char *cstr = (char *)rs_alloc(allocator, sv.len + 1);
    if (!cstr) {
        RS_TRACE_END();
        return NULL;
    }

    if (sv.len > 0) {
        memcpy(cstr, sv.data, sv.len);
    }
    cstr[sv.len] = '\0';

    RS_TRACE_END();
    return cstr;
}
