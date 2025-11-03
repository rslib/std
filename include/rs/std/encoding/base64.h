#pragma once

#include <rs/std/internal/api.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <rs/std/types.h>

RS_EXTERN_C_BEGIN
/**
 * Base64 encoding and decoding.
 *
 * This module provides standard Base64 encoding/decoding as per RFC 4648.
 * Useful for encoding binary data in text formats (config files, JSON, etc.).
 *
 * Example:
 *   // Encode binary data
 *   const rs_u8 data[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f}; // "Hello"
 *   rs_string_t encoded = rs_string_create(allocator);
 *   rs_base64_encode(&encoded, data, sizeof(data));
 *   // encoded now contains: "SGVsbG8="
 *
 *   // Decode back to binary
 *   rs_string_t decoded = rs_string_create(allocator);
 *   rs_base64_decode(&decoded, rs_sv_from_string(&encoded));
 *   // decoded now contains: "Hello"
 */

// ============================================================================
// Encoding
// ============================================================================

/**
 * Encode binary data to Base64 string.
 *
 * Encodes the input data into a Base64 string following RFC 4648.
 * The output string will be resized to fit the encoded data.
 *
 * @param out Output string (will be cleared and filled with encoded data)
 * @param data Input binary data to encode
 * @param len Length of input data in bytes
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_base64_encode(rs_string_t *out, const void *data, rs_size_t len);

/**
 * Encode string to Base64 string.
 *
 * Convenience wrapper for encoding a string_view to Base64.
 *
 * @param out Output string (will be cleared and filled with encoded data)
 * @param input Input string to encode
 * @return RS_OK on success, error code on failure
 */
RS_STD_API rs_result_t rs_base64_encode_str(rs_string_t *out, rs_string_view_t input);

// ============================================================================
// Decoding
// ============================================================================

/**
 * Decode Base64 string to binary data.
 *
 * Decodes a Base64 string following RFC 4648.
 * The output string will be resized to fit the decoded data.
 * Whitespace in the input is ignored.
 *
 * @param out Output string (will be cleared and filled with decoded data)
 * @param input Input Base64 string to decode
 * @return RS_OK on success, error code on failure (invalid Base64)
 */
RS_STD_API rs_result_t rs_base64_decode(rs_string_t *out, rs_string_view_t input);

// ============================================================================
// Size Calculation
// ============================================================================

/**
 * Calculate encoded size for given input length.
 *
 * Returns the size of the Base64 encoded string (including padding)
 * for the given input length.
 *
 * @param input_len Length of input data in bytes
 * @return Size of encoded string in bytes (excluding null terminator)
 */
RS_STD_API rs_size_t rs_base64_encoded_size(rs_size_t input_len);

/**
 * Calculate maximum decoded size for given Base64 string length.
 *
 * Returns the maximum size of decoded data for a Base64 string.
 * The actual decoded size may be smaller due to padding.
 *
 * @param encoded_len Length of Base64 string in bytes
 * @return Maximum size of decoded data in bytes
 */
RS_STD_API rs_size_t rs_base64_decoded_size(rs_size_t encoded_len);

RS_EXTERN_C_END
