CFLAGS:=$(CFLAGS) -D_KERNEL_
LDFLAGS:=$(LDFLAGS) -T linker.ld -Map=linker.map
LIBS=-lk

OBJS:=$(patsubst %.c,%.o,$(shell find src -name '*.c'))
OBJS+=$(patsubst %.S,%.o,$(shell find src -name '*.S'))

LIBK_DEP=$(LIBDIR)/libk.a

KERNEL=$(ISO)/$(BOOTDIR)/SnowflakeOS.kernel
SYMBOLS=$(ISO)/modules/symbols.map

.PHONY: all clean build install-headers install-kernel

$(KERNEL) $(SYMBOLS): $(OBJS) $(LIBK_DEP) linker.ld
	$(info [kernel] linking)
	@mkdir -p $(ISO)/$(BOOTDIR)
	@$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS) $(LIBS)
	$(info [kernel] generating symbol table)
	@mkdir -p $(ISO)/modules
	@awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > $(SYMBOLS)
	@rm linker.map

%.o: %.c
	$(info [kernel] $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [kernel] $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

build: $(KERNEL)

install-headers:
	$(info [kernel] installing headers)
	@mkdir -p $(INCLUDEDIR)
	@cp -rT include $(INCLUDEDIR)

clean:
	$(info [kernel] cleaning)
	@rm -f SnowflakeOS.kernel $(OBJS) *.o */*.o */*/*.o *.map