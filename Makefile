export # Makes variables from this file available in sub-makefiles

LC_ALL=C

HOST=i686-elf
PREFIX=usr
BOOTDIR=boot
ISODIR=isodir
ISO=$(PWD)/$(ISODIR)
TARGETROOT=$(PWD)/misc/root
SYSROOTDIR=sysroot
SYSROOT=$(PWD)/$(SYSROOTDIR)

INCLUDEDIR=$(SYSROOT)/$(PREFIX)/include
LIBDIR=$(SYSROOT)/$(PREFIX)/lib

PATH:=$(PATH):$(PWD)/toolchain/compiler/bin

MAKE:=$(MAKE) -s
LD=$(HOST)-ld
AR=$(HOST)-ar
AS=$(HOST)-as
CC=$(HOST)-gcc

CFLAGS=-O1 -std=gnu11 -ffreestanding -Wall -Wextra
ASFLAGS=--32
LDFLAGS=-nostdlib -L$(SYSROOT)/usr/lib -m elf_i386

ifeq ($(UBSAN),1)
	CFLAGS+=-fsanitize=undefined
endif

# Uncomment the following group of lines to compile with the system's
# clang installation

# CC=clang
# LD=ld
# AR=ar
# AS=as
# CFLAGS+=-target i386-pc-none-eabi -m32
# CFLAGS+=-mno-mmx -mno-sse -mno-sse2

CC+=--sysroot=$(SYSROOT) -isystem=/$(PREFIX)/include

# Make will be called on these folders
PROJECTS=libc snow kernel modules ui doomgeneric

# Generate project sub-targets
PROJECT_HEADERS=$(PROJECTS:=.headers) # appends .headers to every project name
PROJECT_CLEAN=$(PROJECTS:=.clean)

.PHONY: all build qemu bochs clean toolchain assets $(PROJECTS)

all: build SnowflakeOS.iso

strict: CFLAGS += -Werror
strict: build

build: $(PROJECTS)

# Copy headers before building anything
$(PROJECTS): $(PROJECT_HEADERS)
	$(info [$@] building)
	@$(MAKE) -C $@ build

# Specify dependencies
kernel: libc
modules: libc snow ui
doomgeneric: libc snow ui

qemu: SnowflakeOS.iso
	qemu-system-x86_64 -display sdl -cdrom SnowflakeOS.iso -monitor stdio -s -no-reboot -no-shutdown -serial file:serial.log
	cat serial.log

bochs: SnowflakeOS.iso
	bochs -q -rc .bochsrc_cmds
	cat serial.log

clean: $(PROJECT_CLEAN)
	@rm -rf $(SYSROOTDIR)
	@rm -rf $(ISODIR)
	@rm -f SnowflakeOS.iso
	@rm -f misc/grub.cfg
	@rm -f misc/disk.img
	@rm -f misc/*.rgb
	@rm -f xbochs.log
	@rm -f irq.log

SnowflakeOS.iso: build misc/grub.cfg misc/disk.img
	$(info [all] writing $@)
	@mkdir -p $(ISODIR)/boot/grub
	@cp misc/grub.cfg $(ISODIR)/boot/grub
	@grub-mkrescue -o SnowflakeOS.iso $(ISODIR) 2> /dev/null

assets: assets/pisos_16.png assets/wallpaper.png assets/DOOM1.WAD
	$(info [all] generating assets)
	@convert assets/pisos_16.png misc/pisos_16.rgb
	@convert assets/wallpaper.png misc/wallpaper.rgb
	@cp assets/*.WAD misc/root/

# The dependency on disk stuff is temporary
misc/grub.cfg: build misc/disk.img misc/gen-grub-config.sh misc/disk2.img
	$(info [all] generating grub config)
	@cp misc/disk.img $(ISODIR)/modules/disk.img
	@cp misc/disk2.img $(ISODIR)/modules/disk2.img
	@bash ./misc/gen-grub-config.sh

misc/disk.img: assets modules
	$(info [all] writing disk image)
	@touch misc/disk.img
	@dd if=/dev/zero of=misc/disk.img bs=1024 count=10240 2> /dev/null
	@mkdir -p misc/root/etc
	@mkdir -p misc/root/mnt
	@echo "hello ext2 world" > misc/root/motd
	@echo "version: 0.5" > misc/root/etc/config
	@mv misc/*.rgb misc/root/
	@mkfs.ext2 misc/disk.img -d misc/root > /dev/null 2>&1

misc/disk2.img: assets modules
	$(info [all] writing disk2 image)
	@touch misc/disk2.img
	@dd if=/dev/zero of=misc/disk2.img bs=1024 count=1000 2> /dev/null
	@mkfs.ext2 misc/disk2.img -d sysroot > /dev/null 2>&1

toolchain:
	@env -i toolchain/build-toolchain.sh

# Automatic rules for our generated sub-targets
%.headers: %/
	@$(MAKE) -C $< install-headers

%.clean: %/
	@$(MAKE) -C $< clean
