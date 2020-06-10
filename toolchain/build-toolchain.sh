#!/bin/bash

MAKE_OPTIONS="-j1"

function download() { # $1=url, $2=file
	if [ ! -f $2 ]; then
		curl "$1$2" > $2
	fi

	if [ ! -d ${2%.tar.xz} ]; then
		tar xf $2
	fi
}

function download-binutils() {
	url="ftp://ftp.gnu.org/gnu/binutils/"
	file=$(curl -sl $url | grep -E 'binutils-[0-9.]+.tar.xz$' | sort -V | tail -n1)
	download $url $file
	echo $PWD/${file%.tar.xz}
}

function download-gcc() {
	url="ftp://ftp.gnu.org/gnu/gcc/"
	folder=$(curl -sl $url | grep -E 'gcc-[0-9.]+$' | sort -V | tail -n1)
	url="$url$folder/"
	file="$folder.tar.xz"
	download $url $file
	echo $PWD/$folder
}

echo "Downloading dependencies"

mkdir -p src
cd src
echo " - binutils"
binutils_src=$(download-binutils)
echo " - gcc"
gcc_src=$(download-gcc)
cd ..

echo "Building binutils"

mkdir -p compiler
export PREFIX="$PWD/compiler"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p build
cd build

mkdir -p binutils
cd binutils
$binutils_src/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
make $MAKE_OPTIONS
make install
cd ..

echo "Building GCC"

mkdir -p gcc
cd gcc
$gcc_src/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc $MAKE_OPTIONS
make all-target-libgcc $MAKE_OPTIONS
make install-gcc
make install-target-libgcc
cd ..

cd .. # toolchain