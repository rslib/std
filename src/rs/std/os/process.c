#include <rs/std/allocators/allocator.h>
#include <rs/std/error.h>
#include <rs/std/os/process.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/signalfd.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#endif

#endif

// Buffer size for reading command output
#define RS_PROCESS_BUFFER_SIZE 4096

// Helper function to parse command string into argv array
// Handles quotes and escapes
static rs_result_t parse_command_line(const char *cmd, rs_allocator_t *allocator, rs_string_view_t **argv_out,
                                      rs_size_t *argc_out)
{
    const char *p = cmd;
    rs_size_t argc = 0;
    rs_size_t capacity = 8;

    rs_string_view_t *argv = rs_alloc(allocator, sizeof(rs_string_view_t) * capacity);
    if (!argv) {
        return RS_ERR_NOMEM;
    }

    char *token_buf = rs_alloc(allocator, strlen(cmd) + 1);
    if (!token_buf) {
        rs_free(allocator, argv, sizeof(rs_string_view_t) * capacity);
        return RS_ERR_NOMEM;
    }

    while (*p) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }

        if (*p == '\0')
            break;

        // Parse token
        char *token = token_buf;
        int in_single_quote = 0;
        int in_double_quote = 0;

        while (*p && (in_single_quote || in_double_quote || (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r'))) {
            if (*p == '\'' && !in_double_quote) {
                in_single_quote = !in_single_quote;
                p++;
            } else if (*p == '"' && !in_single_quote) {
                in_double_quote = !in_double_quote;
                p++;
            } else if (*p == '\\' && !in_single_quote && *(p + 1)) {
                // Escape character (not in single quotes)
                p++;
                *token++ = *p++;
            } else {
                *token++ = *p++;
            }
        }
        *token = '\0';

        // Resize if needed
        if (argc >= capacity) {
            capacity *= 2;
            rs_string_view_t *new_argv = rs_alloc(allocator, sizeof(rs_string_view_t) * capacity);
            if (!new_argv) {
                rs_free(allocator, token_buf, strlen(cmd) + 1);
                rs_free(allocator, argv, sizeof(rs_string_view_t) * argc);
                return RS_ERR_NOMEM;
            }
            memcpy(new_argv, argv, sizeof(rs_string_view_t) * argc);
            rs_free(allocator, argv, sizeof(rs_string_view_t) * argc);
            argv = new_argv;
        }

        // Store token as string_view
        rs_size_t token_len = strlen(token_buf);
        char *token_copy = rs_alloc(allocator, token_len + 1);
        if (!token_copy) {
            rs_free(allocator, token_buf, strlen(cmd) + 1);
            rs_free(allocator, argv, sizeof(rs_string_view_t) * capacity);
            return RS_ERR_NOMEM;
        }
        memcpy(token_copy, token_buf, token_len + 1);

        argv[argc].data = token_copy;
        argv[argc].len = token_len;
        argc++;
    }

    rs_free(allocator, token_buf, strlen(cmd) + 1);

    *argv_out = argv;
    *argc_out = argc;
    return RS_OK;
}

static void free_parsed_argv(rs_allocator_t *allocator, rs_string_view_t *argv, rs_size_t argc)
{
    for (rs_size_t i = 0; i < argc; i++) {
        if (argv[i].data) {
            rs_free(allocator, (void *)argv[i].data, argv[i].len + 1);
        }
    }
    if (argv) {
        rs_free(allocator, argv, sizeof(rs_string_view_t) * argc);
    }
}

// =============================================================================
// High-Level API Implementation
// =============================================================================

int rs_process_command_exists(rs_string_view_t cmd)
{
    char *c_cmd = rs_sv_to_cstr(cmd);
    if (!c_cmd) {
        RS_ERROR(RS_ERR_NOMEM, "Out of memory converting command name");
        return -1;
    }

    char check_cmd[RS_PROCESS_BUFFER_SIZE];
    int ret;

#ifdef _WIN32
    // Windows: use "where" command
    ret = snprintf(check_cmd, sizeof(check_cmd), "where %s >nul 2>&1", c_cmd);
#else
    // Unix: use "command -v"
    ret = snprintf(check_cmd, sizeof(check_cmd), "command -v %s >/dev/null 2>&1", c_cmd);
#endif

    free(c_cmd);

    if (ret < 0 || ret >= (int)sizeof(check_cmd)) {
        RS_ERROR(RS_ERR_INVALID, "Command name too long");
        return -1;
    }

    // Run the command
    int exit_code = system(check_cmd);

    if (exit_code == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to execute command check");
        return -1;
    }

    // Exit code 0 means command exists
    return (exit_code == 0) ? 1 : 0;
}

