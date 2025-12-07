#include <rs/std/error.h>
#include <rs/std/fs/path.h>
#include <rs/std/os/env.h>
#include <rs/std/string/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <direct.h>
#include <io.h>
#include <shlobj.h>
#include <windows.h>
#define F_OK 0
#define access _access
#else
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// ============================================================================
// Platform detection
// ============================================================================

#ifdef _WIN32
#define RS_PATH_SEP '\\'
#define RS_PATH_LIST_SEP ';'
#define RS_IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
#else
#define RS_PATH_SEP '/'
#define RS_PATH_LIST_SEP ':'
#define RS_IS_PATH_SEP(c) ((c) == '/')
#endif

// ============================================================================
// Path Creators (write to existing string)
// ============================================================================

rs_result_t rs_path_get_home(rs_string_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    rs_string_clear(out);

#ifdef _WIN32
    // Try USERPROFILE first
    if (rs_env_get(out, "USERPROFILE") == RS_OK && rs_string_len(out) > 0) {
        return RS_OK;
    }

    // Try HOMEDRIVE + HOMEPATH
    rs_string_t homepath = rs_string_create(.allocator = rs_string_get_allocator(out));
    if (rs_env_get(out, "HOMEDRIVE") == RS_OK && rs_string_len(out) > 0 && rs_env_get(&homepath, "HOMEPATH") == RS_OK &&
        rs_string_len(&homepath) > 0) {
        rs_result_t ret = rs_string_push_string(out, &homepath);
        rs_string_destroy(&homepath);
        return ret;
    }
    rs_string_destroy(&homepath);

    // Fall back to SHGetFolderPath
    rs_string_clear(out);
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return rs_string_push_cstr(out, path);
    }

    return RS_ERROR_RET(RS_ERR_NOTFOUND, "Could not determine home directory");
#else
    rs_string_view_t home = rs_env_get_view("HOME");
    if (!rs_sv_is_empty(home)) {
        return rs_string_push_buf(out, rs_sv_data(home), rs_sv_len(home));
    }

    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return rs_string_push_cstr(out, pw->pw_dir);
    }

    return RS_ERROR_RET(RS_ERR_NOTFOUND, "Could not determine home directory");
#endif
}

rs_result_t rs_path_get_temp(rs_string_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    rs_string_clear(out);

#ifdef _WIN32
    // Windows: Try TEMP, TMP, then GetTempPath
    if (rs_env_get(out, "TEMP") == RS_OK && rs_string_len(out) > 0) {
        return RS_OK;
    }

    if (rs_env_get(out, "TMP") == RS_OK && rs_string_len(out) > 0) {
        return RS_OK;
    }

    // Fall back to GetTempPath
    rs_string_clear(out);
    char temp_buf[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, temp_buf);
    if (len > 0 && len < MAX_PATH) {
        // Remove trailing backslash if present
        if (len > 0 && temp_buf[len - 1] == '\\') {
            temp_buf[len - 1] = '\0';
        }
        return rs_string_push_cstr(out, temp_buf);
    }

    return RS_ERROR_RET(RS_ERR_NOTFOUND, "Could not determine temp directory");
#else
    // Unix/Linux/macOS: Try TMPDIR, then /tmp, then /var/tmp
    rs_string_view_t tmpdir = rs_env_get_view("TMPDIR");
    if (!rs_sv_is_empty(tmpdir)) {
        return rs_string_push_buf(out, rs_sv_data(tmpdir), rs_sv_len(tmpdir));
    }

    // Try /tmp
    if (access("/tmp", F_OK) == 0) {
        return rs_string_push_cstr(out, "/tmp");
    }

    // Try /var/tmp
    if (access("/var/tmp", F_OK) == 0) {
        return rs_string_push_cstr(out, "/var/tmp");
    }

    return RS_ERROR_RET(RS_ERR_NOTFOUND, "Could not determine temp directory");
#endif
}

