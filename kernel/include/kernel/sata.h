#pragma once

#include <kernel/ahci.h>

#define SATA_MAX_DEVS 256

#define SATA_BLOCK_SIZE 512

#define SATA_SIG_ATA 0x00000101   // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101    // Port multiplier

#define SATA_DEV_MODEL_OFFSET 27 * 2
#define SATA_DEV_LBA_CAP_OFFSET 60 * 2
#define SATA_DEV_LBA_BS_OFFSET 117 * 2

enum {
    FIS_TYPE_REG_H2D = 0x27,   // Register FIS - host to device
    FIS_TYPE_REG_D2H = 0x34,   // Register FIS - device to host
    FIS_TYPE_DMA_ACT = 0x39,   // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA = 0x46,      // Data FIS - bidirectional
    FIS_TYPE_BIST = 0x58,      // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS = 0xA1,  // Set device bits FIS - device to host
};

enum {
    ATA_CMD_IDENTIFY_DEV = 0xEC,
    ATA_CMD_IDENTIFY_DEV_DMA = 0xEE,
    ATA_CMD_READ_DMA_EXT = 0x25,
    ATA_CMD_WRITE_DMA_EXT = 0x35,
};

enum {
    FIS_REC_DMA_SETUP_OFFSET = 0x00,
    FIS_REC_PIO_SETUP_OFFSET = 0x20,
    FIS_REC_D2H_OFFSET = 0x40,
    FIS_REC_DEV_BITS_OFFSET = 0x58,
};

enum {
    SATA_DEV_SATA,
    SATA_DEV_SATAPI,
    SATA_DEV_SEMB,
    SATA_DEV_PM,
};

extern char* sata_types[];

typedef volatile struct FIS_reg_h2d_t {
    uint8_t fis_type;   // FIS_TYPE_REG_H2D
    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 3;   // Reserved
    uint8_t c : 1;      // 1: Command, 0: Control
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

typedef volatile struct FIS_reg_d2h_t {
    uint8_t fis_type;   // FIS_TYPE_REG_D2H
    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 2;   // Reserved
    uint8_t i : 1;      // Interrupt bit
    uint8_t rsv1 : 1;   // Reserved
    uint8_t status;     // Status register
    uint8_t error;      // Error register
    uint8_t lba0;       // LBA low register, 7:0
    uint8_t lba1;       // LBA mid register, 15:8
    uint8_t lba2;       // LBA high register, 23:16
    uint8_t device;     // Device register
    uint8_t lba3;       // LBA register, 31:24
    uint8_t lba4;       // LBA register, 39:32
    uint8_t lba5;       // LBA register, 47:40
    uint8_t rsv2;       // Reserved
    uint8_t countl;     // Count register, 7:0
    uint8_t counth;     // Count register, 15:8
    uint8_t rsv3[2];    // Reserved
    uint8_t rsv4[4];    // Reserved
} __attribute__((packed)) FIS_reg_d2h_t;

typedef struct sata_device_t {
    uint8_t id;
    ahci_port_t* port;
    char model_name[40];
    uint32_t capacity_in_sectors;
    uint32_t type;
} sata_device_t;

bool sata_add_device(ahci_port_t* port, uint32_t type);
bool sata_setup_memory(sata_device_t* dev);
void sata_send_command(sata_device_t* dev);
bool sata_identify_device(sata_device_t* dev);
bool sata_read_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf);
bool sata_write_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf);
