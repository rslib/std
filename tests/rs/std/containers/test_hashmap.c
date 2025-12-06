#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/hashmap.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/string/zstring_view.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_hashmap"

// ============================================================================
// Test Setup
// ============================================================================

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
// Basic Hashmap Tests (Integer Keys)
// ============================================================================

void test_hashmap_create_destroy(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);
    TEST_ASSERT_NOT_NULL(map.entries);
    TEST_ASSERT_EQUAL(0, rs_hashmap_size(&map));
    TEST_ASSERT_TRUE(rs_hashmap_empty(&map));
    rs_hashmap_destroy(&map);
}

void test_hashmap_insers_get(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_u64 key = 42;
    int value = 100;

    rs_result_t result = rs_hashmap_insert(&map, &key, &value);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(1, rs_hashmap_size(&map));

    int *retrieved = rs_hashmap_get(&map, &key);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(100, *retrieved);

    rs_hashmap_destroy(&map);
}

void test_hashmap_insers_multiple(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    for (rs_u64 i = 0; i < 10; i++) {
        int value = (int)(i * 10);
        TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &i, &value));
    }

    TEST_ASSERT_EQUAL(10, rs_hashmap_size(&map));

    // Verify all values
    for (rs_u64 i = 0; i < 10; i++) {
        int *retrieved = rs_hashmap_get(&map, &i);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL(i * 10, *retrieved);
    }

    rs_hashmap_destroy(&map);
}

void test_hashmap_update_existing(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_u64 key = 42;
    int value1 = 100;
    int value2 = 200;

    // Insert initial value
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key, &value1));
    TEST_ASSERT_EQUAL(1, rs_hashmap_size(&map));

    // Update with insers_or_update
    rs_bool inserted;
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insers_or_update(&map, &key, &value2, &inserted));
    TEST_ASSERT_FALSE(inserted);                 // Should be update, not insert
    TEST_ASSERT_EQUAL(1, rs_hashmap_size(&map)); // Size shouldn't change

    // Verify updated value
    int *retrieved = rs_hashmap_get(&map, &key);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(200, *retrieved);

    rs_hashmap_destroy(&map);
}

void test_hashmap_contains(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_u64 key1 = 42;
    rs_u64 key2 = 99;
    int value = 100;

    TEST_ASSERT_FALSE(rs_hashmap_contains(&map, &key1));

    rs_hashmap_insert(&map, &key1, &value);
    TEST_ASSERT_TRUE(rs_hashmap_contains(&map, &key1));
    TEST_ASSERT_FALSE(rs_hashmap_contains(&map, &key2));

    rs_hashmap_destroy(&map);
}

void test_hashmap_remove(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_u64 key = 42;
    int value = 100;

    rs_hashmap_insert(&map, &key, &value);
    TEST_ASSERT_EQUAL(1, rs_hashmap_size(&map));

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_remove(&map, &key));
    TEST_ASSERT_EQUAL(0, rs_hashmap_size(&map));
    TEST_ASSERT_FALSE(rs_hashmap_contains(&map, &key));

    rs_hashmap_destroy(&map);
}

void test_hashmap_remove_nonexistent(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_u64 key = 42;
    rs_result_t result = rs_hashmap_remove(&map, &key);
    TEST_ASSERT_EQUAL(RS_ERR_NOTFOUND, result);

    rs_hashmap_destroy(&map);
}

void test_hashmap_clear(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    // Insert multiple entries
    for (rs_u64 i = 0; i < 10; i++) {
        int value = (int)i;
        rs_hashmap_insert(&map, &i, &value);
    }

    TEST_ASSERT_EQUAL(10, rs_hashmap_size(&map));

    rs_hashmap_clear(&map);
    TEST_ASSERT_EQUAL(0, rs_hashmap_size(&map));
    TEST_ASSERT_TRUE(rs_hashmap_empty(&map));

    rs_hashmap_destroy(&map);
}

// ============================================================================
// String Key Tests
// ============================================================================

void test_hashmap_string_keys(void)
{
    rs_hashmap_t map = rs_hashmap_cstr_create(sizeof(int), .allocator = allocator);

    const char *key1 = "hello";
    const char *key2 = "world";
    int value1 = 100;
    int value2 = 200;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key1, &value1));
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key2, &value2));
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    int *retrieved1 = rs_hashmap_get(&map, &key1);
    int *retrieved2 = rs_hashmap_get(&map, &key2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_hashmap_destroy(&map);
}

