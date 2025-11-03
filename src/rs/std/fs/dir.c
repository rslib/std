#include <rs/std/containers/array.h>
#include <rs/std/error.h>
#include <rs/std/fs/dir.h>
#include <rs/std/fs/path.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// ============================================================================
// Directory Metadata
// ============================================================================

rs_bool rs_dir_exists(rs_string_view_t path)
{
    return rs_path_is_dir(path) != 0;
}

rs_bool rs_dir_is_dir(rs_string_view_t path)
{
    return rs_path_is_dir(path) != 0;
}

// ============================================================================
// Directory Operations
// ============================================================================

rs_result_t rs_dir_create(rs_string_view_t path, rs_file_mode_t mode)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate path string");
    }

#ifdef _WIN32
    RS_UNUSED(mode);
    int result = _mkdir(path_cstr);
#else
    int result = mkdir(path_cstr, mode);
#endif

    free(path_cstr);

    if (result != 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to create directory");
    }

    return RS_OK;
}

rs_result_t rs_dir_create_all(rs_string_view_t path, rs_file_mode_t mode)
{
    // Normalize path first
    rs_allocator_t *alloc = rs_allocator_system();
    rs_string_t normalized = rs_string_from_buf(rs_sv_data(path), rs_sv_len(path), .allocator = alloc);
    rs_path_normalize(&normalized);

    // Skip if already exists
    if (rs_dir_exists(rs_sv_from_string(normalized))) {
        rs_string_destroy(&normalized);
        return RS_OK;
    }

    // Get parent directory
    rs_string_t parent = rs_string_create(.allocator = alloc);
    rs_path_dirname(&parent, rs_sv_from_string(normalized));

    // Recursively create parent if it doesn't exist
    if (!rs_string_eq_cstr(&parent, ".") && !rs_string_eq_cstr(&parent, "/") &&
        !rs_dir_exists(rs_sv_from_string(parent))) {
        rs_result_t result = rs_dir_create_all(rs_sv_from_string(parent), mode);
        if (result != RS_OK) {
            rs_string_destroy(&parent);
            rs_string_destroy(&normalized);
            return result;
        }
    }

    rs_string_destroy(&parent);

    // Create the directory itself
    rs_result_t result = rs_dir_create(rs_sv_from_string(normalized), mode);
    rs_string_destroy(&normalized);

    return result;
}

rs_result_t rs_dir_remove(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate path string");
    }

#ifdef _WIN32
    int result = _rmdir(path_cstr);
#else
    int result = rmdir(path_cstr);
#endif

    free(path_cstr);

    if (result != 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to remove directory");
    }

    return RS_OK;
}

rs_result_t rs_dir_remove_all(rs_string_view_t path)
{
    // Read directory entries
    rs_allocator_t *alloc = rs_allocator_system();
    rs_array_t entries = rs_array_create(sizeof(rs_string_t), .allocator = alloc);

    rs_result_t result = rs_dir_read(path, &entries);
    if (result != RS_OK) {
        rs_array_destroy(&entries);
        return result;
    }

    // Remove each entry recursively
    for (rs_size_t i = 0; i < rs_array_len(&entries); i++) {
        rs_string_t *entry = (rs_string_t *)rs_array_get(&entries, i);

        // Build full path
        rs_string_t full_path = rs_string_from_buf(rs_sv_data(path), rs_sv_len(path), .allocator = alloc);
        rs_path_append(&full_path, rs_sv_from_string(*entry));

        // Check if it's a directory
        if (rs_path_is_dir(rs_sv_from_string(full_path))) {
            result = rs_dir_remove_all(rs_sv_from_string(full_path));
        } else {
            result = rs_path_remove(rs_sv_from_string(full_path));
        }

        rs_string_destroy(&full_path);

        if (result != RS_OK) {
            // Clean up remaining entries
            for (rs_size_t j = i; j < rs_array_len(&entries); j++) {
                rs_string_t *e = (rs_string_t *)rs_array_get(&entries, j);
                rs_string_destroy(e);
            }
            rs_array_destroy(&entries);
            return result;
        }

        rs_string_destroy(entry);
    }

    rs_array_destroy(&entries);

    // Remove the directory itself
    return rs_dir_remove(path);
}

// ============================================================================
// Directory Reading
// ============================================================================

rs_result_t rs_dir_read(rs_string_view_t path, rs_array_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output array is NULL");

    rs_allocator_t *alloc = rs_array_allocator(out);
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate path string");
    }

#ifdef _WIN32
    // Windows implementation using FindFirstFile/FindNextFile
    rs_string_t search_path = rs_string_from_cstr(path_cstr, .allocator = alloc);
    rs_path_append(&search_path, rs_sv_from_cstr("*"));

    char *search_cstr = rs_string_to_cstr(&search_path);
    rs_string_destroy(&search_path);
    free(path_cstr);

    if (!search_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate search path string");
    }

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(search_cstr, &find_data);
    free(search_cstr);

    if (handle == INVALID_HANDLE_VALUE) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to open directory");
    }

    do {
        // Skip "." and ".."
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        rs_string_t entry = rs_string_from_cstr(find_data.cFileName, .allocator = alloc);
        rs_array_push(out, &entry);
    } while (FindNextFileA(handle, &find_data));

    FindClose(handle);
#else
    // Unix implementation using opendir/readdir
    DIR *dir = opendir(path_cstr);
    free(path_cstr);

    if (!dir) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to open directory");
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        rs_string_t name = rs_string_from_cstr(entry->d_name, .allocator = alloc);
        rs_array_push(out, &name);
    }

    closedir(dir);
#endif

    return RS_OK;
}

// ============================================================================
// Current Working Directory
// ============================================================================

rs_result_t rs_dir_get_current(rs_string_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");

#ifdef _WIN32
    char buf[MAX_PATH];
    if (!_getcwd(buf, sizeof(buf))) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to get current directory");
    }
#else
    char buf[4096];
    if (!getcwd(buf, sizeof(buf))) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to get current directory");
    }
#endif

    rs_string_clear(out);
    rs_string_push_cstr(out, buf);

    return RS_OK;
}

rs_result_t rs_dir_set_current(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate path string");
    }

#ifdef _WIN32
    int result = _chdir(path_cstr);
#else
    int result = chdir(path_cstr);
#endif

    free(path_cstr);

    if (result != 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to set current directory");
    }

    return RS_OK;
}
