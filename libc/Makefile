LIBK_CFLAGS:=$(CFLAGS) -D_KERNEL_ -Wno-format
CFLAGS+=-Wno-format

OBJS:=$(patsubst %.c,%.o,$(wildcard src/*/*.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard src/*.c))


OBJS+=$(patsubst %.S,%.o,$(wildcard src/*/*.S))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

LIBK_OBJS:=$(OBJS:.o=.libk.o)

# libk is libc but compiled with _KERNEL_ defined
BINARIES=libc.a libk.a

all: $(BINARIES)

.PHONY: all clean build install-headers install-libs

libc.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

libk.a: $(LIBK_OBJS)
	$(AR) rcs $@ $(LIBK_OBJS)

%.o: %.c
	$(CC) -c $< -o $@ -include stddef.h $(CFLAGS)

%.o: %.S
	$(CC) -c $< -o $@ $(CFLAGS)

%.libk.o: %.c
	$(CC) -c $< -o $@ -std=gnu11 $(LIBK_CFLAGS)

%.libk.o: %.S
	$(CC) -c $< -o $@ $(LIBK_CFLAGS)

clean:
	rm -f $(BINARIES) $(OBJS) $(LIBK_OBJS) *.o */*.o */*/*.o

build: install-libs

install-headers:
	mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	cp -rT include $(SYSROOT)/$(INCLUDEDIR)

install-libs: $(BINARIES)
	mkdir -p $(SYSROOT)/$(LIBDIR)
	cp $(BINARIES) $(SYSROOT)/$(LIBDIR)