int rs_process_exec(rs_string_view_t cmd)
{
    char *c_cmd = rs_sv_to_cstr(cmd);
    if (!c_cmd) {
        RS_ERROR(RS_ERR_NOMEM, "Out of memory converting command");
        return -1;
    }

    int status = system(c_cmd);
    free(c_cmd);

    if (status == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to execute command");
        return -1;
    }

// Extract exit code
#ifdef WIFEXITED
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
#endif

    return status;
}

rs_result_t rs_process_exec_capture(rs_string_view_t cmd, rs_string_t *output, int *exit_code)
{
    if (!output) {
        RS_ERROR(RS_ERR_INVALID, "Invalid arguments: output must not be NULL");
        return RS_ERR_INVALID;
    }

    char *c_cmd = rs_sv_to_cstr(cmd);
    if (!c_cmd) {
        RS_ERROR(RS_ERR_NOMEM, "Out of memory converting command");
        return RS_ERR_NOMEM;
    }

    // Open pipe to command
    FILE *fp = popen(c_cmd, "r");
    free(c_cmd);

    if (!fp) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to execute command");
        return RS_ERR_SYSTEM;
    }

    // Output string should already be initialized by caller

    // Read output and append to string
    char chunk[RS_PROCESS_BUFFER_SIZE];
    while (fgets(chunk, sizeof(chunk), fp) != NULL) {
        rs_result_t res = rs_string_push_cstr(output, chunk);
        if (res != RS_OK) {
            pclose(fp);
            RS_ERROR(RS_ERR_NOMEM, "Out of memory while reading command output");
            return RS_ERR_NOMEM;
        }
    }

    // Close pipe and get exit code
    int status = pclose(fp);
    if (status == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to close command pipe");
        return RS_ERR_SYSTEM;
    }

    // Extract exit code
    int code = 0;
#ifdef WIFEXITED
    if (WIFEXITED(status)) {
        code = WEXITSTATUS(status);
    } else {
        code = status;
    }
#else
    code = status;
#endif

    if (exit_code) {
        *exit_code = code;
    }

    return RS_OK;
}

rs_result_t rs_process_exec_capture_split(rs_string_view_t cmd, rs_string_t *stdout_out, rs_string_t *stderr_out,
                                          int *exit_code)
{
    if (!stdout_out || !stderr_out) {
        RS_ERROR(RS_ERR_INVALID, "Invalid arguments: stdout_out and stderr_out must not be NULL");
        return RS_ERR_INVALID;
    }

    // Parse command line into argv array
    char *cmd_cstr = rs_sv_to_cstr(cmd);
    if (!cmd_cstr) {
        RS_ERROR(RS_ERR_NOMEM, "Out of memory converting command");
        return RS_ERR_NOMEM;
    }

    rs_allocator_t *allocator = rs_allocator_system();
    rs_string_view_t *argv = NULL;
    rs_size_t argc = 0;

    rs_result_t res = parse_command_line(cmd_cstr, allocator, &argv, &argc);
    free(cmd_cstr);

    if (res != RS_OK) {
        RS_ERROR(RS_ERR_NOMEM, "Failed to parse command line");
        return res;
    }

    if (argc == 0) {
        free_parsed_argv(allocator, argv, argc);
        RS_ERROR(RS_ERR_INVALID, "Empty command");
        return RS_ERR_INVALID;
    }

    rs_process_opts_t opts = rs_process_opts_default();
    opts.capture_stdout = true;
    opts.capture_stderr = true;
    rs_process_t *proc = rs_process_spawn(argv, argc, &opts);
    free_parsed_argv(allocator, argv, argc);

    if (!proc) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to spawn process");
        return RS_ERR_SYSTEM;
    }

    char stdout_buf[RS_PROCESS_BUFFER_SIZE];
    char stderr_buf[RS_PROCESS_BUFFER_SIZE];

    while (1) {
        // Try to read from stdout
        rs_ssize_t stdout_bytes = rs_process_read_stdout(proc, stdout_buf, sizeof(stdout_buf) - 1);
        if (stdout_bytes > 0) {
            stdout_buf[stdout_bytes] = '\0';
            rs_result_t push_res = rs_string_push_cstr(stdout_out, stdout_buf);
            if (push_res != RS_OK) {
                rs_process_close(proc);
                RS_ERROR(RS_ERR_NOMEM, "Out of memory while reading stdout");
                return RS_ERR_NOMEM;
            }
        }

        // Try to read from stderr
        rs_ssize_t stderr_bytes = rs_process_read_stderr(proc, stderr_buf, sizeof(stderr_buf) - 1);
        if (stderr_bytes > 0) {
            stderr_buf[stderr_bytes] = '\0';
            rs_result_t push_res = rs_string_push_cstr(stderr_out, stderr_buf);
            if (push_res != RS_OK) {
                rs_process_close(proc);
                RS_ERROR(RS_ERR_NOMEM, "Out of memory while reading stderr");
                return RS_ERR_NOMEM;
            }
        }

        // Check if process has exited
        if (!rs_process_is_running(proc)) {
            // Read any remaining data
            while ((stdout_bytes = rs_process_read_stdout(proc, stdout_buf, sizeof(stdout_buf) - 1)) > 0) {
                stdout_buf[stdout_bytes] = '\0';
                rs_string_push_cstr(stdout_out, stdout_buf);
            }
            while ((stderr_bytes = rs_process_read_stderr(proc, stderr_buf, sizeof(stderr_buf) - 1)) > 0) {
                stderr_buf[stderr_bytes] = '\0';
                rs_string_push_cstr(stderr_out, stderr_buf);
            }
            break;
        }

        // Small delay to avoid busy waiting
#ifdef _WIN32
        Sleep(1); // 1ms
#else
        struct timeval tv = {.tv_sec = 0, .tv_usec = 1000}; // 1ms
        select(0, NULL, NULL, NULL, &tv);
#endif
    }

    // Wait for process to complete and get exit code
    int code = 0;
    res = rs_process_wait(proc, &code, 0);
    if (res != RS_OK) {
        rs_process_close(proc);
        return res;
    }

    if (exit_code) {
        *exit_code = code;
    }

    rs_process_close(proc);
    return RS_OK;
}