static rs_result_t expand_tilde_to_string(rs_string_t *out, rs_string_view_t path)
{
    const char *path_str = rs_sv_data(path);
    rs_size_t path_len = rs_sv_len(path);

    if (path_len == 0 || path_str[0] != '~') {
        return rs_string_push_buf(out, path_str, path_len);
    }

    if (path_len == 1 || RS_IS_PATH_SEP(path_str[1])) {
        RS_TRY(rs_path_get_home(out));

        if (path_len > 1) {
            return rs_string_push_buf(out, path_str + 2, path_len - 2);
        }
        return RS_OK;
    }

    const char *sep = path_str + 1;
    rs_size_t username_len = 0;
    while (username_len < path_len - 1 && !RS_IS_PATH_SEP(sep[username_len])) {
        username_len++;
    }

    char username[256];
    if (username_len >= sizeof(username)) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Username too long");
    }

    memcpy(username, path_str + 1, username_len);
    username[username_len] = '\0';

#ifdef _WIN32
    char user_path[MAX_PATH];
    snprintf(user_path, sizeof(user_path), "C:\\Users\\%.240s", username);
    DWORD attrs = GetFileAttributesA(user_path);
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return RS_ERROR_RET(RS_ERR_NOTFOUND, "User '%s' not found", username);
    }
    RS_TRY(rs_string_push_cstr(out, user_path));
#else
    struct passwd *pw = getpwnam(username);
    if (!pw || !pw->pw_dir) {
        return RS_ERROR_RET(RS_ERR_NOTFOUND, "User '%s' not found", username);
    }
    RS_TRY(rs_string_push_cstr(out, pw->pw_dir));
#endif

    if (username_len + 1 < path_len) {
        return rs_string_push_buf(out, path_str + username_len + 2, path_len - username_len - 2);
    }

    return RS_OK;
}

rs_result_t rs_path_expand(rs_string_t *out, rs_string_view_t path)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    // Clear output
    rs_string_clear(out);

    // Create temporary string for tilde expansion
    rs_string_t tilde_expanded = rs_string_create(.allocator = rs_string_get_allocator(out));
    rs_result_t result = expand_tilde_to_string(&tilde_expanded, path);
    if (result != RS_OK) {
        rs_string_destroy(&tilde_expanded);
        return result;
    }

    // Then expand environment variables
    result = rs_env_expand(out, rs_sv_from_string(&tilde_expanded));
    rs_string_destroy(&tilde_expanded);

    return result;
}

rs_result_t rs_path_dirname(rs_string_t *out, rs_string_view_t path)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    const char *path_str = rs_sv_data(path);
    rs_size_t path_len = rs_sv_len(path);

    rs_string_clear(out);

    if (path_len == 0) {
        return rs_string_push_cstr(out, ".");
    }

#ifdef _WIN32
    // Handle Windows drive letter prefix
    if (path_len >= 2 && ((path_str[0] >= 'A' && path_str[0] <= 'Z') || (path_str[0] >= 'a' && path_str[0] <= 'z')) &&
        path_str[1] == ':') {
        // If path is just "C:" or "C:\", return as-is
        if (path_len == 2 || (path_len == 3 && RS_IS_PATH_SEP(path_str[2]))) {
            return rs_string_push_buf(out, path_str, path_len);
        }
    }
#endif

    // Find last separator
    const char *last_sep = NULL;
    for (rs_size_t i = 0; i < path_len; i++) {
        if (RS_IS_PATH_SEP(path_str[i])) {
            last_sep = path_str + i;
        }
    }

    // No separator found
    if (!last_sep) {
#ifdef _WIN32
        // Return drive letter if present
        if (path_len >= 2 && path_str[1] == ':') {
            return rs_string_push_buf(out, path_str, 2);
        }
#endif
        return rs_string_push_cstr(out, ".");
    }

#ifdef _WIN32
    // Check if separator is right after drive letter (e.g., "C:\foo" -> "C:\")
    if (path_len >= 3 && path_str[1] == ':' && last_sep == path_str + 2) {
        return rs_string_push_buf(out, path_str, 3);
    }
#endif

    // Root directory (Unix)
    if (last_sep == path_str) {
#ifdef _WIN32
        return rs_string_push_cstr(out, "\\");
#else
        return rs_string_push_cstr(out, "/");
#endif
    }

    // Copy up to (but not including) separator
    return rs_string_push_buf(out, path_str, last_sep - path_str);
}

