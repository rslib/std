#include <monocypher.h>
#include <rs/std/crypto/crypto.h>
#include <rs/std/encoding/base64.h>
#include <rs/std/error.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
// clang-format off
#include <windows.h>  // Must be included before bcrypt.h
#include <bcrypt.h>
// clang-format on
#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif
#else
#include <fcntl.h>
#include <unistd.h>
#endif

// ============================================================================
// Random Number Generation
// ============================================================================

rs_result_t rs_crypto_random(void *out, rs_size_t len)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output buffer is NULL");
    RS_CHECK(len > 0, RS_ERR_INVALID, "Length must be > 0");

#ifdef _WIN32
    // Windows: Use BCryptGenRandom
    NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)out, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(status)) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to generate random bytes");
    }
#else
    // Unix: Use /dev/urandom
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to open /dev/urandom");
    }

    ssize_t bytes_read = read(fd, out, len);
    close(fd);

    if (bytes_read < 0 || (rs_size_t)bytes_read != len) {
        return RS_ERROR_RET(RS_ERR_IO, "Failed to read random bytes");
    }
#endif

    return RS_OK;
}

// ============================================================================
// Key Derivation
// ============================================================================

rs_result_t rs_crypto_derive_key(rs_u8 *key_out, rs_string_view_t password, const rs_u8 *salt)
{
    // Use default parameters: 3 iterations, 64 MiB memory
    return rs_crypto_derive_key_ex(key_out, password, salt, 3, 65536);
}

rs_result_t rs_crypto_derive_key_ex(rs_u8 *key_out, rs_string_view_t password, const rs_u8 *salt, rs_u32 iterations,
                                    rs_u32 memory_kib)
{
    RS_CHECK(key_out != NULL, RS_ERR_INVALID, "Key output buffer is NULL");
    RS_CHECK(rs_sv_data(password) != NULL, RS_ERR_INVALID, "Password is NULL");
    RS_CHECK(salt != NULL, RS_ERR_INVALID, "Salt is NULL");
    RS_CHECK(iterations > 0, RS_ERR_INVALID, "Iterations must be > 0");
    RS_CHECK(memory_kib > 0, RS_ERR_INVALID, "Memory must be > 0");

    // Allocate work area for Argon2i
    // nb_blocks = memory_kib / lanes (we use 1 lane)
    rs_u32 nb_blocks = memory_kib;
    rs_size_t work_area_size = nb_blocks * 1024;
    void *work_area = malloc(work_area_size);
    if (!work_area) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate Argon2 work area");
    }

    // Configure Argon2i (memory-hard, recommended for password hashing)
    crypto_argon2_config config = {
        .algorithm = CRYPTO_ARGON2_I, // Argon2i variant
        .nb_blocks = nb_blocks,       // memory hardness
        .nb_passes = iterations,      // CPU hardness (number of iterations)
        .nb_lanes = 1                 // single-threaded
    };

    // Set up inputs
    crypto_argon2_inputs inputs = {.pass = (const rs_u8 *)rs_sv_data(password),
                                   .salt = salt,
                                   .pass_size = (rs_u32)rs_sv_len(password),
                                   .salt_size = RS_CRYPTO_SALT_SIZE};

    // No additional key or data
    crypto_argon2_extras extras = {.key = NULL, .ad = NULL, .key_size = 0, .ad_size = 0};

    // Derive key using Argon2
    crypto_argon2(key_out, RS_CRYPTO_KEY_SIZE, work_area, config, inputs, extras);

    // Wipe work area
    crypto_wipe(work_area, work_area_size);
    free(work_area);

    return RS_OK;
}

// ============================================================================
// Encryption/Decryption
// ============================================================================

