#include <rs/std/allocators/allocator.h>
#include <rs/std/crypto/crypto.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Need at least key size worth of data
    if (size < RS_CRYPTO_KEY_SIZE) {
        return 0;
    }

    // Use first 32 bytes as key
    const rs_u8 *key = data;
    const uint8_t *plaintext = data + RS_CRYPTO_KEY_SIZE;
    size_t plaintext_len = size - RS_CRYPTO_KEY_SIZE;

    rs_string_view_t input = rs_sv_from_buf((const char *)plaintext, plaintext_len);

    // Test encrypt/decrypt round-trip
    rs_string_t encrypted = rs_string_create();
    if (rs_crypto_encrypt(&encrypted, input, key) == RS_OK) {
        rs_string_t decrypted = rs_string_create();
        rs_crypto_decrypt(&decrypted, rs_sv_from_string(&encrypted), key);
        rs_string_destroy(&decrypted);
    }
    rs_string_destroy(&encrypted);

    // Try to decrypt arbitrary input (may fail)
    rs_string_t decrypted_raw = rs_string_create();
    rs_crypto_decrypt(&decrypted_raw, input, key);
    rs_string_destroy(&decrypted_raw);

    return 0;
}
