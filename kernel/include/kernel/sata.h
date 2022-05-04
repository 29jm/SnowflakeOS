#pragma once

#include <kernel/ahci.h>

#define SATA_BLOCK_SIZE 512

enum {
    FIS_TYPE_REG_H2D   = 0x27, // Register FIS - host to device
    FIS_TYPE_REG_D2H   = 0x34, // Register FIS - device to host
    FIS_TYPE_DMA_ACT   = 0x39, // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA      = 0x46, // Data FIS - bidirectional
    FIS_TYPE_BIST      = 0x58, // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS  = 0xA1, // Set device bits FIS - device to host
};

enum {
    ATA_CMD_IDENTIFY_DEV =     0xEC,
    ATA_CMD_IDENTIFY_DEV_DMA = 0xEE,
    ATA_CMD_READ_DMA_EXT =     0x25,
    ATA_CMD_WRITE_DMA_EXT =    0x35,
};

enum {
    SATA_DEV_SATA,
    SATA_DEV_SATAPI,
    SATA_DEV_SEMB,
    SATA_DEV_PM,
};

typedef volatile struct FIS_reg_h2d_t {
    uint8_t fis_type;   // FIS_TYPE_REG_H2D
    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0   : 3; // Reserved
    uint8_t c      : 1; // 1: Command, 0: Control
    uint8_t command;    // Command register
    uint8_t featurel;   // Feature register, 7:0
    uint8_t lba0;       // LBA low register, 7:0
    uint8_t lba1;       // LBA mid register, 15:8
    uint8_t lba2;       // LBA high register, 23:16
    uint8_t device;     // Device register
    uint8_t lba3;       // LBA register, 31:24
    uint8_t lba4;       // LBA register, 39:32
    uint8_t lba5;       // LBA register, 47:40
    uint8_t featureh;   // Feature register, 15:8
    uint8_t countl;     // Count register, 7:0
    uint8_t counth;     // Count register, 15:8
    uint8_t icc;        // Isochronous command completion
    uint8_t control;    // Control register
    uint8_t rsv1[4];    // Reserved
} __attribute__((packed)) FIS_reg_h2d_t;

typedef struct sata_device_t {
    uint8_t id;
    char model_name[20];
    ahci_controller_t* controller;
    uint8_t port_num;
    uint32_t type;
    void* cmdlist_base_virt;
    void* fis_rec_virt;
    HBA_cmd_table_t* cmd_table_virt;
    void* data_base_virt;
} sata_device_t;


void sata_add_device(ahci_controller_t* c, uint32_t port, uint32_t type);
void sata_setup_memory(sata_device_t* dev);
void sata_send_command(sata_device_t* dev);
void sata_identify_device(sata_device_t* dev);
void sata_read(sata_device_t* dev);
void sata_write(sata_device_t* dev);
