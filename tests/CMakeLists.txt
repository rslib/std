# ##############################################################################
# RS Tests
# ##############################################################################

rs_need_unity()

#[=[
Helper function to create a test executable with standard configuration

Usage:
  rs_add_test(
    NAME test_name
    SOURCES source1.c source2.c ...
    [DEPENDENCIES dep1 dep2 ...]  # Additional dependencies beyond rs and Unity
  )

This function:
- Creates an executable with the given name and sources
- Links to rs::std and Unity
- Links to any additional dependencies specified
- Registers the test with CTest
#]=]
function(rs_add_test)
  set(options "")
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES DEPENDENCIES)

  cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Validate required arguments
  if(NOT TEST_NAME)
    message(FATAL_ERROR "rs_add_test: NAME is required")
  endif()

  if(NOT TEST_SOURCES)
    message(FATAL_ERROR "rs_add_test: SOURCES is required")
  endif()

  # Create test executable
  add_executable(${TEST_NAME} ${TEST_SOURCES})

  # On Windows, place test executables in the same directory as DLLs
  # This ensures the test can find all required DLLs at runtime
  if(WIN32)
    set_target_properties(${TEST_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel
    )
  endif()

  # Link to rs_std
  target_link_libraries(${TEST_NAME} PRIVATE rs::std)

  # MSVC parallel builds need explicit dependencies to ensure import libraries
  # are fully generated before linking tests
  if(MSVC)
    if(TARGET rs_std_shared)
      add_dependencies(${TEST_NAME} rs_std_shared)
    endif()
    if(TARGET rs_std_static)
      add_dependencies(${TEST_NAME} rs_std_static)
    endif()
  endif()

  # Link to Unity using rs_link_dependency
  rs_link_dependency(TARGET ${TEST_NAME} DEPENDENCY Unity)

  # Link to additional dependencies if specified
  if(TEST_DEPENDENCIES)
    foreach(dep ${TEST_DEPENDENCIES})
      rs_link_dependency(TARGET ${TEST_NAME} DEPENDENCY ${dep})
    endforeach()
  endif()

  # Add compiler warnings (less strict for tests)
  target_add_warnings(TARGET ${TEST_NAME})
  # Disable some pedantic warnings that are noisy in test code
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${TEST_NAME} PRIVATE
      -Wno-shadow                       # Allow shadowing in nested scopes
    )
  endif()
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(${TEST_NAME} PRIVATE
      -Wno-cast-function-type-mismatch  # Allow function pointer casts in tests
      -Wno-shorten-64-to-32             # Allow size conversions in tests
    )
  endif()

  # Register with CTest
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_subdirectory(rs/std)
add_subdirectory(package-discovery)
