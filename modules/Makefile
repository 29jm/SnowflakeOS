CFLAGS:=$(CFLAGS) -nostartfiles
LDFLAGS:=$(LDFLAGS) -Tmod.ld
LIBS=-lui -lsnow -lc

MODS=$(patsubst %.c,%.mod,$(wildcard src/*.c))

.PHONY: build install-headers clean

build: $(MODS)
	$(info [modules] copying modules)
	@mkdir -p $(TARGETROOT)/bin
	@cp $(MODS) $(TARGETROOT)/bin

install-headers:

clean:
	$(info [modules] cleaning up)
	@rm -fv */*.o */*.mod

%.mod: %.c mod.ld src/start.o FORCE
	$(info [modules] building $@)
	@$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)

%.o: %.S
	$(info [modules] building $@)
	@$(AS) $< -o $@

FORCE: