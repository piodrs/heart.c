{
  description = "ASCII 3D heart";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/26.05";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              gcc
              gnumake

              clang-tools
            ];

            shellHook = ''
              echo "ASCII 3D heart development environment"
            '';
          };
        }
      );
    };
}
