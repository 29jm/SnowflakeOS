LIBK_CFLAGS:=$(CFLAGS) -D_KERNEL_
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
	$(info [libc] linking $@)
	@$(AR) rcs $@ $(OBJS)

libk.a: $(LIBK_OBJS)
	$(info [libc] linking $@)
	@$(AR) rcs $@ $(LIBK_OBJS)

%.o: %.c
	$(info [libc] building $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [libc] building $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.libk.o: %.c
	$(info [libc] building $@)
	@$(CC) -c $< -o $@ $(LIBK_CFLAGS)

%.libk.o: %.S
	$(info [libc] building $@)
	@$(CC) -c $< -o $@ $(LIBK_CFLAGS)

clean:
	$(info [libc] cleaning up)
	@rm -f $(BINARIES) $(OBJS) $(LIBK_OBJS) *.o */*.o */*/*.o

build: install-libs

install-headers:
	$(info [libc] installing headers)
	@mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	@cp -rT include $(SYSROOT)/$(INCLUDEDIR)

install-libs: $(BINARIES)
	$(info [libc] installing shared libraries)
	@mkdir -p $(SYSROOT)/$(LIBDIR)
	@cp $(BINARIES) $(SYSROOT)/$(LIBDIR)
