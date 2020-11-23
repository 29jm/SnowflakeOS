CFLAGS:=$(CFLAGS)
LDFLAGS:=$(LDFLAGS) -Tmod.ld
LIBS=-lui -lsnow -lc

LIB_DEPS=$(LIBDIR)/libc.a $(LIBDIR)/libui.a $(LIBDIR)/libsnow.a

MODS=$(patsubst %.c,%,$(wildcard src/*.c))
MODS:=$(notdir $(MODS))
MODS:=$(MODS:%=$(TARGETROOT)/%)
EXECUTABLES=$(notdir $(MODS))
OBJS=$(EXECUTABLES:%=src/%.o)

.PHONY: build install-headers clean

build: $(MODS)

install-headers:

clean:
	$(info [modules] cleaning up)
	@rm -f */*.o
	@find . -executable -type f -delete

# TODO: every module depends on every other, a change in one rebuilds all of them
$(MODS): $(OBJS) $(LIB_DEPS) src/start.o
	$(info [modules] $(notdir $@))
	@mkdir -p $(TARGETROOT)
	@$(LD) src/start.o src/$(@F).o -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [modules] $@)
	@$(AS) $(ASFLAGS) $< -o $@