void test_hashmap_string_update(void)
{
    rs_hashmap_t map = rs_hashmap_cstr_create(sizeof(int), .allocator = allocator);

    const char *key = "test";
    int value1 = 100;
    int value2 = 200;

    rs_hashmap_insert(&map, &key, &value1);

    // Use insers_or_update for updating existing keys
    rs_bool inserted;
    rs_hashmap_insers_or_update(&map, &key, &value2, &inserted);
    TEST_ASSERT_FALSE(inserted); // Should be an update

    TEST_ASSERT_EQUAL(1, rs_hashmap_size(&map)); // Should still be 1

    int *retrieved = rs_hashmap_get(&map, &key);
    TEST_ASSERT_EQUAL(200, *retrieved);

    rs_hashmap_destroy(&map);
}

void test_hashmap_string_remove(void)
{
    rs_hashmap_t map = rs_hashmap_cstr_create(sizeof(int), .allocator = allocator);

    const char *key = "hello";
    int value = 100;

    rs_hashmap_insert(&map, &key, &value);
    TEST_ASSERT_EQUAL(1, rs_hashmap_size(&map));

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_remove(&map, &key));
    TEST_ASSERT_EQUAL(0, rs_hashmap_size(&map));

    rs_hashmap_destroy(&map);
}

// ============================================================================
// Pointer Key Tests
// ============================================================================

void test_hashmap_pointer_keys(void)
{
    rs_hashmap_t map = rs_hashmap_ptr_create(sizeof(int), .allocator = allocator);

    int obj1 = 1, obj2 = 2;
    int value1 = 100;
    int value2 = 200;

    // For pointer-keyed hashmaps, we need to pass a pointer to the pointer variable
    void *ptr1 = &obj1;
    void *ptr2 = &obj2;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &ptr1, &value1));
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &ptr2, &value2));
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    int *retrieved1 = rs_hashmap_get(&map, &ptr1);
    int *retrieved2 = rs_hashmap_get(&map, &ptr2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_hashmap_destroy(&map);
}

void test_hashmap_pointer_identity(void)
{
    rs_hashmap_t map = rs_hashmap_ptr_create(sizeof(int), .allocator = allocator);

    int obj1 = 42;
    int obj2 = 42; // Same value but different address
    int value1 = 100;
    int value2 = 200;

    // For pointer-keyed hashmaps, we need to pass a pointer to the pointer variable
    void *ptr1 = &obj1;
    void *ptr2 = &obj2;

    rs_hashmap_insert(&map, &ptr1, &value1);
    rs_hashmap_insert(&map, &ptr2, &value2);

    // Should have 2 entries because pointers are different
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    int *retrieved1 = rs_hashmap_get(&map, &ptr1);
    int *retrieved2 = rs_hashmap_get(&map, &ptr2);

    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_hashmap_destroy(&map);
}

// ============================================================================
// Resize Tests
// ============================================================================

void test_hashmap_resize_growth(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_size_t initial_capacity = rs_hashmap_capacity(&map);

    // Insert enough elements to trigger resize
    for (rs_u64 i = 0; i < 100; i++) {
        int value = (int)i;
        rs_hashmap_insert(&map, &i, &value);
    }

    TEST_ASSERT_EQUAL(100, rs_hashmap_size(&map));
    TEST_ASSERT_GREATER_THAN(initial_capacity, rs_hashmap_capacity(&map));

    // Verify all values are still accessible
    for (rs_u64 i = 0; i < 100; i++) {
        int *retrieved = rs_hashmap_get(&map, &i);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL(i, *retrieved);
    }

    rs_hashmap_destroy(&map);
}

// ============================================================================
// Iterator Tests
// ============================================================================

void test_hashmap_iterator_empty(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_hashmap_iter_t iter = rs_hashmap_iter_begin(&map);
    TEST_ASSERT_FALSE(rs_hashmap_iter_valid(&iter));

    rs_hashmap_destroy(&map);
}

