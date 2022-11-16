#pragma once

#include <list.h>
#include <stdint.h>

#define PCI_MAX_NUM_DEVS 256

#define HDR0_BAR0 0x10
#define HDR0_BAR1 0x14
#define HDR0_BAR2 0x18
#define HDR0_BAR3 0x1C
#define HDR0_BAR4 0x20
#define HDR0_BAR5 0x24

#define MM_BAR_INFO_MASK 0x0000000F
#define IO_BAR_INFO_MASK 0x00000003

typedef volatile struct pci_header_t {
    uint16_t vendor, device;
    uint16_t command, status;
    uint8_t rev, interface, subclass, class;
    uint8_t cache_line_size, latency, header_type, bist;
} __attribute__((packed)) pci_header_t;

typedef volatile struct pci_header0_t {
    uint32_t bar[6];
    uint32_t cardbus_cis_addr;
    uint16_t subsystem_vendor, subsystem;
    uint32_t expansion_addr;
    uint16_t capabilities_addr, reserved0;
    uint32_t reserved1;
    uint8_t int_line, int_pin, min_grant, max_latency;
} __attribute__((packed)) pci_header0_t;

/* These are not useable right now: field must be reordered by groups of 4
 * bytes, reversed. */
typedef volatile struct pci_header1_t {
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

typedef volatile struct pci_header2_t {
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

typedef volatile struct pci_device_t {
    uint8_t id;
    uint8_t bus, dev, func;
    uint8_t hdr_type;
    pci_header_t hdr;
    union {
        pci_header0_t header0;
        pci_header1_t header1;
        pci_header2_t header2;
    };
} __attribute__((packed)) pci_device_t;


void init_pci();
void pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t* buf, uint32_t size);

uint16_t pci_read_config_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
uint32_t pci_read_config_long(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void pci_write_config_long(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data);
void pci_print_device(pci_device_t* dev);
void pci_print_all_devices();
