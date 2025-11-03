#pragma once

#include <rs/std/internal/api.h>

/**
 * @file detect.h
 * @brief Operating system detection utilities
 */

RS_EXTERN_C_BEGIN

/**
 * @brief Operating system families (high-level)
 */
typedef enum {
    RS_OS_UNKNOWN = 0, /**< Unknown or unsupported OS */
    RS_OS_LINUX,       /**< Linux (any distribution) */
    RS_OS_MACOS,       /**< macOS (Darwin) */
    RS_OS_WINDOWS      /**< Windows (future support) */
} rs_os_type_t;

/**
 * @brief Detect the current operating system family
 *
 * Detection strategy:
 * - macOS: Check uname -s == "Darwin"
 * - Linux: Check uname -s == "Linux"
 * - Windows: Check _WIN32 macro (compile-time)
 *
 * @return The detected OS family
 */
RS_STD_API rs_os_type_t rs_os_detect(void);

/**
 * @brief Convert OS type to string representation
 *
 * @param os The OS type to convert
 * @return String representation ("linux", "macos", "windows", "unknown")
 */
RS_STD_API const char *rs_os_to_string(rs_os_type_t os);

/**
 * @brief Detect Linux distribution name
 *
 * Parses /etc/os-release to identify the distribution.
 * Returns distribution ID (e.g., "ubuntu", "debian", "arch", "fedora").
 *
 * Only call this function if rs_os_detect() returns RS_OS_LINUX.
 *
 * @return Distribution name as string, or "unknown" if cannot be determined.
 *         The returned string is statically allocated and does not need to be freed.
 */
RS_STD_API const char *rs_os_detect_distro(void);

RS_EXTERN_C_END
