#include <rs/std/allocators/arena.h>
#include <rs/std/containers/array.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_utils.h>
#include <rs/std/string/string_view.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_string_utils"

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
// Replace Tests
// ============================================================================

void test_string_replace_basic(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test basic replacement - SSO
    rs_string_t str = rs_string_from_cstr("hello world", .allocator = alloc);
    rs_result_t result = rs_string_replace(&str, rs_sv_from_cstr("world"), rs_sv_from_cstr("universe"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL_STRING("hello universe", rs_string_cstr(&str));
    rs_string_destroy(&str);

    // Test replacement with same length
    rs_string_t str2 = rs_string_from_cstr("hello world", .allocator = alloc);
    result = rs_string_replace(&str2, rs_sv_from_cstr("world"), rs_sv_from_cstr("earth"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL_STRING("hello earth", rs_string_cstr(&str2));
    rs_string_destroy(&str2);

    // Test replacement with shorter string
    rs_string_t str3 = rs_string_from_cstr("hello world", .allocator = alloc);
    result = rs_string_replace(&str3, rs_sv_from_cstr("world"), rs_sv_from_cstr("hi"));
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL_STRING("hello hi", rs_string_cstr(&str3));
    rs_string_destroy(&str3);

    rs_arena_destroy(arena);
}

void test_string_replace_not_found(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("hello world", .allocator = alloc);
    rs_result_t result = rs_string_replace(&str, rs_sv_from_cstr("foo"), rs_sv_from_cstr("bar"));
    TEST_ASSERT_EQUAL(RS_ERR_NOTFOUND, result);
    TEST_ASSERT_EQUAL_STRING("hello world", rs_string_cstr(&str));
    rs_string_destroy(&str);

    rs_arena_destroy(arena);
}

void test_string_replace_empty(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Empty old substring should fail
    rs_string_t str = rs_string_from_cstr("hello", .allocator = alloc);
    rs_result_t result = rs_string_replace(&str, rs_sv_from_cstr(""), rs_sv_from_cstr("x"));
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, result);
    rs_string_destroy(&str);

    rs_arena_destroy(arena);
}

void test_string_replace_all_basic(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test replace all - SSO
    rs_string_t str = rs_string_from_cstr("foo bar foo baz foo", .allocator = alloc);
    rs_size_t count = rs_string_replace_all(&str, rs_sv_from_cstr("foo"), rs_sv_from_cstr("qux"));
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL_STRING("qux bar qux baz qux", rs_string_cstr(&str));
    rs_string_destroy(&str);

    // Test replace all with longer string
    rs_string_t str2 = rs_string_from_cstr("a b a c a", .allocator = alloc);
    count = rs_string_replace_all(&str2, rs_sv_from_cstr("a"), rs_sv_from_cstr("foo"));
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL_STRING("foo b foo c foo", rs_string_cstr(&str2));
    rs_string_destroy(&str2);

    // Test replace all with shorter string
    rs_string_t str3 = rs_string_from_cstr("hello world hello universe hello", .allocator = alloc);
    count = rs_string_replace_all(&str3, rs_sv_from_cstr("hello"), rs_sv_from_cstr("hi"));
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL_STRING("hi world hi universe hi", rs_string_cstr(&str3));
    rs_string_destroy(&str3);

    rs_arena_destroy(arena);
}

void test_string_replace_all_none_found(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("hello world", .allocator = alloc);
    rs_size_t count = rs_string_replace_all(&str, rs_sv_from_cstr("foo"), rs_sv_from_cstr("bar"));
    TEST_ASSERT_EQUAL(0, count);
    TEST_ASSERT_EQUAL_STRING("hello world", rs_string_cstr(&str));
    rs_string_destroy(&str);

    rs_arena_destroy(arena);
}

void test_string_replace_all_delete(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Replace with empty string (deletion)
    rs_string_t str = rs_string_from_cstr("foo bar foo baz", .allocator = alloc);
    rs_size_t count = rs_string_replace_all(&str, rs_sv_from_cstr("foo "), rs_sv_from_cstr(""));
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("bar baz", rs_string_cstr(&str));
    rs_string_destroy(&str);

    rs_arena_destroy(arena);
}

// ============================================================================
// Split Tests
// ============================================================================

void test_string_split_basic(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("hello,world,foo,bar", .allocator = alloc);
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t), .allocator = alloc);

    rs_result_t result = rs_string_split(&str, rs_sv_from_cstr(","), false, &parts);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(4, rs_array_len(&parts));

    rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
    TEST_ASSERT_TRUE(rs_sv_eq(views[0], rs_sv_from_cstr("hello")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[1], rs_sv_from_cstr("world")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[2], rs_sv_from_cstr("foo")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[3], rs_sv_from_cstr("bar")));

    rs_array_destroy(&parts);
    rs_string_destroy(&str);
    rs_arena_destroy(arena);
}

void test_string_split_empty_parts(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test with empty parts kept
    rs_string_t str = rs_string_from_cstr("a,,b,,c", .allocator = alloc);
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t), .allocator = alloc);

    rs_result_t result = rs_string_split(&str, rs_sv_from_cstr(","), false, &parts);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(5, rs_array_len(&parts));

    rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
    TEST_ASSERT_TRUE(rs_sv_eq(views[0], rs_sv_from_cstr("a")));
    TEST_ASSERT_TRUE(rs_sv_is_empty(views[1]));
    TEST_ASSERT_TRUE(rs_sv_eq(views[2], rs_sv_from_cstr("b")));
    TEST_ASSERT_TRUE(rs_sv_is_empty(views[3]));
    TEST_ASSERT_TRUE(rs_sv_eq(views[4], rs_sv_from_cstr("c")));

    rs_array_destroy(&parts);
    rs_string_destroy(&str);
    rs_arena_destroy(arena);
}

