#!/bin/sh
set -e
. ./headers.sh

for PROJECT in $PROJECTS; do
  DESTDIR="$PWD/sysroot" $MAKE -C $PROJECT install
done
