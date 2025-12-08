# ##############################################################################
# RS Package Discovery Tests
# ##############################################################################

# ==============================================================================
# Package Discovery Tests
# ==============================================================================

# List of all rs packages to test
set(RS_STD_PACKAGES
    rs_std
    # Add more packages here as they are created
)

# List of target aliases to verify for each package
# Format: "package:target_alias"
# The exported targets follow the naming pattern:
#   namespace::export_name, namespace::export_name_static (e.g., rs::std, rs::std_static)
set(RS_STD_PACKAGE_TARGETS "")
if(RS_STD_BUILD_SHARED)
  list(APPEND RS_STD_PACKAGE_TARGETS "rs_std:rs::std")
endif()
if(RS_STD_BUILD_STATIC)
  list(APPEND RS_STD_PACKAGE_TARGETS "rs_std:rs::std_static")
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring Package Discovery Tests")
message(STATUS "========================================")
message(STATUS "Packages to test: ${RS_STD_PACKAGES}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

add_test(
    NAME SetupInstall
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
)
set_tests_properties(SetupInstall PROPERTIES FIXTURES_SETUP PackagesInstalled)

# Create tests for each package using CMake script mode
foreach(pkg ${RS_STD_PACKAGES})
    # Test CMake package discovery (unified config)
    add_test(
        NAME "PackageDiscovery_${pkg}_cmake"
        COMMAND ${CMAKE_COMMAND}
            -DTEST_TYPE=cmake
            -DLIBRARY_NAME=${pkg}
            -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/test_package_discovery.cmake
    )
    set_tests_properties("PackageDiscovery_${pkg}_cmake" PROPERTIES FIXTURES_REQUIRED PackagesInstalled)

    # Test pkg-config discovery (skip on Windows - no pkg-config)
    if(NOT WIN32)
        # Test pkg-config discovery (shared)
        # Note: Search both lib and lib64 for compatibility with different systems
        if(RS_STD_BUILD_SHARED)
            add_test(
                NAME "PackageDiscovery_${pkg}_pkgconfig"
                COMMAND ${CMAKE_COMMAND}
                    -DTEST_TYPE=pkgconfig
                    -DLIBRARY_NAME=${pkg}
                    -DPKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/lib64/pkgconfig
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/test_package_discovery.cmake
            )
            set_tests_properties("PackageDiscovery_${pkg}_pkgconfig" PROPERTIES FIXTURES_REQUIRED PackagesInstalled)
        endif()

        # Test pkg-config discovery (static)
        if(RS_STD_BUILD_STATIC)
            add_test(
                NAME "PackageDiscovery_${pkg}_static_pkgconfig"
                COMMAND ${CMAKE_COMMAND}
                    -DTEST_TYPE=pkgconfig
                    -DLIBRARY_NAME=${pkg}_static
                    -DPKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/lib64/pkgconfig
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/test_package_discovery.cmake
            )
            set_tests_properties("PackageDiscovery_${pkg}_static_pkgconfig" PROPERTIES FIXTURES_REQUIRED PackagesInstalled)
        endif()
    endif()
endforeach()

# Create tests for target aliases
foreach(target_entry ${RS_STD_PACKAGE_TARGETS})
    # Parse "package:target_alias" where target_alias may contain ::
    # First, find the first colon to split package from alias
    string(FIND "${target_entry}" ":" first_colon_pos)
    if(first_colon_pos GREATER -1)
        # Extract package name (before first :)
        string(SUBSTRING "${target_entry}" 0 ${first_colon_pos} pkg_name)

        # Extract target alias (after first :)
        math(EXPR alias_start "${first_colon_pos} + 1")
        string(SUBSTRING "${target_entry}" ${alias_start} -1 target_alias)

        # Sanitize test name (replace :: with _)
        string(REPLACE "::" "_" test_name_suffix ${target_alias})

        add_test(
            NAME "TargetAlias_${test_name_suffix}"
            COMMAND ${CMAKE_COMMAND}
                -DTEST_TYPE=target
                -DTARGET_ALIAS=${target_alias}
                -DLIBRARY_NAME=${pkg_name}
                -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/test_package_discovery.cmake
        )
        set_tests_properties("TargetAlias_${test_name_suffix}" PROPERTIES FIXTURES_REQUIRED PackagesInstalled)
    endif()
endforeach()


message(STATUS "========================================")
message(STATUS "Package discovery tests configured")
message(STATUS "  RS_STD packages: ${RS_STD_PACKAGES}")
message(STATUS "  Target aliases: ${RS_STD_PACKAGE_TARGETS}")
message(STATUS "========================================")
message(STATUS "")
