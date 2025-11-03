#include <rs/std/containers/span.h>
#include <string.h>

// ============================================================================
// Comparison
// ============================================================================

int rs_span_cmp(rs_span_t a, rs_span_t b)
{
    // Element sizes must match
    if (a.elem_size != b.elem_size) {
        return (a.elem_size > b.elem_size) ? 1 : -1;
    }

    // Compare lengths
    rs_size_t min_len = a.len < b.len ? a.len : b.len;

    if (min_len > 0) {
        int cmp = memcmp(a.data, b.data, min_len * a.elem_size);
        if (cmp != 0) {
            return cmp;
        }
    }

    // If prefixes are equal, shorter span is less
    if (a.len < b.len) {
        return -1;
    } else if (a.len > b.len) {
        return 1;
    }

    return 0;
}

rs_bool rs_span_eq(rs_span_t a, rs_span_t b)
{
    if (a.elem_size != b.elem_size) {
        return false;
    }

    if (a.len != b.len) {
        return false;
    }

    if (a.len == 0) {
        return true;
    }

    return memcmp(a.data, b.data, a.len * a.elem_size) == 0;
}
