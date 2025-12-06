#include <rs/std/allocators/allocator.h>
#include <rs/std/error.h>
#include <rs/std/fs/path.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_path"

static rs_allocator_t *allocator;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Path Creators Tests
// ============================================================================

void test_path_get_home(void)
{
    rs_string_t path = rs_string_create(.allocator = allocator);

    rs_result_t result = rs_path_get_home(&path);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_len(&path) > 0);

    // Should be absolute path
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_string(&path)));

    rs_string_destroy(&path);
}

void test_path_expand_tilde(void)
{
    rs_string_t path = rs_string_create(.allocator = allocator);

    // Test "~"
    rs_result_t result = rs_path_expand(&path, rs_sv_from_cstr("~"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_len(&path) > 0);
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_string(&path)));

    // Test "~/config"
    result = rs_path_expand(&path, rs_sv_from_cstr("~/.config"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_ends_with(&path, ".config"));

    rs_string_destroy(&path);
}

void test_path_expand_env_var(void)
{
    rs_string_t path = rs_string_create(.allocator = allocator);

    // Set a test environment variable
    setenv("TEST_VAR", "/test/path", 1);

    // Test "$TEST_VAR"
    rs_result_t result = rs_path_expand(&path, rs_sv_from_cstr("$TEST_VAR/file"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_starts_with(&path, "/test/path"));
    TEST_ASSERT_TRUE(rs_string_ends_with(&path, "file"));

    // Test "${TEST_VAR}"
    result = rs_path_expand(&path, rs_sv_from_cstr("${TEST_VAR}/other"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_starts_with(&path, "/test/path"));
    TEST_ASSERT_TRUE(rs_string_ends_with(&path, "other"));

    unsetenv("TEST_VAR");
    rs_string_destroy(&path);
}

void test_path_dirname(void)
{
    rs_string_t result = rs_string_create(.allocator = allocator);

    // Test "/foo/bar/baz" -> "/foo/bar"
    rs_path_dirname(&result, rs_sv_from_cstr("/foo/bar/baz"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "/foo/bar"));

    // Test "/foo" -> "/"
    rs_path_dirname(&result, rs_sv_from_cstr("/foo"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "/"));

    // Test "foo/bar" -> "foo"
    rs_path_dirname(&result, rs_sv_from_cstr("foo/bar"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "foo"));

    // Test "foo" -> "."
    rs_path_dirname(&result, rs_sv_from_cstr("foo"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "."));

    rs_string_destroy(&result);
}

void test_path_basename(void)
{
    rs_string_t result = rs_string_create(.allocator = allocator);

    // Test "/foo/bar/baz" -> "baz"
    rs_path_basename(&result, rs_sv_from_cstr("/foo/bar/baz"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "baz"));

    // Test "/foo/" -> "foo"
    rs_path_basename(&result, rs_sv_from_cstr("/foo/"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "foo"));

    // Test "foo" -> "foo"
    rs_path_basename(&result, rs_sv_from_cstr("foo"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "foo"));

    // Test "/" -> "/"
    rs_path_basename(&result, rs_sv_from_cstr("/"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "/"));

    rs_string_destroy(&result);
}

// ============================================================================
// Path Modifiers Tests
// ============================================================================

void test_path_append(void)
{
    rs_string_t path = rs_string_create(.allocator = allocator);

    // Test basic append
    rs_string_push_cstr(&path, "/foo");
    rs_path_append(&path, rs_sv_from_cstr("bar"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/foo/bar"));

    // Test append with trailing slash
    rs_string_clear(&path);
    rs_string_push_cstr(&path, "/foo/");
    rs_path_append(&path, rs_sv_from_cstr("bar"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/foo/bar"));

    // Test append with leading slash in component
    rs_string_clear(&path);
    rs_string_push_cstr(&path, "/foo");
    rs_path_append(&path, rs_sv_from_cstr("/bar"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/bar")); // Absolute component replaces

    // Test multiple appends
    rs_string_clear(&path);
    rs_string_push_cstr(&path, "/usr");
    rs_path_append(&path, rs_sv_from_cstr("local"));
    rs_path_append(&path, rs_sv_from_cstr("bin"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/usr/local/bin"));

    rs_string_destroy(&path);
}

void test_path_normalize(void)
{
    rs_string_t path = rs_string_create(.allocator = allocator);

    // Test removing double slashes
    rs_string_push_cstr(&path, "/foo//bar");
    rs_path_normalize(&path);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/foo/bar"));

    // Test removing "."
    rs_string_clear(&path);
    rs_string_push_cstr(&path, "/foo/./bar");
    rs_path_normalize(&path);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/foo/bar"));

    // Test resolving ".."
    rs_string_clear(&path);
    rs_string_push_cstr(&path, "/foo/bar/../baz");
    rs_path_normalize(&path);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/foo/baz"));

    // Test relative path with ".."
    rs_string_clear(&path);
    rs_string_push_cstr(&path, "foo/../bar");
    rs_path_normalize(&path);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "bar"));

    // Test complex path
    rs_string_clear(&path);
    rs_string_push_cstr(&path, "/foo/./bar/../baz//qux");
    rs_path_normalize(&path);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&path, "/foo/baz/qux"));

    rs_string_destroy(&path);
}

// ============================================================================
// Path Queries Tests
// ============================================================================

void test_path_is_absolute(void)
{
    // Unix absolute paths
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_cstr("/")));
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_cstr("/foo")));
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_cstr("/foo/bar")));

    // Relative paths
    TEST_ASSERT_FALSE(rs_path_is_absolute(rs_sv_from_cstr("foo")));
    TEST_ASSERT_FALSE(rs_path_is_absolute(rs_sv_from_cstr("foo/bar")));
    TEST_ASSERT_FALSE(rs_path_is_absolute(rs_sv_from_cstr("./foo")));
    TEST_ASSERT_FALSE(rs_path_is_absolute(rs_sv_from_cstr("../foo")));

    // Empty path
    TEST_ASSERT_FALSE(rs_path_is_absolute(rs_sv_from_cstr("")));
}

