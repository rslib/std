#include <rs/std/containers/array.h>
#include <rs/std/containers/slice.h>
#include <rs/std/containers/span.h>
#include <rs/std/logging/logging.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_span_slice"

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
// Span tests
// ============================================================================

void test_span_create(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_span_t span = rs_span_create(data, 5, sizeof(int));

    TEST_ASSERT_EQUAL(5, rs_span_len(span));
    TEST_ASSERT_FALSE(rs_span_is_empty(span));
    TEST_ASSERT_EQUAL(sizeof(int), rs_span_elem_size(span));
    TEST_ASSERT_EQUAL_PTR(data, rs_span_data(span));
}

void test_span_empty(void)
{
    rs_span_t span = rs_span_empty(sizeof(int));

    TEST_ASSERT_EQUAL(0, rs_span_len(span));
    TEST_ASSERT_TRUE(rs_span_is_empty(span));
    TEST_ASSERT_NULL(rs_span_data(span));
}

void test_span_from_array(void)
{
    rs_array_t arr = rs_array_create(sizeof(int));

    for (int i = 0; i < 5; i++) {
        rs_array_push(&arr, &i);
    }

    rs_span_t span = rs_span_from_array(&arr);

    TEST_ASSERT_EQUAL(5, rs_span_len(span));

    for (rs_size_t i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(i, *(const int *)rs_span_get(span, i));
    }

    rs_array_destroy(&arr);
}

void test_span_access(void)
{
    int data[] = {10, 20, 30, 40, 50};
    rs_span_t span = rs_span_create(data, 5, sizeof(int));

    TEST_ASSERT_EQUAL(10, *(const int *)rs_span_first(span));
    TEST_ASSERT_EQUAL(50, *(const int *)rs_span_last(span));
    TEST_ASSERT_EQUAL(30, *(const int *)rs_span_get(span, 2));
}

void test_span_slice(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_span_t span = rs_span_create(data, 5, sizeof(int));

    // Slice [1..4) -> [2, 3, 4]
    rs_span_t sub = rs_span_slice(span, 1, 4);

    TEST_ASSERT_EQUAL(3, rs_span_len(sub));
    TEST_ASSERT_EQUAL(2, *(const int *)rs_span_get(sub, 0));
    TEST_ASSERT_EQUAL(3, *(const int *)rs_span_get(sub, 1));
    TEST_ASSERT_EQUAL(4, *(const int *)rs_span_get(sub, 2));
}

void test_span_slice_from_to(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_span_t span = rs_span_create(data, 5, sizeof(int));

    // Slice from 2 -> [3, 4, 5]
    rs_span_t from = rs_span_slice_from(span, 2);
    TEST_ASSERT_EQUAL(3, rs_span_len(from));
    TEST_ASSERT_EQUAL(3, *(const int *)rs_span_first(from));

    // Slice to 3 -> [1, 2, 3]
    rs_span_t to = rs_span_slice_to(span, 3);
    TEST_ASSERT_EQUAL(3, rs_span_len(to));
    TEST_ASSERT_EQUAL(1, *(const int *)rs_span_first(to));
}

void test_span_comparison(void)
{
    int data1[] = {1, 2, 3};
    int data2[] = {1, 2, 3};
    int data3[] = {1, 2, 4};

    rs_span_t span1 = rs_span_create(data1, 3, sizeof(int));
    rs_span_t span2 = rs_span_create(data2, 3, sizeof(int));
    rs_span_t span3 = rs_span_create(data3, 3, sizeof(int));

    TEST_ASSERT_TRUE(rs_span_eq(span1, span2));
    TEST_ASSERT_FALSE(rs_span_eq(span1, span3));

    TEST_ASSERT_EQUAL(0, rs_span_cmp(span1, span2));
    TEST_ASSERT_LESS_THAN(0, rs_span_cmp(span1, span3));
    TEST_ASSERT_GREATER_THAN(0, rs_span_cmp(span3, span1));
}

