# DependencyHelpers.cmake - Utilities for managing external dependencies with CPM

#[=[
Generic function to add a library built from source files with automatic shared/static variants

Usage:
  rs_add_source_library(
    NAME library_name
    [GITHUB_REPOSITORY owner/repo]
    [URL url]
    VERSION x.y.z
    [GIT_TAG tag]
    SOURCE_FILES file1.c file2.c ...
    [HEADER_FILES file1.h file2.h ...]
    [HEADER_SUBDIR subdir]                    # Subdirectory containing headers
    [OUTPUT_NAME name]                        # Output library name (defaults to lowercase NAME)
    [PUBLIC_COMPILE_DEFINITIONS def1 def2 ...]
    [PRIVATE_LINK_LIBRARIES lib1 lib2 ...]
    [BUILD_SHARED ON|OFF]                     # Defaults to RS_STD_BUILD_SHARED
    [BUILD_STATIC ON|OFF]                     # Defaults to RS_STD_BUILD_STATIC
    [NAMESPACE ns]                            # Defaults to NAME
    [SOVERSION num]                           # Defaults to PROJECT_VERSION_MAJOR
    [FIND_PACKAGE_NAME pkg]                   # Try find_package(pkg) first before CPM
    [FIND_PACKAGE_TARGETS target1 ...]        # Targets to check from find_package
  )

Returns (sets in parent scope):
  ${NAME}_TARGETS - List of created targets
  ${NAME}_FROM_SYSTEM - TRUE if found via find_package
