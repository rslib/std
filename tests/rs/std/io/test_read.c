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

#define RS_STD_LOG_MODULE "test_read"

static rs_allocator_t *allocator;
static rs_string_t test_file_path;
static const char *test_content = "Hello, World!\nThis is a test file.\n";

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();
    test_file_path = rs_string_create(.allocator = allocator);

    // Get temp directory and create test file path
    rs_path_get_temp(&test_file_path);
    rs_path_append(&test_file_path, rs_sv_from_cstr("rs_test_read.txt"));

    // Create test file
    rs_io_write_str(rs_sv_from_string(&test_file_path), rs_sv_from_cstr(test_content));
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

void test_reader_open_close(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    rs_io_reader_close(reader);
}

void test_reader_open_nonexistent(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_cstr("/nonexistent_file_12345.txt"));
    TEST_ASSERT_NULL(reader);
}

void test_reader_close_null(void)
{
    // Should not crash
    rs_io_reader_close(NULL);
}

// ============================================================================
// Reading Operations Tests
// ============================================================================

void test_read_all(void)
{
    rs_string_t content = rs_string_create(.allocator = allocator);

    rs_result_t result = rs_io_read_all(rs_sv_from_string(&test_file_path), &content);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, test_content));

    rs_string_destroy(&content);
}

void test_read_all_nonexistent(void)
{
    rs_string_t content = rs_string_create(.allocator = allocator);

    rs_result_t result = rs_io_read_all(rs_sv_from_cstr("/nonexistent_file_12345.txt"), &content);
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_string_destroy(&content);
}

void test_reader_read(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    char buf[256];
    rs_ssize_t n = rs_io_reader_read(reader, buf, sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(n <= (rs_ssize_t)sizeof(buf));

    // Verify content
    buf[n] = '\0';
    TEST_ASSERT_EQUAL_STRING(test_content, buf);

    rs_io_reader_close(reader);
}

void test_reader_read_chunks(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    rs_string_t content = rs_string_create(.allocator = allocator);
    char buf[8]; // Small buffer to force multiple reads
    rs_ssize_t n;

    while ((n = rs_io_reader_read(reader, buf, sizeof(buf))) > 0) {
        rs_string_push_buf(&content, buf, n);
    }

    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, test_content));

    rs_string_destroy(&content);
    rs_io_reader_close(reader);
}

void test_reader_read_exact(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    char buf[13]; // "Hello, World!"
    rs_result_t result = rs_io_reader_read_exact(reader, buf, 13);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL_MEMORY("Hello, World!", buf, 13);

    rs_io_reader_close(reader);
}

void test_reader_read_exact_eof(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    char buf[1024];
    rs_result_t result = rs_io_reader_read_exact(reader, buf, 1024);
    TEST_ASSERT_EQUAL(RS_ERR_EOF, result); // File is smaller than 1024 bytes

    rs_io_reader_close(reader);
}

// ============================================================================
// Positional Reading Tests
// ============================================================================

void test_pread(void)
{
    char buf[6];
    rs_ssize_t n = rs_io_pread(rs_sv_from_string(&test_file_path), buf, 5, 7); // Read "World" at offset 7
    TEST_ASSERT_EQUAL(5, n);
    buf[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("World", buf);
}

void test_reader_read_at(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    // Read "World" at offset 7
    char buf[6];
    rs_ssize_t n = rs_io_reader_read_at(reader, buf, 5, 7);
    TEST_ASSERT_EQUAL(5, n);
    buf[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("World", buf);

    // Position should not have changed - read from beginning
    char buf2[6];
    n = rs_io_reader_read(reader, buf2, 5);
    TEST_ASSERT_EQUAL(5, n);
    buf2[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("Hello", buf2);

    rs_io_reader_close(reader);
}

// ============================================================================
// Seeking Tests
// ============================================================================

void test_reader_seek_set(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    // Seek to offset 7 ("World")
    rs_ssize_t pos = rs_io_reader_seek(reader, 7, RS_IO_SEEK_SET);
    TEST_ASSERT_EQUAL(7, pos);

    char buf[6];
    rs_ssize_t n = rs_io_reader_read(reader, buf, 5);
    TEST_ASSERT_EQUAL(5, n);
    buf[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("World", buf);

    rs_io_reader_close(reader);
}

void test_reader_seek_cur(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    // Read "Hello"
    char buf[6];
    rs_io_reader_read(reader, buf, 5);

    // Seek forward 2 bytes from current position (skip ", ")
    rs_ssize_t pos = rs_io_reader_seek(reader, 2, RS_IO_SEEK_CUR);
    TEST_ASSERT_EQUAL(7, pos);

    rs_ssize_t n = rs_io_reader_read(reader, buf, 5);
    TEST_ASSERT_EQUAL(5, n);
    buf[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("World", buf);

    rs_io_reader_close(reader);
}

void test_reader_seek_end(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    // Seek to 5 bytes before end
    rs_ssize_t pos = rs_io_reader_seek(reader, -5, RS_IO_SEEK_END);
    TEST_ASSERT_TRUE(pos > 0);

    char buf[6];
    rs_ssize_t n = rs_io_reader_read(reader, buf, 5);
    TEST_ASSERT_EQUAL(5, n);
    buf[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("ile.\n", buf);

    rs_io_reader_close(reader);
}

void test_reader_tell(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    // Initial position should be 0
    rs_ssize_t pos = rs_io_reader_tell(reader);
    TEST_ASSERT_EQUAL(0, pos);

    // Read 5 bytes
    char buf[5];
    rs_io_reader_read(reader, buf, 5);

    // Position should now be 5
    pos = rs_io_reader_tell(reader);
    TEST_ASSERT_EQUAL(5, pos);

    rs_io_reader_close(reader);
}

void test_reader_size(void)
{
    rs_io_reader_t *reader = rs_io_reader_open(rs_sv_from_string(&test_file_path));
    TEST_ASSERT_NOT_NULL(reader);

    rs_ssize_t size = rs_io_reader_size(reader);
    TEST_ASSERT_EQUAL(strlen(test_content), size);

    rs_io_reader_close(reader);
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_read_write_integration(void)
{
    const char *temp_file = "/tmp/rs_test_integration.txt";
    const char *data = "Integration test data\nLine 2\nLine 3\n";

    // Write data
    rs_result_t result = rs_io_write_str(rs_sv_from_cstr(temp_file), rs_sv_from_cstr(data));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Read it back
    rs_string_t content = rs_string_create(.allocator = allocator);
    result = rs_io_read_all(rs_sv_from_cstr(temp_file), &content);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&content, data));

    rs_string_destroy(&content);
    remove(temp_file);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // File Handle
    RUN_TEST(test_reader_open_close);
    RUN_TEST(test_reader_open_nonexistent);
    RUN_TEST(test_reader_close_null);

    // Reading Operations
    RUN_TEST(test_read_all);
    RUN_TEST(test_read_all_nonexistent);
    RUN_TEST(test_reader_read);
    RUN_TEST(test_reader_read_chunks);
    RUN_TEST(test_reader_read_exact);
    RUN_TEST(test_reader_read_exact_eof);

    // Positional Reading
    RUN_TEST(test_pread);
    RUN_TEST(test_reader_read_at);

    // Seeking
    RUN_TEST(test_reader_seek_set);
    RUN_TEST(test_reader_seek_cur);
    RUN_TEST(test_reader_seek_end);
    RUN_TEST(test_reader_tell);
    RUN_TEST(test_reader_size);

    // Integration
    RUN_TEST(test_read_write_integration);

    return UNITY_END();
}
