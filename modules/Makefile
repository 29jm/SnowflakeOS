CFLAGS:=$(CFLAGS) -nostartfiles
LDFLAGS:=$(LDFLAGS) -T mod.ld
LIBS=-lui -lsnow -lc -lgcc

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

FORCE: