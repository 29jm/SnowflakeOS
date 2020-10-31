CFLAGS:=$(CFLAGS) -D_KERNEL_
LDFLAGS:=$(LDFLAGS) -T linker.ld -Map=linker.map
LIBS=-lk

OBJS:=$(patsubst %.c,%.o,$(shell find src -name '*.c'))
OBJS+=$(patsubst %.S,%.o,$(shell find src -name '*.S'))

.PHONY: all clean build install-headers install-kernel

SnowflakeOS.kernel symbols.map: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > symbols.map
	mkdir -p $(ISO)/modules
	cp symbols.map $(ISO)/modules/
	rm linker.map

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
	rm -f SnowflakeOS.kernel $(OBJS) *.o */*.o */*/*.o *.map