#include <rs/std/allocators/allocator.h>
#include <rs/std/containers/hashmap.h>
#include <rs/std/types.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2) {
        return 0;
    }

    rs_hashmap_t map = rs_hashmap_int_create(sizeof(int));

    // Use input bytes as commands and keys
    for (size_t i = 0; i + 1 < size; i += 2) {
        uint8_t cmd = data[i] % 4;
        rs_u64 key = data[i + 1];
        int value = (int)key;

        switch (cmd) {
        case 0:
            rs_hashmap_insert(&map, &key, &value);
            break;
        case 1:
            rs_hashmap_get(&map, &key);
            break;
        case 2:
            rs_hashmap_remove(&map, &key);
            break;
        case 3:
            rs_hashmap_contains(&map, &key);
            break;
        }
    }

    // Iterate over all entries
    RS_HASHMAP_FOREACH(&map, iter)
    {
        (void)rs_hashmap_iter_key(&iter);
        (void)rs_hashmap_iter_value(&iter);
    }

    rs_hashmap_destroy(&map);

    return 0;
}