void test_string_split_skip_empty(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test with empty parts skipped
    rs_string_t str = rs_string_from_cstr("a,,b,,c", .allocator = alloc);
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t), .allocator = alloc);

    rs_result_t result = rs_string_split(&str, rs_sv_from_cstr(","), true, &parts);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(3, rs_array_len(&parts));

    rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
    TEST_ASSERT_TRUE(rs_sv_eq(views[0], rs_sv_from_cstr("a")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[1], rs_sv_from_cstr("b")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[2], rs_sv_from_cstr("c")));

    rs_array_destroy(&parts);
    rs_string_destroy(&str);
    rs_arena_destroy(arena);
}

void test_string_split_multi_char_delim(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("hello::world::foo", .allocator = alloc);
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t), .allocator = alloc);

    rs_result_t result = rs_string_split(&str, rs_sv_from_cstr("::"), false, &parts);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(3, rs_array_len(&parts));

    rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
    TEST_ASSERT_TRUE(rs_sv_eq(views[0], rs_sv_from_cstr("hello")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[1], rs_sv_from_cstr("world")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[2], rs_sv_from_cstr("foo")));

    rs_array_destroy(&parts);
    rs_string_destroy(&str);
    rs_arena_destroy(arena);
}

void test_string_split_no_delimiter(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("hello", .allocator = alloc);
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t), .allocator = alloc);

    rs_result_t result = rs_string_split(&str, rs_sv_from_cstr(","), false, &parts);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(1, rs_array_len(&parts));

    rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
    TEST_ASSERT_TRUE(rs_sv_eq(views[0], rs_sv_from_cstr("hello")));

    rs_array_destroy(&parts);
    rs_string_destroy(&str);
    rs_arena_destroy(arena);
}

void test_string_split_trailing_delimiter(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test trailing delimiter with empty parts kept
    rs_string_t str = rs_string_from_cstr("a,b,c,", .allocator = alloc);
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t), .allocator = alloc);

    rs_result_t result = rs_string_split(&str, rs_sv_from_cstr(","), false, &parts);
    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(4, rs_array_len(&parts));

    rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
    TEST_ASSERT_TRUE(rs_sv_eq(views[0], rs_sv_from_cstr("a")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[1], rs_sv_from_cstr("b")));
    TEST_ASSERT_TRUE(rs_sv_eq(views[2], rs_sv_from_cstr("c")));
    TEST_ASSERT_TRUE(rs_sv_is_empty(views[3]));

    rs_array_destroy(&parts);
    rs_string_destroy(&str);
    rs_arena_destroy(arena);
}

// ============================================================================
// Join Tests
// ============================================================================

void test_string_join_basic(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_view_t parts[] = {rs_sv_from_cstr("hello"), rs_sv_from_cstr("world"), rs_sv_from_cstr("foo"),
                                rs_sv_from_cstr("bar")};

    rs_string_t result = rs_string_join(parts, 4, rs_sv_from_cstr(","), .allocator = alloc);
    TEST_ASSERT_EQUAL_STRING("hello,world,foo,bar", rs_string_cstr(&result));
    rs_string_destroy(&result);

    rs_arena_destroy(arena);
}

