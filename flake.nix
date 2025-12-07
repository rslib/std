{
  description = "rs_std - A C standard library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      treefmt-nix,
      pre-commit-hooks,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # Custom packages
        monocypher = pkgs.callPackage ./nix/monocypher.nix { };
        unity-test = pkgs.callPackage ./nix/unity-test.nix { };

        treefmtEval = treefmt-nix.lib.evalModule pkgs {
          projectRootFile = "flake.nix";

          programs = {
            nixfmt.enable = true;
            clang-format.enable = true;
          };

          settings.formatter = {
            clang-format = {
              includes = [
                "*.c"
                "*.h"
                "*.cpp"
                "*.hpp"
              ];
              excludes = [
                "build/*"
                ".cpmsource/*"
              ];
            };

            gersemi = {
              command = pkgs.gersemi;
              options = [
                "-i"
                "--indent"
                "2"
              ];
              includes = [
                "CMakeLists.txt"
                "*.cmake"
              ];
              excludes = [
                "build/*"
                ".cpmsource/*"
                "cmake/modules/CPM.cmake"
              ];
            };

            nixfmt = {
              includes = [ "*.nix" ];
            };
          };
        };

        pre-commit-check = pre-commit-hooks.lib.${system}.run {
          src = ./.;
          hooks = {
            treefmt = {
              enable = true;
              package = treefmtEval.config.build.wrapper;
            };
            cocogitto = {
              enable = true;
              entry = "${pkgs.cocogitto}/bin/cog verify --file";
              stages = [ "commit-msg" ];
            };
          };
        };

        buildInputs = [
          monocypher
        ]
        ++ (with pkgs; [
          sqlite
          openssl
          yyjson
          xxHash
        ]);

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
        ];
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs =
            buildInputs
            ++ (with pkgs; [
              act
            ]);
          nativeBuildInputs =
            nativeBuildInputs
            ++ (
              with pkgs;
              [
                alejandra
                nil
                clang-analyzer
                cocogitto
                treefmtEval.config.build.wrapper
              ]
              ++ pkgs.lib.optionals pkgs.stdenv.isLinux [
                gdb
                valgrind
              ]
            );

          shellHook = ''
            ${pre-commit-check.shellHook}
            echo "rs_std development environment"
            echo "Run 'cmake --preset=dev' to configure"
            echo "Run 'cmake --build build' to build"
            echo "Run 'cmake --install build' to install (required for package tests)"
            echo "Run 'ctest --test-dir build' to test"
            echo "Run 'treefmt' to format all files"
          '';
        };

        formatter = treefmtEval.config.build.wrapper;

        checks = {
          formatting = treefmtEval.config.build.check self;
          pre-commit-check = pre-commit-check;
          tests = pkgs.stdenv.mkDerivation {
            pname = "rs_std-tests";
            version = "0.1.0";

            src = ./.;

            inherit buildInputs;
            nativeBuildInputs = nativeBuildInputs ++ [
              unity-test
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DRS_STD_BUILD_TESTS=ON"
              "-DRS_STD_BUILD_SHARED=ON"
              "-DRS_STD_BUILD_STATIC=ON"
            ];

            # Set install prefix to TMPDIR for package discovery tests
            # Also force relative libdir to prevent Nix from using absolute path
            preConfigure = ''
              cmakeFlagsArray+=(
                "-DCMAKE_INSTALL_PREFIX=$TMPDIR/test-install"
                "-DCMAKE_INSTALL_LIBDIR=lib"
              )
            '';

            doCheck = true;
            checkPhase = ''
              ctest --output-on-failure
            '';

            installPhase = ''
              mkdir -p $out
              touch $out/tests-passed
            '';
          };
        };

        packages = {
          inherit monocypher unity-test;

          default = pkgs.stdenv.mkDerivation {
            pname = "rs_std";
            version = "0.1.0";

            src = ./.;

            inherit buildInputs nativeBuildInputs;

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DRS_STD_BUILD_TESTS=OFF"
              "-DRS_STD_BUILD_SHARED=ON"
              "-DRS_STD_BUILD_STATIC=ON"
            ];
          };
        };
      }
    );
}
