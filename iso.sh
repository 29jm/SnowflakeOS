#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/RedStarOS.kernel isodir/boot/RedStarOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "RedStarOS" {
	multiboot /boot/RedStarOS.kernel
}
EOF

grub-mkrescue -o RedStarOS.iso isodir
