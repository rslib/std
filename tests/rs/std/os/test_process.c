#include <rs/std/allocators/allocator.h>
#include <rs/std/error.h>
#include <rs/std/logging/logging.h>
#include <rs/std/os/process.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <string.h>
#include <unity.h>

#define RS_STD_LOG_MODULE "test_process"

static rs_allocator_t *allocator;

void setUp(void)
{
    rs_log_init();
    rs_log_set_level(RS_STD_LOG_TRACE);
    allocator = rs_allocator_system();
}

void tearDown(void)
{
    rs_log_shutdown();
}

// ============================================================================
// Command Exists Tests
// ============================================================================

void test_command_exists_common_commands(void)
{
    // Test for commands available on all platforms
    rs_string_view_t echo = rs_sv_from_cstr("echo");
    TEST_ASSERT_EQUAL(1, rs_process_command_exists(echo));

#ifdef _WIN32
    rs_string_view_t cmd_exe = rs_sv_from_cstr("cmd");
    TEST_ASSERT_EQUAL(1, rs_process_command_exists(cmd_exe));
#else
    rs_string_view_t ls = rs_sv_from_cstr("ls");
    TEST_ASSERT_EQUAL(1, rs_process_command_exists(ls));
#endif
}

void test_command_exists_nonexistent(void)
{
    rs_string_view_t cmd = rs_sv_from_cstr("nonexistent_command_xyz_12345");
    TEST_ASSERT_EQUAL(0, rs_process_command_exists(cmd));
}

// ============================================================================
// Process Exec Tests
// ============================================================================

void test_process_exec_simple(void)
{
    rs_string_view_t cmd = rs_sv_from_cstr("echo test");
    int exit_code = rs_process_exec(cmd);

    TEST_ASSERT_EQUAL(0, exit_code);
}

void test_process_exec_exit_code(void)
{
    // Command that exits with non-zero
#ifdef _WIN32
    rs_string_view_t cmd = rs_sv_from_cstr("cmd /c exit 42");
#else
    rs_string_view_t cmd = rs_sv_from_cstr("sh -c 'exit 42'");
#endif
    int exit_code = rs_process_exec(cmd);

    TEST_ASSERT_EQUAL(42, exit_code);
}

// ============================================================================
// Process Exec Capture Tests
// ============================================================================

void test_process_exec_capture_simple(void)
{
    rs_string_t output = rs_string_create(.allocator = allocator);
    int exit_code;

    rs_string_view_t cmd = rs_sv_from_cstr("echo 'hello world'");
    rs_result_t res = rs_process_exec_capture(cmd, &output, &exit_code);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(0, exit_code);
    TEST_ASSERT_TRUE(rs_string_contains(&output, "hello world"));

    rs_string_destroy(&output);
}

void test_process_exec_capture_multiline(void)
{
    rs_string_t output = rs_string_create(.allocator = allocator);
    int exit_code;

#ifdef _WIN32
    rs_string_view_t cmd = rs_sv_from_cstr("cmd /c \"echo line1 && echo line2 && echo line3\"");
#else
    rs_string_view_t cmd = rs_sv_from_cstr("printf 'line1\\nline2\\nline3'");
#endif
    rs_result_t res = rs_process_exec_capture(cmd, &output, &exit_code);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(0, exit_code);
    TEST_ASSERT_TRUE(rs_string_contains(&output, "line1"));
    TEST_ASSERT_TRUE(rs_string_contains(&output, "line2"));
    TEST_ASSERT_TRUE(rs_string_contains(&output, "line3"));

    rs_string_destroy(&output);
}

void test_process_exec_capture_exit_code(void)
{
    rs_string_t output = rs_string_create(.allocator = allocator);
    int exit_code;

#ifdef _WIN32
    rs_string_view_t cmd = rs_sv_from_cstr("cmd /c \"echo output && exit 5\"");
#else
    rs_string_view_t cmd = rs_sv_from_cstr("sh -c 'echo output && exit 5'");
#endif
    rs_result_t res = rs_process_exec_capture(cmd, &output, &exit_code);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(5, exit_code);
    TEST_ASSERT_TRUE(rs_string_contains(&output, "output"));

    rs_string_destroy(&output);
}

void test_process_exec_capture_null_output(void)
{
    rs_result_t res = rs_process_exec_capture(rs_sv_from_cstr("echo test"), NULL, NULL);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, res);
}

// ============================================================================
// Process Exec Capture Split Tests
// ============================================================================

