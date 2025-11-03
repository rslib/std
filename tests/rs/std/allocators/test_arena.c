#include <rs/std/allocators/allocator.h>
#include <rs/std/allocators/arena.h>
#include <rs/std/logging/logging.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_arena"

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
}

void tearDown(void)
{
    rs_log_shutdown();
}

// Test basic arena creation and destruction
void test_arena_create_destroy(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    TEST_ASSERT_NOT_NULL(arena);

    rs_arena_destroy(arena);
}

// Test basic allocation
void test_arena_alloc(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    TEST_ASSERT_NOT_NULL(arena);

    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Allocate some memory
    int *x = (int *)rs_alloc(alloc, sizeof(int));
    TEST_ASSERT_NOT_NULL(x);
    *x = 42;
    TEST_ASSERT_EQUAL_INT(42, *x);

    // Allocate more
    char *str = (char *)rs_alloc(alloc, 100);
    TEST_ASSERT_NOT_NULL(str);
    strcpy(str, "Hello, Arena!");
    TEST_ASSERT_EQUAL_STRING("Hello, Arena!", str);

    rs_arena_destroy(arena);
}

// Test multiple allocations
void test_arena_multiple_allocs(void)
{

    rs_arena_t *arena = rs_arena_create(64); // Small initial size
    TEST_ASSERT_NOT_NULL(arena);

    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Allocate many small blocks
    for (int i = 0; i < 100; i++) {
        int *p = (int *)rs_alloc(alloc, sizeof(int));
        TEST_ASSERT_NOT_NULL(p);
        *p = i;
        TEST_ASSERT_EQUAL(i, *p);
    }

    rs_arena_destroy(arena);
}

// Test arena growth (exceeding initial capacity)
void test_arena_growth(void)
{

    rs_arena_t *arena = rs_arena_create(64);
    TEST_ASSERT_NOT_NULL(arena);

    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Allocate large block that exceeds initial capacity
    char *large = (char *)rs_alloc(alloc, 1024);
    TEST_ASSERT_NOT_NULL(large);
    memset(large, 'A', 1024);

    // Verify data
    for (int i = 0; i < 1024; i++) {
        TEST_ASSERT_EQUAL('A', large[i]);
    }

    rs_arena_destroy(arena);
}

// Test arena reset
void test_arena_reset(void)
{

    rs_arena_t *arena = rs_arena_create(1024);
    TEST_ASSERT_NOT_NULL(arena);

    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Allocate some memory
    void *p1 = rs_alloc(alloc, 100);
    TEST_ASSERT_NOT_NULL(p1);

    // Reset arena
    rs_arena_reset(arena);

    // Allocate again - should reuse memory
    void *p2 = rs_alloc(alloc, 100);
    TEST_ASSERT_NOT_NULL(p2);

    // After reset, we should get the same address (reused memory)
    TEST_ASSERT_EQUAL(p2, p1);

    rs_arena_destroy(arena);
}

// Test alignment
void test_arena_alignment(void)
{

    rs_arena_t *arena = rs_arena_create(1024);
    TEST_ASSERT_NOT_NULL(arena);

    rs_allocator_t *alloc = rs_arena_allocator(arena);

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

    rs_arena_destroy(arena);
}

// Test nested allocator (arena with backing allocator)
void test_arena_nested(void)
{

    // Create parent arena
    rs_arena_t *parent = rs_arena_create(4096);
    TEST_ASSERT_NOT_NULL(parent);

    // Create child arena using parent as backing allocator
    rs_arena_t *child = rs_arena_create(2048, .allocator = rs_arena_allocator(parent));
    TEST_ASSERT_NOT_NULL(child);

    rs_allocator_t *child_alloc = rs_arena_allocator(child);

    // Allocate from child
    int *x = (int *)rs_alloc(child_alloc, sizeof(int));
    TEST_ASSERT_NOT_NULL(x);
    *x = 123;
    TEST_ASSERT_EQUAL(123, *x);

    // Destroy child (its memory comes from parent)
    rs_arena_destroy(child);

    // Destroy parent
    rs_arena_destroy(parent);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_arena_create_destroy);
    RUN_TEST(test_arena_alloc);
    RUN_TEST(test_arena_multiple_allocs);
    RUN_TEST(test_arena_growth);
    RUN_TEST(test_arena_reset);
    RUN_TEST(test_arena_alignment);
    RUN_TEST(test_arena_nested);

    return UNITY_END();
}
