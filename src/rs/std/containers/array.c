#include <assert.h>
#include <rs/std/containers/array.h>
#include <rs/std/error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Creation & Destruction
// ============================================================================

rs_array_t rs_array_create_with_options(rs_size_t elem_size, rs_array_options_t options)
{
    rs_array_t arr;
    if (rs_array_init_with_options(&arr, elem_size, options) != RS_OK) {
        return (rs_array_t){0};
    }

    return arr;
}

rs_result_t rs_array_init_with_options(rs_array_t *arr, rs_size_t elem_size, rs_array_options_t options)
{
    if (!options.allocator) {
        options.allocator = rs_allocator_system();
    }

    arr->data = NULL;
    arr->len = 0;
    arr->cap = 0;
    arr->elem_size = elem_size;
    arr->allocator = options.allocator;

    // Pre-allocate if initial_capacity is specified
    if (options.initial_capacity > 0) {
        rs_result_t result = rs_array_reserve(arr, options.initial_capacity);
        if (result != RS_OK) {
            return result;
        }
    }

    return RS_OK;
}

rs_array_t rs_array_clone_with_options(rs_array_t arr, rs_array_options_t options)
{
    rs_array_t clone;
    rs_array_options_t opts = {
        .allocator = options.allocator ? options.allocator : arr.allocator,
        .initial_capacity = options.initial_capacity > 0 ? options.initial_capacity : arr.len,
    };

    if (options.initial_capacity < arr.len) {
        opts.initial_capacity = arr.len;
    }

    if (rs_array_init_with_options(&clone, arr.elem_size, opts) != RS_OK) {
        return (rs_array_t){0};
    }

    // here the data pointer is allocated with the correct capacity
    if (clone.data && arr.data && arr.len > 0) {
        memcpy(clone.data, arr.data, arr.len * arr.elem_size);
        clone.len = arr.len;
    }

    return clone;
}

void rs_array_destroy(rs_array_t *arr)
{
    if (arr->data) {
        rs_free(arr->allocator, arr->data, arr->cap * arr->elem_size);
        arr->data = NULL;
    }

    arr->len = 0;
    arr->cap = 0;
}

// ============================================================================
// Modification
// ============================================================================

rs_result_t rs_array_reserve(rs_array_t *arr, rs_size_t additional)
{
    rs_size_t required = arr->len + additional;

    if (required <= arr->cap) {
        return RS_OK; // Already have enough capacity
    }

    // Grow by 1.5x or to required, whichever is larger
    rs_size_t new_cap = arr->cap + (arr->cap / 2);
    if (new_cap < required) {
        new_cap = required;
    }

    // Handle initial allocation
    if (new_cap < 8) {
        new_cap = 8;
    }

    rs_size_t new_size = new_cap * arr->elem_size;
    rs_size_t old_size = arr->cap * arr->elem_size;

    void *new_data = rs_realloc(arr->allocator, arr->data, old_size, new_size);
    if (!new_data) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate %zu bytes for array (cap: %zu -> %zu)", new_size,
                            arr->cap, new_cap);
    }

    arr->data = new_data;
    arr->cap = new_cap;

    return RS_OK;
}

rs_result_t rs_array_push(rs_array_t *arr, const void *elem)
{
    // Ensure capacity
    if (arr->len >= arr->cap) {
        rs_result_t result = rs_array_reserve(arr, 1);
        if (result != RS_OK) {
            return result;
        }
    }

    // Copy element
    void *dest = (char *)arr->data + (arr->len * arr->elem_size);
    memcpy(dest, elem, arr->elem_size);
    arr->len++;

    return RS_OK;
}

rs_result_t rs_array_pop(rs_array_t *arr, void *out)
{
    if (arr->len == 0) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Cannot pop from empty array");
    }

    arr->len--;

    if (out) {
        void *src = (char *)arr->data + (arr->len * arr->elem_size);
        memcpy(out, src, arr->elem_size);
    }

    return RS_OK;
}

