#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/array.h>
#include <rs/std/error.h>
#include <rs/std/fs/dir.h>
#include <rs/std/fs/path.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_dir"

static rs_allocator_t *allocator;
static rs_string_t test_dir_path;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();
    test_dir_path = rs_string_create(.allocator = allocator);

    // Get temp directory and create test directory path
    rs_path_get_temp(&test_dir_path);
    rs_path_append(&test_dir_path, rs_sv_from_cstr("rs_test_dir"));

    // Clean up any existing test directory
    rs_dir_remove_all(rs_sv_from_string(&test_dir_path));
}

void tearDown(void)
{
    // Clean up test directory
    rs_dir_remove_all(rs_sv_from_string(&test_dir_path));
    rs_string_destroy(&test_dir_path);
    rs_log_shutdown();
}

// ============================================================================
// Directory Metadata Tests
// ============================================================================

void test_dir_exists(void)
{
    // Directory doesn't exist yet
    TEST_ASSERT_FALSE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));

    // Create directory
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);

    // Directory should exist now
    TEST_ASSERT_TRUE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));
}

void test_dir_is_dir(void)
{
    // Create directory
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);

    // Should be recognized as a directory
    TEST_ASSERT_TRUE(rs_dir_is_dir(rs_sv_from_string(&test_dir_path)));

    // Non-existent path should not be a directory
    TEST_ASSERT_FALSE(rs_dir_is_dir(rs_sv_from_cstr("/nonexistent_dir_12345")));
}

// ============================================================================
// Directory Operations Tests
// ============================================================================

void test_dir_create(void)
{
    rs_result_t result = rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Directory should exist
    TEST_ASSERT_TRUE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));
}

void test_dir_create_existing(void)
{
    // Create directory first
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);

    // Try to create again - should fail
    rs_result_t result = rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);
}

void test_dir_create_all(void)
{
    // Create nested directories
    rs_string_t nested = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                            rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&nested, rs_sv_from_cstr("level1"));
    rs_path_append(&nested, rs_sv_from_cstr("level2"));
    rs_path_append(&nested, rs_sv_from_cstr("level3"));

    rs_result_t result = rs_dir_create_all(rs_sv_from_string(&nested), 0755);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // All directories should exist
    TEST_ASSERT_TRUE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));

    rs_string_t level1 = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                            rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&level1, rs_sv_from_cstr("level1"));
    TEST_ASSERT_TRUE(rs_dir_exists(rs_sv_from_string(&level1)));

    TEST_ASSERT_TRUE(rs_dir_exists(rs_sv_from_string(&nested)));

    rs_string_destroy(&level1);
    rs_string_destroy(&nested);
}

void test_dir_create_all_existing(void)
{
    // Create directory first
    rs_dir_create_all(rs_sv_from_string(&test_dir_path), 0755);

    // Try to create again - should succeed (idempotent)
    rs_result_t result = rs_dir_create_all(rs_sv_from_string(&test_dir_path), 0755);
    TEST_ASSERT_EQUAL(RS_OK, result);
}

void test_dir_remove(void)
{
    // Create and then remove directory
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);
    TEST_ASSERT_TRUE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));

    rs_result_t result = rs_dir_remove(rs_sv_from_string(&test_dir_path));
    TEST_ASSERT_EQUAL(RS_OK, result);

    TEST_ASSERT_FALSE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));
}

void test_dir_remove_nonexistent(void)
{
    rs_result_t result = rs_dir_remove(rs_sv_from_cstr("/nonexistent_dir_12345"));
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);
}

