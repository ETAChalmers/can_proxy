with import <nixpkgs> {}; # bring all of Nixpkgs into scope

stdenv.mkDerivation rec {
    name = "can_proxy";
    src = ./.;
    buildInputs = [
        gnumake
        automake
        autoconf
        libtool
        pkgconfig

        check

        glib
    ];
}


