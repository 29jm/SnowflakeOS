#include <kernel/ide.h>

void init_ide(pci_device_t dev) {
    pci_header0_t hdr;

    pci_read_config(dev.bus, dev.dev, dev.func, (uint8_t*) &hdr, sizeof(hdr));
}