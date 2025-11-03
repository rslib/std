#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/internal/api.h>

RS_EXTERN_C_BEGIN

/**
 * Arena allocator - fast bump allocator.
 *
 * Properties:
 * - Fast O(1) allocation
 * - No individual free (only reset all)
 * - No fragmentation
 * - Linear memory layout (cache friendly)
 *
 * Use cases:
 * - Temporary allocations within a scope
 * - Parse tree construction
 * - Request/response handling
 */
typedef struct rs_arena_t rs_arena_t;

/**
 * Options for arena creation.
 */
typedef struct {
    rs_allocator_t *allocator; // Backing allocator (default: system allocator)
    rs_u32 reserved;           // Reserved for future use
} rs_arena_options_t;

/**
 * Create arena with options.
 *
 * @param initial_capacity Initial capacity for the arena
 * @param opts Arena options
 *   - allocator: rs_allocator_t* (default: system allocator)
 * @return New arena, or NULL on failure
 */
RS_STD_API rs_arena_t *rs_arena_create_with_options(rs_size_t initial_capacity, rs_arena_options_t opts);

/**
 * Create arena with optional parameters.
 *
 * @param initial_capacity Initial capacity for the arena
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @return New arena, or NULL on failure
 */
#define rs_arena_create(initial_capacity, ...)                                                                         \
    rs_arena_create_with_options(initial_capacity, (rs_arena_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Get allocator interface.
 */
RS_STD_API rs_allocator_t *rs_arena_allocator(rs_arena_t *arena);

/**
 * Reset arena (free all allocations).
 * Keeps backing memory for reuse.
 */
RS_STD_API void rs_arena_reset(rs_arena_t *arena);

/**
 * Destroy arena and free all memory.
 */
RS_STD_API void rs_arena_destroy(rs_arena_t *arena);

/**
 * Get current memory usage.
 */
RS_STD_API rs_size_t rs_arena_used(const rs_arena_t *arena);

/**
 * Get total capacity.
 */
RS_STD_API rs_size_t rs_arena_capacity(const rs_arena_t *arena);

// Convenience macros
#define rs_arena_alloc_type(arena, T) rs_alloc_type(rs_arena_allocator(arena), T)

#define rs_arena_alloc_array(arena, T, count) rs_alloc_array(rs_arena_allocator(arena), T, count)

RS_EXTERN_C_END
