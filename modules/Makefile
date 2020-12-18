CFLAGS:=$(CFLAGS)
LDFLAGS:=$(LDFLAGS) -Tmod.ld
LIBS=-lui -lsnow -lc

LIB_DEPS=$(LIBDIR)/libc.a $(LIBDIR)/libui.a $(LIBDIR)/libsnow.a

MODS=$(patsubst %.c,%,$(wildcard src/*.c))
MODS:=$(notdir $(MODS))
MODS:=$(addprefix $(TARGETROOT)/,$(MODS))
EXECUTABLES=$(notdir $(MODS))
OBJS=$(EXECUTABLES:%=src/%.o)

.PHONY: build install-headers clean

build: $(MODS)

install-headers:

clean:
	$(info [modules] $@)
	@rm -f */*.o

$(MODS): $(TARGETROOT)/% : src/%.o src/start.o
	@$(LD) src/start.o $< -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(info [modules] $(notdir $(basename $@)))
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [modules] $@)
	@$(AS) $(ASFLAGS) $< -o $@