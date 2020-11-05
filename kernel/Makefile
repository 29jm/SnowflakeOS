CFLAGS:=$(CFLAGS) -D_KERNEL_
LDFLAGS:=$(LDFLAGS) -T linker.ld -Map=linker.map
LIBS=-lk

OBJS:=$(patsubst %.c,%.o,$(shell find src -name '*.c'))
OBJS+=$(patsubst %.S,%.o,$(shell find src -name '*.S'))

.PHONY: all clean build install-headers install-kernel

SnowflakeOS.kernel symbols.map: $(OBJS) linker.ld
	$(info [kernel] linking kernel)
	@$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(info [kernel] generating symbol table)
	@awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > symbols.map
	@mkdir -p $(ISO)/modules
	@cp symbols.map $(ISO)/modules/
	@rm linker.map

%.o: %.c
	$(info [kernel] building $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [kernel] building $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

build: SnowflakeOS.kernel
	$(info [kernel] installing)
	@mkdir -p $(ISO)/$(BOOTDIR)
	@cp SnowflakeOS.kernel $(ISO)/$(BOOTDIR)

install-headers:
	$(info [kernel] installing headers)
	@mkdir -p $(SYSROOT)/$(INCLUDEDIR)
	@cp -rT include $(SYSROOT)/$(INCLUDEDIR)

clean:
	$(info [kernel] cleaning)
	@rm -f SnowflakeOS.kernel $(OBJS) *.o */*.o */*/*.o *.map