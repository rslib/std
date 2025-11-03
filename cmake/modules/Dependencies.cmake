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
  )
endfunction()

#[=[
Add SQLite3 database library

This function first tries to find system SQLite3.
If not found, it builds SQLite3 from source using CPM.

Usage:
  rs_need_sqlite3()

  # Then link:
  rs_link_dependency(TARGET my_target DEPENDENCY SQLite)
  # Or use legacy function:
  rs_link_sqlite3(my_target SHARED)  # or STATIC
#]=]
function(rs_need_sqlite3)
  # Option to force building SQLite3 from source
  # Set RS_STD_FORCE_BUILD_SQLITE3=ON to always build from CPM
  if(NOT DEFINED RS_STD_FORCE_BUILD_SQLITE3)
    set(RS_STD_FORCE_BUILD_SQLITE3 OFF)
  endif()

  # Try to find system SQLite3 first (unless forced to build)
  if(NOT RS_STD_FORCE_BUILD_SQLITE3)
    find_package(SQLite3 3.35 QUIET)
  else()
    set(SQLite3_FOUND FALSE)
  endif()

  if(SQLite3_FOUND)
    message(STATUS "Found system SQLite3: ${SQLite3_LIBRARIES}")

    # Create alias for system SQLite3 if it doesn't exist
    if(NOT TARGET SQLite::SQLite3)
      add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
      set_target_properties(
        SQLite::SQLite3
        PROPERTIES
          IMPORTED_LOCATION "${SQLite3_LIBRARIES}"
          INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIRS}"
      )
    endif()

    if(NOT TARGET SQLite::SQLite3_static)
      add_library(SQLite::SQLite3_static ALIAS SQLite::SQLite3)
    endif()

    # Set variables in parent scope
    set(SQLite3_FOUND ${SQLite3_FOUND} PARENT_SCOPE)
    set(SQLite3_LIBRARIES ${SQLite3_LIBRARIES} PARENT_SCOPE)
    set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIRS} PARENT_SCOPE)
    set(SQLite3_CPM FALSE PARENT_SCOPE)
  else()
    # Build with CPM
    # Note: SQLite amalgamation ZIPs contain files in a subdirectory
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
    )

    set(SQLite3_CPM TRUE PARENT_SCOPE)
    message(STATUS "Built SQLite3 with CPM")
  endif()
endfunction()

#[=[
  Add Monocypher cryptography library
  Usage:
    rs_need_monocypher()

    # Then link:
    rs_link_dependency(TARGET my_target DEPENDENCY Monocypher)
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
  )
endfunction()

#[=[
Add libcurl HTTP client library

This function first tries to find system libcurl.
If not found, it builds libcurl from source using CPM.

Usage:
  rs_need_curl()

  # Then link:
  rs_link_dependency(TARGET my_target DEPENDENCY CURL)
#]=]
function(rs_need_curl)
  # Option to force building libcurl from source
  # Set RS_STD_FORCE_BUILD_CURL=ON to always build from CPM
  if(NOT DEFINED RS_STD_FORCE_BUILD_CURL)
    set(RS_STD_FORCE_BUILD_CURL OFF)
  endif()

  # Try to find system libcurl first (unless forced to build)
  if(NOT RS_STD_FORCE_BUILD_CURL)
    find_package(CURL QUIET)
  else()
    set(CURL_FOUND FALSE)
  endif()

  if(CURL_FOUND)
    message(STATUS "Found system libcurl: ${CURL_LIBRARIES}")

    # CURL package usually provides CURL::libcurl target
    # But ensure aliases exist for consistency
    if(NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl UNKNOWN IMPORTED)
      set_target_properties(
        CURL::libcurl
        PROPERTIES
          IMPORTED_LOCATION "${CURL_LIBRARIES}"
          INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
      )
    endif()

    # Set variables in parent scope
    set(CURL_FOUND ${CURL_FOUND} PARENT_SCOPE)
    set(CURL_LIBRARIES ${CURL_LIBRARIES} PARENT_SCOPE)
    set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIRS} PARENT_SCOPE)
    set(CURL_CPM FALSE PARENT_SCOPE)
  else()
    # Build with CPM
    rs_add_cmake_library(
      NAME CURL
      GITHUB_REPOSITORY curl/curl
      VERSION 8.16.0
      GIT_TAG curl-8_16_0
      OPTIONS
        "BUILD_CURL_EXE OFF"
        "BUILD_SHARED_LIBS ${RS_STD_BUILD_SHARED}"
        "CURL_DISABLE_TESTS ON"
        "BUILD_TESTING OFF"
        "CURL_USE_LIBSSH2 OFF"
        "CURL_USE_LIBPSL OFF"
    )

    # Create aliases for consistency
    if(TARGET libcurl_shared AND NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl ALIAS libcurl_shared)
    elseif(TARGET libcurl_static AND NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl ALIAS libcurl_static)
    elseif(TARGET libcurl AND NOT TARGET CURL::libcurl)
      add_library(CURL::libcurl ALIAS libcurl)
    endif()

    set(CURL_CPM TRUE PARENT_SCOPE)
    message(STATUS "Built libcurl with CPM")
  endif()
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
  )
endfunction()
