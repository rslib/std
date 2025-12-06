#include <rs/std/allocators/allocator.h>
#include <rs/std/error.h>
#include <rs/std/fs/path.h>
#include <rs/std/io/read.h>
#include <rs/std/io/write.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_write"

static rs_allocator_t *allocator;
static rs_string_t test_file_path;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();
    test_file_path = rs_string_create(.allocator = allocator);

    // Get temp directory and create test file path
    rs_path_get_temp(&test_file_path);
    rs_path_append(&test_file_path, rs_sv_from_cstr("rs_test_write.txt"));

    // Clean up any existing test file
    rs_path_remove(rs_sv_from_string(&test_file_path));
}

void tearDown(void)
{
    // Clean up test file
    rs_path_remove(rs_sv_from_string(&test_file_path));
    rs_string_destroy(&test_file_path);
    rs_log_shutdown();
}

// ============================================================================
// File Handle Tests
// ============================================================================

void test_writer_open_close(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    rs_io_writer_close(writer);
}

void test_writer_open_modes(void)
{
    // Test truncate mode
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);
    rs_io_writer_close(writer);

    // Test append mode
    writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_APPEND);
    TEST_ASSERT_NOT_NULL(writer);
    rs_io_writer_close(writer);

    rs_path_remove(rs_sv_from_string(&test_file_path));

    // Test exclusive mode (file doesn't exist - should succeed)
    writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_EXCL);
    TEST_ASSERT_NOT_NULL(writer);
    rs_io_writer_close(writer);

    // Test exclusive mode again (file exists - should fail)
    writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_EXCL);
    TEST_ASSERT_NULL(writer);
}

void test_writer_close_null(void)
{
    // Should not crash
    rs_io_writer_close(NULL);
}

// ============================================================================
// Writing Operations Tests
// ============================================================================

void test_write_all(void)
{
    const char *data = "Hello, World!";
    rs_result_t result = rs_io_write_all(rs_sv_from_string(&test_file_path), data, strlen(data));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Verify by reading back
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, data));
    rs_string_destroy(&content);
}

void test_write_str(void)
{
    const char *data = "Test string write";
    rs_result_t result = rs_io_write_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr(data));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Verify
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, data));
    rs_string_destroy(&content);
}

void test_append_all(void)
{
    const char *data1 = "First line\n";
    const char *data2 = "Second line\n";

    // Write first line
    rs_io_write_all(rs_sv_from_string(&test_file_path), data1, strlen(data1));

    // Append second line
    rs_result_t result = rs_io_append_all(rs_sv_from_string(&test_file_path), data2, strlen(data2));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Verify both lines are present
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_starts_with(&content, data1));
    TEST_ASSERT_TRUE(rs_string_ends_with(&content, data2));
    rs_string_destroy(&content);
}

void test_append_str(void)
{
    const char *data1 = "Line 1\n";
    const char *data2 = "Line 2\n";

    rs_io_write_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr(data1));
    rs_result_t result = rs_io_append_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr(data2));
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_contains(&content, data1));
    TEST_ASSERT_TRUE(rs_string_contains(&content, data2));
    rs_string_destroy(&content);
}

void test_writer_write(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    const char *data = "Test data";
    rs_ssize_t n = rs_io_writer_write(writer, data, strlen(data));
    TEST_ASSERT_EQUAL(strlen(data), n);

    rs_io_writer_close(writer);

    // Verify
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, data));
    rs_string_destroy(&content);
}

void test_writer_write_exact(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    const char *data = "Exact write test";
    rs_result_t result = rs_io_writer_write_exact(writer, data, strlen(data));
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_io_writer_close(writer);

    // Verify
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, data));
    rs_string_destroy(&content);
}

void test_writer_flush(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    const char *data = "Flush test";
    rs_io_writer_write(writer, data, strlen(data));

    rs_result_t result = rs_io_writer_flush(writer);
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_io_writer_close(writer);
}

// ============================================================================
// Positional Writing Tests
// ============================================================================

void test_pwrite(void)
{
    // Create initial file with some data
    const char *initial = "AAAAAAAAAA"; // 10 A's
    rs_io_write_all(rs_sv_from_string(&test_file_path), initial, strlen(initial));

    // Write "PATCH" at offset 2
    rs_ssize_t n = rs_io_pwrite(rs_sv_from_string(&test_file_path), "PATCH", 5, 2);
    TEST_ASSERT_EQUAL(5, n);

    // Verify result should be "AAPATCHAAA"
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, "AAPATCHAAA"));
    rs_string_destroy(&content);
}

