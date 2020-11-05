OBJS=$(patsubst %.c,%.o,$(wildcard src/*.c))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

all: libui.a

.PHONY: all clean build install-headers install-libs

libui.a: $(OBJS)
	$(info [ui] linking $@)
	@$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(info [ui] building $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	$(info [ui] building $@)
	@rm -f *.a $(OBJS)

build: install-libs

install-headers:
	$(info [ui] installing headers)
	@mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	@cp -rT include $(SYSROOT)/$(INCLUDEDIR)

install-libs: libui.a
	$(info [ui] installing shared libraries)
	@mkdir -p $(SYSROOT)/$(LIBDIR)
	@cp libui.a $(SYSROOT)/$(LIBDIR)
