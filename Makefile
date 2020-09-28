export # Makes variables from this file available in sub-makefiles

HOST=i686-elf
PREFIX=usr
BOOTDIR=boot
LIBDIR=$(PREFIX)/lib
INCLUDEDIR=$(PREFIX)/include
ISODIR=isodir
ISO=$(PWD)/$(ISODIR)
SYSROOTDIR=sysroot
SYSROOT=$(PWD)/$(SYSROOTDIR)

PATH:=$(PATH):$(PWD)/toolchain/compiler/bin

AR=$(HOST)-ar
AS=$(HOST)-as
LD=$(HOST)-ld
CC=$(HOST)-gcc --sysroot=$(SYSROOT) -isystem=/$(INCLUDEDIR)

CFLAGS=-g -std=gnu11 -ffreestanding -Wall -Wextra -Wno-format
LDFLAGS=-nostdlib

# Make will be called on these folders
PROJECTS=libc snow kernel modules ui

# Generate project sub-targets
PROJECT_HEADERS=$(PROJECTS:=.headers) # appends .headers to every project name
PROJECT_CLEAN=$(PROJECTS:=.clean)

.PHONY: all build qemu bochs clean toolchain $(PROJECTS)

all: build SnowflakeOS.iso

strict: CFLAGS += -Werror
strict: build

build: $(PROJECTS)

# Copy headers before building anything
$(PROJECTS): $(PROJECT_HEADERS)
	$(MAKE) -C $@ build

# Specify dependencies
kernel: libc
snow: libc
ui: libc snow
modules: libc snow ui modules/src/bg.h # TODO: cleanup

qemu: SnowflakeOS.iso
	qemu-system-x86_64 -display sdl -cdrom SnowflakeOS.iso -monitor stdio -s -no-reboot -no-shutdown -serial file:serial.log
	cat serial.log

bochs: SnowflakeOS.iso
	bochs -q -rc .bochsrc_cmds
	cat serial.log

clean: $(PROJECT_CLEAN)
	rm -rf $(SYSROOTDIR)
	rm -rf $(ISODIR)
	rm -f SnowflakeOS.iso
	rm -f misc/grub.cfg
	rm -f misc/disk.img
	rm -f misc/*.rgb
	rm -f xbochs.log
	rm -f irq.log

SnowflakeOS.iso: build misc/grub.cfg misc/disk.img
	mkdir -p $(ISODIR)/boot/grub
	cp misc/grub.cfg $(ISODIR)/boot/grub
	grub-mkrescue -o SnowflakeOS.iso $(ISODIR)

modules/src/bg.h: assets/wallpaper.png
	convert assets/wallpaper.png misc/wallpaper.rgb
	xxd -i misc/wallpaper.rgb > modules/src/bg.h
	rm misc/wallpaper.rgb

misc/pisos_16.rgb: assets/pisos_16.png
	convert assets/pisos_16.png misc/pisos_16.rgb

misc/wallpaper.rgb: assets/wallpaper.png
	convert assets/wallpaper.png misc/wallpaper.rgb

# The dependency on disk stuff is temporary
misc/grub.cfg: build misc/disk.img misc/gen-grub-config.sh
	cp misc/disk.img $(ISODIR)/modules/disk.img
	bash ./misc/gen-grub-config.sh

misc/disk.img: misc/pisos_16.rgb misc/wallpaper.rgb
	touch misc/disk.img
	dd if=/dev/zero of=misc/disk.img bs=1024 count=2500
	mkdir -p misc/root/etc
	echo "hello ext2 world" > misc/root/motd
	echo "version: 0.5" > misc/root/etc/config
	mv misc/pisos_16.rgb misc/root/pisos_16.rgb
	mv misc/wallpaper.rgb misc/root/wallpaper.rgb
	mkfs.ext2 misc/disk.img -d misc/root
	rm -r misc/root

toolchain:
	env -i toolchain/build-toolchain.sh

# Automatic rules for our generated sub-targets
%.headers: %/
	$(MAKE) -C $< install-headers

%.clean: %/
	$(MAKE) -C $< clean