void test_process_exec_capture_split_both_streams(void)
{
#ifdef _WIN32
    // Skip on Windows as rs_process_spawn is not implemented yet
    TEST_IGNORE_MESSAGE("rs_process_spawn not implemented on Windows");
#else
    rs_string_t stdout_out = rs_string_create(.allocator = allocator);
    rs_string_t stderr_out = rs_string_create(.allocator = allocator);
    int exit_code;

    rs_string_view_t cmd = rs_sv_from_cstr("sh -c 'echo stdout message; echo stderr message >&2'");

    rs_result_t res = rs_process_exec_capture_split(cmd, &stdout_out, &stderr_out, &exit_code);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(0, exit_code);
    TEST_ASSERT_TRUE(rs_string_contains(&stdout_out, "stdout message"));
    TEST_ASSERT_TRUE(rs_string_contains(&stderr_out, "stderr message"));

    rs_string_destroy(&stdout_out);
    rs_string_destroy(&stderr_out);
#endif
}

void test_process_exec_capture_split_only_stdout(void)
{
#ifdef _WIN32
    // Skip on Windows as rs_process_spawn is not implemented yet
    TEST_IGNORE_MESSAGE("rs_process_spawn not implemented on Windows");
#else
    rs_string_t stdout_out = rs_string_create(.allocator = allocator);
    rs_string_t stderr_out = rs_string_create(.allocator = allocator);
    int exit_code;

    rs_string_view_t cmd = rs_sv_from_cstr("echo 'only stdout'");
    rs_result_t res = rs_process_exec_capture_split(cmd, &stdout_out, &stderr_out, &exit_code);

    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(0, exit_code);
    TEST_ASSERT_TRUE(rs_string_contains(&stdout_out, "only stdout"));
    TEST_ASSERT_EQUAL(0, rs_string_len(&stderr_out));

    rs_string_destroy(&stdout_out);
    rs_string_destroy(&stderr_out);
#endif
}

void test_process_exec_capture_split_null_args(void)
{
    rs_string_t output = rs_string_create(.allocator = allocator);

    rs_result_t res = rs_process_exec_capture_split(rs_sv_from_cstr("echo test"), NULL, &output, NULL);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, res);

    res = rs_process_exec_capture_split(rs_sv_from_cstr("echo test"), &output, NULL, NULL);
    TEST_ASSERT_EQUAL(RS_ERR_INVALID, res);

    rs_string_destroy(&output);
}

// ============================================================================
// Low-Level Process Spawn Tests
// ============================================================================

void test_process_spawn_simple(void)
{
    rs_string_view_t argv[] = {rs_sv_from_cstr("echo"), rs_sv_from_cstr("hello")};

    rs_process_opts_t opts = rs_process_opts_default();
    rs_process_t *proc = rs_process_spawn(argv, 2, &opts);

    if (!proc) {
        TEST_IGNORE_MESSAGE("Process spawn not implemented on this platform");
        return;
    }

    TEST_ASSERT_NOT_NULL(proc);
    TEST_ASSERT_TRUE(rs_process_get_pid(proc) > 0);

    int exit_code;
    rs_result_t res = rs_process_wait(proc, &exit_code, 0);
    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(0, exit_code);

    rs_process_close(proc);
}

void test_process_spawn_with_capture(void)
{
    rs_string_view_t argv[] = {rs_sv_from_cstr("echo"), rs_sv_from_cstr("captured")};

    rs_process_opts_t opts = rs_process_opts_default();
    opts.capture_stdout = true;

    rs_process_t *proc = rs_process_spawn(argv, 2, &opts);

    if (!proc) {
        TEST_IGNORE_MESSAGE("Process spawn not implemented on this platform");
        return;
    }

    char buffer[256];
    rs_ssize_t bytes = rs_process_read_stdout(proc, buffer, sizeof(buffer) - 1);
    TEST_ASSERT_TRUE(bytes > 0);

    buffer[bytes] = '\0';
    TEST_ASSERT_TRUE(strstr(buffer, "captured") != NULL);

    int exit_code;
    rs_process_wait(proc, &exit_code, 0);
    TEST_ASSERT_EQUAL(0, exit_code);

    rs_process_close(proc);
}

