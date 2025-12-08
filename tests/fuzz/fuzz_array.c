#include <rs/std/containers/array.h>
#include <rs/std/types.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2) {
        return 0;
    }

    rs_array_t arr = rs_array_create(sizeof(int));

    for (size_t i = 0; i + 1 < size; i += 2) {
        uint8_t cmd = data[i] % 8;
        int value = (int)data[i + 1];

        switch (cmd) {
        case 0: // Push
            rs_array_push(&arr, &value);
            break;
        case 1: // Pop
            rs_array_pop(&arr, NULL);
            break;
        case 2: // Get (if not empty)
            if (rs_array_len(&arr) > 0) {
                size_t idx = value % rs_array_len(&arr);
                (void)rs_array_get(&arr, idx);
            }
            break;
        case 3: // Insert (if not too large)
            if (rs_array_len(&arr) < 1000) {
                size_t idx = rs_array_len(&arr) > 0 ? value % rs_array_len(&arr) : 0;
                rs_array_insert(&arr, idx, &value);
            }
            break;
        case 4: // Remove (if not empty)
            if (rs_array_len(&arr) > 0) {
                size_t idx = value % rs_array_len(&arr);
                rs_array_remove(&arr, idx, NULL);
            }
            break;
        case 5: // Clear
            rs_array_clear(&arr);
            break;
        case 6:                // Reserve
            if (value < 100) { // Limit allocation
                rs_array_reserve(&arr, value);
            }
            break;
        case 7:                // Resize
            if (value < 100) { // Limit allocation
                rs_array_resize(&arr, value);
            }
            break;
        }
    }

    // Test properties
    (void)rs_array_len(&arr);
    (void)rs_array_cap(&arr);
    (void)rs_array_is_empty(&arr);
    (void)rs_array_first(&arr);
    (void)rs_array_last(&arr);

    // Clone and destroy clone
    rs_array_t clone = rs_array_clone(arr);
    rs_array_destroy(&clone);

    rs_array_destroy(&arr);
    return 0;
}
