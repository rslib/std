#include <rs/std/allocators/arena.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_string"

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

void test_string_create(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_create(.allocator = alloc);

    TEST_ASSERT_EQUAL(0, rs_string_len(&str));
    TEST_ASSERT_TRUE(rs_string_is_empty(&str));
    TEST_ASSERT_TRUE(rs_string_is_small(&str)); // Empty string uses SSO

    rs_arena_destroy(arena);
}

void test_string_new(void)
{
    rs_string_t str = rs_string_create();

    TEST_ASSERT_EQUAL(0, rs_string_len(&str));
    TEST_ASSERT_TRUE(rs_string_is_empty(&str));
    TEST_ASSERT_TRUE(rs_string_is_small(&str));

    rs_string_destroy(&str);
}

void test_string_from_cstr_small(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Small string (should use SSO)
    rs_string_t str = rs_string_from_cstr("Hello", .allocator = alloc);

    TEST_ASSERT_EQUAL(5, rs_string_len(&str));
    TEST_ASSERT_FALSE(rs_string_is_empty(&str));
    TEST_ASSERT_TRUE(rs_string_is_small(&str));
    TEST_ASSERT_EQUAL_STRING("Hello", rs_string_cstr(&str));

    rs_arena_destroy(arena);
}

void test_string_from_cstr_large(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Large string (exceeds SSO capacity)
    const char *long_str = "This is a very long string that exceeds the SSO capacity";
    rs_string_t str = rs_string_from_cstr(long_str, .allocator = alloc);

    TEST_ASSERT_EQUAL(strlen(long_str), rs_string_len(&str));
    TEST_ASSERT_FALSE(rs_string_is_small(&str)); // Should be large
    TEST_ASSERT_EQUAL_STRING(long_str, rs_string_cstr(&str));

    rs_arena_destroy(arena);
}

void test_string_from(void)
{
    rs_string_t str = rs_string_from_cstr("test");

    TEST_ASSERT_EQUAL(4, rs_string_len(&str));
    TEST_ASSERT_EQUAL_STRING("test", rs_string_cstr(&str));

    rs_string_destroy(&str);
}

void test_string_from_buf(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Buffer with embedded null (string can contain null bytes)
    const char buf[] = {'H', 'e', 'l', '\0', 'l', 'o'};
    rs_string_t str = rs_string_from_buf(buf, 6, .allocator = alloc);

    TEST_ASSERT_EQUAL(6, rs_string_len(&str));
    TEST_ASSERT_EQUAL_MEMORY(buf, rs_string_cstr(&str), 6);

    rs_arena_destroy(arena);
}

void test_string_with_capacity(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_create(.initial_capacity = 100, .allocator = alloc);

    TEST_ASSERT_EQUAL(0, rs_string_len(&str));
    TEST_ASSERT_GREATER_OR_EQUAL(100, rs_string_cap(&str));
    TEST_ASSERT_FALSE(rs_string_is_small(&str)); // Large capacity

    rs_arena_destroy(arena);
}

void test_string_clone_small(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str1 = rs_string_from_cstr("Hello", .allocator = alloc);
    rs_string_t str2 = rs_string_clone(&str1);

    TEST_ASSERT_EQUAL(rs_string_len(&str1), rs_string_len(&str2));
    TEST_ASSERT_EQUAL_STRING(rs_string_cstr(&str1), rs_string_cstr(&str2));
    TEST_ASSERT_TRUE(rs_string_is_small(&str2));

    // Modify clone, should not affect original
    rs_string_push_cstr(&str2, " World");
    TEST_ASSERT_EQUAL_STRING("Hello", rs_string_cstr(&str1));
    TEST_ASSERT_EQUAL_STRING("Hello World", rs_string_cstr(&str2));

    rs_arena_destroy(arena);
}

void test_string_clone_large(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    const char *long_str = "This is a very long string that exceeds the SSO capacity";
    rs_string_t str1 = rs_string_from_cstr(long_str, .allocator = alloc);
    rs_string_t str2 = rs_string_clone(&str1);

    TEST_ASSERT_EQUAL(rs_string_len(&str1), rs_string_len(&str2));
    TEST_ASSERT_EQUAL_STRING(rs_string_cstr(&str1), rs_string_cstr(&str2));
    TEST_ASSERT_FALSE(rs_string_is_small(&str2));

    rs_arena_destroy(arena);
}

// ============================================================================
// SSO threshold tests
// ============================================================================

