#include <rs/std/allocators/arena.h>
#include <rs/std/containers/array.h>
#include <rs/std/logging/logging.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_array"

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Creation & Destruction tests
// ============================================================================

void test_array_create(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    TEST_ASSERT_EQUAL(0, rs_array_len(&arr));
    TEST_ASSERT_EQUAL(0, rs_array_cap(&arr));
    TEST_ASSERT_TRUE(rs_array_is_empty(&arr));
    TEST_ASSERT_EQUAL(sizeof(int), rs_array_elem_size(&arr));

    rs_arena_destroy(arena);
}

void test_array_new(void)
{
    rs_array_t arr = rs_array_create(sizeof(int));

    TEST_ASSERT_EQUAL(0, rs_array_len(&arr));
    TEST_ASSERT_TRUE(rs_array_is_empty(&arr));

    rs_array_destroy(&arr);
}

void test_array_with_capacity(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc, .initial_capacity = 100);

    TEST_ASSERT_EQUAL(0, rs_array_len(&arr));
    TEST_ASSERT_GREATER_OR_EQUAL(100, rs_array_cap(&arr));
    TEST_ASSERT_NOT_NULL(rs_array_data(&arr));

    rs_arena_destroy(arena);
}

void test_array_clone(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Add some elements
    for (int i = 0; i < 5; i++) {
        rs_array_push(&arr, &i);
    }

    // Clone
    rs_array_t clone = rs_array_clone(arr);

    TEST_ASSERT_EQUAL(rs_array_len(&arr), rs_array_len(&clone));

    // Verify elements
    for (rs_size_t i = 0; i < rs_array_len(&clone); i++) {
        int *val1 = (int *)rs_array_get(&arr, i);
        int *val2 = (int *)rs_array_get(&clone, i);
        TEST_ASSERT_EQUAL(*val1, *val2);
    }

    // Modify clone - should not affect original
    int new_val = 999;
    rs_array_push(&clone, &new_val);
    TEST_ASSERT_EQUAL(5, rs_array_len(&arr));
    TEST_ASSERT_EQUAL(6, rs_array_len(&clone));

    rs_arena_destroy(arena);
}

// ============================================================================
// Push/Pop tests
// ============================================================================

void test_array_push(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push elements
    for (int i = 0; i < 10; i++) {
        rs_result_t res = rs_array_push(&arr, &i);
        TEST_ASSERT_EQUAL(RS_OK, res);
    }

    TEST_ASSERT_EQUAL(10, rs_array_len(&arr));

    // Verify elements
    for (int i = 0; i < 10; i++) {
        int *val = (int *)rs_array_get(&arr, i);
        TEST_ASSERT_EQUAL(i, *val);
    }

    rs_arena_destroy(arena);
}

void test_array_pop(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push elements
    for (int i = 0; i < 5; i++) {
        rs_array_push(&arr, &i);
    }

    // Pop elements
    for (int i = 4; i >= 0; i--) {
        int val;
        rs_result_t res = rs_array_pop(&arr, &val);
        TEST_ASSERT_EQUAL(RS_OK, res);
        TEST_ASSERT_EQUAL(i, val);
    }

    TEST_ASSERT_EQUAL(0, rs_array_len(&arr));
    TEST_ASSERT_TRUE(rs_array_is_empty(&arr));

    // Pop from empty should fail
    int val;
    rs_result_t res = rs_array_pop(&arr, &val);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, res);

    rs_arena_destroy(arena);
}

void test_array_push_pop_many(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push many elements (trigger growth)
    for (int i = 0; i < 1000; i++) {
        rs_array_push(&arr, &i);
    }

    TEST_ASSERT_EQUAL(1000, rs_array_len(&arr));

    // Verify all elements
    for (int i = 0; i < 1000; i++) {
        int *val = (int *)rs_array_get(&arr, i);
        TEST_ASSERT_EQUAL(i, *val);
    }

    rs_arena_destroy(arena);
}

// ============================================================================
// Insert/Remove tests
// ============================================================================

void test_array_insert(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Initial: [0, 1, 2, 3, 4]
    for (int i = 0; i < 5; i++) {
        rs_array_push(&arr, &i);
    }

    // Insert 99 at index 2: [0, 1, 99, 2, 3, 4]
    int val = 99;
    rs_array_insert(&arr, 2, &val);

    TEST_ASSERT_EQUAL(6, rs_array_len(&arr));
    TEST_ASSERT_EQUAL(0, *(int *)rs_array_get(&arr, 0));
    TEST_ASSERT_EQUAL(1, *(int *)rs_array_get(&arr, 1));
    TEST_ASSERT_EQUAL(99, *(int *)rs_array_get(&arr, 2));
    TEST_ASSERT_EQUAL(2, *(int *)rs_array_get(&arr, 3));

    rs_arena_destroy(arena);
}