void test_hashmap_iterator_single(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_u64 key = 42;
    int value = 100;
    rs_hashmap_insert(&map, &key, &value);

    rs_hashmap_iter_t iter = rs_hashmap_iter_begin(&map);
    TEST_ASSERT_TRUE(rs_hashmap_iter_valid(&iter));

    rs_u64 *iter_key = rs_hashmap_iter_key(&iter);
    int *iter_value = rs_hashmap_iter_value(&iter);

    TEST_ASSERT_NOT_NULL(iter_key);
    TEST_ASSERT_NOT_NULL(iter_value);
    TEST_ASSERT_EQUAL(42, *iter_key);
    TEST_ASSERT_EQUAL(100, *iter_value);

    rs_hashmap_iter_next(&iter);
    TEST_ASSERT_FALSE(rs_hashmap_iter_valid(&iter));

    rs_hashmap_destroy(&map);
}

void test_hashmap_iterator_multiple(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    // Insert 10 entries
    for (rs_u64 i = 0; i < 10; i++) {
        int value = (int)(i * 10);
        rs_hashmap_insert(&map, &i, &value);
    }

    // Count entries via iterator
    int count = 0;
    for (rs_hashmap_iter_t iter = rs_hashmap_iter_begin(&map); rs_hashmap_iter_valid(&iter);
         rs_hashmap_iter_next(&iter)) {
        TEST_ASSERT_NOT_NULL(rs_hashmap_iter_key(&iter));
        TEST_ASSERT_NOT_NULL(rs_hashmap_iter_value(&iter));
        count++;
    }

    TEST_ASSERT_EQUAL(10, count);

    rs_hashmap_destroy(&map);
}

void test_hashmap_foreach_macro(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    // Insert entries
    for (rs_u64 i = 0; i < 5; i++) {
        int value = (int)i;
        rs_hashmap_insert(&map, &i, &value);
    }

    // Use foreach macro
    int sum = 0;
    RS_HASHMAP_FOREACH(&map, iter)
    {
        int *value = rs_hashmap_iter_value(&iter);
        sum += *value;
    }

    TEST_ASSERT_EQUAL(0 + 1 + 2 + 3 + 4, sum);

    rs_hashmap_destroy(&map);
}

// ============================================================================
// Collision Handling Tests
// ============================================================================

void test_hashmap_collision_handling(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    // Insert many elements to create collisions
    for (rs_u64 i = 0; i < 1000; i++) {
        int value = (int)i;
        TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &i, &value));
    }

    TEST_ASSERT_EQUAL(1000, rs_hashmap_size(&map));

    // Verify all can be retrieved
    for (rs_u64 i = 0; i < 1000; i++) {
        int *retrieved = rs_hashmap_get(&map, &i);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL(i, *retrieved);
    }

    rs_hashmap_destroy(&map);
}

// ============================================================================
// Tombstone Tests
// ============================================================================

void test_hashmap_tombstones(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    // Insert, remove, re-insert pattern
    for (rs_u64 i = 0; i < 20; i++) {
        int value = (int)i;
        rs_hashmap_insert(&map, &i, &value);
    }

    // Remove half
    for (rs_u64 i = 0; i < 20; i += 2) {
        rs_hashmap_remove(&map, &i);
    }

    TEST_ASSERT_EQUAL(10, rs_hashmap_size(&map));

    // Re-insert removed keys
    for (rs_u64 i = 0; i < 20; i += 2) {
        int value = (int)(i * 100);
        rs_hashmap_insert(&map, &i, &value);
    }

    TEST_ASSERT_EQUAL(20, rs_hashmap_size(&map));

    // Verify all values
    for (rs_u64 i = 0; i < 20; i++) {
        int *retrieved = rs_hashmap_get(&map, &i);
        TEST_ASSERT_NOT_NULL(retrieved);
        int expected = (i % 2 == 0) ? (i * 100) : i;
        TEST_ASSERT_EQUAL(expected, *retrieved);
    }

    rs_hashmap_destroy(&map);
}

// ============================================================================
// Custom Configuration Tests
// ============================================================================

void test_hashmap_custom_config(void)
{
    rs_hashmap_t map = rs_hashmap_create(sizeof(rs_u64), sizeof(int), .initial_capacity = 32, .load_factor = 0.5f,
                                         .hash_fn = rs_hash_xxh3, .key_eq_fn = rs_hashmap_internal_key_eq_memcmp,
                                         .key_destroy_fn = NULL, .value_destroy_fn = NULL, .user_data = NULL);
    TEST_ASSERT_NOT_NULL(map.entries);
    TEST_ASSERT_EQUAL(32, rs_hashmap_capacity(&map));

    rs_hashmap_destroy(&map);
}