void test_path_extension(void)
{
    rs_string_view_t ext;

    // Test "foo.txt" -> ".txt"
    ext = rs_path_extension(rs_sv_from_cstr("foo.txt"));
    TEST_ASSERT_TRUE(rs_sv_eq_cstr(ext, ".txt"));

    // Test "foo.tar.gz" -> ".gz"
    ext = rs_path_extension(rs_sv_from_cstr("foo.tar.gz"));
    TEST_ASSERT_TRUE(rs_sv_eq_cstr(ext, ".gz"));

    // Test "foo" -> ""
    ext = rs_path_extension(rs_sv_from_cstr("foo"));
    TEST_ASSERT_TRUE(rs_sv_is_empty(ext));

    // Test ".vimrc" -> "" (hidden file, no extension)
    ext = rs_path_extension(rs_sv_from_cstr(".vimrc"));
    TEST_ASSERT_TRUE(rs_sv_is_empty(ext));

    // Test "/path/to/file.txt" -> ".txt"
    ext = rs_path_extension(rs_sv_from_cstr("/path/to/file.txt"));
    TEST_ASSERT_TRUE(rs_sv_eq_cstr(ext, ".txt"));

    // Test "/path.with.dots/file" -> ""
    ext = rs_path_extension(rs_sv_from_cstr("/path.with.dots/file"));
    TEST_ASSERT_TRUE(rs_sv_is_empty(ext));
}

void test_path_has_extension(void)
{
    // Test matching extensions
    TEST_ASSERT_TRUE(rs_path_has_extension(rs_sv_from_cstr("foo.txt"), ".txt"));
    TEST_ASSERT_TRUE(rs_path_has_extension(rs_sv_from_cstr("foo.tar.gz"), ".gz"));
    TEST_ASSERT_TRUE(rs_path_has_extension(rs_sv_from_cstr("/path/to/file.c"), ".c"));

    // Test non-matching extensions
    TEST_ASSERT_FALSE(rs_path_has_extension(rs_sv_from_cstr("foo.txt"), ".c"));
    TEST_ASSERT_FALSE(rs_path_has_extension(rs_sv_from_cstr("foo"), ".txt"));
    TEST_ASSERT_FALSE(rs_path_has_extension(rs_sv_from_cstr(".vimrc"), ".txt"));
}