void test_array_remove(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Initial: [0, 1, 2, 3, 4]
    for (int i = 0; i < 5; i++) {
        rs_array_push(&arr, &i);
    }

    // Remove index 2: [0, 1, 3, 4]
    int removed;
    rs_array_remove(&arr, 2, &removed);

    TEST_ASSERT_EQUAL(4, rs_array_len(&arr));
    TEST_ASSERT_EQUAL(2, removed);
    TEST_ASSERT_EQUAL(0, *(int *)rs_array_get(&arr, 0));
    TEST_ASSERT_EQUAL(1, *(int *)rs_array_get(&arr, 1));
    TEST_ASSERT_EQUAL(3, *(int *)rs_array_get(&arr, 2));
    TEST_ASSERT_EQUAL(4, *(int *)rs_array_get(&arr, 3));

    rs_arena_destroy(arena);
}

// ============================================================================
// Access tests
// ============================================================================

void test_array_first_last(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Empty array
    TEST_ASSERT_NULL(rs_array_first(&arr));
    TEST_ASSERT_NULL(rs_array_last(&arr));

    // Add elements: [10, 20, 30]
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        rs_array_push(&arr, &vals[i]);
    }

    TEST_ASSERT_EQUAL(10, *(int *)rs_array_first(&arr));
    TEST_ASSERT_EQUAL(30, *(int *)rs_array_last(&arr));

    rs_arena_destroy(arena);
}

// ============================================================================
// Resize/Reserve tests
// ============================================================================

void test_array_reserve(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    rs_result_t res = rs_array_reserve(&arr, 100);
    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_GREATER_OR_EQUAL(100, rs_array_cap(&arr));
    TEST_ASSERT_EQUAL(0, rs_array_len(&arr)); // Length unchanged

    rs_arena_destroy(arena);
}

void test_array_resize(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Resize to 10 (should zero-initialize)
    rs_array_resize(&arr, 10);

    TEST_ASSERT_EQUAL(10, rs_array_len(&arr));

    for (rs_size_t i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(0, *(int *)rs_array_get(&arr, i));
    }

    // Set some values
    for (rs_size_t i = 0; i < 10; i++) {
        *(int *)rs_array_get(&arr, i) = i;
    }

    // Resize down to 5
    rs_array_resize(&arr, 5);
    TEST_ASSERT_EQUAL(5, rs_array_len(&arr));

    // Verify remaining elements
    for (rs_size_t i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(i, *(int *)rs_array_get(&arr, i));
    }

    rs_arena_destroy(arena);
}

void test_array_clear(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    for (int i = 0; i < 10; i++) {
        rs_array_push(&arr, &i);
    }

    rs_size_t cap_before = rs_array_cap(&arr);

    rs_array_clear(&arr);

    TEST_ASSERT_EQUAL(0, rs_array_len(&arr));
    TEST_ASSERT_TRUE(rs_array_is_empty(&arr));
    TEST_ASSERT_EQUAL(cap_before, rs_array_cap(&arr)); // Capacity unchanged

    rs_arena_destroy(arena);
}

void test_array_shrink_to_fit(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push elements
    for (int i = 0; i < 10; i++) {
        rs_array_push(&arr, &i);
    }

    // Capacity might be larger than 10
    rs_size_t cap_before = rs_array_cap(&arr);

    rs_array_shrink_to_fit(&arr);

    TEST_ASSERT_EQUAL(10, rs_array_len(&arr));
    TEST_ASSERT_EQUAL(10, rs_array_cap(&arr));
    TEST_ASSERT_LESS_OR_EQUAL(cap_before, rs_array_cap(&arr));

    // Verify elements still intact
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(i, *(int *)rs_array_get(&arr, i));
    }

    rs_arena_destroy(arena);
}

// ============================================================================
// Struct tests (non-trivial element type)
// ============================================================================

typedef struct {
    int x;
    int y;
} Point;

void test_array_structs(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(Point), .allocator = alloc);

    // Push points
    for (int i = 0; i < 5; i++) {
        Point pt = {i, i * 10};
        rs_array_push(&arr, &pt);
    }

    TEST_ASSERT_EQUAL(5, rs_array_len(&arr));

    // Verify points
    for (int i = 0; i < 5; i++) {
        Point *pt = (Point *)rs_array_get(&arr, i);
        TEST_ASSERT_EQUAL(i, pt->x);
        TEST_ASSERT_EQUAL(i * 10, pt->y);
    }

    rs_arena_destroy(arena);
}

// ============================================================================
// System allocator test
// ============================================================================

