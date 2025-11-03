#include <rs/std/allocators/arena.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string_view.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_string_view"

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

void test_sv_from_cstr(void)
{
    const char *text = "Hello, World!";
    rs_string_view_t sv = rs_sv_from_cstr(text);

    TEST_ASSERT_NOT_NULL(sv.data);
    TEST_ASSERT_EQUAL(13, rs_sv_len(sv));
    TEST_ASSERT_EQUAL_STRING_LEN("Hello, World!", sv.data, 13);
}

void test_sv_from_buf(void)
{
    const char *buf = "Hello, World!";
    rs_string_view_t sv = rs_sv_from_buf(buf, 5); // Just "Hello"

    TEST_ASSERT_NOT_NULL(sv.data);
    TEST_ASSERT_EQUAL(5, rs_sv_len(sv));
    TEST_ASSERT_EQUAL_MEMORY("Hello", sv.data, 5);
}

void test_sv_empty(void)
{
    rs_string_view_t sv = rs_sv_empty();

    TEST_ASSERT_NULL(sv.data);
    TEST_ASSERT_EQUAL(0, rs_sv_len(sv));
    TEST_ASSERT_TRUE(rs_sv_is_empty(sv));
}

// ============================================================================
// Property tests
// ============================================================================

void test_sv_len_and_is_empty(void)
{
    rs_string_view_t empty = rs_sv_empty();
    TEST_ASSERT_EQUAL(0, rs_sv_len(empty));
    TEST_ASSERT_TRUE(rs_sv_is_empty(empty));

    rs_string_view_t sv = rs_sv_from_cstr("test");
    TEST_ASSERT_EQUAL(4, rs_sv_len(sv));
    TEST_ASSERT_FALSE(rs_sv_is_empty(sv));
}

void test_sv_at(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello");

    TEST_ASSERT_EQUAL('H', rs_sv_at(sv, 0));
    TEST_ASSERT_EQUAL('e', rs_sv_at(sv, 1));
    TEST_ASSERT_EQUAL('l', rs_sv_at(sv, 2));
    TEST_ASSERT_EQUAL('l', rs_sv_at(sv, 3));
    TEST_ASSERT_EQUAL('o', rs_sv_at(sv, 4));
}

// ============================================================================
// Slicing tests
// ============================================================================

void test_sv_slice(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    // Slice "Hello"
    rs_string_view_t hello = rs_sv_slice(sv, 0, 5);
    TEST_ASSERT_EQUAL(5, rs_sv_len(hello));
    TEST_ASSERT_EQUAL_MEMORY("Hello", hello.data, 5);

    // Slice "World"
    rs_string_view_t world = rs_sv_slice(sv, 7, 12);
    TEST_ASSERT_EQUAL(5, rs_sv_len(world));
    TEST_ASSERT_EQUAL_MEMORY("World", world.data, 5);

    // Slice beyond end (should clamp)
    rs_string_view_t clamped = rs_sv_slice(sv, 7, 100);
    TEST_ASSERT_EQUAL(6, rs_sv_len(clamped)); // "World!"
}

void test_sv_slice_from(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    rs_string_view_t from_7 = rs_sv_slice_from(sv, 7);
    TEST_ASSERT_EQUAL(6, rs_sv_len(from_7));
    TEST_ASSERT_EQUAL_MEMORY("World!", from_7.data, 6);
}

void test_sv_slice_to(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    rs_string_view_t to_5 = rs_sv_slice_to(sv, 5);
    TEST_ASSERT_EQUAL(5, rs_sv_len(to_5));
    TEST_ASSERT_EQUAL_MEMORY("Hello", to_5.data, 5);
}

void test_sv_trim_prefix(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    rs_string_view_t trimmed = rs_sv_trim_prefix_cstr(sv, "Hello, ");
    TEST_ASSERT_EQUAL(6, rs_sv_len(trimmed));
    TEST_ASSERT_EQUAL_MEMORY("World!", trimmed.data, 6);

    // Non-matching prefix (should return original)
    rs_string_view_t unchanged = rs_sv_trim_prefix_cstr(sv, "Goodbye");
    TEST_ASSERT_EQUAL(13, rs_sv_len(unchanged));
}

void test_sv_trim_suffix(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    rs_string_view_t trimmed = rs_sv_trim_suffix_cstr(sv, ", World!");
    TEST_ASSERT_EQUAL(5, rs_sv_len(trimmed));
    TEST_ASSERT_EQUAL_MEMORY("Hello", trimmed.data, 5);

    // Non-matching suffix (should return original)
    rs_string_view_t unchanged = rs_sv_trim_suffix_cstr(sv, "Universe");
    TEST_ASSERT_EQUAL(13, rs_sv_len(unchanged));
}