void test_path_separator(void)
{
    char sep = rs_path_separator();

#ifdef _WIN32
    TEST_ASSERT_EQUAL('\\', sep);
#else
    TEST_ASSERT_EQUAL('/', sep);
#endif
}

void test_path_list_separator(void)
{
    char sep = rs_path_list_separator();

#ifdef _WIN32
    TEST_ASSERT_EQUAL(';', sep);
#else
    TEST_ASSERT_EQUAL(':', sep);
#endif
}

void test_path_exists(void)
{
    // Test with root directory (should always exist)
    TEST_ASSERT_EQUAL(1, rs_path_exists(rs_sv_from_cstr("/")));

    // Test with likely non-existent path
    TEST_ASSERT_EQUAL(0, rs_path_exists(rs_sv_from_cstr("/nonexistent_path_12345")));

    // Test with current directory
    TEST_ASSERT_EQUAL(1, rs_path_exists(rs_sv_from_cstr(".")));
}

void test_path_relative(void)
{
    rs_string_t result = rs_string_create(.allocator = allocator);

    // Test: from="/foo/bar", to="/foo/baz" -> "../baz"
    rs_path_relative(&result, rs_sv_from_cstr("/foo/bar"), rs_sv_from_cstr("/foo/baz"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "../baz"));

    // Test: from="/foo/bar", to="/foo/bar/baz" -> "baz"
    rs_path_relative(&result, rs_sv_from_cstr("/foo/bar"), rs_sv_from_cstr("/foo/bar/baz"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "baz"));

    // Test: from="/foo", to="/bar" -> "../bar"
    rs_path_relative(&result, rs_sv_from_cstr("/foo"), rs_sv_from_cstr("/bar"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "../bar"));

    // Test: same path -> "."
    rs_path_relative(&result, rs_sv_from_cstr("/foo/bar"), rs_sv_from_cstr("/foo/bar"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "."));

    // Test: nested paths
    rs_path_relative(&result, rs_sv_from_cstr("/a/b/c/d"), rs_sv_from_cstr("/a/b/e/f"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "../../e/f"));

    // Test: relative paths
    rs_path_relative(&result, rs_sv_from_cstr("foo/bar"), rs_sv_from_cstr("foo/baz"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "../baz"));

    rs_string_destroy(&result);
}

void test_path_proximate(void)
{
    rs_string_t result = rs_string_create(.allocator = allocator);

    // Test: short relative path preferred
    rs_path_proximate(&result, rs_sv_from_cstr("/foo/bar"), rs_sv_from_cstr("/foo/baz"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "../baz"));

    // Test: short absolute path preferred (when relative would be longer)
    rs_path_proximate(&result, rs_sv_from_cstr("/a/b/c/d/e/f"), rs_sv_from_cstr("/x"));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "/x"));

    // Test: equal length - prefer relative
    rs_path_proximate(&result, rs_sv_from_cstr("/foo"), rs_sv_from_cstr("/bar"));
    // Both "../bar" (6 chars) and "/bar" (4 chars) - absolute is shorter
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&result, "/bar"));

    rs_string_destroy(&result);
}

// ============================================================================
// Filesystem Operations Tests
// ============================================================================

void test_path_is_file(void)
{
    rs_string_t test_file = rs_string_create(.allocator = allocator);
    rs_path_get_temp(&test_file);
    rs_path_append(&test_file, rs_sv_from_cstr("rs_test_is_file.txt"));

    // File doesn't exist yet
    TEST_ASSERT_EQUAL(0, rs_path_is_file(rs_sv_from_string(&test_file)));

    // Create file
    FILE *fp = fopen(rs_string_cstr(&test_file), "w");
    if (fp) {
        fprintf(fp, "test");
        fclose(fp);
    }

    // Should be recognized as a file
    TEST_ASSERT_NOT_EQUAL(0, rs_path_is_file(rs_sv_from_string(&test_file)));

    // Clean up
    rs_path_remove(rs_sv_from_string(&test_file));
    rs_string_destroy(&test_file);
}

