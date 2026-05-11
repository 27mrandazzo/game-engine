{
  description = "A project for testing C++ language features";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs = inputs @ { flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" ];
      perSystem =
        {
          pkgs,
          self',
          meta,
          deps,
          mkPkg,
          ...
        }:
        rec {
          _module.args = {
            meta = {
              version = "0.1.0";
              description = "Game engine!";
            };
            deps = {
              runtime = with pkgs; [ 
                pkgs.mesa
                pkgs.libGL
                pkgs.wayland
                pkgs.libxkbcommon
               ];
              build = with pkgs; [ cmake ninja pkg-config ];
              devel = with pkgs; [ clang-tools ];
            };
            mkPkg = { 
              cpp-template = pkgs.llvmPackages_19.libcxxStdenv.mkDerivation {
                pname = "cpplab";
                version = meta.version;
                src = ./.;

                nativeBuildInputs = deps.build ++ deps.devel;
                buildInputs = deps.runtime;

                cmakeFlags = [
                  "-DPROJECT_VERSION=${meta.version}"
                  "-DPROJECT_DESCRIPTION=${meta.description}"
                  "-DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath,${pkgs.libglvnd}/lib"
                ];
                
                # shellHook = ''
                #   export CXXFLAGS="${CXXFLAGS} -nostdinc++ -stdlib=libc++"
                # '';
              };
            };
          };
            packages.default = mkPkg.cpp-template;           
            devShells.default = pkgs.mkShell {
              packages = deps.build ++ deps.devel;
              inputsFrom = [ packages.default ];
              
              shellHook = ''
                export LD_LIBRARY_PATH=${pkgs.mesa}/lib:${pkgs.libGL}/lib:${pkgs.wayland}/lib:$LD_LIBRARY_PATH
                export LIBGL_ALWAYS_SOFTWARE=1
                export __EGL_VENDOR_LIBRARY_DIRS=${pkgs.mesa}/share/glvnd/egl_vendor.d
              '';
            };
          };
        };
    }
  
