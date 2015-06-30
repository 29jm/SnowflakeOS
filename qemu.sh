#!/bin/sh
set -e
. ./iso.sh

qemu-system-i386 -cdrom RedStarOS.iso -monitor stdio
