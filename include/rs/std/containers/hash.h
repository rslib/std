#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN

// ============================================================================
// Hash Function Types
// ============================================================================

/**
 * Hash function signature.
 *
 * @param key Pointer to key data
 * @param key_size Size of key in bytes (0 if key is a pointer)
 * @param user_data Optional user data passed to the hash function
 * @return 64-bit hash value
 */
typedef rs_u64 (*rs_hash_fn)(const void *key, rs_size_t key_size, void *user_data);

/**
 * Key equality function signature.
 *
 * @param key1 First key to compare
 * @param key2 Second key to compare
 * @param key_size Size of keys in bytes (0 if keys are pointers)
 * @param user_data Optional user data passed to the equality function
 * @return true if keys are equal, false otherwise
 */
typedef rs_bool (*rs_key_eq_fn)(const void *key1, const void *key2, rs_size_t key_size, void *user_data);

/**
 * Destructor function signature.
 *
 * @param data Pointer to data to destroy
 * @param user_data Optional user data passed to the destructor
 */
typedef void (*rs_destroy_fn)(void *data, void *user_data);

// ============================================================================
// Default Hash Functions
// ============================================================================

/**
 * Hash arbitrary data using xxHash XXH3_64bits (default hash function).
 *
 * This is the default hash function used by hashmaps. It provides excellent
 * performance and distribution for most use cases.
 *
 * @param data Pointer to data to hash
 * @param len Length of data in bytes
 * @param user_data Unused (for API compatibility)
 * @return 64-bit hash value
 */
RS_STD_API rs_u64 rs_hash_xxh3(const void *data, rs_size_t len, void *user_data);

/**
 * Hash a null-terminated C string.
 *
 * @param str String to hash (must be null-terminated)
 * @param user_data Unused (for API compatibility)
 * @return 64-bit hash value
 */
RS_STD_API rs_u64 rs_hash_string(const char *str, void *user_data);

/**
 * Hash a 64-bit integer.
 *
 * Uses a simple mixing function for good distribution.
 *
 * @param value Integer value to hash
 * @param user_data Unused (for API compatibility)
 * @return 64-bit hash value
 */
RS_STD_API rs_u64 rs_hash_int64(rs_u64 value, void *user_data);

#define rs_hash_int(value, user_data) rs_hash_int64((rs_u64)(value), user_data)

/**
 * Hash a pointer (identity-based hashing).
 *
 * Simply returns the pointer value cast to rs_u64. Useful for
 * pointer-based keys where identity matters more than content.
 *
 * @param ptr Pointer to hash
 * @param user_data Unused (for API compatibility)
 * @return 64-bit hash value (pointer address)
 */
RS_STD_API rs_u64 rs_hash_pointer(const void *ptr, void *user_data);

RS_EXTERN_C_END
