#include <rs/std/error.h>
#include <rs/std/io/write.h>
#include <rs/std/string/string_view.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// ============================================================================
// File Handle
// ============================================================================

struct rs_io_writer_t {
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;
#endif
};

rs_io_writer_t *rs_io_writer_open(rs_string_view_t path, rs_file_mode_t mode, rs_io_create_mode_t create_mode)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate path string");
        return NULL;
    }

    rs_io_writer_t *writer = (rs_io_writer_t *)malloc(sizeof(rs_io_writer_t));
    if (!writer) {
        free(path_cstr);
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate writer");
        return NULL;
    }

#ifdef _WIN32
    RS_UNUSED(mode); // Windows doesn't use Unix file permissions
    DWORD creation_disposition;
    switch (create_mode) {
    case RS_IO_CREATE_TRUNCATE:
        creation_disposition = CREATE_ALWAYS;
        break;
    case RS_IO_CREATE_APPEND:
        creation_disposition = OPEN_ALWAYS;
        break;
    case RS_IO_CREATE_EXCL:
        creation_disposition = CREATE_NEW;
        break;
    case RS_IO_CREATE_OPEN:
        creation_disposition = OPEN_ALWAYS;
        break;
    default:
        free(path_cstr);
        free(writer);
        RS_ERROR(RS_ERR_INVALID, "Invalid create mode");
        return NULL;
    }

    writer->handle = CreateFileA(path_cstr, GENERIC_WRITE, 0, NULL, creation_disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    free(path_cstr);

    if (writer->handle == INVALID_HANDLE_VALUE) {
        free(writer);
        RS_ERROR(RS_ERR_IO, "Failed to open file for writing");
        return NULL;
    }

    // For append mode, seek to end
    if (create_mode == RS_IO_CREATE_APPEND) {
        SetFilePointer(writer->handle, 0, NULL, FILE_END);
    }
#else
    int flags = O_WRONLY | O_CREAT;
    switch (create_mode) {
    case RS_IO_CREATE_TRUNCATE:
        flags |= O_TRUNC;
        break;
    case RS_IO_CREATE_APPEND:
        flags |= O_APPEND;
        break;
    case RS_IO_CREATE_EXCL:
        flags |= O_EXCL;
        break;
    case RS_IO_CREATE_OPEN:
        // O_CREAT without O_TRUNC - open existing or create new
        break;
    default:
        free(path_cstr);
        free(writer);
        RS_ERROR(RS_ERR_INVALID, "Invalid create mode");
        return NULL;
    }

    writer->fd = open(path_cstr, flags, mode);
    free(path_cstr);

    if (writer->fd < 0) {
        free(writer);
        RS_ERROR(RS_ERR_IO, "Failed to open file for writing");
        return NULL;
    }
#endif

    return writer;
}

void rs_io_writer_close(rs_io_writer_t *writer)
{
    if (!writer) {
        return;
    }

#ifdef _WIN32
    if (writer->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(writer->handle);
    }
#else
    if (writer->fd >= 0) {
        close(writer->fd);
    }
#endif

    free(writer);
}

// ============================================================================
// Writing Operations
// ============================================================================

rs_result_t rs_io_write_all(rs_string_view_t path, const void *buf, rs_size_t count)
{
    RS_CHECK(buf != NULL, RS_ERR_INVALID, "Buffer is NULL");

    rs_io_writer_t *writer = rs_io_writer_open(path, 0644, RS_IO_CREATE_TRUNCATE);
    if (!writer) {
        return RS_ERR_IO;
    }

    rs_result_t result = rs_io_writer_write_exact(writer, buf, count);
    rs_io_writer_close(writer);

    return result;
}

rs_result_t rs_io_append_all(rs_string_view_t path, const void *buf, rs_size_t count)
{
    RS_CHECK(buf != NULL, RS_ERR_INVALID, "Buffer is NULL");

    rs_io_writer_t *writer = rs_io_writer_open(path, 0644, RS_IO_CREATE_APPEND);
    if (!writer) {
        return RS_ERR_IO;
    }

    rs_result_t result = rs_io_writer_write_exact(writer, buf, count);
    rs_io_writer_close(writer);

    return result;
}

rs_result_t rs_io_write_str(rs_string_view_t path, rs_string_view_t content)
{
    return rs_io_write_all(path, rs_sv_data(content), rs_sv_len(content));
}

rs_result_t rs_io_append_str(rs_string_view_t path, rs_string_view_t content)
{
    return rs_io_append_all(path, rs_sv_data(content), rs_sv_len(content));
}

rs_ssize_t rs_io_writer_write(rs_io_writer_t *writer, const void *buf, rs_size_t count)
{
    if (!writer || !buf) {
        RS_ERROR(RS_ERR_INVALID, "NULL parameter to rs_io_writer_write");
        return -1;
    }

#ifdef _WIN32
    DWORD bytes_written;
    if (!WriteFile(writer->handle, buf, (DWORD)count, &bytes_written, NULL)) {
        RS_ERROR(RS_ERR_IO, "WriteFile failed");
        return -1;
    }
    return (rs_ssize_t)bytes_written;
#else
    ssize_t n = write(writer->fd, buf, count);
    if (n < 0) {
        RS_ERROR(RS_ERR_IO, "write failed");
        return -1;
    }
    return (rs_ssize_t)n;
#endif
}

