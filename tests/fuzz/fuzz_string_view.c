#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) {
        return 0;
    }

    // Create string view from input
    rs_string_view_t sv = rs_sv_from_buf((const char *)data, size);

    // Properties
    (void)rs_sv_data(sv);
    (void)rs_sv_len(sv);
    (void)rs_sv_is_empty(sv);

    // Slicing operations
    if (size > 2) {
        size_t mid = size / 2;
        (void)rs_sv_slice(sv, 0, mid);
        (void)rs_sv_slice(sv, mid, size);
        (void)rs_sv_slice_from(sv, mid);
        (void)rs_sv_slice_to(sv, mid);
    }

    // Trimming
    (void)rs_sv_trim(sv);
    (void)rs_sv_trim_left(sv);
    (void)rs_sv_trim_right(sv);

    // Use part of input as pattern for searching
    if (size > 4) {
        rs_string_view_t pattern = rs_sv_slice_to(sv, 4);
        (void)rs_sv_find(sv, pattern);
        (void)rs_sv_starts_with(sv, pattern);
        (void)rs_sv_ends_with(sv, pattern);
        (void)rs_sv_contains(sv, pattern);
        (void)rs_sv_trim_prefix(sv, pattern);
        (void)rs_sv_trim_suffix(sv, pattern);
    }

    // Character search
    if (size > 0) {
        (void)rs_sv_find_char(sv, (char)data[0]);
    }

    // Comparison with itself
    (void)rs_sv_cmp(sv, sv);
    (void)rs_sv_eq(sv, sv);

    // Convert to string (exercises allocation)
    rs_string_t str = rs_sv_to_string(sv);
    rs_string_destroy(&str);

    return 0;
}