rs_result_t rs_path_basename(rs_string_t *out, rs_string_view_t path)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    const char *path_str = rs_sv_data(path);
    rs_size_t path_len = rs_sv_len(path);

    rs_string_clear(out);

    if (path_len == 0) {
        return rs_string_push_cstr(out, ".");
    }

    // Skip trailing separators
    while (path_len > 1 && RS_IS_PATH_SEP(path_str[path_len - 1])) {
        path_len--;
    }

    // Find last separator before trailing ones
    const char *last_sep = NULL;
    for (rs_size_t i = 0; i < path_len; i++) {
        if (RS_IS_PATH_SEP(path_str[i])) {
            last_sep = path_str + i;
        }
    }

    // No separator or root
    if (!last_sep) {
        return rs_string_push_buf(out, path_str, path_len);
    } else if (last_sep == path_str && path_len == 1) {
        // Root directory
        return rs_string_push_cstr(out, "/");
    }

    // Return basename
    const char *basename_start = last_sep + 1;
    rs_size_t basename_len = path_len - (last_sep - path_str) - 1;
    return rs_string_push_buf(out, basename_start, basename_len);
}

// ============================================================================
// Path Modifiers (work on existing string)
// ============================================================================

rs_result_t rs_path_append(rs_string_t *path, rs_string_view_t component)
{
    RS_CHECK(path != NULL, RS_ERR_INVALID, "Path is NULL");

    const char *comp_str = rs_sv_data(component);
    rs_size_t comp_len = rs_sv_len(component);

    // If component is absolute, replace entire path
    if (comp_len > 0 && RS_IS_PATH_SEP(comp_str[0])) {
        rs_string_clear(path);
        return rs_string_push_buf(path, comp_str, comp_len);
    }

    // Skip leading separators in component
    while (comp_len > 0 && RS_IS_PATH_SEP(*comp_str)) {
        comp_str++;
        comp_len--;
    }

    // Add separator if needed
    rs_size_t path_len = rs_string_len(path);
    if (path_len > 0 && !RS_IS_PATH_SEP(rs_string_cstr(path)[path_len - 1])) {
        RS_TRY(rs_string_push_char(path, RS_PATH_SEP));
    }

    // Append component
    return rs_string_push_buf(path, comp_str, comp_len);
}