void test_string_join_empty_delimiter(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_view_t parts[] = {rs_sv_from_cstr("hello"), rs_sv_from_cstr("world")};

    rs_string_t result = rs_string_join(parts, 2, rs_sv_from_cstr(""), .allocator = alloc);
    TEST_ASSERT_EQUAL_STRING("helloworld", rs_string_cstr(&result));
    rs_string_destroy(&result);

    rs_arena_destroy(arena);
}

void test_string_join_multi_char_delimiter(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_view_t parts[] = {rs_sv_from_cstr("hello"), rs_sv_from_cstr("world"), rs_sv_from_cstr("foo")};

    rs_string_t result = rs_string_join(parts, 3, rs_sv_from_cstr(" :: "), .allocator = alloc);
    TEST_ASSERT_EQUAL_STRING("hello :: world :: foo", rs_string_cstr(&result));
    rs_string_destroy(&result);

    rs_arena_destroy(arena);
}

void test_string_join_single_part(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_view_t parts[] = {rs_sv_from_cstr("hello")};

    rs_string_t result = rs_string_join(parts, 1, rs_sv_from_cstr(","), .allocator = alloc);
    TEST_ASSERT_EQUAL_STRING("hello", rs_string_cstr(&result));
    rs_string_destroy(&result);

    rs_arena_destroy(arena);
}

void test_string_join_empty_parts(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_view_t parts[] = {rs_sv_from_cstr("a"), rs_sv_from_cstr(""), rs_sv_from_cstr("b"), rs_sv_from_cstr(""),
                                rs_sv_from_cstr("c")};

    rs_string_t result = rs_string_join(parts, 5, rs_sv_from_cstr(","), .allocator = alloc);
    TEST_ASSERT_EQUAL_STRING("a,,b,,c", rs_string_cstr(&result));
    rs_string_destroy(&result);

    rs_arena_destroy(arena);
}

void test_string_join_zero_parts(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t result = rs_string_join(NULL, 0, rs_sv_from_cstr(","), .allocator = alloc);
    TEST_ASSERT_TRUE(rs_string_is_empty(&result));
    rs_string_destroy(&result);

    rs_arena_destroy(arena);
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_string_split_join_roundtrip(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    const char *original = "hello,world,foo,bar";
    rs_string_t str = rs_string_from_cstr(original, .allocator = alloc);
    rs_array_t parts = rs_array_create(sizeof(rs_string_view_t), .allocator = alloc);

    // Split
    rs_result_t result = rs_string_split(&str, rs_sv_from_cstr(","), false, &parts);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Join back
    rs_string_view_t *views = (rs_string_view_t *)rs_array_data(&parts);
    rs_string_t joined = rs_string_join(views, rs_array_len(&parts), rs_sv_from_cstr(","), .allocator = alloc);

    TEST_ASSERT_EQUAL_STRING(original, rs_string_cstr(&joined));

    rs_string_destroy(&joined);
    rs_array_destroy(&parts);
    rs_string_destroy(&str);
    rs_arena_destroy(arena);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Replace tests
    RUN_TEST(test_string_replace_basic);
    RUN_TEST(test_string_replace_not_found);
    RUN_TEST(test_string_replace_empty);
    RUN_TEST(test_string_replace_all_basic);
    RUN_TEST(test_string_replace_all_none_found);
    RUN_TEST(test_string_replace_all_delete);

    // Split tests
    RUN_TEST(test_string_split_basic);
    RUN_TEST(test_string_split_empty_parts);
    RUN_TEST(test_string_split_skip_empty);
    RUN_TEST(test_string_split_multi_char_delim);
    RUN_TEST(test_string_split_no_delimiter);
    RUN_TEST(test_string_split_trailing_delimiter);

    // Join tests
    RUN_TEST(test_string_join_basic);
    RUN_TEST(test_string_join_empty_delimiter);
    RUN_TEST(test_string_join_multi_char_delimiter);
    RUN_TEST(test_string_join_single_part);
    RUN_TEST(test_string_join_empty_parts);
    RUN_TEST(test_string_join_zero_parts);

    // Integration tests
    RUN_TEST(test_string_split_join_roundtrip);

    return UNITY_END();
}
