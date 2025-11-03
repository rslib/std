#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/os/detect.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_detect"

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// OS Detection Tests
// ============================================================================

void test_os_detect_returns_valid_os(void)
{
    rs_os_type_t os = rs_os_detect();

    // Should detect one of the known operating systems
    TEST_ASSERT_TRUE(os == RS_OS_LINUX || os == RS_OS_MACOS || os == RS_OS_WINDOWS);
    TEST_ASSERT_NOT_EQUAL(RS_OS_UNKNOWN, os);
}

void test_os_to_string_valid(void)
{
    const char *linux_str = rs_os_to_string(RS_OS_LINUX);
    TEST_ASSERT_EQUAL_STRING("linux", linux_str);

    const char *macos_str = rs_os_to_string(RS_OS_MACOS);
    TEST_ASSERT_EQUAL_STRING("macos", macos_str);

    const char *windows_str = rs_os_to_string(RS_OS_WINDOWS);
    TEST_ASSERT_EQUAL_STRING("windows", windows_str);

    const char *unknown_str = rs_os_to_string(RS_OS_UNKNOWN);
    TEST_ASSERT_EQUAL_STRING("unknown", unknown_str);
}

void test_os_to_string_current_os(void)
{
    rs_os_type_t os = rs_os_detect();
    const char *os_str = rs_os_to_string(os);

    TEST_ASSERT_NOT_NULL(os_str);
    TEST_ASSERT_TRUE(strlen(os_str) > 0);
    TEST_ASSERT_TRUE(strcmp("unknown", os_str) != 0);
}

void test_os_detect_distro_linux_only(void)
{
    rs_os_type_t os = rs_os_detect();

    if (os == RS_OS_LINUX) {
        const char *distro = rs_os_detect_distro();
        TEST_ASSERT_NOT_NULL(distro);
        TEST_ASSERT_TRUE(strlen(distro) > 0);

        // Call again to test caching
        const char *distro2 = rs_os_detect_distro();
        TEST_ASSERT_EQUAL_PTR(distro, distro2);
    } else {
        // On non-Linux, just verify it returns something
        const char *distro = rs_os_detect_distro();
        TEST_ASSERT_NOT_NULL(distro);
    }
}

void test_os_detect_consistency(void)
{
    // Multiple calls should return same result
    rs_os_type_t os1 = rs_os_detect();
    rs_os_type_t os2 = rs_os_detect();
    rs_os_type_t os3 = rs_os_detect();

    TEST_ASSERT_EQUAL(os1, os2);
    TEST_ASSERT_EQUAL(os2, os3);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_os_detect_returns_valid_os);
    RUN_TEST(test_os_to_string_valid);
    RUN_TEST(test_os_to_string_current_os);
    RUN_TEST(test_os_detect_distro_linux_only);
    RUN_TEST(test_os_detect_consistency);

    return UNITY_END();
}
