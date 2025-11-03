#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

/**
 * Cryptography functions using Monocypher.
 *
 * This module provides encryption/decryption, key derivation, and other
 * cryptographic operations. It wraps the Monocypher library for ease of use.
 *
 * Key features:
 * - Symmetric encryption using XChaCha20-Poly1305 (AEAD)
 * - Password-based key derivation using Argon2i
 * - Secure random number generation
 * - Key size: 32 bytes (256 bits)
 * - Nonce size: 24 bytes for XChaCha20
 *
 * Example:
 *   // Derive key from password
 *   rs_u8 key[RS_CRYPTO_KEY_SIZE];
 *   rs_u8 salt[RS_CRYPTO_SALT_SIZE];
 *   rs_crypto_random(salt, sizeof(salt));
 *   rs_crypto_derive_key(key, "my-password", salt);
 *
 *   // Encrypt data
 *   rs_string_t ciphertext = rs_string_create(allocator);
 *   rs_crypto_encrypt(&ciphertext, rs_sv_from_cstr("secret"), key);
 *
 *   // Decrypt data
 *   rs_string_t plaintext = rs_string_create(allocator);
 *   rs_crypto_decrypt(&plaintext, rs_sv_from_string(&ciphertext), key);
 */

// ============================================================================
// Constants
// ============================================================================

/** Size of encryption key in bytes (256 bits) */
#define RS_CRYPTO_KEY_SIZE 32

/** Size of salt for key derivation in bytes */
#define RS_CRYPTO_SALT_SIZE 16

/** Size of nonce for XChaCha20 in bytes */
#define RS_CRYPTO_NONCE_SIZE 24

/** Size of authentication tag (MAC) in bytes */
#define RS_CRYPTO_MAC_SIZE 16

// ============================================================================
// Random Number Generation
// ============================================================================

/**
 * Generate cryptographically secure random bytes.
 *
 * Uses the system's cryptographically secure random number generator.
 * On Unix: /dev/urandom
 * On Windows: BCryptGenRandom
 *
 * @param out Output buffer to fill with random bytes
 * @param len Number of random bytes to generate
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_crypto_random(void *out, rs_size_t len);

// ============================================================================
// Key Derivation
// ============================================================================

/**
 * Derive an encryption key from a password using Argon2i.
 *
 * Uses Argon2i (memory-hard) to derive a key from a password and salt.
 * This is suitable for password-based encryption.
 *
 * Parameters are chosen for moderate security:
 * - Iterations: 3
 * - Memory: 64 MiB (65536 KiB)
 *
 * @param key_out Output buffer for derived key (must be RS_CRYPTO_KEY_SIZE bytes)
 * @param password Password string to derive key from
 * @param salt Salt bytes (must be RS_CRYPTO_SALT_SIZE bytes)
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_crypto_derive_key(rs_u8 *key_out, rs_string_view_t password, const rs_u8 *salt);

/**
 * Derive an encryption key from a password with custom Argon2i parameters.
 *
 * @param key_out Output buffer for derived key (must be RS_CRYPTO_KEY_SIZE bytes)
 * @param password Password string to derive key from
 * @param salt Salt bytes (must be RS_CRYPTO_SALT_SIZE bytes)
 * @param iterations Number of iterations (recommended: 3+)
 * @param memory_kib Memory usage in KiB (recommended: 65536 = 64 MiB)
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_crypto_derive_key_ex(rs_u8 *key_out, rs_string_view_t password, const rs_u8 *salt,
                                               rs_u32 iterations, rs_u32 memory_kib);

// ============================================================================
// Encryption/Decryption
// ============================================================================

/**
 * Encrypt data using XChaCha20-Poly1305 (AEAD).
 *
 * The output format is: [24-byte nonce][ciphertext][16-byte MAC]
 * The nonce is randomly generated for each encryption.
 *
 * @param out Output string (will be cleared and filled with encrypted data)
 * @param plaintext Input data to encrypt
 * @param key Encryption key (must be RS_CRYPTO_KEY_SIZE bytes)
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_crypto_encrypt(rs_string_t *out, rs_string_view_t plaintext, const rs_u8 *key);

/**
 * Decrypt data using XChaCha20-Poly1305 (AEAD).
 *
 * The input must be in the format: [24-byte nonce][ciphertext][16-byte MAC]
 * This is the same format produced by rs_crypto_encrypt().
 *
 * @param out Output string (will be cleared and filled with decrypted data)
 * @param ciphertext Input encrypted data (including nonce and MAC)
 * @param key Decryption key (must be RS_CRYPTO_KEY_SIZE bytes)
 * @return RS_OK on success, RS_ERR_INVALID if authentication fails
 */
