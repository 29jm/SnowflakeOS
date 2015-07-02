#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/SnowflakeOS.kernel isodir/boot/SnowflakeOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "SnowflakeOS" {
	multiboot /boot/SnowflakeOS.kernel
}
EOF

grub-mkrescue -o SnowflakeOS.iso isodir
