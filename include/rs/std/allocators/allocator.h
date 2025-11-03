#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/types.h>
#include <stddef.h>

RS_EXTERN_C_BEGIN

// Forward declarations
typedef struct rs_allocator_t rs_allocator_t;

// Allocator vtable (interface)
typedef struct {
    /**
     * Allocate memory.
     * Returns NULL on failure.
     */
    void *(*alloc)(rs_allocator_t *allocator, rs_size_t size, rs_size_t align);

    /**
     * Reallocate memory.
     * Returns NULL on failure (old pointer remains valid).
     */
    void *(*realloc)(rs_allocator_t *allocator, void *ptr, rs_size_t old_size, rs_size_t new_size, rs_size_t align);

    /**
     * Free memory.
     * Some allocators (arena) may be no-op.
     */
    void (*free)(rs_allocator_t *allocator, void *ptr, rs_size_t size);

    /**
     * Free all memory (reset allocator).
     */
    void (*reset)(rs_allocator_t *allocator);

    /**
     * Destroy allocator and free all resources.
     */
    void (*destroy)(rs_allocator_t *allocator);
} rs_allocator_vtable_t;

// Allocator base struct
struct rs_allocator_t {
    const rs_allocator_vtable_t *vtable;
    void *ctx; // Allocator-specific context
};

// Helper macros
#define rs_alloc(a, size) (a)->vtable->alloc((a), (size), _Alignof(max_align_t))

#define rs_alloc_aligned(a, size, align) (a)->vtable->alloc((a), (size), (align))

#define rs_realloc(a, ptr, old_size, new_size)                                                                         \
    (a)->vtable->realloc((a), (ptr), (old_size), (new_size), _Alignof(max_align_t))

#define rs_free(a, ptr, size) (a)->vtable->free((a), (ptr), (size))

#define rs_allocator_reset(a) (a)->vtable->reset((a))

#define rs_allocator_destroy(a) (a)->vtable->destroy((a))

// Allocation helpers
#define rs_alloc_type(a, T) ((T *)rs_alloc_aligned((a), sizeof(T), _Alignof(T)))

#define rs_alloc_array(a, T, count) ((T *)rs_alloc_aligned((a), sizeof(T) * (count), _Alignof(T)))

// Default system allocator (uses malloc/free)
RS_STD_API rs_allocator_t *rs_allocator_system(void);

RS_EXTERN_C_END
