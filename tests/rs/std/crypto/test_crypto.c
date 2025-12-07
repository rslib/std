#include <rs/std/allocators/allocator.h>
#include <rs/std/crypto/crypto.h>
#include <rs/std/encoding/base64.h>
#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_crypto"

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
// Random Number Generation Tests
// ============================================================================

void test_crypto_random(void)
{
    rs_u8 buf1[32];
    rs_u8 buf2[32];

    // Generate random bytes
    rs_result_t result = rs_crypto_random(buf1, sizeof(buf1));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Generate more random bytes
    result = rs_crypto_random(buf2, sizeof(buf2));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // They should be different (extremely unlikely to be the same)
    TEST_ASSERT_NOT_EQUAL(0, memcmp(buf1, buf2, sizeof(buf1)));
}

void test_crypto_random_invalid(void)
{
    // NULL buffer should fail
    rs_result_t result = rs_crypto_random(NULL, 32);
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    // Zero length should fail
    rs_u8 buf[32];
    result = rs_crypto_random(buf, 0);
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);
}

// ============================================================================
// Key Derivation Tests
// ============================================================================

void test_crypto_derive_key(void)
{
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    rs_u8 salt[RS_CRYPTO_SALT_SIZE];

    // Generate salt
    rs_result_t result = rs_crypto_random(salt, sizeof(salt));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Derive key
    result = rs_crypto_derive_key(key, rs_sv_from_cstr("test-password"), salt);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Key should not be all zeros
    rs_bool all_zeros = true;
    for (size_t i = 0; i < RS_CRYPTO_KEY_SIZE; i++) {
        if (key[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(all_zeros);
}

void test_crypto_derive_key_deterministic(void)
{
    rs_u8 key1[RS_CRYPTO_KEY_SIZE];
    rs_u8 key2[RS_CRYPTO_KEY_SIZE];
    rs_u8 salt[RS_CRYPTO_SALT_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    // Derive key twice with same password and salt
    rs_crypto_derive_key(key1, rs_sv_from_cstr("test-password"), salt);
    rs_crypto_derive_key(key2, rs_sv_from_cstr("test-password"), salt);

    // Keys should be identical
    TEST_ASSERT_EQUAL_MEMORY(key1, key2, RS_CRYPTO_KEY_SIZE);
}

void test_crypto_derive_key_different_passwords(void)
{
    rs_u8 key1[RS_CRYPTO_KEY_SIZE];
    rs_u8 key2[RS_CRYPTO_KEY_SIZE];
    rs_u8 salt[RS_CRYPTO_SALT_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    // Derive keys with different passwords
    rs_crypto_derive_key(key1, rs_sv_from_cstr("password1"), salt);
    rs_crypto_derive_key(key2, rs_sv_from_cstr("password2"), salt);

    // Keys should be different
    TEST_ASSERT_NOT_EQUAL(0, memcmp(key1, key2, RS_CRYPTO_KEY_SIZE));
}

void test_crypto_derive_key_different_salts(void)
{
    rs_u8 key1[RS_CRYPTO_KEY_SIZE];
    rs_u8 key2[RS_CRYPTO_KEY_SIZE];
    rs_u8 salt1[RS_CRYPTO_SALT_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    rs_u8 salt2[RS_CRYPTO_SALT_SIZE] = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    // Derive keys with different salts
    rs_crypto_derive_key(key1, rs_sv_from_cstr("test-password"), salt1);
    rs_crypto_derive_key(key2, rs_sv_from_cstr("test-password"), salt2);

    // Keys should be different
    TEST_ASSERT_NOT_EQUAL(0, memcmp(key1, key2, RS_CRYPTO_KEY_SIZE));
}

// ============================================================================
// Encryption/Decryption Tests
// ============================================================================

void test_crypto_encrypt_decrypt(void)
{
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    rs_crypto_random(key, sizeof(key));

    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    const char *original = "Hello, World!";

    // Encrypt
    rs_result_t result = rs_crypto_encrypt(&ciphertext, rs_sv_from_cstr(original), key);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Ciphertext should be larger than plaintext (nonce + data + MAC)
    TEST_ASSERT_GREATER_THAN(strlen(original), rs_string_len(&ciphertext));

    // Decrypt
    result = rs_crypto_decrypt(&plaintext, rs_sv_from_string(&ciphertext), key);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should match original
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&plaintext, original));

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

void test_crypto_encrypt_empty(void)
{
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    rs_crypto_random(key, sizeof(key));

    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    // Encrypt empty string
    rs_result_t result = rs_crypto_encrypt(&ciphertext, rs_sv_from_cstr(""), key);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Decrypt
    result = rs_crypto_decrypt(&plaintext, rs_sv_from_string(&ciphertext), key);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should be empty
    TEST_ASSERT_EQUAL(0, rs_string_len(&plaintext));

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

void test_crypto_encrypt_binary(void)
{
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    rs_crypto_random(key, sizeof(key));

    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    // Binary data with null bytes
    rs_u8 original[256];
    for (int i = 0; i < 256; i++) {
        original[i] = (rs_u8)i;
    }

    // Encrypt
    rs_result_t result = rs_crypto_encrypt(&ciphertext, rs_sv_from_buf((const char *)original, sizeof(original)), key);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Decrypt
    result = rs_crypto_decrypt(&plaintext, rs_sv_from_string(&ciphertext), key);
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should match original
    TEST_ASSERT_EQUAL(256, rs_string_len(&plaintext));
    TEST_ASSERT_EQUAL_MEMORY(original, rs_string_cstr(&plaintext), 256);

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

void test_crypto_decrypt_wrong_key(void)
{
    rs_u8 key1[RS_CRYPTO_KEY_SIZE];
    rs_u8 key2[RS_CRYPTO_KEY_SIZE];
    rs_crypto_random(key1, sizeof(key1));
    rs_crypto_random(key2, sizeof(key2));

    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    // Encrypt with key1
    rs_crypto_encrypt(&ciphertext, rs_sv_from_cstr("secret"), key1);

    // Try to decrypt with key2 (should fail)
    rs_result_t result = rs_crypto_decrypt(&plaintext, rs_sv_from_string(&ciphertext), key2);
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

void test_crypto_decrypt_corrupted(void)
{
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    rs_crypto_random(key, sizeof(key));

    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    // Encrypt
    rs_crypto_encrypt(&ciphertext, rs_sv_from_cstr("secret"), key);
    TEST_ASSERT_TRUE(rs_string_len(&ciphertext) > RS_CRYPTO_NONCE_SIZE + 2);

    // Corrupt one byte in the middle
    char *data = rs_string_data_mut(&ciphertext);
    data[RS_CRYPTO_NONCE_SIZE + 2] ^= 0xFF;

    // Decrypt should fail (authentication error)
    rs_result_t result = rs_crypto_decrypt(&plaintext, rs_sv_from_string(&ciphertext), key);
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

void test_crypto_decrypt_too_short(void)
{
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    rs_crypto_random(key, sizeof(key));

    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    // Too short ciphertext
    rs_result_t result = rs_crypto_decrypt(&plaintext, rs_sv_from_cstr("short"), key);
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_string_destroy(&plaintext);
}

// ============================================================================
// Password-based Encryption Tests
// ============================================================================

void test_crypto_encrypt_decrypt_with_password(void)
{
    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    const char *original = "Secret message";
    const char *password = "my-password";

    // Encrypt
    rs_result_t result =
        rs_crypto_encrypt_with_password(&ciphertext, rs_sv_from_cstr(original), rs_sv_from_cstr(password));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Ciphertext should include salt
    TEST_ASSERT_GREATER_THAN(RS_CRYPTO_SALT_SIZE, rs_string_len(&ciphertext));

    // Decrypt
    result = rs_crypto_decrypt_with_password(&plaintext, rs_sv_from_string(&ciphertext), rs_sv_from_cstr(password));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should match original
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&plaintext, original));

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

void test_crypto_decrypt_with_wrong_password(void)
{
    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    // Encrypt with password1
    rs_crypto_encrypt_with_password(&ciphertext, rs_sv_from_cstr("secret"), rs_sv_from_cstr("password1"));

    // Try to decrypt with password2 (should fail)
    rs_result_t result =
        rs_crypto_decrypt_with_password(&plaintext, rs_sv_from_string(&ciphertext), rs_sv_from_cstr("password2"));
    TEST_ASSERT_NOT_EQUAL(RS_OK, result);

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

// ============================================================================
// Base64 Encoding Tests
// ============================================================================

void test_crypto_encrypt_decrypt_base64(void)
{
    rs_string_t ciphertext = rs_string_create(.allocator = allocator);
    rs_string_t plaintext = rs_string_create(.allocator = allocator);

    const char *original = "Secret message for Base64 test";
    const char *password = "test-password";

    // Encrypt and encode to Base64
    rs_result_t result = rs_crypto_encrypt_base64(&ciphertext, rs_sv_from_cstr(original), rs_sv_from_cstr(password));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Ciphertext should be valid Base64 (only contains Base64 characters)
    const char *data = rs_string_cstr(&ciphertext);
    for (size_t i = 0; i < rs_string_len(&ciphertext); i++) {
        char c = data[i];
        rs_bool valid = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' ||
                        c == '/' || c == '=';
        TEST_ASSERT_TRUE(valid);
    }

    // Decode and decrypt
    result = rs_crypto_decrypt_base64(&plaintext, rs_sv_from_string(&ciphertext), rs_sv_from_cstr(password));
    TEST_ASSERT_EQUAL(RS_OK, result);

    // Should match original
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&plaintext, original));

    rs_string_destroy(&ciphertext);
    rs_string_destroy(&plaintext);
}

// ============================================================================
// Memory Wiping Tests
// ============================================================================

void test_crypto_wipe(void)
{
    rs_u8 data[32];

    // Fill with non-zero data
    memset(data, 0xFF, sizeof(data));

    // Wipe
    rs_crypto_wipe(data, sizeof(data));

    // Should be all zeros
    for (size_t i = 0; i < sizeof(data); i++) {
        TEST_ASSERT_EQUAL(0, data[i]);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_crypto_integration(void)
{
    rs_string_t encrypted1 = rs_string_create(.allocator = allocator);
    rs_string_t encrypted2 = rs_string_create(.allocator = allocator);
    rs_string_t decrypted = rs_string_create(.allocator = allocator);

    const char *password = "integration-test-password";
    const char *message = "This is a test message for integration testing";

    // Encrypt same message twice
    rs_crypto_encrypt_base64(&encrypted1, rs_sv_from_cstr(message), rs_sv_from_cstr(password));
    rs_crypto_encrypt_base64(&encrypted2, rs_sv_from_cstr(message), rs_sv_from_cstr(password));

    // Ciphertexts should be different (random salt and nonce)
    TEST_ASSERT_FALSE(rs_string_eq(&encrypted1, &encrypted2));

    // Both should decrypt to the same message
    rs_crypto_decrypt_base64(&decrypted, rs_sv_from_string(&encrypted1), rs_sv_from_cstr(password));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&decrypted, message));

    rs_string_clear(&decrypted);
    rs_crypto_decrypt_base64(&decrypted, rs_sv_from_string(&encrypted2), rs_sv_from_cstr(password));
    TEST_ASSERT_TRUE(rs_string_eq_cstr(&decrypted, message));

    rs_string_destroy(&encrypted1);
    rs_string_destroy(&encrypted2);
    rs_string_destroy(&decrypted);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Random number generation
    RUN_TEST(test_crypto_random);
    RUN_TEST(test_crypto_random_invalid);

    // Key derivation
    RUN_TEST(test_crypto_derive_key);
    RUN_TEST(test_crypto_derive_key_deterministic);
    RUN_TEST(test_crypto_derive_key_different_passwords);
    RUN_TEST(test_crypto_derive_key_different_salts);

    // Encryption/Decryption
    RUN_TEST(test_crypto_encrypt_decrypt);
    RUN_TEST(test_crypto_encrypt_empty);
    RUN_TEST(test_crypto_encrypt_binary);
    RUN_TEST(test_crypto_decrypt_wrong_key);
    RUN_TEST(test_crypto_decrypt_corrupted);
    RUN_TEST(test_crypto_decrypt_too_short);

    // Password-based encryption
    RUN_TEST(test_crypto_encrypt_decrypt_with_password);
    RUN_TEST(test_crypto_decrypt_with_wrong_password);

    // Base64 encoding
    RUN_TEST(test_crypto_encrypt_decrypt_base64);

    // Memory wiping
    RUN_TEST(test_crypto_wipe);

    // Integration
    RUN_TEST(test_crypto_integration);

    return UNITY_END();
}
