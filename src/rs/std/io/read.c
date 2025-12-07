#include <rs/std/error.h>
#include <rs/std/io/read.h>
#include <rs/std/string/string.h>
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

struct rs_io_reader_t {
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;
#endif
};

rs_io_reader_t *rs_io_reader_open(rs_string_view_t path)
{
    char *path_cstr = rs_sv_to_cstr(path);
    if (!path_cstr) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate path string");
        return NULL;
    }

    rs_io_reader_t *reader = (rs_io_reader_t *)malloc(sizeof(rs_io_reader_t));
    if (!reader) {
        free(path_cstr);
        RS_ERROR(RS_ERR_NOMEM, "Failed to allocate reader");
        return NULL;
    }

#ifdef _WIN32
    reader->handle =
        CreateFileA(path_cstr, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free(path_cstr);

    if (reader->handle == INVALID_HANDLE_VALUE) {
        free(reader);
        RS_ERROR(RS_ERR_IO, "Failed to open file for reading");
        return NULL;
    }
#else
    reader->fd = open(path_cstr, O_RDONLY);
    free(path_cstr);

    if (reader->fd < 0) {
        free(reader);
        RS_ERROR(RS_ERR_IO, "Failed to open file for reading");
        return NULL;
    }
#endif

    return reader;
}

void rs_io_reader_close(rs_io_reader_t *reader)
{
    if (!reader) {
        return;
    }

#ifdef _WIN32
    if (reader->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(reader->handle);
    }
#else
    if (reader->fd >= 0) {
        close(reader->fd);
    }
#endif

    free(reader);
}

// ============================================================================
// Reading Operations
// ============================================================================

rs_result_t rs_io_read_all(rs_string_view_t path, rs_string_t *out)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output parameter is NULL");

    rs_io_reader_t *reader = rs_io_reader_open(path);
    if (!reader) {
        return RS_ERR_IO;
    }

    // Get file size
    rs_ssize_t size = rs_io_reader_size(reader);
    if (size < 0) {
        rs_io_reader_close(reader);
        return RS_ERROR_RET(RS_ERR_IO, "Failed to get file size");
    }

    // Reserve space
    rs_string_reserve(out, rs_string_len(out) + size);

    // Read in chunks
    char buf[4096];

    while (1) {
        rs_ssize_t n = rs_io_reader_read(reader, buf, sizeof(buf));
        if (n < 0) {
            rs_io_reader_close(reader);
            return RS_ERROR_RET(RS_ERR_IO, "Failed to read from file");
        }
        if (n == 0) {
            break; // EOF
        }

        rs_result_t result = rs_string_push_buf(out, buf, n);
        if (result != RS_OK) {
            rs_io_reader_close(reader);
            return result;
        }
    }

    rs_io_reader_close(reader);
    return RS_OK;
}

rs_ssize_t rs_io_reader_read(rs_io_reader_t *reader, void *buf, rs_size_t count)
{
    if (!reader || !buf) {
        RS_ERROR(RS_ERR_INVALID, "NULL parameter to rs_io_reader_read");
        return -1;
    }

#ifdef _WIN32
    DWORD bytes_read;
    if (!ReadFile(reader->handle, buf, (DWORD)count, &bytes_read, NULL)) {
        RS_ERROR(RS_ERR_IO, "ReadFile failed");
        return -1;
    }
    return (rs_ssize_t)bytes_read;
#else
    ssize_t n = read(reader->fd, buf, count);
    if (n < 0) {
        RS_ERROR(RS_ERR_IO, "read failed");
        return -1;
    }
    return (rs_ssize_t)n;
#endif
}

rs_result_t rs_io_reader_read_exact(rs_io_reader_t *reader, void *buf, rs_size_t count)
{
    RS_CHECK(reader != NULL, RS_ERR_INVALID, "Reader is NULL");
    RS_CHECK(buf != NULL, RS_ERR_INVALID, "Buffer is NULL");

    rs_size_t total_read = 0;
    char *ptr = (char *)buf;

    while (total_read < count) {
        rs_ssize_t n = rs_io_reader_read(reader, ptr + total_read, count - total_read);
        if (n < 0) {
            return RS_ERR_IO;
        }
        if (n == 0) {
            return RS_ERROR_RET(RS_ERR_EOF, "EOF before reading requested bytes");
        }
        total_read += n;
    }

    return RS_OK;
}

