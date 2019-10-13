#!/usr/bin/bash

# Expects to be run from the main Makefile
grub_config=misc/grub.cfg

echo "menuentry \"SnowflakeOS - Challenge Edition\" {" > $grub_config
echo "    multiboot /boot/SnowflakeOS.kernel" >> $grub_config

for f in $(ls "$ISODIR/modules"); do
    echo "    module /modules/$f" >> $grub_config
done

echo "}" >> $grub_config