// ============================================================================
// rs_string_t Key Tests
// ============================================================================

void test_hashmap_rs_string_keys(void)
{
    rs_hashmap_t map = rs_hashmap_string_create(sizeof(int), .allocator = allocator);

    rs_string_t key1 = rs_string_from_cstr("hello", .allocator = allocator);
    rs_string_t key2 = rs_string_from_cstr("world", .allocator = allocator);
    int value1 = 100;
    int value2 = 200;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key1, &value1));
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key2, &value2));
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    int *retrieved1 = rs_hashmap_get(&map, &key1);
    int *retrieved2 = rs_hashmap_get(&map, &key2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_string_destroy(&key1);
    rs_string_destroy(&key2);
    rs_hashmap_destroy(&map);
}

void test_hashmap_rs_string_update(void)
{
    rs_hashmap_t map = rs_hashmap_string_create(sizeof(int), .allocator = allocator);

    rs_string_t key = rs_string_from_cstr("test", .allocator = allocator);
    int value1 = 100;
    int value2 = 200;

    rs_hashmap_insert(&map, &key, &value1);

    rs_bool inserted;
    rs_hashmap_insers_or_update(&map, &key, &value2, &inserted);
    TEST_ASSERT_FALSE(inserted);
    TEST_ASSERT_EQUAL(1, rs_hashmap_size(&map));

    int *retrieved = rs_hashmap_get(&map, &key);
    TEST_ASSERT_EQUAL(200, *retrieved);

    rs_string_destroy(&key);
    rs_hashmap_destroy(&map);
}

// ============================================================================
// rs_string_view_t Key Tests
// ============================================================================

void test_hashmap_string_view_keys(void)
{
    rs_hashmap_t map = rs_hashmap_string_view_create(sizeof(int), .allocator = allocator);

    rs_string_view_t key1 = rs_sv_from_cstr("hello");
    rs_string_view_t key2 = rs_sv_from_cstr("world");
    int value1 = 100;
    int value2 = 200;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key1, &value1));
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key2, &value2));
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    int *retrieved1 = rs_hashmap_get(&map, &key1);
    int *retrieved2 = rs_hashmap_get(&map, &key2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_hashmap_destroy(&map);
}

void test_hashmap_string_view_substrings(void)
{
    rs_hashmap_t map = rs_hashmap_string_view_create(sizeof(int), .allocator = allocator);

    const char *text = "hello world";
    rs_string_view_t sv = rs_sv_from_cstr(text);
    rs_string_view_t key1 = rs_sv_slice_to(sv, 5);   // "hello"
    rs_string_view_t key2 = rs_sv_slice_from(sv, 6); // "world"
    int value1 = 100;
    int value2 = 200;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key1, &value1));
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key2, &value2));
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    // Verify lookups work with different views of same content
    rs_string_view_t lookup1 = rs_sv_from_cstr("hello");
    rs_string_view_t lookup2 = rs_sv_from_cstr("world");

    int *retrieved1 = rs_hashmap_get(&map, &lookup1);
    int *retrieved2 = rs_hashmap_get(&map, &lookup2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_hashmap_destroy(&map);
}

// ============================================================================
// rs_zstring_view_t Key Tests
// ============================================================================

void test_hashmap_zstring_view_keys(void)
{
    rs_hashmap_t map = rs_hashmap_zstring_view_create(sizeof(int), .allocator = allocator);

    rs_zstring_view_t key1 = rs_zsv_from_cstr("hello");
    rs_zstring_view_t key2 = rs_zsv_from_cstr("world");
    int value1 = 100;
    int value2 = 200;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key1, &value1));
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key2, &value2));
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    int *retrieved1 = rs_hashmap_get(&map, &key1);
    int *retrieved2 = rs_hashmap_get(&map, &key2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_hashmap_destroy(&map);
}