void test_array_system_allocator(void)
{
    rs_array_t arr = rs_array_create(sizeof(int));

    for (int i = 0; i < 10; i++) {
        rs_array_push(&arr, &i);
    }

    TEST_ASSERT_EQUAL(10, rs_array_len(&arr));

    rs_array_destroy(&arr);
}

// ============================================================================
// Sorting tests
// ============================================================================

// Comparator for integers (ascending)
static int compare_ints_asc(const void *a, const void *b, void *user_data)
{
    (void)user_data; // Unused
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

// Comparator for integers (descending)
static int compare_ints_desc(const void *a, const void *b, void *user_data)
{
    (void)user_data; // Unused
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ib - ia;
}

// Comparator with user_data to control order
static int compare_ints_with_order(const void *a, const void *b, void *user_data)
{
    rs_bool reverse = *(rs_bool *)user_data;
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    int cmp = ia - ib;
    return reverse ? -cmp : cmp;
}

// Comparator for Points (by x coordinate)
static int compare_points_by_x(const void *a, const void *b, void *user_data)
{
    (void)user_data; // Unused
    const Point *pa = (const Point *)a;
    const Point *pb = (const Point *)b;
    return pa->x - pb->x;
}

// Comparator for Points (by y coordinate)
static int compare_points_by_y(const void *a, const void *b, void *user_data)
{
    (void)user_data; // Unused
    const Point *pa = (const Point *)a;
    const Point *pb = (const Point *)b;
    return pa->y - pb->y;
}

void test_array_sors_ascending(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push unsorted elements: [5, 2, 8, 1, 9, 3]
    int vals[] = {5, 2, 8, 1, 9, 3};
    for (int i = 0; i < 6; i++) {
        rs_array_push(&arr, &vals[i]);
    }

    // Sort ascending
    rs_array_sort(&arr, compare_ints_asc, NULL);

    // Verify sorted: [1, 2, 3, 5, 8, 9]
    int expected[] = {1, 2, 3, 5, 8, 9};
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected[i], *(int *)rs_array_get(&arr, i));
    }

    rs_arena_destroy(arena);
}

void test_array_sors_descending(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push unsorted elements: [5, 2, 8, 1, 9, 3]
    int vals[] = {5, 2, 8, 1, 9, 3};
    for (int i = 0; i < 6; i++) {
        rs_array_push(&arr, &vals[i]);
    }

    // Sort descending
    rs_array_sort(&arr, compare_ints_desc, NULL);

    // Verify sorted: [9, 8, 5, 3, 2, 1]
    int expected[] = {9, 8, 5, 3, 2, 1};
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected[i], *(int *)rs_array_get(&arr, i));
    }

    rs_arena_destroy(arena);
}

void test_array_sors_with_user_data(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push unsorted elements: [5, 2, 8, 1, 9, 3]
    int vals[] = {5, 2, 8, 1, 9, 3};
    for (int i = 0; i < 6; i++) {
        rs_array_push(&arr, &vals[i]);
    }

    // Sort with user_data controlling order (reverse = true)
    rs_bool reverse = true;
    rs_array_sort(&arr, compare_ints_with_order, &reverse);

    // Verify sorted descending: [9, 8, 5, 3, 2, 1]
    int expected_desc[] = {9, 8, 5, 3, 2, 1};
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_desc[i], *(int *)rs_array_get(&arr, i));
    }

    // Sort with user_data controlling order (reverse = false)
    reverse = false;
    rs_array_sort(&arr, compare_ints_with_order, &reverse);

    // Verify sorted ascending: [1, 2, 3, 5, 8, 9]
    int expected_asc[] = {1, 2, 3, 5, 8, 9};
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_asc[i], *(int *)rs_array_get(&arr, i));
    }

    rs_arena_destroy(arena);
}

void test_array_sors_structs(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(Point), .allocator = alloc);

    // Push unsorted points
    Point points[] = {{5, 100}, {2, 50}, {8, 200}, {1, 25}};
    for (int i = 0; i < 4; i++) {
        rs_array_push(&arr, &points[i]);
    }

    // Sort by x coordinate
    rs_array_sort(&arr, compare_points_by_x, NULL);

    // Verify sorted by x: [1, 2, 5, 8]
    int expected_x[] = {1, 2, 5, 8};
    for (int i = 0; i < 4; i++) {
        Point *pt = (Point *)rs_array_get(&arr, i);
        TEST_ASSERT_EQUAL(expected_x[i], pt->x);
    }

    // Now sort by y coordinate
    rs_array_sort(&arr, compare_points_by_y, NULL);

    // Verify sorted by y: [25, 50, 100, 200]
    int expected_y[] = {25, 50, 100, 200};
    for (int i = 0; i < 4; i++) {
        Point *pt = (Point *)rs_array_get(&arr, i);
        TEST_ASSERT_EQUAL(expected_y[i], pt->y);
    }

    rs_arena_destroy(arena);
}

