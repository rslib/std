#include "allocator_internal.h"

#include <assert.h>
#include <rs/std/allocators/stack.h>
#include <rs/std/error.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Allocation header (stored before each allocation)
// ============================================================================

typedef struct {
    rs_size_t prev_offset; // Offset before this allocation (for unwinding)
} rs_stack_alloc_header_t;

// ============================================================================
// Stack structure
// ============================================================================

struct rs_stack_t {
    rs_allocator_t allocator; // Embedded allocator interface
    rs_allocator_t *backing;  // Backing allocator for buffer
    char *buf;                // Buffer
    rs_size_t buf_len;        // Buffer capacity
    rs_size_t offset;         // Current offset (top of stack)
};

// ============================================================================
// Internal helpers
// ============================================================================

static inline rs_bool ptr_in_stack(const rs_stack_t *stack, const void *ptr)
{
    rs_uintptr start = (rs_uintptr)stack->buf;
    rs_uintptr end = start + (rs_uintptr)stack->buf_len;
    rs_uintptr addr = (rs_uintptr)ptr;
    return start <= addr && addr < end;
}

// ============================================================================
// Allocator vtable implementation
// ============================================================================

static void *stack_alloc(rs_allocator_t *allocator, rs_size_t size, rs_size_t align)
{
    rs_stack_t *stack = (rs_stack_t *)allocator->ctx;

    if (size == 0) {
        return NULL;
    }

    // Save offset before this allocation for unwinding
    rs_size_t prev_offset = stack->offset;

    // Calculate alignment: we need header immediately before aligned data
    // so that free can find it at (data_ptr - sizeof(header))
    rs_uintptr curr_addr = (rs_uintptr)stack->buf + (rs_uintptr)stack->offset;
    rs_size_t header_size = sizeof(rs_stack_alloc_header_t);

    // Find where aligned data would start (accounting for header space)
    // We need (curr_addr + padding + header_size) to be aligned to 'align'
    rs_uintptr data_addr = align_forward(curr_addr + header_size, align);
    rs_uintptr header_addr = data_addr - header_size;

    // Calculate total size from current offset
    rs_size_t total_size = (rs_size_t)(data_addr - curr_addr) + size;

    // Check capacity
    if (stack->offset + total_size > stack->buf_len) {
        RS_ERROR(RS_ERR_NOMEM, "Stack allocator out of memory: need %zu bytes, only %zu available", total_size,
                 stack->buf_len - stack->offset);
        return NULL;
    }

    // Write header with previous offset for unwinding (immediately before data)
    rs_stack_alloc_header_t *header = (rs_stack_alloc_header_t *)header_addr;
    header->prev_offset = prev_offset;

    // Get data pointer
    void *ptr = (void *)data_addr;

    // Update offset
    stack->offset += total_size;

    return ptr;
}

static void *stack_realloc(rs_allocator_t *allocator, void *ptr, rs_size_t old_size, rs_size_t new_size,
                           rs_size_t align)
{
    // Stack doesn't support efficient realloc
    // Just alloc new and copy
    if (new_size == 0) {
        return NULL;
    }

    void *new_ptr = stack_alloc(allocator, new_size, align);
    if (!new_ptr) {
        return NULL;
    }

    if (ptr && old_size > 0) {
        rs_size_t copy_size = old_size < new_size ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
    }

    return new_ptr;
}

static void stack_free_impl(rs_allocator_t *allocator, void *ptr, rs_size_t size)
{
    RS_UNUSED(size);

    if (!ptr) {
        return;
    }

    rs_stack_t *stack = (rs_stack_t *)allocator->ctx;

    // Validate pointer is in stack
    if (!ptr_in_stack(stack, ptr)) {
        assert(0 && "Pointer not in stack allocator bounds");
        return;
    }

    rs_uintptr start = (rs_uintptr)stack->buf;
    rs_uintptr curr_addr = (rs_uintptr)ptr;

    // Check if already freed (double-free protection)
    if (curr_addr >= start + (rs_uintptr)stack->offset) {
        // Already freed, allow silently
        return;
    }

    // Get header (it's immediately before the data)
    rs_stack_alloc_header_t *header = (rs_stack_alloc_header_t *)(curr_addr - sizeof(rs_stack_alloc_header_t));

    // Read the previous offset stored in the header
    rs_size_t prev_offset = header->prev_offset;

    // Validate this is the top allocation (LIFO)
    if (prev_offset > stack->offset) {
        assert(0 && "Stack allocator free must be in LIFO order");
        return;
    }

    // Rewind stack to previous offset
    stack->offset = prev_offset;
}

static void stack_reset_impl(rs_allocator_t *allocator)
{
    rs_stack_t *stack = (rs_stack_t *)allocator->ctx;
    stack->offset = 0;
}

static void stack_destroy_impl(rs_allocator_t *allocator)
{
    rs_stack_t *stack = (rs_stack_t *)allocator->ctx;

    // Free buffer
    rs_free(stack->backing, stack->buf, stack->buf_len);

    // Free stack struct
    rs_free(stack->backing, stack, sizeof(rs_stack_t));
}

// Stack allocator vtable
static const rs_allocator_vtable_t stack_allocator_vtable = {
    .alloc = stack_alloc,
    .realloc = stack_realloc,
    .free = stack_free_impl,
    .reset = stack_reset_impl,
    .destroy = stack_destroy_impl,
};

// ============================================================================
// Public API
// ============================================================================

rs_stack_t *rs_stack_create_with_options(rs_size_t capacity, rs_stack_options_t opts)
{
    rs_allocator_t *backing = opts.allocator;
    if (!backing) {
        backing = rs_allocator_system();
    }

    // Allocate stack struct
    rs_stack_t *stack = (rs_stack_t *)rs_alloc(backing, sizeof(rs_stack_t));
    if (!stack) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate stack struct (%zu bytes)", sizeof(rs_stack_t));
        return NULL;
    }

    // Allocate buffer
    char *buf = (char *)rs_alloc(backing, capacity);
    if (!buf) {
        rs_free(backing, stack, sizeof(rs_stack_t));
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate stack buffer (%zu bytes)", capacity);
        return NULL;
    }

    // Initialize stack
    stack->allocator.vtable = &stack_allocator_vtable;
    stack->allocator.ctx = stack;
    stack->backing = backing;
    stack->buf = buf;
    stack->buf_len = capacity;
    stack->offset = 0;

    return stack;
}

rs_allocator_t *rs_stack_allocator(rs_stack_t *stack)
{
    return &stack->allocator;
}

void rs_stack_free(rs_stack_t *stack, void *ptr)
{
    stack_free_impl(&stack->allocator, ptr, 0);
}

rs_stack_marker_t rs_stack_mark(rs_stack_t *stack)
{
    rs_stack_marker_t marker;
    marker.offset = stack->offset;
    return marker;
}

void rs_stack_free_to_marker(rs_stack_t *stack, rs_stack_marker_t marker)
{
    if (marker.offset <= stack->buf_len) {
        stack->offset = marker.offset;
    }
}

void rs_stack_reset(rs_stack_t *stack)
{
    stack_reset_impl(&stack->allocator);
}

void rs_stack_destroy(rs_stack_t *stack)
{
    stack_destroy_impl(&stack->allocator);
}

rs_size_t rs_stack_used(const rs_stack_t *stack)
{
    return stack->offset;
}

rs_size_t rs_stack_capacity(const rs_stack_t *stack)
{
    return stack->buf_len;
}
