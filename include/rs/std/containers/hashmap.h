#pragma once

#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/hash.h>
#include <rs/std/internal/api.h>
#include <rs/std/types.h>
#include <stdbool.h>
#include <stdint.h>

RS_EXTERN_C_BEGIN

// ============================================================================
// Forward Declarations & Internal Types
// ============================================================================

// Slot state for Robin Hood hashing
typedef enum {
    RS_HASHMAP_SLOT_EMPTY = 0,    // Never used
    RS_HASHMAP_SLOT_OCCUPIED = 1, // Currently used
    RS_HASHMAP_SLOT_TOMBSTONE = 2 // Previously used, now deleted
} rs_hashmap_slot_state_t;

// Entry structure
typedef struct {
    rs_u8 state; // Slot state
    rs_u8 psl;   // Probe sequence length (for Robin Hood)
    rs_u64 hash; // Cached hash value
    void *key;   // Pointer to key data
    void *value; // Pointer to value data
} rs_hashmap_entry_t;

// Main hashmap structure
typedef struct rs_hashmap {
    rs_allocator_t *allocator;
    rs_hashmap_entry_t *entries; // Array of entries
    rs_size_t capacity;          // Number of slots (power of 2)
    rs_size_t size;              // Number of occupied slots
    rs_size_t tombstone_count;   // Number of tombstone slots
    rs_size_t key_size;          // Size of key type (0 = pointer)
    rs_size_t value_size;        // Size of value type (0 = pointer)
    float load_factor;           // Resize threshold (default 0.75)

    // Function pointers
    rs_hash_fn hash_fn;             // Hash function
    rs_key_eq_fn key_eq_fn;         // Key equality function
    rs_destroy_fn key_destroy_fn;   // Optional key destructor
    rs_destroy_fn value_destroy_fn; // Optional value destructor
    void *user_data;                // User data for callbacks
} rs_hashmap_t;

// ============================================================================
// Configuration
// ============================================================================

/**
 * Configuration for hashmap creation.
 */
typedef struct {
    rs_allocator_t *allocator;      // Allocator to use (NULL = system allocator)
    rs_hash_fn hash_fn;             // Optional custom hash (NULL = use xxHash)
    rs_key_eq_fn key_eq_fn;         // Optional custom equality (NULL = memcmp)
    rs_destroy_fn key_destroy_fn;   // Optional key destructor
    rs_destroy_fn value_destroy_fn; // Optional value destructor
    void *user_data;                // Passed to all callbacks
    rs_size_t initial_capacity;     // Default: 16 (must be power of 2)
    float load_factor;              // Default: 0.75
    rs_u32 reserved;                // Reserved for future use
} rs_hashmap_options_t;

// ============================================================================
// Core Functions
// ============================================================================

/**
 * Initialize a stack-allocated hashmap with options.
 *
 * @param map Pointer to uninitialized hashmap
 * @param key_size Size of key type (0 = store key as pointer)
 * @param value_size Size of value type (0 = store value as pointer)
 * @param opts Hashmap options
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 */
RS_STD_API rs_result_t rs_hashmap_init_with_options(rs_hashmap_t *map, rs_size_t key_size, rs_size_t value_size,
                                                    rs_hashmap_options_t opts);

/**
 * Initialize a stack-allocated hashmap.
 *
 * @param map Pointer to uninitialized hashmap
 * @param key_size Size of key type (0 = store key as pointer)
 * @param value_size Size of value type (0 = store value as pointer)
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @param .initial_capacity: rs_size_t (default: 16)
 * @param .load_factor: float (default: 0.75)
 * @param .hash_fn: rs_hash_fn (default: xxHash)
 * @param .key_eq_fn: rs_key_eq_fn (default: memcmp)
 * @param .key_destroy_fn: rs_destroy_fn (default: NULL)
 * @param .value_destroy_fn: rs_destroy_fn (default: NULL)
 * @param .user_data: void* (default: NULL)
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 *
 * Example:
 *   rs_hashmap_t map = {0};
 *   rs_result_t result = rs_hashmap_init(&map, sizeof(int), sizeof(char*));
 *   // ... use map ...
 *   rs_hashmap_destroy(&map);
 */
