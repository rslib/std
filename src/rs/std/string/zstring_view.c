#include <rs/std/string/zstring_view.h>
#include <string.h>

// ============================================================================
// Creation
// ============================================================================

rs_zstring_view_t rs_zsv_from_cstr(const char *cstr)
{
    if (!cstr) {
        return (rs_zstring_view_t){NULL, 0};
    }
    return (rs_zstring_view_t){cstr, strlen(cstr)};
}

rs_zstring_view_t rs_zsv_from_string(const rs_string_t *str)
{
    if (!str) {
        return (rs_zstring_view_t){NULL, 0};
    }
    // rs_string_t is always null-terminated
    return (rs_zstring_view_t){rs_string_cstr(str), rs_string_len(str)};
}

rs_zstring_view_t rs_zsv_empty(void)
{
    static const char empty_str[] = "";
    return (rs_zstring_view_t){empty_str, 0};
}