rs_result_t rs_path_normalize(rs_string_t *path)
{
    RS_CHECK(path != NULL, RS_ERR_INVALID, "Path is NULL");

    const char *path_str = rs_string_cstr(path);
    int is_absolute = rs_path_is_absolute(rs_sv_from_string(path));

    // Create temporary result string
    rs_string_t result = rs_string_create(.allocator = rs_string_get_allocator(path));

    // Split by separator and process each component
    const char *p = path_str;

#ifdef _WIN32
    // Handle Windows drive letter prefix (e.g., "C:")
    if (is_absolute && ((path_str[0] >= 'A' && path_str[0] <= 'Z') || (path_str[0] >= 'a' && path_str[0] <= 'z')) &&
        path_str[1] == ':') {
        rs_string_push_char(&result, path_str[0]);
        rs_string_push_char(&result, ':');
        p = path_str + 2;
        // Add separator after drive letter
        if (RS_IS_PATH_SEP(*p)) {
            rs_string_push_char(&result, RS_PATH_SEP);
            p++;
        }
    } else if (is_absolute) {
        rs_string_push_char(&result, RS_PATH_SEP);
        if (RS_IS_PATH_SEP(*p)) {
            p++;
        }
    }
#else
    if (is_absolute) {
        rs_string_push_char(&result, RS_PATH_SEP);
        if (RS_IS_PATH_SEP(*p)) {
            p++;
        }
    }
#endif

    while (*p) {
        // Skip multiple separators
        if (RS_IS_PATH_SEP(*p)) {
            p++;
            continue;
        }

        // Find end of component
        const char *comp_start = p;
        while (*p && !RS_IS_PATH_SEP(*p)) {
            p++;
        }
        rs_size_t comp_len = p - comp_start;

        // Skip "."
        if (comp_len == 1 && comp_start[0] == '.') {
            continue;
        }

        // Handle ".."
        if (comp_len == 2 && comp_start[0] == '.' && comp_start[1] == '.') {
            // Remove last component if not empty
            char *data = rs_string_data_mut(&result);
            rs_size_t len = rs_string_len(&result);

            if (len > 0 && RS_IS_PATH_SEP(data[len - 1])) {
                len--;
            }

            // Find last separator (but don't go past drive letter on Windows)
            rs_size_t min_pos = 0;
#ifdef _WIN32
            // Don't remove drive letter
            if (len >= 2 && data[1] == ':') {
                min_pos = 2;
                if (len > 2 && RS_IS_PATH_SEP(data[2])) {
                    min_pos = 3;
                }
            }
#endif
            rs_size_t i = len;
            while (i > min_pos && !RS_IS_PATH_SEP(data[i - 1])) {
                i--;
            }

            rs_string_truncate(&result, i);
        } else {
            // Add component
            rs_size_t result_len = rs_string_len(&result);
            if (result_len > 0 && !RS_IS_PATH_SEP(rs_string_cstr(&result)[result_len - 1])) {
                rs_string_push_char(&result, RS_PATH_SEP);
            }
            rs_string_push_buf(&result, comp_start, comp_len);
        }
    }

    // Handle empty result
    if (rs_string_len(&result) == 0) {
        rs_string_destroy(&result);
        rs_string_clear(path);
#ifdef _WIN32
        return rs_string_push_cstr(path, is_absolute ? "\\" : ".");
#else
        return rs_string_push_cstr(path, is_absolute ? "/" : ".");
#endif
    }

    // Replace path with result
    rs_string_clear(path);
    rs_result_t ret = rs_string_push_cstr(path, rs_string_cstr(&result));
    rs_string_destroy(&result);
    return ret;
}

// ============================================================================
// Path Queries
// ============================================================================

int rs_path_is_absolute(rs_string_view_t path)
{
    const char *path_str = rs_sv_data(path);
    rs_size_t path_len = rs_sv_len(path);

    if (path_len == 0) {
        return 0;
    }

#ifdef _WIN32
    // Check for drive letter (C:) or UNC path (\\)
    if ((path_str[0] >= 'A' && path_str[0] <= 'Z') || (path_str[0] >= 'a' && path_str[0] <= 'z')) {
        return path_len > 1 && path_str[1] == ':';
    }
    return path_len > 1 && path_str[0] == '\\' && path_str[1] == '\\';
#else
    return path_str[0] == '/';
#endif
}

rs_string_view_t rs_path_extension(rs_string_view_t path)
{
    const char *path_str = rs_sv_data(path);
    rs_size_t path_len = rs_sv_len(path);

    const char *last_dot = NULL;
    const char *last_sep = NULL;

    for (rs_size_t i = 0; i < path_len; i++) {
        if (path_str[i] == '.') {
            last_dot = path_str + i;
        } else if (RS_IS_PATH_SEP(path_str[i])) {
            last_sep = path_str + i;
            last_dot = NULL; // Reset dot after separator
        }
    }

    // No dot, or dot is first character (hidden file), or no dot after last separator
    if (!last_dot || last_dot == path_str || (last_sep && last_dot < last_sep)) {
        return rs_sv_from_buf("", 0);
    }

    return rs_sv_from_buf(last_dot, path_len - (last_dot - path_str));
}

int rs_path_has_extension(rs_string_view_t path, const char *ext)
{
    if (!ext) {
        return 0;
    }

    rs_string_view_t path_ext = rs_path_extension(path);
    rs_string_view_t ext_sv = rs_sv_from_cstr(ext);

    return rs_sv_eq(path_ext, ext_sv);
}

int rs_path_exists(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return -1;
    }

#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path_cstr);
    free(path_cstr);
    return attrs != INVALID_FILE_ATTRIBUTES ? 1 : 0;