#define rs_hashmap_init(map, key_size, value_size, ...)                                                                \
    rs_hashmap_init_with_options(map, key_size, value_size, (rs_hashmap_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Create a new hashmap (returns by value) with options.
 *
 * @param key_size Size of key type (0 = store key as pointer)
 * @param value_size Size of value type (0 = store value as pointer)
 * @param opts Hashmap options
 * @return New hashmap by value (check map.entries != NULL for success)
 */
RS_STD_API rs_hashmap_t rs_hashmap_create_with_options(rs_size_t key_size, rs_size_t value_size,
                                                       rs_hashmap_options_t opts);

/**
 * Create a new hashmap (returns by value).
 *
 * @param key_size Size of key type (0 = store key as pointer)
 * @param value_size Size of value type (0 = store value as pointer)
 * @param .allocator (optional): rs_allocator_t* (default: system allocator)
 * @param .initial_capacity: rs_size_t (default: 16)
 * @param .load_factor: float (default: 0.75)
 * @param .hash_fn: rs_hash_fn (default: xxHash)
 * @param .key_eq_fn: rs_key_eq_fn (default: memcmp)
 * @param .key_destroy_fn: rs_destroy_fn (default: NULL)
 * @param .value_destroy_fn: rs_destroy_fn (default: NULL)
 * @param .user_data: void* (default: NULL)
 * @return New hashmap by value (check map.entries != NULL for success)
 *
 * Example:
 *   rs_hashmap_t map = rs_hashmap_create(sizeof(int), sizeof(char*));
 *   if (!map.entries) {
 *       // handle error
 *   }
 *   // ... use map ...
 *   rs_hashmap_destroy(&map);
 */
#define rs_hashmap_create(key_size, value_size, ...)                                                                   \
    rs_hashmap_create_with_options(key_size, value_size, (rs_hashmap_options_t){.reserved = 0, ##__VA_ARGS__})

/**
 * Destroy a hashmap and free all internal memory.
 *
 * Calls destructors for all remaining entries if configured.
 * Works for both stack-allocated and heap-allocated hashmaps.
 *
 * @param map Pointer to hashmap to destroy
 */
RS_STD_API void rs_hashmap_destroy(rs_hashmap_t *map);

// ============================================================================
// Insert/Update
// ============================================================================

/**
 * Insert a key-value pair into the hashmap.
 *
 * If the key already exists, this function returns RS_ERR_EXISTS without
 * modifying the map. Use rs_hashmap_insers_or_update() to update existing keys.
 *
 * @param map Hashmap
 * @param key Pointer to key data (copied if key_size > 0, else stored as-is)
 * @param value Pointer to value data (copied if value_size > 0, else stored as-is)
 * @return RS_OK on success, RS_ERR_EXISTS if key exists, RS_ERR_NOMEM on allocation failure
 */
RS_STD_API rs_result_t rs_hashmap_insert(rs_hashmap_t *map, const void *key, const void *value);

/**
 * Insert or update a key-value pair.
 *
 * If the key already exists, the value is updated (old value destructor is called if set).
 *
 * @param map Hashmap
 * @param key Pointer to key data
 * @param value Pointer to value data
 * @param inserted Optional output: true if inserted, false if updated
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 */
RS_STD_API rs_result_t rs_hashmap_insers_or_update(rs_hashmap_t *map, const void *key, const void *value,
                                                   rs_bool *inserted);

// ============================================================================
// Lookup
// ============================================================================

/**
 * Get a pointer to the value associated with a key.
 *
 * @param map Hashmap
 * @param key Pointer to key data
 * @return Pointer to value, or NULL if key not found
 */
RS_STD_API void *rs_hashmap_get(rs_hashmap_t *map, const void *key);

/**
 * Get a const pointer to the value associated with a key.
 *
 * @param map Hashmap (const)
 * @param key Pointer to key data
 * @return Const pointer to value, or NULL if key not found
 */
RS_STD_API const void *rs_hashmap_get_const(const rs_hashmap_t *map, const void *key);

/**
 * Check if a key exists in the hashmap.
 *
 * @param map Hashmap
 * @param key Pointer to key data
 * @return true if key exists, false otherwise
 */
RS_STD_API rs_bool rs_hashmap_contains(const rs_hashmap_t *map, const void *key);

// ============================================================================
// Remove
// ============================================================================

/**
 * Remove a key-value pair from the hashmap.
 *
 * Calls destructors if configured.
 *
 * @param map Hashmap
 * @param key Pointer to key data
 * @return RS_OK on success, RS_ERR_NOT_FOUND if key doesn't exist
 */
RS_STD_API rs_result_t rs_hashmap_remove(rs_hashmap_t *map, const void *key);

/**
 * Remove a key-value pair and optionally retrieve the old value.
 *
 * @param map Hashmap
 * @param key Pointer to key data
 * @param old_value_out Optional: copy removed value here (before destructor is called)
 * @return RS_OK on success, RS_ERR_NOT_FOUND if key doesn't exist
 */
RS_STD_API rs_result_t rs_hashmap_remove_ex(rs_hashmap_t *map, const void *key, void *old_value_out);

// ============================================================================
// Size/Capacity
// ============================================================================

/**
 * Get the number of entries in the hashmap.
 *
 * @param map Hashmap
 * @return Number of entries
 */
RS_STD_API rs_size_t rs_hashmap_size(const rs_hashmap_t *map);

/**
 * Get the capacity (number of slots) in the hashmap.
 *
 * @param map Hashmap
 * @return Capacity
 */
RS_STD_API rs_size_t rs_hashmap_capacity(const rs_hashmap_t *map);

/**
 * Check if the hashmap is empty.
 *
 * @param map Hashmap
 * @return true if empty, false otherwise
 */
RS_STD_API rs_bool rs_hashmap_empty(const rs_hashmap_t *map);

/**
 * Remove all entries from the hashmap.
 *
 * Calls destructors for all entries if configured.
 *
 * @param map Hashmap
 */
RS_STD_API void rs_hashmap_clear(rs_hashmap_t *map);

/**
 * Reserve capacity for at least the specified number of entries.
 *
 * This can be used to preallocate space and avoid resizing during insertions.
 *
 * @param map Hashmap
 * @param new_capacity Minimum capacity (will be rounded up to power of 2)
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 */
RS_STD_API rs_result_t rs_hashmap_reserve(rs_hashmap_t *map, rs_size_t new_capacity);

// ============================================================================
// Iteration
// ============================================================================

/**
 * Iterator for traversing hashmap entries.
 */
typedef struct {
    rs_hashmap_t *map;
    rs_size_t index;
} rs_hashmap_iter_t;

/**
 * Get an iterator to the beginning of the hashmap.
 *
 * @param map Hashmap
 * @return Iterator
 */
RS_STD_API rs_hashmap_iter_t rs_hashmap_iter_begin(rs_hashmap_t *map);

/**
 * Check if an iterator is valid.
 *
 * @param iter Iterator
 * @return true if valid, false if end of hashmap
 */
RS_STD_API rs_bool rs_hashmap_iter_valid(const rs_hashmap_iter_t *iter);

/**
 * Advance iterator to next entry.
 *
 * @param iter Iterator
 */
RS_STD_API void rs_hashmap_iter_next(rs_hashmap_iter_t *iter);

/**
 * Get pointer to current entry's key.
 *
 * @param iter Iterator
 * @return Pointer to key
 */
RS_STD_API void *rs_hashmap_iter_key(rs_hashmap_iter_t *iter);

/**
 * Get pointer to current entry's value.
 *
 * @param iter Iterator
 * @return Pointer to value
 */
RS_STD_API void *rs_hashmap_iter_value(rs_hashmap_iter_t *iter);

/**
 * Macro for iterating over all entries in a hashmap.
 *
 * Example:
 *   RS_HASHMAP_FOREACH(map, iter) {
 *       int *key = rs_hashmap_iter_key(&iter);
 *       char *value = rs_hashmap_iter_value(&iter);
 *       printf("%d -> %s\n", *key, value);
 *   }
 */
#define RS_HASHMAP_FOREACH(map, iter)                                                                                  \
    for (rs_hashmap_iter_t iter = rs_hashmap_iter_begin(map); rs_hashmap_iter_valid(&iter); rs_hashmap_iter_next(&iter))

// ============================================================================
// Convenience Wrappers - Common Key Types
// ============================================================================

// Internal hash functions (do not use directly)
RS_STD_API rs_u64 rs_hashmap_internal_hash_cstr(const void *key, rs_size_t key_size, void *user_data);
RS_STD_API rs_u64 rs_hashmap_internal_hash_int(const void *key, rs_size_t key_size, void *user_data);
RS_STD_API rs_u64 rs_hashmap_internal_hash_ptr(const void *key, rs_size_t key_size, void *user_data);
RS_STD_API rs_u64 rs_hashmap_internal_hash_string(const void *key, rs_size_t key_size, void *user_data);
RS_STD_API rs_u64 rs_hashmap_internal_hash_string_view(const void *key, rs_size_t key_size, void *user_data);
RS_STD_API rs_u64 rs_hashmap_internal_hash_zstring_view(const void *key, rs_size_t key_size, void *user_data);

// Internal equality functions (do not use directly)
RS_STD_API rs_bool rs_hashmap_internal_key_eq_memcmp(const void *key1, const void *key2, rs_size_t key_size,
                                                     void *user_data);
RS_STD_API rs_bool rs_hashmap_internal_key_eq_cstr(const void *key1, const void *key2, rs_size_t key_size,
                                                   void *user_data);
RS_STD_API rs_bool rs_hashmap_internal_key_eq_ptr(const void *key1, const void *key2, rs_size_t key_size,
                                                  void *user_data);
RS_STD_API rs_bool rs_hashmap_internal_key_eq_string(const void *key1, const void *key2, rs_size_t key_size,
                                                     void *user_data);
RS_STD_API rs_bool rs_hashmap_internal_key_eq_string_view(const void *key1, const void *key2, rs_size_t key_size,
                                                          void *user_data);
RS_STD_API rs_bool rs_hashmap_internal_key_eq_zstring_view(const void *key1, const void *key2, rs_size_t key_size,
                                                           void *user_data);

/**
 * Predefined options for C-string keys (null-terminated const char*).
 * Keys are stored as pointers (not copied).
 *
 * Usage:
 *   rs_hashmap_t map = rs_hashmap_create(0, sizeof(int),
 *                                         RS_HASHMAP_CSTR_OPTIONS(.allocator = my_alloc));
 */
#define RS_HASHMAP_CSTR_OPTIONS(...)                                                                                   \
    (rs_hashmap_options_t)                                                                                             \
    {                                                                                                                  \
        .hash_fn = rs_hashmap_internal_hash_cstr, .key_eq_fn = rs_hashmap_internal_key_eq_cstr, .reserved = 0,         \
        ##__VA_ARGS__                                                                                                  \
    }

/**
 * Predefined options for integer keys (uint64_t).
 *
 * Usage:
 *   rs_hashmap_t map = rs_hashmap_create(sizeof(uint64_t), sizeof(int),
 *                                         RS_HASHMAP_INT_OPTIONS());
 */
#define RS_HASHMAP_INT_OPTIONS(...)                                                                                    \
    (rs_hashmap_options_t)                                                                                             \
    {                                                                                                                  \
        .hash_fn = rs_hashmap_internal_hash_int, .key_eq_fn = rs_hashmap_internal_key_eq_memcmp, .reserved = 0,        \
        ##__VA_ARGS__                                                                                                  \
    }

/**
 * Predefined options for pointer keys (void*).
 * Uses identity hashing (pointer address comparison).
 *
 * Usage:
 *   rs_hashmap_t map = rs_hashmap_create(0, sizeof(int),
 *                                         RS_HASHMAP_PTR_OPTIONS());
 */
#define RS_HASHMAP_PTR_OPTIONS(...)                                                                                    \
    (rs_hashmap_options_t)                                                                                             \
    {                                                                                                                  \
        .hash_fn = rs_hashmap_internal_hash_ptr, .key_eq_fn = rs_hashmap_internal_key_eq_ptr, .reserved = 0,           \
        ##__VA_ARGS__                                                                                                  \
    }

/**
 * Initialize a stack-allocated hashmap with C-string keys.
 *
 * @param map Pointer to uninitialized hashmap
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 *
 * Example:
 *   rs_hashmap_t map;
 *   rs_hashmap_cstr_init(&map, sizeof(int), .allocator = my_alloc);
 *   rs_hashmap_insert(&map, &"key", &value);
 */
#define rs_hashmap_cstr_init(map, value_size, ...)                                                                     \
    rs_hashmap_init_with_options(map, 0, value_size, RS_HASHMAP_CSTR_OPTIONS(__VA_ARGS__))

/**
 * Create a hashmap with C-string keys (null-terminated const char*).
 *
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return New hashmap by value (check map.entries != NULL for success)
 *
 * Example:
 *   rs_hashmap_t map = rs_hashmap_cstr_create(sizeof(int));
 *   rs_hashmap_insert(&map, &"key", &value);
 */
#define rs_hashmap_cstr_create(value_size, ...)                                                                        \
    rs_hashmap_create_with_options(0, value_size, RS_HASHMAP_CSTR_OPTIONS(__VA_ARGS__))

/**
 * Initialize a stack-allocated hashmap with integer keys.
 *
 * @param map Pointer to uninitialized hashmap
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 *
 * Example:
 *   rs_hashmap_t map;
 *   rs_hashmap_int_init(&map, sizeof(int));
 *   uint64_t key = 42;
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_int_init(map, value_size, ...)                                                                      \
    rs_hashmap_init_with_options(map, sizeof(uint64_t), value_size, RS_HASHMAP_INT_OPTIONS(__VA_ARGS__))

/**
 * Create a hashmap with integer keys (uint64_t).
 *
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return New hashmap by value (check map.entries != NULL for success)
 *
 * Example:
 *   rs_hashmap_t map = rs_hashmap_int_create(sizeof(int));
 *   uint64_t key = 42;
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_int_create(value_size, ...)                                                                         \
    rs_hashmap_create_with_options(sizeof(uint64_t), value_size, RS_HASHMAP_INT_OPTIONS(__VA_ARGS__))

/**
 * Initialize a stack-allocated hashmap with pointer keys.
 *
 * @param map Pointer to uninitialized hashmap
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 *
 * Example:
 *   rs_hashmap_t map;
 *   rs_hashmap_ptr_init(&map, sizeof(int));
 *   void *key = some_pointer;
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_ptr_init(map, value_size, ...)                                                                      \
    rs_hashmap_init_with_options(map, 0, value_size, RS_HASHMAP_PTR_OPTIONS(__VA_ARGS__))

/**
 * Create a hashmap with pointer keys (void*).
 *
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return New hashmap by value (check map.entries != NULL for success)
 *
 * Example:
 *   rs_hashmap_t map = rs_hashmap_ptr_create(sizeof(int));
 *   void *key = some_pointer;
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_ptr_create(value_size, ...)                                                                         \
    rs_hashmap_create_with_options(0, value_size, RS_HASHMAP_PTR_OPTIONS(__VA_ARGS__))

/**
 * Predefined options for rs_string_t keys (owned, mutable strings).
 * Keys are stored by value (copied).
 *
 * Usage:
 *   rs_hashmap_t map = rs_hashmap_create(sizeof(rs_string_t), sizeof(int),
 *                                         RS_HASHMAP_STRING_OPTIONS());
 */
#define RS_HASHMAP_STRING_OPTIONS(...)                                                                                 \
    (rs_hashmap_options_t)                                                                                             \
    {                                                                                                                  \
        .hash_fn = rs_hashmap_internal_hash_string, .key_eq_fn = rs_hashmap_internal_key_eq_string, .reserved = 0,     \
        ##__VA_ARGS__                                                                                                  \
    }

/**
 * Initialize a stack-allocated hashmap with rs_string_t keys.
 *
 * @param map Pointer to uninitialized hashmap
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 *
 * Example:
 *   rs_hashmap_t map;
 *   rs_hashmap_string_init(&map, sizeof(int));
 *   rs_string_t key = rs_string_from_cstr("hello");
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_string_init(map, value_size, ...)                                                                   \
    rs_hashmap_init_with_options(map, sizeof(rs_string_t), value_size, RS_HASHMAP_STRING_OPTIONS(__VA_ARGS__))

/**
 * Create a hashmap with rs_string_t keys (owned, mutable strings).
 *
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return New hashmap by value (check map.entries != NULL for success)
 *
 * Example:
 *   rs_hashmap_t map = rs_hashmap_string_create(sizeof(int));
 *   rs_string_t key = rs_string_from_cstr("hello");
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_string_create(value_size, ...)                                                                      \
    rs_hashmap_create_with_options(sizeof(rs_string_t), value_size, RS_HASHMAP_STRING_OPTIONS(__VA_ARGS__))

/**
 * Predefined options for rs_string_view_t keys (non-owning views).
 * Keys are stored by value (copied).
 *
 * Usage:
 *   rs_hashmap_t map = rs_hashmap_create(sizeof(rs_string_view_t), sizeof(int),
 *                                         RS_HASHMAP_STRING_VIEW_OPTIONS());
 */
#define RS_HASHMAP_STRING_VIEW_OPTIONS(...)                                                                            \
    (rs_hashmap_options_t)                                                                                             \
    {                                                                                                                  \
        .hash_fn = rs_hashmap_internal_hash_string_view, .key_eq_fn = rs_hashmap_internal_key_eq_string_view,          \
        .reserved = 0, ##__VA_ARGS__                                                                                   \
    }

/**
 * Initialize a stack-allocated hashmap with rs_string_view_t keys.
 *
 * @param map Pointer to uninitialized hashmap
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 *
 * Example:
 *   rs_hashmap_t map;
 *   rs_hashmap_string_view_init(&map, sizeof(int));
 *   rs_string_view_t key = rs_sv_from_cstr("hello");
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_string_view_init(map, value_size, ...)                                                              \
    rs_hashmap_init_with_options(map, sizeof(rs_string_view_t), value_size, RS_HASHMAP_STRING_VIEW_OPTIONS(__VA_ARGS__))

/**
 * Create a hashmap with rs_string_view_t keys (non-owning views).
 *
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return New hashmap by value (check map.entries != NULL for success)
 *
 * Example:
 *   rs_hashmap_t map = rs_hashmap_string_view_create(sizeof(int));
 *   rs_string_view_t key = rs_sv_from_cstr("hello");
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_string_view_create(value_size, ...)                                                                 \
    rs_hashmap_create_with_options(sizeof(rs_string_view_t), value_size, RS_HASHMAP_STRING_VIEW_OPTIONS(__VA_ARGS__))

/**
 * Predefined options for rs_zstring_view_t keys (null-terminated views).
 * Keys are stored by value (copied).
 *
 * Usage:
 *   rs_hashmap_t map = rs_hashmap_create(sizeof(rs_zstring_view_t), sizeof(int),
 *                                         RS_HASHMAP_ZSTRING_VIEW_OPTIONS());
 */
#define RS_HASHMAP_ZSTRING_VIEW_OPTIONS(...)                                                                           \
    (rs_hashmap_options_t)                                                                                             \
    {                                                                                                                  \
        .hash_fn = rs_hashmap_internal_hash_zstring_view, .key_eq_fn = rs_hashmap_internal_key_eq_zstring_view,        \
        .reserved = 0, ##__VA_ARGS__                                                                                   \
    }

/**
 * Initialize a stack-allocated hashmap with rs_zstring_view_t keys.
 *
 * @param map Pointer to uninitialized hashmap
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return RS_OK on success, RS_ERR_NOMEM on allocation failure
 *
 * Example:
 *   rs_hashmap_t map;
 *   rs_hashmap_zstring_view_init(&map, sizeof(int));
 *   rs_zstring_view_t key = rs_zsv_from_cstr("hello");
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_zstring_view_init(map, value_size, ...)                                                             \
    rs_hashmap_init_with_options(map, sizeof(rs_zstring_view_t), value_size,                                           \
                                 RS_HASHMAP_ZSTRING_VIEW_OPTIONS(__VA_ARGS__))

/**
 * Create a hashmap with rs_zstring_view_t keys (null-terminated views).
 *
 * @param value_size Size of value type (0 = store value as pointer)
 * @param ... Optional: .allocator, .initial_capacity, .load_factor, .value_destroy_fn, .user_data
 * @return New hashmap by value (check map.entries != NULL for success)
 *
 * Example:
 *   rs_hashmap_t map = rs_hashmap_zstring_view_create(sizeof(int));
 *   rs_zstring_view_t key = rs_zsv_from_cstr("hello");
 *   rs_hashmap_insert(&map, &key, &value);
 */
#define rs_hashmap_zstring_view_create(value_size, ...)                                                                \
    rs_hashmap_create_with_options(sizeof(rs_zstring_view_t), value_size, RS_HASHMAP_ZSTRING_VIEW_OPTIONS(__VA_ARGS__))

RS_EXTERN_C_END
