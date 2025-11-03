#include <rs/std/allocators/arena.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/zstring_view.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_zstring_view"

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
// Creation tests
// ============================================================================

void test_zsv_from_cstr(void)
{
    const char *text = "Hello, World!";
    rs_zstring_view_t zsv = rs_zsv_from_cstr(text);

    TEST_ASSERT_NOT_NULL(zsv.data);
    TEST_ASSERT_EQUAL(13, zsv.len);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", rs_zsv_cstr(zsv));
    TEST_ASSERT_EQUAL_PTR(text, zsv.data);
}

void test_zsv_from_cstr_null(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr(NULL);

    TEST_ASSERT_NULL(zsv.data);
    TEST_ASSERT_EQUAL(0, zsv.len);
    TEST_ASSERT_TRUE(rs_zsv_is_empty(zsv));
}

void test_zsv_from_string(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("test string", .allocator = alloc);
    rs_zstring_view_t zsv = rs_zsv_from_string(&str);

    TEST_ASSERT_NOT_NULL(zsv.data);
    TEST_ASSERT_EQUAL(11, zsv.len);
    TEST_ASSERT_EQUAL_STRING("test string", rs_zsv_cstr(zsv));

    rs_arena_destroy(arena);
}

void test_zsv_empty(void)
{
    rs_zstring_view_t zsv = rs_zsv_empty();

    TEST_ASSERT_NOT_NULL(zsv.data);
    TEST_ASSERT_EQUAL(0, zsv.len);
    TEST_ASSERT_TRUE(rs_zsv_is_empty(zsv));
    TEST_ASSERT_EQUAL_STRING("", rs_zsv_cstr(zsv));
}

// ============================================================================
// Conversion tests
// ============================================================================

void test_zsv_to_sv(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("Hello");
    rs_string_view_t sv = rs_zsv_to_sv(zsv);

    TEST_ASSERT_EQUAL_PTR(zsv.data, sv.data);
    TEST_ASSERT_EQUAL(zsv.len, sv.len);
}

void test_zsv_cstr(void)
{
    const char *original = "test";
    rs_zstring_view_t zsv = rs_zsv_from_cstr(original);
    const char *cstr = rs_zsv_cstr(zsv);

    TEST_ASSERT_EQUAL_PTR(original, cstr);
    TEST_ASSERT_EQUAL_STRING(original, cstr);
}

void test_zsv_cstr_null_safety(void)
{
    rs_zstring_view_t zsv = {NULL, 0};
    const char *cstr = rs_zsv_cstr(zsv);

    // Should return empty string, not NULL
    TEST_ASSERT_NOT_NULL(cstr);
    TEST_ASSERT_EQUAL_STRING("", cstr);
}

// ============================================================================
// Properties tests
// ============================================================================

void test_zsv_properties(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("Hello");

    TEST_ASSERT_EQUAL(5, rs_zsv_len(zsv));
    TEST_ASSERT_FALSE(rs_zsv_is_empty(zsv));
    TEST_ASSERT_EQUAL('H', rs_zsv_at(zsv, 0));
    TEST_ASSERT_EQUAL('o', rs_zsv_at(zsv, 4));
}

// ============================================================================
// Comparison tests
// ============================================================================

void test_zsv_eq(void)
{
    rs_zstring_view_t a = rs_zsv_from_cstr("hello");
    rs_zstring_view_t b = rs_zsv_from_cstr("hello");
    rs_zstring_view_t c = rs_zsv_from_cstr("world");

    TEST_ASSERT_TRUE(rs_zsv_eq(a, b));
    TEST_ASSERT_FALSE(rs_zsv_eq(a, c));
}

void test_zsv_eq_cstr(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("hello");

    TEST_ASSERT_TRUE(rs_zsv_eq_cstr(zsv, "hello"));
    TEST_ASSERT_FALSE(rs_zsv_eq_cstr(zsv, "world"));
}

