#include <rs/std/allocators/allocator.h>
#include <rs/std/error.h>
#include <rs/std/fs/file.h>
#include <rs/std/os/detect.h>
#include <rs/std/string/string.h>
#include <rs/std/string/string_view.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/utsname.h>
#endif

rs_os_type_t rs_os_detect(void)
{
#ifdef _WIN32
    return RS_OS_WINDOWS;
#elif defined(__APPLE__)
    return RS_OS_MACOS;
#elif defined(__linux__)
    return RS_OS_LINUX;
#else
    struct utsname uname_data;

    if (uname(&uname_data) != 0) {
        return RS_OS_UNKNOWN;
    }

    if (strcmp(uname_data.sysname, "Darwin") == 0) {
        return RS_OS_MACOS;
    }

    if (strcmp(uname_data.sysname, "Linux") == 0) {
        return RS_OS_LINUX;
    }

    return RS_OS_UNKNOWN;
#endif
}

const char *rs_os_to_string(rs_os_type_t os)
{
    switch (os) {
    case RS_OS_LINUX:
        return "linux";
    case RS_OS_MACOS:
        return "macos";
    case RS_OS_WINDOWS:
        return "windows";
    case RS_OS_UNKNOWN:
    default:
        return "unknown";
    }
}

const char *rs_os_detect_distro(void)
{
    static char distro_name[64] = {0};
    static int detected = 0;

    if (detected) {
        return distro_name;
    }

#ifdef _WIN32
    detected = 1;
    strcpy(distro_name, "windows");
    return distro_name;
#endif

    rs_string_view_t os_release_path = rs_sv_from_cstr("/etc/os-release");

    // Check if /etc/os-release exists
    if (!rs_file_exists(os_release_path)) {
        detected = 1;
        strcpy(distro_name, "unknown");
        return distro_name;
    }

    // Read the file
    rs_allocator_t *allocator = rs_allocator_system();
    rs_string_t content = rs_string_create(.allocator = allocator);
    rs_result_t res = rs_file_read(os_release_path, &content);

    if (res != RS_OK) {
        rs_string_destroy(&content);
        detected = 1;
        strcpy(distro_name, "unknown");
        return distro_name;
    }

    // Default to unknown
    strcpy(distro_name, "unknown");

    // Get C string from rs_string_t
    const char *content_cstr = rs_string_cstr(&content);

    // Parse line by line looking for ID=
    // We need to make a copy because strtok modifies the string
    char *content_copy = strdup(content_cstr);
    char *line = strtok(content_copy, "\n");

    while (line != NULL) {
        // Trim leading whitespace
        while (*line == ' ' || *line == '\t') {
            line++;
        }

        // Check for ID= line
        if (strncmp(line, "ID=", 3) == 0) {
            const char *id = line + 3;

            // Remove quotes if present
            if (*id == '"' || *id == '\'') {
                id++;
            }

            // Copy the ID to our static buffer
            size_t i = 0;
            while (id[i] != '\0' && id[i] != '"' && id[i] != '\'' && id[i] != '\n' && i < sizeof(distro_name) - 1) {
                distro_name[i] = id[i];
                i++;
            }
            distro_name[i] = '\0';
            break;
        }

        line = strtok(NULL, "\n");
    }

    free(content_copy);
    rs_string_destroy(&content);
    detected = 1;
    return distro_name;
}
