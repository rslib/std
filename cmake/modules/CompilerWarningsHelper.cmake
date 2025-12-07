# CompilerWarningsHelper.cmake - Utilities for adding compiler warnings to targets

#[=[
Adds recommended compiler warnings to a target.
This function adds a comprehensive set of warnings appropriate for C projects,
with platform-specific handling for GCC, Clang, and MSVC.

Usage:
  target_add_warnings(
    TARGET target_name
    [WERROR]              # Treat warnings as errors
    [EXTRA_WARNINGS]      # Enable extra pedantic warnings
  )

Examples:
  # Basic warnings for a library
  target_add_warnings(TARGET mylib)

  # Strict warnings for tests (warnings as errors)
  target_add_warnings(TARGET mytest WERROR)

  # Extra pedantic warnings for development
  target_add_warnings(TARGET mylib EXTRA_WARNINGS)
#]=]
function(target_add_warnings)
  set(options WERROR EXTRA_WARNINGS)
  set(oneValueArgs TARGET)
  set(multiValueArgs "")

  cmake_parse_arguments(
    WARN
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  # Validate required arguments
  if(NOT WARN_TARGET)
    message(FATAL_ERROR "target_add_warnings: TARGET is required")
  endif()

  # Check if target exists
  if(NOT TARGET ${WARN_TARGET})
    message(
      FATAL_ERROR
      "target_add_warnings: Target '${WARN_TARGET}' does not exist"
    )
  endif()

  # ============================================================================
  # Common warnings for all compilers
  # ============================================================================
  set(WARNING_FLAGS "")

  # ============================================================================
  # GCC and Clang warnings
  # ============================================================================
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    list(
      APPEND WARNING_FLAGS
      -Wall # Enable most warnings
      -Wextra # Enable extra warnings
      -Wpedantic # Strict ISO C compliance
      -Wshadow # Warn about variable shadowing
      -Wcast-align # Warn about pointer cast alignment issues
      -Wunused # Warn about unused variables/functions
      -Wmisleading-indentation # Warn about misleading indentation
      -Wnull-dereference # Warn about null pointer dereference
      -Wdouble-promotion # Warn about implicit double promotion
      -Wformat=2 # Stricter format string checking
      -Wno-format-nonliteral # Allow non-literal format strings (needed for vsnprintf wrappers)
    )

    # Clang-specific warnings
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
      list(
        APPEND WARNING_FLAGS
        -Wconditional-uninitialized
        -Wshorten-64-to-32
        -Wno-gnu-zero-variadic-macro-arguments # Allow ##__VA_ARGS__ extension
      )
    endif()

    # GCC-specific warnings
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
      list(
        APPEND WARNING_FLAGS
        -Wduplicated-cond # Warn about duplicated conditions
        -Wlogical-op # Warn about suspicious logical operations
      )
      # Check GCC version for version-specific warnings
      if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "7.0")
        list(APPEND WARNING_FLAGS -Wduplicated-branches)
      endif()
    endif()

    # Extra warnings for pedantic mode
    if(WARN_EXTRA_WARNINGS)
      list(
        APPEND WARNING_FLAGS
        -Wcast-qual # Warn about casting away const
        -Wswitch-default # Warn about missing default in switch
        -Wswitch-enum # Warn about missing enum cases
        -Wstrict-prototypes # Require function prototypes
        -Wmissing-prototypes # Warn about missing prototypes
        -Wold-style-definition # Warn about old K&R style functions
        -Wundef # Warn about undefined macros
        -Wwrite-strings # Warn about string literal conversions
        -Wconversion # Warn about implicit type conversions
        -Wsign-conversion # Warn about sign conversions
      )
    endif()

    # Treat warnings as errors
    if(WARN_WERROR)
      list(APPEND WARNING_FLAGS -Werror)
    endif()

    # ============================================================================
    # MSVC warnings
    # ============================================================================
  elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    list(
      APPEND WARNING_FLAGS
      /W4 # Enable high warning level
      /w14242 # Conversion warnings
      /w14254 # Different const/volatile qualifiers
      /w14263 # Member function does not override
      /w14265 # Class has virtual functions but no virtual destructor
      /w14287 # Unsigned/negative constant mismatch
      /we4289 # Loop variable used outside loop
      /w14296 # Expression always true/false
      /w14311 # Pointer truncation
      /w14545 # Expression before comma has no effect
      /w14546 # Function call before comma missing argument
      /w14547 # Operator before comma has no effect
      /w14549 # Operator before comma has no effect
      /w14555 # Expression has no effect
      /w14619 # Pragma warning unavailable
      /w14640 # Thread-unsafe static initialization
      /w14826 # Conversion is sign-extended
      /w14905 # Wide string literal cast
      /w14906 # String literal cast
      /w14928 # Illegal copy-initialization
    )

    # Extra warnings for pedantic mode
    if(WARN_EXTRA_WARNINGS)
      list(
        APPEND WARNING_FLAGS
        /w14255 # No function prototype given
        /w14388 # Signed/unsigned mismatch
      )
    endif()

    # Treat warnings as errors
    if(WARN_WERROR)
      list(APPEND WARNING_FLAGS /WX)
    endif()
  endif()

  # ============================================================================
  # Apply warnings to target
  # ============================================================================
  target_compile_options(${WARN_TARGET} PRIVATE ${WARNING_FLAGS})

  # Log applied warnings (debug output)
  message(
    VERBOSE
    "Applied compiler warnings to target '${WARN_TARGET}': ${WARNING_FLAGS}"
  )
endfunction()

#[=[
Adds warnings to multiple targets at once.

Usage:
  target_add_warnings_multi(
    TARGETS target1 target2 ...
    [WERROR]
    [EXTRA_WARNINGS]
  )

Example:
  target_add_warnings_multi(
    TARGETS lib1 lib2 lib3
    WERROR
  )
#]=]
function(target_add_warnings_multi)
  set(options WERROR EXTRA_WARNINGS)
  set(oneValueArgs "")
  set(multiValueArgs TARGETS)

  cmake_parse_arguments(
    WARN
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  # Validate required arguments
  if(NOT WARN_TARGETS)
    message(FATAL_ERROR "target_add_warnings_multi: TARGETS is required")
  endif()

  # Build options list
  set(OPTIONS_LIST "")
  if(WARN_WERROR)
    list(APPEND OPTIONS_LIST WERROR)
  endif()
  if(WARN_EXTRA_WARNINGS)
    list(APPEND OPTIONS_LIST EXTRA_WARNINGS)
  endif()

  # Apply warnings to each target
  foreach(target ${WARN_TARGETS})
    target_add_warnings(TARGET ${target} ${OPTIONS_LIST})
  endforeach()
endfunction()
