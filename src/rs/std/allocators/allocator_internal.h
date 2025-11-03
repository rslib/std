#pragma once

#include <rs/std/types.h>

// Internal utilities shared between allocators

/**
 * Align a pointer/offset forward to the specified alignment.
 * Alignment must be a power of 2.
 */
static inline rs_size_t align_forward(rs_size_t ptr, rs_size_t align)
{
    rs_size_t p = ptr;
    rs_size_t a = align;
    rs_size_t modulo = p & (a - 1);

    if (modulo != 0) {
        p += a - modulo;
    }

    return p;
}