void test_path_is_dir(void)
{
    // Root directory should be a directory
    TEST_ASSERT_NOT_EQUAL(0, rs_path_is_dir(rs_sv_from_cstr("/")));

    // Current directory should be a directory
    TEST_ASSERT_NOT_EQUAL(0, rs_path_is_dir(rs_sv_from_cstr(".")));

    // Non-existent path should not be a directory
    TEST_ASSERT_EQUAL(0, rs_path_is_dir(rs_sv_from_cstr("/nonexistent_dir_12345")));
}

void test_path_remove_file(void)
{
    rs_string_t test_file = rs_string_create(.allocator = allocator);
    rs_path_get_temp(&test_file);
    rs_path_append(&test_file, rs_sv_from_cstr("rs_test_remove.txt"));

    // Create file
    FILE *fp = fopen(rs_string_cstr(&test_file), "w");
    if (fp) {
        fprintf(fp, "test");
        fclose(fp);
    }

    TEST_ASSERT_NOT_EQUAL(0, rs_path_exists(rs_sv_from_string(&test_file)));

    // Remove file
    rs_result_t result = rs_path_remove(rs_sv_from_string(&test_file));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // File should no longer exist
    TEST_ASSERT_EQUAL(0, rs_path_exists(rs_sv_from_string(&test_file)));

    rs_string_destroy(&test_file);
}

void test_path_remove_nonexistent(void)
{
    rs_result_t result = rs_path_remove(rs_sv_from_cstr("/nonexistent_file_12345.txt"));
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);
}

#ifndef _WIN32
void test_path_symlink(void)
{
    rs_string_t target = rs_string_create(.allocator = allocator);
    rs_string_t link = rs_string_create(.allocator = allocator);

    rs_path_get_temp(&target);
    rs_path_append(&target, rs_sv_from_cstr("rs_test_symlink_target.txt"));

    rs_path_get_temp(&link);
    rs_path_append(&link, rs_sv_from_cstr("rs_test_symlink.txt"));

    // Create target file
    FILE *fp = fopen(rs_string_cstr(&target), "w");
    if (fp) {
        fprintf(fp, "target content");
        fclose(fp);
    }

    // Create symlink
    rs_result_t result = rs_path_symlink(rs_sv_from_string(&target), rs_sv_from_string(&link));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Link should exist
    TEST_ASSERT_NOT_EQUAL(0, rs_path_exists(rs_sv_from_string(&link)));
    TEST_ASSERT_NOT_EQUAL(0, rs_path_is_symlink(rs_sv_from_string(&link)));

    // Clean up
    rs_path_remove(rs_sv_from_string(&link));
    rs_path_remove(rs_sv_from_string(&target));

    rs_string_destroy(&target);
    rs_string_destroy(&link);
}

void test_path_read_symlink(void)
{
    rs_string_t target = rs_string_create(.allocator = allocator);
    rs_string_t link = rs_string_create(.allocator = allocator);
    rs_string_t result = rs_string_create(.allocator = allocator);

    rs_path_get_temp(&target);
    rs_path_append(&target, rs_sv_from_cstr("rs_test_read_symlink_target.txt"));

    rs_path_get_temp(&link);
    rs_path_append(&link, rs_sv_from_cstr("rs_test_read_symlink.txt"));

    // Create target file
    FILE *fp = fopen(rs_string_cstr(&target), "w");
    if (fp) {
        fprintf(fp, "target content");
        fclose(fp);
    }

    // Create symlink
    rs_path_symlink(rs_sv_from_string(&target), rs_sv_from_string(&link));

    // Read symlink
    rs_result_t res = rs_path_read_symlink(rs_sv_from_string(&link), &result);
    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_TRUE(rs_string_eq(&result, &target));

    // Clean up
    rs_path_remove(rs_sv_from_string(&link));
    rs_path_remove(rs_sv_from_string(&target));

    rs_string_destroy(&target);
    rs_string_destroy(&link);
    rs_string_destroy(&result);
}