void test_sv_trim(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("  \t  Hello  \n  ");

    rs_string_view_t trimmed = rs_sv_trim(sv);
    TEST_ASSERT_EQUAL(5, rs_sv_len(trimmed));
    TEST_ASSERT_EQUAL_MEMORY("Hello", trimmed.data, 5);

    // Already trimmed
    rs_string_view_t no_ws = rs_sv_from_cstr("Hello");
    rs_string_view_t unchanged = rs_sv_trim(no_ws);
    TEST_ASSERT_EQUAL(5, rs_sv_len(unchanged));
}

void test_sv_trim_left(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("  \t  Hello  ");

    rs_string_view_t trimmed = rs_sv_trim_left(sv);
    TEST_ASSERT_EQUAL(7, rs_sv_len(trimmed)); // "Hello  "
    TEST_ASSERT_EQUAL_MEMORY("Hello  ", trimmed.data, 7);
}

void test_sv_trim_right(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("  Hello  \n  ");

    rs_string_view_t trimmed = rs_sv_trim_right(sv);
    TEST_ASSERT_EQUAL(7, rs_sv_len(trimmed)); // "  Hello"
    TEST_ASSERT_EQUAL_MEMORY("  Hello", trimmed.data, 7);
}

// ============================================================================
// Comparison tests
// ============================================================================

void test_sv_cmp(void)
{
    rs_string_view_t a = rs_sv_from_cstr("apple");
    rs_string_view_t b = rs_sv_from_cstr("banana");
    rs_string_view_t c = rs_sv_from_cstr("apple");

    TEST_ASSERT_LESS_THAN(0, rs_sv_cmp(a, b));    // "apple" < "banana"
    TEST_ASSERT_GREATER_THAN(0, rs_sv_cmp(b, a)); // "banana" > "apple"
    TEST_ASSERT_EQUAL(0, rs_sv_cmp(a, c));        // "apple" == "apple"
}

void test_sv_eq(void)
{
    rs_string_view_t a = rs_sv_from_cstr("test");
    rs_string_view_t b = rs_sv_from_cstr("test");
    rs_string_view_t c = rs_sv_from_cstr("different");

    TEST_ASSERT_TRUE(rs_sv_eq(a, b));
    TEST_ASSERT_FALSE(rs_sv_eq(a, c));

    // Different lengths
    rs_string_view_t shors_sv = rs_sv_from_buf("test", 3);
    TEST_ASSERT_FALSE(rs_sv_eq(a, shors_sv));
}

void test_sv_cmp_cstr(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("hello");

    TEST_ASSERT_EQUAL(0, rs_sv_cmp_cstr(sv, "hello"));
    TEST_ASSERT_LESS_THAN(0, rs_sv_cmp_cstr(sv, "world"));
    TEST_ASSERT_GREATER_THAN(0, rs_sv_cmp_cstr(sv, "abc"));
}

void test_sv_eq_cstr(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("test");

    TEST_ASSERT_TRUE(rs_sv_eq_cstr(sv, "test"));
    TEST_ASSERT_FALSE(rs_sv_eq_cstr(sv, "different"));
}

// ============================================================================
// Searching tests
// ============================================================================

void test_sv_find_char(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    TEST_ASSERT_EQUAL(0, rs_sv_find_char(sv, 'H'));
    TEST_ASSERT_EQUAL(1, rs_sv_find_char(sv, 'e'));
    TEST_ASSERT_EQUAL(7, rs_sv_find_char(sv, 'W'));
    TEST_ASSERT_EQUAL(-1, rs_sv_find_char(sv, 'x')); // Not found
}

void test_sv_find(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    TEST_ASSERT_EQUAL(0, rs_sv_find_cstr(sv, "Hello"));
    TEST_ASSERT_EQUAL(7, rs_sv_find_cstr(sv, "World"));
    TEST_ASSERT_EQUAL(5, rs_sv_find_cstr(sv, ", "));
    TEST_ASSERT_EQUAL(-1, rs_sv_find_cstr(sv, "Goodbye")); // Not found
}

void test_sv_starts_with(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    TEST_ASSERT_TRUE(rs_sv_starts_with_cstr(sv, "Hello"));
    TEST_ASSERT_TRUE(rs_sv_starts_with_cstr(sv, "H"));
    TEST_ASSERT_FALSE(rs_sv_starts_with_cstr(sv, "World"));
    TEST_ASSERT_FALSE(rs_sv_starts_with_cstr(sv, "x"));
}

