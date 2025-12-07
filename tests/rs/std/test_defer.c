#include <rs/std/defer.h>
#include <rs/std/logging/logging.h>
#include <stdio.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_defer"

// ============================================================================
// Test Setup
// ============================================================================

// Global counter for tracking cleanup calls
static int cleanup_counter = 0;
static int cleanup_order[32];
static int cleanup_order_index = 0;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    cleanup_counter = 0;
    cleanup_order_index = 0;
    for (int i = 0; i < 32; i++) {
        cleanup_order[i] = 0;
    }
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Helper Functions
// ============================================================================

void increment_counter(void *data)
{
    (void)data;
    cleanup_counter++;
}

void record_order(void *data)
{
    int value = *(int *)data;
    cleanup_order[cleanup_order_index++] = value;
}

void set_flag(void *data)
{
    rs_bool *flag = (rs_bool *)data;
    *flag = true;
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

void test_defer_scope_init(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    TEST_ASSERT_EQUAL(0, scope.count);
}

void test_defer_scope_add(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    rs_result_t result = rs_defer_scope_add(&scope, increment_counter, NULL);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(1, scope.count);
}

void test_defer_scope_add_multiple(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    for (int i = 0; i < 5; i++) {
        rs_result_t result = rs_defer_scope_add(&scope, increment_counter, NULL);
        TEST_ASSERT_EQUAL(RS_OK, result);
    }

    TEST_ASSERT_EQUAL(5, scope.count);
}

void test_defer_scope_execute(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    rs_defer_scope_add(&scope, increment_counter, NULL);
    rs_defer_scope_add(&scope, increment_counter, NULL);
    rs_defer_scope_add(&scope, increment_counter, NULL);

    TEST_ASSERT_EQUAL(0, cleanup_counter);

    rs_defer_scope_execute(&scope);

    TEST_ASSERT_EQUAL(3, cleanup_counter);
    TEST_ASSERT_EQUAL(0, scope.count); // Should be cleared after execution
}

void test_defer_scope_execute_lifo_order(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    int values[] = {1, 2, 3, 4, 5};

    for (int i = 0; i < 5; i++) {
        rs_defer_scope_add(&scope, record_order, &values[i]);
    }

    rs_defer_scope_execute(&scope);

    // Should execute in reverse order (LIFO)
    TEST_ASSERT_EQUAL(5, cleanup_order[0]);
    TEST_ASSERT_EQUAL(4, cleanup_order[1]);
    TEST_ASSERT_EQUAL(3, cleanup_order[2]);
    TEST_ASSERT_EQUAL(2, cleanup_order[3]);
    TEST_ASSERT_EQUAL(1, cleanup_order[4]);
}

void test_defer_scope_clear(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    rs_defer_scope_add(&scope, increment_counter, NULL);
    rs_defer_scope_add(&scope, increment_counter, NULL);

    TEST_ASSERT_EQUAL(2, scope.count);

    rs_defer_scope_clear(&scope);

    TEST_ASSERT_EQUAL(0, scope.count);
    TEST_ASSERT_EQUAL(0, cleanup_counter); // Should not execute
}

void test_defer_scope_overflow(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    // Fill up the scope
    for (int i = 0; i < RS_DEFER_MAX_ACTIONS; i++) {
        rs_result_t result = rs_defer_scope_add(&scope, increment_counter, NULL);
        TEST_ASSERT_EQUAL(RS_OK, result);
    }

    // Try to add one more - should fail
    rs_result_t result = rs_defer_scope_add(&scope, increment_counter, NULL);
    TEST_ASSERT_EQUAL(RS_ERR_OVERFLOW, result);
}

// ============================================================================
// Macro Tests
// ============================================================================

void test_defer_scope_init_macro(void)
{
    rs_defer_scope_t scope = RS_DEFER_SCOPE_INIT();
    TEST_ASSERT_EQUAL(0, scope.count);
}

#if RS_HAS_CLEANUP_ATTRIBUTE
void test_defer_scoped_automatic_cleanup(void)
{
    rs_bool flag = false;

    {
        RS_SCOPED rs_defer_scope_t scope = RS_DEFER_SCOPE_INIT();
        rs_defer_scope_add(&scope, set_flag, &flag);

        TEST_ASSERT_FALSE(flag);
        // scope goes out of here, cleanup should be called
    }

    TEST_ASSERT_TRUE(flag);
}
#endif

void test_defer_scope_begin_end(void)
{
    rs_bool flag = false;

    RS_DEFER_SCOPE_BEGIN();
    RS_DEFER_ADD(set_flag, &flag);
    TEST_ASSERT_FALSE(flag);
    RS_DEFER_SCOPE_END();

    TEST_ASSERT_TRUE(flag);
}

// ============================================================================
// Practical Use Cases
// ============================================================================

void test_defer_memory_cleanup(void)
{
    RS_DEFER_SCOPE_BEGIN();
    char *ptr1 = malloc(100);
    TEST_ASSERT_NOT_NULL(ptr1);
    RS_DEFER_ADD(free, ptr1);

    char *ptr2 = malloc(200);
    TEST_ASSERT_NOT_NULL(ptr2);
    RS_DEFER_ADD(free, ptr2);

    // Use the memory
    ptr1[0] = 'A';
    ptr2[0] = 'B';

    // Memory will be freed automatically
    RS_DEFER_SCOPE_END();

    // If we got here without crashing, the test passed
    TEST_PASS();
}

void test_defer_file_cleanup(void)
{
    RS_DEFER_SCOPE_BEGIN();
    FILE *f = fopen("test_defer_temp.txt", "w");
    if (f) {
        RS_DEFER_ADD(fclose, f);
        fprintf(f, "test\n");
        // File will be closed automatically
    }
    RS_DEFER_SCOPE_END();

    // Verify file was written
    FILE *f = fopen("test_defer_temp.txt", "r");
    TEST_ASSERT_NOT_NULL(f);
    char buf[16];
    char *result = fgets(buf, sizeof(buf), f);
    TEST_ASSERT_NOT_NULL(result);
    fclose(f);
    remove("test_defer_temp.txt");

    TEST_ASSERT_EQUAL_STRING("test\n", buf);
}

// Helper function for test_defer_early_return
static rs_bool test_defer_flag1 = false;
static rs_bool test_defer_flag2 = false;

static rs_result_t simulate_early_return(rs_bool should_fail)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    rs_defer_scope_add(&scope, set_flag, &test_defer_flag1);

    if (should_fail) {
        rs_defer_scope_execute(&scope);
        return RS_ERR_INVALID;
    }

    rs_defer_scope_add(&scope, set_flag, &test_defer_flag2);
    rs_defer_scope_execute(&scope);
    return RS_OK;
}

