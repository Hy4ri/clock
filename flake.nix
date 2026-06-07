{
  description = "clock";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    allSystems = ["x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"];
    forAllSystems = f:
      nixpkgs.lib.genAttrs allSystems (system:
        f {
          pkgs = import nixpkgs {inherit system;};
        });
  in {
    devShells = forAllSystems ({pkgs}: {
      default = pkgs.mkShell {
        buildInputs = with pkgs; [
          clang-tools
          gcc
          gnumake
          pkg-config
          libX11
          libXft
          fontconfig
        ];

        env = {
          C_INCLUDE_PATH = "${pkgs.glibc.dev}/include";
          CPLUS_INCLUDE_PATH = "${pkgs.glibc.dev}/include";
        };
      };
    });
  };
}
