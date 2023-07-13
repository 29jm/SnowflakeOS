#pragma once

#include <kernel/pci.h>

#define AHCI_MAX_CONTROLLERS_NUM 256

#define HBA_PORT_CMD_ST (1 << 0)  // start processing the command list
#define HBA_PORT_CMD_CR (1 << 15) // indicates the command list engine is running
#define HBA_PORT_CMD_FRE (1 << 4) // can post received FIS to fb pointer area
#define HBA_PORT_CMD_FR (1 << 14) // indicates FIS engine is running

#define HBA_PORT_IS_TFES (1 << 30) // was there an error in the received fis

#define HBA_CMDLIST_SIZE 0x400 // 1 KiB
#define FIS_REC_SIZE 0x100     // 256 B
#define AHCI_PRDT_SIZE 0x1000  // 4 KiB

#define AHCI_ENABLE (1 << 31)
#define AHCI_MAX_PORTS 32
#define AHCI_PORTS_START_OFFSET 0x100
#define AHCI_PORT_SIZE sizeof(HBA_port_t)

#define AHCI_GET_PORT(c_ptr, p_num) \
    ((HBA_port_t*) (c_ptr->abar_virt + AHCI_PORTS_START_OFFSET + p_num * AHCI_PORT_SIZE))

// Generic Host Control - Content of the ABAR
typedef volatile struct HBA_ghc_t {
    uint32_t cap1;              // 0x00, Host capability
    uint32_t ghc;               // 0x04, Global host control
    uint32_t int_status;        // 0x08, Interrupt status
    uint32_t ports_implemented; // 0x0C, Port implemented
    uint32_t version;           // 0x10, Version
    uint32_t ccc_ctl;           // 0x14, Command completion coalescing control
    uint32_t ccc_pts;           // 0x18, Command completion coalescing ports
    uint32_t em_loc;            // 0x1C, Enclosure management location
    uint32_t em_ctl;            // 0x20, Enclosure management control
    uint32_t cap2;              // 0x24, Host capabilities extended
    uint32_t bohc;              // 0x28, BIOS/OS handoff control and status
} __attribute__((packed)) HBA_ghc_t;

typedef volatile struct HBA_port_t {
    uint32_t clb;       // 0x00, command list base address, 1K-byte aligned
    uint32_t clbu;      // 0x04, command list base address upper 32 bits
    uint32_t fb;        // 0x08, FIS base address, 256-byte aligned
    uint32_t fbu;       // 0x0C, FIS base address upper 32 bits
    uint32_t is;        // 0x10, interrupt status
    uint32_t ie;        // 0x14, interrupt enable
    uint32_t cmd;       // 0x18, command and status
    uint32_t rsv0;      // 0x1C, Reserved
    uint32_t tfd;       // 0x20, task file data
    uint32_t sig;       // 0x24, signature
    uint32_t ssts;      // 0x28, SATA status (SCR0:SStatus)
    uint32_t sctl;      // 0x2C, SATA control (SCR2:SControl)
    uint32_t serr;      // 0x30, SATA error (SCR1:SError)
    uint32_t sact;      // 0x34, SATA active (SCR3:SActive)
    uint32_t ci;        // 0x38, command issue
    uint32_t sntf;      // 0x3C, SATA notification (SCR4:SNotification)
    uint32_t fbs;       // 0x40, FIS-based switch control
    uint32_t rsv1[11];  // 0x44 ~ 0x6F, Reserved
    uint32_t vendor[4]; // 0x70 ~ 0x7F, vendor specific
} __attribute__((packed)) HBA_port_t;

typedef volatile struct HBA_cmd_hdr_t {
    uint8_t cfl : 5;  // Command FIS length in DWORDS, 2 ~ 16
    uint8_t a : 1;    // ATAPI
    uint8_t w : 1;    // Write, 1: H2D, 0: D2H
    uint8_t p : 1;    // Prefetchable
    uint8_t r : 1;    // Reset
    uint8_t b : 1;    // BIST
    uint8_t c : 1;    // Clear busy upon R_OK
    uint8_t rsv0 : 1; // Reserved
    uint8_t pmp : 4;  // Port multiplier port
    uint16_t prdtl; // Physical region descriptor table length in entries, 0 indicates don't process cmd
    volatile uint32_t prdbc; // Physical region descriptor byte count transferred
    uint32_t ctba;           // Command table descriptor base address
    uint32_t ctbau;          // Command table descriptor base address upper 32 bits
    uint32_t rsv1[4];        // Reserved
} __attribute__((packed)) HBA_cmd_hdr_t;

typedef volatile struct HBA_prdt_entry_t {
    uint32_t dba;                           // Data base address
    uint32_t dbau;                          // Data base address upper 32 bits
    uint32_t rsv0;                          // Reserved
    uint32_t dbc : 22;                      // Byte count, 4M max
    uint32_t rsv1 : 9;                      // Reserved
    uint32_t i : 1;                         // Interrupt on completion
} __attribute__((packed)) HBA_prdt_entry_t; // size is 0x10

typedef volatile struct HBA_cmd_table_t {  // must be 128 byte aligned
    uint8_t cfis[64];                      // Command FIS
    uint8_t acmd[16];                      // ATAPI command, 12 or 16 bytes
    uint8_t reserved[48];                  // Reserved
    HBA_prdt_entry_t prdt_entry[1];        // Physical region descriptor table entries, 0 ~ 65535
} __attribute__((packed)) HBA_cmd_table_t; // size is 0x80 + sizeof(HBA_pdtr_entry) = 0x90 total

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

typedef struct ahci_controller_t {
    uint8_t id;
    pci_device_t* pci_dev;
    void* abar_phys;       // AHCI Base Address Register, from BAR5 of PCI header
    void* abar_virt;       // Mapping into virtual memory of the above
    uint32_t address_size; // Size of the ABAR = sizeof(HBA_ghc_t) + num_ports*sizeof(HBA_port_t)
} ahci_controller_t;

typedef struct ahci_port_t {
    HBA_port_t* port;
    ahci_controller_t* ahci_controller;
    uint32_t port_num;
    HBA_cmd_hdr_t* command_list;    // Array of 32 entries
    FIS_reg_d2h_t* fis_received;    // Points to one FIS (hopefully the right kind)
    HBA_cmd_table_t* command_table; // Points to one command table
} ahci_port_t;

void init_ahci();
bool ahci_add_controller(pci_device_t* pci_dev);
void init_controller(ahci_controller_t* controller);

bool ahci_reset_port(HBA_port_t* port);
void ahci_start_port(HBA_port_t* port);
bool ahci_stop_port(HBA_port_t* port);
void ahci_port_clear_err(HBA_port_t* p);

void ahci_print_controller(ahci_controller_t* controller);
void ahci_print_all_controllers();
