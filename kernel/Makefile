CFLAGS:=$(CFLAGS) -D_KERNEL_
LDFLAGS:=$(LDFLAGS) -T linker.ld
LIBS=-lk

OBJS:=$(patsubst %.c,%.o,$(shell find src -name '*.c'))
OBJS+=$(patsubst %.S,%.o,$(shell find src -name '*.S'))

.PHONY: all clean build install-headers install-kernel

SnowflakeOS.kernel: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

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
