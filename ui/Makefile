OBJS=$(patsubst %.c,%.o,$(wildcard src/*.c))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

LIBUI=$(LIBDIR)/libui.a

.PHONY: all clean build install-headers

$(LIBUI): $(OBJS)
	$(info [ui] linking $(notdir $@))
	@mkdir -p $(LIBDIR)
	@$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(info [ui] $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	$(info [ui] $@)
	@rm -f *.a $(OBJS)

build: $(LIBUI)

install-headers:
	$(info [ui] installing headers)
	@cp -rT include $(INCLUDEDIR)