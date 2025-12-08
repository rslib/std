#include <rs/std/allocators/allocator.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) {
        return 0;
    }

    // Create string from arbitrary input
    rs_string_t str = rs_string_from_buf((const char *)data, size);

    // Exercise property accessors
    (void)rs_string_len(&str);
    (void)rs_string_cap(&str);
    (void)rs_string_is_empty(&str);
    (void)rs_string_cstr(&str);

    // Clone and manipulate
    rs_string_t copy = rs_string_clone(&str);
    rs_string_trim(&copy);
    rs_string_to_lower(&copy);
    rs_string_to_upper(&copy);
    rs_string_reverse(&copy);

    // Append operations
    rs_string_push_char(&str, 'X');
    rs_string_push_cstr(&str, "test");

    // Search operations
    if (size > 1) {
        rs_string_find_char(&str, (char)data[0]);
        rs_string_starts_with(&str, "test");
        rs_string_ends_with(&str, "test");
    }

    rs_string_destroy(&str);
    rs_string_destroy(&copy);

    return 0;
}
