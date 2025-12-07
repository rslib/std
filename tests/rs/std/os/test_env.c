#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/array.h>
#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/os/env.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_env"

static rs_allocator_t *allocator;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();

    // Clean up test environment variables
    rs_env_unset("RS_TEST_VAR");
    rs_env_unset("RS_TEST_VAR_2");
}

void tearDown(void)
{
    // Clean up test environment variables
    rs_env_unset("RS_TEST_VAR");
    rs_env_unset("RS_TEST_VAR_2");
    rs_log_shutdown();
}

// ============================================================================
// Basic Operations Tests
// ============================================================================

void test_env_get_existing(void)
{
    rs_string_t value = rs_string_create(.allocator = allocator);

    // Test with PATH (should always exist)
    rs_result_t result = rs_env_get(&value, "PATH");
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_len(&value) > 0);

    rs_string_destroy(&value);
}

void test_env_get_nonexistent(void)
{
    rs_string_t value = rs_string_create(.allocator = allocator);

    // Test with a variable that shouldn't exist
    rs_result_t result = rs_env_get(&value, "RS_NONEXISTENT_VAR_12345");
    TEST_ASSERT_EQUAL(RS_ERR_NOTFOUND, result);
    TEST_ASSERT_EQUAL(0, rs_string_len(&value));

    rs_string_destroy(&value);
}

void test_env_exists(void)
{
    // Test with PATH (should always exist)
    TEST_ASSERT_TRUE(rs_env_exists("PATH"));

    // Test with nonexistent variable
    TEST_ASSERT_FALSE(rs_env_exists("RS_NONEXISTENT_VAR_12345"));
}

void test_env_set_and_get(void)
{
    rs_string_t value = rs_string_create(.allocator = allocator);

    // Set a test variable
    rs_result_t result = rs_env_set("RS_TEST_VAR", "test_value");
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Verify it exists
    TEST_ASSERT_TRUE(rs_env_exists("RS_TEST_VAR"));

    // Get and verify value
    result = rs_env_get(&value, "RS_TEST_VAR");
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&value, "test_value"));

    rs_string_destroy(&value);
}

void test_env_set_overwrite(void)
{
    rs_string_t value = rs_string_create(.allocator = allocator);

    // Set initial value
    rs_env_set("RS_TEST_VAR", "initial");

    // Overwrite with new value
    rs_result_t result = rs_env_set("RS_TEST_VAR", "overwritten");
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Verify new value
    result = rs_env_get(&value, "RS_TEST_VAR");
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&value, "overwritten"));

    rs_string_destroy(&value);
}

void test_env_unset(void)
{
    // Set a test variable
    rs_env_set("RS_TEST_VAR", "test_value");
    TEST_ASSERT_TRUE(rs_env_exists("RS_TEST_VAR"));

    // Unset it
    rs_result_t result = rs_env_unset("RS_TEST_VAR");
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Verify it no longer exists
    TEST_ASSERT_FALSE(rs_env_exists("RS_TEST_VAR"));
}

void test_env_unset_nonexistent(void)
{
    // Unsetting a nonexistent variable should succeed (no-op)
    rs_result_t result = rs_env_unset("RS_NONEXISTENT_VAR_12345");
    TEST_ASSERT_EQUAL(RS_OK, result);
}

void test_env_expand_simple(void)
{
    rs_string_t expanded = rs_string_create(.allocator = allocator);

    // Set test variable
    rs_env_set("RS_TEST_VAR", "hello");

    // Test $VAR syntax
    rs_result_t result = rs_env_expand(&expanded, rs_sv_from_cstr("$RS_TEST_VAR world"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "hello world"));

    rs_string_destroy(&expanded);
}

void test_env_expand_braced(void)
{
    rs_string_t expanded = rs_string_create(.allocator = allocator);

    // Set test variable
    rs_env_set("RS_TEST_VAR", "test");

    // Test ${VAR} syntax
    rs_result_t result = rs_env_expand(&expanded, rs_sv_from_cstr("${RS_TEST_VAR}_value"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "test_value"));

    rs_string_destroy(&expanded);
}

