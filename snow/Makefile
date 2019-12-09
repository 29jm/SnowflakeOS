OBJS=$(patsubst %.c,%.o,$(wildcard src/*.c))

all: libsnow.a

.PHONY: all clean build install-headers install-libs

libsnow.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@ -include stddef.h $(CFLAGS)

clean:
	rm -f *.a $(OBJS)

build: install-libs

install-headers:
	mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	cp -rT include $(SYSROOT)/$(INCLUDEDIR)

install-libs: libsnow.a
	mkdir -p $(SYSROOT)/$(LIBDIR)
	cp libsnow.a $(SYSROOT)/$(LIBDIR)