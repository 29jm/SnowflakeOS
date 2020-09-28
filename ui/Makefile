OBJS=$(patsubst %.c,%.o,$(wildcard src/*.c))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

all: libui.a

.PHONY: all clean build install-headers install-libs

libui.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f *.a $(OBJS)

build: install-libs

install-headers:
	mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	cp -rT include $(SYSROOT)/$(INCLUDEDIR)

install-libs: libui.a
	mkdir -p $(SYSROOT)/$(LIBDIR)
	cp libui.a $(SYSROOT)/$(LIBDIR)