void test_hashmap_zstring_view_from_rs_string(void)
{
    rs_hashmap_t map = rs_hashmap_zstring_view_create(sizeof(int), .allocator = allocator);

    rs_string_t str1 = rs_string_from_cstr("hello", .allocator = allocator);
    rs_string_t str2 = rs_string_from_cstr("world", .allocator = allocator);

    rs_zstring_view_t key1 = rs_zsv_from_string(&str1);
    rs_zstring_view_t key2 = rs_zsv_from_string(&str2);
    int value1 = 100;
    int value2 = 200;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key1, &value1));
    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key2, &value2));
    TEST_ASSERT_EQUAL(2, rs_hashmap_size(&map));

    int *retrieved1 = rs_hashmap_get(&map, &key1);
    int *retrieved2 = rs_hashmap_get(&map, &key2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL(100, *retrieved1);
    TEST_ASSERT_EQUAL(200, *retrieved2);

    rs_string_destroy(&str1);
    rs_string_destroy(&str2);
    rs_hashmap_destroy(&map);
}

// ============================================================================
// Internal Equality Function Tests
// ============================================================================

void test_hashmap_internal_key_eq_memcmp_equal(void)
{
    rs_u64 key1 = 42;
    rs_u64 key2 = 42;

    rs_bool result = rs_hashmap_internal_key_eq_memcmp(&key1, &key2, sizeof(rs_u64), NULL);
    TEST_ASSERT_TRUE(result);
}

void test_hashmap_internal_key_eq_memcmp_not_equal(void)
{
    rs_u64 key1 = 42;
    rs_u64 key2 = 43;

    rs_bool result = rs_hashmap_internal_key_eq_memcmp(&key1, &key2, sizeof(rs_u64), NULL);
    TEST_ASSERT_FALSE(result);
}

void test_hashmap_internal_key_eq_memcmp_struct(void)
{
    typedef struct {
        int a;
        int b;
    } test_struct_t;

    test_struct_t s1 = {.a = 1, .b = 2};
    test_struct_t s2 = {.a = 1, .b = 2};
    test_struct_t s3 = {.a = 1, .b = 3};

    TEST_ASSERT_TRUE(rs_hashmap_internal_key_eq_memcmp(&s1, &s2, sizeof(test_struct_t), NULL));
    TEST_ASSERT_FALSE(rs_hashmap_internal_key_eq_memcmp(&s1, &s3, sizeof(test_struct_t), NULL));
}

void test_hashmap_internal_key_eq_cstr_equal(void)
{
    const char *str1 = "hello";
    const char *str2 = "hello";

    rs_bool result = rs_hashmap_internal_key_eq_cstr(&str1, &str2, 0, NULL);
    TEST_ASSERT_TRUE(result);
}