RS_STD_API rs_result_t rs_crypto_decrypt(rs_string_t *out, rs_string_view_t ciphertext, const rs_u8 *key);

// ============================================================================
// Convenience Functions
// ============================================================================

/**
 * Encrypt data with password (convenience wrapper).
 *
 * This function:
 * 1. Generates a random salt
 * 2. Derives a key from the password using Argon2i
 * 3. Encrypts the data
 * 4. Returns: [16-byte salt][encrypted data]
 *
 * @param out Output string (will be cleared and filled with salt + encrypted data)
 * @param plaintext Input data to encrypt
 * @param password Password to derive encryption key from
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_crypto_encrypt_with_password(rs_string_t *out, rs_string_view_t plaintext,
                                                       rs_string_view_t password);

/**
 * Decrypt data with password (convenience wrapper).
 *
 * This function:
 * 1. Extracts the salt from the input (first 16 bytes)
 * 2. Derives a key from the password using Argon2i
 * 3. Decrypts the data
 *
 * Input format: [16-byte salt][encrypted data]
 *
 * @param out Output string (will be cleared and filled with decrypted data)
 * @param ciphertext Input data (salt + encrypted data)
 * @param password Password to derive decryption key from
 * @return RS_OK on success, RS_ERR_INVALID if authentication fails or format is wrong
 */
RS_STD_API rs_result_t rs_crypto_decrypt_with_password(rs_string_t *out, rs_string_view_t ciphertext,
                                                       rs_string_view_t password);

// ============================================================================
// Base64 Encoding Helpers
// ============================================================================

/**
 * Encrypt data and encode to Base64.
 *
 * Convenience function that encrypts data and Base64-encodes the result.
 * Useful for storing encrypted data in text formats (JSON, YAML, etc.).
 *
 * @param out Output string (will be cleared and filled with Base64-encoded encrypted data)
 * @param plaintext Input data to encrypt
 * @param password Password to derive encryption key from
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_crypto_encrypt_base64(rs_string_t *out, rs_string_view_t plaintext,
                                                rs_string_view_t password);

/**
 * Decode Base64 and decrypt data.
 *
 * Convenience function that Base64-decodes and then decrypts data.
 *
 * @param out Output string (will be cleared and filled with decrypted data)
 * @param ciphertext_base64 Base64-encoded encrypted data
 * @param password Password to derive decryption key from
 * @return RS_OK on success, RS_ERR_INVALID if decode or authentication fails
 */
RS_STD_API rs_result_t rs_crypto_decrypt_base64(rs_string_t *out, rs_string_view_t ciphertext_base64,
                                                rs_string_view_t password);

// ============================================================================
// Memory Wiping
// ============================================================================

/**
 * Securely wipe sensitive data from memory.
 *
 * Uses a compiler barrier to prevent the wipe from being optimized away.
 * Use this to clear keys, passwords, and other sensitive data.
 *
 * @param data Pointer to data to wipe
 * @param len Length of data in bytes
 */
RS_STD_API void rs_crypto_wipe(void *data, rs_size_t len);

RS_EXTERN_C_END