void test_env_expand_multiple(void)
{
    rs_string_t expanded = rs_string_create(.allocator = allocator);

    // Set test variables
    rs_env_set("RS_TEST_VAR", "foo");
    rs_env_set("RS_TEST_VAR_2", "bar");

    // Test multiple expansions
    rs_result_t result = rs_env_expand(&expanded, rs_sv_from_cstr("$RS_TEST_VAR and ${RS_TEST_VAR_2}"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "foo and bar"));

    rs_string_destroy(&expanded);
}

void test_env_expand_nonexistent(void)
{
    rs_string_t expanded = rs_string_create(.allocator = allocator);

    // Expand nonexistent variable (should be replaced with empty string)
    rs_result_t result = rs_env_expand(&expanded, rs_sv_from_cstr("before $RS_NONEXISTENT_VAR_12345 after"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "before  after"));

    rs_string_destroy(&expanded);
}

void test_env_expand_no_vars(void)
{
    rs_string_t expanded = rs_string_create(.allocator = allocator);

    // Expand string with no variables
    rs_result_t result = rs_env_expand(&expanded, rs_sv_from_cstr("plain text"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "plain text"));

    rs_string_destroy(&expanded);
}

void test_env_expand_dollar_no_var(void)
{
    rs_string_t expanded = rs_string_create(.allocator = allocator);

    // Dollar sign not followed by valid variable name
    rs_result_t result = rs_env_expand(&expanded, rs_sv_from_cstr("Price: $5.00"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    // The '5' is treated as a variable name, but doesn't exist, so becomes empty
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "Price: .00"));

    rs_string_destroy(&expanded);
}

// ============================================================================
// Advanced Operations Tests
// ============================================================================

void test_env_get_view(void)
{
    // Set test variable
    rs_env_set("RS_TEST_VAR", "test_value");

    // Get view
    rs_string_view_t view = rs_env_get_view("RS_TEST_VAR");

#ifndef _WIN32
    // On Unix, should return valid view
    TEST_ASSERT_FALSE(rs_sv_is_empty(view));
    TEST_ASSERT_TRUE(rs_sv_eq_cstr(view, "test_value"));
#else
    // On Windows, get_view returns empty (not safe to return view)
    TEST_ASSERT_TRUE(rs_sv_is_empty(view));
#endif
}

void test_env_get_view_nonexistent(void)
{
    rs_string_view_t view = rs_env_get_view("RS_NONEXISTENT_VAR_12345");
    TEST_ASSERT_TRUE(rs_sv_is_empty(view));
}

// Helper for foreach test
typedef struct {
    int count;
    int found_path;
    int found_test_var;
} foreach_context_t;

