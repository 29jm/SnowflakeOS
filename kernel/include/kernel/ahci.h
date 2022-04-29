#pragma once

#include <kernel/pci.h>


typedef volatile struct hba_ghc_t {
	uint32_t capabilities1;		// 0x00, Host capability
	uint32_t ghc;			// 0x04, Global host control
	uint32_t int_status;		// 0x08, Interrupt status
	uint32_t ports_implemented;	// 0x0C, Port implemented
	uint32_t version;		// 0x10, Version
	uint32_t ccc_ctl;		// 0x14, Command completion coalescing control
	uint32_t ccc_pts;		// 0x18, Command completion coalescing ports
	uint32_t em_loc;		// 0x1C, Enclosure management location
	uint32_t em_ctl;		// 0x20, Enclosure management control
	uint32_t capabilities2;	 	// 0x24, Host capabilities extended
	uint32_t bohc;			// 0x28, BIOS/OS handoff control and status
}__attribute__((packed)) hba_ghc_t;

typedef struct ahci_controller_t {
	pci_device_t* pci_dev;
	void* base_address;
	void* base_address_virt;
	uint32_t address_size;
	volatile hba_ghc_t ghc;
} ahci_controller_t;

void init_ahci();
void ahci_add_controller(pci_device_t* pci_dev);
void init_controller(ahci_controller_t* controller);

void ahci_print_controller(ahci_controller_t* controller);
void ahci_print_all_controllers();