// =============================================================================
// Low-Level API Implementation
// =============================================================================

struct rs_process_t {
#ifdef _WIN32
    HANDLE process_handle;
    HANDLE stdin_handle;
    HANDLE stdout_handle;
    HANDLE stderr_handle;
    DWORD pid;
#else
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
#endif
    rs_allocator_t *allocator;
    rs_bool exited;
    int exit_code;
};

rs_process_opts_t rs_process_opts_default(void)
{
    rs_process_opts_t opts = {.working_dir = {.data = NULL, .len = 0},
                              .allocator = rs_allocator_system(),
                              .capture_stdout = false,
                              .capture_stderr = false,
                              .redirect_stdin = false,
                              .timeout_ms = 0};
    return opts;
}

// Helper function to escape and quote a string for Windows command line
#ifdef _WIN32
static char *win32_quote_arg(const char *arg)
{
    // Calculate required buffer size
    rs_size_t len = strlen(arg);
    rs_size_t extra = 2; // For surrounding quotes

    for (rs_size_t i = 0; i < len; i++) {
        if (arg[i] == '"' || arg[i] == '\\') {
            extra++;
        }
    }

    char *quoted = malloc(len + extra + 1);
    if (!quoted) {
        return NULL;
    }

    char *p = quoted;
    *p++ = '"';

    for (rs_size_t i = 0; i < len; i++) {
        if (arg[i] == '\\') {
            *p++ = '\\';
            *p++ = '\\';
        } else if (arg[i] == '"') {
            *p++ = '\\';
            *p++ = '"';
        } else {
            *p++ = arg[i];
        }
    }

    *p++ = '"';
    *p = '\0';

    return quoted;
}

static char *win32_build_cmdline(const rs_string_view_t *argv, rs_size_t argc)
{
    // Convert argv array to Windows command line string
    rs_size_t total_len = 0;
    char **quoted_args = malloc(sizeof(char *) * argc);
    if (!quoted_args) {
        return NULL;
    }

    // Quote each argument
    for (rs_size_t i = 0; i < argc; i++) {
        char *arg = rs_sv_to_cstr(argv[i]);
        if (!arg) {
            for (rs_size_t j = 0; j < i; j++) {
                free(quoted_args[j]);
            }
            free(quoted_args);
            return NULL;
        }

        quoted_args[i] = win32_quote_arg(arg);
        free(arg);

        if (!quoted_args[i]) {
            for (rs_size_t j = 0; j < i; j++) {
                free(quoted_args[j]);
            }
            free(quoted_args);
            return NULL;
        }

        total_len += strlen(quoted_args[i]) + 1; // +1 for space
    }

    // Build command line
    char *cmdline = malloc(total_len + 1);
    if (!cmdline) {
        for (rs_size_t i = 0; i < argc; i++) {
            free(quoted_args[i]);
        }
        free(quoted_args);
        return NULL;
    }

    char *p = cmdline;
    for (rs_size_t i = 0; i < argc; i++) {
        if (i > 0) {
            *p++ = ' ';
        }
        strcpy(p, quoted_args[i]);
        p += strlen(quoted_args[i]);
        free(quoted_args[i]);
    }
    *p = '\0';

    free(quoted_args);
    return cmdline;
}
#endif