void test_zsv_cmp(void)
{
    rs_zstring_view_t a = rs_zsv_from_cstr("apple");
    rs_zstring_view_t b = rs_zsv_from_cstr("banana");
    rs_zstring_view_t c = rs_zsv_from_cstr("apple");

    TEST_ASSERT_TRUE(rs_zsv_cmp(a, b) < 0);
    TEST_ASSERT_TRUE(rs_zsv_cmp(b, a) > 0);
    TEST_ASSERT_EQUAL(0, rs_zsv_cmp(a, c));
}

// ============================================================================
// Searching tests
// ============================================================================

void test_zsv_find_char(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("Hello, World!");

    TEST_ASSERT_EQUAL(0, rs_zsv_find_char(zsv, 'H'));
    TEST_ASSERT_EQUAL(7, rs_zsv_find_char(zsv, 'W'));
    TEST_ASSERT_EQUAL(-1, rs_zsv_find_char(zsv, 'x'));
}

void test_zsv_starts_with(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("Hello, World!");

    TEST_ASSERT_TRUE(rs_zsv_starts_with_cstr(zsv, "Hello"));
    TEST_ASSERT_TRUE(rs_zsv_starts_with_cstr(zsv, ""));
    TEST_ASSERT_FALSE(rs_zsv_starts_with_cstr(zsv, "World"));
}

void test_zsv_ends_with(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("Hello, World!");

    TEST_ASSERT_TRUE(rs_zsv_ends_with_cstr(zsv, "World!"));
    TEST_ASSERT_TRUE(rs_zsv_ends_with_cstr(zsv, ""));
    TEST_ASSERT_FALSE(rs_zsv_ends_with_cstr(zsv, "Hello"));
}

void test_zsv_contains(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("Hello, World!");

    TEST_ASSERT_TRUE(rs_zsv_contains_cstr(zsv, "World"));
    TEST_ASSERT_TRUE(rs_zsv_contains_cstr(zsv, ", "));
    TEST_ASSERT_FALSE(rs_zsv_contains_cstr(zsv, "xyz"));
}

// ============================================================================
// C API interop tests
// ============================================================================

void test_zsv_c_api_usage(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("test");

    // Should be usable directly with C string functions
    TEST_ASSERT_EQUAL(4, strlen(rs_zsv_cstr(zsv)));
    TEST_ASSERT_EQUAL(0, strcmp(rs_zsv_cstr(zsv), "test"));
}

void test_zsv_format_specifiers(void)
{
    rs_zstring_view_t zsv = rs_zsv_from_cstr("Hello");

    // Both format specifiers should work
    char buf1[64];
    char buf2[64];

    snprintf(buf1, sizeof(buf1), RS_ZSV_FMT, RS_ZSV_ARG(zsv));
    snprintf(buf2, sizeof(buf2), RS_ZSV_FMT_LEN, RS_ZSV_ARG_LEN(zsv));

    TEST_ASSERT_EQUAL_STRING("Hello", buf1);
    TEST_ASSERT_EQUAL_STRING("Hello", buf2);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Creation
    RUN_TEST(test_zsv_from_cstr);
    RUN_TEST(test_zsv_from_cstr_null);
    RUN_TEST(test_zsv_from_string);
    RUN_TEST(test_zsv_empty);

    // Conversion
    RUN_TEST(test_zsv_to_sv);
    RUN_TEST(test_zsv_cstr);
    RUN_TEST(test_zsv_cstr_null_safety);

    // Properties
    RUN_TEST(test_zsv_properties);

    // Comparison
    RUN_TEST(test_zsv_eq);
    RUN_TEST(test_zsv_eq_cstr);
    RUN_TEST(test_zsv_cmp);

    // Searching
    RUN_TEST(test_zsv_find_char);
    RUN_TEST(test_zsv_starts_with);
    RUN_TEST(test_zsv_ends_with);
    RUN_TEST(test_zsv_contains);

    // C API interop
    RUN_TEST(test_zsv_c_api_usage);
    RUN_TEST(test_zsv_format_specifiers);

    return UNITY_END();
}
