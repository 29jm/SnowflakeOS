#pragma once

#include <kernel/ahci.h>
#include <kernel/fs.h>

#define SATA_MAX_DEVS 256

#define SATA_BLOCK_SIZE 512

#define SATA_SIG_ATA 0x00000101   // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101    // Port multiplier

#define SATA_DEV_MODEL_OFFSET (27 * 2)
#define SATA_DEV_LBA_CAP_OFFSET (60 * 2)
#define SATA_DEV_LBA_BS_OFFSET (117 * 2)

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
uint32_t sata_read_device(sata_device_t* dev, uint32_t blk_l, uint16_t blk_h, uint32_t size, uint8_t* buf);
uint32_t sata_write_device(sata_device_t* dev, uint32_t blk_l, uint16_t blk_h, uint32_t size, uint8_t* buf);

sata_device_t* sata_get_device_by_name(const char* name);
fs_device_t sata_to_fs_device(sata_device_t* dev);