void test_dir_remove_all(void)
{
    // Create directory structure with files
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);

    // Create subdirectory
    rs_string_t subdir = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                            rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&subdir, rs_sv_from_cstr("subdir"));
    rs_dir_create(rs_sv_from_string(&subdir), 0755);

    // Create some files
    rs_string_t file1 = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                           rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&file1, rs_sv_from_cstr("file1.txt"));

    rs_string_t file2 = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&subdir)),
                                           rs_sv_len(rs_sv_from_string(&subdir)), .allocator = allocator);
    rs_path_append(&file2, rs_sv_from_cstr("file2.txt"));

    FILE *fp1 = fopen(rs_string_cstr(&file1), "w");
    if (fp1) {
        fprintf(fp1, "test1");
        fclose(fp1);
    }

    FILE *fp2 = fopen(rs_string_cstr(&file2), "w");
    if (fp2) {
        fprintf(fp2, "test2");
        fclose(fp2);
    }

    // Remove all
    rs_result_t result = rs_dir_remove_all(rs_sv_from_string(&test_dir_path));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Everything should be gone
    TEST_ASSERT_FALSE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));
    TEST_ASSERT_FALSE(rs_path_exists(rs_sv_from_string(&file1)));
    TEST_ASSERT_FALSE(rs_path_exists(rs_sv_from_string(&file2)));

    rs_string_destroy(&subdir);
    rs_string_destroy(&file1);
    rs_string_destroy(&file2);
}

// ============================================================================
// Directory Reading Tests
// ============================================================================

void test_dir_read_empty(void)
{
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);

    rs_array_t entries = rs_array_create(sizeof(rs_string_t), .allocator = allocator);
    rs_result_t result = rs_dir_read(rs_sv_from_string(&test_dir_path), &entries);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(0, rs_array_len(&entries));

    rs_array_destroy(&entries);
}

void test_dir_read_with_files(void)
{
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);

    // Create some files
    rs_string_t file1 = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                           rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&file1, rs_sv_from_cstr("file1.txt"));

    rs_string_t file2 = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                           rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&file2, rs_sv_from_cstr("file2.txt"));

    rs_string_t subdir = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                            rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&subdir, rs_sv_from_cstr("subdir"));

    FILE *fp1 = fopen(rs_string_cstr(&file1), "w");
    if (fp1)
        fclose(fp1);

    FILE *fp2 = fopen(rs_string_cstr(&file2), "w");
    if (fp2)
        fclose(fp2);

    rs_dir_create(rs_sv_from_string(&subdir), 0755);

    // Read directory
    rs_array_t entries = rs_array_create(sizeof(rs_string_t), .allocator = allocator);
    rs_result_t result = rs_dir_read(rs_sv_from_string(&test_dir_path), &entries);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(3, rs_array_len(&entries));

    // Check that entries contain our files/dirs (order may vary)
    rs_bool found_file1 = false, found_file2 = false, found_subdir = false;
    for (rs_size_t i = 0; i < rs_array_len(&entries); i++) {
        rs_string_t *entry = (rs_string_t *)rs_array_get(&entries, i);
        if (rs_string_eq_cstr(entry, "file1.txt"))
            found_file1 = true;
        if (rs_string_eq_cstr(entry, "file2.txt"))
            found_file2 = true;
        if (rs_string_eq_cstr(entry, "subdir"))
            found_subdir = true;
        rs_string_destroy(entry);
    }

    TEST_ASSERT_TRUE(found_file1);
    TEST_ASSERT_TRUE(found_file2);
    TEST_ASSERT_TRUE(found_subdir);

    rs_array_destroy(&entries);
    rs_string_destroy(&file1);
    rs_string_destroy(&file2);
    rs_string_destroy(&subdir);
}

void test_dir_read_nonexistent(void)
{
    rs_array_t entries = rs_array_create(sizeof(rs_string_t), .allocator = allocator);
    rs_result_t result = rs_dir_read(rs_sv_from_cstr("/nonexistent_dir_12345"), &entries);

    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_array_destroy(&entries);
}

// ============================================================================
// Current Working Directory Tests
// ============================================================================

void test_dir_get_current(void)
{
    rs_string_t cwd = rs_string_create(.allocator = allocator);
    rs_result_t result = rs_dir_get_current(&cwd);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_len(&cwd) > 0);
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_string(&cwd)));

    rs_string_destroy(&cwd);
}