rs_result_t rs_io_writer_write_exact(rs_io_writer_t *writer, const void *buf, rs_size_t count)
{
    RS_CHECK(writer != NULL, RS_ERR_INVALID, "Writer is NULL");
    RS_CHECK(buf != NULL, RS_ERR_INVALID, "Buffer is NULL");

    rs_size_t total_written = 0;
    const char *ptr = (const char *)buf;

    while (total_written < count) {
        rs_ssize_t n = rs_io_writer_write(writer, ptr + total_written, count - total_written);
        if (n < 0) {
            return RS_ERR_IO;
        }
        if (n == 0) {
            return RS_ERROR_RET(RS_ERR_IO, "Failed to write requested bytes");
        }
        total_written += n;
    }

    return RS_OK;
}

rs_result_t rs_io_writer_flush(rs_io_writer_t *writer)
{
    RS_CHECK(writer != NULL, RS_ERR_INVALID, "Writer is NULL");

#ifdef _WIN32
    if (!FlushFileBuffers(writer->handle)) {
        return RS_ERROR_RET(RS_ERR_IO, "FlushFileBuffers failed");
    }
    return RS_OK;
#else
    if (fsync(writer->fd) < 0) {
        return RS_ERROR_RET(RS_ERR_IO, "fsync failed");
    }
    return RS_OK;
#endif
}

// ============================================================================
// Positional Writing (pwrite-style)
// ============================================================================

rs_ssize_t rs_io_pwrite(rs_string_view_t path, const void *buf, rs_size_t count, rs_size_t offset)
{
    // For pwrite on a file path, we need to open with appropriate flags
    // that allow writing at specific positions without truncating
    rs_io_writer_t *writer = rs_io_writer_open(path, 0644, RS_IO_CREATE_OPEN);
    if (!writer) {
        return -1;
    }

    rs_ssize_t n = rs_io_writer_write_at(writer, buf, count, offset);
    rs_io_writer_close(writer);

    return n;
}

rs_ssize_t rs_io_writer_write_at(rs_io_writer_t *writer, const void *buf, rs_size_t count, rs_size_t offset)
{
    if (!writer || !buf) {
        RS_ERROR(RS_ERR_INVALID, "NULL parameter to rs_io_writer_write_at");
        return -1;
    }

#ifdef _WIN32
    // For synchronous handles, we must manually seek and restore position
    // Save current position
    LARGE_INTEGER zero = {0};
    LARGE_INTEGER saved_pos;
    if (!SetFilePointerEx(writer->handle, zero, &saved_pos, FILE_CURRENT)) {
        RS_ERROR(RS_ERR_IO, "Failed to get current file position");
        return -1;
    }

    // Seek to offset
    LARGE_INTEGER seek_pos;
    seek_pos.QuadPart = (LONGLONG)offset;
    if (!SetFilePointerEx(writer->handle, seek_pos, NULL, FILE_BEGIN)) {
        RS_ERROR(RS_ERR_IO, "Failed to seek to offset");
        return -1;
    }

    // Write
    DWORD bytes_written;
    BOOL success = WriteFile(writer->handle, buf, (DWORD)count, &bytes_written, NULL);

    // Restore position
    SetFilePointerEx(writer->handle, saved_pos, NULL, FILE_BEGIN);

    if (!success) {
        RS_ERROR(RS_ERR_IO, "WriteFile failed");
        return -1;
    }
    return (rs_ssize_t)bytes_written;
#else
    ssize_t n = pwrite(writer->fd, buf, count, offset);
    if (n < 0) {
        RS_ERROR(RS_ERR_IO, "pwrite failed");
        return -1;
    }
    return (rs_ssize_t)n;
#endif
}

// ============================================================================
// Seeking
// ============================================================================

rs_ssize_t rs_io_writer_seek(rs_io_writer_t *writer, rs_ssize_t offset, rs_io_seek_mode_t mode)
{
    if (!writer) {
        RS_ERROR(RS_ERR_INVALID, "Writer is NULL");
        return -1;
    }

#ifdef _WIN32
    DWORD move_method;
    switch (mode) {
    case RS_IO_SEEK_SET:
        move_method = FILE_BEGIN;
        break;
    case RS_IO_SEEK_CUR:
        move_method = FILE_CURRENT;
        break;
    case RS_IO_SEEK_END:
        move_method = FILE_END;
        break;
    default:
        RS_ERROR(RS_ERR_INVALID, "Invalid seek mode");
        return -1;
    }

    LARGE_INTEGER li;
    li.QuadPart = offset;

    LARGE_INTEGER new_pos;
    if (!SetFilePointerEx(writer->handle, li, &new_pos, move_method)) {
        RS_ERROR(RS_ERR_IO, "SetFilePointerEx failed");
        return -1;
    }

    return (rs_ssize_t)new_pos.QuadPart;
#else
    int whence;
    switch (mode) {
    case RS_IO_SEEK_SET:
        whence = SEEK_SET;
        break;
    case RS_IO_SEEK_CUR:
        whence = SEEK_CUR;
        break;
    case RS_IO_SEEK_END:
        whence = SEEK_END;
        break;
    default:
        RS_ERROR(RS_ERR_INVALID, "Invalid seek mode");
        return -1;
    }

    off_t new_pos = lseek(writer->fd, offset, whence);
    if (new_pos < 0) {
        RS_ERROR(RS_ERR_IO, "lseek failed");
        return -1;
    }

    return (rs_ssize_t)new_pos;
#endif
}

rs_ssize_t rs_io_writer_tell(rs_io_writer_t *writer)
{
    return rs_io_writer_seek(writer, 0, RS_IO_SEEK_CUR);
}