void test_writer_write_at(void)
{
    // Create initial file
    const char *initial = "0123456789";
    rs_io_write_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr(initial));

    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    // Write initial data
    rs_io_writer_write(writer, initial, strlen(initial));

    // Write "XYZ" at offset 3 (position should not change)
    rs_ssize_t n = rs_io_writer_write_at(writer, "XYZ", 3, 3);
    TEST_ASSERT_EQUAL(3, n);

    // Write more data at current position (should be at end)
    rs_io_writer_write(writer, "!", 1);

    rs_io_writer_close(writer);

    // Result should be "012XYZ6789!"
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, "012XYZ6789!"));
    rs_string_destroy(&content);
}

// ============================================================================
// Seeking Tests
// ============================================================================

void test_writer_seek_set(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    // Write initial data
    rs_io_writer_write(writer, "0123456789", 10);

    // Seek to position 3
    rs_ssize_t pos = rs_io_writer_seek(writer, 3, RS_IO_SEEK_SET);
    TEST_ASSERT_EQUAL(3, pos);

    // Overwrite with "ABC"
    rs_io_writer_write(writer, "ABC", 3);

    rs_io_writer_close(writer);

    // Result should be "012ABC6789"
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, "012ABC6789"));
    rs_string_destroy(&content);
}

void test_writer_seek_cur(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    rs_io_writer_write(writer, "0123456789", 10);

    // Seek back 5 bytes from current position
    rs_ssize_t pos = rs_io_writer_seek(writer, -5, RS_IO_SEEK_CUR);
    TEST_ASSERT_EQUAL(5, pos);

    rs_io_writer_write(writer, "XYZ", 3);

    rs_io_writer_close(writer);

    // Result should be "01234XYZ89"
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, "01234XYZ89"));
    rs_string_destroy(&content);
}

void test_writer_seek_end(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    rs_io_writer_write(writer, "0123456789", 10);

    // Seek to end
    rs_ssize_t pos = rs_io_writer_seek(writer, 0, RS_IO_SEEK_END);
    TEST_ASSERT_EQUAL(10, pos);

    // Append data
    rs_io_writer_write(writer, "!", 1);

    rs_io_writer_close(writer);

    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, "0123456789!"));
    rs_string_destroy(&content);
}

void test_writer_tell(void)
{
    rs_io_writer_t *writer = rs_io_writer_open(rs_sv_from_string(&test_file_path), 0644, RS_IO_CREATE_TRUNCATE);
    TEST_ASSERT_NOT_NULL(writer);

    // Initial position should be 0
    rs_ssize_t pos = rs_io_writer_tell(writer);
    TEST_ASSERT_EQUAL(0, pos);

    // Write 5 bytes
    rs_io_writer_write(writer, "Hello", 5);

    // Position should now be 5
    pos = rs_io_writer_tell(writer);
    TEST_ASSERT_EQUAL(5, pos);

    rs_io_writer_close(writer);
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_truncate_behavior(void)
{
    // Write initial data
    rs_io_write_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr("Long initial content"));

    // Open in truncate mode and write shorter content
    rs_io_write_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr("Short"));

    // Verify file was truncated
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, "Short"));
    TEST_ASSERT_EQUAL(5, rs_string_len(&content));
    rs_string_destroy(&content);
}

void test_append_behavior(void)
{
    // Write initial data
    rs_io_write_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr("Line 1\n"));

    // Append more data
    rs_io_append_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr("Line 2\n"));
    rs_io_append_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr("Line 3\n"));

    // Verify all lines present
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, "Line 1\nLine 2\nLine 3\n"));
    rs_string_destroy(&content);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // File Handle
    RUN_TEST(test_writer_open_close);
    RUN_TEST(test_writer_open_modes);
    RUN_TEST(test_writer_close_null);

    // Writing Operations
    RUN_TEST(test_write_all);
    RUN_TEST(test_write_str);
    RUN_TEST(test_append_all);
    RUN_TEST(test_append_str);
    RUN_TEST(test_writer_write);
    RUN_TEST(test_writer_write_exact);
    RUN_TEST(test_writer_flush);

    // Positional Writing
    RUN_TEST(test_pwrite);
    RUN_TEST(test_writer_write_at);

    // Seeking
    RUN_TEST(test_writer_seek_set);
    RUN_TEST(test_writer_seek_cur);
    RUN_TEST(test_writer_seek_end);
    RUN_TEST(test_writer_tell);

    // Integration
    RUN_TEST(test_truncate_behavior);
    RUN_TEST(test_append_behavior);

    return UNITY_END();
}
