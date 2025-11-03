#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/array.h>
#include <rs/std/error.h>
#include <rs/std/fs/file.h>
#include <rs/std/fs/path.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_file"

static rs_allocator_t *allocator;
static rs_string_t test_file_path;
static rs_string_t test_file_path2;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();
    test_file_path = rs_string_create(.allocator = allocator);
    test_file_path2 = rs_string_create(.allocator = allocator);

    // Get temp directory and create test file paths
    rs_path_get_temp(&test_file_path);
    rs_path_append(&test_file_path, rs_sv_from_cstr("rs_test_file.txt"));

    rs_path_get_temp(&test_file_path2);
    rs_path_append(&test_file_path2, rs_sv_from_cstr("rs_test_file2.txt"));

    // Clean up any existing test files
    rs_path_remove(rs_sv_from_string(test_file_path));
    rs_path_remove(rs_sv_from_string(test_file_path2));
}

void tearDown(void)
{
    // Clean up test files
    rs_path_remove(rs_sv_from_string(test_file_path));
    rs_path_remove(rs_sv_from_string(test_file_path2));
    rs_string_destroy(&test_file_path);
    rs_string_destroy(&test_file_path2);
    rs_log_shutdown();
}

// ============================================================================
// File Metadata Tests
// ============================================================================

void test_file_exists(void)
{
    // File doesn't exist yet
    TEST_ASSERT_FALSE(rs_file_exists(rs_sv_from_string(test_file_path)));

    // Create file
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr("test"));

    // File should exist now
    TEST_ASSERT_TRUE(rs_file_exists(rs_sv_from_string(test_file_path)));
}

void test_file_is_file(void)
{
    // Create file
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr("test"));

    // Should be recognized as a file
    TEST_ASSERT_TRUE(rs_file_is_file(rs_sv_from_string(test_file_path)));

    // Non-existent path should not be a file
    TEST_ASSERT_FALSE(rs_file_is_file(rs_sv_from_cstr("/nonexistent_file_12345.txt")));
}

void test_file_size(void)
{
    const char *content = "Hello, World!";
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(content));

    rs_ssize_t size = rs_file_size(rs_sv_from_string(test_file_path));
    TEST_ASSERT_EQUAL(strlen(content), size);
}

void test_file_size_nonexistent(void)
{
    rs_ssize_t size = rs_file_size(rs_sv_from_cstr("/nonexistent_file_12345.txt"));
    TEST_ASSERT_EQUAL(-1, size);
}

// ============================================================================
// Reading Tests
// ============================================================================

void test_file_read(void)
{
    const char *content = "Hello, World!\nThis is a test.";
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(content));

    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_result_t res = rs_file_read(rs_sv_from_string(test_file_path), &result);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, content));

    rs_string_destroy(&result);
}

void test_file_read_nonexistent(void)
{
    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_result_t res = rs_file_read(rs_sv_from_cstr("/nonexistent_file_12345.txt"), &result);

    TEST_ASSERT_NOT_EQUAL(RS_OK, res);

    rs_string_destroy(&result);
}

void test_file_read_lines(void)
{
    const char *content = "Line 1\nLine 2\nLine 3";
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(content));

    rs_array_t lines = rs_array_create(sizeof(rs_string_t), .allocator = allocator);
    rs_result_t res = rs_file_read_lines(rs_sv_from_string(test_file_path), &lines);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(3, rs_array_len(&lines));

    rs_string_t *line1 = (rs_string_t *)rs_array_get(&lines, 0);
    rs_string_t *line2 = (rs_string_t *)rs_array_get(&lines, 1);
    rs_string_t *line3 = (rs_string_t *)rs_array_get(&lines, 2);

    TEST_ASSERT_TRUE(rs_string_eq_cstr(line1, "Line 1"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(line2, "Line 2"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(line3, "Line 3"));

    // Clean up strings in array
    for (rs_size_t i = 0; i < rs_array_len(&lines); i++) {
        rs_string_t *line = (rs_string_t *)rs_array_get(&lines, i);
        rs_string_destroy(line);
    }
    rs_array_destroy(&lines);
}

void test_file_read_lines_crlf(void)
{
    const char *content = "Line 1\r\nLine 2\r\nLine 3\r\n";
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(content));

    rs_array_t lines = rs_array_create(sizeof(rs_string_t), .allocator = allocator);
    rs_result_t res = rs_file_read_lines(rs_sv_from_string(test_file_path), &lines);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(3, rs_array_len(&lines));

    rs_string_t *line1 = (rs_string_t *)rs_array_get(&lines, 0);
    rs_string_t *line2 = (rs_string_t *)rs_array_get(&lines, 1);
    rs_string_t *line3 = (rs_string_t *)rs_array_get(&lines, 2);

    TEST_ASSERT_TRUE(rs_string_eq_cstr(line1, "Line 1"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(line2, "Line 2"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(line3, "Line 3"));

    // Clean up strings in array
    for (rs_size_t i = 0; i < rs_array_len(&lines); i++) {
        rs_string_t *line = (rs_string_t *)rs_array_get(&lines, i);
        rs_string_destroy(line);
    }
    rs_array_destroy(&lines);
}

void test_file_read_lines_empty(void)
{
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(""));

    rs_array_t lines = rs_array_create(sizeof(rs_string_t), .allocator = allocator);
    rs_result_t res = rs_file_read_lines(rs_sv_from_string(test_file_path), &lines);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(0, rs_array_len(&lines));

    rs_array_destroy(&lines);
}

// ============================================================================
// Writing Tests
// ============================================================================

void test_file_write(void)
{
    const char *content = "Test content";
    rs_result_t res = rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(content));

    TEST_ASSERT_EQUAL(RS_OK, res);

    // Verify by reading back
    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_file_read(rs_sv_from_string(test_file_path), &result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, content));
    rs_string_destroy(&result);
}