rs_result_t rs_crypto_encrypt(rs_string_t *out, rs_string_view_t plaintext, const rs_u8 *key)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");
    RS_CHECK(key != NULL, RS_ERR_INVALID, "Key is NULL");

    const rs_u8 *plain_data = (const rs_u8 *)rs_sv_data(plaintext);
    rs_size_t plain_len = rs_sv_len(plaintext);

    // Generate random nonce
    rs_u8 nonce[RS_CRYPTO_NONCE_SIZE];
    rs_result_t result = rs_crypto_random(nonce, sizeof(nonce));
    if (result != RS_OK) {
        return result;
    }

    // Calculate output size: nonce + ciphertext + MAC
    rs_size_t output_len = RS_CRYPTO_NONCE_SIZE + plain_len + RS_CRYPTO_MAC_SIZE;

    // Clear output and reserve space
    rs_string_clear(out);
    rs_string_reserve(out, output_len);

    // Write nonce to output
    for (rs_size_t i = 0; i < RS_CRYPTO_NONCE_SIZE; i++) {
        rs_string_push_char(out, (char)nonce[i]);
    }

    // Allocate space for ciphertext + MAC
    rs_u8 *cipher_start = (rs_u8 *)rs_string_cstr(out) + RS_CRYPTO_NONCE_SIZE;
    rs_u8 mac[RS_CRYPTO_MAC_SIZE];

    // Ensure string has enough space
    for (rs_size_t i = 0; i < plain_len + RS_CRYPTO_MAC_SIZE; i++) {
        rs_string_push_char(out, '\0');
    }

    // Encrypt using XChaCha20-Poly1305 (no additional data)
    crypto_aead_lock(cipher_start, mac, key, nonce, NULL, 0, plain_data, plain_len);

    // Append MAC to output (overwrite the zeros we added)
    rs_u8 *mac_start = cipher_start + plain_len;
    memcpy(mac_start, mac, RS_CRYPTO_MAC_SIZE);

    return RS_OK;
}

rs_result_t rs_crypto_decrypt(rs_string_t *out, rs_string_view_t ciphertext, const rs_u8 *key)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");
    RS_CHECK(key != NULL, RS_ERR_INVALID, "Key is NULL");

    const rs_u8 *cipher_data = (const rs_u8 *)rs_sv_data(ciphertext);
    rs_size_t cipher_len = rs_sv_len(ciphertext);

    // Check minimum size: nonce + MAC (no plaintext)
    if (cipher_len < RS_CRYPTO_NONCE_SIZE + RS_CRYPTO_MAC_SIZE) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Ciphertext too short");
    }

    // Extract components
    const rs_u8 *nonce = cipher_data;
    const rs_u8 *cipher = cipher_data + RS_CRYPTO_NONCE_SIZE;
    rs_size_t cipher_only_len = cipher_len - RS_CRYPTO_NONCE_SIZE - RS_CRYPTO_MAC_SIZE;
    const rs_u8 *mac = cipher + cipher_only_len;

    // Clear output and reserve space
    rs_string_clear(out);
    rs_string_reserve(out, cipher_only_len);

    // Allocate space for plaintext
    for (rs_size_t i = 0; i < cipher_only_len; i++) {
        rs_string_push_char(out, '\0');
    }

    // Decrypt using XChaCha20-Poly1305
    rs_u8 *plain_data = (rs_u8 *)rs_string_cstr(out);
    int auth_result = crypto_aead_unlock(plain_data, mac, key, nonce, NULL, 0, cipher, cipher_only_len);

    if (auth_result != 0) {
        // Authentication failed - wipe output
        rs_string_clear(out);
        return RS_ERROR_RET(RS_ERR_INVALID, "Decryption failed: authentication error");
    }

    return RS_OK;
}

// ============================================================================
// Convenience Functions
// ============================================================================