void test_string_sso_threshold(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Create strings of varying lengths to test SSO boundary
    char buf[50];

    // Just under SSO capacity
    memset(buf, 'a', RS_STRING_SSO_CAP);
    buf[RS_STRING_SSO_CAP] = '\0';
    rs_string_t small = rs_string_from_cstr(buf, .allocator = alloc);
    TEST_ASSERT_TRUE(rs_string_is_small(&small));
    TEST_ASSERT_EQUAL(RS_STRING_SSO_CAP, rs_string_len(&small));

    // Just over SSO capacity
    memset(buf, 'b', RS_STRING_SSO_CAP + 1);
    buf[RS_STRING_SSO_CAP + 1] = '\0';
    rs_string_t large = rs_string_from_cstr(buf, .allocator = alloc);
    TEST_ASSERT_FALSE(rs_string_is_small(&large));
    TEST_ASSERT_EQUAL(RS_STRING_SSO_CAP + 1, rs_string_len(&large));

    rs_arena_destroy(arena);
}

void test_string_sso_to_large_transition(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Start with small string
    rs_string_t str = rs_string_from_cstr("small", .allocator = alloc);
    TEST_ASSERT_TRUE(rs_string_is_small(&str));

    // Grow it beyond SSO capacity
    char long_suffix[50];
    memset(long_suffix, 'x', 40);
    long_suffix[40] = '\0';
    rs_string_push_cstr(&str, long_suffix);

    // Should now be large
    TEST_ASSERT_FALSE(rs_string_is_small(&str));
    TEST_ASSERT_EQUAL(5 + 40, rs_string_len(&str));

    rs_arena_destroy(arena);
}

// ============================================================================
// Modification tests
// ============================================================================

void test_string_reserve(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello", .allocator = alloc);
    rs_size_t current_len = rs_string_len(&str);

    rs_result_t res = rs_string_reserve(&str, 100);
    TEST_ASSERT_EQUAL(RS_OK, res);

    // After reserving space for 100 additional bytes,
    // capacity should be at least current_len + 100
    TEST_ASSERT_GREATER_OR_EQUAL(current_len + 100, rs_string_cap(&str));

    rs_arena_destroy(arena);
}

void test_string_push_cstr(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello", .allocator = alloc);
    rs_string_push_cstr(&str, " World");

    TEST_ASSERT_EQUAL(11, rs_string_len(&str));
    TEST_ASSERT_EQUAL_STRING("Hello World", rs_string_cstr(&str));

    rs_arena_destroy(arena);
}

void test_string_push_buf(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello", .allocator = alloc);

    const char buf[] = {' ', 'W', 'o', 'r', 'l', 'd'};
    rs_string_push_buf(&str, buf, 6);

    TEST_ASSERT_EQUAL(11, rs_string_len(&str));
    TEST_ASSERT_EQUAL_STRING("Hello World", rs_string_cstr(&str));

    rs_arena_destroy(arena);
}

void test_string_push_char(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("test", .allocator = alloc);

    rs_string_push_char(&str, '!');
    TEST_ASSERT_EQUAL(5, rs_string_len(&str));
    TEST_ASSERT_EQUAL_STRING("test!", rs_string_cstr(&str));

    rs_arena_destroy(arena);
}

void test_string_push_string(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str1 = rs_string_from_cstr("Hello", .allocator = alloc);
    rs_string_t str2 = rs_string_from_cstr(" World", .allocator = alloc);

    rs_string_push_string(&str1, &str2);

    TEST_ASSERT_EQUAL(11, rs_string_len(&str1));
    TEST_ASSERT_EQUAL_STRING("Hello World", rs_string_cstr(&str1));

    rs_arena_destroy(arena);
}

void test_string_clear(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello", .allocator = alloc);
    rs_size_t cap_before = rs_string_cap(&str);

    rs_string_clear(&str);

    TEST_ASSERT_EQUAL(0, rs_string_len(&str));
    TEST_ASSERT_TRUE(rs_string_is_empty(&str));
    TEST_ASSERT_EQUAL(cap_before, rs_string_cap(&str)); // Capacity unchanged

    rs_arena_destroy(arena);
}

void test_string_truncate(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello, World!", .allocator = alloc);

    rs_string_truncate(&str, 5);
    TEST_ASSERT_EQUAL(5, rs_string_len(&str));
    TEST_ASSERT_EQUAL_STRING("Hello", rs_string_cstr(&str));

    // Truncate to 0
    rs_string_truncate(&str, 0);
    TEST_ASSERT_EQUAL(0, rs_string_len(&str));
    TEST_ASSERT_TRUE(rs_string_is_empty(&str));

    rs_arena_destroy(arena);
}