#else
    int result = access(path_cstr, F_OK);
    free(path_cstr);
    return result == 0 ? 1 : 0;
#endif
}

int rs_path_is_file(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return 0;
    }

#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path_cstr);
    free(path_cstr);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
    struct stat st;
    int result = stat(path_cstr, &st);
    free(path_cstr);
    if (result != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
#endif
}

int rs_path_is_dir(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return 0;
    }

#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path_cstr);
    free(path_cstr);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    int result = stat(path_cstr, &st);
    free(path_cstr);
    if (result != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
#endif
}

int rs_path_is_symlink(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return 0;
    }

#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path_cstr);
    free(path_cstr);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    return (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0 ? 1 : 0;
#else
    struct stat st;
    int result = lstat(path_cstr, &st);
    free(path_cstr);
    if (result != 0) {
        return 0;
    }
    return S_ISLNK(st.st_mode);
#endif
}

rs_result_t rs_path_relative(rs_string_t *out, rs_string_view_t from, rs_string_view_t to)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    // Both must be absolute or both must be relative
    int from_abs = rs_path_is_absolute(from);
    int to_abs = rs_path_is_absolute(to);

    if (from_abs != to_abs) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Both paths must be absolute or both must be relative");
    }

    // Normalize both paths first
    rs_string_t from_norm = rs_string_create(.allocator = rs_string_get_allocator(out));
    rs_string_t to_norm = rs_string_create(.allocator = rs_string_get_allocator(out));

    RS_TRY(rs_string_push_buf(&from_norm, rs_sv_data(from), rs_sv_len(from)));
    RS_TRY(rs_path_normalize(&from_norm));

    RS_TRY(rs_string_push_buf(&to_norm, rs_sv_data(to), rs_sv_len(to)));
    RS_TRY(rs_path_normalize(&to_norm));

    // Split both paths into components
    const char *from_str = rs_string_cstr(&from_norm);
    const char *to_str = rs_string_cstr(&to_norm);

    // Skip leading separator for absolute paths
    if (from_abs && *from_str == '/') {
        from_str++;
    }
    if (to_abs && *to_str == '/') {
        to_str++;
    }

    // Find common prefix
    const char *from_p = from_str;
    const char *to_p = to_str;
    const char *last_common_sep_from = from_str;
    const char *last_common_sep_to = to_str;

    while (*from_p && *to_p && *from_p == *to_p) {
        if (RS_IS_PATH_SEP(*from_p)) {
            last_common_sep_from = from_p + 1;
            last_common_sep_to = to_p + 1;
        }
        from_p++;
        to_p++;
    }

    // Check if we matched everything in 'from'
    if (*from_p == '\0') {
        // 'to' is a child of 'from' or they're the same
        if (*to_p == '\0') {
            // Same path
            last_common_sep_from = from_p;
            last_common_sep_to = to_p;
        } else if (RS_IS_PATH_SEP(*to_p)) {
            // 'to' is a direct child
            last_common_sep_from = from_p;
            last_common_sep_to = to_p + 1;
        }
    } else if (*to_p == '\0' && RS_IS_PATH_SEP(*from_p)) {
        // 'from' is a child of 'to'
        last_common_sep_from = from_p + 1;
        last_common_sep_to = to_p;
    }

    // Count remaining components in 'from' (these become "..")
    int up_count = 0;
    const char *p = last_common_sep_from;
    while (*p) {
        if (RS_IS_PATH_SEP(*p)) {
            up_count++;
        }
        p++;
    }
    // If there's a final component (no trailing slash), count it
    if (p > last_common_sep_from && !RS_IS_PATH_SEP(*(p - 1))) {
        up_count++;
    }

    // Build result
    rs_string_clear(out);

    // Add ".." for each level up
    for (int i = 0; i < up_count; i++) {
        if (i > 0) {
            RS_TRY(rs_string_push_char(out, RS_PATH_SEP));
        }
        RS_TRY(rs_string_push_cstr(out, ".."));
    }

    // Add remaining 'to' path
    if (*last_common_sep_to) {
        if (up_count > 0) {
            RS_TRY(rs_string_push_char(out, RS_PATH_SEP));
        }
        RS_TRY(rs_string_push_cstr(out, last_common_sep_to));
    }

    // Handle case where paths are the same
    if (rs_string_is_empty(out)) {
        RS_TRY(rs_string_push_cstr(out, "."));
    }

    rs_string_destroy(&from_norm);
    rs_string_destroy(&to_norm);

    return RS_OK;
}

