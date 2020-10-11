CFLAGS:=$(CFLAGS) -nostartfiles
LDFLAGS:=$(LDFLAGS) -Tmod.ld
LIBS=-lui -lsnow -lc

MODS=$(patsubst %.c,%.mod,$(wildcard src/*.c))

.PHONY: build install-headers clean

build: $(MODS)
	mkdir -p $(ISO)/modules
	cp $(MODS) $(ISO)/modules

install-headers:

clean:
	rm -fv */*.o */*.mod

%.mod: %.c mod.ld src/start.o FORCE
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)

%.o: %.S
	$(AS) $< -o $@

FORCE: