LIBK_CFLAGS:=$(CFLAGS) -D_KERNEL_
CFLAGS+=-Wno-format

OBJS+=$(patsubst %.c,%.o,$(shell find src/ -name '*.c'))
OBJS+=$(patsubst %.S,%.o,$(shell find src/ -name '*.S'))

LIBK_OBJS:=$(OBJS:.o=.libk.o)

# libk is libc but compiled with _KERNEL_ defined
LIBC=$(LIBDIR)/libc.a
LIBK=$(LIBDIR)/libk.a

.PHONY: all clean build install-headers

$(LIBC): $(OBJS)
	$(info [libc] linking $(notdir $@))
	@mkdir -p $(LIBDIR)
	@$(AR) rcs $@ $(OBJS)

$(LIBK): $(LIBK_OBJS)
	$(info [libc] linking $(notdir $@))
	@mkdir -p $(LIBDIR)
	@$(AR) rcs $@ $(LIBK_OBJS)

%.o: %.c
	$(info [libc] $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [libc] $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.libk.o: %.c
	$(info [libc] $@)
	@$(CC) -c $< -o $@ $(LIBK_CFLAGS)

%.libk.o: %.S
	$(info [libc] $@)
	@$(CC) -c $< -o $@ $(LIBK_CFLAGS)

clean:
	$(info [libc] cleaning up)
	@rm -f $(OBJS) $(LIBK_OBJS) *.o */*.o */*/*.o

build: $(LIBC) $(LIBK)

install-headers:
	$(info [libc] installing headers)
	@mkdir -p $(INCLUDEDIR)
	@cp -rT include $(INCLUDEDIR)