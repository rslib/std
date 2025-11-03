#include <rs/std/containers/hashmap.h>
#include <rs/std/error.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/string/zstring_view.h>
#include <string.h>

// ============================================================================
// Internal Helpers
// ============================================================================

// Hash functions for common key types (exposed for use in header macros)
rs_u64 rs_hashmap_internal_hash_cstr(const void *key, rs_size_t key_size, void *user_data)
{
    (void)key_size; // Unused
    const char *str = *(const char **)key;
    return rs_hash_string(str, user_data);
}

rs_u64 rs_hashmap_internal_hash_int(const void *key, rs_size_t key_size, void *user_data)
{
    (void)key_size; // Unused
    rs_u64 value = *(const rs_u64 *)key;
    return rs_hash_int(value, user_data);
}

rs_u64 rs_hashmap_internal_hash_ptr(const void *key, rs_size_t key_size, void *user_data)
{
    (void)key_size; // Unused
    const void *ptr = *(const void **)key;
    return rs_hash_pointer(ptr, user_data);
}

rs_u64 rs_hashmap_internal_hash_string(const void *key, rs_size_t key_size, void *user_data)
{
    (void)key_size; // Unused
    const rs_string_t *str = (const rs_string_t *)key;
    return rs_hash_xxh3(rs_string_cstr(str), rs_string_len(str), user_data);
}

rs_u64 rs_hashmap_internal_hash_string_view(const void *key, rs_size_t key_size, void *user_data)
{
    (void)key_size; // Unused
    const rs_string_view_t *sv = (const rs_string_view_t *)key;
    return rs_hash_xxh3(sv->data, sv->len, user_data);
}

rs_u64 rs_hashmap_internal_hash_zstring_view(const void *key, rs_size_t key_size, void *user_data)
{
    (void)key_size; // Unused
    const rs_zstring_view_t *zsv = (const rs_zstring_view_t *)key;
    return rs_hash_xxh3(zsv->data, zsv->len, user_data);
}

// Equality functions for string key types
rs_bool rs_hashmap_internal_key_eq_string(const void *key1, const void *key2, rs_size_t key_size, void *user_data)
{
    (void)key_size;
    (void)user_data;
    const rs_string_t *s1 = (const rs_string_t *)key1;
    const rs_string_t *s2 = (const rs_string_t *)key2;
    return rs_string_eq(s1, s2);
}

rs_bool rs_hashmap_internal_key_eq_string_view(const void *key1, const void *key2, rs_size_t key_size, void *user_data)
{
    (void)key_size;
    (void)user_data;
    const rs_string_view_t *sv1 = (const rs_string_view_t *)key1;
    const rs_string_view_t *sv2 = (const rs_string_view_t *)key2;
    return rs_sv_eq(*sv1, *sv2);
}

rs_bool rs_hashmap_internal_key_eq_zstring_view(const void *key1, const void *key2, rs_size_t key_size, void *user_data)
{
    (void)key_size;
    (void)user_data;
    const rs_zstring_view_t *zsv1 = (const rs_zstring_view_t *)key1;
    const rs_zstring_view_t *zsv2 = (const rs_zstring_view_t *)key2;
    return rs_zsv_eq(*zsv1, *zsv2);
}

// Equality functions for basic key types (cstr, memcmp, pointer)
rs_bool rs_hashmap_internal_key_eq_memcmp(const void *key1, const void *key2, rs_size_t key_size, void *user_data)
{
    (void)user_data;
    return memcmp(key1, key2, key_size) == 0;
}

rs_bool rs_hashmap_internal_key_eq_cstr(const void *key1, const void *key2, rs_size_t key_size, void *user_data)
{
    (void)key_size; // Unused (strings are null-terminated)
    (void)user_data;

    const char *str1 = *(const char **)key1;
    const char *str2 = *(const char **)key2;

    return strcmp(str1, str2) == 0;
}

rs_bool rs_hashmap_internal_key_eq_ptr(const void *key1, const void *key2, rs_size_t key_size, void *user_data)
{
    (void)key_size;
    (void)user_data;

    const void *ptr1 = *(const void **)key1;
    const void *ptr2 = *(const void **)key2;

    return ptr1 == ptr2;
}

// Round up to next power of 2
static rs_size_t next_power_of_2(rs_size_t n)
{
    if (n == 0)
        return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    n |= n >> 32;
#endif
    return n + 1;
}