#]=]
function(rs_add_source_library)
  set(options "")
  set(
    oneValueArgs
    NAME
    GITHUB_REPOSITORY
    VERSION
    GIT_TAG
    URL
    SOURCE_SUBDIR
    HEADER_SUBDIR
    OUTPUT_NAME
    BUILD_SHARED
    BUILD_STATIC
    NAMESPACE
    SOVERSION
    FIND_PACKAGE_NAME
  )
  set(
    multiValueArgs
    SOURCE_FILES
    HEADER_FILES
    PUBLIC_COMPILE_DEFINITIONS
    PRIVATE_LINK_LIBRARIES
    FIND_PACKAGE_TARGETS
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
    message(FATAL_ERROR "rs_add_cpm_library: NAME is required")
  endif()

  # Check if already processed (cache first call only)
  string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
  # if(DEFINED RS_SOURCE_LIB_${LIB_NAME_UPPER}_PROCESSED)
  #   # Already processed, return the cached targets
  #   set(${LIB_NAME_UPPER}_TARGETS ${RS_SOURCE_LIB_${LIB_NAME_UPPER}_TARGETS} PARENT_SCOPE)
  #   return()
  # endif()

  if(NOT LIB_SOURCE_FILES)
    message(FATAL_ERROR "rs_add_cpm_library: SOURCE_FILES is required")
  endif()

  # Set defaults
  if(NOT DEFINED LIB_BUILD_SHARED)
    set(LIB_BUILD_SHARED ${RS_STD_BUILD_SHARED})
  endif()

  if(NOT DEFINED LIB_BUILD_STATIC)
    set(LIB_BUILD_STATIC ${RS_STD_BUILD_STATIC})
  endif()

  if(NOT LIB_NAMESPACE)
    set(LIB_NAMESPACE ${LIB_NAME})
  endif()

  if(NOT DEFINED LIB_SOVERSION)
    set(LIB_SOVERSION ${PROJECT_VERSION_MAJOR})
  endif()

  # Default OUTPUT_NAME to lowercase NAME if not provided
  if(NOT LIB_OUTPUT_NAME)
    string(TOLOWER ${LIB_NAME} LIB_OUTPUT_NAME)
  endif()

  # Try find_package first if FIND_PACKAGE_NAME is specified
  set(FOUND_SYSTEM_PACKAGE FALSE)
  if(LIB_FIND_PACKAGE_NAME)
    find_package(${LIB_FIND_PACKAGE_NAME} QUIET)
    if(${LIB_FIND_PACKAGE_NAME}_FOUND)
      message(
        STATUS
        "Found system ${LIB_NAME} via find_package(${LIB_FIND_PACKAGE_NAME})"
      )
      set(FOUND_SYSTEM_PACKAGE TRUE)

      # Create aliases to match our naming convention if targets exist
      if(LIB_FIND_PACKAGE_TARGETS)
        foreach(target ${LIB_FIND_PACKAGE_TARGETS})
          if(TARGET ${target})
            # Create static alias
            if(NOT TARGET ${LIB_NAMESPACE}::${LIB_NAME}_static)
              add_library(${LIB_NAMESPACE}::${LIB_NAME}_static ALIAS ${target})
            endif()
            # Create default alias
            if(NOT TARGET ${LIB_NAMESPACE}::${LIB_NAME})
              add_library(${LIB_NAMESPACE}::${LIB_NAME} ALIAS ${target})
            endif()
            break()
          endif()
        endforeach()
      endif()

      set(${LIB_NAME}_FROM_SYSTEM TRUE PARENT_SCOPE)
      set(${LIB_NAME_UPPER}_TARGETS "" PARENT_SCOPE)
      return()
    endif()
  endif()

  # Download the library via CPM if not already added and not found via find_package
  string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
  if(NOT ${LIB_NAME}_ADDED)
    if(LIB_GITHUB_REPOSITORY)
      cpmaddpackage(
        NAME ${LIB_NAME}
        GITHUB_REPOSITORY ${LIB_GITHUB_REPOSITORY}
        VERSION ${LIB_VERSION}
        GIT_TAG ${LIB_GIT_TAG}
        DOWNLOAD_ONLY YES
      )
    elseif(LIB_URL)
      cpmaddpackage(
        NAME ${LIB_NAME}
        URL ${LIB_URL}
        VERSION ${LIB_VERSION}
        DOWNLOAD_ONLY YES
      )
    else()
      message(
        FATAL_ERROR
        "rs_add_cpm_library: Either GITHUB_REPOSITORY or URL is required"
      )
    endif()
  endif()

  set(${LIB_NAME}_FROM_SYSTEM FALSE PARENT_SCOPE)

  set(LIB_TARGETS)

  # Determine base source directory (accounting for SOURCE_SUBDIR)
  if(LIB_SOURCE_SUBDIR)
    set(BASE_SOURCE_DIR ${${LIB_NAME}_SOURCE_DIR}/${LIB_SOURCE_SUBDIR})
  else()
    set(BASE_SOURCE_DIR ${${LIB_NAME}_SOURCE_DIR})
  endif()

  # Determine header directory
  if(LIB_HEADER_SUBDIR)
    set(HEADER_DIR ${BASE_SOURCE_DIR}/${LIB_HEADER_SUBDIR})
  else()
    set(HEADER_DIR ${BASE_SOURCE_DIR})
  endif()

  # Prepend source directory to source files
  set(FULL_SOURCE_FILES)
  foreach(src ${LIB_SOURCE_FILES})
    list(APPEND FULL_SOURCE_FILES ${BASE_SOURCE_DIR}/${src})
  endforeach()

  # Prepend header directory to header files
  set(FULL_HEADER_FILES)
  if(LIB_HEADER_FILES)
    foreach(hdr ${LIB_HEADER_FILES})
      list(APPEND FULL_HEADER_FILES ${HEADER_DIR}/${hdr})
    endforeach()
  endif()

  # Build static library
  if(LIB_BUILD_STATIC)
    set(STATIC_TARGET ${LIB_NAME}_static)
    add_library(${STATIC_TARGET} STATIC ${FULL_SOURCE_FILES})

    target_include_directories(
      ${STATIC_TARGET}
      PUBLIC
        $<BUILD_INTERFACE:${HEADER_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    if(LIB_PUBLIC_COMPILE_DEFINITIONS)
      target_compile_definitions(
        ${STATIC_TARGET}
        PUBLIC ${LIB_PUBLIC_COMPILE_DEFINITIONS}
      )
    endif()

    if(LIB_PRIVATE_LINK_LIBRARIES)
      target_link_libraries(
        ${STATIC_TARGET}
        PRIVATE ${LIB_PRIVATE_LINK_LIBRARIES}
      )
    endif()

    set_target_properties(
      ${STATIC_TARGET}
      PROPERTIES
        OUTPUT_NAME ${LIB_OUTPUT_NAME}
        VERSION ${LIB_VERSION}
        SOVERSION ${LIB_SOVERSION}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )

    add_library(${LIB_NAMESPACE}::${LIB_NAME}_static ALIAS ${STATIC_TARGET})
    list(APPEND LIB_TARGETS ${STATIC_TARGET})
    message(STATUS "Added ${LIB_NAME} static library")
  endif()

  # Build shared library
  if(LIB_BUILD_SHARED)
    set(SHARED_TARGET ${LIB_NAME}_shared)
    add_library(${SHARED_TARGET} SHARED ${FULL_SOURCE_FILES})

    target_include_directories(
      ${SHARED_TARGET}
      PUBLIC
        $<BUILD_INTERFACE:${HEADER_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    if(LIB_PUBLIC_COMPILE_DEFINITIONS)
      target_compile_definitions(
        ${SHARED_TARGET}
        PUBLIC ${LIB_PUBLIC_COMPILE_DEFINITIONS}
      )
    endif()

    if(LIB_PRIVATE_LINK_LIBRARIES)
      target_link_libraries(
        ${SHARED_TARGET}
        PRIVATE ${LIB_PRIVATE_LINK_LIBRARIES}
      )
    endif()

    set_target_properties(
      ${SHARED_TARGET}
      PROPERTIES
        OUTPUT_NAME ${LIB_OUTPUT_NAME}
        VERSION ${LIB_VERSION}
        SOVERSION ${LIB_SOVERSION}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )

    add_library(${LIB_NAMESPACE}::${LIB_NAME} ALIAS ${SHARED_TARGET})
    list(APPEND LIB_TARGETS ${SHARED_TARGET})
    message(STATUS "Added ${LIB_NAME} shared library")
  elseif(LIB_BUILD_STATIC)
    # If only static is built, make it the default alias
    add_library(${LIB_NAMESPACE}::${LIB_NAME} ALIAS ${STATIC_TARGET})
  endif()

  # Install headers
  if(FULL_HEADER_FILES)
    install(FILES ${FULL_HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  endif()

  # Install targets
  if(LIB_TARGETS)
    install(
      TARGETS ${LIB_TARGETS}
      EXPORT ${LIB_NAME}Targets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(
      EXPORT ${LIB_NAME}Targets
      FILE ${LIB_NAME}Targets.cmake
      NAMESPACE ${LIB_NAMESPACE}::
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}
    )
  endif()

  # Set targets in parent scope
  set(${LIB_NAME_UPPER}_TARGETS ${LIB_TARGETS} PARENT_SCOPE)

  # Mark as processed and cache the targets
  # set(RS_SOURCE_LIB_${LIB_NAME_UPPER}_PROCESSED TRUE CACHE INTERNAL "")
  # set(RS_SOURCE_LIB_${LIB_NAME_UPPER}_TARGETS ${LIB_TARGETS} CACHE INTERNAL "")
endfunction()

#[=[
Add a library that has its own CMakeLists.txt (direct CPM integration)

Usage:
  rs_add_cmake_library(
    NAME library_name
    [GITHUB_REPOSITORY owner/repo]
    [URL url]
    VERSION x.y.z
    [GIT_TAG tag]
    [OPTIONS opt1=val1 opt2=val2 ...]   # CMake options to pass to the library
    [TARGETS target1 target2 ...]        # Targets provided by the library
    [NAMESPACE ns]                       # Namespace for aliases (optional)
    [FIND_PACKAGE_NAME pkg]              # Try find_package(pkg) first before CPM
    [FIND_PACKAGE_TARGETS target1 ...]   # Targets to check from find_package
  )

This function uses the library's own build system and creates aliases if needed.
Returns (sets in parent scope):
  ${NAME}_FROM_SYSTEM - TRUE if found via find_package
#]=]
function(rs_add_cmake_library)
  set(options "")
  set(
    oneValueArgs
    NAME
    GITHUB_REPOSITORY
    VERSION
    GIT_TAG
    URL
    NAMESPACE
    FIND_PACKAGE_NAME
  )
  set(multiValueArgs OPTIONS TARGETS FIND_PACKAGE_TARGETS)

  cmake_parse_arguments(
    LIB
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  # Validate required arguments
  if(NOT LIB_NAME)
    message(FATAL_ERROR "rs_add_cmake_library: NAME is required")
  endif()

  # Check if already processed (cache first call only)
  string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)
  # if(DEFINED RS_CMAKE_LIB_${LIB_NAME_UPPER}_PROCESSED)
  #   # Already processed, return cached values
  #   set(${LIB_NAME}_ADDED ${RS_CMAKE_LIB_${LIB_NAME_UPPER}_ADDED} PARENT_SCOPE)
  #   set(${LIB_NAME}_SOURCE_DIR ${RS_CMAKE_LIB_${LIB_NAME_UPPER}_SOURCE_DIR} PARENT_SCOPE)
  #   return()
  # endif()

  # Set namespace default
  if(NOT LIB_NAMESPACE)
    set(LIB_NAMESPACE ${LIB_NAME})
  endif()

  # Try find_package first if FIND_PACKAGE_NAME is specified
  if(LIB_FIND_PACKAGE_NAME)
    find_package(${LIB_FIND_PACKAGE_NAME} QUIET)
    if(${LIB_FIND_PACKAGE_NAME}_FOUND)
      message(
        STATUS
        "Found system ${LIB_NAME} via find_package(${LIB_FIND_PACKAGE_NAME})"
      )

      # Create aliases to match our naming convention if targets exist
      if(LIB_FIND_PACKAGE_TARGETS)
        foreach(target ${LIB_FIND_PACKAGE_TARGETS})
          if(TARGET ${target})
            # Create static alias
            if(NOT TARGET ${LIB_NAMESPACE}::${LIB_NAME}_static)
              add_library(${LIB_NAMESPACE}::${LIB_NAME}_static ALIAS ${target})
            endif()
            # Create default alias
            if(NOT TARGET ${LIB_NAMESPACE}::${LIB_NAME})
              add_library(${LIB_NAMESPACE}::${LIB_NAME} ALIAS ${target})
            endif()
            break()
          endif()
        endforeach()
      endif()

      set(${LIB_NAME}_FROM_SYSTEM TRUE PARENT_SCOPE)
      set(${LIB_NAME}_ADDED TRUE PARENT_SCOPE)
      return()
    endif()
  endif()

  # Set up options as a list
  set(CPM_OPTIONS "")
  if(LIB_OPTIONS)
    foreach(opt ${LIB_OPTIONS})
      list(APPEND CPM_OPTIONS "OPTIONS" ${opt})
    endforeach()
  endif()

  # Download and build the library via CPM
  if(NOT ${LIB_NAME}_ADDED)
    if(LIB_GITHUB_REPOSITORY)
      cpmaddpackage(
        NAME ${LIB_NAME}
        GITHUB_REPOSITORY ${LIB_GITHUB_REPOSITORY}
        VERSION ${LIB_VERSION}
        GIT_TAG ${LIB_GIT_TAG}
        ${CPM_OPTIONS}
      )
    elseif(LIB_URL)
      cpmaddpackage(
        NAME ${LIB_NAME}
        URL ${LIB_URL}
        VERSION ${LIB_VERSION}
        ${CPM_OPTIONS}
      )
    else()
      message(
        FATAL_ERROR
        "rs_add_cmake_library: Either GITHUB_REPOSITORY or URL is required"
      )
    endif()
  endif()

  set(${LIB_NAME}_FROM_SYSTEM FALSE PARENT_SCOPE)

  # Create namespace aliases if requested and targets are specified
  if(LIB_NAMESPACE AND LIB_TARGETS)
    foreach(target ${LIB_TARGETS})
      if(TARGET ${target})
        if(NOT TARGET ${LIB_NAMESPACE}::${target})
          add_library(${LIB_NAMESPACE}::${target} ALIAS ${target})
          message(
            STATUS
            "Created alias ${LIB_NAMESPACE}::${target} for ${target}"
          )
        endif()
      else()
        message(WARNING "Target ${target} not found for ${LIB_NAME}")
      endif()
    endforeach()
  endif()

  # Set added flag in parent scope
  set(${LIB_NAME}_ADDED ${${LIB_NAME}_ADDED} PARENT_SCOPE)
  set(${LIB_NAME}_SOURCE_DIR ${${LIB_NAME}_SOURCE_DIR} PARENT_SCOPE)

  # Mark as processed and cache the values
  # set(RS_CMAKE_LIB_${LIB_NAME_UPPER}_PROCESSED TRUE CACHE INTERNAL "")
  # set(RS_CMAKE_LIB_${LIB_NAME_UPPER}_ADDED ${${LIB_NAME}_ADDED} CACHE INTERNAL "")
  # set(RS_CMAKE_LIB_${LIB_NAME_UPPER}_SOURCE_DIR ${${LIB_NAME}_SOURCE_DIR} CACHE INTERNAL "")
endfunction()

#[=[
Get the correct target name for a dependency

This helper function determines which target variant (shared/static) to use
for a given dependency and returns the target name.

Usage:
  rs_get_dependency_target(
    DEPENDENCY dep_name
    RESULT_VAR variable_name
    [NAMESPACE namespace]           # Optional, defaults to DEPENDENCY
    [NAME target_name]              # Optional, defaults to DEPENDENCY
    [PREFER_SHARED|PREFER_STATIC]
  )

Examples:
  # When namespace, dependency, and target name are all the same
  rs_get_dependency_target(
    DEPENDENCY Monocypher
    RESULT_VAR MONO_TARGET
    PREFER_SHARED
  )
  # Produces: Monocypher::Monocypher or Monocypher::Monocypher_static

  # When namespace differs from target name (e.g., SQLite namespace, SQLite3 target)
  rs_get_dependency_target(
    DEPENDENCY SQLite
    NAME SQLite3
    RESULT_VAR SQLITE_TARGET
    PREFER_SHARED
  )
  # Produces: SQLite::SQLite3 or SQLite::SQLite3_static
#]=]
function(rs_get_dependency_target)
  set(options PREFER_SHARED PREFER_STATIC)
  set(oneValueArgs DEPENDENCY NAMESPACE NAME RESULT_VAR)
  set(multiValueArgs "")

  cmake_parse_arguments(
    GET_TARGET
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  # Validate parameters
  if(NOT GET_TARGET_DEPENDENCY)
    message(FATAL_ERROR "rs_get_dependency_target: DEPENDENCY is required")
  endif()
  if(NOT GET_TARGET_RESULT_VAR)
    message(FATAL_ERROR "rs_get_dependency_target: RESULT_VAR is required")
  endif()

  # Set defaults
  if(NOT GET_TARGET_NAMESPACE)
    set(GET_TARGET_NAMESPACE ${GET_TARGET_DEPENDENCY})
  endif()
  if(NOT GET_TARGET_NAME)
    set(GET_TARGET_NAME ${GET_TARGET_DEPENDENCY})
  endif()

  # Determine preference
  set(PREFER_STATIC_VAR OFF)
  if(GET_TARGET_PREFER_STATIC)
    set(PREFER_STATIC_VAR ON)
  endif()

  # Try to get target based on preference
  if(PREFER_STATIC_VAR)
    # Prefer static
    if(TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME}_static)
      set(RESULT_TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME}_static)
    elseif(TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME})
      set(RESULT_TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME})
    else()
      message(
        FATAL_ERROR
        "No target found for dependency ${GET_TARGET_DEPENDENCY} (tried ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME}_static and ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME})"
      )
    endif()
  else()
    # Prefer shared (default)
    if(TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME})
      set(RESULT_TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME})
    elseif(TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME}_static)
      set(RESULT_TARGET ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME}_static)
    else()
      message(
        FATAL_ERROR
        "No target found for dependency ${GET_TARGET_DEPENDENCY} (tried ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME} and ${GET_TARGET_NAMESPACE}::${GET_TARGET_NAME}_static)"
      )
    endif()
  endif()

  # Return the result in the specified variable
  set(${GET_TARGET_RESULT_VAR} ${RESULT_TARGET} PARENT_SCOPE)