void test_file_write_truncate(void)
{
    // Write initial content
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr("Long initial content"));

    // Overwrite with shorter content
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr("Short"));

    // Verify file was truncated
    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_file_read(rs_sv_from_string(test_file_path), &result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "Short"));
    rs_string_destroy(&result);
}

void test_file_append(void)
{
    // Write initial content
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr("Line 1\n"));

    // Append more content
    rs_result_t res = rs_file_append(rs_sv_from_string(test_file_path), rs_sv_from_cstr("Line 2\n"));
    TEST_ASSERT_EQUAL(RS_OK, res);

    // Verify both lines present
    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_file_read(rs_sv_from_string(test_file_path), &result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "Line 1\nLine 2\n"));
    rs_string_destroy(&result);
}

// ============================================================================
// File Operations Tests
// ============================================================================

void test_file_copy(void)
{
    const char *content = "File to copy";
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(content));

    rs_result_t res = rs_file_copy(rs_sv_from_string(test_file_path), rs_sv_from_string(test_file_path2));
    TEST_ASSERT_EQUAL(RS_OK, res);

    // Verify destination exists and has same content
    TEST_ASSERT_TRUE(rs_file_exists(rs_sv_from_string(test_file_path2)));

    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_file_read(rs_sv_from_string(test_file_path2), &result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, content));
    rs_string_destroy(&result);

    // Verify source still exists
    TEST_ASSERT_TRUE(rs_file_exists(rs_sv_from_string(test_file_path)));
}

void test_file_copy_overwrite(void)
{
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr("Source content"));
    rs_file_write(rs_sv_from_string(test_file_path2), rs_sv_from_cstr("Old destination content"));

    rs_result_t res = rs_file_copy(rs_sv_from_string(test_file_path), rs_sv_from_string(test_file_path2));
    TEST_ASSERT_EQUAL(RS_OK, res);

    // Verify destination has source content
    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_file_read(rs_sv_from_string(test_file_path2), &result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "Source content"));
    rs_string_destroy(&result);
}

void test_file_move(void)
{
    const char *content = "File to move";
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr(content));

    rs_result_t res = rs_file_move(rs_sv_from_string(test_file_path), rs_sv_from_string(test_file_path2));
    TEST_ASSERT_EQUAL(RS_OK, res);

    // Verify destination exists and has content
    TEST_ASSERT_TRUE(rs_file_exists(rs_sv_from_string(test_file_path2)));

    rs_string_t result = rs_string_create(.allocator = allocator);
    rs_file_read(rs_sv_from_string(test_file_path2), &result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, content));
    rs_string_destroy(&result);

    // Verify source no longer exists
    TEST_ASSERT_FALSE(rs_file_exists(rs_sv_from_string(test_file_path)));
}

void test_file_remove(void)
{
    rs_file_write(rs_sv_from_string(test_file_path), rs_sv_from_cstr("File to remove"));
    TEST_ASSERT_TRUE(rs_file_exists(rs_sv_from_string(test_file_path)));

    rs_result_t res = rs_file_remove(rs_sv_from_string(test_file_path));
    TEST_ASSERT_EQUAL(RS_OK, res);

    TEST_ASSERT_FALSE(rs_file_exists(rs_sv_from_string(test_file_path)));
}

void test_file_remove_nonexistent(void)
{
    rs_result_t res = rs_file_remove(rs_sv_from_cstr("/nonexistent_file_12345.txt"));
    TEST_ASSERT_NOT_EQUAL(RS_OK, res);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Metadata
    RUN_TEST(test_file_exists);
    RUN_TEST(test_file_is_file);
    RUN_TEST(test_file_size);
    RUN_TEST(test_file_size_nonexistent);

    // Reading
    RUN_TEST(test_file_read);
    RUN_TEST(test_file_read_nonexistent);
    RUN_TEST(test_file_read_lines);
    RUN_TEST(test_file_read_lines_crlf);
    RUN_TEST(test_file_read_lines_empty);

    // Writing
    RUN_TEST(test_file_write);
    RUN_TEST(test_file_write_truncate);
    RUN_TEST(test_file_append);

    // Operations
    RUN_TEST(test_file_copy);
    RUN_TEST(test_file_copy_overwrite);
    RUN_TEST(test_file_move);
    RUN_TEST(test_file_remove);
    RUN_TEST(test_file_remove_nonexistent);

    return UNITY_END();
}
