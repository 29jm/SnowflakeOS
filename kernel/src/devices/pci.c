#include <kernel/ahci.h>
#include <kernel/com.h>
#include <kernel/pci.h>
#include <kernel/sys.h>

#include <list.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CFG_ADDR 0xCF8
#define CFG_DATA 0xCFC

#define CFG_ENABLE ((uint32_t) 1 << 31)

#define HEADER_MULTIFUNC (1 << 7)

static list_t pci_devices;
static uint32_t next_dev_id = 0;

uint16_t pci_read_config_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t addr = (offset & ~3) | func << 8 | dev << 11 | bus << 16 | CFG_ENABLE;

    outportl(CFG_ADDR, addr);
    uint32_t out = inportl(CFG_DATA);

    /* In this case, we want the second word */
    if (offset & 2) {
        return out >> 16;
    }

    return (uint16_t) out;
}

uint32_t pci_read_config_long(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t addr = (offset & ~3) | func << 8 | dev << 11 | bus << 16 | CFG_ENABLE;

    outportl(CFG_ADDR, addr);
    uint32_t out = inportl(CFG_DATA);

    return out;
}
void pci_write_config_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t data) {
    uint32_t addr = (offset & ~3) | func << 8 | dev << 11 | bus << 16 | CFG_ENABLE;

    uint32_t tmp = pci_read_config_long(bus, dev, func, offset);

    if (offset & 2) {
        tmp &= 0x0000FFFF;
        tmp |= (data << 16);
    } else {
        tmp &= 0xFFFF0000;
        tmp |= data;
    }

    outportl(CFG_ADDR, addr);
    outportl(CFG_DATA, tmp);
}

void pci_write_config_long(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data) {
    uint32_t addr = (offset & ~3) | func << 8 | dev << 11 | bus << 16 | CFG_ENABLE;

    outportl(CFG_ADDR, addr);
    outportl(CFG_DATA, data);
}

bool pci_bus_has_device(uint8_t bus, uint8_t dev) {
    return pci_read_config_word(bus, dev, 0, 0) != 0xFFFF;
}

bool pci_device_has_function(uint8_t bus, uint8_t dev, uint8_t func) {
    return pci_read_config_word(bus, dev, func, 0) != 0xFFFF;
}

uint8_t pci_header_type(uint8_t bus, uint8_t dev) {
    return pci_read_config_word(bus, dev, 0, 14) & 0x00FF;
}

void pci_print_device(pci_device_t* dev) {
    uint8_t type = dev->hdr.header_type & ~HEADER_MULTIFUNC;

    /* Print information common to all devices */
    printk("id: %d, location: %x:%x:%x, device %x:%x, class %02x:%02x:%02x", dev->id, dev->bus, dev->dev,
        dev->func, dev->hdr.vendor, dev->hdr.device, dev->hdr.class, dev->hdr.subclass, dev->hdr.interface);

    /* Print device-type-specific information */
    switch (type) {
    case 0x00:
        printk(" - interrupt pin: %d, interrupt line: %d, header type: %d", dev->header0.int_pin,
            dev->header0.int_line, dev->hdr_type);

        for (uint32_t i = 0; i < 6; i++) {
            if (dev->header0.bar[i]) {
                printk("   BAR%d: 0x%p", i, dev->header0.bar[i]);
            }
        }

        break;
    }
}

void pci_print_all_devices() {
    pci_device_t* dev;
    list_for_each_entry(dev, &pci_devices) {
        pci_print_device(dev);
    }
}

pci_device_t* pci_register_device(uint32_t bus, uint32_t dev, uint32_t func) {
    if (next_dev_id >= PCI_MAX_NUM_DEVS) {
        printke("Trying to register a PCI device with no spots left");
        return NULL;
    }

    pci_device_t* device = (pci_device_t*) kmalloc(sizeof(pci_device_t));
    if (!device) {
        printke("unable to allocate space for a PCI device bus: %d, dev: %d, func: %d", bus, dev, func);
        return NULL;
    }

    device->id = next_dev_id;
    next_dev_id++;

    device->bus = bus;
    device->dev = dev;
    device->func = func;

    device->hdr_type = pci_header_type(bus, dev) & ~HEADER_MULTIFUNC;

    uint32_t size;
    switch (device->hdr_type) {
    case 0x00:
        size = sizeof(device->hdr) + sizeof(device->header0);
        pci_read_config(bus, dev, func, (uint8_t*) &device->hdr, size);
        break;
    case 0x01:
        size = sizeof(device->hdr) + sizeof(device->header1);
        pci_read_config(bus, dev, func, (uint8_t*) &device->hdr, size);
        break;
    case 0x02:
        size = sizeof(device->hdr) + sizeof(device->header2);
        pci_read_config(bus, dev, func, (uint8_t*) &device->hdr, size);
        break;
    default:
        printke("error with pci header type. Value provided: %d", device->hdr_type);
        return NULL;
    }

    if (!list_add(&pci_devices, device)) {
        printke("unable to add pci device to list");
        kfree(device);
        return NULL;
    }

    /* AHCI controller */
    if (device->hdr.class == 0x01 && device->hdr.subclass == 0x06 && device->hdr.interface == 0x01) {
        ahci_add_controller(device);
    }

    return device;
}

void pci_enumerate_devices() {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t dev = 0; dev < 32; dev++) {
            if (!pci_bus_has_device(bus, dev)) {
                continue;
            }

            uint8_t header_type = pci_header_type(bus, dev);

            if (header_type & HEADER_MULTIFUNC) {
                for (uint32_t func = 0; func < 8; func++) {
                    if (!pci_device_has_function(bus, dev, func)) {
                        continue;
                    }

                    if (!pci_register_device(bus, dev, func))
                        printke("error registering device bus: %d, dev: %d, func: %d", bus, dev, func);
                }
            } else {
                if (!pci_register_device(bus, dev, 0))
                    printke("error registering device bus: %d, dev: %d, func: %d", bus, dev, 0);
            }
        }
    }
}

void pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i += 2) {
        uint16_t word = pci_read_config_word(bus, dev, func, i);
        buf[i + 0] = (uint8_t) (word & 0x00FF);
        buf[i + 1] = (uint8_t) (word >> 8);
    }
}

void init_pci() {
    printk("Initializing PCI");
    pci_devices = LIST_HEAD_INIT(pci_devices);
    pci_enumerate_devices();
    pci_print_all_devices();
}
