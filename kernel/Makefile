CFLAGS:=$(CFLAGS) -D_KERNEL_
LDFLAGS:=$(LDFLAGS) -T linker.ld
LIBS=-lk -lgcc

OBJS:=$(patsubst %.c,%.o,$(wildcard src/*/*/*.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard src/*/*.c))
OBJS+=$(patsubst %.c,%.o,$(wildcard src/*.c))

OBJS+=$(patsubst %.S,%.o,$(wildcard src/*/*/*.S))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*/*.S))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

.PHONY: all clean build install-headers install-kernel

SnowflakeOS.kernel: $(OBJS) linker.ld
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(CC) -c $< -o $@ $(CFLAGS)

build: SnowflakeOS.kernel
	mkdir -p $(ISO)/$(BOOTDIR)
	cp SnowflakeOS.kernel $(ISO)/$(BOOTDIR)

install-headers:
	mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	cp -rT include $(SYSROOT)/$(INCLUDEDIR)

clean:
	rm -f SnowflakeOS.kernel $(OBJS) *.o */*.o */*/*.o
