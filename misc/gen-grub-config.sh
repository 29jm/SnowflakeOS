#!/usr/bin/bash

# Expects to be run from the main Makefile
echo "insmod efi_gop" > "$GRUBCFG"

echo "menuentry \"SnowflakeOS - Challenge Edition\" {" >> "$GRUBCFG"
echo "    multiboot /boot/SnowflakeOS.kernel" >> "$GRUBCFG"

for f in "$ISODIR"/modules/*; do
    bname=$(basename "$f")
    name=$(basename "$f" | cut -d. -f1)
    echo "    module /modules/$bname" "$name" >> "$GRUBCFG"
done

echo "}" >> "$GRUBCFG"
