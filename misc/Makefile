.SILENT:
AS=i686-elf-as

SOURCES = $(wildcard *.S)
OBJS = $(SOURCES:.S=.out)
FINAL = $(OBJS:.out=.bin)

%.out: %.S
	$(AS) $< -o $@

%.bin: %.out
	objcopy -O binary --only-section=.text $< $@
	xxd -i $@
	echo ""

all: clean $(FINAL)

.PHONY: clean
clean:
	rm -rf *.out *.bin
