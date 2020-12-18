#!/usr/bin/env sh

log() {
    printf "\033[33m->\033[m %s\n" "$*"
}

download() {
    log "Downloading $2"
    wget -c "$1"
    log "Inflating $2"
    tar xf "$2"
}

download_binutils() {
    version="2.34"
    binutils="binutils-$version.tar.xz"
    download "https://ftp.gnu.org/gnu/binutils/$binutils" "$binutils"
    export binutils_src="$PWD/binutils-$version"
}

download_gcc() {
    version="10.1.0"
    gcc="gcc-$version.tar.xz"
    download "ftp://ftp.gnu.org/gnu/gcc/gcc-$version/$gcc" "$gcc"
    export gcc_src="$PWD/gcc-$version"
}

build_binutils() {
    log "Building binutils"
    cd binutils
    "$binutils_src/configure" --target="$TARGET"    \
                              --prefix="$PREFIX"    \
                              --with-sysroot        \
                              --disable-nls         \
                              --disable-werror
    make
    make install
    cd ..
}

build_gcc() {
    log "Building gcc"
    cd gcc
    "$gcc_src/configure" --target="$TARGET"         \
                         --prefix="$PREFIX"         \
                         --disable-nls              \
                         --enable-languages=c,c++   \
                         --without-headers
    make all-gcc
    make all-target-libgcc
    make install-gcc
    make install-target-libgcc
    cd ..
}

main() {
    # exit if any commands fails
    set -e
    # disable word globbing
    set -f

    cd toolchain

    mkdir -p src compiler build
    mkdir -p build/binutils build/gcc

    export PREFIX="$PWD/compiler"
    export TARGET="i686-elf"
    export PATH="$PREFIX/bin:$PATH"

    # download sources
    cd src
    download_binutils
    download_gcc
    cd .. # toolchain/

    # build binutils
    cd build
    build_binutils 
    build_gcc
}

main "$@"