// ============================================================================
// Positional Reading (pread-style)
// ============================================================================

rs_ssize_t rs_io_pread(rs_string_view_t path, void *buf, rs_size_t count, rs_size_t offset)
{
    rs_io_reader_t *reader = rs_io_reader_open(path);
    if (!reader) {
        return -1;
    }

    rs_ssize_t n = rs_io_reader_read_at(reader, buf, count, offset);
    rs_io_reader_close(reader);

    return n;
}

rs_ssize_t rs_io_reader_read_at(rs_io_reader_t *reader, void *buf, rs_size_t count, rs_size_t offset)
{
    if (!reader || !buf) {
        RS_ERROR(RS_ERR_INVALID, "NULL parameter to rs_io_reader_read_at");
        return -1;
    }

#ifdef _WIN32
    // For synchronous handles, we must manually seek and restore position
    // Save current position
    LARGE_INTEGER zero = {0};
    LARGE_INTEGER saved_pos;
    if (!SetFilePointerEx(reader->handle, zero, &saved_pos, FILE_CURRENT)) {
        RS_ERROR(RS_ERR_IO, "Failed to get current file position");
        return -1;
    }

    // Seek to offset
    LARGE_INTEGER seek_pos;
    seek_pos.QuadPart = (LONGLONG)offset;
    if (!SetFilePointerEx(reader->handle, seek_pos, NULL, FILE_BEGIN)) {
        RS_ERROR(RS_ERR_IO, "Failed to seek to offset");
        return -1;
    }

    // Read
    DWORD bytes_read;
    BOOL success = ReadFile(reader->handle, buf, (DWORD)count, &bytes_read, NULL);

    // Restore position
    SetFilePointerEx(reader->handle, saved_pos, NULL, FILE_BEGIN);

    if (!success) {
        RS_ERROR(RS_ERR_IO, "ReadFile failed");
        return -1;
    }
    return (rs_ssize_t)bytes_read;
#else
    ssize_t n = pread(reader->fd, buf, count, offset);
    if (n < 0) {
        RS_ERROR(RS_ERR_IO, "pread failed");
        return -1;
    }
    return (rs_ssize_t)n;
#endif
}

// ============================================================================
// Seeking
// ============================================================================

rs_ssize_t rs_io_reader_seek(rs_io_reader_t *reader, rs_ssize_t offset, rs_io_seek_mode_t mode)
{
    if (!reader) {
        RS_ERROR(RS_ERR_INVALID, "Reader is NULL");
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
    if (!SetFilePointerEx(reader->handle, li, &new_pos, move_method)) {
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

    off_t new_pos = lseek(reader->fd, offset, whence);
    if (new_pos < 0) {
        RS_ERROR(RS_ERR_IO, "lseek failed");
        return -1;
    }

    return (rs_ssize_t)new_pos;
#endif
}

rs_ssize_t rs_io_reader_tell(rs_io_reader_t *reader)
{
    return rs_io_reader_seek(reader, 0, RS_IO_SEEK_CUR);
}

rs_ssize_t rs_io_reader_size(rs_io_reader_t *reader)
{
    if (!reader) {
        RS_ERROR(RS_ERR_INVALID, "Reader is NULL");
        return -1;
    }

#ifdef _WIN32
    LARGE_INTEGER size;
    if (!GetFileSizeEx(reader->handle, &size)) {
        RS_ERROR(RS_ERR_IO, "GetFileSizeEx failed");
        return -1;
    }
    return (rs_ssize_t)size.QuadPart;
#else
    struct stat st;
    if (fstat(reader->fd, &st) < 0) {
        RS_ERROR(RS_ERR_IO, "fstat failed");
        return -1;
    }
    return (rs_ssize_t)st.st_size;
#endif
}