rs_result_t rs_array_insert(rs_array_t *arr, rs_size_t index, const void *elem)
{
    if (index > arr->len) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Insert index %zu out of bounds (len: %zu)", index, arr->len);
    }

    // Ensure capacity
    if (arr->len >= arr->cap) {
        rs_result_t result = rs_array_reserve(arr, 1);
        if (result != RS_OK) {
            return result;
        }
    }

    // Shift elements right
    if (index < arr->len) {
        void *dest = (char *)arr->data + ((index + 1) * arr->elem_size);
        void *src = (char *)arr->data + (index * arr->elem_size);
        rs_size_t count = (arr->len - index) * arr->elem_size;
        memmove(dest, src, count);
    }

    // Insert element
    void *dest = (char *)arr->data + (index * arr->elem_size);
    memcpy(dest, elem, arr->elem_size);
    arr->len++;

    return RS_OK;
}

rs_result_t rs_array_remove(rs_array_t *arr, rs_size_t index, void *out)
{
    if (index >= arr->len) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Remove index %zu out of bounds (len: %zu)", index, arr->len);
    }

    // Copy element to out if requested
    if (out) {
        void *src = (char *)arr->data + (index * arr->elem_size);
        memcpy(out, src, arr->elem_size);
    }

    // Shift elements left
    if (index < arr->len - 1) {
        void *dest = (char *)arr->data + (index * arr->elem_size);
        void *src = (char *)arr->data + ((index + 1) * arr->elem_size);
        rs_size_t count = (arr->len - index - 1) * arr->elem_size;
        memmove(dest, src, count);
    }

    arr->len--;

    return RS_OK;
}

void rs_array_clear(rs_array_t *arr)
{
    arr->len = 0;
}

rs_result_t rs_array_resize(rs_array_t *arr, rs_size_t new_len)
{
    if (new_len > arr->cap) {
        // Need to grow
        rs_result_t result = rs_array_reserve(arr, new_len - arr->len);
        if (result != RS_OK) {
            return result;
        }
    }

    // Zero-initialize new elements if growing
    if (new_len > arr->len) {
        void *start = (char *)arr->data + (arr->len * arr->elem_size);
        rs_size_t count = (new_len - arr->len) * arr->elem_size;
        memset(start, 0, count);
    }

    arr->len = new_len;

    return RS_OK;
}

rs_result_t rs_array_shrink_to_fit(rs_array_t *arr)
{
    if (arr->len == arr->cap) {
        return RS_OK; // Already optimal
    }

    if (arr->len == 0) {
        // Free all memory
        rs_array_destroy(arr);
        return RS_OK;
    }

    // Reallocate to exact size
    rs_size_t new_size = arr->len * arr->elem_size;
    rs_size_t old_size = arr->cap * arr->elem_size;

    void *new_data = rs_realloc(arr->allocator, arr->data, old_size, new_size);
    if (!new_data) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to shrink array to %zu bytes (cap: %zu -> %zu)", new_size, arr->cap,
                            arr->len);
    }

    arr->data = new_data;
    arr->cap = arr->len;

    return RS_OK;
}

// ============================================================================
// Sorting
// ============================================================================

// Thread-local storage for comparator context (used by qsort wrapper)
static RS_THREAD_LOCAL struct {
    rs_compare_fn compare;
    void *user_data;
} g_sors_context;

static int rs_array_qsors_wrapper(const void *a, const void *b)
{
    return g_sors_context.compare(a, b, g_sors_context.user_data);
}

void rs_array_sort(rs_array_t *arr, rs_compare_fn compare, void *user_data)
{
    if (arr->len <= 1 || !compare) {
        return; // Nothing to sort or no comparator
    }

    // Store context for qsort wrapper
    g_sors_context.compare = compare;
    g_sors_context.user_data = user_data;

    // Sort using qsort
    qsort(arr->data, arr->len, arr->elem_size, rs_array_qsors_wrapper);
}
