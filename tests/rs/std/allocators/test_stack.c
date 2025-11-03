#include <rs/std/allocators/allocator.h>
#include <rs/std/allocators/arena.h>
#include <rs/std/allocators/stack.h>
#include <rs/std/logging/logging.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_stack"

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
}

void tearDown(void)
{
    rs_log_shutdown();
}

// Test basic stack creation and destruction
void test_stack_create_destroy(void)
{
    rs_stack_t *stack = rs_stack_create(1024);
    TEST_ASSERT_NOT_NULL(stack);

    rs_stack_destroy(stack);
}

// Test basic allocation
void test_stack_alloc(void)
{
    rs_stack_t *stack = rs_stack_create(1024);
    TEST_ASSERT_NOT_NULL(stack);

    rs_allocator_t *alloc = rs_stack_allocator(stack);

    // Allocate some memory
    int *x = (int *)rs_alloc(alloc, sizeof(int));
    TEST_ASSERT_NOT_NULL(x);
    *x = 42;
    TEST_ASSERT_EQUAL_INT(42, *x);

    // Allocate more
    char *str = (char *)rs_alloc(alloc, 100);
    TEST_ASSERT_NOT_NULL(str);
    strcpy(str, "Hello, Stack!");
    TEST_ASSERT_EQUAL_STRING("Hello, Stack!", str);

    rs_stack_destroy(stack);
}

// Test LIFO free order
void test_stack_lifo_free(void)
{
    rs_stack_t *stack = rs_stack_create(1024);
    TEST_ASSERT_NOT_NULL(stack);

    rs_allocator_t *alloc = rs_stack_allocator(stack);

    // Allocate in order: a, b, c
    int *a = (int *)rs_alloc(alloc, sizeof(int));
    int *b = (int *)rs_alloc(alloc, sizeof(int));
    int *c = (int *)rs_alloc(alloc, sizeof(int));

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    *a = 1;
    *b = 2;
    *c = 3;

    // Free in LIFO order: c, b, a
    rs_stack_free(stack, c);
    rs_stack_free(stack, b);
    rs_stack_free(stack, a);

    // Stack should be empty now
    TEST_ASSERT_EQUAL(0, rs_stack_used(stack));

    rs_stack_destroy(stack);
}

// Test stack markers
void test_stack_markers(void)
{
    rs_stack_t *stack = rs_stack_create(1024);
    TEST_ASSERT_NOT_NULL(stack);

    rs_allocator_t *alloc = rs_stack_allocator(stack);

    // Allocate some initial data
    int *a = (int *)rs_alloc(alloc, sizeof(int));
    *a = 1;

    // Save marker
    rs_stack_marker_t marker = rs_stack_mark(stack);

    // Allocate more data after marker
    int *b = (int *)rs_alloc(alloc, sizeof(int));
    int *c = (int *)rs_alloc(alloc, sizeof(int));
    *b = 2;
    *c = 3;

    rs_size_t used_before_free = rs_stack_used(stack);
    TEST_ASSERT_GREATER_THAN(sizeof(int), used_before_free);

    // Free back to marker (should free b and c)
    rs_stack_free_to_marker(stack, marker);

    // After freeing to marker, usage should be less
    rs_size_t used_after_free = rs_stack_used(stack);
    TEST_ASSERT_LESS_THAN(used_before_free, used_after_free);

    // 'a' should still have its value
    TEST_ASSERT_EQUAL_INT(1, *a);

    rs_stack_destroy(stack);
}

// Test stack reset
void test_stack_reset(void)
{
    rs_stack_t *stack = rs_stack_create(1024);
    TEST_ASSERT_NOT_NULL(stack);

    rs_allocator_t *alloc = rs_stack_allocator(stack);

    // Allocate some memory
    void *p1 = rs_alloc(alloc, 100);
    void *p2 = rs_alloc(alloc, 200);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);

    rs_size_t used_before = rs_stack_used(stack);
    TEST_ASSERT_GREATER_THAN(0, used_before);

    // Reset stack
    rs_stack_reset(stack);

    // After reset, usage should be 0
    TEST_ASSERT_EQUAL(0, rs_stack_used(stack));

    rs_stack_destroy(stack);
}

