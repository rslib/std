#include <rs/std/allocators/allocator.h>
#include <rs/std/encoding/base64.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Test encode: arbitrary binary data -> base64
    rs_string_t encoded = rs_string_create();
    rs_base64_encode(&encoded, data, size);

    // Test decode round-trip: base64 -> binary
    rs_string_t decoded = rs_string_create();
    rs_base64_decode(&decoded, rs_sv_from_string(&encoded));

    // Test decode arbitrary input (may fail with invalid base64)
    rs_string_t decoded_raw = rs_string_create();
    rs_string_view_t input = rs_sv_from_buf((const char *)data, size);
    rs_base64_decode(&decoded_raw, input);

    rs_string_destroy(&encoded);
    rs_string_destroy(&decoded);
    rs_string_destroy(&decoded_raw);

    return 0;
}
