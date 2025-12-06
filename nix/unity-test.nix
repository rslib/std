{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  ninja,
}:

stdenv.mkDerivation rec {
  pname = "unity-test";
  version = "2.6.0";

  src = fetchFromGitHub {
    owner = "ThrowTheSwitch";
    repo = "Unity";
    rev = "v${version}";
    hash = "sha256-SCcUGNN/UJlu3ALJiZ9bQKxYRZey3cm9QG+NOehp6Ow=";
  };

  nativeBuildInputs = [
    cmake
    ninja
  ];

  preConfigure = ''
        cat > CMakeLists.txt << 'CMAKEFILE'
        cmake_minimum_required(VERSION 3.14)
        project(unity VERSION 2.6.0 LANGUAGES C)

        include(GNUInstallDirs)

        option(UNITY_BUILD_SHARED "Build shared library" ON)
        option(UNITY_BUILD_STATIC "Build static library" ON)

        set(SOURCES src/unity.c)
        set(HEADERS src/unity.h src/unity_internals.h)

        if(UNITY_BUILD_SHARED)
          add_library(unity_shared SHARED ''${SOURCES})
          target_include_directories(unity_shared
            PUBLIC
              $<BUILD_INTERFACE:''${CMAKE_CURRENT_SOURCE_DIR}/src>
              $<INSTALL_INTERFACE:''${CMAKE_INSTALL_INCLUDEDIR}>
          )
          set_target_properties(unity_shared PROPERTIES
            OUTPUT_NAME unity
            VERSION ''${PROJECT_VERSION}
            SOVERSION 2
          )
        endif()

        if(UNITY_BUILD_STATIC)
          add_library(unity_static STATIC ''${SOURCES})
          target_include_directories(unity_static
            PUBLIC
              $<BUILD_INTERFACE:''${CMAKE_CURRENT_SOURCE_DIR}/src>
              $<INSTALL_INTERFACE:''${CMAKE_INSTALL_INCLUDEDIR}>
          )
          set_target_properties(unity_static PROPERTIES
            OUTPUT_NAME unity
          )
        endif()

        install(FILES ''${HEADERS}
          DESTINATION ''${CMAKE_INSTALL_INCLUDEDIR}
        )

        # Install libraries
        if(UNITY_BUILD_SHARED)
          install(TARGETS unity_shared
            EXPORT UnityTargets
            LIBRARY DESTINATION ''${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ''${CMAKE_INSTALL_LIBDIR}
          )
        endif()

        if(UNITY_BUILD_STATIC)
          install(TARGETS unity_static
            EXPORT UnityTargets
            LIBRARY DESTINATION ''${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ''${CMAKE_INSTALL_LIBDIR}
          )
        endif()

        install(EXPORT UnityTargets
          FILE UnityTargets.cmake
          NAMESPACE Unity::
          DESTINATION ''${CMAKE_INSTALL_LIBDIR}/cmake/Unity
        )

        include(CMakePackageConfigHelpers)
        configure_package_config_file(
          ''${CMAKE_CURRENT_SOURCE_DIR}/cmake/UnityConfig.cmake.in
          ''${CMAKE_CURRENT_BINARY_DIR}/UnityConfig.cmake
          INSTALL_DESTINATION ''${CMAKE_INSTALL_LIBDIR}/cmake/Unity
        )

        write_basic_package_version_file(
          ''${CMAKE_CURRENT_BINARY_DIR}/UnityConfigVersion.cmake
          VERSION ''${PROJECT_VERSION}
          COMPATIBILITY SameMajorVersion
        )

        install(FILES
          ''${CMAKE_CURRENT_BINARY_DIR}/UnityConfig.cmake
          ''${CMAKE_CURRENT_BINARY_DIR}/UnityConfigVersion.cmake
          DESTINATION ''${CMAKE_INSTALL_LIBDIR}/cmake/Unity
        )

        # Generate pkg-config file
        file(WRITE ''${CMAKE_CURRENT_BINARY_DIR}/unity.pc
          "prefix=''${CMAKE_INSTALL_PREFIX}\n"
          "exec_prefix=\''${prefix}\n"
          "libdir=''${CMAKE_INSTALL_FULL_LIBDIR}\n"
          "includedir=''${CMAKE_INSTALL_FULL_INCLUDEDIR}\n"
          "\n"
          "Name: unity\n"
          "Description: Unity Unit Testing Framework for C\n"
          "Version: ''${PROJECT_VERSION}\n"
          "Libs: -L\''${libdir} -lunity\n"
          "Cflags: -I\''${includedir}\n"
        )
        install(FILES ''${CMAKE_CURRENT_BINARY_DIR}/unity.pc
          DESTINATION ''${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
    CMAKEFILE

        mkdir -p cmake
        cat > cmake/UnityConfig.cmake.in << 'CMAKECONFIG'
    @PACKAGE_INIT@
    include("''${CMAKE_CURRENT_LIST_DIR}/UnityTargets.cmake")

    # Mark Unity as found from system (prevent CPM fallback logic from creating aliases)
    set(Unity_FROM_SYSTEM TRUE)

    # Create convenience target that matches what DependencyHelpers expects
    # Unity::unity_static is the imported target from UnityTargets.cmake
    # Unity::Unity is what rs_link_dependency will look for
    if(TARGET Unity::unity_static AND NOT TARGET Unity::Unity)
      # Create an INTERFACE IMPORTED library that forwards to unity_static
      add_library(Unity::Unity INTERFACE IMPORTED)
      set_target_properties(Unity::Unity PROPERTIES
        INTERFACE_LINK_LIBRARIES Unity::unity_static
      )
    endif()

    check_required_components(Unity)
    CMAKECONFIG
  '';

  cmakeFlags = [
    "-DUNITY_BUILD_SHARED=OFF"
    "-DUNITY_BUILD_STATIC=ON"
  ];

  meta = with lib; {
    description = "Unity Unit Testing Framework for C";
    homepage = "https://github.com/ThrowTheSwitch/Unity";
    license = licenses.mit;
    maintainers = [ ];
    platforms = platforms.all;
  };
}