// Check if value is power of 2
static rs_bool is_power_of_2(rs_size_t n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

// Hash a key
static rs_u64 hash_key(const rs_hashmap_t *map, const void *key)
{
    if (map->key_size == 0) {
        // Pointer key: key is a pointer to a pointer
        return map->hash_fn(key, 0, map->user_data);
    } else {
        // Value key: hash the data directly
        return map->hash_fn(key, map->key_size, map->user_data);
    }
}

// Check if two keys are equal
static rs_bool keys_equal(const rs_hashmap_t *map, const void *key1, const void *key2)
{
    return map->key_eq_fn(key1, key2, map->key_size, map->user_data);
}

// Allocate and copy key
static void *copy_key(rs_hashmap_t *map, const void *key)
{
    if (map->key_size == 0) {
        // Pointer key: just copy the pointer
        void *key_ptr = rs_alloc(map->allocator, sizeof(void *));
        if (!key_ptr)
            return NULL;
        memcpy(key_ptr, key, sizeof(void *));
        return key_ptr;
    } else {
        // Value key: allocate and copy
        void *key_copy = rs_alloc(map->allocator, map->key_size);
        if (!key_copy)
            return NULL;
        memcpy(key_copy, key, map->key_size);
        return key_copy;
    }
}

// Allocate and copy value
static void *copy_value(rs_hashmap_t *map, const void *value)
{
    if (map->value_size == 0) {
        // Pointer value: just copy the pointer
        void *value_ptr = rs_alloc(map->allocator, sizeof(void *));
        if (!value_ptr)
            return NULL;
        memcpy(value_ptr, value, sizeof(void *));
        return value_ptr;
    } else {
        // Value value: allocate and copy
        void *value_copy = rs_alloc(map->allocator, map->value_size);
        if (!value_copy)
            return NULL;
        memcpy(value_copy, value, map->value_size);
        return value_copy;
    }
}

// Free key
static void free_key(rs_hashmap_t *map, void *key)
{
    rs_size_t size = map->key_size == 0 ? sizeof(void *) : map->key_size;
    if (map->key_destroy_fn) {
        if (map->key_size == 0) {
            // Pointer key: call destructor on the actual pointer
            map->key_destroy_fn(*(void **)key, map->user_data);
        } else {
            // Value key: call destructor on the data
            map->key_destroy_fn(key, map->user_data);
        }
    }
    rs_free(map->allocator, key, size);
}

// Free value
static void free_value(rs_hashmap_t *map, void *value)
{
    rs_size_t size = map->value_size == 0 ? sizeof(void *) : map->value_size;
    if (map->value_destroy_fn) {
        if (map->value_size == 0) {
            // Pointer value: call destructor on the actual pointer
            map->value_destroy_fn(*(void **)value, map->user_data);
        } else {
            // Value value: call destructor on the data
            map->value_destroy_fn(value, map->user_data);
        }
    }
    rs_free(map->allocator, value, size);
}

// Find entry index (returns capacity if not found)
static rs_size_t find_entry(const rs_hashmap_t *map, const void *key, rs_u64 hash)
{
    rs_size_t mask = map->capacity - 1;
    rs_size_t index = (rs_size_t)(hash & mask);
    rs_u8 psl = 0;

    while (1) {
        rs_hashmap_entry_t *entry = &map->entries[index];

        if (entry->state == RS_HASHMAP_SLOT_EMPTY) {
            // Not found
            return map->capacity;
        }

        if (entry->state == RS_HASHMAP_SLOT_OCCUPIED) {
            // Check PSL (Robin Hood: if our PSL exceeds entry's PSL, key can't be here)
            if (psl > entry->psl) {
                return map->capacity;
            }

            // Check hash and key equality
            if (entry->hash == hash && keys_equal(map, key, entry->key)) {
                return index;
            }
        }

        // Continue probing
        psl++;
        index = (index + 1) & mask;
    }
}

// Resize and rehash
static rs_result_t resize_hashmap(rs_hashmap_t *map, rs_size_t new_capacity)
{
    // Allocate new entry array
    rs_hashmap_entry_t *new_entries = rs_alloc(map->allocator, new_capacity * sizeof(rs_hashmap_entry_t));
    if (!new_entries) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate hashmap entries");
    }

    // Initialize all slots to empty
    for (rs_size_t i = 0; i < new_capacity; i++) {
        new_entries[i].state = RS_HASHMAP_SLOT_EMPTY;
        new_entries[i].psl = 0;
    }

    // Save old entries and capacity
    rs_hashmap_entry_t *old_entries = map->entries;
    rs_size_t old_capacity = map->capacity;

    // Update map
    map->entries = new_entries;
    map->capacity = new_capacity;
    map->tombstone_count = 0;

    // Re-insert all occupied entries
    rs_size_t mask = new_capacity - 1;
    for (rs_size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].state == RS_HASHMAP_SLOT_OCCUPIED) {
            rs_u64 hash = old_entries[i].hash;
            void *key = old_entries[i].key;
            void *value = old_entries[i].value;

            // Find insertion point with Robin Hood
            rs_size_t index = (rs_size_t)(hash & mask);
            rs_u8 psl = 0;

            while (1) {
                rs_hashmap_entry_t *entry = &map->entries[index];

                if (entry->state == RS_HASHMAP_SLOT_EMPTY) {
                    // Insert here
                    entry->state = RS_HASHMAP_SLOT_OCCUPIED;
                    entry->psl = psl;
                    entry->hash = hash;
                    entry->key = key;
                    entry->value = value;
                    break;
                }

                // Robin Hood: swap if new entry has higher PSL
                if (psl > entry->psl) {
                    // Swap
                    rs_u8 tmp_psl = entry->psl;
                    rs_u64 tmp_hash = entry->hash;
                    void *tmp_key = entry->key;
                    void *tmp_value = entry->value;

                    entry->psl = psl;
                    entry->hash = hash;
                    entry->key = key;
                    entry->value = value;

                    psl = tmp_psl;
                    hash = tmp_hash;
                    key = tmp_key;
                    value = tmp_value;
                }

                psl++;
                index = (index + 1) & mask;
            }
        }
    }

    // Free old entries array
    rs_free(map->allocator, old_entries, old_capacity * sizeof(rs_hashmap_entry_t));

    return RS_OK;
}