void test_span_from_c_array(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_span_t span = rs_span_from_c_array(data);

    TEST_ASSERT_EQUAL(5, rs_span_len(span));
    TEST_ASSERT_EQUAL(sizeof(int), rs_span_elem_size(span));
}

// ============================================================================
// Slice tests
// ============================================================================

void test_slice_create(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));

    TEST_ASSERT_EQUAL(5, rs_slice_len(slice));
    TEST_ASSERT_FALSE(rs_slice_is_empty(slice));
    TEST_ASSERT_EQUAL(sizeof(int), rs_slice_elem_size(slice));
    TEST_ASSERT_EQUAL_PTR(data, rs_slice_data(slice));
}

void test_slice_empty(void)
{
    rs_slice_t slice = rs_slice_empty(sizeof(int));

    TEST_ASSERT_EQUAL(0, rs_slice_len(slice));
    TEST_ASSERT_TRUE(rs_slice_is_empty(slice));
    TEST_ASSERT_NULL(rs_slice_data(slice));
}

void test_slice_from_array(void)
{
    rs_array_t arr = rs_array_create(sizeof(int));

    for (int i = 0; i < 5; i++) {
        rs_array_push(&arr, &i);
    }

    rs_slice_t slice = rs_slice_from_array(&arr);

    TEST_ASSERT_EQUAL(5, rs_slice_len(slice));

    // Modify through slice
    *(int *)rs_slice_get(slice, 0) = 99;

    // Verify modification in array
    TEST_ASSERT_EQUAL(99, *(int *)rs_array_get(&arr, 0));

    rs_array_destroy(&arr);
}

void test_slice_to_span(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));

    rs_span_t span = rs_slice_to_span(slice);

    TEST_ASSERT_EQUAL(rs_slice_len(slice), rs_span_len(span));
    TEST_ASSERT_EQUAL_PTR(rs_slice_data(slice), rs_span_data(span));
}

void test_slice_access(void)
{
    int data[] = {10, 20, 30, 40, 50};
    rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));

    TEST_ASSERT_EQUAL(10, *(int *)rs_slice_first(slice));
    TEST_ASSERT_EQUAL(50, *(int *)rs_slice_last(slice));
    TEST_ASSERT_EQUAL(30, *(int *)rs_slice_get(slice, 2));

    // Modify
    *(int *)rs_slice_get(slice, 2) = 999;
    TEST_ASSERT_EQUAL(999, data[2]);
}

void test_slice_slice(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));

    // Sub-slice [1..4) -> [2, 3, 4]
    rs_slice_t sub = rs_slice_slice(slice, 1, 4);

    TEST_ASSERT_EQUAL(3, rs_slice_len(sub));
    TEST_ASSERT_EQUAL(2, *(int *)rs_slice_get(sub, 0));

    // Modify sub-slice
    *(int *)rs_slice_get(sub, 0) = 99;
    TEST_ASSERT_EQUAL(99, data[1]); // Modifies original
}

void test_slice_fill(void)
{
    int data[5] = {0};
    rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));

    int val = 42;
    rs_slice_fill(slice, &val);

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(42, data[i]);
    }
}

void test_slice_copy_from_span(void)
{
    int src_data[] = {1, 2, 3, 4, 5};
    int dst_data[3] = {0};

    rs_span_t src = rs_span_create(src_data, 5, sizeof(int));
    rs_slice_t dst = rs_slice_create(dst_data, 3, sizeof(int));

    rs_size_t copied = rs_slice_copy_from_span(dst, src);

    TEST_ASSERT_EQUAL(3, copied);
    TEST_ASSERT_EQUAL(1, dst_data[0]);
    TEST_ASSERT_EQUAL(2, dst_data[1]);
    TEST_ASSERT_EQUAL(3, dst_data[2]);
}

void test_slice_copy_from_slice(void)
{
    int src_data[] = {1, 2, 3};
    int dst_data[3] = {0};

    rs_slice_t src = rs_slice_create(src_data, 3, sizeof(int));
    rs_slice_t dst = rs_slice_create(dst_data, 3, sizeof(int));

    rs_size_t copied = rs_slice_copy_from_slice(dst, src);

    TEST_ASSERT_EQUAL(3, copied);
    TEST_ASSERT_EQUAL_MEMORY(src_data, dst_data, 3 * sizeof(int));
}

