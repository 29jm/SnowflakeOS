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

PATH:=$(PATH):/sbin:$(PWD)/toolchain/compiler/bin

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

ASSETS_IMAGE=$(notdir $(shell find assets/used -name '*.png'))
ASSETS_IMAGE:=$(ASSETS_IMAGE:%=$(TARGETROOT)/%)
ASSETS_IMAGE:=$(patsubst %.png,%.rgb,$(ASSETS_IMAGE))

ASSETS_OTHER=$(shell find assets/used -type f ! -name '*.png')
ASSETS_OTHER:=$(notdir $(ASSETS_OTHER))
ASSETS_OTHER:=$(ASSETS_OTHER:%=$(TARGETROOT)/%)

DISKIMAGE=$(ISODIR)/modules/disk.img
GRUBCFG=$(ISODIR)/boot/grub/grub.cfg

.PHONY: all build qemu bochs clean toolchain assets

all: build SnowflakeOS.iso

strict: CFLAGS += -Werror
strict: build

build: $(PROJECTS)

# Copy headers before building anything
$(PROJECTS): $(PROJECT_HEADERS) | $(TARGETROOT) $(LIBDIR)
	@$(MAKE) -C $@ build

# Specify dependencies
kernel: libc
modules: libc snow ui
doomgeneric: libc snow ui

qemu: SnowflakeOS.iso
	qemu-system-x86_64  \
			   -display gtk \
	                   -drive file=SnowflakeOS.iso,id=disk,if=none \
	                   -drive file=drive.img,id=test,if=none \
			   -device ahci,id=ahci \
			   -device ide-hd,drive=disk,bus=ahci.0,model=HOSTDEVICE,serial=10101010101 \
			   -device ide-hd,drive=test,bus=ahci.1,model=TESTDRIVE,serial=6969696969696 \
			   -monitor stdio \
			   -s -no-reboot -no-shutdown -serial file:serial.log
	cat serial.log

bochs: SnowflakeOS.iso
	bochs -q -rc .bochsrc_cmds
	cat serial.log

clean: $(PROJECT_CLEAN)
	@rm -rf $(SYSROOTDIR)
	@rm -rf $(TARGETROOT)
	@rm -rf $(ISODIR)
	@rm -f SnowflakeOS.iso
	@rm -f misc/grub.cfg
	@rm -f misc/disk.img

SnowflakeOS.iso: $(PROJECTS) $(GRUBCFG)
	$(info [all] writing $@)
	@grub-mkrescue -o SnowflakeOS.iso $(ISODIR) 2> /dev/null

assets: $(ASSETS_IMAGE) $(ASSETS_OTHER)
	$(info [all] building assets)

$(ASSETS_IMAGE) $(ASSETS_OTHER): | $(TARGETROOT)

$(ASSETS_IMAGE): $(TARGETROOT)/%.rgb : assets/used/%.png
	@convert $< $@

$(ASSETS_OTHER): $(TARGETROOT)/% : assets/used/%
	@cp $< $@

$(GRUBCFG): $(DISKIMAGE)
	$(info [all] generating grub config)
	@mkdir -p $(ISODIR)/boot/grub
	@bash ./misc/gen-grub-config.sh

$(DISKIMAGE): modules doomgeneric $(ASSETS_IMAGE) $(ASSETS_OTHER)
	$(info [all] writing disk image)
	@mkdir -p $(ISODIR)/modules
	@touch $(DISKIMAGE)
	@dd if=/dev/zero of=$(DISKIMAGE) bs=1024 count=12000 2> /dev/null
	@mkdir -p $(TARGETROOT)/{etc,mnt}
	@echo "hello ext2 world" > $(TARGETROOT)/motd
	@echo "version: 0.7" > $(TARGETROOT)/etc/config
	@mkfs.ext2 $(DISKIMAGE) -d $(TARGETROOT) > /dev/null 2>&1

toolchain:
	@env -i toolchain/build-toolchain.sh

$(TARGETROOT):
	@mkdir -p $(TARGETROOT)

$(INCLUDEDIR):
	@mkdir -p $(INCLUDEDIR)

$(LIBDIR):
	@mkdir -p $(LIBDIR)


# Automatic rules for our generated sub-targets
%.headers: %/ | $(INCLUDEDIR)
	@$(MAKE) -C $< install-headers

%.clean: %/
	@$(MAKE) -C $< clean
