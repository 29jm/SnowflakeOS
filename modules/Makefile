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

$(MODS): $(TARGETROOT)/% : src/%.o src/start.o $(LIB_DEPS)
	$(info [modules] $(notdir $(basename $@)))
	@touch $(CURDIR)
	@$(LD) src/start.o $< -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	@$(AS) $(ASFLAGS) $< -o $@