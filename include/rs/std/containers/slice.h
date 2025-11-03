#pragma once

#include <rs/std/containers/array.h>
#include <rs/std/containers/span.h>
#include <rs/std/internal/api.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Generic mutable slice (non-owning mutable view into contiguous memory).
 *
 * Properties:
 * - Does not own memory
 * - Mutable (read-write)
 * - Lightweight (pointer + length + elem_size)
 * - Type-erased (user handles casting)
 *
 * Use cases:
 * - Mutable function parameters
 * - In-place modification of sub-ranges
 * - Sorting/transforming portions of arrays
 *
 * Example:
 *   int data[] = {1, 2, 3, 4, 5};
 *   rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));
 *   rs_slice_t sub = rs_slice_slice(slice, 1, 4);  // [2, 3, 4]
 *   *(int*)rs_slice_get(sub, 0) = 99;  // data is now {1, 99, 3, 4, 5}
 */
typedef struct {
    void *data;
    rs_size_t len;       // Length in elements
    rs_size_t elem_size; // Size of each element
} rs_slice_t;

// ============================================================================
// Creation
// ============================================================================

/**
 * Create slice from data pointer.
 */
static inline rs_slice_t rs_slice_create(void *data, rs_size_t len, rs_size_t elem_size)
{
    rs_slice_t slice = {
        .data = data,
        .len = len,
        .elem_size = elem_size,
    };
    return slice;
}

/**
 * Create empty slice.
 */
static inline rs_slice_t rs_slice_empty(rs_size_t elem_size)
{
    rs_slice_t slice = {
        .data = NULL,
        .len = 0,
        .elem_size = elem_size,
    };
    return slice;
}

/**
 * Create slice from array.
 */
static inline rs_slice_t rs_slice_from_array(rs_array_t *arr)
{
    return rs_slice_create(arr->data, arr->len, arr->elem_size);
}

/**
 * Create immutable span from slice.
 */
static inline rs_span_t rs_slice_to_span(rs_slice_t slice)
{
    return rs_span_create(slice.data, slice.len, slice.elem_size);
}

// ============================================================================
// Properties
// ============================================================================

/**
 * Get length.
 */
static inline rs_size_t rs_slice_len(rs_slice_t slice)
{
    return slice.len;
}

/**
 * Check if empty.
 */
static inline rs_bool rs_slice_is_empty(rs_slice_t slice)
{
    return slice.len == 0;
}

/**
 * Get element size.
 */
static inline rs_size_t rs_slice_elem_size(rs_slice_t slice)
{
    return slice.elem_size;
}

/**
 * Get data pointer.
 */
static inline void *rs_slice_data(rs_slice_t slice)
{
    return slice.data;
}

// ============================================================================
// Access
// ============================================================================

/**
 * Get pointer to element at index (no bounds checking).
 */
static inline void *rs_slice_get(rs_slice_t slice, rs_size_t index)
{
    return (char *)slice.data + (index * slice.elem_size);
}

/**
 * Get pointer to first element (NULL if empty).
 */
static inline void *rs_slice_first(rs_slice_t slice)
{
    return slice.len > 0 ? slice.data : NULL;
}

/**
 * Get pointer to last element (NULL if empty).
 */
static inline void *rs_slice_last(rs_slice_t slice)
{
    return slice.len > 0 ? rs_slice_get(slice, slice.len - 1) : NULL;
}

// ============================================================================
// Slicing
// ============================================================================

/**
 * Create sub-slice [start..end).
 * If end > len, clamps to len.
 */
static inline rs_slice_t rs_slice_slice(rs_slice_t slice, rs_size_t start, rs_size_t end)
{
    if (start > slice.len) {
        start = slice.len;
    }
    if (end > slice.len) {
        end = slice.len;
    }
    if (start > end) {
        start = end;
    }

    return rs_slice_create((char *)slice.data + (start * slice.elem_size), end - start, slice.elem_size);
}

/**
 * Slice from start to end.
 */
static inline rs_slice_t rs_slice_slice_from(rs_slice_t slice, rs_size_t start)
{
    return rs_slice_slice(slice, start, slice.len);
}

/**
 * Slice from beginning to end.
 */
static inline rs_slice_t rs_slice_slice_to(rs_slice_t slice, rs_size_t end)
{
    return rs_slice_slice(slice, 0, end);
}

// ============================================================================
// Modification
// ============================================================================

/**
 * Fill slice with value (copies elem_size bytes from val to each element).
 */
RS_STD_API void rs_slice_fill(rs_slice_t slice, const void *val);

/**
 * Copy from source span to destination slice.
 * Copies min(dst.len, src.len) elements.
 * Returns number of elements copied.
 */
RS_STD_API rs_size_t rs_slice_copy_from_span(rs_slice_t dst, rs_span_t src);

/**
 * Copy from source slice to destination slice.
 * Copies min(dst.len, src.len) elements.
 * Returns number of elements copied.
 */
RS_STD_API rs_size_t rs_slice_copy_from_slice(rs_slice_t dst, rs_slice_t src);

/**
 * Swap two elements at given indices.
 */
RS_STD_API void rs_slice_swap(rs_slice_t slice, rs_size_t i, rs_size_t j);

/**
 * Reverse elements in slice.
 */
RS_STD_API void rs_slice_reverse(rs_slice_t slice);

// ============================================================================
// Comparison
// ============================================================================

/**
 * Compare two slices (memcmp-style).
 * Returns <0, 0, or >0.
 */
RS_STD_API int rs_slice_cmp(rs_slice_t a, rs_slice_t b);

/**
 * Check equality.
 */
RS_STD_API rs_bool rs_slice_eq(rs_slice_t a, rs_slice_t b);

// ============================================================================
// Convenience macros
// ============================================================================

/**
 * Create typed slice.
 */
#define rs_slice_create_type(T, data, len) rs_slice_create(data, len, sizeof(T))

/**
 * Create slice from C array.
 */
#define rs_slice_from_c_array(arr) rs_slice_create(arr, sizeof(arr) / sizeof((arr)[0]), sizeof((arr)[0]))

RS_EXTERN_C_END