rs_process_t *rs_process_spawn(const rs_string_view_t *argv, rs_size_t argc, const rs_process_opts_t *opts)
{
#ifdef _WIN32
    if (!argv || argc == 0) {
        RS_ERROR(RS_ERR_INVALID, "Invalid arguments: argv must not be NULL and argc > 0");
        return NULL;
    }

    rs_process_opts_t default_opts = rs_process_opts_default();
    if (!opts) {
        opts = &default_opts;
    }

    // Build command line from argv
    char *cmdline = win32_build_cmdline(argv, argc);
    if (!cmdline) {
        RS_ERROR(RS_ERR_NOMEM, "Out of memory building command line");
        return NULL;
    }

    // Create pipes for stdin/stdout/stderr if needed
    HANDLE stdin_read = NULL, stdin_write = NULL;
    HANDLE stdout_read = NULL, stdout_write = NULL;
    HANDLE stderr_read = NULL, stderr_write = NULL;

    SECURITY_ATTRIBUTES sa = {
        .nLength = sizeof(SECURITY_ATTRIBUTES), .bInheritHandle = TRUE, .lpSecurityDescriptor = NULL};

    if (opts->redirect_stdin) {
        if (!CreatePipe(&stdin_read, &stdin_write, &sa, 0)) {
            RS_ERROR(RS_ERR_SYSTEM, "Failed to create stdin pipe");
            free(cmdline);
            return NULL;
        }
        SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);
    }

    if (opts->capture_stdout) {
        if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
            RS_ERROR(RS_ERR_SYSTEM, "Failed to create stdout pipe");
            if (stdin_read)
                CloseHandle(stdin_read);
            if (stdin_write)
                CloseHandle(stdin_write);
            free(cmdline);
            return NULL;
        }
        SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    }

    if (opts->capture_stderr) {
        if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
            RS_ERROR(RS_ERR_SYSTEM, "Failed to create stderr pipe");
            if (stdin_read)
                CloseHandle(stdin_read);
            if (stdin_write)
                CloseHandle(stdin_write);
            if (stdout_read)
                CloseHandle(stdout_read);
            if (stdout_write)
                CloseHandle(stdout_write);
            free(cmdline);
            return NULL;
        }
        SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);
    }

    // Set up STARTUPINFO
    STARTUPINFOA si = {0};
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdin_read ? stdin_read : GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = stdout_write ? stdout_write : GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = stderr_write ? stderr_write : GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi = {0};

    // Convert working directory if specified
    char *cwd = NULL;
    if (opts->working_dir.len > 0) {
        cwd = rs_sv_to_cstr(opts->working_dir);
    }

    // Create the process
    BOOL success = CreateProcessA(NULL,    // lpApplicationName
                                  cmdline, // lpCommandLine
                                  NULL,    // lpProcessAttributes
                                  NULL,    // lpThreadAttributes
                                  TRUE,    // bInheritHandles
                                  0,       // dwCreationFlags
                                  NULL,    // lpEnvironment
                                  cwd,     // lpCurrentDirectory
                                  &si,     // lpStartupInfo
                                  &pi      // lpProcessInformation
    );

    free(cmdline);
    if (cwd)
        free(cwd);

    // Close child ends of pipes
    if (stdin_read)
        CloseHandle(stdin_read);
    if (stdout_write)
        CloseHandle(stdout_write);
    if (stderr_write)
        CloseHandle(stderr_write);

    if (!success) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to create process");
        if (stdin_write)
            CloseHandle(stdin_write);
        if (stdout_read)
            CloseHandle(stdout_read);
        if (stderr_read)
            CloseHandle(stderr_read);
        return NULL;
    }

    // Allocate process structure
    rs_process_t *proc = rs_alloc_type(opts->allocator, rs_process_t);
    if (!proc) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (stdin_write)
            CloseHandle(stdin_write);
        if (stdout_read)
            CloseHandle(stdout_read);
        if (stderr_read)
            CloseHandle(stderr_read);
        RS_ERROR(RS_ERR_NOMEM, "Out of memory allocating process handle");
        return NULL;
    }

    proc->process_handle = pi.hProcess;
    proc->stdin_handle = stdin_write;
    proc->stdout_handle = stdout_read;
    proc->stderr_handle = stderr_read;
    proc->pid = pi.dwProcessId;
    proc->allocator = opts->allocator;
    proc->exited = false;
    proc->exit_code = 0;

    // Close thread handle (we don't need it)
    CloseHandle(pi.hThread);

    return proc;
