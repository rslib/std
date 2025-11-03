#include <rs/std/containers/hash.h>
#include <string.h>
#include <xxhash.h>

// ============================================================================
// Default Hash Functions
// ============================================================================

rs_u64 rs_hash_xxh3(const void *data, rs_size_t len, void *user_data)
{
    (void)user_data; // Unused
    return XXH3_64bits(data, len);
}

rs_u64 rs_hash_string(const char *str, void *user_data)
{
    (void)user_data; // Unused
    return XXH3_64bits(str, strlen(str));
}

rs_u64 rs_hash_int64(rs_u64 value, void *user_data)
{
    (void)user_data; // Unused

    // Add a constant to ensure zero input doesn't produce zero output
    value += 0x9e3779b97f4a7c15ULL;

    // Simple mixing function for good distribution
    // Based on MurmurHash3's finalizer
    value ^= value >> 33;
    value *= 0xff51afd7ed558ccdULL;
    value ^= value >> 33;
    value *= 0xc4ceb9fe1a85ec53ULL;
    value ^= value >> 33;

    return value;
}

rs_u64 rs_hash_pointer(const void *ptr, void *user_data)
{
    (void)user_data; // Unused

    // Cast pointer to rs_u64
    // On 32-bit systems, upper 32 bits will be zero
    return (rs_u64)(rs_uintptr)ptr;
}
