#pragma once

#include <stdint.h>

#define MB2_MAGIC 0x36D76289
#define MB2_TAG_END 0
#define MB2_TAG_CMDLINE 1
#define MB2_TAG_MODULE 3
#define MB2_TAG_MEM 4
#define MB2_TAG_MMAP 6
#define MB2_TAG_VBE 7
#define MB2_TAG_FB 8
#define MB2_TAG_APM 10
#define MB2_TAG_RSDP1 14
#define MB2_TAG_RSDP2 15

#define MB2_MMAP_AVAIL 1

typedef struct mb2_tag_t {
    uint32_t type;
    uint32_t size;
} mb2_tag_t __attribute__((packed));

typedef struct mb2_tag_mem_t {
    mb2_tag_t header;
    uint32_t mem_lower;
    uint32_t mem_upper;
} mb2_tag_mem_t __attribute__((packed));

typedef struct mb2_tag_cmdline_t {
    mb2_tag_t header;
    unsigned char cmdline[];
} mb2_tag_cmdline_t __attribute__((packed));

typedef struct mb2_tag_module_t {
    mb2_tag_t header;
    uintptr_t mod_start;
    uintptr_t mod_end;
    unsigned char name[];
} mb2_tag_module_t __attribute__((packed));

typedef struct mb2_mmap_entry_t {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} mb2_mmap_entry_t __attribute__((packed));

typedef struct mb2_tag_mmap_t {
    mb2_tag_t header;
    uint32_t entry_size;
    uint32_t entry_version;
    mb2_mmap_entry_t entries[];
} mb2_tag_mmap_t __attribute__((packed));

typedef struct mb2_tag_apm_t {
    mb2_tag_t header;
    uint16_t version;
    uint16_t cseg;
    uint32_t offset;
    uint16_t cseg16;
    uint16_t dseg;
    uint16_t flags;
    uint16_t cseg_len;
    uint16_t cseg16_len;
    uint16_t dseg_len;
} mb2_tag_apm_t __attribute__((packed));

typedef struct mb2_tag_vbe_t {
    mb2_tag_t header;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint8_t vbe_control_info[512];
    uint8_t vbe_mode_info[256];
} mb2_tag_vbe_t __attribute__((packed));

typedef struct mb2_tag_fb_t {
    mb2_tag_t header;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type;
    uint8_t reserved;
    /* Color info stuff goes here, but it's tedious & useless */
} mb2_tag_fb_t __attribute__((packed));

/* TODO: reconsider moving those two once ACPI gets there */
typedef struct acpi_rsdp1_t {
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
} acpi_rsdp1_t __attribute__((packed));

typedef struct acpi_rsdp2_t {
    acpi_rsdp1_t rsdp1;
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t checksum;
    uint8_t reserved[3];
} acpi_rsdp2_t __attribute__((packed));

typedef struct mb2_tag_rsdp1_t {
    mb2_tag_t header;
    acpi_rsdp1_t rsdp;
} mb2_tag_rsdp1_t __attribute__((packed));

typedef struct mb2_tag_rsdp2_t {
    mb2_tag_t header;
    acpi_rsdp2_t rsdp;
} mb2_tag_rsdp2_t __attribute__((packed));

typedef struct mb2_t {
    uint32_t total_size;
    uint32_t reserved;
    /* Not an array: just points to the first tag */
    mb2_tag_t tags[];
} mb2_t __attribute__((packed));

void mb2_print_tags(mb2_t* boot);
mb2_tag_t* mb2_find_tag(mb2_t* boot, uint32_t tag_type);