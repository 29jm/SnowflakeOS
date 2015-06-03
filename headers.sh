#!/bin/sh
set -e
. ./config.sh

mkdir -p sysroot

for PROJECT in $SYSTEM_HEADER_PROJECTS; do
  DESTDIR="$PWD/sysroot" $MAKE -C $PROJECT install-headers
done