void test_string_format(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_create(.allocator = alloc);

    rs_string_format(&str, "Number: %d, String: %s", 42, "test");
    TEST_ASSERT_EQUAL_STRING("Number: 42, String: test", rs_string_cstr(&str));

    // Format again (replaces content)
    rs_string_format(&str, "New: %d", 123);
    TEST_ASSERT_EQUAL_STRING("New: 123", rs_string_cstr(&str));

    rs_arena_destroy(arena);
}

// ============================================================================
// Comparison tests
// ============================================================================

void test_string_cmp(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t a = rs_string_from_cstr("apple", .allocator = alloc);
    rs_string_t b = rs_string_from_cstr("banana", .allocator = alloc);
    rs_string_t c = rs_string_from_cstr("apple", .allocator = alloc);

    TEST_ASSERT_LESS_THAN(0, rs_string_cmp(&a, &b));    // "apple" < "banana"
    TEST_ASSERT_GREATER_THAN(0, rs_string_cmp(&b, &a)); // "banana" > "apple"
    TEST_ASSERT_EQUAL(0, rs_string_cmp(&a, &c));        // "apple" == "apple"

    rs_arena_destroy(arena);
}

void test_string_eq(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t a = rs_string_from_cstr("test", .allocator = alloc);
    rs_string_t b = rs_string_from_cstr("test", .allocator = alloc);
    rs_string_t c = rs_string_from_cstr("different", .allocator = alloc);

    TEST_ASSERT_TRUE(rs_string_eq(&a, &b));
    TEST_ASSERT_FALSE(rs_string_eq(&a, &c));

    rs_arena_destroy(arena);
}

void test_string_cmp_cstr(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("hello", .allocator = alloc);

    TEST_ASSERT_EQUAL(0, rs_string_cmp_cstr(&str, "hello"));
    TEST_ASSERT_LESS_THAN(0, rs_string_cmp_cstr(&str, "world"));
    TEST_ASSERT_GREATER_THAN(0, rs_string_cmp_cstr(&str, "abc"));

    rs_arena_destroy(arena);
}

void test_string_eq_cstr(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("test", .allocator = alloc);

    TEST_ASSERT_TRUE(rs_string_eq_cstr(&str, "test"));
    TEST_ASSERT_FALSE(rs_string_eq_cstr(&str, "different"));

    rs_arena_destroy(arena);
}

// ============================================================================
// Searching tests
// ============================================================================

void test_string_find_char(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello, World!", .allocator = alloc);

    TEST_ASSERT_EQUAL(0, rs_string_find_char(&str, 'H'));
    TEST_ASSERT_EQUAL(1, rs_string_find_char(&str, 'e'));
    TEST_ASSERT_EQUAL(7, rs_string_find_char(&str, 'W'));
    TEST_ASSERT_EQUAL(-1, rs_string_find_char(&str, 'x')); // Not found

    rs_arena_destroy(arena);
}

void test_string_find(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello, World!", .allocator = alloc);

    TEST_ASSERT_EQUAL(0, rs_string_find(&str, "Hello"));
    TEST_ASSERT_EQUAL(7, rs_string_find(&str, "World"));
    TEST_ASSERT_EQUAL(5, rs_string_find(&str, ", "));
    TEST_ASSERT_EQUAL(-1, rs_string_find(&str, "Goodbye")); // Not found

    rs_arena_destroy(arena);
}

void test_string_starts_with(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello, World!", .allocator = alloc);

    TEST_ASSERT_TRUE(rs_string_starts_with(&str, "Hello"));
    TEST_ASSERT_TRUE(rs_string_starts_with(&str, "H"));
    TEST_ASSERT_FALSE(rs_string_starts_with(&str, "World"));
    TEST_ASSERT_FALSE(rs_string_starts_with(&str, "x"));

    rs_arena_destroy(arena);
}

void test_string_ends_with(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_from_cstr("Hello, World!", .allocator = alloc);

    TEST_ASSERT_TRUE(rs_string_ends_with(&str, "World!"));
    TEST_ASSERT_TRUE(rs_string_ends_with(&str, "!"));
    TEST_ASSERT_FALSE(rs_string_ends_with(&str, "Hello"));
    TEST_ASSERT_FALSE(rs_string_ends_with(&str, "x"));

    rs_arena_destroy(arena);
}