void test_sv_ends_with(void)
{
    rs_string_view_t sv = rs_sv_from_cstr("Hello, World!");

    TEST_ASSERT_TRUE(rs_sv_ends_with_cstr(sv, "World!"));
    TEST_ASSERT_TRUE(rs_sv_ends_with_cstr(sv, "!"));
    TEST_ASSERT_FALSE(rs_sv_ends_with_cstr(sv, "Hello"));
    TEST_ASSERT_FALSE(rs_sv_ends_with_cstr(sv, "x"));
}

// ============================================================================
// Conversion tests
// ============================================================================

void test_sv_to_string(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_view_t sv = rs_sv_from_cstr("Hello");
    rs_string_t str = rs_sv_to_string(sv, .allocator = alloc);

    TEST_ASSERT_EQUAL(5, rs_string_len(&str));
    TEST_ASSERT_EQUAL_STRING("Hello", rs_string_cstr(&str));

    rs_arena_destroy(arena);
}

void test_sv_to_cstr(void)
{
    rs_string_view_t sv = rs_sv_from_buf("Hello", 5); // Not null-terminated!

    char *cstr = rs_sv_to_cstr(sv);
    TEST_ASSERT_NOT_NULL(cstr);
    TEST_ASSERT_EQUAL_STRING("Hello", cstr);

    free(cstr); // Uses system allocator
}

void test_sv_to_cstr_alloc(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_view_t sv = rs_sv_from_buf("World", 5);

    char *cstr = rs_sv_to_cstr(sv, .allocator = alloc);
    TEST_ASSERT_NOT_NULL(cstr);
    TEST_ASSERT_EQUAL_STRING("World", cstr);

    // No need to free - arena will clean up
    rs_arena_destroy(arena);
}

// ============================================================================
// Edge cases
// ============================================================================

void test_sv_empty_operations(void)
{
    rs_string_view_t empty = rs_sv_empty();

    // Slicing empty
    rs_string_view_t sliced = rs_sv_slice(empty, 0, 10);
    TEST_ASSERT_EQUAL(0, rs_sv_len(sliced));

    // Trimming empty
    rs_string_view_t trimmed = rs_sv_trim(empty);
    TEST_ASSERT_EQUAL(0, rs_sv_len(trimmed));

    // Searching in empty
    TEST_ASSERT_EQUAL(-1, rs_sv_find_char(empty, 'x'));
    TEST_ASSERT_EQUAL(-1, rs_sv_find_cstr(empty, "test"));
    TEST_ASSERT_FALSE(rs_sv_starts_with_cstr(empty, "x"));
    TEST_ASSERT_FALSE(rs_sv_ends_with_cstr(empty, "x"));
}

void test_sv_non_null_terminated(void)
{
    // Create a view that is NOT null-terminated
    const char *buf = "HelloWorld";
    rs_string_view_t sv = rs_sv_from_buf(buf, 5); // Just "Hello"

    TEST_ASSERT_EQUAL(5, rs_sv_len(sv));

    // Should not read past the view length
    rs_string_view_t hello = rs_sv_from_cstr("Hello");
    TEST_ASSERT_TRUE(rs_sv_eq(sv, hello));
}

int main(void)
{
    UNITY_BEGIN();

    // Creation tests
    RUN_TEST(test_sv_from_cstr);
    RUN_TEST(test_sv_from_buf);
    RUN_TEST(test_sv_empty);

    // Property tests
    RUN_TEST(test_sv_len_and_is_empty);
    RUN_TEST(test_sv_at);

    // Slicing tests
    RUN_TEST(test_sv_slice);
    RUN_TEST(test_sv_slice_from);
    RUN_TEST(test_sv_slice_to);
    RUN_TEST(test_sv_trim_prefix);
    RUN_TEST(test_sv_trim_suffix);
    RUN_TEST(test_sv_trim);
    RUN_TEST(test_sv_trim_left);
    RUN_TEST(test_sv_trim_right);

    // Comparison tests
    RUN_TEST(test_sv_cmp);
    RUN_TEST(test_sv_eq);
    RUN_TEST(test_sv_cmp_cstr);
    RUN_TEST(test_sv_eq_cstr);

    // Searching tests
    RUN_TEST(test_sv_find_char);
    RUN_TEST(test_sv_find);
    RUN_TEST(test_sv_starts_with);
    RUN_TEST(test_sv_ends_with);

    // Conversion tests
    RUN_TEST(test_sv_to_string);
    RUN_TEST(test_sv_to_cstr);
    RUN_TEST(test_sv_to_cstr_alloc);

    // Edge cases
    RUN_TEST(test_sv_empty_operations);
    RUN_TEST(test_sv_non_null_terminated);

    return UNITY_END();
}