// Test capacity limits
void test_stack_capacity(void)
{
    rs_stack_t *stack = rs_stack_create(100); // Very small capacity
    TEST_ASSERT_NOT_NULL(stack);

    rs_allocator_t *alloc = rs_stack_allocator(stack);

    // Allocate within capacity should succeed
    void *p1 = rs_alloc(alloc, 50);
    TEST_ASSERT_NOT_NULL(p1);

    // Allocating beyond capacity should fail
    void *p2 = rs_alloc(alloc, 100); // Exceeds remaining capacity
    TEST_ASSERT_NULL(p2);

    rs_stack_destroy(stack);
}

// Test alignment
void test_stack_alignment(void)
{
    rs_stack_t *stack = rs_stack_create(1024);
    TEST_ASSERT_NOT_NULL(stack);

    rs_allocator_t *alloc = rs_stack_allocator(stack);

    // Allocate with different alignments
    void *p1 = rs_alloc_aligned(alloc, 100, 8);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_EQUAL(0, ((rs_uintptr)p1 % 8));

    void *p2 = rs_alloc_aligned(alloc, 100, 16);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_EQUAL(0, ((rs_uintptr)p2 % 16));

    void *p3 = rs_alloc_aligned(alloc, 100, 32);
    TEST_ASSERT_NOT_NULL(p3);
    TEST_ASSERT_EQUAL(0, ((rs_uintptr)p3 % 32));

    rs_stack_destroy(stack);
}

// Test nested allocator (stack with backing allocator)
void test_stack_nested(void)
{
    // Create parent arena
    rs_arena_t *arena = rs_arena_create(4096);
    TEST_ASSERT_NOT_NULL(arena);

    // Create stack using arena as backing allocator
    rs_stack_t *stack = rs_stack_create(2048, .allocator = rs_arena_allocator(arena));
    TEST_ASSERT_NOT_NULL(stack);

    rs_allocator_t *stack_alloc = rs_stack_allocator(stack);

    // Allocate from stack
    int *x = (int *)rs_alloc(stack_alloc, sizeof(int));
    TEST_ASSERT_NOT_NULL(x);
    *x = 123;
    TEST_ASSERT_EQUAL(123, *x);

    // Destroy stack (its memory comes from arena)
    rs_stack_destroy(stack);

    // Destroy arena
    rs_arena_destroy(arena);
}

// Test usage and capacity reporting
void test_stack_usage_capacity(void)
{
    rs_stack_t *stack = rs_stack_create(1024);
    TEST_ASSERT_NOT_NULL(stack);

    // Initial state
    TEST_ASSERT_EQUAL(1024, rs_stack_capacity(stack));
    TEST_ASSERT_EQUAL(0, rs_stack_used(stack));

    rs_allocator_t *alloc = rs_stack_allocator(stack);

    // Allocate some memory
    void *p1 = rs_alloc(alloc, 100);
    TEST_ASSERT_NOT_NULL(p1);

    rs_size_t used_after_first = rs_stack_used(stack);
    TEST_ASSERT_GREATER_OR_EQUAL(100, used_after_first);

    // Allocate more
    void *p2 = rs_alloc(alloc, 200);
    TEST_ASSERT_NOT_NULL(p2);

    rs_size_t used_after_second = rs_stack_used(stack);
    TEST_ASSERT_GREATER_THAN(used_after_first, used_after_second);

    // Capacity should remain constant
    TEST_ASSERT_EQUAL(1024, rs_stack_capacity(stack));

    rs_stack_destroy(stack);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_stack_create_destroy);
    RUN_TEST(test_stack_alloc);
    RUN_TEST(test_stack_lifo_free);
    RUN_TEST(test_stack_markers);
    RUN_TEST(test_stack_reset);
    RUN_TEST(test_stack_capacity);
    RUN_TEST(test_stack_alignment);
    RUN_TEST(test_stack_nested);
    RUN_TEST(test_stack_usage_capacity);

    return UNITY_END();
}
