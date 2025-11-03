#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>

RS_EXTERN_C_BEGIN

/**
 * Stack allocator - LIFO allocator with individual frees.
 *
 * Properties:
 * - Fast O(1) allocation (bump pointer)
 * - Can free individual allocations in LIFO order
 * - Fixed capacity (doesn't grow)
 * - Validates free order (must be most recent)
 * - Each allocation has a small header for tracking
 *
 * Use cases:
 * - Function-local temporary allocations
 * - LIFO data structures
 * - Nested scope allocations with strict order
 *
 * Example:
 *   rs_stack_t *stack = rs_stack_create(1024 * 1024);
 *   int *a = rs_alloc_type(rs_stack_allocator(stack), int);
 *   float *b = rs_alloc_type(rs_stack_allocator(stack), float);
 *   rs_stack_free(stack, b);  // OK - b is on top
 *   rs_stack_free(stack, a);  // OK - a is now on top
 */
typedef struct rs_stack_t rs_stack_t;

/**
 * Stack marker for batch freeing.
 */
typedef struct {
    rs_size_t offset;
} rs_stack_marker_t;

/**
 * Options for stack creation.
 */
typedef struct {
    rs_allocator_t *allocator; // Backing allocator (default: system allocator)
    rs_u32 reserved;           // Reserved for future use
} rs_stack_options_t;

/**
 * Create stack allocator with options.
 *
 * @param capacity Fixed capacity for the stack
 * @param opts Stack options
 *   - allocator: rs_allocator_t* (default: system allocator)
 * @return New stack, or NULL on failure
 */
RS_STD_API rs_stack_t *rs_stack_create_with_options(rs_size_t capacity, rs_stack_options_t opts);

/**
 * Create stack allocator with optional parameters.
 *
 * @param capacity Fixed capacity for the stack
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @return New stack, or NULL on failure
 *
 * Example:
 *   rs_stack_t *stack1 = rs_stack_create(1024 * 1024);
 *
 *   rs_arena_t *arena = rs_arena_create(10 * 1024 * 1024);
 *   rs_stack_t *stack2 = rs_stack_create(1024 * 1024, .allocator = rs_arena_allocator(arena));
 *   // Stack memory comes from arena
 */
#define rs_stack_create(capacity, ...)                                                                                 \
    rs_stack_create_with_options(capacity, (rs_stack_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Get allocator interface.
 */
RS_STD_API rs_allocator_t *rs_stack_allocator(rs_stack_t *stack);

/**
 * Free most recent allocation.
 * WARNING: Must be called in LIFO order!
 * Calling with wrong order will assert in debug builds.
 */
RS_STD_API void rs_stack_free(rs_stack_t *stack, void *ptr);

/**
 * Get current stack marker (save current position).
 */
RS_STD_API rs_stack_marker_t rs_stack_mark(rs_stack_t *stack);

/**
 * Free all allocations back to marker.
 */
RS_STD_API void rs_stack_free_to_marker(rs_stack_t *stack, rs_stack_marker_t marker);

/**
 * Reset stack (free all allocations).
 */
RS_STD_API void rs_stack_reset(rs_stack_t *stack);

/**
 * Destroy stack and free memory.
 */
RS_STD_API void rs_stack_destroy(rs_stack_t *stack);

/**
 * Get current stack usage.
 */
RS_STD_API rs_size_t rs_stack_used(const rs_stack_t *stack);

/**
 * Get total capacity.
 */
RS_STD_API rs_size_t rs_stack_capacity(const rs_stack_t *stack);

// Convenience macros
#define rs_stack_alloc_type(stack, T) rs_alloc_type(rs_stack_allocator(stack), T)

#define rs_stack_alloc_array(stack, T, count) rs_alloc_array(rs_stack_allocator(stack), T, count)

// Scoped allocation (automatically free when scope ends)
#define RS_STACK_SCOPE(stack)                                                                                          \
    for (rs_stack_marker_t _rs_marker = rs_stack_mark(stack), _rs_once = {.offset = 0}; _rs_once.offset == 0;          \
         rs_stack_free_to_marker(stack, _rs_marker), _rs_once.offset = 1)

RS_EXTERN_C_END
