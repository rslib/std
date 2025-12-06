#include <rs/std/allocators/allocator.h>
#include <rs/std/encoding/base64.h>
#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_base64"

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
// Size Calculation Tests
// ============================================================================

void test_base64_encoded_size(void)
{
    // 0 bytes -> 0 bytes
    TEST_ASSERT_EQUAL(0, rs_base64_encoded_size(0));

    // 1 byte -> 4 bytes (with padding)
    TEST_ASSERT_EQUAL(4, rs_base64_encoded_size(1));

    // 2 bytes -> 4 bytes (with padding)
    TEST_ASSERT_EQUAL(4, rs_base64_encoded_size(2));

    // 3 bytes -> 4 bytes (no padding)
    TEST_ASSERT_EQUAL(4, rs_base64_encoded_size(3));

    // 4 bytes -> 8 bytes
    TEST_ASSERT_EQUAL(8, rs_base64_encoded_size(4));

    // 6 bytes -> 8 bytes
    TEST_ASSERT_EQUAL(8, rs_base64_encoded_size(6));
}

void test_base64_decoded_size(void)
{
    // 0 bytes -> 0 bytes
    TEST_ASSERT_EQUAL(0, rs_base64_decoded_size(0));

    // 4 bytes -> 3 bytes (max)
    TEST_ASSERT_EQUAL(3, rs_base64_decoded_size(4));

    // 8 bytes -> 6 bytes (max)
    TEST_ASSERT_EQUAL(6, rs_base64_decoded_size(8));
}

// ============================================================================
// Encoding Tests
// ============================================================================

void test_base64_encode_empty(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);
    rs_result_t result = rs_base64_encode(&encoded, "", 0);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(0, rs_string_len(&encoded));

    rs_string_destroy(&encoded);
}

void test_base64_encode_simple(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);

    // "Hello" -> "SGVsbG8="
    const char *input = "Hello";
    rs_result_t result = rs_base64_encode(&encoded, input, strlen(input));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&encoded, "SGVsbG8="));

    rs_string_destroy(&encoded);
}

void test_base64_encode_str(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);

    // "Hello World" -> "SGVsbG8gV29ybGQ="
    rs_result_t result = rs_base64_encode_str(&encoded, rs_sv_from_cstr("Hello World"));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&encoded, "SGVsbG8gV29ybGQ="));

    rs_string_destroy(&encoded);
}

void test_base64_encode_binary(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);

    // Binary data: {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
    const rs_u8 data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    rs_result_t result = rs_base64_encode(&encoded, data, sizeof(data));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&encoded, "AAECAwQF"));

    rs_string_destroy(&encoded);
}

void test_base64_encode_padding_1(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);

    // "H" (1 byte) -> "SA==" (needs 2 padding chars)
    rs_result_t result = rs_base64_encode(&encoded, "H", 1);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&encoded, "SA=="));

    rs_string_destroy(&encoded);
}

void test_base64_encode_padding_2(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);

    // "He" (2 bytes) -> "SGU=" (needs 1 padding char)
    rs_result_t result = rs_base64_encode(&encoded, "He", 2);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&encoded, "SGU="));

    rs_string_destroy(&encoded);
}

void test_base64_encode_no_padding(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);

    // "Hel" (3 bytes) -> "SGVs" (no padding needed)
    rs_result_t result = rs_base64_encode(&encoded, "Hel", 3);

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&encoded, "SGVs"));

    rs_string_destroy(&encoded);
}

void test_base64_encode_all_bytes(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);

    // Test encoding all possible byte values
    rs_u8 data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = (rs_u8)i;
    }

    rs_result_t result = rs_base64_encode(&encoded, data, sizeof(data));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // The encoded string should have the expected length
    TEST_ASSERT_EQUAL(rs_base64_encoded_size(256), rs_string_len(&encoded));

    rs_string_destroy(&encoded);
}

// ============================================================================
// Decoding Tests
// ============================================================================

void test_base64_decode_empty(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr(""));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(0, rs_string_len(&decoded));

    rs_string_destroy(&decoded);
}

void test_base64_decode_simple(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // "SGVsbG8=" -> "Hello"
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("SGVsbG8="));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&decoded, "Hello"));

    rs_string_destroy(&decoded);
}

void test_base64_decode_with_whitespace(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // "SGVs bG8g V29y bGQ=" (with spaces) -> "Hello World"
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("SGVs bG8g V29y bGQ="));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&decoded, "Hello World"));

    rs_string_destroy(&decoded);
}

void test_base64_decode_binary(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // "AAECAwQF" -> {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("AAECAwQF"));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_EQUAL(6, rs_string_len(&decoded));

    const rs_u8 *data = (const rs_u8 *)rs_string_cstr(&decoded);
    TEST_ASSERT_EQUAL(0x00, data[0]);
    TEST_ASSERT_EQUAL(0x01, data[1]);
    TEST_ASSERT_EQUAL(0x02, data[2]);
    TEST_ASSERT_EQUAL(0x03, data[3]);
    TEST_ASSERT_EQUAL(0x04, data[4]);
    TEST_ASSERT_EQUAL(0x05, data[5]);

    rs_string_destroy(&decoded);
}

void test_base64_decode_padding_1(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // "SGU=" -> "He"
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("SGU="));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&decoded, "He"));

    rs_string_destroy(&decoded);
}

