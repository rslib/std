#include <rs/std/containers/array.h>
#include <rs/std/error.h>
#include <rs/std/os/env.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
// Access to environ for iteration
extern char **environ;
#endif

// ============================================================================
// Basic Operations
// ============================================================================

rs_result_t rs_env_get(rs_string_t *out, const char *name)
{
    if (!out || !name) {
        return RS_ERROR_RET(RS_ERR_INVALID, "NULL parameter to rs_env_get");
    }

    rs_string_clear(out);

#ifdef _WIN32
    // Windows: GetEnvironmentVariable
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    if (size == 0) {
        return RS_ERROR_RET(RS_ERR_NOTFOUND, "Environment variable not found");
    }

    char *buf = (char *)malloc(size);
    if (!buf) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate buffer");
    }

    DWORD result = GetEnvironmentVariableA(name, buf, size);
    if (result == 0 || result >= size) {
        free(buf);
        return RS_ERROR_RET(RS_ERR_SYSTEM, "Failed to get environment variable");
    }

    rs_result_t ret = rs_string_push_cstr(out, buf);
    free(buf);
    return ret;
#else
    // Unix: getenv
    const char *value = getenv(name);
    if (!value) {
        return RS_ERROR_RET(RS_ERR_NOTFOUND, "Environment variable not found");
    }

    return rs_string_push_cstr(out, value);
#endif
}

int rs_env_exists(const char *name)
{
    if (!name) {
        return 0;
    }

#ifdef _WIN32
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    return size > 0;
#else
    return getenv(name) != NULL;
#endif
}

rs_result_t rs_env_set(const char *name, const char *value)
{
    if (!name || !value) {
        return RS_ERROR_RET(RS_ERR_INVALID, "NULL parameter to rs_env_set");
    }

#ifdef _WIN32
    if (!SetEnvironmentVariableA(name, value)) {
        return RS_ERROR_RET(RS_ERR_SYSTEM, "Failed to set environment variable");
    }
    return RS_OK;
#else
    if (setenv(name, value, 1) != 0) {
        return RS_ERROR_RET(RS_ERR_SYSTEM, "Failed to set environment variable");
    }
    return RS_OK;
#endif
}

rs_result_t rs_env_unset(const char *name)
{
    if (!name) {
        return RS_ERROR_RET(RS_ERR_INVALID, "NULL parameter to rs_env_unset");
    }

#ifdef _WIN32
    if (!SetEnvironmentVariableA(name, NULL)) {
        DWORD err = GetLastError();
        // ERROR_ENVVAR_NOT_FOUND is OK (variable didn't exist)
        if (err != ERROR_ENVVAR_NOT_FOUND) {
            return RS_ERROR_RET(RS_ERR_SYSTEM, "Failed to unset environment variable");
        }
    }
    return RS_OK;
#else
    if (unsetenv(name) != 0) {
        return RS_ERROR_RET(RS_ERR_SYSTEM, "Failed to unset environment variable");
    }
    return RS_OK;
#endif
}

rs_result_t rs_env_expand(rs_string_t *out, rs_string_view_t input)
{
    if (!out) {
        return RS_ERROR_RET(RS_ERR_INVALID, "NULL output parameter to rs_env_expand");
    }

    rs_string_clear(out);

    const char *p = rs_sv_data(input);
    const char *end = p + rs_sv_len(input);

    while (p < end) {
        if (*p == '$') {
            p++; // Skip $

            // Handle ${VAR} or $VAR
            int braced = 0;
            if (p < end && *p == '{') {
                braced = 1;
                p++;
            }

            const char *var_start = p;
            while (p < end &&
                   ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_')) {
                p++;
            }

            if (braced && p < end && *p == '}') {
                p++;
            }

            rs_size_t var_len = (braced && p > var_start ? p - var_start - 1 : p - var_start);
            if (var_len > 0) {
                char var_name[256];
                if (var_len >= sizeof(var_name)) {
                    return RS_ERROR_RET(RS_ERR_INVALID, "Environment variable name too long");
                }

                memcpy(var_name, var_start, var_len);
                var_name[var_len] = '\0';

#ifdef _WIN32
                // On Windows, use GetEnvironmentVariable to match rs_env_set
                char value_buf[4096];
                DWORD result = GetEnvironmentVariableA(var_name, value_buf, sizeof(value_buf));
                if (result > 0 && result < sizeof(value_buf)) {
                    RS_TRY(rs_string_push_cstr(out, value_buf));
                }
#else
                const char *var_value = getenv(var_name);
                if (var_value) {
                    RS_TRY(rs_string_push_cstr(out, var_value));
                }
#endif
            }
        } else {
            RS_TRY(rs_string_push_char(out, *p));
            p++;
        }
    }

    return RS_OK;
}