void test_path_is_symlink(void)
{
    rs_string_t target = rs_string_create(.allocator = allocator);
    rs_string_t link = rs_string_create(.allocator = allocator);

    rs_path_get_temp(&target);
    rs_path_append(&target, rs_sv_from_cstr("rs_test_is_symlink_target.txt"));

    rs_path_get_temp(&link);
    rs_path_append(&link, rs_sv_from_cstr("rs_test_is_symlink.txt"));

    // Create target file
    FILE *fp = fopen(rs_string_cstr(&target), "w");
    if (fp) {
        fprintf(fp, "target content");
        fclose(fp);
    }

    // Target should not be a symlink
    TEST_ASSERT_EQUAL(0, rs_path_is_symlink(rs_sv_from_string(&target)));

    // Create symlink
    rs_path_symlink(rs_sv_from_string(&target), rs_sv_from_string(&link));

    // Link should be a symlink
    TEST_ASSERT_NOT_EQUAL(0, rs_path_is_symlink(rs_sv_from_string(&link)));

    // Clean up
    rs_path_remove(rs_sv_from_string(&link));
    rs_path_remove(rs_sv_from_string(&target));

    rs_string_destroy(&target);
    rs_string_destroy(&link);
}
#endif

// ============================================================================
// Integration Tests
// ============================================================================

void test_path_integration(void)
{
    rs_string_t path = rs_string_create(.allocator = allocator);

    // Build a complex path: home/.config/nvim/init.lua
    rs_path_get_home(&path);
    rs_path_append(&path, rs_sv_from_cstr(".config"));
    rs_path_append(&path, rs_sv_from_cstr("nvim"));
    rs_path_append(&path, rs_sv_from_cstr("init.lua"));

    // Check it's absolute
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_string(&path)));

    // Check it ends with correct parts
    TEST_ASSERT_TRUE(rs_string_ends_with(&path, "init.lua"));
    TEST_ASSERT_TRUE(rs_path_has_extension(rs_sv_from_string(&path), ".lua"));

    // Get basename
    rs_string_t basename = rs_string_create(.allocator = allocator);
    rs_path_basename(&basename, rs_sv_from_string(&path));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&basename, "init.lua"));

    // Get dirname
    rs_string_t dirname = rs_string_create(.allocator = allocator);
    rs_path_dirname(&dirname, rs_sv_from_string(&path));
    TEST_ASSERT_TRUE(rs_string_ends_with(&dirname, "nvim"));

    rs_string_destroy(&path);
    rs_string_destroy(&basename);
    rs_string_destroy(&dirname);
}

void test_path_normalize_integration(void)
{
    rs_string_t path = rs_string_create(.allocator = allocator);

    // Expand a path with tilde and normalize it
    rs_path_expand(&path, rs_sv_from_cstr("~/./../foo//bar/./baz"));
    rs_path_normalize(&path);

    // Should be absolute and normalized
    TEST_ASSERT_TRUE(rs_path_is_absolute(rs_sv_from_string(&path)));
    TEST_ASSERT_TRUE(rs_string_ends_with(&path, "baz"));

    rs_string_destroy(&path);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Path Creators
    RUN_TEST(test_path_get_home);
    RUN_TEST(test_path_expand_tilde);
    RUN_TEST(test_path_expand_env_var);
    RUN_TEST(test_path_dirname);
    RUN_TEST(test_path_basename);

    // Path Modifiers
    RUN_TEST(test_path_append);
    RUN_TEST(test_path_normalize);

    // Path Queries
    RUN_TEST(test_path_is_absolute);
    RUN_TEST(test_path_extension);
    RUN_TEST(test_path_has_extension);
    RUN_TEST(test_path_separator);
    RUN_TEST(test_path_list_separator);
    RUN_TEST(test_path_exists);
    RUN_TEST(test_path_relative);
    RUN_TEST(test_path_proximate);

    // Filesystem Operations
    RUN_TEST(test_path_is_file);
    RUN_TEST(test_path_is_dir);
    RUN_TEST(test_path_remove_file);
    RUN_TEST(test_path_remove_nonexistent);
#ifndef _WIN32
    RUN_TEST(test_path_symlink);
    RUN_TEST(test_path_read_symlink);
    RUN_TEST(test_path_is_symlink);
#endif

    // Integration
    RUN_TEST(test_path_integration);
    RUN_TEST(test_path_normalize_integration);

    return UNITY_END();
}
