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
  message(STATUS "Testing target alias: ${TARGET_ALIAS}")

  # Derive library name from target alias (e.g., rs::rs_std_shared -> rs_std)
  string(REGEX REPLACE "^.*::" "" TARGET_NAME "${TARGET_ALIAS}")

  # Determine variant and base library name from different naming patterns:
  # Pattern 1: rs::rs_std_shared (namespace::name_variant)
  # Pattern 2: rs_std::shared (name::variant)
  # Pattern 3: rs_std::rs_std (name::name - default/header-only)
  if(TARGET_NAME MATCHES "^(.+)_(shared|static)$")
    # Pattern 1: name_variant
    set(BASE_LIBRARY_NAME "${CMAKE_MATCH_1}")
    set(VARIANT "${CMAKE_MATCH_2}")
  elseif(TARGET_NAME MATCHES "^(shared|static)$")
    # Pattern 2: variant only, need to extract base name from alias
    set(VARIANT "${TARGET_NAME}")
    # Extract namespace part before :: (e.g., cfgmgr from cfgmgr::shared)
    string(REGEX REPLACE "::.*$" "" BASE_LIBRARY_NAME "${TARGET_ALIAS}")
  else()
    # Pattern 3: Assume it's the base target (e.g., cfgmgr::cfgmgr)
    # Check both shared and static configs, prefer shared
    set(BASE_LIBRARY_NAME "${TARGET_NAME}")
    set(VARIANT "shared")
    set(CHECK_BOTH_VARIANTS ON)
  endif()

  string(TOUPPER "${BASE_LIBRARY_NAME}" BASE_LIBRARY_NAME_UPPER)

  # For base targets (e.g., cfgmgr::cfgmgr), check unified config first
  set(CONFIG_ALIAS_FOUND -1)
  set(TARGETS_ALIAS_FOUND -1)

  if(CHECK_BOTH_VARIANTS)
    # Check unified config file first (search both lib and lib64)
    find_file(
      ${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG
      NAMES ${BASE_LIBRARY_NAME}Config.cmake
      PATHS ${CMAKE_PREFIX_PATH}
      PATH_SUFFIXES
        lib/cmake/${BASE_LIBRARY_NAME}
        lib64/cmake/${BASE_LIBRARY_NAME}
      NO_DEFAULT_PATH
    )

    if(${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG)
      file(READ ${${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG} CONFIG_CONTENT)
      string(FIND "${CONFIG_CONTENT}" "${TARGET_ALIAS}" CONFIG_ALIAS_FOUND)

      if(CONFIG_ALIAS_FOUND GREATER -1)
        message(STATUS "✅ Target alias test successful: ${TARGET_ALIAS}")
        message(
          STATUS
          "   Found in unified config file: ${${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG}"
        )
        return()
      endif()
    endif()

    # If not found in unified config, try both shared and static
    foreach(VARIANT_TO_CHECK shared static)
      find_file(
        ${BASE_LIBRARY_NAME_UPPER}_${VARIANT_TO_CHECK}_CONFIG_CHECK
        NAMES ${BASE_LIBRARY_NAME}_${VARIANT_TO_CHECK}Config.cmake
        PATHS ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES
          lib/cmake/${BASE_LIBRARY_NAME}_${VARIANT_TO_CHECK}
          lib64/cmake/${BASE_LIBRARY_NAME}_${VARIANT_TO_CHECK}
        NO_DEFAULT_PATH
      )

      if(${BASE_LIBRARY_NAME_UPPER}_${VARIANT_TO_CHECK}_CONFIG_CHECK)
        file(
          READ ${${BASE_LIBRARY_NAME_UPPER}_${VARIANT_TO_CHECK}_CONFIG_CHECK}
          CONFIG_CONTENT
        )
        string(FIND "${CONFIG_CONTENT}" "${TARGET_ALIAS}" CONFIG_ALIAS_FOUND)

        if(CONFIG_ALIAS_FOUND GREATER -1)
          message(STATUS "✅ Target alias test successful: ${TARGET_ALIAS}")
          message(
            STATUS
            "   Found in ${VARIANT_TO_CHECK} config file: ${${BASE_LIBRARY_NAME_UPPER}_${VARIANT_TO_CHECK}_CONFIG_CHECK}"
          )
          return()
        endif()
      endif()
    endforeach()

    message(
      FATAL_ERROR
      "❌ Target alias test failed: ${TARGET_ALIAS} not found in any config files"
    )
  endif()

  # Check unified config file first (aliases like cfgmgr::shared are defined there)
  find_file(
    ${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG_CHECK
    NAMES ${BASE_LIBRARY_NAME}Config.cmake
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES
      lib/cmake/${BASE_LIBRARY_NAME}
      lib64/cmake/${BASE_LIBRARY_NAME}
    NO_DEFAULT_PATH
  )

  if(${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG_CHECK)
    file(READ ${${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG_CHECK} CONFIG_CONTENT)
    string(FIND "${CONFIG_CONTENT}" "${TARGET_ALIAS}" CONFIG_ALIAS_FOUND)

    if(CONFIG_ALIAS_FOUND GREATER -1)
      message(STATUS "✅ Target alias test successful: ${TARGET_ALIAS}")
      message(
        STATUS
        "   Found in unified config file: ${${BASE_LIBRARY_NAME_UPPER}_UNIFIED_CONFIG_CHECK}"
      )
      return()
    endif()
  endif()

  # If not found in unified config, check variant-specific config
  find_file(
    ${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_CONFIG
    NAMES ${BASE_LIBRARY_NAME}_${VARIANT}Config.cmake
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES
      lib/cmake/${BASE_LIBRARY_NAME}_${VARIANT}
      lib64/cmake/${BASE_LIBRARY_NAME}_${VARIANT}
    NO_DEFAULT_PATH
  )

  if(NOT ${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_CONFIG)
    message(
      FATAL_ERROR
      "❌ Target alias test failed: "
      "${BASE_LIBRARY_NAME}_${VARIANT}Config.cmake not found"
    )
  endif()

  # Read config file to check if the target alias exists
  file(READ ${${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_CONFIG} CONFIG_CONTENT)
  string(FIND "${CONFIG_CONTENT}" "${TARGET_ALIAS}" CONFIG_ALIAS_FOUND)

  # Also check targets file
  find_file(
    ${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_TARGETS
    NAMES ${BASE_LIBRARY_NAME}_${VARIANT}Targets.cmake
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES
      lib/cmake/${BASE_LIBRARY_NAME}_${VARIANT}
      lib64/cmake/${BASE_LIBRARY_NAME}_${VARIANT}
    NO_DEFAULT_PATH
  )

  set(TARGETS_ALIAS_FOUND -1)
  if(${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_TARGETS)
    file(READ ${${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_TARGETS} TARGETS_CONTENT)
    string(FIND "${TARGETS_CONTENT}" "${TARGET_ALIAS}" TARGETS_ALIAS_FOUND)
  endif()

  if(CONFIG_ALIAS_FOUND EQUAL -1 AND TARGETS_ALIAS_FOUND EQUAL -1)
    message(
      FATAL_ERROR
      "❌ Target alias test failed: ${TARGET_ALIAS} not found in "
      "config or targets files"
    )
  endif()

  message(STATUS "✅ Target alias test successful: ${TARGET_ALIAS}")
  if(CONFIG_ALIAS_FOUND GREATER -1)
    message(
      STATUS
      "   Found in config file: ${${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_CONFIG}"
    )
  endif()
  if(TARGETS_ALIAS_FOUND GREATER -1)
    message(
      STATUS
      "   Found in targets file: ${${BASE_LIBRARY_NAME_UPPER}_${VARIANT}_TARGETS}"
    )
  endif()
else()
  message(
    FATAL_ERROR
    "Unknown TEST_TYPE: ${TEST_TYPE}. Must be one of: pkgconfig, cmake, target, dependency"
  )
endif()
