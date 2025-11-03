#include <rs/std/containers/slice.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Modification
// ============================================================================

void rs_slice_fill(rs_slice_t slice, const void *val)
{
    for (rs_size_t i = 0; i < slice.len; i++) {
        void *dest = (char *)slice.data + (i * slice.elem_size);
        memcpy(dest, val, slice.elem_size);
    }
}

rs_size_t rs_slice_copy_from_span(rs_slice_t dst, rs_span_t src)
{
    // Element sizes must match
    if (dst.elem_size != src.elem_size) {
        return 0;
    }

    rs_size_t count = dst.len < src.len ? dst.len : src.len;

    if (count > 0) {
        memcpy(dst.data, src.data, count * dst.elem_size);
    }

    return count;
}

rs_size_t rs_slice_copy_from_slice(rs_slice_t dst, rs_slice_t src)
{
    // Element sizes must match
    if (dst.elem_size != src.elem_size) {
        return 0;
    }

    rs_size_t count = dst.len < src.len ? dst.len : src.len;

    if (count > 0) {
        // Use memmove in case of overlap
        memmove(dst.data, src.data, count * dst.elem_size);
    }

    return count;
}

void rs_slice_swap(rs_slice_t slice, rs_size_t i, rs_size_t j)
{
    if (i >= slice.len || j >= slice.len || i == j) {
        return;
    }

    char *a = (char *)slice.data + (i * slice.elem_size);
    char *b = (char *)slice.data + (j * slice.elem_size);

    // Swap using temporary buffer
    char temp[256]; // Stack buffer for small elements
    char *temp_buf = temp;
    char *heap_buf = NULL;

    if (slice.elem_size > sizeof(temp)) {
        // Large element - allocate on heap
        heap_buf = (char *)malloc(slice.elem_size);
        if (!heap_buf) {
            return; // Failed to allocate
        }
        temp_buf = heap_buf;
    }

    memcpy(temp_buf, a, slice.elem_size);
    memcpy(a, b, slice.elem_size);
    memcpy(b, temp_buf, slice.elem_size);

    if (heap_buf) {
        free(heap_buf);
    }
}

void rs_slice_reverse(rs_slice_t slice)
{
    if (slice.len <= 1) {
        return;
    }

    rs_size_t left = 0;
    rs_size_t right = slice.len - 1;

    while (left < right) {
        rs_slice_swap(slice, left, right);
        left++;
        right--;
    }
}

// ============================================================================
// Comparison
// ============================================================================

int rs_slice_cmp(rs_slice_t a, rs_slice_t b)
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

    // If prefixes are equal, shorter slice is less
    if (a.len < b.len) {
        return -1;
    } else if (a.len > b.len) {
        return 1;
    }

    return 0;
}

rs_bool rs_slice_eq(rs_slice_t a, rs_slice_t b)
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
