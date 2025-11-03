#define RS_STD_LOG_MODULE "string_utils"

#include <ctype.h>
#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string_utils.h>
#include <string.h>

// ============================================================================
// Replace Functions
// ============================================================================

rs_result_t rs_string_replace(rs_string_t *str, rs_string_view_t old, rs_string_view_t new_str)
{
    RS_TRACE_BEGIN_FMT("str=%p, old=\"" RS_SV_FMT "\", new=\"" RS_SV_FMT "\"", (void *)str, RS_SV_ARG(old),
                       RS_SV_ARG(new_str));

    if (!str) {
        RS_TRACE_END();
        return RS_ERROR_RET(RS_ERR_INVALID, "String is NULL");
    }

    if (rs_sv_is_empty(old)) {
        RS_TRACE_END();
        return RS_ERROR_RET(RS_ERR_INVALID, "Old substring cannot be empty");
    }

    rs_size_t str_len = rs_string_len(str);
    rs_size_t old_len = rs_sv_len(old);

    if (old_len > str_len) {
        RS_TRACE_END();
        return RS_ERROR_RET(RS_ERR_NOTFOUND, "Substring not found");
    }

    // Find first occurrence
    const char *str_data = rs_string_cstr(str);
    const char *old_data = rs_sv_data(old);
    const char *found = NULL;
    rs_size_t found_pos = 0;

    for (rs_size_t i = 0; i <= str_len - old_len; i++) {
        if (memcmp(str_data + i, old_data, old_len) == 0) {
            found = str_data + i;
            found_pos = i;
            break;
        }
    }

    if (!found) {
        RS_TRACE_END();
        return RS_ERROR_RET(RS_ERR_NOTFOUND, "Substring not found");
    }

    rs_size_t new_len = rs_sv_len(new_str);
    rs_size_t final_len = str_len - old_len + new_len;

    // Check if we need to grow the string
    if (final_len > rs_string_cap(str)) {
        rs_result_t result = rs_string_reserve(str, final_len);
        if (result != RS_OK) {
            return result;
        }
    }

    // Get data pointer again after potential reallocation
    char *data = rs_string_data_mut(str);

    // If lengths differ, shift the tail
    if (old_len != new_len) {
        rs_size_t tail_start = found_pos + old_len;
        rs_size_t tail_len = str_len - tail_start;
        memmove(data + found_pos + new_len, data + tail_start, tail_len);
    }

    // Copy new string
    if (new_len > 0) {
        memcpy(data + found_pos, rs_sv_data(new_str), new_len);
    }

    // Update length and null terminate
    data[final_len] = '\0';
    if (rs_string_is_small(str)) {
        if (final_len <= RS_STRING_SSO_CAP) {
            str->u.small.len = (unsigned char)final_len;
        } else {
            // Should not happen if reserve worked correctly
            RS_TRACE_END();
            return RS_ERROR_RET(RS_ERR_OVERFLOW, "String too large for small string");
        }
    } else {
        str->u.large.len = final_len;
    }

    RS_TRACE_END();
    return RS_OK;
}

rs_size_t rs_string_replace_all(rs_string_t *str, rs_string_view_t old, rs_string_view_t new_str)
{
    RS_TRACE_BEGIN_FMT("str=%p, old=\"" RS_SV_FMT "\", new=\"" RS_SV_FMT "\"", (void *)str, RS_SV_ARG(old),
                       RS_SV_ARG(new_str));
    if (!str || rs_sv_is_empty(old)) {
        RS_TRACE_END();
        return 0;
    }

    rs_size_t count = 0;
    rs_size_t old_len = rs_sv_len(old);
    rs_size_t new_len = rs_sv_len(new_str);
    const char *old_data = rs_sv_data(old);
    const char *new_data = rs_sv_data(new_str);

    // First, count occurrences to calculate final size
    rs_size_t str_len = rs_string_len(str);
    const char *str_data = rs_string_cstr(str);

    for (rs_size_t i = 0; i <= str_len - old_len;) {
        if (memcmp(str_data + i, old_data, old_len) == 0) {
            count++;
            i += old_len;
        } else {
            i++;
        }
    }

    if (count == 0) {
        RS_TRACE_END();
        return 0;
    }

    // Calculate final length
    rs_size_t final_len = str_len - (count * old_len) + (count * new_len);

    // If replacement is larger, we might need more space
    if (final_len > rs_string_cap(str)) {
        rs_result_t result = rs_string_reserve(str, final_len);
        if (result != RS_OK) {
            RS_TRACE_END();
            return 0; // Allocation failed
        }
    }

    // Perform replacements
    // If new_len == old_len, we can replace in place
    // If new_len < old_len, we replace and shift left
    // If new_len > old_len, we need to work backwards to avoid overwriting

    if (new_len <= old_len) {
        // Replace forward
        char *data = rs_string_data_mut(str);
        rs_size_t write_pos = 0;
        rs_size_t read_pos = 0;

        while (read_pos < str_len) {
            if (read_pos <= str_len - old_len && memcmp(data + read_pos, old_data, old_len) == 0) {
                // Replace
                if (new_len > 0) {
                    memcpy(data + write_pos, new_data, new_len);
                }
                write_pos += new_len;
                read_pos += old_len;
            } else {
                // Copy
                if (write_pos != read_pos) {
                    data[write_pos] = data[read_pos];
                }
                write_pos++;
                read_pos++;
            }
        }

        data[final_len] = '\0';
    } else {
        // new_len > old_len: Work with a temporary buffer or backwards
        // For simplicity, allocate a new buffer
        rs_allocator_t *alloc = rs_string_get_allocator(str);
        char *new_buffer = rs_alloc(alloc, final_len + 1);
        if (!new_buffer) {
            RS_TRACE_END();
            return 0;
        }

        char *data = rs_string_data_mut(str);
        rs_size_t write_pos = 0;
        rs_size_t read_pos = 0;

        while (read_pos < str_len) {
            if (read_pos <= str_len - old_len && memcmp(data + read_pos, old_data, old_len) == 0) {
                // Replace
                memcpy(new_buffer + write_pos, new_data, new_len);
                write_pos += new_len;
                read_pos += old_len;
            } else {
                // Copy
                new_buffer[write_pos] = data[read_pos];
                write_pos++;
                read_pos++;
            }
        }

        new_buffer[final_len] = '\0';

        // Copy back
        memcpy(data, new_buffer, final_len + 1);
        rs_free(alloc, new_buffer, final_len + 1);
    }

    // Update length
    if (rs_string_is_small(str)) {
        if (final_len <= RS_STRING_SSO_CAP) {
            str->u.small.len = (unsigned char)final_len;
        }
    } else {
        str->u.large.len = final_len;
    }

    RS_TRACE_END();
    return count;
}