// Insert with Robin Hood hashing
static rs_result_t insers_entry(rs_hashmap_t *map, const void *key, const void *value, rs_bool update,
                                rs_bool *inserted_out)
{
    // Check if we need to resize
    rs_size_t threshold = (rs_size_t)(map->capacity * map->load_factor);
    if (map->size + map->tombstone_count >= threshold) {
        rs_result_t result = resize_hashmap(map, map->capacity * 2);
        if (result != RS_OK) {
            return result;
        }
    }

    rs_u64 hash = hash_key(map, key);
    rs_size_t mask = map->capacity - 1;
    rs_size_t index = (rs_size_t)(hash & mask);
    rs_u8 psl = 0;

    // Copy key and value
    void *key_copy = copy_key(map, key);
    if (!key_copy) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to copy key");
    }

    void *value_copy = copy_value(map, value);
    if (!value_copy) {
        free_key(map, key_copy);
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to copy value");
    }

    while (1) {
        rs_hashmap_entry_t *entry = &map->entries[index];

        if (entry->state == RS_HASHMAP_SLOT_EMPTY || entry->state == RS_HASHMAP_SLOT_TOMBSTONE) {
            // Insert here
            if (entry->state == RS_HASHMAP_SLOT_TOMBSTONE) {
                map->tombstone_count--;
            }
            entry->state = RS_HASHMAP_SLOT_OCCUPIED;
            entry->psl = psl;
            entry->hash = hash;
            entry->key = key_copy;
            entry->value = value_copy;
            map->size++;
            if (inserted_out)
                *inserted_out = true;
            return RS_OK;
        }

        if (entry->state == RS_HASHMAP_SLOT_OCCUPIED) {
            // Check if key already exists
            if (entry->hash == hash && keys_equal(map, key, entry->key)) {
                if (update) {
                    // Update existing value
                    free_value(map, entry->value);
                    entry->value = value_copy;
                    free_key(map, key_copy);
                    if (inserted_out)
                        *inserted_out = false;
                    return RS_OK;
                } else {
                    // Key already exists
                    free_key(map, key_copy);
                    free_value(map, value_copy);
                    return RS_ERROR_RET(RS_ERR_INVALID, "Key already exists");
                }
            }

            // Robin Hood: swap if new entry has higher PSL
            if (psl > entry->psl) {
                // Swap
                rs_u8 tmp_psl = entry->psl;
                rs_u64 tmp_hash = entry->hash;
                void *tmp_key = entry->key;
                void *tmp_value = entry->value;

                entry->psl = psl;
                entry->hash = hash;
                entry->key = key_copy;
                entry->value = value_copy;

                psl = tmp_psl;
                hash = tmp_hash;
                key_copy = tmp_key;
                value_copy = tmp_value;
            }
        }

        psl++;
        index = (index + 1) & mask;
    }
}

