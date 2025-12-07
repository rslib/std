cmake_minimum_required(VERSION 3.10)

# Prefer Ninja if available
if(NOT CMAKE_GENERATOR)
  find_program(NINJA_EXECUTABLE ninja)
  if(NINJA_EXECUTABLE)
    set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
  endif()
endif()

set(RS_STD_VERSION_MAJOR 0)
set(RS_STD_VERSION_MINOR 1)
set(RS_STD_VERSION_PATCH 1)
set(
  RS_STD_VERSION
  "${RS_STD_VERSION_MAJOR}.${RS_STD_VERSION_MINOR}.${RS_STD_VERSION_PATCH}"
)

project(rs_std VERSION ${RS_STD_VERSION} LANGUAGES C)

# ##############################################################################
# Project Configuration
# ##############################################################################

set(RS_STD_PACKAGE ${PROJECT_NAME})
set(RS_STD_PACKAGE_NAME ${PROJECT_NAME})
set(RS_STD_PACKAGE_VERSION "${PROJECT_VERSION}")
set(
  RS_STD_PACKAGE_VERSION_MAJOR
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)
set(RS_STD_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_PATCH}")
set(RS_STD_PACKAGE_STRING "${RS_STD_PACKAGE_NAME} ${RS_STD_PACKAGE_VERSION}")
set(RS_STD_PACKAGE_TARNAME "${RS_STD_PACKAGE}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORS_COMPILE_COMMANDS ON)

# RPATH settings for finding shared libraries at runtime
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# Use $ORIGIN on Linux, @loader_path on macOS
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
else()
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()
# For build tree, use the binary lib directory
set(CMAKE_BUILD_RPATH "${CMAKE_BINARY_DIR}/lib")

# ##############################################################################
# Options
# ##############################################################################

option(RS_STD_DEBUG "Enable debug mode including verbose logging" OFF)
option(RS_STD_BUILD_SHARED "Build shared library" ON)
option(RS_STD_BUILD_STATIC "Build static library" ON)
option(RS_STD_BUILD_BINARIES "Build command-line binaries" ON)
option(RS_STD_BUILD_TESTS "Build tests" OFF)
option(
  RS_STD_FORCE_BUILD_SQLITE3
  "Force building SQLite3 from source instead of using system package"
  OFF
)
option(RS_STD_ENABLE_ASAN "Enable AddressSanitizer for debugging" OFF)
option(RS_STD_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Logging options
option(RS_STD_ENABLE_LOGGING "Enable logging system" ON)
option(
  RS_STD_ENABLE_TRACE_LOGS
  "Enable trace-level logging (may impact performance)"
  OFF
)
option(RS_STD_LOG_USE_COLOR "Enable colored log output by default" ON)
option(RS_STD_LOG_USE_TIMESTAMP "Include timestamps in log output" ON)
set(
  RS_STD_LOG_LEVEL
  "INFO"
  CACHE STRING
  "Minimum compile-time log level (TRACE|DEBUG|INFO|WARN|ERROR|FATAL|OFF)"
)
set_property(
  CACHE RS_STD_LOG_LEVEL
  PROPERTY STRINGS TRACE DEBUG INFO WARN ERROR FATAL OFF
)

# ##############################################################################
# Sanitizer Configuration
# ##############################################################################

if(RS_STD_ENABLE_ASAN)
  message(STATUS "AddressSanitizer enabled")
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address)
endif()

if(RS_STD_ENABLE_UBSAN)
  message(STATUS "UndefinedBehaviorSanitizer enabled")
  add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=undefined)
endif()

# ##############################################################################
# Internal Paths for cmake libraries and Setup install and output Directories
# ##############################################################################

# This sets where to look for dependent libraries
set(
  CMAKE_PREFIX_PATH
  ${CMAKE_PREFIX_PATH}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_INSTALL_PREFIX}
)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/CPM.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/CompilerWarningsHelper.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/LibraryHelpers.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/DependencyHelpers.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/Dependencies.cmake)

# ##############################################################################
# Generate logging configuration header
# ##############################################################################

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/rs/std/logging/logging_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/rs/std/logging/logging_config.h"
  @ONLY
)

# Add binary dir to include path for generated headers
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

# ##############################################################################
# Subdirectories
# ##############################################################################

add_subdirectory(src)

# Install public headers
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
)

if(RS_STD_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# Symlink compile_commands.json to build/ for clangd (done at configure time)
# Skip if source and binary dirs would create the same path (e.g., in Nix builds)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(_compile_commands_src "${CMAKE_BINARY_DIR}/compile_commands.json")
  set(_compile_commands_dst "${CMAKE_SOURCE_DIR}/build/compile_commands.json")
  if(NOT "${_compile_commands_src}" STREQUAL "${_compile_commands_dst}")
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
    file(
      CREATE_LINK "${_compile_commands_src}" "${_compile_commands_dst}"
      COPY_ON_ERROR
      SYMBOLIC
    )
  endif()
endif()