void test_slice_swap(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));

    rs_slice_swap(slice, 1, 3);

    TEST_ASSERT_EQUAL(1, data[0]);
    TEST_ASSERT_EQUAL(4, data[1]); // Was 2, now 4
    TEST_ASSERT_EQUAL(3, data[2]);
    TEST_ASSERT_EQUAL(2, data[3]); // Was 4, now 2
    TEST_ASSERT_EQUAL(5, data[4]);
}

void test_slice_reverse(void)
{
    int data[] = {1, 2, 3, 4, 5};
    rs_slice_t slice = rs_slice_create(data, 5, sizeof(int));

    rs_slice_reverse(slice);

    TEST_ASSERT_EQUAL(5, data[0]);
    TEST_ASSERT_EQUAL(4, data[1]);
    TEST_ASSERT_EQUAL(3, data[2]);
    TEST_ASSERT_EQUAL(2, data[3]);
    TEST_ASSERT_EQUAL(1, data[4]);
}

void test_slice_comparison(void)
{
    int data1[] = {1, 2, 3};
    int data2[] = {1, 2, 3};
    int data3[] = {1, 2, 4};

    rs_slice_t slice1 = rs_slice_create(data1, 3, sizeof(int));
    rs_slice_t slice2 = rs_slice_create(data2, 3, sizeof(int));
    rs_slice_t slice3 = rs_slice_create(data3, 3, sizeof(int));

    TEST_ASSERT_TRUE(rs_slice_eq(slice1, slice2));
    TEST_ASSERT_FALSE(rs_slice_eq(slice1, slice3));

    TEST_ASSERT_EQUAL(0, rs_slice_cmp(slice1, slice2));
    TEST_ASSERT_LESS_THAN(0, rs_slice_cmp(slice1, slice3));
    TEST_ASSERT_GREATER_THAN(0, rs_slice_cmp(slice3, slice1));
}

// ============================================================================
// Edge cases
// ============================================================================

void test_span_empty_operations(void)
{
    rs_span_t empty = rs_span_empty(sizeof(int));

    TEST_ASSERT_NULL(rs_span_first(empty));
    TEST_ASSERT_NULL(rs_span_last(empty));

    rs_span_t sliced = rs_span_slice(empty, 0, 10);
    TEST_ASSERT_EQUAL(0, rs_span_len(sliced));
}

void test_slice_empty_operations(void)
{
    rs_slice_t empty = rs_slice_empty(sizeof(int));

    TEST_ASSERT_NULL(rs_slice_first(empty));
    TEST_ASSERT_NULL(rs_slice_last(empty));

    rs_slice_reverse(empty); // Should not crash
}

int main(void)
{
    UNITY_BEGIN();

    // Span tests
    RUN_TEST(test_span_create);
    RUN_TEST(test_span_empty);
    RUN_TEST(test_span_from_array);
    RUN_TEST(test_span_access);
    RUN_TEST(test_span_slice);
    RUN_TEST(test_span_slice_from_to);
    RUN_TEST(test_span_comparison);
    RUN_TEST(test_span_from_c_array);

    // Slice tests
    RUN_TEST(test_slice_create);
    RUN_TEST(test_slice_empty);
    RUN_TEST(test_slice_from_array);
    RUN_TEST(test_slice_to_span);
    RUN_TEST(test_slice_access);
    RUN_TEST(test_slice_slice);
    RUN_TEST(test_slice_fill);
    RUN_TEST(test_slice_copy_from_span);
    RUN_TEST(test_slice_copy_from_slice);
    RUN_TEST(test_slice_swap);
    RUN_TEST(test_slice_reverse);
    RUN_TEST(test_slice_comparison);

    // Edge cases
    RUN_TEST(test_span_empty_operations);
    RUN_TEST(test_slice_empty_operations);

    return UNITY_END();
}
