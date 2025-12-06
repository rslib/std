{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  ninja,
}:

stdenv.mkDerivation rec {
  pname = "monocypher";
  version = "4.0.2";

  src = fetchFromGitHub {
    owner = "LoupVaillant";
    repo = "Monocypher";
    rev = version;
    hash = "sha256-RrM8Ep/CM7U5Q4+4FAHfBknb6b0upohoiqy4f7eMye0=";
  };

  nativeBuildInputs = [
    cmake
    ninja
  ];

  cmakeFlags = [
    "-DMONOCYPHER_SHARED=ON"
  ];

  # Monocypher doesn't have a CMakeLists.txt by default, so we create one
  preConfigure = ''
    cat > CMakeLists.txt << 'EOF'
    cmake_minimum_required(VERSION 3.14)
    project(monocypher VERSION ${version} LANGUAGES C)

    include(GNUInstallDirs)

    option(MONOCYPHER_SHARED "Build shared library" ON)

    set(SOURCES src/monocypher.c)
    set(HEADERS src/monocypher.h)

    if(MONOCYPHER_SHARED)
      add_library(monocypher SHARED ''${SOURCES})
    else()
      add_library(monocypher STATIC ''${SOURCES})
    endif()

    target_include_directories(monocypher
      PUBLIC
        $<BUILD_INTERFACE:''${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:''${CMAKE_INSTALL_INCLUDEDIR}>
    )

    set_target_properties(monocypher PROPERTIES
      VERSION ${version}
      SOVERSION 4
      PUBLIC_HEADER "''${HEADERS}"
    )

    install(TARGETS monocypher
      EXPORT monocypherTargets
      LIBRARY DESTINATION ''${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ''${CMAKE_INSTALL_LIBDIR}
      PUBLIC_HEADER DESTINATION ''${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT monocypherTargets
      FILE monocypherTargets.cmake
      NAMESPACE monocypher::
      DESTINATION ''${CMAKE_INSTALL_LIBDIR}/cmake/monocypher
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
      ''${CMAKE_CURRENT_SOURCE_DIR}/cmake/monocypherConfig.cmake.in
      ''${CMAKE_CURRENT_BINARY_DIR}/monocypherConfig.cmake
      INSTALL_DESTINATION ''${CMAKE_INSTALL_LIBDIR}/cmake/monocypher
    )

    write_basic_package_version_file(
      ''${CMAKE_CURRENT_BINARY_DIR}/monocypherConfigVersion.cmake
      VERSION ${version}
      COMPATIBILITY SameMajorVersion
    )

    install(FILES
      ''${CMAKE_CURRENT_BINARY_DIR}/monocypherConfig.cmake
      ''${CMAKE_CURRENT_BINARY_DIR}/monocypherConfigVersion.cmake
      DESTINATION ''${CMAKE_INSTALL_LIBDIR}/cmake/monocypher
    )
    EOF

    mkdir -p cmake
    cat > cmake/monocypherConfig.cmake.in << 'EOF'
    @PACKAGE_INIT@
    include("''${CMAKE_CURRENT_LIST_DIR}/monocypherTargets.cmake")
    check_required_components(monocypher)
    EOF
  '';

  meta = with lib; {
    description = "An easy to use, easy to deploy crypto library";
    homepage = "https://monocypher.org/";
    license = with licenses; [
      bsd2
      cc0
    ];
    maintainers = [ ];
    platforms = platforms.all;
  };
}
