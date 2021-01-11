#include <kernel/com.h>
#include <kernel/pci.h>
#include <kernel/sys.h>

#include <stdbool.h>

#define CFG_ADDR 0xCF8
#define CFG_DATA 0xCFC

#define CFG_ENABLE ((uint32_t) 1 << 31)

#define HEADER_MULTIFUNC (1 << 7)

/* These are not useable right now: field must be reordered by groups of 4
 * bytes, reversed. */
typedef struct pci_header1_t {
    pci_header_t header;
    uint32_t bar[2];
    uint8_t latency2, sub_bus, sec_bus, prim_bus;
    uint16_t secondary_status;
    uint8_t io_limit, io_base;
    uint16_t memory_limit, memory_base;
    uint16_t prefetch_limit, prefetch_base;
    uint32_t prefetch_upper_limit, prefetch_upper_base;
    uint16_t io_upper_limit, io_upper_base; // TODO use io_limit* prefixes
    uint8_t reserved[3];
    uint8_t capabilities_addr;
    uint32_t expansion;
    uint16_t bridge_control;
    uint8_t int_pin, int_line;
} __attribute__((packed)) pci_header1_t;

typedef struct pci_header2_t {
    pci_header_t header;
    uint32_t carbus_addr;
    uint16_t sec_status;
    uint8_t reserved, capabilities_offset;
    uint8_t cardbus_latency, sub_bus, carbus_bus, pci_bus;
    uint32_t mem_base_addr0;
    uint32_t mem_limit0;
    uint32_t mem_base_addr1;
    uint32_t mem_limit1;
    uint32_t io_base_addr0;
    uint32_t io_limit0;
    uint32_t io_base_addr1;
    uint32_t io_limit1;
    uint16_t bridge_control;
    uint8_t int_pin, int_line;
    uint16_t sub_vendor, sub_device;
    uint32_t legacy_base_addr;
} __attribute__((packed)) pci_header2_t;

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

bool pci_bus_has_device(uint8_t bus, uint8_t dev) {
    return pci_read_config_word(bus, dev, 0, 0) != 0xFFFF;
}

bool pci_device_has_function(uint8_t bus, uint8_t dev, uint8_t func) {
    return pci_read_config_word(bus, dev, func, 0) != 0xFFFF;
}

uint8_t pci_header_type(uint8_t bus, uint8_t dev) {
    return pci_read_config_word(bus, dev, 0, 14) & 0xFFFF;
}

void pci_print_device(pci_device_t dev) {
    uint8_t type = pci_header_type(dev.bus, dev.dev) & ~HEADER_MULTIFUNC;
    pci_header_t hdr;
    pci_header0_t hdr0;
    // pci_header1_t hdr1;
    // pci_header2_t hdr2;

    /* Print information common to all devices */
    pci_read_config(dev.bus, dev.dev, dev.func, (uint8_t*) &hdr, sizeof(hdr));
    printk("%x:%x:%x: device %x:%x, class %02x:%02x:%02x",
        dev.bus, dev.dev, dev.func, hdr.vendor, hdr.device, hdr.class, hdr.subclass, hdr.interface);

    /* Print device-type-specific information */
    switch (type) {
    case 0x00:
        pci_read_config(dev.bus, dev.dev, dev.func, (uint8_t*) &hdr0, sizeof(hdr0));
        printk(" - interrupt pin: %d, interrupt line: %d", hdr0.int_pin, hdr0.int_line);

        for (uint32_t i = 0; i < 6; i++) {
            if (hdr0.bar[i]) {
                printk("   BAR%d: 0x%p", i, hdr0.bar[i]);
            }
        }

        break;
    }
}

void pci_register_device(pci_device_t dev) {
    pci_header_t hdr;

    pci_read_config(dev.bus, dev.dev, dev.func, (uint8_t*) &hdr, sizeof(hdr));

    /* IDE controller */
    if (hdr.class == 0x01 && hdr.subclass == 0x01) {

    }
}

void pci_enumerate_devices() {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t dev = 0; dev < 32; dev++) {
            if (!pci_bus_has_device(bus, dev)) {
                continue;
            }

            uint8_t header_type = pci_header_type(bus, dev);
            pci_header_t hdr;

            if (header_type & HEADER_MULTIFUNC) {
                for (uint32_t func = 0; func < 8; func++) {
                    if (!pci_device_has_function(bus, dev, func)) {
                        continue;
                    }

                    pci_print_device((pci_device_t) {bus, dev, func});
                }
            } else {
                pci_read_config(bus, dev, 0, (uint8_t*) &hdr, sizeof(hdr));
                pci_print_device((pci_device_t) {bus, dev, 0});
            }

        }
    }
}

void init_pci() {
    pci_enumerate_devices();
}