void test_array_sors_already_sorted(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push already sorted elements: [1, 2, 3, 4, 5]
    for (int i = 1; i <= 5; i++) {
        rs_array_push(&arr, &i);
    }

    // Sort (should remain the same)
    rs_array_sort(&arr, compare_ints_asc, NULL);

    // Verify still sorted: [1, 2, 3, 4, 5]
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(i + 1, *(int *)rs_array_get(&arr, i));
    }

    rs_arena_destroy(arena);
}

void test_array_sors_empty_and_single(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test empty array
    rs_array_t empty = rs_array_create(sizeof(int), .allocator = alloc);
    rs_array_sort(&empty, compare_ints_asc, NULL); // Should not crash
    TEST_ASSERT_EQUAL(0, rs_array_len(&empty));

    // Test single element
    rs_array_t single = rs_array_create(sizeof(int), .allocator = alloc);
    int val = 42;
    rs_array_push(&single, &val);
    rs_array_sort(&single, compare_ints_asc, NULL); // Should not crash
    TEST_ASSERT_EQUAL(1, rs_array_len(&single));
    TEST_ASSERT_EQUAL(42, *(int *)rs_array_get(&single, 0));

    rs_arena_destroy(arena);
}

void test_array_sors_duplicates(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push array with duplicates: [5, 2, 5, 1, 2, 5]
    int vals[] = {5, 2, 5, 1, 2, 5};
    for (int i = 0; i < 6; i++) {
        rs_array_push(&arr, &vals[i]);
    }

    // Sort
    rs_array_sort(&arr, compare_ints_asc, NULL);

    // Verify sorted: [1, 2, 2, 5, 5, 5]
    int expected[] = {1, 2, 2, 5, 5, 5};
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected[i], *(int *)rs_array_get(&arr, i));
    }

    rs_arena_destroy(arena);
}

void test_array_sorted_copy(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Push unsorted elements: [5, 2, 8, 1]
    int vals[] = {5, 2, 8, 1};
    for (int i = 0; i < 4; i++) {
        rs_array_push(&arr, &vals[i]);
    }

    // Create sorted copy
    rs_array_t sorted = rs_array_clone(arr);
    rs_array_sort(&sorted, compare_ints_asc, NULL);

    // Original should remain unsorted
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(vals[i], *(int *)rs_array_get(&arr, i));
    }

    // Sorted copy should be sorted: [1, 2, 5, 8]
    int expected[] = {1, 2, 5, 8};
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(expected[i], *(int *)rs_array_get(&sorted, i));
    }

    rs_arena_destroy(arena);
}

// ============================================================================
// Edge cases
// ============================================================================

void test_array_empty_operations(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_array_t arr = rs_array_create(sizeof(int), .allocator = alloc);

    // Pop from empty
    int val;
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, rs_array_pop(&arr, &val));

    // Remove from empty
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, rs_array_remove(&arr, 0, &val));

    // First/Last on empty
    TEST_ASSERT_NULL(rs_array_first(&arr));
    TEST_ASSERT_NULL(rs_array_last(&arr));

    rs_arena_destroy(arena);
}

int main(void)
{
    UNITY_BEGIN();

    // Creation & Destruction
    RUN_TEST(test_array_create);
    RUN_TEST(test_array_new);
    RUN_TEST(test_array_with_capacity);
    RUN_TEST(test_array_clone);

    // Push/Pop
    RUN_TEST(test_array_push);
    RUN_TEST(test_array_pop);
    RUN_TEST(test_array_push_pop_many);

    // Insert/Remove
    RUN_TEST(test_array_insert);
    RUN_TEST(test_array_remove);

    // Access
    RUN_TEST(test_array_first_last);

    // Resize/Reserve
    RUN_TEST(test_array_reserve);
    RUN_TEST(test_array_resize);
    RUN_TEST(test_array_clear);
    RUN_TEST(test_array_shrink_to_fit);

    // Struct elements
    RUN_TEST(test_array_structs);

    // System allocator
    RUN_TEST(test_array_system_allocator);

    // Edge cases
    RUN_TEST(test_array_empty_operations);

    // Sorting
    RUN_TEST(test_array_sors_ascending);
    RUN_TEST(test_array_sors_descending);
    RUN_TEST(test_array_sors_with_user_data);
    RUN_TEST(test_array_sors_structs);
    RUN_TEST(test_array_sors_already_sorted);
    RUN_TEST(test_array_sors_empty_and_single);
    RUN_TEST(test_array_sors_duplicates);
    RUN_TEST(test_array_sorted_copy);

    return UNITY_END();
}
