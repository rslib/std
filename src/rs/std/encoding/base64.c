#include <rs/std/encoding/base64.h>
#include <rs/std/error.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <string.h>

// Standard Base64 alphabet (RFC 4648)
static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Decoding table: maps ASCII character to 6-bit value (or 0xFF for invalid)
static const rs_u8 base64_decode_table[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0-15
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 16-31
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F, // 32-47
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 48-63
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, // 64-79
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 80-95
    0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, // 96-111
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 112-127
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 128-143
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 144-159
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 160-175
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 176-191
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 192-207
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 208-223
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 224-239
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // 240-255
};

// ============================================================================
// Size Calculation
// ============================================================================

rs_size_t rs_base64_encoded_size(rs_size_t input_len)
{
    // Each 3 bytes of input becomes 4 bytes of output
    // Round up to nearest multiple of 3, then multiply by 4/3
    return ((input_len + 2) / 3) * 4;
}

rs_size_t rs_base64_decoded_size(rs_size_t encoded_len)
{
    // Each 4 bytes of Base64 becomes 3 bytes of output (maximum)
    // Actual size may be 1-2 bytes smaller due to padding
    return (encoded_len / 4) * 3;
}

// ============================================================================
// Encoding
// ============================================================================

rs_result_t rs_base64_encode(rs_string_t *out, const void *data, rs_size_t len)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");
    RS_CHECK(data != NULL || len == 0, RS_ERR_INVALID, "Input data is NULL");

    const rs_u8 *input = (const rs_u8 *)data;
    rs_size_t encoded_len = rs_base64_encoded_size(len);

    // Clear output and reserve space
    rs_string_clear(out);
    rs_string_reserve(out, encoded_len);

    // Process input in 3-byte chunks
    rs_size_t i = 0;
    while (i + 2 < len) {
        rs_u32 triple = ((rs_u32)input[i] << 16) | ((rs_u32)input[i + 1] << 8) | input[i + 2];

        rs_string_push_char(out, base64_alphabet[(triple >> 18) & 0x3F]);
        rs_string_push_char(out, base64_alphabet[(triple >> 12) & 0x3F]);
        rs_string_push_char(out, base64_alphabet[(triple >> 6) & 0x3F]);
        rs_string_push_char(out, base64_alphabet[triple & 0x3F]);

        i += 3;
    }

    // Handle remaining 1 or 2 bytes with padding
    if (i < len) {
        rs_u32 triple = (rs_u32)input[i] << 16;
        if (i + 1 < len) {
            triple |= (rs_u32)input[i + 1] << 8;
        }

        rs_string_push_char(out, base64_alphabet[(triple >> 18) & 0x3F]);
        rs_string_push_char(out, base64_alphabet[(triple >> 12) & 0x3F]);

        if (i + 1 < len) {
            rs_string_push_char(out, base64_alphabet[(triple >> 6) & 0x3F]);
            rs_string_push_char(out, '=');
        } else {
            rs_string_push_char(out, '=');
            rs_string_push_char(out, '=');
        }
    }

    return RS_OK;
}

rs_result_t rs_base64_encode_str(rs_string_t *out, rs_string_view_t input)
{
    return rs_base64_encode(out, rs_sv_data(input), rs_sv_len(input));
}

// ============================================================================
// Decoding
// ============================================================================

rs_result_t rs_base64_decode(rs_string_t *out, rs_string_view_t input)
{
    RS_CHECK(out != NULL, RS_ERR_INVALID, "Output string is NULL");

    const char *data = rs_sv_data(input);
    rs_size_t len = rs_sv_len(input);

    // Clear output and reserve space
    rs_string_clear(out);
    rs_size_t max_decoded = rs_base64_decoded_size(len);
    rs_string_reserve(out, max_decoded);

    // Process input in 4-character chunks
    rs_u8 quad[4];
    rs_size_t quad_pos = 0;

    for (rs_size_t i = 0; i < len; i++) {
        char c = data[i];

        // Skip whitespace
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }

        // Check for padding
        if (c == '=') {
            // Padding must be at the end
            quad[quad_pos++] = 0xFF; // Mark as padding

            if (quad_pos == 4) {
                // Decode the quad
                if (quad[0] == 0xFF || quad[1] == 0xFF) {
                    return RS_ERROR_RET(RS_ERR_INVALID, "Invalid Base64: padding in wrong position");
                }

                rs_u32 triple = ((rs_u32)quad[0] << 18) | ((rs_u32)quad[1] << 12);

                rs_string_push_char(out, (char)(triple >> 16));

                if (quad[2] != 0xFF) {
                    triple |= (rs_u32)quad[2] << 6;
                    rs_string_push_char(out, (char)(triple >> 8));
                }

                quad_pos = 0;
            }
            continue;
        }

        // Decode character
        rs_u8 value = base64_decode_table[(rs_u8)c];
        if (value == 0xFF) {
            return RS_ERROR_RET(RS_ERR_INVALID, "Invalid Base64 character");
        }

        quad[quad_pos++] = value;

        // Process complete quad
        if (quad_pos == 4) {
            rs_u32 triple = ((rs_u32)quad[0] << 18) | ((rs_u32)quad[1] << 12) | ((rs_u32)quad[2] << 6) | quad[3];

            rs_string_push_char(out, (char)(triple >> 16));
            rs_string_push_char(out, (char)(triple >> 8));
            rs_string_push_char(out, (char)triple);

            quad_pos = 0;
        }
    }

    // Check for incomplete input
    if (quad_pos != 0) {
        return RS_ERROR_RET(RS_ERR_INVALID, "Invalid Base64: incomplete input");
    }

    return RS_OK;
}