void test_defer_early_return(void)
{
    test_defer_flag1 = false;
    test_defer_flag2 = false;

    // Test early return
    rs_result_t result = simulate_early_return(true);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, result);
    TEST_ASSERT_TRUE(test_defer_flag1);  // Should be cleaned up
    TEST_ASSERT_FALSE(test_defer_flag2); // Should not be added

    // Reset flags
    test_defer_flag1 = false;
    test_defer_flag2 = false;

    // Test normal path
    result = simulate_early_return(false);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(test_defer_flag1); // Should be cleaned up
    TEST_ASSERT_TRUE(test_defer_flag2); // Should be cleaned up
}

void test_defer_nested_scopes(void)
{
    int outer_value = 1;
    int inner_value = 2;

    RS_DEFER_SCOPE_BEGIN();
    RS_DEFER_ADD(record_order, &outer_value);

    RS_DEFER_SCOPE_BEGIN();
    RS_DEFER_ADD(record_order, &inner_value);
    RS_DEFER_SCOPE_END();

    // Inner scope should have executed
    TEST_ASSERT_EQUAL(2, cleanup_order[0]);
    RS_DEFER_SCOPE_END();

    // Outer scope should execute after
    TEST_ASSERT_EQUAL(1, cleanup_order[1]);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void test_defer_null_scope(void)
{
    rs_result_t result = rs_defer_scope_add(NULL, increment_counter, NULL);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, result);

    rs_defer_scope_execute(NULL); // Should not crash
    rs_defer_scope_clear(NULL);   // Should not crash
}

void test_defer_null_function(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    rs_result_t result = rs_defer_scope_add(&scope, NULL, NULL);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, result);
}

// ============================================================================
// Helper Wrapper Tests
// ============================================================================

void test_defer_free_helper(void)
{
    rs_defer_scope_t scope;
    rs_defer_scope_init(&scope);

    char *ptr = malloc(100);
    TEST_ASSERT_NOT_NULL(ptr);

    rs_result_t result = rs_defer_free(&scope, ptr);
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_defer_scope_execute(&scope);
    // If we got here without crashing, free was called successfully
    TEST_PASS();
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Basic functionality
    RUN_TEST(test_defer_scope_init);
    RUN_TEST(test_defer_scope_add);
    RUN_TEST(test_defer_scope_add_multiple);
    RUN_TEST(test_defer_scope_execute);
    RUN_TEST(test_defer_scope_execute_lifo_order);
    RUN_TEST(test_defer_scope_clear);
    RUN_TEST(test_defer_scope_overflow);

    // Macro tests
    RUN_TEST(test_defer_scope_init_macro);
#if RS_HAS_CLEANUP_ATTRIBUTE
    RUN_TEST(test_defer_scoped_automatic_cleanup);
#endif
    RUN_TEST(test_defer_scope_begin_end);

    // Practical use cases
    RUN_TEST(test_defer_memory_cleanup);
    RUN_TEST(test_defer_file_cleanup);
    RUN_TEST(test_defer_early_return);
    RUN_TEST(test_defer_nested_scopes);

    // Error handling
    RUN_TEST(test_defer_null_scope);
    RUN_TEST(test_defer_null_function);

    // Helper wrappers
    RUN_TEST(test_defer_free_helper);

    return UNITY_END();
}
