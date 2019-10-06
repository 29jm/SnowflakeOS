export

HOST=i686-elf
PREFIX=usr
BOOTDIR=boot
LIBDIR=$(PREFIX)/lib
INCLUDEDIR=$(PREFIX)/include
ISODIR=isodir
SYSROOTDIR=sysroot
SYSROOT=$(PWD)/$(SYSROOTDIR)

AR=$(HOST)-ar
AS=$(HOST)-as
LD=$(HOST)-ld
CC=$(HOST)-gcc --sysroot=$(SYSROOT) -isystem=/$(INCLUDEDIR)

CFLAGS=-g -std=gnu11 -ffreestanding -fbuiltin -Wall -Wextra
LDFLAGS=-nostdlib

PROJECTS=libc kernel modules

# Generate sub-targets
PROJECT_HEADERS=$(PROJECTS:=.headers) #adds .headers to every project name
PROJECT_INSTALL=$(PROJECTS:=.install)
PROJECT_CLEAN=$(PROJECTS:=.clean)

.PHONY: all install-headers install qemu bochs clean

all: install-headers install SnowflakeOS.iso

install-headers: $(PROJECT_HEADERS)

install: $(PROJECT_INSTALL)

SnowflakeOS.iso: all
	mkdir -p $(ISODIR)/modules
	mkdir -p $(ISODIR)/boot/grub
	cp sysroot/boot/SnowflakeOS.kernel $(ISODIR)/boot
	cp sysroot/modules/* $(ISODIR)/modules
	cp grub.cfg $(ISODIR)/boot/grub
	grub-mkrescue -o SnowflakeOS.iso $(ISODIR)

qemu: SnowflakeOS.iso
	qemu-system-x86_64 -cdrom SnowflakeOS.iso -monitor stdio -s -no-reboot -no-shutdown

bochs: SnowflakeOS.iso
	bochs -q -rc .bochsrc_cmds

clean: $(PROJECT_CLEAN)
	rm -rfv $(SYSROOTDIR)
	rm -rfv $(ISODIR)
	rm -fv SnowflakeOS.iso
	rm -fv xbochs.log
	rm -fv irq.log

%.headers: %
	$(MAKE) -C $< install-headers

%.install: %
	$(MAKE) -C $< install

%.clean: %
	$(MAKE) -C $< clean