// ============================================================================
// Edge cases & stress tests
// ============================================================================

void test_string_empty_operations(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t empty = rs_string_create(.allocator = alloc);

    // Operations on empty string
    TEST_ASSERT_EQUAL(-1, rs_string_find_char(&empty, 'x'));
    TEST_ASSERT_EQUAL(-1, rs_string_find(&empty, "test"));
    TEST_ASSERT_FALSE(rs_string_starts_with(&empty, "x"));
    TEST_ASSERT_FALSE(rs_string_ends_with(&empty, "x"));

    // Appending to empty
    rs_string_push_cstr(&empty, "test");
    TEST_ASSERT_EQUAL_STRING("test", rs_string_cstr(&empty));

    rs_arena_destroy(arena);
}

void test_string_null_bytes(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // String with embedded null bytes
    const char buf[] = {'H', 'e', 'l', '\0', 'l', 'o', '\0'};
    rs_string_t str = rs_string_from_buf(buf, 7, .allocator = alloc);

    TEST_ASSERT_EQUAL(7, rs_string_len(&str));
    TEST_ASSERT_EQUAL_MEMORY(buf, rs_string_cstr(&str), 7);

    // Length should be accurate despite nulls
    TEST_ASSERT_EQUAL('H', rs_string_cstr(&str)[0]);
    TEST_ASSERT_EQUAL('\0', rs_string_cstr(&str)[3]);
    TEST_ASSERT_EQUAL('l', rs_string_cstr(&str)[4]);

    rs_arena_destroy(arena);
}

void test_string_many_small_appends(void)
{
    rs_arena_t *arena = rs_arena_create(1024);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    rs_string_t str = rs_string_create(.allocator = alloc);

    // Many small appends
    for (int i = 0; i < 100; i++) {
        rs_string_push_char(&str, 'a');
    }

    TEST_ASSERT_EQUAL(100, rs_string_len(&str));
    TEST_ASSERT_FALSE(rs_string_is_small(&str)); // Should have grown to large

    rs_arena_destroy(arena);
}

void test_string_system_allocator(void)
{
    // Test using system allocator (rs_string_new and rs_string_from)
    rs_string_t str = rs_string_create();
    rs_string_push_cstr(&str, "test");

    TEST_ASSERT_EQUAL(4, rs_string_len(&str));
    TEST_ASSERT_EQUAL_STRING("test", rs_string_cstr(&str));

    rs_string_destroy(&str);

    // rs_string_from also uses system allocator
    rs_string_t str2 = rs_string_from_cstr("hello");
    TEST_ASSERT_EQUAL_STRING("hello", rs_string_cstr(&str2));
    rs_string_destroy(&str2);
}

// ============================================================================
// Manipulation Tests
// ============================================================================

void test_string_trim(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test trim both sides - SSO
    rs_string_t str = rs_string_from_cstr("  hello world  ", .allocator = alloc);
    rs_string_trim(&str);
    TEST_ASSERT_EQUAL_STRING("hello world", rs_string_cstr(&str));
    TEST_ASSERT_EQUAL(11, rs_string_len(&str));
    rs_string_destroy(&str);

    // Test trim both sides - large
    rs_string_t large =
        rs_string_from_cstr("    this is a much longer string with lots of whitespace    ", .allocator = alloc);
    rs_string_trim(&large);
    TEST_ASSERT_EQUAL_STRING("this is a much longer string with lots of whitespace", rs_string_cstr(&large));
    rs_string_destroy(&large);

    // Test no trimming needed
    rs_string_t no_trim = rs_string_from_cstr("hello", .allocator = alloc);
    rs_string_trim(&no_trim);
    TEST_ASSERT_EQUAL_STRING("hello", rs_string_cstr(&no_trim));
    rs_string_destroy(&no_trim);

    // Test all whitespace
    rs_string_t all_ws = rs_string_from_cstr("   \t\n\r   ", .allocator = alloc);
    rs_string_trim(&all_ws);
    TEST_ASSERT_EQUAL_STRING("", rs_string_cstr(&all_ws));
    TEST_ASSERT_EQUAL(0, rs_string_len(&all_ws));
    rs_string_destroy(&all_ws);

    // Test various whitespace types
    rs_string_t ws_types = rs_string_from_cstr("\t\n\r\v\f hello \t\n\r\v\f", .allocator = alloc);
    rs_string_trim(&ws_types);
    TEST_ASSERT_EQUAL_STRING("hello", rs_string_cstr(&ws_types));
    rs_string_destroy(&ws_types);

    rs_arena_destroy(arena);
}

