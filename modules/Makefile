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

$(MODS): $(OBJS) $(LIB_DEPS) src/start.o
	@mkdir -p $(TARGETROOT)
	@$(LD) src/start.o src/$(@F).o -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(info [modules] $(notdir $(basename $@)))
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [modules] $@)
	@$(AS) $(ASFLAGS) $< -o $@