// ============================================================================
// Split Function
// ============================================================================

rs_result_t rs_string_split(const rs_string_t *str, rs_string_view_t delim, rs_bool skip_empty, rs_array_t *out_array)
{
    RS_TRACE_BEGIN_FMT("str=%p, delim=\"" RS_SV_FMT "\", skip_empty=%d, out_array=%p", (void *)str, RS_SV_ARG(delim),
                       skip_empty, (void *)out_array);
    if (!str || !out_array) {
        RS_TRACE_END();
        return RS_ERROR_RET(RS_ERR_INVALID, "String or array is NULL");
    }

    if (rs_sv_is_empty(delim)) {
        RS_TRACE_END();
        return RS_ERROR_RET(RS_ERR_INVALID, "Delimiter cannot be empty");
    }

    rs_size_t str_len = rs_string_len(str);
    const char *str_data = rs_string_cstr(str);
    rs_size_t delim_len = rs_sv_len(delim);
    const char *delim_data = rs_sv_data(delim);

    rs_size_t start = 0;

    while (start < str_len) {
        // Find next delimiter
        rs_size_t end = start;
        rs_bool found = false;

        while (end <= str_len - delim_len) {
            if (memcmp(str_data + end, delim_data, delim_len) == 0) {
                found = true;
                break;
            }
            end++;
        }

        if (!found) {
            end = str_len;
        }

        // Add part if not empty or if we're keeping empty parts
        rs_size_t pars_len = end - start;
        if (pars_len > 0 || !skip_empty) {
            rs_string_view_t part = rs_sv_from_buf(str_data + start, pars_len);
            rs_result_t result = rs_array_push(out_array, &part);
            if (result != RS_OK) {
                RS_TRACE_END();
                return result;
            }
        }

        if (found) {
            start = end + delim_len;
        } else {
            break;
        }
    }

    // Handle trailing delimiter (creates empty part at end if not skipping)
    if (str_len >= delim_len && memcmp(str_data + str_len - delim_len, delim_data, delim_len) == 0) {
        if (!skip_empty) {
            rs_string_view_t part = rs_sv_from_buf(str_data + str_len, 0);
            rs_result_t result = rs_array_push(out_array, &part);
            if (result != RS_OK) {
                RS_TRACE_END();
                return result;
            }
        }
    }

    RS_TRACE_END();
    return RS_OK;
}

// ============================================================================
// Join Function
// ============================================================================

rs_string_t rs_string_join_with_options(const rs_string_view_t *parts, rs_size_t count, rs_string_view_t delim,
                                        rs_string_join_options_t options)
{
    rs_allocator_t *allocator = options.allocator;
    RS_TRACE_BEGIN_FMT("parts=%p, count=%zu, delim=\"" RS_SV_FMT "\", allocator=%p", (void *)parts, count,
                       RS_SV_ARG(delim), (void *)allocator);
    if (!parts || count == 0) {
        RS_TRACE_END();
        return rs_string_create(.allocator = allocator);
    }

    // Calculate total length
    rs_size_t total_len = 0;
    rs_size_t delim_len = rs_sv_len(delim);

    for (rs_size_t i = 0; i < count; i++) {
        total_len += rs_sv_len(parts[i]);
    }

    if (count > 1) {
        total_len += delim_len * (count - 1);
    }

    // Create result string
    rs_string_t result = rs_string_create(.initial_capacity = total_len, .allocator = allocator);

    // Join parts
    for (rs_size_t i = 0; i < count; i++) {
        if (i > 0 && delim_len > 0) {
            rs_string_push_buf(&result, rs_sv_data(delim), delim_len);
        }
        rs_string_push_buf(&result, rs_sv_data(parts[i]), rs_sv_len(parts[i]));
    }

    RS_TRACE_END();

    return result;
}
