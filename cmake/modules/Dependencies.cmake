# Dependencies.cmake - Project-specific dependency definitions
# Uses helper functions from DependencyHelpers.cmake

# CPM Configuration
set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpmsource")

# Use package lock if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/package-lock.cmake")
  cpmusepackagelock("${CMAKE_SOURCE_DIR}/package-lock.cmake")
endif()

# ==============================================================================
# Project-specific dependency functions
# ==============================================================================

#[=[
Add Unity testing framework

Usage:
  rs_need_unity()

  # Then link:
  rs_link_dependency(TARGET my_test DEPENDENCY Unity)
#]=]
function(rs_need_unity)
  rs_add_source_library(
    NAME Unity
    GITHUB_REPOSITORY ThrowTheSwitch/Unity
    VERSION 2.6.0
    GIT_TAG v2.6.0
    SOURCE_FILES src/unity.c
    HEADER_FILES unity.h unity_internals.h
    HEADER_SUBDIR src
    NAMESPACE Unity
    OUTPUT_NAME unity
    SOVERSION 2
    BUILD_SHARED OFF
    BUILD_STATIC ON
    FIND_PACKAGE_NAME Unity
    FIND_PACKAGE_TARGETS Unity::Unity Unity::unity_static Unity::unity_shared
  )
endfunction()

#[=[
Add yyjson JSON library

Usage:
  rs_need_yyjson()

  # Then link:
  rs_link_dependency(TARGET my_target DEPENDENCY yyjson)
#]=]
function(rs_need_yyjson)
  rs_add_source_library(
    NAME yyjson
    GITHUB_REPOSITORY ibireme/yyjson
    VERSION 0.12.0
    GIT_TAG 0.12.0
    SOURCE_FILES src/yyjson.c
    HEADER_FILES src/yyjson.h
    NAMESPACE yyjson
    OUTPUT_NAME yyjson
    SOVERSION 0
    FIND_PACKAGE_NAME yyjson
    FIND_PACKAGE_TARGETS yyjson::yyjson
  )
endfunction()

#[=[
Add SQLite3 database library

This function first tries to find system SQLite3.
If not found, it builds SQLite3 from source using CPM.

Usage:
  rs_need_sqlite3()

  # Then link:
  rs_link_dependency(TARGET my_target DEPENDENCY SQLite NAME SQLite3)
#]=]
function(rs_need_sqlite3)
  rs_add_source_library(
    NAME SQLite3
    URL https://sqlite.org/2025/sqlite-amalgamation-3510100.zip
    VERSION 3.51.1
    SOURCE_FILES sqlite3.c
    HEADER_FILES sqlite3.h
    NAMESPACE SQLite
    OUTPUT_NAME sqlite3
    SOVERSION 3
    PUBLIC_COMPILE_DEFINITIONS
      SQLITE_ENABLE_FTS5
      SQLITE_ENABLE_JSON1
      SQLITE_ENABLE_RTREE
      SQLITE_THREADSAFE=1
    PRIVATE_LINK_LIBRARIES
      $<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
      $<$<NOT:$<PLATFORM_ID:Windows>>:dl>
      $<$<NOT:$<PLATFORM_ID:Windows>>:m>
    FIND_PACKAGE_NAME SQLite3
    FIND_PACKAGE_TARGETS SQLite::SQLite3
    PKG_CONFIG_NAME sqlite3
    SKIP_FIND_PACKAGE_VAR RS_STD_FORCE_BUILD_SQLITE3
  )

  # Propagate FROM_SYSTEM to parent scope
  set(SQLite3_FROM_SYSTEM ${SQLite3_FROM_SYSTEM} PARENT_SCOPE)
endfunction()

#[=[
  Add Monocypher cryptography library
  Usage:
    rs_need_monocypher()

    # Then link:
    rs_link_dependency(TARGET my_target DEPENDENCY Monocypher)

  Note: monocypher package is not available on macOS via nixpkgs,
  so it will fallback to CPM on that platform.
#]=]
function(rs_need_monocypher)
  rs_add_source_library(
    NAME Monocypher
    GITHUB_REPOSITORY LoupVaillant/Monocypher
    VERSION 4.0.2
    GIT_TAG 4.0.2
    HEADER_SUBDIR src
    SOURCE_FILES src/monocypher.c
    HEADER_FILES monocypher.h
    NAMESPACE Monocypher
    OUTPUT_NAME monocypher
    SOVERSION 4
    FIND_PACKAGE_NAME monocypher
    FIND_PACKAGE_TARGETS monocypher::monocypher
    PKG_CONFIG_NAME monocypher
  )

  # Propagate FROM_SYSTEM to parent scope
  set(Monocypher_FROM_SYSTEM ${Monocypher_FROM_SYSTEM} PARENT_SCOPE)
endfunction()

#[=[
Add libcurl HTTP client library

This function first tries to find system libcurl.
If not found, it builds libcurl from source using CPM.

Usage:
  rs_need_curl()

  # Then link:
  rs_link_dependency(TARGET my_target DEPENDENCY CURL NAME libcurl)
#]=]
function(rs_need_curl)
  rs_add_cmake_library(
    NAME CURL
    GITHUB_REPOSITORY curl/curl
    VERSION 8.16.0
    GIT_TAG curl-8_16_0
    NAMESPACE CURL
    OPTIONS
      "BUILD_CURL_EXE OFF"
      "BUILD_SHARED_LIBS ${RS_STD_BUILD_SHARED}"
      "CURL_DISABLE_TESTS ON"
      "BUILD_TESTING OFF"
      "CURL_USE_LIBSSH2 OFF"
      "CURL_USE_LIBPSL OFF"
    FIND_PACKAGE_NAME CURL
    FIND_PACKAGE_TARGETS CURL::libcurl
  )

  # Create aliases for CPM-built targets
  if(NOT CURL_FROM_SYSTEM)
    if(TARGET libcurl_shared AND NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl ALIAS libcurl_shared)
    elseif(TARGET libcurl_static AND NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl ALIAS libcurl_static)
    elseif(TARGET libcurl AND NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl ALIAS libcurl)
    endif()
  endif()

  # Propagate FROM_SYSTEM to parent scope
  set(CURL_FROM_SYSTEM ${CURL_FROM_SYSTEM} PARENT_SCOPE)
endfunction()

#[=[
Add xxHash hashing library

Usage:
  rs_need_xxhash()

  # Then link:
  rs_link_dependency(TARGET my_target DEPENDENCY xxHash)
#]=]
function(rs_need_xxhash)
  rs_add_source_library(
    NAME xxHash
    GITHUB_REPOSITORY Cyan4973/xxHash
    VERSION 0.8.3
    GIT_TAG v0.8.3
    SOURCE_FILES xxhash.c
    HEADER_FILES xxhash.h
    NAMESPACE xxHash
    OUTPUT_NAME xxhash
    SOVERSION 0
    FIND_PACKAGE_NAME xxHash
    FIND_PACKAGE_TARGETS xxHash::xxhash
    PKG_CONFIG_NAME libxxhash
  )

  # Propagate FROM_SYSTEM to parent scope
  set(xxHash_FROM_SYSTEM ${xxHash_FROM_SYSTEM} PARENT_SCOPE)
endfunction()