// ============================================================================
// Advanced Operations
// ============================================================================

rs_string_view_t rs_env_get_view(const char *name)
{
    if (!name) {
        return rs_sv_empty();
    }

#ifdef _WIN32
    // Windows: Not safe to return a view since we'd need to allocate.
    // Return empty view and let caller use rs_env_get() instead.
    return rs_sv_empty();
#else
    const char *value = getenv(name);
    if (!value) {
        return rs_sv_empty();
    }
    return rs_sv_from_cstr(value);
#endif
}

void rs_env_foreach(rs_env_foreach_fn callback, void *userdata)
{
    if (!callback) {
        return;
    }

#ifdef _WIN32
    // Windows: GetEnvironmentStrings returns a block of null-terminated strings
    // Format: "NAME=VALUE\0NAME2=VALUE2\0\0"
    // Note: The returned block is read-only, so we must copy names before use
    LPCH env = GetEnvironmentStringsA();
    if (!env) {
        return;
    }

    LPCH current = env;
    while (*current) {
        // Skip environment variables that start with '=' (Windows internal vars)
        if (*current == '=') {
            current += strlen(current) + 1;
            continue;
        }

        // Find the '=' separator
        const char *eq = strchr(current, '=');
        if (eq) {
            size_t name_len = eq - current;
            char name[256];
            if (name_len < sizeof(name)) {
                memcpy(name, current, name_len);
                name[name_len] = '\0';
                const char *value = eq + 1;
                callback(name, value, userdata);
            }
        }
        // Move to next string
        current += strlen(current) + 1;
    }

    FreeEnvironmentStringsA(env);
#else
    // Unix: iterate over environ
    if (!environ) {
        return;
    }

    for (char **env = environ; *env; env++) {
        // Parse "NAME=VALUE"
        char *eq = strchr(*env, '=');
        if (eq) {
            rs_size_t name_len = eq - *env;
            char *name = (char *)malloc(name_len + 1);
            if (!name) {
                continue;
            }

            memcpy(name, *env, name_len);
            name[name_len] = '\0';

            const char *value = eq + 1;
            callback(name, value, userdata);

            free(name);
        }
    }
#endif
}

rs_result_t rs_env_get_all(rs_array_t *out_pairs, rs_allocator_t *allocator)
{
    if (!out_pairs || !allocator) {
        return RS_ERROR_RET(RS_ERR_INVALID, "NULL parameter to rs_env_get_all");
    }

    rs_array_clear(out_pairs);

#ifdef _WIN32
    LPCH env = GetEnvironmentStringsA();
    if (!env) {
        return RS_ERROR_RET(RS_ERR_SYSTEM, "Failed to get environment strings");
    }

    LPCH current = env;
    while (*current) {
        // Skip environment variables that start with '=' (Windows internal vars)
        if (*current == '=') {
            current += strlen(current) + 1;
            continue;
        }

        const char *eq = strchr(current, '=');
        if (eq) {
            rs_env_pair_t pair;
            pair.name = rs_string_create(.allocator = allocator);
            pair.value = rs_string_create(.allocator = allocator);

            rs_size_t name_len = eq - current;
            RS_TRY(rs_string_push_buf(&pair.name, current, name_len));
            RS_TRY(rs_string_push_cstr(&pair.value, eq + 1));

            RS_TRY(rs_array_push(out_pairs, &pair));
        }
        current += strlen(current) + 1;
    }

    FreeEnvironmentStringsA(env);
#else
    if (!environ) {
        return RS_OK; // Empty environment
    }

    for (char **env = environ; *env; env++) {
        char *eq = strchr(*env, '=');
        if (eq) {
            rs_env_pair_t pair;
            pair.name = rs_string_create(.allocator = allocator);
            pair.value = rs_string_create(.allocator = allocator);

            rs_size_t name_len = eq - *env;
            RS_TRY(rs_string_push_buf(&pair.name, *env, name_len));
            RS_TRY(rs_string_push_cstr(&pair.value, eq + 1));

            RS_TRY(rs_array_push(out_pairs, &pair));
        }
    }
#endif

    return RS_OK;
}