void test_string_trim_start(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test trim start only - SSO
    rs_string_t str = rs_string_from_cstr("  hello  ", .allocator = alloc);
    rs_string_trim_start(&str);
    TEST_ASSERT_EQUAL_STRING("hello  ", rs_string_cstr(&str));
    TEST_ASSERT_EQUAL(7, rs_string_len(&str));
    rs_string_destroy(&str);

    // Test trim start only - large
    rs_string_t large = rs_string_from_cstr("    long string with leading whitespace", .allocator = alloc);
    rs_string_trim_start(&large);
    TEST_ASSERT_EQUAL_STRING("long string with leading whitespace", rs_string_cstr(&large));
    rs_string_destroy(&large);

    // Test no leading whitespace
    rs_string_t no_trim = rs_string_from_cstr("hello  ", .allocator = alloc);
    rs_string_trim_start(&no_trim);
    TEST_ASSERT_EQUAL_STRING("hello  ", rs_string_cstr(&no_trim));
    rs_string_destroy(&no_trim);

    rs_arena_destroy(arena);
}

void test_string_trim_end(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test trim end only - SSO
    rs_string_t str = rs_string_from_cstr("  hello  ", .allocator = alloc);
    rs_string_trim_end(&str);
    TEST_ASSERT_EQUAL_STRING("  hello", rs_string_cstr(&str));
    TEST_ASSERT_EQUAL(7, rs_string_len(&str));
    rs_string_destroy(&str);

    // Test trim end only - large
    rs_string_t large = rs_string_from_cstr("long string with trailing whitespace    ", .allocator = alloc);
    rs_string_trim_end(&large);
    TEST_ASSERT_EQUAL_STRING("long string with trailing whitespace", rs_string_cstr(&large));
    rs_string_destroy(&large);

    // Test no trailing whitespace
    rs_string_t no_trim = rs_string_from_cstr("  hello", .allocator = alloc);
    rs_string_trim_end(&no_trim);
    TEST_ASSERT_EQUAL_STRING("  hello", rs_string_cstr(&no_trim));
    rs_string_destroy(&no_trim);

    rs_arena_destroy(arena);
}

void test_string_to_lower(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test mixed case - SSO
    rs_string_t str = rs_string_from_cstr("Hello World", .allocator = alloc);
    rs_string_to_lower(&str);
    TEST_ASSERT_EQUAL_STRING("hello world", rs_string_cstr(&str));
    rs_string_destroy(&str);

    // Test all uppercase - large
    rs_string_t upper = rs_string_from_cstr("THIS IS A MUCH LONGER STRING IN ALL CAPS", .allocator = alloc);
    rs_string_to_lower(&upper);
    TEST_ASSERT_EQUAL_STRING("this is a much longer string in all caps", rs_string_cstr(&upper));
    rs_string_destroy(&upper);

    // Test already lowercase
    rs_string_t lower = rs_string_from_cstr("already lowercase", .allocator = alloc);
    rs_string_to_lower(&lower);
    TEST_ASSERT_EQUAL_STRING("already lowercase", rs_string_cstr(&lower));
    rs_string_destroy(&lower);

    // Test with numbers and symbols
    rs_string_t mixed = rs_string_from_cstr("Hello123!@#", .allocator = alloc);
    rs_string_to_lower(&mixed);
    TEST_ASSERT_EQUAL_STRING("hello123!@#", rs_string_cstr(&mixed));
    rs_string_destroy(&mixed);

    rs_arena_destroy(arena);
}

