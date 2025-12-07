# ==============================================================================
# rs_std - Core library
# Provides reusable utilities for all tools in this project
# ==============================================================================

# Link Monocypher for crypto support
rs_need_monocypher()

# Link xxHash for hashing support
rs_need_xxhash()

# Link SQLite3 for database support
rs_need_sqlite3()

# Get the correct Monocypher targets for shared and static variants
if(RS_STD_BUILD_SHARED)
  rs_get_dependency_target(
      DEPENDENCY Monocypher
      RESULT_VAR MONOCYPHER_SHARED_TARGET
      PREFER_SHARED
  )
endif()

if(RS_STD_BUILD_STATIC)
  rs_get_dependency_target(
      DEPENDENCY Monocypher
      RESULT_VAR MONOCYPHER_STATIC_TARGET
      PREFER_STATIC
  )
endif()

# Get the correct xxHash targets for shared and static variants
if(RS_STD_BUILD_SHARED)
  rs_get_dependency_target(
      DEPENDENCY xxHash
      RESULT_VAR XXHASH_SHARED_TARGET
      PREFER_SHARED
  )
endif()

if(RS_STD_BUILD_STATIC)
  rs_get_dependency_target(
      DEPENDENCY xxHash
      RESULT_VAR XXHASH_STATIC_TARGET
      PREFER_STATIC
  )
endif()

# Get the correct SQLite3 targets for shared and static variants
# Namespace is SQLite (per CMake's FindSQLite3), but target name is SQLite3
if(RS_STD_BUILD_SHARED)
  rs_get_dependency_target(
      DEPENDENCY SQLite
      NAMESPACE SQLite
      NAME SQLite3
      RESULT_VAR SQLITE3_SHARED_TARGET
      PREFER_SHARED
  )
endif()

if(RS_STD_BUILD_STATIC)
  rs_get_dependency_target(
      DEPENDENCY SQLite
      NAMESPACE SQLite
      NAME SQLite3
      RESULT_VAR SQLITE3_STATIC_TARGET
      PREFER_STATIC
  )
endif()

# Platform-specific libraries
set(RS_STD_PLATFORM_LIBS "")
if(WIN32)
  list(APPEND RS_STD_PLATFORM_LIBS bcrypt)
endif()

rs_create_library(
    NAME rs_std
    NAMESPACE rs
    EXPORT_NAME std
    SOURCES
        # Error handling
        rs/std/error.c
        rs/std/defer.c
        # Logging
        rs/std/logging/logging.c
        # Allocators
        rs/std/allocators/allocator.c
        rs/std/allocators/arena.c
        rs/std/allocators/stack.c
        # Strings
        rs/std/string/string.c
        rs/std/string/string_view.c
        rs/std/string/zstring_view.c
        rs/std/string/string_utils.c
        # Containers
        rs/std/containers/array.c
        rs/std/containers/span.c
        rs/std/containers/slice.c
        rs/std/containers/hash.c
        rs/std/containers/hashmap.c
        # Operating System
        rs/std/os/env.c
        rs/std/os/detect.c
        rs/std/os/process.c
        # I/O
        rs/std/io/read.c
        rs/std/io/write.c
        # Filesystem
        rs/std/fs/path.c
        rs/std/fs/file.c
        rs/std/fs/dir.c
        # Encoding
        rs/std/encoding/base64.c
        # Cryptography
        rs/std/crypto/crypto.c
        # Database
        rs/std/db/sqlite.c
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Core utilities library (allocators, strings, containers, etc.)"
    BUILD_SHARED ${RS_STD_BUILD_SHARED}
    BUILD_STATIC ${RS_STD_BUILD_STATIC}
    SHARED_LINK_LIBRARIES
      ${MONOCYPHER_SHARED_TARGET}
      ${XXHASH_SHARED_TARGET}
      ${SQLITE3_SHARED_TARGET}
      ${RS_STD_PLATFORM_LIBS}
    STATIC_LINK_LIBRARIES
      ${MONOCYPHER_STATIC_TARGET}
      ${XXHASH_STATIC_TARGET}
      ${SQLITE3_STATIC_TARGET}
      ${RS_STD_PLATFORM_LIBS}
    PUBLIC_INCLUDE_DIRS
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    FIND_DEPENDENCIES
      monocypher
      xxHash
      SQLite3
    PKG_CONFIG_LIBS_PRIVATE
      "-lpthread -ldl -lm"
    STATIC_PKG_CONFIG_DEPS
      "Monocypher:monocypher:monocypher_static"
      "xxHash:libxxhash:libxxhash_static"
      "SQLite3:sqlite3:sqlite3_static"
)
