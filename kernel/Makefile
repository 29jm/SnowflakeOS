CFLAGS:=$(CFLAGS) -Wno-format
LDFLAGS:=$(LDFLAGS) -T linker.ld
LIBS=-lk -lgcc

OBJS:=$(patsubst %.c,%.o,$(wildcard src/*/*/*.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard src/*/*.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard src/*.c))

OBJS+=$(patsubst %.S,%.o,$(wildcard src/*/*/*.S))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*/*.S))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

.PHONY: all clean install install-headers install-kernel

SnowflakeOS.kernel: $(OBJS) linker.ld
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@ -include stddef.h $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

install: SnowflakeOS.kernel
	mkdir -p $(SYSROOT)/$(BOOTDIR)
	cp SnowflakeOS.kernel $(SYSROOT)/$(BOOTDIR)

install-headers:
	mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	cp -RTv include $(SYSROOT)/$(INCLUDEDIR)

clean:
	rm -f SnowflakeOS.kernel $(OBJS) *.o */*.o */*/*.o