#else
    if (!argv || argc == 0) {
        RS_ERROR(RS_ERR_INVALID, "Invalid arguments: argv must not be NULL and argc > 0");
        return NULL;
    }

    rs_process_opts_t default_opts = rs_process_opts_default();
    if (!opts) {
        opts = &default_opts;
    }

    // Convert argv to null-terminated C strings
    char **c_argv = malloc(sizeof(char *) * (argc + 1));
    if (!c_argv) {
        RS_ERROR(RS_ERR_NOMEM, "Out of memory allocating argv");
        return NULL;
    }

    for (rs_size_t i = 0; i < argc; i++) {
        c_argv[i] = rs_sv_to_cstr(argv[i]);
        if (!c_argv[i]) {
            // Clean up previously allocated strings
            for (rs_size_t j = 0; j < i; j++) {
                free(c_argv[j]);
            }
            free(c_argv);
            RS_ERROR(RS_ERR_NOMEM, "Out of memory converting argv[%zu]", i);
            return NULL;
        }
    }
    c_argv[argc] = NULL;

    // Create pipes for stdin/stdout/stderr if needed
    int stdin_pipe[2] = {-1, -1};
    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};

    if (opts->redirect_stdin && pipe(stdin_pipe) == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to create stdin pipe");
        goto cleanup_argv;
    }

    if (opts->capture_stdout && pipe(stdout_pipe) == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to create stdout pipe");
        goto cleanup_pipes;
    }

    if (opts->capture_stderr && pipe(stderr_pipe) == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to create stderr pipe");
        goto cleanup_pipes;
    }

    // Fork process
    pid_t pid = fork();
    if (pid == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to fork process");
        goto cleanup_pipes;
    }

    if (pid == 0) {
        // Child process

        // Redirect stdin
        if (opts->redirect_stdin) {
            close(stdin_pipe[1]); // Close write end
            dup2(stdin_pipe[0], STDIN_FILENO);
            close(stdin_pipe[0]);
        }

        // Redirect stdout
        if (opts->capture_stdout) {
            close(stdout_pipe[0]); // Close read end
            dup2(stdout_pipe[1], STDOUT_FILENO);
            close(stdout_pipe[1]);
        }

        // Redirect stderr
        if (opts->capture_stderr) {
            close(stderr_pipe[0]); // Close read end
            dup2(stderr_pipe[1], STDERR_FILENO);
            close(stderr_pipe[1]);
        }

        // Change working directory if specified
        if (opts->working_dir.len > 0) {
            char *cwd = rs_sv_to_cstr(opts->working_dir);
            if (cwd) {
                chdir(cwd);
                free(cwd);
            }
        }

        // Execute
        execvp(c_argv[0], c_argv);

        // If exec fails, exit
        _exit(127);
    }

    // Parent process
    rs_process_t *proc = rs_alloc_type(opts->allocator, rs_process_t);
    if (!proc) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        RS_ERROR(RS_ERR_NOMEM, "Out of memory allocating process handle");
        goto cleanup_pipes;
    }

    proc->pid = pid;
    proc->allocator = opts->allocator;
    proc->exited = false;
    proc->exit_code = 0;

    // Close unused pipe ends and store file descriptors
    if (opts->redirect_stdin) {
        close(stdin_pipe[0]); // Close read end
        proc->stdin_fd = stdin_pipe[1];
    } else {
        proc->stdin_fd = -1;
    }

    if (opts->capture_stdout) {
        close(stdout_pipe[1]); // Close write end
        proc->stdout_fd = stdout_pipe[0];
    } else {
        proc->stdout_fd = -1;
    }

    if (opts->capture_stderr) {
        close(stderr_pipe[1]); // Close write end
        proc->stderr_fd = stderr_pipe[0];
    } else {
        proc->stderr_fd = -1;
    }

    // Clean up argv
    for (rs_size_t i = 0; i < argc; i++) {
        free(c_argv[i]);
    }
    free(c_argv);

    return proc;

cleanup_pipes:
    if (stdin_pipe[0] != -1)
        close(stdin_pipe[0]);
    if (stdin_pipe[1] != -1)
        close(stdin_pipe[1]);
    if (stdout_pipe[0] != -1)
        close(stdout_pipe[0]);
    if (stdout_pipe[1] != -1)
        close(stdout_pipe[1]);
    if (stderr_pipe[0] != -1)
        close(stderr_pipe[0]);
    if (stderr_pipe[1] != -1)
        close(stderr_pipe[1]);

cleanup_argv:
    for (rs_size_t i = 0; i < argc; i++) {
        if (c_argv[i])
            free(c_argv[i]);
    }
    free(c_argv);
    return NULL;
#endif // !_WIN32
}

