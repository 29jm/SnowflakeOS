export # Makes variables from this file available in sub-makefiles

LC_ALL=C

HOST=i686-elf
PREFIX=usr
BOOTDIR=boot
LIBDIR=$(PREFIX)/lib
INCLUDEDIR=$(PREFIX)/include
ISODIR=isodir
ISO=$(PWD)/$(ISODIR)
TARGETROOT=$(PWD)/misc/root
SYSROOTDIR=sysroot
SYSROOT=$(PWD)/$(SYSROOTDIR)

PATH:=$(PATH):$(PWD)/toolchain/compiler/bin

MAKE:=$(MAKE) -s
LD=$(HOST)-ld
AR=$(HOST)-ar
AS=$(HOST)-as
CC=$(HOST)-gcc

CFLAGS=-g -std=gnu11 -ffreestanding -Wall -Wextra
LDFLAGS=-nostdlib -L$(SYSROOT)/usr/lib

ifeq ($(UBSAN),1)
	CFLAGS+=-fsanitize=undefined
endif

# Uncomment the following group of lines to compile with the system's
# clang installation

# CC=clang
# CFLAGS+=-target i386-pc-none-eabi -m32
# CFLAGS+=-mno-mmx -mno-sse -mno-sse2

CC+=--sysroot=$(SYSROOT) -isystem=/$(INCLUDEDIR)

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
	$(info [$@] building)
	@$(MAKE) -C $@ build

# Specify dependencies
kernel: libc
snow: libc
ui: libc snow
modules: libc snow ui

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
	@grub-mkrescue -o SnowflakeOS.iso $(ISODIR) &> /dev/null

assets: assets/pisos_16.png assets/wallpaper.png
	@convert assets/pisos_16.png misc/pisos_16.rgb
	@convert assets/wallpaper.png misc/wallpaper.rgb

# The dependency on disk stuff is temporary
misc/grub.cfg: build misc/disk.img misc/gen-grub-config.sh
	$(info [all] generating grub config)
	@cp misc/disk.img $(ISODIR)/modules/disk.img
	@bash ./misc/gen-grub-config.sh

misc/disk.img: assets modules
	$(info [all] writing disk image)
	@touch misc/disk.img
	@dd if=/dev/zero of=misc/disk.img bs=1024 count=10240 &> /dev/null
	@mkdir -p misc/root/etc
	@echo "hello ext2 world" > misc/root/motd
	@echo "version: 0.5" > misc/root/etc/config
	@mv misc/*.rgb misc/root/
	@mkfs.ext2 misc/disk.img -d misc/root &> /dev/null
	@rm -r misc/root

toolchain:
	@env -i toolchain/build-toolchain.sh

# Automatic rules for our generated sub-targets
%.headers: %/
	@$(MAKE) -C $< install-headers

%.clean: %/
	@$(MAKE) -C $< clean
