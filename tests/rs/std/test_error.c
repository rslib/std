#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/types.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_error"

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    // Clear error state before each test
    rs_clear_error();
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Basic error functionality
// ============================================================================

void test_no_error_initially(void)
{
    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NULL(err);
}

void test_clear_error(void)
{
    RS_ERROR(RS_ERR_NOMEM, "Test error");
    TEST_ASSERT_NOT_NULL(rs_last_error());

    rs_clear_error();
    TEST_ASSERT_NULL(rs_last_error());
}

void test_error_string(void)
{
    TEST_ASSERT_EQUAL_STRING("Success", rs_error_string(RS_OK));
    TEST_ASSERT_EQUAL_STRING("Out of memory", rs_error_string(RS_ERR_NOMEM));
    TEST_ASSERT_EQUAL_STRING("Invalid argument", rs_error_string(RS_ERR_INVALID));
    TEST_ASSERT_EQUAL_STRING("I/O error", rs_error_string(RS_ERR_IO));
    TEST_ASSERT_EQUAL_STRING("Not found", rs_error_string(RS_ERR_NOTFOUND));
}

// ============================================================================
// Error context
// ============================================================================

void test_error_with_message(void)
{
    RS_ERROR(RS_ERR_NOMEM, "Failed to allocate %d bytes", 1024);

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_INT(RS_ERR_NOMEM, err->code);
    TEST_ASSERT_EQUAL_STRING("Failed to allocate 1024 bytes", err->message);
    TEST_ASSERT_NOT_NULL(err->file);
    TEST_ASSERT_TRUE(err->line > 0);
}

void test_error_without_custom_message(void)
{
    RS_ERROR(RS_ERR_INVALID, "");

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_INT(RS_ERR_INVALID, err->code);
    // Should fall back to default error string
    TEST_ASSERT_EQUAL_STRING("Invalid argument", err->message);
}

void test_error_file_and_line(void)
{
    RS_ERROR(RS_ERR_IO, "Test");

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_NOT_NULL(err->file);
    TEST_ASSERT_TRUE(strstr(err->file, "test_error.c") != NULL);
    TEST_ASSERT_TRUE(err->line > 0);
}

// ============================================================================
// Error macros
// ============================================================================

static rs_result_t helper_function_that_fails(void)
{
    return RS_ERROR_RET(RS_ERR_NOTFOUND, "Item not found");
}

void test_error_ret_macro(void)
{
    rs_result_t result = helper_function_that_fails();

    TEST_ASSERT_EQUAL_INT(RS_ERR_NOTFOUND, result);

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_INT(RS_ERR_NOTFOUND, err->code);
    TEST_ASSERT_EQUAL_STRING("Item not found", err->message);
}

static rs_result_t helper_with_check(int value)
{
    RS_CHECK(value > 0, RS_ERR_INVALID, "Value must be positive, got %d", value);
    return RS_OK;
}

void test_check_macro_success(void)
{
    rs_result_t result = helper_with_check(5);
    TEST_ASSERT_EQUAL_INT(RS_OK, result);
    TEST_ASSERT_NULL(rs_last_error());
}

void test_check_macro_failure(void)
{
    rs_result_t result = helper_with_check(-1);
    TEST_ASSERT_EQUAL_INT(RS_ERR_INVALID, result);

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_STRING("Value must be positive, got -1", err->message);
}

static rs_result_t helper_that_calls_failing_function(void)
{
    RS_TRY(helper_function_that_fails());
    return RS_OK;
}

void test_try_macro_propagates_error(void)
{
    rs_result_t result = helper_that_calls_failing_function();

    TEST_ASSERT_EQUAL_INT(RS_ERR_NOTFOUND, result);

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_INT(RS_ERR_NOTFOUND, err->code);
}

static rs_result_t helper_that_succeeds(void)
{
    return RS_OK;
}

static rs_result_t helper_that_tries_success(void)
{
    RS_TRY(helper_that_succeeds());
    return RS_OK;
}

void test_try_macro_continues_on_success(void)
{
    rs_result_t result = helper_that_tries_success();
    TEST_ASSERT_EQUAL_INT(RS_OK, result);
    TEST_ASSERT_NULL(rs_last_error());
}

// ============================================================================
// Multiple errors
// ============================================================================

void test_error_overwrites_previous(void)
{
    RS_ERROR(RS_ERR_NOMEM, "First error");
    RS_ERROR(RS_ERR_IO, "Second error");

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_INT(RS_ERR_IO, err->code);
    TEST_ASSERT_EQUAL_STRING("Second error", err->message);
}

// ============================================================================
// Long messages
// ============================================================================

void test_long_message_truncated(void)
{
    char long_msg[512];
    memset(long_msg, 'A', sizeof(long_msg) - 1);
    long_msg[sizeof(long_msg) - 1] = '\0';

    RS_ERROR(RS_ERR_SYSTEM, "%s", long_msg);

    const rs_error_t *err = rs_last_error();
    TEST_ASSERT_NOT_NULL(err);
    // Message should be truncated to fit in 256 bytes
    TEST_ASSERT_TRUE(strlen(err->message) < 256);
}

// ============================================================================
// All error codes
// ============================================================================

void test_all_error_codes(void)
{
    rs_result_t codes[] = {
        RS_ERR_NOMEM,  RS_ERR_IO,     RS_ERR_NOTFOUND, RS_ERR_INVALID,
        RS_ERR_SYSTEM, RS_ERR_CRYPTO, RS_ERR_DB,       RS_ERR_OVERFLOW,
    };

    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        rs_clear_error();
        RS_ERROR(codes[i], "Test error %d", (int)i);

        const rs_error_t *err = rs_last_error();
        TEST_ASSERT_NOT_NULL(err);
        TEST_ASSERT_EQUAL_INT(codes[i], err->code);
        TEST_ASSERT_NOT_NULL(rs_error_string(codes[i]));
    }
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(test_no_error_initially);
    RUN_TEST(test_clear_error);
    RUN_TEST(test_error_string);

    // Error context
    RUN_TEST(test_error_with_message);
    RUN_TEST(test_error_without_custom_message);
    RUN_TEST(test_error_file_and_line);

    // Macros
    RUN_TEST(test_error_ret_macro);
    RUN_TEST(test_check_macro_success);
    RUN_TEST(test_check_macro_failure);
    RUN_TEST(test_try_macro_propagates_error);
    RUN_TEST(test_try_macro_continues_on_success);

    // Edge cases
    RUN_TEST(test_error_overwrites_previous);
    RUN_TEST(test_long_message_truncated);
    RUN_TEST(test_all_error_codes);

    return UNITY_END();
}