static void foreach_callback(const char *name, const char *value, void *userdata)
{
    foreach_context_t *ctx = (foreach_context_t *)userdata;
    ctx->count++;

    // Windows env var names are case-insensitive
#ifdef _WIN32
    if (_stricmp(name, "PATH") == 0) {
#else
    if (strcmp(name, "PATH") == 0) {
#endif
        ctx->found_path = 1;
    }
    if (strcmp(name, "RS_TEST_VAR") == 0 && strcmp(value, "foreach_test") == 0) {
        ctx->found_test_var = 1;
    }
}

void test_env_foreach(void)
{
    // Set test variable
    rs_env_set("RS_TEST_VAR", "foreach_test");

    foreach_context_t ctx = {0, 0, 0};
    rs_env_foreach(foreach_callback, &ctx);

    // Should have found multiple variables
    TEST_ASSERT_TRUE(ctx.count > 0);

    // Should have found PATH
    TEST_ASSERT_TRUE(ctx.found_path);

    // Should have found our test variable
    TEST_ASSERT_TRUE(ctx.found_test_var);
}

void test_env_get_all(void)
{
    rs_array_t pairs = rs_array_create(sizeof(rs_env_pair_t), .allocator = allocator);

    // Set test variable
    rs_env_set("RS_TEST_VAR", "get_all_test");

    rs_result_t result = rs_env_get_all(&pairs, allocator);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should have multiple pairs
    rs_size_t count = rs_array_len(&pairs);
    TEST_ASSERT_TRUE(count > 0);

    // Find PATH and our test variable
    int found_path = 0;
    int found_test_var = 0;

    for (rs_size_t i = 0; i < count; i++) {
        rs_env_pair_t *pair = (rs_env_pair_t *)rs_array_get(&pairs, i);

        // Windows env var names are case-insensitive
#ifdef _WIN32
        if (_stricmp(rs_string_cstr(&pair->name), "PATH") == 0) {
#else
        if (rs_string_eq_cstr(&pair->name, "PATH")) {
#endif
            found_path = 1;
            TEST_ASSERT_TRUE(rs_string_len(&pair->value) > 0);
        }

        if (rs_string_eq_cstr(&pair->name, "RS_TEST_VAR")) {
            found_test_var = 1;
            TEST_ASSERT_TRUE(rs_string_eq_cstr(&pair->value, "get_all_test"));
        }

        // Cleanup strings
        rs_string_destroy(&pair->name);
        rs_string_destroy(&pair->value);
    }

    TEST_ASSERT_TRUE(found_path);
    TEST_ASSERT_TRUE(found_test_var);

    rs_array_destroy(&pairs);
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_env_integration_workflow(void)
{
    rs_string_t value = rs_string_create(.allocator = allocator);
    rs_string_t expanded = rs_string_create(.allocator = allocator);

    // Set multiple variables
    rs_env_set("RS_TEST_VAR", "/usr/local");
    rs_env_set("RS_TEST_VAR_2", "config");

    // Verify they exist
    TEST_ASSERT_TRUE(rs_env_exists("RS_TEST_VAR"));
    TEST_ASSERT_TRUE(rs_env_exists("RS_TEST_VAR_2"));

    // Expand a path using both
    rs_result_t result = rs_env_expand(&expanded, rs_sv_from_cstr("$RS_TEST_VAR/${RS_TEST_VAR_2}/app.conf"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "/usr/local/config/app.conf"));

    // Modify one
    rs_env_set("RS_TEST_VAR", "/opt");

    // Re-expand
    rs_string_clear(&expanded);
    result = rs_env_expand(&expanded, rs_sv_from_cstr("$RS_TEST_VAR/${RS_TEST_VAR_2}/app.conf"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "/opt/config/app.conf"));

    // Unset and verify expansion changes
    rs_env_unset("RS_TEST_VAR");
    rs_string_clear(&expanded);
    result = rs_env_expand(&expanded, rs_sv_from_cstr("$RS_TEST_VAR/${RS_TEST_VAR_2}/app.conf"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&expanded, "/config/app.conf"));

    rs_string_destroy(&value);
    rs_string_destroy(&expanded);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Basic Operations
    RUN_TEST(test_env_get_existing);
    RUN_TEST(test_env_get_nonexistent);
    RUN_TEST(test_env_exists);
    RUN_TEST(test_env_set_and_get);
    RUN_TEST(test_env_set_overwrite);
    RUN_TEST(test_env_unset);
    RUN_TEST(test_env_unset_nonexistent);
    RUN_TEST(test_env_expand_simple);
    RUN_TEST(test_env_expand_braced);
    RUN_TEST(test_env_expand_multiple);
    RUN_TEST(test_env_expand_nonexistent);
    RUN_TEST(test_env_expand_no_vars);
    RUN_TEST(test_env_expand_dollar_no_var);

    // Advanced Operations
    RUN_TEST(test_env_get_view);
    RUN_TEST(test_env_get_view_nonexistent);
    RUN_TEST(test_env_foreach);
    RUN_TEST(test_env_get_all);

    // Integration
    RUN_TEST(test_env_integration_workflow);

    return UNITY_END();
}
