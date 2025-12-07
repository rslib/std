#include "allocator_internal.h"

#include <assert.h>
#include <rs/std/allocators/arena.h>
#include <rs/std/error.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Arena block
// ============================================================================

typedef struct arena_block_t {
    struct arena_block_t *next;
    rs_size_t capacity;
    rs_size_t used;
    rs_size_t data_offset; // Offset from start of block to data area
} arena_block_t;

// ============================================================================
// Arena structure
// ============================================================================

struct rs_arena_t {
    rs_allocator_t allocator;     // Embedded allocator interface
    rs_allocator_t *backing;      // Backing allocator for blocks
    arena_block_t *current_block; // Current allocation block
    arena_block_t *first_block;   // First block (for reset)
    rs_size_t default_block_size; // Size for new blocks
};

// ============================================================================
// Internal helpers
// ============================================================================

static arena_block_t *arena_create_block(rs_allocator_t *backing, rs_size_t capacity)
{
    // Ensure the data area starts at a properly aligned offset
    // Align header size to rs_max_align_t so data is maximally aligned
    rs_size_t header_size = sizeof(arena_block_t);
    rs_size_t data_offset = align_forward(header_size, RS_DEFAULT_ALIGNMENT);
    rs_size_t total_size = data_offset + capacity;

    arena_block_t *block = (arena_block_t *)rs_alloc(backing, total_size);

    if (!block) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate arena block of %zu bytes", total_size);
        return NULL;
    }

    block->next = NULL;
    block->capacity = capacity;
    block->used = 0;
    block->data_offset = data_offset;

    return block;
}

static void arena_free_blocks(rs_arena_t *arena, arena_block_t *start)
{
    arena_block_t *block = start;
    while (block) {
        arena_block_t *next = block->next;
        rs_free(arena->backing, block, block->data_offset + block->capacity);
        block = next;
    }
}

// ============================================================================
// Allocator vtable implementation
// ============================================================================

static void *arena_alloc(rs_allocator_t *allocator, rs_size_t size, rs_size_t align)
{
    rs_arena_t *arena = (rs_arena_t *)allocator->ctx;

    if (size == 0) {
        return NULL;
    }

    // Get pointer to data area
    char *data_start = (char *)arena->current_block + arena->current_block->data_offset;

    // Calculate the actual address at current used offset
    rs_uintptr current_addr = (rs_uintptr)(data_start + arena->current_block->used);

    // Align the actual address, not just the offset
    rs_uintptr aligned_addr = align_forward(current_addr, align);

    // Calculate the aligned offset within the block
    rs_size_t aligned_offset = (rs_size_t)(aligned_addr - (rs_uintptr)data_start);

    // Check if current block has space
    if (aligned_offset + size > arena->current_block->capacity) {
        // Need a new block
        rs_size_t new_block_size = arena->default_block_size;
        if (size > new_block_size) {
            new_block_size = size * 2; // Grow if needed
        }

        arena_block_t *new_block = arena_create_block(arena->backing, new_block_size);
        if (!new_block) {
            RS_ERROR(RS_ERR_NOMEM, "Arena exhausted: failed to allocate %zu bytes", size);
            return NULL;
        }

        // Link new block
        arena->current_block->next = new_block;
        arena->current_block = new_block;

        // Recalculate for the new block
        data_start = (char *)new_block + new_block->data_offset;
        current_addr = (rs_uintptr)data_start;
        aligned_addr = align_forward(current_addr, align);
        aligned_offset = (rs_size_t)(aligned_addr - (rs_uintptr)data_start);
    }

    // Allocate from current block
    void *ptr = data_start + aligned_offset;
    arena->current_block->used = aligned_offset + size;

    return ptr;
}

static void *arena_realloc(rs_allocator_t *allocator, void *ptr, rs_size_t old_size, rs_size_t new_size,
                           rs_size_t align)
{
    // Arena doesn't support efficient realloc, just alloc new and copy
    if (new_size == 0) {
        return NULL;
    }

    void *new_ptr = arena_alloc(allocator, new_size, align);
    if (!new_ptr) {
        return NULL;
    }

    if (ptr && old_size > 0) {
        rs_size_t copy_size = old_size < new_size ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
    }

    return new_ptr;
}

static void arena_free(rs_allocator_t *allocator, void *ptr, rs_size_t size)
{
    RS_UNUSED(allocator);
    RS_UNUSED(ptr);
    RS_UNUSED(size);
    // No-op: arena doesn't support individual frees
}

static void arena_reset(rs_allocator_t *allocator)
{
    rs_arena_t *arena = (rs_arena_t *)allocator->ctx;

    // Free all blocks except the first
    if (arena->first_block->next) {
        arena_free_blocks(arena, arena->first_block->next);
        arena->first_block->next = NULL;
    }

    // Reset first block
    arena->first_block->used = 0;
    arena->current_block = arena->first_block;
}

static void arena_destroy(rs_allocator_t *allocator)
{
    rs_arena_t *arena = (rs_arena_t *)allocator->ctx;

    // Free all blocks
    arena_free_blocks(arena, arena->first_block);

    // Free arena struct
    rs_free(arena->backing, arena, sizeof(rs_arena_t));
}

// Arena allocator vtable
static const rs_allocator_vtable_t arena_allocator_vtable = {
    .alloc = arena_alloc,
    .realloc = arena_realloc,
    .free = arena_free,
    .reset = arena_reset,
    .destroy = arena_destroy,
};

// ============================================================================
// Public API
// ============================================================================

rs_arena_t *rs_arena_create_with_options(rs_size_t initial_capacity, rs_arena_options_t opts)
{
    rs_allocator_t *backing = opts.allocator;
    if (!backing) {
        backing = rs_allocator_system();
    }

    // Allocate arena struct
    rs_arena_t *arena = (rs_arena_t *)rs_alloc(backing, sizeof(rs_arena_t));
    if (!arena) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate arena struct (%zu bytes)", sizeof(rs_arena_t));
        return NULL;
    }

    // Create first block
    arena_block_t *first_block = arena_create_block(backing, initial_capacity);
    if (!first_block) {
        rs_free(backing, arena, sizeof(rs_arena_t));
        // Error already set by arena_create_block
        return NULL;
    }

    // Initialize arena
    arena->allocator.vtable = &arena_allocator_vtable;
    arena->allocator.ctx = arena;
    arena->backing = backing;
    arena->current_block = first_block;
    arena->first_block = first_block;
    arena->default_block_size = initial_capacity;

    return arena;
}

rs_allocator_t *rs_arena_allocator(rs_arena_t *arena)
{
    return &arena->allocator;
}

void rs_arena_reset(rs_arena_t *arena)
{
    arena_reset(&arena->allocator);
}

void rs_arena_destroy(rs_arena_t *arena)
{
    arena_destroy(&arena->allocator);
}

rs_size_t rs_arena_used(const rs_arena_t *arena)
{
    rs_size_t total = 0;
    const arena_block_t *block = arena->first_block;

    while (block) {
        total += block->used;
        block = block->next;
    }

    return total;
}

rs_size_t rs_arena_capacity(const rs_arena_t *arena)
{
    rs_size_t total = 0;
    const arena_block_t *block = arena->first_block;

    while (block) {
        total += block->capacity;
        block = block->next;
    }

    return total;
}
