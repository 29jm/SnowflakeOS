#!/usr/bin/bash

# Expects to be run from the main Makefile
grub_config=misc/grub.cfg

echo "menuentry \"SnowflakeOS - Challenge Edition\" {" > $grub_config
echo "    multiboot /boot/SnowflakeOS.kernel" >> $grub_config

for f in $(ls "$ISODIR/modules"); do
	name=$(echo $f | cut -d. -f1)
	echo "    module /modules/$f" $name >> $grub_config
done

echo "}" >> $grub_config
