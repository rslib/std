#include <rs/std/containers/array.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_utils.h>
#include <rs/std/string/string_view.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 4) {
        return 0;
    }

    // Split input into main string and pattern
    size_t split_point = size / 2;
    rs_string_t str = rs_string_from_buf((const char *)data, split_point);
    rs_string_view_t pattern = rs_sv_from_buf((const char *)data + split_point, size - split_point);

    // Test replace (first occurrence)
    rs_string_t str_copy = rs_string_clone(&str);
    rs_string_replace(&str_copy, pattern, rs_sv_from_cstr("REPLACED"));
    rs_string_destroy(&str_copy);

    // Test replace_all
    str_copy = rs_string_clone(&str);
    rs_string_replace_all(&str_copy, pattern, rs_sv_from_cstr("X"));
    rs_string_destroy(&str_copy);

    // Test split
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t));

    // Use first byte as delimiter
    if (size > 0) {
        char delim_char = (char)data[0];
        char delim_buf[2] = {delim_char, '\0'};
        rs_string_view_t delim = rs_sv_from_buf(delim_buf, 1);

        rs_string_split(&str, delim, 0, &parts);

        // Join back together
        if (rs_array_len(&parts) > 0) {
            rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
            rs_string_t joined = rs_string_join(views, rs_array_len(&parts), delim);
            rs_string_destroy(&joined);
        }
    }

    rs_array_destroy(&parts);

    // Test split with skip_empty = true
    parts = rs_array_create(sizeof(rs_string_view_t));
    rs_string_split(&str, rs_sv_from_cstr(" "), 1, &parts);
    rs_array_destroy(&parts);

    rs_string_destroy(&str);
    return 0;
}
