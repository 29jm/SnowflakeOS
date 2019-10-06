CFLAGS:=$(CFLAGS) -nostartfiles
LDFLAGS:=$(LDFLAGS) -T mod.ld
LIBS=-lc -lgcc

MODS=$(patsubst %.c,%.mod,$(wildcard src/*.c))

install: $(MODS)
	mkdir -p $(SYSROOT)/modules
	cp $(MODS) $(SYSROOT)/modules

install-headers:

clean:
	rm -fv */*.o */*.mod

%.mod: %.c mod.ld src/start.S
	$(CC) $< src/start.S -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)