void test_string_to_upper(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test mixed case - SSO
    rs_string_t str = rs_string_from_cstr("Hello World", .allocator = alloc);
    rs_string_to_upper(&str);
    TEST_ASSERT_EQUAL_STRING("HELLO WORLD", rs_string_cstr(&str));
    rs_string_destroy(&str);

    // Test all lowercase - large
    rs_string_t lower = rs_string_from_cstr("this is a much longer string in all lowercase", .allocator = alloc);
    rs_string_to_upper(&lower);
    TEST_ASSERT_EQUAL_STRING("THIS IS A MUCH LONGER STRING IN ALL LOWERCASE", rs_string_cstr(&lower));
    rs_string_destroy(&lower);

    // Test already uppercase
    rs_string_t upper = rs_string_from_cstr("ALREADY UPPERCASE", .allocator = alloc);
    rs_string_to_upper(&upper);
    TEST_ASSERT_EQUAL_STRING("ALREADY UPPERCASE", rs_string_cstr(&upper));
    rs_string_destroy(&upper);

    // Test with numbers and symbols
    rs_string_t mixed = rs_string_from_cstr("hello123!@#", .allocator = alloc);
    rs_string_to_upper(&mixed);
    TEST_ASSERT_EQUAL_STRING("HELLO123!@#", rs_string_cstr(&mixed));
    rs_string_destroy(&mixed);

    rs_arena_destroy(arena);
}

void test_string_reverse(void)
{
    rs_arena_t *arena = rs_arena_create(4096);
    rs_allocator_t *alloc = rs_arena_allocator(arena);

    // Test simple reverse - SSO
    rs_string_t str = rs_string_from_cstr("hello", .allocator = alloc);
    rs_string_reverse(&str);
    TEST_ASSERT_EQUAL_STRING("olleh", rs_string_cstr(&str));
    rs_string_destroy(&str);

    // Test reverse with spaces - large
    rs_string_t large = rs_string_from_cstr("this is a much longer string to reverse", .allocator = alloc);
    rs_string_reverse(&large);
    TEST_ASSERT_EQUAL_STRING("esrever ot gnirts regnol hcum a si siht", rs_string_cstr(&large));
    rs_string_destroy(&large);

    // Test single character
    rs_string_t single = rs_string_from_cstr("a", .allocator = alloc);
    rs_string_reverse(&single);
    TEST_ASSERT_EQUAL_STRING("a", rs_string_cstr(&single));
    rs_string_destroy(&single);

    // Test empty string
    rs_string_t empty = rs_string_create(.allocator = alloc);
    rs_string_reverse(&empty);
    TEST_ASSERT_EQUAL_STRING("", rs_string_cstr(&empty));
    rs_string_destroy(&empty);

    // Test palindrome
    rs_string_t palindrome = rs_string_from_cstr("racecar", .allocator = alloc);
    rs_string_reverse(&palindrome);
    TEST_ASSERT_EQUAL_STRING("racecar", rs_string_cstr(&palindrome));
    rs_string_destroy(&palindrome);

    rs_arena_destroy(arena);
}

int main(void)
{
    UNITY_BEGIN();

    // Creation & Destruction tests
    RUN_TEST(test_string_create);
    RUN_TEST(test_string_new);
    RUN_TEST(test_string_from_cstr_small);
    RUN_TEST(test_string_from_cstr_large);
    RUN_TEST(test_string_from);
    RUN_TEST(test_string_from_buf);
    RUN_TEST(test_string_with_capacity);
    RUN_TEST(test_string_clone_small);
    RUN_TEST(test_string_clone_large);

    // SSO threshold tests
    RUN_TEST(test_string_sso_threshold);
    RUN_TEST(test_string_sso_to_large_transition);

    // Modification tests
    RUN_TEST(test_string_reserve);
    RUN_TEST(test_string_push_cstr);
    RUN_TEST(test_string_push_buf);
    RUN_TEST(test_string_push_char);
    RUN_TEST(test_string_push_string);
    RUN_TEST(test_string_clear);
    RUN_TEST(test_string_truncate);
    RUN_TEST(test_string_format);

    // Comparison tests
    RUN_TEST(test_string_cmp);
    RUN_TEST(test_string_eq);
    RUN_TEST(test_string_cmp_cstr);
    RUN_TEST(test_string_eq_cstr);

    // Searching tests
    RUN_TEST(test_string_find_char);
    RUN_TEST(test_string_find);
    RUN_TEST(test_string_starts_with);
    RUN_TEST(test_string_ends_with);

    // Edge cases & stress tests
    RUN_TEST(test_string_empty_operations);
    RUN_TEST(test_string_null_bytes);
    RUN_TEST(test_string_many_small_appends);
    RUN_TEST(test_string_system_allocator);

    // Manipulation tests
    RUN_TEST(test_string_trim);
    RUN_TEST(test_string_trim_start);
    RUN_TEST(test_string_trim_end);
    RUN_TEST(test_string_to_lower);
    RUN_TEST(test_string_to_upper);
    RUN_TEST(test_string_reverse);

    return UNITY_END();
}
