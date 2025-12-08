#include <rs/std/fs/path.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) {
        return 0;
    }

    rs_string_view_t path_sv = rs_sv_from_buf((const char *)data, size);

    // Query operations (no allocation)
    (void)rs_path_is_absolute(path_sv);
    (void)rs_path_extension(path_sv);

    // Dirname and basename
    rs_string_t dirname_out = rs_string_create();
    rs_path_dirname(&dirname_out, path_sv);
    rs_string_destroy(&dirname_out);

    rs_string_t basename_out = rs_string_create();
    rs_path_basename(&basename_out, path_sv);
    rs_string_destroy(&basename_out);

    // Normalize (modifies in-place)
    rs_string_t path = rs_string_from_buf((const char *)data, size);
    rs_path_normalize(&path);

    // Append (use second half of input as component)
    if (size > 2) {
        rs_string_view_t component = rs_sv_from_buf((const char *)data + size / 2, size - size / 2);
        rs_path_append(&path, component);
    }

    rs_string_destroy(&path);

    // Relative path computation (use input as both from and to)
    if (size > 4) {
        rs_string_view_t from = rs_sv_slice_to(path_sv, size / 2);
        rs_string_view_t to = rs_sv_slice_from(path_sv, size / 2);

        rs_string_t relative_out = rs_string_create();
        rs_path_relative(&relative_out, from, to);
        rs_string_destroy(&relative_out);
    }

    return 0;
}