// ============================================================================
// Core Functions
// ============================================================================

rs_result_t rs_hashmap_init_with_options(rs_hashmap_t *map, rs_size_t key_size, rs_size_t value_size,
                                         rs_hashmap_options_t opts)
{
    if (!map) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid hashmap pointer");
    }

    if (!opts.allocator) {
        opts.allocator = rs_allocator_system();
    }

    // Set defaults
    rs_size_t capacity = opts.initial_capacity > 0 ? opts.initial_capacity : 16;
    if (!is_power_of_2(capacity)) {
        capacity = next_power_of_2(capacity);
    }

    map->allocator = opts.allocator;
    map->capacity = capacity;
    map->size = 0;
    map->tombstone_count = 0;
    map->key_size = key_size;
    map->value_size = value_size;
    map->load_factor = opts.load_factor > 0.0f ? opts.load_factor : 0.75f;
    map->hash_fn = opts.hash_fn ? opts.hash_fn : rs_hash_xxh3;
    map->key_eq_fn = opts.key_eq_fn ? opts.key_eq_fn : rs_hashmap_internal_key_eq_memcmp;
    map->key_destroy_fn = opts.key_destroy_fn;
    map->value_destroy_fn = opts.value_destroy_fn;
    map->user_data = opts.user_data;

    // Allocate entries
    map->entries = rs_alloc(opts.allocator, capacity * sizeof(rs_hashmap_entry_t));
    if (!map->entries) {
        return RS_ERROR_RET(RS_ERR_NOMEM, "Failed to allocate hashmap entries");
    }

    // Initialize all slots to empty
    for (rs_size_t i = 0; i < capacity; i++) {
        map->entries[i].state = RS_HASHMAP_SLOT_EMPTY;
        map->entries[i].psl = 0;
    }

    return RS_OK;
}

rs_hashmap_t rs_hashmap_create_with_options(rs_size_t key_size, rs_size_t value_size, rs_hashmap_options_t opts)
{
    rs_hashmap_t map = {0};
    rs_result_t result = rs_hashmap_init_with_options(&map, key_size, value_size, opts);
    if (result != RS_OK) {
        // Return zero-initialized map on failure (entries will be NULL)
        rs_hashmap_t failed_map = {0};
        return failed_map;
    }
    return map;
}

void rs_hashmap_destroy(rs_hashmap_t *map)
{
    if (!map || !map->entries)
        return;

    // Free all entries
    for (rs_size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].state == RS_HASHMAP_SLOT_OCCUPIED) {
            free_key(map, map->entries[i].key);
            free_value(map, map->entries[i].value);
        }
    }

    rs_free(map->allocator, map->entries, map->capacity * sizeof(rs_hashmap_entry_t));

    // Zero out the map to prevent use-after-free
    map->entries = NULL;
    map->capacity = 0;
    map->size = 0;
}

// ============================================================================
// Insert/Update
// ============================================================================

rs_result_t rs_hashmap_insert(rs_hashmap_t *map, const void *key, const void *value)
{
    if (!map || !key || !value) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid arguments");
    }

    return insers_entry(map, key, value, false, NULL);
}

rs_result_t rs_hashmap_insers_or_update(rs_hashmap_t *map, const void *key, const void *value, rs_bool *inserted)
{
    if (!map || !key || !value) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid arguments");
    }

    return insers_entry(map, key, value, true, inserted);
}

// ============================================================================
// Lookup
// ============================================================================

void *rs_hashmap_get(rs_hashmap_t *map, const void *key)
{
    return (void *)rs_hashmap_get_const(map, key);
}

const void *rs_hashmap_get_const(const rs_hashmap_t *map, const void *key)
{
    if (!map || !key)
        return NULL;

    rs_u64 hash = hash_key(map, key);
    rs_size_t index = find_entry(map, key, hash);

    if (index == map->capacity) {
        return NULL; // Not found
    }

    // Return pointer to value (or pointer to pointer for pointer values)
    return map->entries[index].value;
}

rs_bool rs_hashmap_contains(const rs_hashmap_t *map, const void *key)
{
    return rs_hashmap_get_const(map, key) != NULL;
}

// ============================================================================
// Remove
// ============================================================================

