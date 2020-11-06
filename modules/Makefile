CFLAGS:=$(CFLAGS) -nostartfiles
LDFLAGS:=$(LDFLAGS) -Tmod.ld
LIBS=-lui -lsnow -lc

MODS=$(patsubst %.c,%,$(wildcard src/*.c))
MODS_BUILD=$(patsubst %.c,%.mod,$(wildcard src/*.c))

.PHONY: build install-headers clean

build: $(MODS)
	$(info [modules] copying modules)
	@mkdir -p $(TARGETROOT)
	@cp $(MODS) $(TARGETROOT)

install-headers:

clean:
	$(info [modules] cleaning up)
	@rm -fv */*.o
	@find . -executable -type f -delete

$(MODS): % : %.mod
	$(info [modules] building $@)
	mv $^ $@

%.mod: %.c mod.ld src/start.o FORCE
	@$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)

%.o: %.S
	$(info [modules] building $@)
	@$(AS) $< -o $@

FORCE: