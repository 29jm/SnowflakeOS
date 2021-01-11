#pragma once

#include <stdint.h>

typedef struct pci_header_t {
    uint16_t vendor, device;
    uint16_t command, status;
    uint8_t rev, interface, subclass, class;
    uint8_t cache_line_size, latency, header_type, bist;
} __attribute__((packed)) pci_header_t;

typedef struct pci_header0_t {
    pci_header_t header;
    uint32_t bar[6];
    uint32_t cardbus_cis_addr;
    uint16_t subsystem_vendor, subsystem;
    uint32_t expansion_addr;
    uint16_t capabilities_addr, reserved0;
    uint32_t reserved1;
    uint8_t int_line, int_pin, min_grant, max_latency;
} __attribute__((packed)) pci_header0_t;

typedef struct pci_device_t {
    uint8_t bus, dev, func;
} pci_device_t;

void init_pci();
uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t* buf, uint32_t size);