rs_result_t rs_process_wait(rs_process_t *proc, int *exit_code, rs_u32 timeout_ms)
{
    if (!proc) {
        RS_ERROR(RS_ERR_INVALID, "Process handle is NULL");
        return RS_ERR_INVALID;
    }

    if (proc->exited) {
        if (exit_code) {
            *exit_code = proc->exit_code;
        }
        return RS_OK;
    }

#ifdef _WIN32
    DWORD wait_time = (timeout_ms == 0) ? INFINITE : timeout_ms;
    DWORD wait_result = WaitForSingleObject(proc->process_handle, wait_time);

    if (wait_result == WAIT_TIMEOUT) {
        return RS_ERR_TIMEOUT;
    } else if (wait_result == WAIT_FAILED) {
        RS_ERROR(RS_ERR_SYSTEM, "WaitForSingleObject failed");
        return RS_ERR_SYSTEM;
    }

    // Process has exited, get exit code
    DWORD code;
    if (!GetExitCodeProcess(proc->process_handle, &code)) {
        RS_ERROR(RS_ERR_SYSTEM, "GetExitCodeProcess failed");
        return RS_ERR_SYSTEM;
    }

    proc->exited = true;
    proc->exit_code = (int)code;

    if (exit_code) {
        *exit_code = proc->exit_code;
    }

    return RS_OK;
#else
    int status;
    pid_t result;

    if (timeout_ms == 0) {
        // Wait indefinitely
        result = waitpid(proc->pid, &status, 0);
    } else {
        // Platform-specific event-driven timeout implementations
        result = 0;          // Initialize result for all paths
        int use_polling = 0; // Flag to indicate if we should use polling fallback

#ifdef __linux__
        // Linux: Use signalfd() to convert SIGCHLD to a file descriptor

        // Block SIGCHLD so we can handle it via signalfd
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);

        // Create signalfd
        int sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
        if (sfd == -1) {
            // Fall back to polling if signalfd fails
            use_polling = 1;
        } else {
            // First check if process already exited
            result = waitpid(proc->pid, &status, WNOHANG);
            if (result > 0) {
                close(sfd);
                goto process_exited;
            } else if (result == -1) {
                close(sfd);
                RS_ERROR(RS_ERR_SYSTEM, "waitpid failed");
                return RS_ERR_SYSTEM;
            }

            // Wait for SIGCHLD with timeout using select()
            struct timeval tv = {.tv_sec = (time_t)(timeout_ms / 1000),
                                 .tv_usec = (suseconds_t)((timeout_ms % 1000) * 1000)};

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sfd, &readfds);

            int select_result = select(sfd + 1, &readfds, NULL, NULL, &tv);

            if (select_result > 0) {
                // SIGCHLD received, drain the signalfd
                struct signalfd_siginfo fdsi;
                ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
                (void)s; // Ignore result, we'll check with waitpid

                // Check if it's our process
                result = waitpid(proc->pid, &status, WNOHANG);
                close(sfd);

                if (result == 0) {
                    // SIGCHLD was for a different process
                    return RS_ERR_TIMEOUT;
                }
            } else if (select_result == 0) {
                // Timeout
                close(sfd);
                return RS_ERR_TIMEOUT;
            } else {
                // Error
                close(sfd);
                RS_ERROR(RS_ERR_SYSTEM, "select failed");
                return RS_ERR_SYSTEM;
            }
        }

#elif defined(__APPLE__)
        // macOS: Use kqueue() with EVFILT_PROC to monitor process exit

        int kq = kqueue();
        if (kq == -1) {
            // Fall back to polling if kqueue fails
            use_polling = 1;
        } else {
            // First check if process already exited
            result = waitpid(proc->pid, &status, WNOHANG);
            if (result > 0) {
                close(kq);
                goto process_exited;
            } else if (result == -1) {
                close(kq);
                RS_ERROR(RS_ERR_SYSTEM, "waitpid failed");
                return RS_ERR_SYSTEM;
            }

            // Set up kevent to monitor process exit
            struct kevent kev;
            EV_SET(&kev, proc->pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, NULL);

            if (kevent(kq, &kev, 1, NULL, 0, NULL) == -1) {
                close(kq);
                // Fall back to polling if kevent setup fails
                use_polling = 1;
            } else {
                // Wait for the event with timeout
                struct timespec ts = {.tv_sec = (time_t)(timeout_ms / 1000),
                                      .tv_nsec = (long)((timeout_ms % 1000) * 1000000)};

                struct kevent event;
                int nev = kevent(kq, NULL, 0, &event, 1, &ts);

                close(kq);

                if (nev > 0) {
                    // Process exited
                    result = waitpid(proc->pid, &status, WNOHANG);
                    if (result == 0) {
                        // Spurious wakeup
                        return RS_ERR_TIMEOUT;
                    }
                } else if (nev == 0) {
                    // Timeout
                    return RS_ERR_TIMEOUT;
                } else {
                    // Error
                    RS_ERROR(RS_ERR_SYSTEM, "kevent failed");
                    return RS_ERR_SYSTEM;
                }
            }
        }

