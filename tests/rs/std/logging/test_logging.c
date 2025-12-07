#include <rs/std/allocators/allocator.h>
#include <rs/std/fs/path.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/types.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test"

void setUp(void)
{
    // Initialize logging
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Basic logging functionality
// ============================================================================

void test_log_init(void)
{
    rs_result_t result = rs_log_init();
    TEST_ASSERT_EQUAL(RS_OK, result);
}

void test_log_levels(void)
{
    rs_log_set_level(RS_STD_LOG_DEBUG);
    TEST_ASSERT_EQUAL(RS_STD_LOG_DEBUG, rs_log_get_level());

    rs_log_set_level(RS_STD_LOG_INFO);
    TEST_ASSERT_EQUAL(RS_STD_LOG_INFO, rs_log_get_level());

    rs_log_set_level(RS_STD_LOG_ERROR);
    TEST_ASSERT_EQUAL(RS_STD_LOG_ERROR, rs_log_get_level());
}

void test_log_module(void)
{
    rs_log_set_module("my_module");
    TEST_ASSERT_EQUAL_STRING("my_module", rs_log_get_module());

    rs_log_set_module(NULL);
    TEST_ASSERT_NULL(rs_log_get_module());
}

void test_basic_logging(void)
{
    // These should not crash
    RS_STD_LOG_INFO("Basic info message");
    RS_STD_LOG_DEBUG("Debug message with number: %d", 42);
    RS_STD_LOG_WARN("Warning message");
    RS_STD_LOG_ERROR("Error message: %s", "test error");
}

void test_flush(void)
{
    RS_STD_LOG_INFO("Message before flush");
    rs_result_t result = rs_log_flush();
    TEST_ASSERT_EQUAL(RS_OK, result);
}

// ============================================================================
// File output
// ============================================================================

void test_log_to_file(void)
{
    // Use portable temp path
    rs_allocator_t *allocator = rs_allocator_system();
    rs_string_t test_file = rs_string_create(.allocator = allocator);
    rs_path_get_temp(&test_file);
    rs_path_append(&test_file, rs_sv_from_cstr("rs_test_log.txt"));

    rs_result_t result = rs_log_set_output_path(rs_string_cstr(&test_file), false, true);
    TEST_ASSERT_EQUAL(RS_OK, result);

    RS_STD_LOG_INFO("Test message to file");
    RS_STD_LOG_DEBUG("Another test message: %d", 123);

    rs_log_flush();

    // Close the log file by setting output back to stderr
    rs_log_set_output_file(stderr, false);

    // Verify file exists by trying to open it
    FILE *f = fopen(rs_string_cstr(&test_file), "r");
    TEST_ASSERT_NOT_NULL(f);

    // Read first line
    char line[256];
    char *read_result = fgets(line, sizeof(line), f);
    TEST_ASSERT_NOT_NULL(read_result);
    TEST_ASSERT_TRUE(strlen(line) > 0);

    fclose(f);
    rs_path_remove(rs_sv_from_string(&test_file));
    rs_string_destroy(&test_file);
}

// ============================================================================
// Trace logging
// ============================================================================

static void traced_function(int x, int y)
{
    RS_UNUSED(x);
    RS_UNUSED(y);
    RS_TRACE_SCOPE_FMT("x=%d, y=%d", x, y);
    RS_STD_LOG_DEBUG("Inside traced function: %d + %d = %d", x, y, x + y);
}

static void simple_traced_function(void)
{
    RS_TRACE_SCOPE();
    RS_STD_LOG_DEBUG("Inside simple traced function");
}

void test_trace_logging(void)
{
    rs_log_set_level(RS_STD_LOG_TRACE);

    // These should not crash
    traced_function(10, 20);
    simple_traced_function();
}

// ============================================================================
// Custom output handler
// ============================================================================

static int custom_output_called = 0;
static rs_log_level_t last_level = RS_STD_LOG_OFF;

static void custom_output_handler(rs_log_level_t level, const char *module, const char *file, int line,
                                  const char *func, const char *message, void *userdata)
{
    (void)module;
    (void)file;
    (void)line;
    (void)func;
    (void)message;
    (void)userdata;

    custom_output_called++;
    last_level = level;
}

void test_custom_output(void)
{
    custom_output_called = 0;
    last_level = RS_STD_LOG_OFF;

    rs_log_set_output_handler(custom_output_handler, NULL);

    RS_STD_LOG_INFO("Test message");

    TEST_ASSERT_EQUAL(1, custom_output_called);
    TEST_ASSERT_EQUAL(RS_STD_LOG_INFO, last_level);

    // Restore default handler
    rs_log_set_output_handler(NULL, NULL);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_log_init);
    RUN_TEST(test_log_levels);
    RUN_TEST(test_log_module);
    RUN_TEST(test_basic_logging);
    RUN_TEST(test_flush);
    RUN_TEST(test_log_to_file);
    RUN_TEST(test_trace_logging);
    RUN_TEST(test_custom_output);

    return UNITY_END();
}