rs_result_t rs_crypto_encrypt_with_password(rs_string_t *out, rs_string_view_t plaintext, rs_string_view_t password)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");
    RS_CHECK(rs_sv_data(password) != NULL, RS_ERR_INVALID, "Password is NULL");

    // Generate random salt
    rs_u8 salt[RS_CRYPTO_SALT_SIZE];
    rs_result_t result = rs_crypto_random(salt, sizeof(salt));
    if (result != RS_OK) {
        return result;
    }

    // Derive key from password
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    result = rs_crypto_derive_key(key, password, salt);
    if (result != RS_OK) {
        crypto_wipe(key, sizeof(key));
        return result;
    }

    // Encrypt data
    rs_string_t encrypted = rs_string_create(.allocator = rs_string_get_allocator(out));
    result = rs_crypto_encrypt(&encrypted, plaintext, key);
    crypto_wipe(key, sizeof(key));

    if (result != RS_OK) {
        rs_string_destroy(&encrypted);
        return result;
    }

    // Output format: [salt][encrypted data]
    rs_string_clear(out);
    rs_string_reserve(out, RS_CRYPTO_SALT_SIZE + rs_string_len(&encrypted));

    // Write salt
    for (rs_size_t i = 0; i < RS_CRYPTO_SALT_SIZE; i++) {
        rs_string_push_char(out, (char)salt[i]);
    }

    // Write encrypted data
    rs_string_push_buf(out, rs_string_cstr(&encrypted), rs_string_len(&encrypted));

    rs_string_destroy(&encrypted);
    return RS_OK;
}

rs_result_t rs_crypto_decrypt_with_password(rs_string_t *out, rs_string_view_t ciphertext, rs_string_view_t password)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");
    RS_CHECK(rs_sv_data(password) != NULL, RS_ERR_INVALID, "Password is NULL");

    const rs_u8 *data = (const rs_u8 *)rs_sv_data(ciphertext);
    rs_size_t len = rs_sv_len(ciphertext);

    // Check minimum size: salt + nonce + MAC
    if (len < RS_CRYPTO_SALT_SIZE + RS_CRYPTO_NONCE_SIZE + RS_CRYPTO_MAC_SIZE) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Ciphertext too short");
    }

    // Extract salt
    const rs_u8 *salt = data;
    const rs_u8 *encrypted_data = data + RS_CRYPTO_SALT_SIZE;
    rs_size_t encrypted_len = len - RS_CRYPTO_SALT_SIZE;

    // Derive key from password
    rs_u8 key[RS_CRYPTO_KEY_SIZE];
    rs_result_t result = rs_crypto_derive_key(key, password, salt);
    if (result != RS_OK) {
        crypto_wipe(key, sizeof(key));
        return result;
    }

    // Decrypt data
    result = rs_crypto_decrypt(out, rs_sv_from_buf((const char *)encrypted_data, encrypted_len), key);
    crypto_wipe(key, sizeof(key));

    return result;
}

// ============================================================================
// Base64 Encoding Helpers
// ============================================================================

rs_result_t rs_crypto_encrypt_base64(rs_string_t *out, rs_string_view_t plaintext, rs_string_view_t password)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");

    // Encrypt data
    rs_string_t encrypted = rs_string_create(.allocator = rs_string_get_allocator(out));
    rs_result_t result = rs_crypto_encrypt_with_password(&encrypted, plaintext, password);
    if (result != RS_OK) {
        rs_string_destroy(&encrypted);
        return result;
    }

    // Encode to Base64
    result = rs_base64_encode(out, rs_string_cstr(&encrypted), rs_string_len(&encrypted));
    rs_string_destroy(&encrypted);

    return result;
}

rs_result_t rs_crypto_decrypt_base64(rs_string_t *out, rs_string_view_t ciphertext_base64, rs_string_view_t password)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");

    // Decode from Base64
    rs_string_t encrypted = rs_string_create(.allocator = rs_string_get_allocator(out));
    rs_result_t result = rs_base64_decode(&encrypted, ciphertext_base64);
    if (result != RS_OK) {
        rs_string_destroy(&encrypted);
        return result;
    }

    // Decrypt data
    result = rs_crypto_decrypt_with_password(out, rs_sv_from_string(&encrypted), password);
    rs_string_destroy(&encrypted);

    return result;
}

// ============================================================================
// Memory Wiping
// ============================================================================

void rs_crypto_wipe(void *data, rs_size_t len)
{
    crypto_wipe(data, len);
}
