#include "allocator_internal.h"

#include <rs/std/allocators/allocator.h>
#include <rs/std/error.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// System allocator implementation
// ============================================================================

static void *system_alloc(rs_allocator_t *allocator, rs_size_t size, rs_size_t align)
{
    RS_UNUSED(allocator);
    RS_UNUSED(align); // malloc already aligns to max_align_t

    if (size == 0) {
        return NULL;
    }

    void *ptr = malloc(size);
    if (!ptr) {
        RS_ERROR(RS_ERR_NOMEM, "System allocator failed to allocate %zu bytes", size);
        return NULL;
    }

    return ptr;
}

static void *system_realloc(rs_allocator_t *allocator, void *ptr, rs_size_t old_size, rs_size_t new_size,
                            rs_size_t align)
{
    RS_UNUSED(allocator);
    RS_UNUSED(old_size);
    RS_UNUSED(align);

    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        RS_ERROR(RS_ERR_NOMEM, "System allocator failed to reallocate to %zu bytes (old: %zu)", new_size, old_size);
        return NULL;
    }

    return new_ptr;
}

static void system_free(rs_allocator_t *allocator, void *ptr, rs_size_t size)
{
    RS_UNUSED(allocator);
    RS_UNUSED(size);

    free(ptr);
}

static void system_reset(rs_allocator_t *allocator)
{
    RS_UNUSED(allocator);
    // No-op for system allocator
}

static void system_destroy(rs_allocator_t *allocator)
{
    RS_UNUSED(allocator);
    // No-op for system allocator (it's a singleton)
}

// System allocator vtable
static const rs_allocator_vtable_t system_allocator_vtable = {
    .alloc = system_alloc,
    .realloc = system_realloc,
    .free = system_free,
    .reset = system_reset,
    .destroy = system_destroy,
};

// System allocator singleton
static rs_allocator_t system_allocator_instance = {
    .vtable = &system_allocator_vtable,
    .ctx = NULL,
};

rs_allocator_t *rs_allocator_system(void)
{
    return &system_allocator_instance;
}