rs_result_t rs_hashmap_remove(rs_hashmap_t *map, const void *key)
{
    return rs_hashmap_remove_ex(map, key, NULL);
}

rs_result_t rs_hashmap_remove_ex(rs_hashmap_t *map, const void *key, void *old_value_out)
{
    if (!map || !key) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid arguments");
    }

    rs_u64 hash = hash_key(map, key);
    rs_size_t index = find_entry(map, key, hash);

    if (index == map->capacity) {
        return RS_ERROR_RET(RS_ERR_NOTFOUND, "Key not found");
    }

    rs_hashmap_entry_t *entry = &map->entries[index];

    // Copy old value if requested
    if (old_value_out) {
        if (map->value_size == 0) {
            memcpy(old_value_out, entry->value, sizeof(void *));
        } else {
            memcpy(old_value_out, entry->value, map->value_size);
        }
    }

    // Free key and value
    free_key(map, entry->key);
    if (!old_value_out) {
        free_value(map, entry->value);
    } else {
        // Don't call destructor if we're returning the value
        rs_size_t value_size = map->value_size == 0 ? sizeof(void *) : map->value_size;
        rs_free(map->allocator, entry->value, value_size);
    }

    // Mark as tombstone
    entry->state = RS_HASHMAP_SLOT_TOMBSTONE;
    entry->key = NULL;
    entry->value = NULL;
    map->size--;
    map->tombstone_count++;

    // Consider rehashing if too many tombstones
    if (map->tombstone_count > map->capacity / 4) {
        resize_hashmap(map, map->capacity);
    }

    return RS_OK;
}

// ============================================================================
// Size/Capacity
// ============================================================================

rs_size_t rs_hashmap_size(const rs_hashmap_t *map)
{
    return map ? map->size : 0;
}

rs_size_t rs_hashmap_capacity(const rs_hashmap_t *map)
{
    return map ? map->capacity : 0;
}

rs_bool rs_hashmap_empty(const rs_hashmap_t *map)
{
    return map ? map->size == 0 : true;
}

void rs_hashmap_clear(rs_hashmap_t *map)
{
    if (!map)
        return;

    // Free all entries
    for (rs_size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].state == RS_HASHMAP_SLOT_OCCUPIED) {
            free_key(map, map->entries[i].key);
            free_value(map, map->entries[i].value);
        }
        map->entries[i].state = RS_HASHMAP_SLOT_EMPTY;
        map->entries[i].psl = 0;
    }

    map->size = 0;
    map->tombstone_count = 0;
}

rs_result_t rs_hashmap_reserve(rs_hashmap_t *map, rs_size_t new_capacity)
{
    if (!map) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid hashmap");
    }

    if (!is_power_of_2(new_capacity)) {
        new_capacity = next_power_of_2(new_capacity);
    }

    if (new_capacity <= map->capacity) {
        return RS_OK; // Already have enough capacity
    }

    return resize_hashmap(map, new_capacity);
}

// ============================================================================
// Iteration
// ============================================================================

rs_hashmap_iter_t rs_hashmap_iter_begin(rs_hashmap_t *map)
{
    rs_hashmap_iter_t iter = {.map = map, .index = 0};

    if (!map)
        return iter;

    // Find first occupied slot
    while (iter.index < map->capacity && map->entries[iter.index].state != RS_HASHMAP_SLOT_OCCUPIED) {
        iter.index++;
    }

    return iter;
}

rs_bool rs_hashmap_iter_valid(const rs_hashmap_iter_t *iter)
{
    if (!iter || !iter->map)
        return false;
    return iter->index < iter->map->capacity;
}

void rs_hashmap_iter_next(rs_hashmap_iter_t *iter)
{
    if (!iter || !iter->map)
        return;

    iter->index++;

    // Find next occupied slot
    while (iter->index < iter->map->capacity && iter->map->entries[iter->index].state != RS_HASHMAP_SLOT_OCCUPIED) {
        iter->index++;
    }
}

void *rs_hashmap_iter_key(rs_hashmap_iter_t *iter)
{
    if (!iter || !iter->map || iter->index >= iter->map->capacity)
        return NULL;
    return iter->map->entries[iter->index].key;
}

void *rs_hashmap_iter_value(rs_hashmap_iter_t *iter)
{
    if (!iter || !iter->map || iter->index >= iter->map->capacity)
        return NULL;
    return iter->map->entries[iter->index].value;
}
