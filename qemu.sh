#!/bin/sh
set -e
. ./iso.sh

qemu-system-i386 -cdrom SnowflakeOS.iso -monitor stdio -s -d int -D irq.log