void test_hashmap_internal_key_eq_cstr_not_equal(void)
{
    const char *str1 = "hello";
    const char *str2 = "world";

    rs_bool result = rs_hashmap_internal_key_eq_cstr(&str1, &str2, 0, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_hashmap_internal_key_eq_cstr_case_sensitive(void)
{
    const char *str1 = "Hello";
    const char *str2 = "hello";

    rs_bool result = rs_hashmap_internal_key_eq_cstr(&str1, &str2, 0, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_hashmap_internal_key_eq_cstr_empty(void)
{
    const char *str1 = "";
    const char *str2 = "";

    rs_bool result = rs_hashmap_internal_key_eq_cstr(&str1, &str2, 0, NULL);
    TEST_ASSERT_TRUE(result);
}

void test_hashmap_internal_key_eq_ptr_equal(void)
{
    int value = 42;
    void *ptr1 = &value;
    void *ptr2 = &value;

    rs_bool result = rs_hashmap_internal_key_eq_ptr(&ptr1, &ptr2, 0, NULL);
    TEST_ASSERT_TRUE(result);
}

void test_hashmap_internal_key_eq_ptr_not_equal(void)
{
    int value1 = 42;
    int value2 = 42;
    void *ptr1 = &value1;
    void *ptr2 = &value2;

    rs_bool result = rs_hashmap_internal_key_eq_ptr(&ptr1, &ptr2, 0, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_hashmap_internal_key_eq_ptr_null(void)
{
    void *ptr1 = NULL;
    void *ptr2 = NULL;

    rs_bool result = rs_hashmap_internal_key_eq_ptr(&ptr1, &ptr2, 0, NULL);
    TEST_ASSERT_TRUE(result);
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_hashmap_zero_key(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int), .allocator = allocator);

    rs_u64 key = 0;
    int value = 42;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key, &value));

    int *retrieved = rs_hashmap_get(&map, &key);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(42, *retrieved);

    rs_hashmap_destroy(&map);
}

void test_hashmap_large_values(void)
{
    rs_hashmap_t map = rs_hashmap_int_create(sizeof(rs_u64), .allocator = allocator);

    rs_u64 key1 = 0xFFFFFFFFFFFFFFFFULL;
    rs_u64 key2 = 0xFFFFFFFFFFFFFFFEULL;
    rs_u64 value1 = 0xDEADBEEFCAFEBABEULL;
    rs_u64 value2 = 0xBADC0FFEEBADBABEULL;

    rs_hashmap_insert(&map, &key1, &value1);
    rs_hashmap_insert(&map, &key2, &value2);

    rs_u64 *retrieved1 = rs_hashmap_get(&map, &key1);
    rs_u64 *retrieved2 = rs_hashmap_get(&map, &key2);

    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL_UINT64(value1, *retrieved1);
    TEST_ASSERT_EQUAL_UINT64(value2, *retrieved2);

    rs_hashmap_destroy(&map);
}

void test_hashmap_empty_string_key(void)
{
    rs_hashmap_t map = rs_hashmap_cstr_create(sizeof(int), .allocator = allocator);

    const char *key = "";
    int value = 100;

    TEST_ASSERT_EQUAL(RS_OK, rs_hashmap_insert(&map, &key, &value));

    int *retrieved = rs_hashmap_get(&map, &key);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(100, *retrieved);

    rs_hashmap_destroy(&map);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Basic tests
    RUN_TEST(test_hashmap_create_destroy);
    RUN_TEST(test_hashmap_insers_get);
    RUN_TEST(test_hashmap_insers_multiple);
    RUN_TEST(test_hashmap_update_existing);
    RUN_TEST(test_hashmap_contains);
    RUN_TEST(test_hashmap_remove);
    RUN_TEST(test_hashmap_remove_nonexistent);
    RUN_TEST(test_hashmap_clear);

    // String key tests
    RUN_TEST(test_hashmap_string_keys);
    RUN_TEST(test_hashmap_string_update);
    RUN_TEST(test_hashmap_string_remove);

    // Pointer key tests
    RUN_TEST(test_hashmap_pointer_keys);
    RUN_TEST(test_hashmap_pointer_identity);

    // Resize tests
    RUN_TEST(test_hashmap_resize_growth);

    // Iterator tests
    RUN_TEST(test_hashmap_iterator_empty);
    RUN_TEST(test_hashmap_iterator_single);
    RUN_TEST(test_hashmap_iterator_multiple);
    RUN_TEST(test_hashmap_foreach_macro);

    // Collision handling
    RUN_TEST(test_hashmap_collision_handling);

    // Tombstone tests
    RUN_TEST(test_hashmap_tombstones);

    // Custom configuration
    RUN_TEST(test_hashmap_custom_config);

    // rs_string_t key tests
    RUN_TEST(test_hashmap_rs_string_keys);
    RUN_TEST(test_hashmap_rs_string_update);

    // rs_string_view_t key tests
    RUN_TEST(test_hashmap_string_view_keys);
    RUN_TEST(test_hashmap_string_view_substrings);

    // rs_zstring_view_t key tests
    RUN_TEST(test_hashmap_zstring_view_keys);
    RUN_TEST(test_hashmap_zstring_view_from_rs_string);

    // Internal equality function tests
    RUN_TEST(test_hashmap_internal_key_eq_memcmp_equal);
    RUN_TEST(test_hashmap_internal_key_eq_memcmp_not_equal);
    RUN_TEST(test_hashmap_internal_key_eq_memcmp_struct);
    RUN_TEST(test_hashmap_internal_key_eq_cstr_equal);
    RUN_TEST(test_hashmap_internal_key_eq_cstr_not_equal);
    RUN_TEST(test_hashmap_internal_key_eq_cstr_case_sensitive);
    RUN_TEST(test_hashmap_internal_key_eq_cstr_empty);
    RUN_TEST(test_hashmap_internal_key_eq_ptr_equal);
    RUN_TEST(test_hashmap_internal_key_eq_ptr_not_equal);
    RUN_TEST(test_hashmap_internal_key_eq_ptr_null);

    // Edge cases
    RUN_TEST(test_hashmap_zero_key);
    RUN_TEST(test_hashmap_large_values);
    RUN_TEST(test_hashmap_empty_string_key);

    return UNITY_END();
}
