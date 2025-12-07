# LibraryHelpers.cmake - Utilities for creating C libraries with automatic
# shared/static variants, exports, and package configs

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

#[=[
Creates a C library with automatic shared/static variants, complete with:
- Shared library target (if BUILD_SHARED is ON)
- Static library target (if BUILD_STATIC is ON)
- Aliases with nice namespaces
- Export configuration
- CMake package config files (programmatically generated)
- pkg-config files

Usage:
  rs_create_library(
    NAME base_name                    # e.g., "rs_std"
    NAMESPACE namespace               # e.g., "rs"
    [EXPORT_NAME export_name]         # e.g., "std" -> creates rs::std and rs::std_static
    SOURCES file1.c file2.c ...       # Source files
    [PUBLIC_HEADERS file1.h ...]      # Public headers to install
    [VERSION version]                 # Defaults to PROJECT_VERSION
    [DESCRIPTION "desc"]              # Package description
    [URL "https://..."]               # Package URL

    # Dependencies
    [PUBLIC_LINK_LIBRARIES lib1 lib2 ...]     # Public dependencies (both shared and static)
    [PRIVATE_LINK_LIBRARIES lib1 lib2 ...]    # Private dependencies (both shared and static)
    [INTERFACE_LINK_LIBRARIES lib1 lib2 ...]  # Interface dependencies (for INTERFACE libraries)
    [SHARED_LINK_LIBRARIES lib1 lib2 ...]     # Public dependencies ONLY for shared variant
    [STATIC_LINK_LIBRARIES lib1 lib2 ...]     # Public dependencies ONLY for static variant

    # Build options
    [BUILD_SHARED ON|OFF]             # Build shared library (defaults to parent option)
    [BUILD_STATIC ON|OFF]             # Build static library (defaults to parent option)
    [BUILD_INTERFACE ON|OFF]          # Build as INTERFACE library only

    # Include directories
    [PUBLIC_INCLUDE_DIRS dir1 dir2 ...] # Public include directories
    [PRIVATE_INCLUDE_DIRS dir1 ...]     # Private include directories

    # Compile options
    [PUBLIC_COMPILE_OPTIONS opt1 ...]   # Public compile options
    [PRIVATE_COMPILE_OPTIONS opt1 ...]  # Private compile options

    # Compile definitions
    [PUBLIC_COMPILE_DEFINITIONS def1 ...] # Public compile definitions
    [PRIVATE_COMPILE_DEFINITIONS def1 ...] # Private compile definitions

    # Properties
    [PROPERTIES prop1 val1 prop2 val2 ...] # Additional target properties

    # Package config
    [PKG_CONFIG_REQUIRES "dep1 dep2"]           # pkg-config Requires (always needed)
    [PKG_CONFIG_LIBS_PRIVATE "-lpthread -ldl"]  # pkg-config Libs.private (for static linking)
    [PKG_CONFIG_CFLAGS_PRIVATE "flags"]         # pkg-config Cflags.private
    [STATIC_PKG_CONFIG_DEPS                     # Dependencies for static pkg-config
      "VarName:system_pc_name:cpm_pc_name"      # e.g., "Monocypher:monocypher:monocypher_static"
    ]

    # CMake config dependencies (for programmatic generation)
    [FIND_DEPENDENCIES dep1 dep2 ...]      # Dependencies to find_dependency() in config
  )

Returns (sets in parent scope):
  ${NAME}_SHARED_TARGET - Name of shared library target (if created)
  ${NAME}_STATIC_TARGET - Name of static library target (if created)
  ${NAME}_INTERFACE_TARGET - Name of interface library target (if created)
  ${NAME}_TARGETS - List of all created targets