endfunction()

#[=[
Link a library to a specific variant of a dependency

Usage:
  rs_link_dependency(
    TARGET target_name
    DEPENDENCY dep_name
    [NAMESPACE namespace]           # Optional, defaults to DEPENDENCY
    [NAME target_name]              # Optional, defaults to DEPENDENCY
    [PREFER_SHARED|PREFER_STATIC]
  )

Examples:
  # When namespace == dependency name (e.g., yyjson::yyjson)
  rs_link_dependency(TARGET mytarget DEPENDENCY yyjson PREFER_SHARED)

  # When namespace differs from target name (e.g., SQLite::SQLite3)
  rs_link_dependency(TARGET mytarget DEPENDENCY SQLite NAME SQLite3 PREFER_SHARED)
#]=]
function(rs_link_dependency)
  set(options PREFER_SHARED PREFER_STATIC)
  set(oneValueArgs TARGET DEPENDENCY NAMESPACE NAME)
  set(multiValueArgs "")

  cmake_parse_arguments(
    LINK
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  # Validate parameters
  if(NOT LINK_TARGET)
    message(FATAL_ERROR "rs_link_dependency: TARGET is required")
  endif()

  if(NOT TARGET ${LINK_TARGET})
    message(
      FATAL_ERROR
      "rs_link_dependency: Target '${LINK_TARGET}' does not exist"
    )
  endif()

  if(NOT LINK_DEPENDENCY)
    message(FATAL_ERROR "rs_link_dependency: DEPENDENCY is required")
  endif()

  # Set defaults
  if(NOT LINK_NAMESPACE)
    set(LINK_NAMESPACE ${LINK_DEPENDENCY})
  endif()
  if(NOT LINK_NAME)
    set(LINK_NAME ${LINK_DEPENDENCY})
  endif()

  # Determine preference
  set(PREFER_STATIC_VAR OFF)
  if(LINK_PREFER_STATIC)
    set(PREFER_STATIC_VAR ON)
  endif()

  # Get the correct dependency target
  if(PREFER_STATIC_VAR)
    rs_get_dependency_target(
      DEPENDENCY ${LINK_DEPENDENCY}
      NAMESPACE ${LINK_NAMESPACE}
      NAME ${LINK_NAME}
      RESULT_VAR DEP_TARGET
      PREFER_STATIC
    )
  else()
    rs_get_dependency_target(
      DEPENDENCY ${LINK_DEPENDENCY}
      NAMESPACE ${LINK_NAMESPACE}
      NAME ${LINK_NAME}
      RESULT_VAR DEP_TARGET
      PREFER_SHARED
    )
  endif()

  # Link to the target
  target_link_libraries(${LINK_TARGET} PUBLIC ${DEP_TARGET})
  message(STATUS "Linked ${LINK_TARGET} to ${DEP_TARGET}")
endfunction()
