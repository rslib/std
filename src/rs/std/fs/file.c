#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/array.h>
#include <rs/std/error.h>
#include <rs/std/fs/file.h>
#include <rs/std/fs/path.h>
#include <rs/std/io/read.h>
#include <rs/std/io/write.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

// ============================================================================
// File Metadata
// ============================================================================

rs_bool rs_file_exists(rs_string_view_t path)
{
    return rs_path_is_file(path) != 0;
}

rs_bool rs_file_is_file(rs_string_view_t path)
{
    return rs_path_is_file(path) != 0;
}

rs_ssize_t rs_file_size(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate path string");
        return -1;
    }

#ifdef _WIN32
    HANDLE handle =
        CreateFileA(path_cstr, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free(path_cstr);

    if (handle == INVALID_HANDLE_VALUE) {
        RS_ERROR(RS_ERR_IO, "Failed to open file");
        return -1;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(handle, &file_size)) {
        CloseHandle(handle);
        RS_ERROR(RS_ERR_IO, "Failed to get file size");
        return -1;
    }

    CloseHandle(handle);
    return (rs_ssize_t)file_size.QuadPart;
#else
    struct stat st;
    int result = stat(path_cstr, &st);
    free(path_cstr);

    if (result != 0) {
        RS_ERROR(RS_ERR_IO, "Failed to stat file");
        return -1;
    }

    return (rs_ssize_t)st.st_size;
#endif
}

// ============================================================================
// Reading
// ============================================================================

rs_result_t rs_file_read(rs_string_view_t path, rs_string_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");
    return rs_io_read_all(path, out);
}

rs_result_t rs_file_read_lines(rs_string_view_t path, rs_array_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output array is NULL");

    // Get allocator from array for temporary content and line strings
    rs_allocator_t *alloc = rs_array_allocator(out);

    // Read entire file
    rs_string_t content = rs_string_create(.allocator = alloc);
    rs_result_t result = rs_io_read_all(path, &content);
    if (result != RS_OK) {
        rs_string_destroy(&content);
        return result;
    }

    // Split by lines
    const char *data = rs_string_cstr(&content);
    rs_size_t len = rs_string_len(&content);
    rs_size_t line_start = 0;

    for (rs_size_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            // Create line (excluding \n)
            rs_size_t line_len = i - line_start;
            // Handle \r\n
            if (line_len > 0 && data[i - 1] == '\r') {
                line_len--;
            }

            rs_string_t line = rs_string_from_buf(data + line_start, line_len, .allocator = alloc);
            rs_array_push(out, &line);

            line_start = i + 1;
        }
    }

    // Add last line if not empty
    if (line_start < len) {
        rs_size_t line_len = len - line_start;
        // Handle trailing \r
        if (line_len > 0 && data[len - 1] == '\r') {
            line_len--;
        }
        rs_string_t line = rs_string_from_buf(data + line_start, line_len, .allocator = alloc);
        rs_array_push(out, &line);
    }

    rs_string_destroy(&content);
    return RS_OK;
}

// ============================================================================
// Writing
// ============================================================================

rs_result_t rs_file_write(rs_string_view_t path, rs_string_view_t content)
{
    return rs_io_write_str(path, content);
}

rs_result_t rs_file_append(rs_string_view_t path, rs_string_view_t content)
{
    return rs_io_append_str(path, content);
}

// ============================================================================
// File Operations
// ============================================================================

rs_result_t rs_file_copy(rs_string_view_t src, rs_string_view_t dst)
{
    char *src_cstr = rs_sv_to_cstr(src);
    if (!src_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate source path string");
    }

    char *dst_cstr = rs_sv_to_cstr(dst);
    if (!dst_cstr) {
        free(src_cstr);
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate destination path string");
    }

#ifdef _WIN32
    BOOL result = CopyFileA(src_cstr, dst_cstr, FALSE);
    free(src_cstr);
    free(dst_cstr);

    if (!result) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to copy file");
    }
    return RS_OK;
#else
    // Read source file
    rs_io_reader_t *reader = rs_io_reader_open(src);
    if (!reader) {
        free(src_cstr);
        free(dst_cstr);
        return RS_ERROR_RET(RS_ERR_IO, "Failed to open source file");
    }

    // Open destination file
    rs_io_writer_t *writer = rs_io_writer_open(dst, 0644, RS_IO_CREATE_TRUNCATE);
    if (!writer) {
        rs_io_reader_close(reader);
        free(src_cstr);
        free(dst_cstr);
        return RS_ERROR_RET(RS_ERR_IO, "Failed to open destination file");
    }

    // Copy data
    char buf[8192];
    rs_ssize_t n;
    rs_result_t copy_result = RS_OK;

    while ((n = rs_io_reader_read(reader, buf, sizeof(buf))) > 0) {
        rs_result_t write_result = rs_io_writer_write_exact(writer, buf, n);
        if (write_result != RS_OK) {
            copy_result = write_result;
            break;
        }
    }

    if (n < 0) {
        copy_result = RS_ERR_IO;
    }

    rs_io_reader_close(reader);
    rs_io_writer_close(writer);
    free(src_cstr);
    free(dst_cstr);

    return copy_result;
#endif
}

rs_result_t rs_file_move(rs_string_view_t src, rs_string_view_t dst)
{
    char *src_cstr = rs_sv_to_cstr(src);
    if (!src_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate source path string");
    }

    char *dst_cstr = rs_sv_to_cstr(dst);
    if (!dst_cstr) {
        free(src_cstr);
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate destination path string");
    }

#ifdef _WIN32
    BOOL result = MoveFileExA(src_cstr, dst_cstr, MOVEFILE_REPLACE_EXISTING);
    free(src_cstr);
    free(dst_cstr);

    if (!result) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to move file");
    }
    return RS_OK;
#else
    int result = rename(src_cstr, dst_cstr);
    free(src_cstr);
    free(dst_cstr);

    if (result != 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to move file");
    }
    return RS_OK;
#endif
}

rs_result_t rs_file_remove(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate path string");
    }

#ifdef _WIN32
    BOOL result = DeleteFileA(path_cstr);
    free(path_cstr);

    if (!result) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to delete file");
    }
    return RS_OK;
#else
    int result = unlink(path_cstr);
    free(path_cstr);

    if (result != 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to delete file");
    }
    return RS_OK;
#endif
}