rs_result_t rs_path_proximate(rs_string_t *out, rs_string_view_t from, rs_string_view_t to)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    // Get relative path
    rs_string_t relative = rs_string_create(.allocator = rs_string_get_allocator(out));
    rs_result_t result = rs_path_relative(&relative, from, to);

    if (result != RS_OK) {
        rs_string_destroy(&relative);
        return result;
    }

    // Compare lengths: relative vs absolute 'to'
    rs_size_t relative_len = rs_string_len(&relative);
    rs_size_t to_len = rs_sv_len(to);

    // Use whichever is shorter
    if (relative_len <= to_len) {
        rs_string_clear(out);
        result = rs_string_push_string(out, &relative);
    } else {
        rs_string_clear(out);
        result = rs_string_push_buf(out, rs_sv_data(to), to_len);
    }

    rs_string_destroy(&relative);
    return result;
}

// ============================================================================
// Filesystem Operations
// ============================================================================

rs_result_t rs_path_remove(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate path string");
    }

#ifdef _WIN32
    // Check if it's a directory or file and use the appropriate function
    DWORD attrs = GetFileAttributesA(path_cstr);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        free(path_cstr);
        return RS_ERROR_RET(RS_ERR_IO, "Path does not exist");
    }

    BOOL result;
    if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
        result = RemoveDirectoryA(path_cstr);
    } else {
        result = DeleteFileA(path_cstr);
    }

    free(path_cstr);
    if (!result) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to remove path");
    }
    return RS_OK;
#else
    int result = remove(path_cstr);
    free(path_cstr);
    if (result != 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to remove path");
    }
    return RS_OK;
#endif
}

rs_result_t rs_path_symlink(rs_string_view_t target, rs_string_view_t link_path)
{
    char *target_cstr = rs_sv_to_cstr(target);
    if (!target_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate target string");
    }

    char *link_cstr = rs_sv_to_cstr(link_path);
    if (!link_cstr) {
        free(target_cstr);
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate link path string");
    }

#ifdef _WIN32
    // Windows symlinks require special privileges or developer mode
    // Use CreateSymbolicLinkA
    DWORD flags = 0;
    // Check if target is a directory
    DWORD attrs = GetFileAttributesA(target_cstr);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags = SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    BOOL result = CreateSymbolicLinkA(link_cstr, target_cstr, flags);
    free(target_cstr);
    free(link_cstr);

    if (!result) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to create symbolic link");
    }
    return RS_OK;
#else
    int result = symlink(target_cstr, link_cstr);
    free(target_cstr);
    free(link_cstr);

    if (result != 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to create symbolic link");
    }
    return RS_OK;
#endif
}

rs_result_t rs_path_read_symlink(rs_string_view_t link_path, rs_string_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");

    char *link_cstr = rs_sv_to_cstr(link_path);
    if (!link_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate link path string");
    }

    rs_string_clear(out);

#ifdef _WIN32
    // Windows symlink reading is complex and requires opening the file
    // For now, return an error indicating it's not implemented
    free(link_cstr);
    return RS_ERROR_RET(RS_ERR_UNSUPPORTED, "Reading symlinks not yet implemented on Windows");
#else
    char buf[4096];
    ssize_t len = readlink(link_cstr, buf, sizeof(buf) - 1);
    free(link_cstr);

    if (len < 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to read symbolic link");
    }

    buf[len] = '\0';
    return rs_string_push_cstr(out, buf);
#endif
}

// ============================================================================
// Platform-specific
// ============================================================================

char rs_path_separator(void)
{
    return RS_PATH_SEP;
}

char rs_path_list_separator(void)
{
    return RS_PATH_LIST_SEP;
}
