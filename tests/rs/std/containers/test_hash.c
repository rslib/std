#include <rs/std/containers/hash.h>
#include <rs/std/logging/logging.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_hash"

// ============================================================================
// Test Setup
// ============================================================================

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
// xxHash Tests
// ============================================================================

void test_hash_xxh3_basic(void)
{
    const char *data = "Hello, World!";
    rs_u64 hash1 = rs_hash_xxh3(data, strlen(data), NULL);
    rs_u64 hash2 = rs_hash_xxh3(data, strlen(data), NULL);

    // Same input should produce same hash
    TEST_ASSERT_EQUAL_UINT64(hash1, hash2);

    // Hash should not be zero
    TEST_ASSERT_NOT_EQUAL(0, hash1);
}

void test_hash_xxh3_different_data(void)
{
    const char *data1 = "Hello, World!";
    const char *data2 = "Hello, World?";

    rs_u64 hash1 = rs_hash_xxh3(data1, strlen(data1), NULL);
    rs_u64 hash2 = rs_hash_xxh3(data2, strlen(data2), NULL);

    // Different input should produce different hash
    TEST_ASSERT_NOT_EQUAL(hash1, hash2);
}

void test_hash_xxh3_empty(void)
{
    const char *data = "";
    rs_u64 hash = rs_hash_xxh3(data, 0, NULL);

    // Empty data should still produce a hash
    TEST_ASSERT_NOT_EQUAL(0, hash);
}

void test_hash_xxh3_binary(void)
{
    rs_u8 data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = (rs_u8)i;
    }

    rs_u64 hash = rs_hash_xxh3(data, sizeof(data), NULL);
    TEST_ASSERT_NOT_EQUAL(0, hash);
}

// ============================================================================
// String Hash Tests
// ============================================================================

void test_hash_string_basic(void)
{
    const char *str = "test string";
    rs_u64 hash1 = rs_hash_string(str, NULL);
    rs_u64 hash2 = rs_hash_string(str, NULL);

    // Same string should produce same hash
    TEST_ASSERT_EQUAL_UINT64(hash1, hash2);
}

void test_hash_string_different(void)
{
    const char *str1 = "test";
    const char *str2 = "Test";

    rs_u64 hash1 = rs_hash_string(str1, NULL);
    rs_u64 hash2 = rs_hash_string(str2, NULL);

    // Different strings should produce different hashes
    TEST_ASSERT_NOT_EQUAL(hash1, hash2);
}

void test_hash_string_empty(void)
{
    const char *str = "";
    rs_u64 hash = rs_hash_string(str, NULL);

    // Empty string should produce a hash
    TEST_ASSERT_NOT_EQUAL(0, hash);
}

void test_hash_string_collision_resistance(void)
{
    // Test that similar strings produce different hashes
    const char *str1 = "abc";
    const char *str2 = "abd";
    const char *str3 = "bac";

    rs_u64 hash1 = rs_hash_string(str1, NULL);
    rs_u64 hash2 = rs_hash_string(str2, NULL);
    rs_u64 hash3 = rs_hash_string(str3, NULL);

    TEST_ASSERT_NOT_EQUAL(hash1, hash2);
    TEST_ASSERT_NOT_EQUAL(hash1, hash3);
    TEST_ASSERT_NOT_EQUAL(hash2, hash3);
}

// ============================================================================
// Integer Hash Tests
// ============================================================================

void test_hash_int64_basic(void)
{
    rs_u64 value = 42;
    rs_u64 hash1 = rs_hash_int64(value, NULL);
    rs_u64 hash2 = rs_hash_int64(value, NULL);

    // Same value should produce same hash
    TEST_ASSERT_EQUAL_UINT64(hash1, hash2);
}

void test_hash_int64_different(void)
{
    rs_u64 hash1 = rs_hash_int64(1, NULL);
    rs_u64 hash2 = rs_hash_int64(2, NULL);

    // Different values should produce different hashes
    TEST_ASSERT_NOT_EQUAL(hash1, hash2);
}

void test_hash_int64_zero(void)
{
    rs_u64 hash = rs_hash_int64(0, NULL);

    // Zero should produce a non-zero hash
    TEST_ASSERT_NOT_EQUAL(0, hash);
}

void test_hash_int64_distribution(void)
{
    // Test that sequential integers produce well-distributed hashes
    rs_u64 hash1 = rs_hash_int64(1, NULL);
    rs_u64 hash2 = rs_hash_int64(2, NULL);
    rs_u64 hash3 = rs_hash_int64(3, NULL);

    // Hashes should be very different even for sequential inputs
    TEST_ASSERT_NOT_EQUAL(hash1, hash2);
    TEST_ASSERT_NOT_EQUAL(hash2, hash3);
    TEST_ASSERT_NOT_EQUAL(hash1, hash3);
}

void test_hash_int64_large_values(void)
{
    rs_u64 large1 = 0xFFFFFFFFFFFFFFFFULL;
    rs_u64 large2 = 0xFFFFFFFFFFFFFFFEULL;

    rs_u64 hash1 = rs_hash_int64(large1, NULL);
    rs_u64 hash2 = rs_hash_int64(large2, NULL);

    TEST_ASSERT_NOT_EQUAL(hash1, hash2);
}

// ============================================================================
// Pointer Hash Tests
// ============================================================================

void test_hash_pointer_basic(void)
{
    int value = 42;
    void *ptr = &value;

    rs_u64 hash1 = rs_hash_pointer(ptr, NULL);
    rs_u64 hash2 = rs_hash_pointer(ptr, NULL);

    // Same pointer should produce same hash
    TEST_ASSERT_EQUAL_UINT64(hash1, hash2);
}

void test_hash_pointer_different(void)
{
    int value1 = 1;
    int value2 = 2;

    rs_u64 hash1 = rs_hash_pointer(&value1, NULL);
    rs_u64 hash2 = rs_hash_pointer(&value2, NULL);

    // Different pointers should produce different hashes
    TEST_ASSERT_NOT_EQUAL(hash1, hash2);
}

void test_hash_pointer_null(void)
{
    rs_u64 hash = rs_hash_pointer(NULL, NULL);

    // NULL pointer should hash to 0
    TEST_ASSERT_EQUAL_UINT64(0, hash);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // xxHash tests
    RUN_TEST(test_hash_xxh3_basic);
    RUN_TEST(test_hash_xxh3_different_data);
    RUN_TEST(test_hash_xxh3_empty);
    RUN_TEST(test_hash_xxh3_binary);

    // String hash tests
    RUN_TEST(test_hash_string_basic);
    RUN_TEST(test_hash_string_different);
    RUN_TEST(test_hash_string_empty);
    RUN_TEST(test_hash_string_collision_resistance);

    // Integer hash tests
    RUN_TEST(test_hash_int64_basic);
    RUN_TEST(test_hash_int64_different);
    RUN_TEST(test_hash_int64_zero);
    RUN_TEST(test_hash_int64_distribution);
    RUN_TEST(test_hash_int64_large_values);

    // Pointer hash tests
    RUN_TEST(test_hash_pointer_basic);
    RUN_TEST(test_hash_pointer_different);
    RUN_TEST(test_hash_pointer_null);

    return UNITY_END();
}