#]=]
function(rs_create_library)
  set(options BUILD_INTERFACE)
  set(
    oneValueArgs
    NAME
    NAMESPACE
    EXPORT_NAME
    VERSION
    DESCRIPTION
    URL
    BUILD_SHARED
    BUILD_STATIC
    PKG_CONFIG_REQUIRES
    PKG_CONFIG_LIBS_PRIVATE
    PKG_CONFIG_CFLAGS_PRIVATE
  )
  set(
    multiValueArgs
    SOURCES
    PUBLIC_HEADERS
    PUBLIC_LINK_LIBRARIES
    PRIVATE_LINK_LIBRARIES
    INTERFACE_LINK_LIBRARIES
    SHARED_LINK_LIBRARIES # Specific to shared variant only
    STATIC_LINK_LIBRARIES # Specific to static variant only
    PUBLIC_INCLUDE_DIRS
    PRIVATE_INCLUDE_DIRS
    PUBLIC_COMPILE_OPTIONS
    PRIVATE_COMPILE_OPTIONS
    PUBLIC_COMPILE_DEFINITIONS
    PRIVATE_COMPILE_DEFINITIONS
    PROPERTIES
    FIND_DEPENDENCIES
    STATIC_PKG_CONFIG_DEPS
  )

  cmake_parse_arguments(
    LIB
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  # Validate required arguments
  if(NOT LIB_NAME)
    message(FATAL_ERROR "rs_create_library: NAME is required")
  endif()

  if(NOT LIB_NAMESPACE)
    set(LIB_NAMESPACE ${LIB_NAME})
  endif()

  # If not provided, defaults to NAME
  if(NOT LIB_EXPORT_NAME)
    set(LIB_EXPORT_NAME ${LIB_NAME})
  endif()

  # Set defaults
  if(NOT DEFINED LIB_VERSION)
    set(LIB_VERSION ${PROJECT_VERSION})
  endif()

  if(NOT DEFINED LIB_DESCRIPTION)
    set(LIB_DESCRIPTION "${LIB_NAME} library")
  endif()

  # Determine what to build
  if(LIB_BUILD_INTERFACE)
    set(BUILD_INTERFACE_LIB ON)
    set(BUILD_SHARED_LIB OFF)
    set(BUILD_STATIC_LIB OFF)
  else()
    if(NOT DEFINED LIB_BUILD_SHARED)
      # Use parent variable if available
      string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
      if(DEFINED ${LIB_NAME_UPPER}_BUILD_SHARED)
        set(LIB_BUILD_SHARED ${${LIB_NAME_UPPER}_BUILD_SHARED})
      else()
        set(LIB_BUILD_SHARED ON)
      endif()
    endif()

    if(NOT DEFINED LIB_BUILD_STATIC)
      string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
      if(DEFINED ${LIB_NAME_UPPER}_BUILD_STATIC)
        set(LIB_BUILD_STATIC ${${LIB_NAME_UPPER}_BUILD_STATIC})
      else()
        set(LIB_BUILD_STATIC ON)
      endif()
    endif()

    set(BUILD_SHARED_LIB ${LIB_BUILD_SHARED})
    set(BUILD_STATIC_LIB ${LIB_BUILD_STATIC})
    set(BUILD_INTERFACE_LIB OFF)
  endif()

  set(CREATED_TARGETS "")

  # Helper macro to set common properties
  macro(setup_target_common TARGET_NAME TARGET_TYPE VARIANT)
    # Link libraries
    if(LIB_PUBLIC_LINK_LIBRARIES)
      target_link_libraries(${TARGET_NAME} PUBLIC ${LIB_PUBLIC_LINK_LIBRARIES})
    endif()

    if(LIB_PRIVATE_LINK_LIBRARIES AND NOT ${TARGET_TYPE} STREQUAL "INTERFACE")
      target_link_libraries(
        ${TARGET_NAME}
        PRIVATE ${LIB_PRIVATE_LINK_LIBRARIES}
      )
    endif()

    if(LIB_INTERFACE_LINK_LIBRARIES)
      target_link_libraries(
        ${TARGET_NAME}
        INTERFACE ${LIB_INTERFACE_LINK_LIBRARIES}
      )
    endif()

    # Variant-specific libraries
    if("${VARIANT}" STREQUAL "shared" AND LIB_SHARED_LINK_LIBRARIES)
      target_link_libraries(${TARGET_NAME} PUBLIC ${LIB_SHARED_LINK_LIBRARIES})
      # Add explicit dependencies for MSVC parallel builds
      foreach(dep ${LIB_SHARED_LINK_LIBRARIES})
        if(TARGET ${dep})
          get_target_property(dep_type ${dep} TYPE)
          get_target_property(dep_aliased ${dep} ALIASED_TARGET)
          if(dep_aliased)
            add_dependencies(${TARGET_NAME} ${dep_aliased})
          elseif(NOT dep_type STREQUAL "INTERFACE_LIBRARY")
            add_dependencies(${TARGET_NAME} ${dep})
          endif()
        endif()
      endforeach()
    endif()

    if("${VARIANT}" STREQUAL "static" AND LIB_STATIC_LINK_LIBRARIES)
      target_link_libraries(${TARGET_NAME} PUBLIC ${LIB_STATIC_LINK_LIBRARIES})
      # Add explicit dependencies for MSVC parallel builds
      foreach(dep ${LIB_STATIC_LINK_LIBRARIES})
        if(TARGET ${dep})
          get_target_property(dep_type ${dep} TYPE)
          get_target_property(dep_aliased ${dep} ALIASED_TARGET)
          if(dep_aliased)
            add_dependencies(${TARGET_NAME} ${dep_aliased})
          elseif(NOT dep_type STREQUAL "INTERFACE_LIBRARY")
            add_dependencies(${TARGET_NAME} ${dep})
          endif()
        endif()
      endforeach()
    endif()

    # Include directories
    if(LIB_PUBLIC_INCLUDE_DIRS)
      target_include_directories(
        ${TARGET_NAME}
        PUBLIC ${LIB_PUBLIC_INCLUDE_DIRS}
      )
    endif()

    if(LIB_PRIVATE_INCLUDE_DIRS AND NOT ${TARGET_TYPE} STREQUAL "INTERFACE")
      target_include_directories(
        ${TARGET_NAME}
        PRIVATE ${LIB_PRIVATE_INCLUDE_DIRS}
      )
    endif()

    # Compile options
    if(LIB_PUBLIC_COMPILE_OPTIONS)
      target_compile_options(
        ${TARGET_NAME}
        PUBLIC ${LIB_PUBLIC_COMPILE_OPTIONS}
      )
    endif()

    if(LIB_PRIVATE_COMPILE_OPTIONS AND NOT ${TARGET_TYPE} STREQUAL "INTERFACE")
      target_compile_options(
        ${TARGET_NAME}
        PRIVATE ${LIB_PRIVATE_COMPILE_OPTIONS}
      )
    endif()

    # Compile definitions
    if(LIB_PUBLIC_COMPILE_DEFINITIONS)
      target_compile_definitions(
        ${TARGET_NAME}
        PUBLIC ${LIB_PUBLIC_COMPILE_DEFINITIONS}
      )
    endif()

    if(
      LIB_PRIVATE_COMPILE_DEFINITIONS
      AND NOT ${TARGET_TYPE} STREQUAL "INTERFACE"
    )
      target_compile_definitions(
        ${TARGET_NAME}
        PRIVATE ${LIB_PRIVATE_COMPILE_DEFINITIONS}
      )
    endif()

    # Additional properties
    if(LIB_PROPERTIES)
      set_target_properties(${TARGET_NAME} PROPERTIES ${LIB_PROPERTIES})
    endif()
  endmacro()

  # ============================================================================
  # Create INTERFACE library
  # ============================================================================
  if(BUILD_INTERFACE_LIB)
    set(INTERFACE_TARGET_NAME ${LIB_NAME})
    add_library(${INTERFACE_TARGET_NAME} INTERFACE)
    add_library(${LIB_NAMESPACE}::${LIB_NAME} ALIAS ${INTERFACE_TARGET_NAME})

    setup_target_common(${INTERFACE_TARGET_NAME} "INTERFACE" "interface")

    list(APPEND CREATED_TARGETS ${INTERFACE_TARGET_NAME})
    set(${LIB_NAME}_INTERFACE_TARGET ${INTERFACE_TARGET_NAME} PARENT_SCOPE)

    # Export and install
    install(
      TARGETS ${INTERFACE_TARGET_NAME}
      EXPORT ${LIB_NAME}Targets
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Create package configs
    _rs_create_library_package_configs(
      NAME
      ${LIB_NAME}
      VERSION
      ${LIB_VERSION}
      DESCRIPTION
      ${LIB_DESCRIPTION}
      URL
      ${LIB_URL}
      PKG_CONFIG_REQUIRES
      ${LIB_PKG_CONFIG_REQUIRES}
      PKG_CONFIG_REQUIRES_PRIVATE
      ${LIB_PKG_CONFIG_REQUIRES_PRIVATE}
      PKG_CONFIG_LIBS_PRIVATE
      ${LIB_PKG_CONFIG_LIBS_PRIVATE}
      PKG_CONFIG_CFLAGS_PRIVATE
      ${LIB_PKG_CONFIG_CFLAGS_PRIVATE}
      FIND_DEPENDENCIES
      ${LIB_FIND_DEPENDENCIES}
      OUTPUT_NAME
      ${LIB_NAME}
    )
  endif()

  # ============================================================================
  # Create SHARED library
  # ============================================================================
  if(BUILD_SHARED_LIB)
    set(SHARED_TARGET_NAME ${LIB_NAME}_shared)
    add_library(${SHARED_TARGET_NAME} SHARED ${LIB_SOURCES})
    add_library(
      ${LIB_NAMESPACE}::${LIB_EXPORT_NAME}
      ALIAS ${SHARED_TARGET_NAME}
    )

    # Set output name to base name (without _shared suffix)
    get_filename_component(BASE_NAME ${LIB_NAME} NAME)
    set_target_properties(
      ${SHARED_TARGET_NAME}
      PROPERTIES
        OUTPUT_NAME ${BASE_NAME}
        VERSION ${LIB_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        # Export all symbols on Windows so import library is generated
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO
          ${CMAKE_BINARY_DIR}/lib/RelWithDebInfo
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib/MinSizeRel
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release
        ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO
          ${CMAKE_BINARY_DIR}/lib/RelWithDebInfo
        ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib/MinSizeRel
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO
          ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
    )

    # Add export definition for shared library builds
    # Convert name to uppercase for the define (e.g., rs_std -> RS_STD_EXPORTS)
    string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
    string(REPLACE "-" "_" LIB_NAME_UPPER ${LIB_NAME_UPPER})
    target_compile_definitions(
      ${SHARED_TARGET_NAME}
      PRIVATE ${LIB_NAME_UPPER}_EXPORTS
    )

    setup_target_common(${SHARED_TARGET_NAME} "SHARED" "shared")

    # Add compiler warnings
    target_add_warnings(TARGET ${SHARED_TARGET_NAME})

    list(APPEND CREATED_TARGETS ${SHARED_TARGET_NAME})
    set(${LIB_NAME}_SHARED_TARGET ${SHARED_TARGET_NAME} PARENT_SCOPE)

    # Export and install (to unified export)
    install(
      TARGETS ${SHARED_TARGET_NAME}
      EXPORT ${LIB_NAME}Targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()

  # ============================================================================
  # Create STATIC library
  # ============================================================================
  if(BUILD_STATIC_LIB)
    set(STATIC_TARGET_NAME ${LIB_NAME}_static)
    add_library(${STATIC_TARGET_NAME} STATIC ${LIB_SOURCES})
    add_library(
      ${LIB_NAMESPACE}::${LIB_EXPORT_NAME}_static
      ALIAS ${STATIC_TARGET_NAME}
    )

    # Set output name to base name (without _static suffix)
    get_filename_component(BASE_NAME ${LIB_NAME} NAME)
    set_target_properties(
      ${STATIC_TARGET_NAME}
      PROPERTIES
        OUTPUT_NAME ${BASE_NAME}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release
        ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO
          ${CMAKE_BINARY_DIR}/lib/RelWithDebInfo
        ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib/MinSizeRel
    )

    # Add static definition for static library builds
    # Convert name to uppercase for the define (e.g., rs_std -> RS_STD_STATIC)
    string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
    string(REPLACE "-" "_" LIB_NAME_UPPER ${LIB_NAME_UPPER})
    target_compile_definitions(
      ${STATIC_TARGET_NAME}
      PUBLIC ${LIB_NAME_UPPER}_STATIC
    )

    setup_target_common(${STATIC_TARGET_NAME} "STATIC" "static")

    # Add compiler warnings
    target_add_warnings(TARGET ${STATIC_TARGET_NAME})

    list(APPEND CREATED_TARGETS ${STATIC_TARGET_NAME})
    set(${LIB_NAME}_STATIC_TARGET ${STATIC_TARGET_NAME} PARENT_SCOPE)

    # Export and install (to unified export)
    install(
      TARGETS ${STATIC_TARGET_NAME}
      EXPORT ${LIB_NAME}Targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endif()

  # Install public headers if specified
  if(LIB_PUBLIC_HEADERS)
    install(
      FILES ${LIB_PUBLIC_HEADERS}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
    )
  endif()

  # Create unified config that provides convenience aliases
  if(NOT BUILD_INTERFACE_LIB)
    rs_create_unified_library_config(
      NAME ${LIB_NAME}
      NAMESPACE ${LIB_NAMESPACE}
      EXPORT_NAME "${LIB_EXPORT_NAME}"
      VERSION ${LIB_VERSION}
      DESCRIPTION "${LIB_DESCRIPTION}"
      URL "${LIB_URL}"
      PKG_CONFIG_REQUIRES "${LIB_PKG_CONFIG_REQUIRES}"
      PKG_CONFIG_LIBS_PRIVATE "${LIB_PKG_CONFIG_LIBS_PRIVATE}"
      FIND_DEPENDENCIES ${LIB_FIND_DEPENDENCIES}
      BUILD_SHARED ${BUILD_SHARED_LIB}
      BUILD_STATIC ${BUILD_STATIC_LIB}
      STATIC_PKG_CONFIG_DEPS ${LIB_STATIC_PKG_CONFIG_DEPS}
    )
  endif()

  # Return list of all created targets
  set(${LIB_NAME}_TARGETS ${CREATED_TARGETS} PARENT_SCOPE)
endfunction()

#[=[
Internal function to create CMake and pkg-config package configuration files
This generates the config files programmatically without template files
#]=]
function(_rs_create_library_package_configs)
  set(options "")
  set(
    oneValueArgs
    NAME
    VERSION
    DESCRIPTION
    URL
    PKG_CONFIG_REQUIRES
    PKG_CONFIG_REQUIRES_PRIVATE
    PKG_CONFIG_LIBS_PRIVATE
    PKG_CONFIG_CFLAGS_PRIVATE
    OUTPUT_NAME
    LIBRARY_TYPE
  )
  set(multiValueArgs FIND_DEPENDENCIES)

  cmake_parse_arguments(
    PKG
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  # ============================================================================
  # Create pkg-config file
  # ============================================================================
  # Use FULL paths to avoid issues with absolute CMAKE_INSTALL_*DIR in Nix
  include(GNUInstallDirs)
  set(PC_CONTENT "")
  string(APPEND PC_CONTENT "prefix=${CMAKE_INSTALL_PREFIX}\n")
  string(APPEND PC_CONTENT "exec_prefix=\${prefix}\n")
  string(APPEND PC_CONTENT "libdir=${CMAKE_INSTALL_FULL_LIBDIR}\n")
  string(APPEND PC_CONTENT "includedir=${CMAKE_INSTALL_FULL_INCLUDEDIR}\n\n")
  string(APPEND PC_CONTENT "Name: ${PKG_NAME}\n")
  string(APPEND PC_CONTENT "Description: ${PKG_DESCRIPTION}\n")
  string(APPEND PC_CONTENT "Version: ${PKG_VERSION}\n")

  if(PKG_URL)
    string(APPEND PC_CONTENT "URL: ${PKG_URL}\n")
  endif()

  if(PKG_PKG_CONFIG_REQUIRES)
    string(APPEND PC_CONTENT "Requires: ${PKG_PKG_CONFIG_REQUIRES}\n")
  endif()

  if(PKG_PKG_CONFIG_REQUIRES_PRIVATE)
    string(
      APPEND PC_CONTENT
      "Requires.private: ${PKG_PKG_CONFIG_REQUIRES_PRIVATE}\n"
    )
  endif()

  # For static libraries, use full path to .a file to ensure static linking
  # For shared libraries, use -l flag which finds .dylib/.so first
  if(PKG_LIBRARY_TYPE STREQUAL "STATIC")
    string(APPEND PC_CONTENT "Libs: \${libdir}/lib${PKG_OUTPUT_NAME}.a\n")
  else()
    string(APPEND PC_CONTENT "Libs: -L\${libdir} -l${PKG_OUTPUT_NAME}\n")
  endif()

  if(PKG_PKG_CONFIG_LIBS_PRIVATE)
    string(APPEND PC_CONTENT "Libs.private: ${PKG_PKG_CONFIG_LIBS_PRIVATE}\n")
  endif()

  string(APPEND PC_CONTENT "Cflags: -I\${includedir}\n")

  if(PKG_PKG_CONFIG_CFLAGS_PRIVATE)
    string(
      APPEND PC_CONTENT
      "Cflags.private: ${PKG_PKG_CONFIG_CFLAGS_PRIVATE}\n"
    )
  endif()

  set(PC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}.pc")
  file(WRITE ${PC_FILE} ${PC_CONTENT})
  install(FILES ${PC_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

  # ============================================================================
  # Create CMake config file programmatically
  # ============================================================================
  set(CONFIG_CONTENT "")
  string(APPEND CONFIG_CONTENT "@PACKAGE_INIT@\n\n")
  string(APPEND CONFIG_CONTENT "include(CMakeFindDependencyMacro)\n\n")

  # Add find_dependency calls for each dependency
  if(PKG_FIND_DEPENDENCIES)
    string(APPEND CONFIG_CONTENT "# Find dependencies\n")
    foreach(dep ${PKG_FIND_DEPENDENCIES})
      string(APPEND CONFIG_CONTENT "find_dependency(${dep})\n")
    endforeach()
    string(APPEND CONFIG_CONTENT "\n")
  endif()

  # Include the targets file
  string(
    APPEND CONFIG_CONTENT
    "# Include the targets file\n"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/${PKG_NAME}Targets.cmake\")\n\n"
    "check_required_components(${PKG_NAME})\n"
  )

  # Write the config template
  set(CONFIG_TEMPLATE "${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}Config.cmake.in")
  file(WRITE ${CONFIG_TEMPLATE} ${CONFIG_CONTENT})

  # Configure the config file
  set(CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}Config.cmake")
  configure_package_config_file(
    ${CONFIG_TEMPLATE}
    ${CONFIG_FILE}
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PKG_NAME}
  )

  # Create version file
  set(VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}ConfigVersion.cmake")
  write_basic_package_version_file(
    ${VERSION_FILE}
    VERSION ${PKG_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Install config files
  install(
    FILES ${CONFIG_FILE} ${VERSION_FILE}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PKG_NAME}
  )

  # Install export targets
  install(
    EXPORT ${PKG_NAME}Targets
    FILE ${PKG_NAME}Targets.cmake
    NAMESPACE ${PKG_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PKG_NAME}
  )
endfunction()

#[=[
Creates a unified config for a library that has shared/static variants.

Usage:
  rs_create_unified_library_config(
    NAME library_name
    VERSION version
    [DESCRIPTION "desc"]
    [URL "https://..."]
    [PKG_CONFIG_REQUIRES "deps"]           # For shared (rs_std.pc)
    [PKG_CONFIG_LIBS_PRIVATE "libs"]       # For static (rs_std_static.pc)
    [FIND_DEPENDENCIES dep1 dep2]
    [BUILD_SHARED ON|OFF]
    [BUILD_STATIC ON|OFF]
    [STATIC_PKG_CONFIG_DEPS                # Dependencies for static pkg-config
      "VarName:system_pc_name:cpm_pc_name" # e.g., "Monocypher:monocypher:monocypher_static"
      ...
    ]
  )

This creates:
- Single CMake config (NAME/NAMEConfig.cmake) with all targets
- NAME.pc for shared linking
- NAME_static.pc for static linking (with correct Requires based on system vs CPM)
#]=]
function(rs_create_unified_library_config)
  set(options "")
  set(
    oneValueArgs
    NAME
    NAMESPACE
    EXPORT_NAME
    VERSION
    DESCRIPTION
    URL
    PKG_CONFIG_REQUIRES
    PKG_CONFIG_LIBS_PRIVATE
    BUILD_SHARED
    BUILD_STATIC
  )
  set(multiValueArgs FIND_DEPENDENCIES STATIC_PKG_CONFIG_DEPS)

  cmake_parse_arguments(
    CFG
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  if(NOT CFG_NAME)
    message(FATAL_ERROR "NAME is required for rs_create_unified_library_config")
  endif()

  if(NOT CFG_NAMESPACE)
    set(CFG_NAMESPACE ${CFG_NAME})
  endif()

  # If not provided, defaults to NAME
  if(NOT CFG_EXPORT_NAME)
    set(CFG_EXPORT_NAME ${CFG_NAME})
  endif()

  if(NOT CFG_VERSION)
    set(CFG_VERSION ${PROJECT_VERSION})
  endif()

  if(NOT CFG_DESCRIPTION)
    set(CFG_DESCRIPTION "${CFG_NAME} library")
  endif()

  if(NOT DEFINED CFG_BUILD_SHARED)
    set(CFG_BUILD_SHARED ON)
  endif()
  if(NOT DEFINED CFG_BUILD_STATIC)
    set(CFG_BUILD_STATIC ON)
  endif()

  # ==========================================================================
  # Create unified CMake config
  # ==========================================================================
  set(CONFIG_CONTENT "")
  string(APPEND CONFIG_CONTENT "@PACKAGE_INIT@\n\n")
  string(APPEND CONFIG_CONTENT "# ${CFG_NAME} package config\n\n")

  # Add find_dependency calls
  if(CFG_FIND_DEPENDENCIES)
    string(APPEND CONFIG_CONTENT "include(CMakeFindDependencyMacro)\n\n")
    foreach(dep ${CFG_FIND_DEPENDENCIES})
      string(APPEND CONFIG_CONTENT "find_dependency(${dep})\n")
    endforeach()
    string(APPEND CONFIG_CONTENT "\n")
  endif()

  # Include the targets file
  string(APPEND CONFIG_CONTENT "# Include targets\n")
  string(
    APPEND CONFIG_CONTENT
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/${CFG_NAME}Targets.cmake\")\n\n"
  )

  # Create convenience aliases using NAMESPACE::EXPORT_NAME pattern
  # Note: Exported targets have NAMESPACE prefix, so check for ${CFG_NAMESPACE}::${CFG_NAME}_shared
  string(APPEND CONFIG_CONTENT "# Create convenience aliases\n")
  if(CFG_BUILD_SHARED)
    string(
      APPEND CONFIG_CONTENT
      "if(TARGET ${CFG_NAMESPACE}::${CFG_NAME}_shared AND NOT TARGET ${CFG_NAMESPACE}::${CFG_EXPORT_NAME})\n"
    )
    string(
      APPEND CONFIG_CONTENT
      "  add_library(${CFG_NAMESPACE}::${CFG_EXPORT_NAME} ALIAS ${CFG_NAMESPACE}::${CFG_NAME}_shared)\n"
    )
    string(APPEND CONFIG_CONTENT "endif()\n")
  endif()
  if(CFG_BUILD_STATIC)
    string(
      APPEND CONFIG_CONTENT
      "if(TARGET ${CFG_NAMESPACE}::${CFG_NAME}_static AND NOT TARGET ${CFG_NAMESPACE}::${CFG_EXPORT_NAME}_static)\n"
    )
    string(
      APPEND CONFIG_CONTENT
      "  add_library(${CFG_NAMESPACE}::${CFG_EXPORT_NAME}_static ALIAS ${CFG_NAMESPACE}::${CFG_NAME}_static)\n"
    )
    string(APPEND CONFIG_CONTENT "endif()\n")
  endif()

  string(APPEND CONFIG_CONTENT "\ncheck_required_components(${CFG_NAME})\n")

  # Write and configure
  set(CONFIG_TEMPLATE "${CMAKE_CURRENT_BINARY_DIR}/${CFG_NAME}Config.cmake.in")
  file(WRITE ${CONFIG_TEMPLATE} ${CONFIG_CONTENT})

  set(CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CFG_NAME}Config.cmake")
  configure_package_config_file(
    ${CONFIG_TEMPLATE}
    ${CONFIG_FILE}
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CFG_NAME}
  )

  # Version file
  set(VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CFG_NAME}ConfigVersion.cmake")
  write_basic_package_version_file(
    ${VERSION_FILE}
    VERSION ${CFG_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Install config files
  install(
    FILES ${CONFIG_FILE} ${VERSION_FILE}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CFG_NAME}
  )

  # Install targets export
  install(
    EXPORT ${CFG_NAME}Targets
    FILE ${CFG_NAME}Targets.cmake
    NAMESPACE ${CFG_NAMESPACE}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CFG_NAME}
  )

  # ==========================================================================
  # Create pkg-config files
  # ==========================================================================
  include(GNUInstallDirs)

  # rs_std.pc - for shared linking (default)
  set(PC_SHARED "")
  string(APPEND PC_SHARED "prefix=${CMAKE_INSTALL_PREFIX}\n")
  string(APPEND PC_SHARED "exec_prefix=\${prefix}\n")
  string(APPEND PC_SHARED "libdir=${CMAKE_INSTALL_FULL_LIBDIR}\n")
  string(APPEND PC_SHARED "includedir=${CMAKE_INSTALL_FULL_INCLUDEDIR}\n\n")
  string(APPEND PC_SHARED "Name: ${CFG_NAME}\n")
  string(APPEND PC_SHARED "Description: ${CFG_DESCRIPTION}\n")
  string(APPEND PC_SHARED "Version: ${CFG_VERSION}\n")
  if(CFG_URL)
    string(APPEND PC_SHARED "URL: ${CFG_URL}\n")
  endif()
  if(CFG_PKG_CONFIG_REQUIRES)
    string(APPEND PC_SHARED "Requires: ${CFG_PKG_CONFIG_REQUIRES}\n")
  endif()
  string(APPEND PC_SHARED "Libs: -L\${libdir} -l${CFG_NAME}\n")
  string(APPEND PC_SHARED "Cflags: -I\${includedir}\n")

  set(PC_SHARED_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CFG_NAME}.pc")
  file(WRITE ${PC_SHARED_FILE} ${PC_SHARED})
  install(FILES ${PC_SHARED_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

  # rs_std_static.pc - for static linking
  set(PC_STATIC "")
  string(APPEND PC_STATIC "prefix=${CMAKE_INSTALL_PREFIX}\n")
  string(APPEND PC_STATIC "exec_prefix=\${prefix}\n")
  string(APPEND PC_STATIC "libdir=${CMAKE_INSTALL_FULL_LIBDIR}\n")
  string(APPEND PC_STATIC "includedir=${CMAKE_INSTALL_FULL_INCLUDEDIR}\n\n")
  string(APPEND PC_STATIC "Name: ${CFG_NAME}_static\n")
  string(APPEND PC_STATIC "Description: ${CFG_DESCRIPTION} (static)\n")
  string(APPEND PC_STATIC "Version: ${CFG_VERSION}\n")
  if(CFG_URL)
    string(APPEND PC_STATIC "URL: ${CFG_URL}\n")
  endif()

  # Build Requires line based on STATIC_PKG_CONFIG_DEPS
  # Each entry is "VarName:system_pc_name:cpm_pc_name"
  # We check ${VarName}_FROM_SYSTEM to decide which pc name to use
  if(CFG_STATIC_PKG_CONFIG_DEPS)
    set(_PC_REQUIRES "")
    foreach(_dep ${CFG_STATIC_PKG_CONFIG_DEPS})
      # Parse the colon-separated string
      string(REPLACE ":" ";" _dep_parts "${_dep}")
      list(LENGTH _dep_parts _dep_parts_len)
      if(_dep_parts_len EQUAL 3)
        list(GET _dep_parts 0 _var_name)
        list(GET _dep_parts 1 _system_pc)
        list(GET _dep_parts 2 _cpm_pc)
        if(${_var_name}_FROM_SYSTEM)
          string(APPEND _PC_REQUIRES "${_system_pc} ")
        else()
          string(APPEND _PC_REQUIRES "${_cpm_pc} ")
        endif()
      endif()
    endforeach()
    string(STRIP "${_PC_REQUIRES}" _PC_REQUIRES)
    if(_PC_REQUIRES)
      string(APPEND PC_STATIC "Requires: ${_PC_REQUIRES}\n")
    endif()
  endif()

  string(APPEND PC_STATIC "Libs: \${libdir}/lib${CFG_NAME}.a\n")
  if(CFG_PKG_CONFIG_LIBS_PRIVATE AND NOT WIN32)
    string(APPEND PC_STATIC "Libs.private: ${CFG_PKG_CONFIG_LIBS_PRIVATE}\n")
  endif()
  string(APPEND PC_STATIC "Cflags: -I\${includedir}\n")

  set(PC_STATIC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CFG_NAME}_static.pc")
  file(WRITE ${PC_STATIC_FILE} ${PC_STATIC})
  install(FILES ${PC_STATIC_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endfunction()