void test_process_spawn_exit_code(void)
{
    rs_string_view_t argv[] = {rs_sv_from_cstr("sh"), rs_sv_from_cstr("-c"), rs_sv_from_cstr("exit 7")};

    rs_process_opts_t opts = rs_process_opts_default();
    rs_process_t *proc = rs_process_spawn(argv, 3, &opts);

    if (!proc) {
        TEST_IGNORE_MESSAGE("Process spawn not implemented on this platform");
        return;
    }

    int exit_code;
    rs_result_t res = rs_process_wait(proc, &exit_code, 0);
    TEST_ASSERT_EQUAL(RS_OK, res);
    TEST_ASSERT_EQUAL(7, exit_code);

    rs_process_close(proc);
}

void test_process_spawn_is_running(void)
{
    rs_string_view_t argv[] = {rs_sv_from_cstr("sleep"), rs_sv_from_cstr("0.1")};

    rs_process_opts_t opts = rs_process_opts_default();
    rs_process_t *proc = rs_process_spawn(argv, 2, &opts);

    if (!proc) {
        TEST_IGNORE_MESSAGE("Process spawn not implemented on this platform");
        return;
    }

    // Immediately after spawn, should be running
    TEST_ASSERT_EQUAL(1, rs_process_is_running(proc));

    // Wait and check again
    rs_process_wait(proc, NULL, 0);
    TEST_ASSERT_EQUAL(0, rs_process_is_running(proc));

    rs_process_close(proc);
}

void test_process_spawn_timeout(void)
{
    rs_string_view_t argv[] = {rs_sv_from_cstr("sleep"), rs_sv_from_cstr("5")};

    rs_process_opts_t opts = rs_process_opts_default();
    rs_process_t *proc = rs_process_spawn(argv, 2, &opts);

    if (!proc) {
        TEST_IGNORE_MESSAGE("Process spawn not implemented on this platform");
        return;
    }

    // Wait with timeout that should expire
    int exit_code;
    rs_result_t res = rs_process_wait(proc, &exit_code, 100);
    TEST_ASSERT_EQUAL(RS_ERR_TIMEOUT, res);

    // Still running
    TEST_ASSERT_EQUAL(1, rs_process_is_running(proc));

    // Kill and cleanup
    rs_process_kill(proc);
    rs_process_wait(proc, NULL, 0);
    rs_process_close(proc);
}

void test_process_spawn_terminate(void)
{
    rs_string_view_t argv[] = {rs_sv_from_cstr("sleep"), rs_sv_from_cstr("10")};

    rs_process_opts_t opts = rs_process_opts_default();
    rs_process_t *proc = rs_process_spawn(argv, 2, &opts);

    if (!proc) {
        TEST_IGNORE_MESSAGE("Process spawn not implemented on this platform");
        return;
    }

    TEST_ASSERT_EQUAL(1, rs_process_is_running(proc));

    rs_result_t res = rs_process_terminate(proc);
    TEST_ASSERT_EQUAL(RS_OK, res);

    // Wait briefly for termination
    rs_process_wait(proc, NULL, 1000);

    rs_process_close(proc);
}

void test_process_spawn_invalid_args(void)
{
    rs_process_opts_t opts = rs_process_opts_default();

    // NULL argv
    rs_process_t *proc = rs_process_spawn(NULL, 0, &opts);
    TEST_ASSERT_NULL(proc);

    // Zero argc
    rs_string_view_t argv[] = {rs_sv_from_cstr("echo")};
    proc = rs_process_spawn(argv, 0, &opts);
    TEST_ASSERT_NULL(proc);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();

    // Command Exists
    RUN_TEST(test_command_exists_common_commands);
    RUN_TEST(test_command_exists_nonexistent);

    // Process Exec
    RUN_TEST(test_process_exec_simple);
    RUN_TEST(test_process_exec_exit_code);

    // Process Exec Capture
    RUN_TEST(test_process_exec_capture_simple);
    RUN_TEST(test_process_exec_capture_multiline);
    RUN_TEST(test_process_exec_capture_exit_code);
    RUN_TEST(test_process_exec_capture_null_output);

    // Process Exec Capture Split
    RUN_TEST(test_process_exec_capture_split_both_streams);
    RUN_TEST(test_process_exec_capture_split_only_stdout);
    RUN_TEST(test_process_exec_capture_split_null_args);

    // Low-Level Process Spawn
    RUN_TEST(test_process_spawn_simple);
    RUN_TEST(test_process_spawn_with_capture);
    RUN_TEST(test_process_spawn_exit_code);
    RUN_TEST(test_process_spawn_is_running);
    RUN_TEST(test_process_spawn_timeout);
    RUN_TEST(test_process_spawn_terminate);
    RUN_TEST(test_process_spawn_invalid_args);

    return UNITY_END();
}