void test_base64_decode_padding_2(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // "SA==" -> "H"
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("SA=="));

    TEST_ASSERT_EQUAL(RS_OK, result);
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&decoded, "H"));

    rs_string_destroy(&decoded);
}

void test_base64_decode_invalid_char(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // Invalid character '@' in Base64
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("SGVs@G8="));

    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_string_destroy(&decoded);
}

void test_base64_decode_incomplete(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // Incomplete Base64 (not a multiple of 4 after removing whitespace)
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("SGVs"));

    // This should actually succeed - "SGVs" is valid Base64 without padding
    TEST_ASSERT_EQUAL(RS_OK, result);

    rs_string_destroy(&decoded);
}

void test_base64_decode_incomplete_invalid(void)
{
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // Actually incomplete (1 char)
    rs_result_t result = rs_base64_decode(&decoded, rs_sv_from_cstr("S"));

    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_string_destroy(&decoded);
}

// ============================================================================
// Round-trip Tests
// ============================================================================

void test_base64_roundtrip_text(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    const char *original = "The quick brown fox jumps over the lazy dog";

    // Encode
    rs_result_t result = rs_base64_encode(&encoded, original, strlen(original));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Decode
    result = rs_base64_decode(&decoded, rs_sv_from_string(&encoded));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should match original
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&decoded, original));

    rs_string_destroy(&encoded);
    rs_string_destroy(&decoded);
}

void test_base64_roundtrip_binary(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // Binary data with all possible byte values
    rs_u8 data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = (rs_u8)i;
    }

    // Encode
    rs_result_t result = rs_base64_encode(&encoded, data, sizeof(data));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Decode
    result = rs_base64_decode(&decoded, rs_sv_from_string(&encoded));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should match original
    TEST_ASSERT_EQUAL(256, rs_string_len(&decoded));
    TEST_ASSERT_EQUAL_MEMORY(data, rs_string_cstr(&decoded), 256);

    rs_string_destroy(&encoded);
    rs_string_destroy(&decoded);
}

void test_base64_roundtrip_empty(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // Encode empty
    rs_result_t result = rs_base64_encode(&encoded, "", 0);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Decode empty
    result = rs_base64_decode(&decoded, rs_sv_from_string(&encoded));
    TEST_ASSERT_EQUAL(RS_OK, result);

    TEST_ASSERT_EQUAL(0, rs_string_len(&decoded));

    rs_string_destroy(&encoded);
    rs_string_destroy(&decoded);
}

// ============================================================================
// RFC 4648 Test Vectors
// ============================================================================

void test_base64_rfc4648_vectors(void)
{
    rs_string_t encoded = rs_string_create(.allocator = allocator);
    rs_string_t decoded = rs_string_create(.allocator = allocator);

    // Test vectors from RFC 4648
    struct {
        const char *plain;
        const char *encoded;
    } vectors[] = {
        {"", ""},
        {"f", "Zg=="},
        {"fo", "Zm8="},
        {"foo", "Zm9v"},
        {"foob", "Zm9vYg=="},
        {"fooba", "Zm9vYmE="},
        {"foobar", "Zm9vYmFy"},
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        // Test encoding
        rs_string_clear(&encoded);
        rs_result_t result = rs_base64_encode(&encoded, vectors[i].plain, strlen(vectors[i].plain));
        TEST_ASSERT_EQUAL(RS_OK, result);
        TEST_ASSERT_TRUE(rs_string_eq_cstr(&encoded, vectors[i].encoded));

        // Test decoding
        rs_string_clear(&decoded);
        result = rs_base64_decode(&decoded, rs_sv_from_cstr(vectors[i].encoded));
        TEST_ASSERT_EQUAL(RS_OK, result);
        TEST_ASSERT_TRUE(rs_string_eq_cstr(&decoded, vectors[i].plain));
    }

    rs_string_destroy(&encoded);
    rs_string_destroy(&decoded);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Size calculation
    RUN_TEST(test_base64_encoded_size);
    RUN_TEST(test_base64_decoded_size);

    // Encoding
    RUN_TEST(test_base64_encode_empty);
    RUN_TEST(test_base64_encode_simple);
    RUN_TEST(test_base64_encode_str);
    RUN_TEST(test_base64_encode_binary);
    RUN_TEST(test_base64_encode_padding_1);
    RUN_TEST(test_base64_encode_padding_2);
    RUN_TEST(test_base64_encode_no_padding);
    RUN_TEST(test_base64_encode_all_bytes);

    // Decoding
    RUN_TEST(test_base64_decode_empty);
    RUN_TEST(test_base64_decode_simple);
    RUN_TEST(test_base64_decode_with_whitespace);
    RUN_TEST(test_base64_decode_binary);
    RUN_TEST(test_base64_decode_padding_1);
    RUN_TEST(test_base64_decode_padding_2);
    RUN_TEST(test_base64_decode_invalid_char);
    RUN_TEST(test_base64_decode_incomplete);
    RUN_TEST(test_base64_decode_incomplete_invalid);

    // Round-trip
    RUN_TEST(test_base64_roundtrip_text);
    RUN_TEST(test_base64_roundtrip_binary);
    RUN_TEST(test_base64_roundtrip_empty);

    // RFC 4648 test vectors
    RUN_TEST(test_base64_rfc4648_vectors);

    return UNITY_END();
}
