#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Comparator function for array elements.
 *
 * @param a First element to compare
 * @param b Second element to compare
 * @param user_data Optional user data passed to the comparator
 * @return < 0 if a < b, 0 if a == b, > 0 if a > b
 *
 * This follows the standard qsort comparator convention.
 */
typedef int (*rs_compare_fn)(const void *a, const void *b, void *user_data);

/**
 * Generic dynamic array (growable, type-erased).
 *
 * Properties:
 * - Contiguous storage
 * - Automatic growth (amortized O(1) push)
 * - Type-erased (user handles casting)
 * - Custom allocator support
 *
 * Use cases:
 * - Variable-length collections
 * - Stack-like operations (push/pop)
 * - Sequential data processing
 *
 * Example:
 *   rs_array_t arr = rs_array_create(sizeof(int), my_allocator);
 *   int x = 42;
 *   rs_array_push(&arr, &x);
 *   int *val = (int*)rs_array_get(&arr, 0);
 *   printf("%d\n", *val);
 *   rs_array_destroy(&arr);
 */
typedef struct {
    void *data;                // Pointer to elements
    rs_size_t len;             // Number of elements
    rs_size_t cap;             // Capacity in elements
    rs_size_t elem_size;       // Size of each element
    rs_allocator_t *allocator; // Allocator
} rs_array_t;

typedef struct {
    rs_size_t initial_capacity; // Initial capacity (0 = default)
    rs_allocator_t *allocator;  // Allocator
    rs_u64 reserved;            // Reserved for future use
} rs_array_options_t;

// ============================================================================
// Creation & Destruction
// ============================================================================

/**
 * Create array.
 */
RS_STD_API rs_array_t rs_array_create_with_options(rs_size_t elem_size, rs_array_options_t options);

#define rs_array_create(elem_size, ...)                                                                                \
    rs_array_create_with_options(elem_size, (rs_array_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Create empty array with default options (system allocator, zero capacity).
 */
RS_STD_API rs_result_t rs_array_init_with_options(rs_array_t *arr, rs_size_t elem_size, rs_array_options_t options);

/**
 * Create empty array with options specified as variadic arguments.
 */
#define rs_array_init(array, ...) rs_array_init_with_options(array, (rs_array_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Clone array (deep copy).
 */
RS_STD_API rs_array_t rs_array_clone_with_options(rs_array_t arr, rs_array_options_t options);

/**
 * Clone array with default options.
 */
#define rs_array_clone(arr) rs_array_clone_with_options(arr, (rs_array_options_t){0})

/*
 * Clone array with same options with as original.
 */
static inline rs_array_t rs_array_clone_with_same_options(rs_array_t arr)
{
    return rs_array_clone_with_options(arr, (rs_array_options_t){
                                                .allocator = arr.allocator,
                                                .initial_capacity = arr.len,
                                            });
}

/**
 * Destroy array and free memory.
 */
RS_STD_API void rs_array_destroy(rs_array_t *arr);

// ============================================================================
// Properties
// ============================================================================

/**
 * Get number of elements.
 */
static inline rs_size_t rs_array_len(const rs_array_t *arr)
{
    return arr->len;
}

/**
 * Get capacity.
 */
static inline rs_size_t rs_array_cap(const rs_array_t *arr)
{
    return arr->cap;
}

/**
 * Check if empty.
 */
static inline rs_bool rs_array_is_empty(const rs_array_t *arr)
{
    return arr->len == 0;
}

/**
 * Get element size.
 */
static inline rs_size_t rs_array_elem_size(const rs_array_t *arr)
{
    return arr->elem_size;
}

/**
 * Get array allocator.
 */
static inline rs_allocator_t *rs_array_allocator(const rs_array_t *arr)
{
    return arr->allocator;
}

// ============================================================================
// Access
// ============================================================================

/**
 * Get pointer to element at index (no bounds checking).
 * Returns void* - user must cast to appropriate type.
 */
static inline void *rs_array_get(const rs_array_t *arr, rs_size_t index)
{
    return (char *)arr->data + (index * arr->elem_size);
}

/**
 * Get pointer to first element (NULL if empty).
 */
static inline void *rs_array_first(const rs_array_t *arr)
{
    return arr->len > 0 ? arr->data : NULL;
}

/**
 * Get pointer to last element (NULL if empty).
 */
static inline void *rs_array_last(const rs_array_t *arr)
{
    return arr->len > 0 ? rs_array_get(arr, arr->len - 1) : NULL;
}

/**
 * Get raw data pointer.
 */
static inline void *rs_array_data(const rs_array_t *arr)
{
    return arr->data;
}

// ============================================================================
// Modification
// ============================================================================

/**
 * Reserve capacity for additional elements.
 */
RS_STD_API rs_result_t rs_array_reserve(rs_array_t *arr, rs_size_t additional);

/**
 * Push element to end (copies elem_size bytes from elem).
 */
RS_STD_API rs_result_t rs_array_push(rs_array_t *arr, const void *elem);

/**
 * Pop element from end (copies to out if non-NULL).
 * Returns RS_ERR_INVALID if array is empty.
 */
RS_STD_API rs_result_t rs_array_pop(rs_array_t *arr, void *out);

/**
 * Insert element at index (shifts elements right).
 */
RS_STD_API rs_result_t rs_array_insert(rs_array_t *arr, rs_size_t index, const void *elem);

/**
 * Remove element at index (shifts elements left).
 * Copies removed element to out if non-NULL.
 */
RS_STD_API rs_result_t rs_array_remove(rs_array_t *arr, rs_size_t index, void *out);

/**
 * Clear array (set length to 0, keep capacity).
 */
RS_STD_API void rs_array_clear(rs_array_t *arr);

/**
 * Resize array to new length.
 * If growing, new elements are zero-initialized.
 * If shrinking, excess elements are discarded.
 */
RS_STD_API rs_result_t rs_array_resize(rs_array_t *arr, rs_size_t new_len);

/**
 * Shrink capacity to fit length (minimize memory usage).
 */
RS_STD_API rs_result_t rs_array_shrink_to_fit(rs_array_t *arr);

// ============================================================================
// Sorting
// ============================================================================

/**
 * Sort array in-place using qsort.
 *
 * @param arr Array to sort
 * @param compare Comparator function
 * @param user_data Optional user data passed to comparator (can be NULL)
 *
 * Example:
 *   int compare_ints(const void *a, const void *b, void *user_data) {
 *       return *(int*)a - *(int*)b;
 *   }
 *   rs_array_sort(&arr, compare_ints, NULL);
 *
 *   // For a sorted copy:
 *   rs_array_t sorted = rs_array_clone(arr);
 *   rs_array_sort(&sorted, compare_ints, NULL);
 */
RS_STD_API void rs_array_sort(rs_array_t *arr, rs_compare_fn compare, void *user_data);

// ============================================================================
// Convenience macros
// ============================================================================

/**
 * Create array for specific type.
 */
#define rs_array_create_type(T, alloc) rs_array_create(sizeof(T), alloc)

/**
 * Push value (takes value, not pointer).
 */
#define rs_array_push_val(arr, val) rs_array_push(arr, &(typeof(val)){val})

/**
 * Pop value (returns by value, zero if empty).
 */
#define rs_array_pop_val(arr, T)                                                                                       \
    ({                                                                                                                 \
        T _tmp = {0};                                                                                                  \
        rs_array_pop(arr, &_tmp);                                                                                      \
        _tmp;                                                                                                          \
    })

RS_EXTERN_C_END
