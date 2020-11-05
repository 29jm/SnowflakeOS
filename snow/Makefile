OBJS=$(patsubst %.c,%.o,$(wildcard src/*.c))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

all: libsnow.a

.PHONY: all clean build install-headers install-libs

libsnow.a: $(OBJS)
	$(info [snow] linking $@)
	@$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(info [snow] building $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [snow] building $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	$(info [snow] cleaning up)
	@rm -f *.a $(OBJS)

build: install-libs

install-headers:
	$(info [snow] installing headers)
	@mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	@cp -rT include $(SYSROOT)/$(INCLUDEDIR)

install-libs: libsnow.a
	$(info [snow] installing shared libraries)
	@mkdir -p $(SYSROOT)/$(LIBDIR)
	@cp libsnow.a $(SYSROOT)/$(LIBDIR)