#else
        // Other Unix systems - use polling
        use_polling = 1;
#endif

        // Portable fallback: Use polling with select() for sleep
        // This works on all Unix systems but uses more CPU
        if (use_polling) {
            struct timespec start, current;
            clock_gettime(CLOCK_MONOTONIC, &start);

            rs_u64 timeout_ns = (rs_u64)timeout_ms * 1000000ULL;
            rs_u64 elapsed_ns = 0;

            result = 0;

            while (elapsed_ns < timeout_ns) {
                // Non-blocking check
                result = waitpid(proc->pid, &status, WNOHANG);

                if (result > 0) {
                    // Process exited
                    break;
                } else if (result == -1) {
                    RS_ERROR(RS_ERR_SYSTEM, "waitpid failed");
                    return RS_ERR_SYSTEM;
                }

                rs_u64 remaining_ns = timeout_ns - elapsed_ns;
                rs_u64 sleep_ns = remaining_ns < 10000000ULL ? remaining_ns : 10000000ULL; // min(remaining, 10ms)

                struct timeval tv = {.tv_sec = (time_t)(sleep_ns / 1000000000ULL),
                                     .tv_usec = (suseconds_t)((sleep_ns % 1000000000ULL) / 1000ULL)};
                select(0, NULL, NULL, NULL, &tv);

                // Update elapsed time
                clock_gettime(CLOCK_MONOTONIC, &current);
                elapsed_ns =
                    (rs_u64)(current.tv_sec - start.tv_sec) * 1000000000ULL + (rs_u64)(current.tv_nsec - start.tv_nsec);
            }

            // Check one final time after timeout
            if (result == 0) {
                result = waitpid(proc->pid, &status, WNOHANG);
                if (result == 0) {
                    // Still running after timeout
                    return RS_ERR_TIMEOUT;
                }
            }
        }
    }

process_exited:

    if (result == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "waitpid failed");
        return RS_ERR_SYSTEM;
    }

    if (result > 0) {
        proc->exited = true;
        if (WIFEXITED(status)) {
            proc->exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            proc->exit_code = 128 + WTERMSIG(status);
        } else {
            proc->exit_code = -1;
        }

        if (exit_code) {
            *exit_code = proc->exit_code;
        }
    }

    return RS_OK;
#endif // _WIN32
}

int rs_process_is_running(rs_process_t *proc)
{
    if (!proc) {
        return 0;
    }

    if (proc->exited) {
        return 0;
    }

#ifdef _WIN32
    DWORD code;
    if (!GetExitCodeProcess(proc->process_handle, &code)) {
        return -1;
    }

    if (code == STILL_ACTIVE) {
        return 1;
    } else {
        proc->exited = true;
        proc->exit_code = (int)code;
        return 0;
    }
#else
    // Check if process is still running
    int status;
    pid_t result = waitpid(proc->pid, &status, WNOHANG);

    if (result == 0) {
        // Still running
        return 1;
    } else if (result > 0) {
        // Process exited
        proc->exited = true;
        if (WIFEXITED(status)) {
            proc->exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            proc->exit_code = 128 + WTERMSIG(status);
        }
        return 0;
    } else {
        // Error
        return -1;
    }
#endif
}

rs_ssize_t rs_process_read_stdout(rs_process_t *proc, void *buffer, rs_size_t size)
{
#ifdef _WIN32
    if (!proc || !proc->stdout_handle) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stdout not captured");
        return -1;
    }

    DWORD bytes_read;
    if (!ReadFile(proc->stdout_handle, buffer, (DWORD)size, &bytes_read, NULL)) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE) {
            // Pipe closed, return 0 (EOF)
            return 0;
        }
        RS_ERROR(RS_ERR_SYSTEM, "Failed to read from stdout");
        return -1;
    }

    return (rs_ssize_t)bytes_read;
#else
    if (!proc || proc->stdout_fd == -1) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stdout not captured");
        return -1;
    }

    ssize_t bytes_read = read(proc->stdout_fd, buffer, size);
    if (bytes_read == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to read from stdout");
        return -1;
    }

    return (rs_ssize_t)bytes_read;
#endif
}

rs_ssize_t rs_process_read_stderr(rs_process_t *proc, void *buffer, rs_size_t size)
{
#ifdef _WIN32
    if (!proc || !proc->stderr_handle) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stderr not captured");
        return -1;
    }

    DWORD bytes_read;
    if (!ReadFile(proc->stderr_handle, buffer, (DWORD)size, &bytes_read, NULL)) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE) {
            // Pipe closed, return 0 (EOF)
            return 0;
        }
        RS_ERROR(RS_ERR_SYSTEM, "Failed to read from stderr");
        return -1;
    }

    return (rs_ssize_t)bytes_read;
