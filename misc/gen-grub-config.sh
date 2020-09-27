#!/usr/bin/bash

# Expects to be run from the main Makefile
grub_config=misc/grub.cfg

echo "menuentry \"SnowflakeOS - Challenge Edition\" {" > $grub_config
echo "    multiboot /boot/SnowflakeOS.kernel" >> $grub_config

for f in "$ISODIR"/modules/*; do
	bname=$(basename "$f")
	name=$(basename "$f" | cut -d. -f1)
	echo "    module /modules/$bname" "$name" >> $grub_config
done

echo "}" >> $grub_config
