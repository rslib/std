#pragma once

#include <rs/std/containers/array.h>
#include <rs/std/internal/api.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Generic immutable span (non-owning view into contiguous memory).
 *
 * Properties:
 * - Does not own memory
 * - Immutable (read-only)
 * - Lightweight (pointer + length + elem_size)
 * - Type-erased (user handles casting)
 *
 * Use cases:
 * - Function parameters (avoid copies)
 * - Sub-ranges without allocation
 * - Viewing arrays/buffers
 *
 * Example:
 *   int data[] = {1, 2, 3, 4, 5};
 *   rs_span_t span = rs_span_create(data, 5, sizeof(int));
 *   rs_span_t sub = rs_span_slice(span, 1, 4);  // [2, 3, 4]
 */
typedef struct {
    const void *data;
    rs_size_t len;       // Length in elements
    rs_size_t elem_size; // Size of each element
} rs_span_t;

// ============================================================================
// Creation
// ============================================================================

/**
 * Create span from data pointer.
 */
static inline rs_span_t rs_span_create(const void *data, rs_size_t len, rs_size_t elem_size)
{
    rs_span_t span = {
        .data = data,
        .len = len,
        .elem_size = elem_size,
    };
    return span;
}

/**
 * Create empty span.
 */
static inline rs_span_t rs_span_empty(rs_size_t elem_size)
{
    rs_span_t span = {
        .data = NULL,
        .len = 0,
        .elem_size = elem_size,
    };
    return span;
}

/**
 * Create span from array.
 */
static inline rs_span_t rs_span_from_array(const rs_array_t *arr)
{
    return rs_span_create(arr->data, arr->len, arr->elem_size);
}

// ============================================================================
// Properties
// ============================================================================

/**
 * Get length.
 */
static inline rs_size_t rs_span_len(rs_span_t span)
{
    return span.len;
}

/**
 * Check if empty.
 */
static inline rs_bool rs_span_is_empty(rs_span_t span)
{
    return span.len == 0;
}

/**
 * Get element size.
 */
static inline rs_size_t rs_span_elem_size(rs_span_t span)
{
    return span.elem_size;
}

/**
 * Get data pointer.
 */
static inline const void *rs_span_data(rs_span_t span)
{
    return span.data;
}

// ============================================================================
// Access
// ============================================================================

/**
 * Get pointer to element at index (no bounds checking).
 */
static inline const void *rs_span_get(rs_span_t span, rs_size_t index)
{
    return (const char *)span.data + (index * span.elem_size);
}

/**
 * Get pointer to first element (NULL if empty).
 */
static inline const void *rs_span_first(rs_span_t span)
{
    return span.len > 0 ? span.data : NULL;
}

/**
 * Get pointer to last element (NULL if empty).
 */
static inline const void *rs_span_last(rs_span_t span)
{
    return span.len > 0 ? rs_span_get(span, span.len - 1) : NULL;
}

// ============================================================================
// Slicing
// ============================================================================

/**
 * Create sub-span [start..end).
 * If end > len, clamps to len.
 */
static inline rs_span_t rs_span_slice(rs_span_t span, rs_size_t start, rs_size_t end)
{
    if (start > span.len) {
        start = span.len;
    }
    if (end > span.len) {
        end = span.len;
    }
    if (start > end) {
        start = end;
    }

    return rs_span_create((const char *)span.data + (start * span.elem_size), end - start, span.elem_size);
}

/**
 * Slice from start to end.
 */
static inline rs_span_t rs_span_slice_from(rs_span_t span, rs_size_t start)
{
    return rs_span_slice(span, start, span.len);
}

/**
 * Slice from beginning to end.
 */
static inline rs_span_t rs_span_slice_to(rs_span_t span, rs_size_t end)
{
    return rs_span_slice(span, 0, end);
}

// ============================================================================
// Comparison
// ============================================================================

/**
 * Compare two spans (memcmp-style).
 * Returns <0, 0, or >0.
 */
RS_STD_API int rs_span_cmp(rs_span_t a, rs_span_t b);

/**
 * Check equality.
 */
RS_STD_API rs_bool rs_span_eq(rs_span_t a, rs_span_t b);

// ============================================================================
// Convenience macros
// ============================================================================

/**
 * Create typed span.
 */
#define rs_span_create_type(T, data, len) rs_span_create(data, len, sizeof(T))

/**
 * Create span from C array.
 */
#define rs_span_from_c_array(arr) rs_span_create(arr, sizeof(arr) / sizeof((arr)[0]), sizeof((arr)[0]))

RS_EXTERN_C_END
