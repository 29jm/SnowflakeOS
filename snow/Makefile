OBJS=$(patsubst %.c,%.o,$(wildcard src/*.c))
OBJS+=$(patsubst %.S,%.o,$(wildcard src/*.S))

LIBSNOW=$(LIBDIR)/libsnow.a

.PHONY: all clean build install-headers

$(LIBSNOW): $(OBJS)
	$(info [snow] linking $(notdir $@))
	@$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(info [snow] $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(info [snow] $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	$(info [snow] $@)
	@rm -f *.a $(OBJS)

build: $(LIBSNOW)

install-headers:
	$(info [snow] installing headers)
	@cp -rT include $(INCLUDEDIR)