void test_dir_set_current(void)
{
    // Save original cwd
    rs_string_t original_cwd = rs_string_create(.allocator = allocator);
    rs_dir_get_current(&original_cwd);

    // Create test directory
    rs_dir_create(rs_sv_from_string(&test_dir_path), 0755);

    // Change to test directory
    rs_result_t result = rs_dir_set_current(rs_sv_from_string(&test_dir_path));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Verify we're in the new directory by checking if the cwd ends with our test dir name
    rs_string_t new_cwd = rs_string_create(.allocator = allocator);
    rs_dir_get_current(&new_cwd);

    // The new cwd should end with "rs_test_dir"
    TEST_ASSERT_TRUE(rs_string_ends_with(&new_cwd, "rs_test_dir"));

    // Restore original cwd
    rs_dir_set_current(rs_sv_from_string(&original_cwd));

    rs_string_destroy(&original_cwd);
    rs_string_destroy(&new_cwd);
}

void test_dir_set_current_nonexistent(void)
{
    rs_result_t result = rs_dir_set_current(rs_sv_from_cstr("/nonexistent_dir_12345"));
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_dir_integration(void)
{
    // Create nested structure
    rs_string_t nested = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                            rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&nested, rs_sv_from_cstr("a"));
    rs_path_append(&nested, rs_sv_from_cstr("b"));
    rs_path_append(&nested, rs_sv_from_cstr("c"));

    rs_dir_create_all(rs_sv_from_string(&nested), 0755);

    // Create files at different levels
    rs_string_t file_a = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                            rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&file_a, rs_sv_from_cstr("a"));
    rs_path_append(&file_a, rs_sv_from_cstr("file.txt"));

    rs_string_t file_c = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&nested)),
                                            rs_sv_len(rs_sv_from_string(&nested)), .allocator = allocator);
    rs_path_append(&file_c, rs_sv_from_cstr("deep.txt"));

    FILE *fp = fopen(rs_string_cstr(&file_a), "w");
    if (fp) {
        fprintf(fp, "level a");
        fclose(fp);
    }

    fp = fopen(rs_string_cstr(&file_c), "w");
    if (fp) {
        fprintf(fp, "level c");
        fclose(fp);
    }

    // Read directories
    rs_string_t dir_a = rs_string_from_buf(rs_sv_data(rs_sv_from_string(&test_dir_path)),
                                           rs_sv_len(rs_sv_from_string(&test_dir_path)), .allocator = allocator);
    rs_path_append(&dir_a, rs_sv_from_cstr("a"));

    rs_array_t entries = rs_array_create(sizeof(rs_string_t), .allocator = allocator);
    rs_dir_read(rs_sv_from_string(&dir_a), &entries);

    TEST_ASSERT_EQUAL(2, rs_array_len(&entries)); // "b" directory and "file.txt"

    for (rs_size_t i = 0; i < rs_array_len(&entries); i++) {
        rs_string_t *entry = (rs_string_t *)rs_array_get(&entries, i);
        rs_string_destroy(entry);
    }
    rs_array_destroy(&entries);

    // Remove all
    rs_dir_remove_all(rs_sv_from_string(&test_dir_path));
    TEST_ASSERT_FALSE(rs_dir_exists(rs_sv_from_string(&test_dir_path)));

    rs_string_destroy(&nested);
    rs_string_destroy(&file_a);
    rs_string_destroy(&file_c);
    rs_string_destroy(&dir_a);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Metadata
    RUN_TEST(test_dir_exists);
    RUN_TEST(test_dir_is_dir);

    // Operations
    RUN_TEST(test_dir_create);
    RUN_TEST(test_dir_create_existing);
    RUN_TEST(test_dir_create_all);
    RUN_TEST(test_dir_create_all_existing);
    RUN_TEST(test_dir_remove);
    RUN_TEST(test_dir_remove_nonexistent);
    RUN_TEST(test_dir_remove_all);

    // Reading
    RUN_TEST(test_dir_read_empty);
    RUN_TEST(test_dir_read_with_files);
    RUN_TEST(test_dir_read_nonexistent);

    // Current directory
    RUN_TEST(test_dir_get_current);
    RUN_TEST(test_dir_set_current);
    RUN_TEST(test_dir_set_current_nonexistent);

    // Integration
    RUN_TEST(test_dir_integration);

    return UNITY_END();
}
