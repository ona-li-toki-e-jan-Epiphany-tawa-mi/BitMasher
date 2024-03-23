{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = with pkgs.buildPackages; [
    python311
    python311Packages.build
    python311Packages.twine
  ];
}
