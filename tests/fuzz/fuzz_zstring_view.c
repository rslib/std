#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/string/zstring_view.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Need null-terminated string for zstring_view
    // Create a copy with null terminator
    char *buf = malloc(size + 1);
    if (!buf) {
        return 0;
    }
    memcpy(buf, data, size);
    buf[size] = '\0';

    // Create zstring_view from null-terminated buffer
    rs_zstring_view_t zsv = rs_zsv_from_cstr(buf);

    // Properties
    (void)rs_zsv_data(zsv);
    (void)rs_zsv_len(zsv);
    (void)rs_zsv_is_empty(zsv);
    (void)rs_zsv_cstr(zsv);

    // Convert to string_view
    rs_string_view_t sv = rs_zsv_to_sv(zsv);
    (void)rs_sv_len(sv);

    // Comparison
    (void)rs_zsv_cmp(zsv, zsv);
    (void)rs_zsv_eq(zsv, zsv);

    // Search operations
    if (size > 0) {
        (void)rs_zsv_find_char(zsv, (char)data[0]);
    }

    if (size > 4) {
        rs_string_view_t pattern = rs_sv_slice_to(sv, 4);
        (void)rs_zsv_starts_with(zsv, pattern);
        (void)rs_zsv_ends_with(zsv, pattern);
        (void)rs_zsv_contains(zsv, pattern);
    }

    // Test empty zstring_view
    rs_zstring_view_t empty = rs_zsv_empty();
    (void)rs_zsv_is_empty(empty);
    (void)rs_zsv_cstr(empty);

    free(buf);
    return 0;
}
