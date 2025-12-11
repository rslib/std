# ==============================================================================
# rs_std library tests
# ==============================================================================

# ------------------------------------------------------------------------------
# Error handling tests
# ------------------------------------------------------------------------------

# Test: Error context and reporting
rs_add_test(
    NAME test_error
    SOURCES test_error.c
)

# Test: Defer scope cleanup
rs_add_test(
    NAME test_defer
    SOURCES test_defer.c
)

# ------------------------------------------------------------------------------
# Logging tests
# ------------------------------------------------------------------------------

# Test: Logging system
rs_add_test(
    NAME test_logging
    SOURCES logging/test_logging.c
)

# ------------------------------------------------------------------------------
# Allocator tests
# ------------------------------------------------------------------------------

# Test: Arena allocator
rs_add_test(
    NAME test_arena
    SOURCES allocators/test_arena.c
)

# Test: Stack allocator
rs_add_test(
    NAME test_stack
    SOURCES allocators/test_stack.c
)

# ------------------------------------------------------------------------------
# String tests
# ------------------------------------------------------------------------------

# Test: rs_string_view
rs_add_test(
    NAME test_string_view
    SOURCES string/test_string_view.c
)

# Test: rs_zstring_view
rs_add_test(
    NAME test_zstring_view
    SOURCES string/test_zstring_view.c
)

# Test: rs_string with SSO
rs_add_test(
    NAME test_string
    SOURCES string/test_string.c
)

# Test: String utilities (replace, split, join)
rs_add_test(
    NAME test_string_utils
    SOURCES string/test_string_utils.c
)

# ------------------------------------------------------------------------------
# Container tests
# ------------------------------------------------------------------------------

# Test: rs_array_t
rs_add_test(
    NAME test_array
    SOURCES containers/test_array.c
)

# Test: rs_span_t and rs_slice_t
rs_add_test(
    NAME test_span_slice
    SOURCES containers/test_span_slice.c
)

# Test: Hash functions
rs_add_test(
    NAME test_hash
    SOURCES containers/test_hash.c
)

# Test: Hashmap
rs_add_test(
    NAME test_hashmap
    SOURCES containers/test_hashmap.c
)

# Test: Intrusive linked list
rs_add_test(
    NAME test_list
    SOURCES containers/test_list.c
)

# ------------------------------------------------------------------------------
# Operating System tests
# ------------------------------------------------------------------------------

# Test: Environment variables
rs_add_test(
    NAME test_env
    SOURCES os/test_env.c
)

# Test: OS detection
rs_add_test(
    NAME test_detect
    SOURCES os/test_detect.c
)

# Test: Process execution
rs_add_test(
    NAME test_process
    SOURCES os/test_process.c
)

# ------------------------------------------------------------------------------
# I/O tests
# ------------------------------------------------------------------------------

# Test: I/O reading
rs_add_test(
    NAME test_io_read
    SOURCES io/test_read.c
)

# Test: I/O writing
rs_add_test(
    NAME test_io_write
    SOURCES io/test_write.c
)

# ------------------------------------------------------------------------------
# Filesystem tests
# ------------------------------------------------------------------------------

# Test: Path utilities
rs_add_test(
    NAME test_path
    SOURCES fs/test_path.c
)

# Test: File operations
rs_add_test(
    NAME test_file
    SOURCES fs/test_file.c
)

# Test: Directory operations
rs_add_test(
    NAME test_dir
    SOURCES fs/test_dir.c
)

# ------------------------------------------------------------------------------
# Encoding tests
# ------------------------------------------------------------------------------

# Test: Base64 encoding/decoding
rs_add_test(
    NAME test_base64
    SOURCES encoding/test_base64.c
)

# ------------------------------------------------------------------------------
# Cryptography tests
# ------------------------------------------------------------------------------

# Test: Crypto encryption/decryption
rs_add_test(
    NAME test_crypto
    SOURCES crypto/test_crypto.c
)

# ------------------------------------------------------------------------------
# Database tests
# ------------------------------------------------------------------------------

# Test: SQLite helpers
rs_add_test(
    NAME test_sqlite
    SOURCES db/test_sqlite.c
)