#else
    if (!proc || proc->stderr_fd == -1) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stderr not captured");
        return -1;
    }

    ssize_t bytes_read = read(proc->stderr_fd, buffer, size);
    if (bytes_read == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to read from stderr");
        return -1;
    }

    return (rs_ssize_t)bytes_read;
#endif
}

rs_ssize_t rs_process_write(rs_process_t *proc, const void *buffer, rs_size_t size)
{
#ifdef _WIN32
    if (!proc || !proc->stdin_handle) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stdin not redirected");
        return -1;
    }

    DWORD bytes_written;
    if (!WriteFile(proc->stdin_handle, buffer, (DWORD)size, &bytes_written, NULL)) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to write to stdin");
        return -1;
    }

    return (rs_ssize_t)bytes_written;
#else
    if (!proc || proc->stdin_fd == -1) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stdin not redirected");
        return -1;
    }

    ssize_t bytes_written = write(proc->stdin_fd, buffer, size);
    if (bytes_written == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to write to stdin");
        return -1;
    }

    return (rs_ssize_t)bytes_written;
#endif
}

rs_result_t rs_process_close_stdin(rs_process_t *proc)
{
#ifdef _WIN32
    if (!proc || !proc->stdin_handle) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stdin not redirected");
        return RS_ERR_INVALID;
    }

    CloseHandle(proc->stdin_handle);
    proc->stdin_handle = NULL;
    return RS_OK;
#else
    if (!proc || proc->stdin_fd == -1) {
        RS_ERROR(RS_ERR_INVALID, "Invalid process or stdin not redirected");
        return RS_ERR_INVALID;
    }

    close(proc->stdin_fd);
    proc->stdin_fd = -1;
    return RS_OK;
#endif
}

rs_result_t rs_process_terminate(rs_process_t *proc)
{
    if (!proc) {
        RS_ERROR(RS_ERR_INVALID, "Process handle is NULL");
        return RS_ERR_INVALID;
    }

    if (proc->exited) {
        return RS_OK;
    }

#ifdef _WIN32
    if (!TerminateProcess(proc->process_handle, 1)) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to terminate process");
        return RS_ERR_SYSTEM;
    }
    return RS_OK;
#else
    if (kill(proc->pid, SIGTERM) == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to send SIGTERM");
        return RS_ERR_SYSTEM;
    }

    return RS_OK;
#endif
}

rs_result_t rs_process_kill(rs_process_t *proc)
{
    if (!proc) {
        RS_ERROR(RS_ERR_INVALID, "Process handle is NULL");
        return RS_ERR_INVALID;
    }

    if (proc->exited) {
        return RS_OK;
    }

#ifdef _WIN32
    // On Windows, TerminateProcess is the equivalent of SIGKILL
    if (!TerminateProcess(proc->process_handle, 1)) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to kill process");
        return RS_ERR_SYSTEM;
    }
    return RS_OK;
#else
    if (kill(proc->pid, SIGKILL) == -1) {
        RS_ERROR(RS_ERR_SYSTEM, "Failed to send SIGKILL");
        return RS_ERR_SYSTEM;
    }

    return RS_OK;
#endif
}

int rs_process_get_pid(rs_process_t *proc)
{
    if (!proc) {
        return -1;
    }
#ifdef _WIN32
    return (int)proc->pid;
#else
    return (int)proc->pid;
#endif
}

void rs_process_close(rs_process_t *proc)
{
    if (!proc) {
        return;
    }

#ifdef _WIN32
    // Close handles
    if (proc->stdin_handle) {
        CloseHandle(proc->stdin_handle);
    }
    if (proc->stdout_handle) {
        CloseHandle(proc->stdout_handle);
    }
    if (proc->stderr_handle) {
        CloseHandle(proc->stderr_handle);
    }

    // Wait for process if still running
    if (!proc->exited) {
        WaitForSingleObject(proc->process_handle, INFINITE);
    }

    // Close process handle
    if (proc->process_handle) {
        CloseHandle(proc->process_handle);
    }

    // Free memory
    if (proc->allocator) {
        rs_free(proc->allocator, proc, sizeof(rs_process_t));
    }
#else
    // Close file descriptors
    if (proc->stdin_fd != -1) {
        close(proc->stdin_fd);
    }
    if (proc->stdout_fd != -1) {
        close(proc->stdout_fd);
    }
    if (proc->stderr_fd != -1) {
        close(proc->stderr_fd);
    }

    // Wait for process if still running
    if (!proc->exited) {
        int status;
        waitpid(proc->pid, &status, 0);
    }

    // Free memory
    if (proc->allocator) {
        rs_free(proc->allocator, proc, sizeof(rs_process_t));
    }
#endif
}
