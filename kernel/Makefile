CFLAGS?=-g
CPPFLAGS?=
LDFLAGS?=-T linker.ld
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -fbuiltin -Wall -Wextra -Wno-format
CPPFLAGS:=$(CPPFLAGS) -D_KERNEL_ -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

OBJS:=$(patsubst %.c,%.o,$(wildcard src/*/*/*.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard src/*/*.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard src/*.c))

OBJS+=$(patsubst %.S,%.o,$(wildcard src/*/*/*.S))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*/*.S))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

all: SnowflakeOS.kernel

.PHONY: all clean install install-headers install-kernel

SnowflakeOS.kernel: $(OBJS) linker.ld
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@ -include stddef.h -std=gnu11 $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f SnowflakeOS.kernel $(OBJS) *.o */*.o */*/*.o

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install-kernel: SnowflakeOS.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp SnowflakeOS.kernel $(DESTDIR)$(BOOTDIR)
