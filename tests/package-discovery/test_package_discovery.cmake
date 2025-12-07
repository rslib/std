# Package Discovery Test Script
# This script tests package discovery without compilation
# Usage: cmake -DTEST_TYPE=<type> -DLIBRARY_NAME=<name> -DCMAKE_PREFIX_PATH=<path> -P test_package_discovery.cmake

# Default library name to rs_std if not specified
if(NOT LIBRARY_NAME)
  set(LIBRARY_NAME "rs_std")
endif()

# Convert library name to uppercase for variable names
string(TOUPPER "${LIBRARY_NAME}" LIBRARY_NAME_UPPER)
string(REPLACE "-" "_" LIBRARY_NAME_UPPER "${LIBRARY_NAME_UPPER}")

# Test different discovery methods based on TEST_TYPE
if(TEST_TYPE STREQUAL "pkgconfig")
  # Test pkg-config discovery
  message(STATUS "Testing pkg-config discovery for ${LIBRARY_NAME}...")

  # Prepend our path to existing PKG_CONFIG_PATH to find both our .pc files
  # and system dependency .pc files (important for Nix builds)
  if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
    set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH}")
  else()
    set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}")
  endif()

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(${LIBRARY_NAME_UPPER} ${LIBRARY_NAME})

  if(NOT ${LIBRARY_NAME_UPPER}_FOUND)
    message(
      FATAL_ERROR
      "pkg-config discovery failed: ${LIBRARY_NAME} not found"
    )
  endif()

  message(STATUS "pkg-config discovery successful for ${LIBRARY_NAME}")
  message(STATUS "   Version: ${${LIBRARY_NAME_UPPER}_VERSION}")
elseif(TEST_TYPE STREQUAL "cmake")
  # Test CMake find_package discovery
  message(STATUS "Testing CMake find_package discovery for ${LIBRARY_NAME}...")

  # Check if unified config file exists (search both lib and lib64)
  find_file(
    ${LIBRARY_NAME_UPPER}_CONFIG
    NAMES ${LIBRARY_NAME}Config.cmake
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib/cmake/${LIBRARY_NAME} lib64/cmake/${LIBRARY_NAME}
    NO_DEFAULT_PATH
  )

  if(NOT ${LIBRARY_NAME_UPPER}_CONFIG)
    message(
      FATAL_ERROR
      "CMake discovery failed: ${LIBRARY_NAME}Config.cmake not found in ${CMAKE_PREFIX_PATH}/{lib,lib64}/cmake/${LIBRARY_NAME}"
    )
  endif()

  message(STATUS "CMake find_package discovery successful for ${LIBRARY_NAME}")
  message(STATUS "   Config: ${${LIBRARY_NAME_UPPER}_CONFIG}")
elseif(TEST_TYPE STREQUAL "dependency")
  # Test dependency discovery (dependencies may not follow _shared/_static naming)
  message(STATUS "Testing dependency discovery for ${LIBRARY_NAME}...")

  # Check if dependency config file exists (look for standard CMake config)
  find_file(
    ${LIBRARY_NAME_UPPER}_DEP_CONFIG
    NAMES ${LIBRARY_NAME}Config.cmake ${LIBRARY_NAME}-config.cmake
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES
      lib/cmake/${LIBRARY_NAME}
      lib64/cmake/${LIBRARY_NAME}
      lib/cmake
      lib64/cmake
      share/cmake/${LIBRARY_NAME}
    NO_DEFAULT_PATH
  )

  if(NOT ${LIBRARY_NAME_UPPER}_DEP_CONFIG)
    message(
      FATAL_ERROR
      "❌ Dependency discovery failed: ${LIBRARY_NAME}Config.cmake not found in ${CMAKE_PREFIX_PATH}"
    )
  endif()

  message(STATUS "✅ Dependency discovery successful for ${LIBRARY_NAME}")
  message(STATUS "   Config file: ${${LIBRARY_NAME_UPPER}_DEP_CONFIG}")
elseif(TEST_TYPE STREQUAL "target")
  # Test specific target alias by checking config files
  # LIBRARY_NAME must be provided to specify which library's config to check
  message(STATUS "Testing target alias: ${TARGET_ALIAS}")

  if(NOT LIBRARY_NAME)
    message(FATAL_ERROR "LIBRARY_NAME is required for target alias tests")
  endif()

  string(TOUPPER "${LIBRARY_NAME}" LIBRARY_NAME_UPPER)

  # Check unified config file (e.g., rs_stdConfig.cmake)
  find_file(
    ${LIBRARY_NAME_UPPER}_CONFIG
    NAMES ${LIBRARY_NAME}Config.cmake
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib/cmake/${LIBRARY_NAME} lib64/cmake/${LIBRARY_NAME}
    NO_DEFAULT_PATH
  )

  if(NOT ${LIBRARY_NAME_UPPER}_CONFIG)
    message(
      FATAL_ERROR
      "❌ Target alias test failed: ${LIBRARY_NAME}Config.cmake not found"
    )
  endif()

  # Read config file and check if the target alias is defined
  file(READ ${${LIBRARY_NAME_UPPER}_CONFIG} CONFIG_CONTENT)
  string(FIND "${CONFIG_CONTENT}" "${TARGET_ALIAS}" CONFIG_ALIAS_FOUND)

  if(CONFIG_ALIAS_FOUND EQUAL -1)
    message(
      FATAL_ERROR
      "❌ Target alias test failed: ${TARGET_ALIAS} not found in "
      "${LIBRARY_NAME}Config.cmake"
    )
  endif()

  message(STATUS "✅ Target alias test successful: ${TARGET_ALIAS}")
  message(STATUS "   Found in config file: ${${LIBRARY_NAME_UPPER}_CONFIG}")
else()
  message(
    FATAL_ERROR
    "Unknown TEST_TYPE: ${TEST_TYPE}. Must be one of: pkgconfig, cmake, target, dependency"
  )
endif()
