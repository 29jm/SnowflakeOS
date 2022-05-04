#include <kernel/ahci.h>

#include <kernel/pci.h>
#include <kernel/sys.h>
#include <list.h>
#include <stdlib.h>
#include <kernel/paging.h>
#include <string.h>
#include <kernel/sata.h>


static list_t ahci_controllers;
static uint8_t next_dev_id = 0;
static bool list_inited = false;

void init_ahci() {
    printk("initalizing ahci");
    ahci_print_all_controllers();
}

void ahci_add_controller(pci_device_t* pci_dev) {
    if(!list_inited) {
        ahci_controllers = LIST_HEAD_INIT(ahci_controllers);
        list_inited = true;
    }

    ahci_controller_t* controller = (ahci_controller_t*)kmalloc(sizeof(ahci_controller_t));
    controller->id = next_dev_id;
    next_dev_id++;
    controller->pci_dev = pci_dev;
    init_controller(controller);
    if(!list_add(&ahci_controllers, (void*)controller)) {
        printke("unable to add ahci controller");
	kfree(controller);
    }
}

void init_controller(ahci_controller_t* controller) {
    // get BAR size
    uint32_t bar5, size;
    bar5 = controller->pci_dev->header0.bar[5];

    // write all 1s to indicate get address size
    pci_write_config_long(controller->pci_dev->bus, controller->pci_dev->dev,
        controller->pci_dev->func, HDR0_BAR5, 0xFFFFFFFF);

    // read back the value
    size = pci_read_config_long(
        controller->pci_dev->bus, controller->pci_dev->dev, controller->pci_dev->func, HDR0_BAR5);

    // calculation to get size
    size &= ~MM_BAR_INFO_MASK; // mask information bits
    size = ~size;              // invert
    size += 1;

    // reset bar
    pci_write_config_long(controller->pci_dev->bus, controller->pci_dev->dev,
        controller->pci_dev->func, HDR0_BAR5, bar5);

    bar5 &= ~MM_BAR_INFO_MASK; // mask off the information bits
    controller->base_address = (void*) bar5;
    controller->address_size = size;

    ///// map base address into virtual memory - mark as do not cache because phys addr is hardware
    uint32_t num_pages = controller->address_size / 0x1000;
    if (controller->address_size % 0x1000 != 0)
        num_pages++; // grab the remaining bottom page if the required memory isn't paged aligned

    // NOTE: not sure how best to implement getting free virtual
    // memory in kernel space that can be mapped to hardware
    void* virt = kamalloc(0x1000, num_pages * 0x1000);
    paging_unmap_pages((uintptr_t) virt, num_pages);

    paging_map_pages((uintptr_t) virt, (uintptr_t) controller->base_address, num_pages, PAGE_CACHE_DISABLE);

    controller->base_address_virt = virt;
    controller->ghc = controller->base_address_virt;

    // mark that ahci mode is enabled / notify the controller we are going to use it
    controller->ghc->ghc |= AHCI_ENABLE;


    // check devices on available ports
    for(int i = 0; i < AHCI_MAX_PORTS; i++){
        if(!((controller->ghc->ports_implemented >> i) & 0x01))
            continue;

        // check port device status and type
        HBA_port_t *port = (HBA_port_t*)(controller->base_address_virt + AHCI_PORTS_START_OFFSET + i*AHCI_PORT_SIZE);
            
        if((port->ssts & 0xF) == 0x03) { // device present and connection established

            uint32_t device_type = port->sig;
            switch(device_type) {
            case SATA_SIG_ATA:
                sata_add_device(controller, i, SATA_DEV_SATA);
                break;
            case SATA_SIG_ATAPI:
                sata_add_device(controller, i, SATA_DEV_SATAPI);
                break;
            case SATA_SIG_SEMB:
                sata_add_device(controller, i, SATA_DEV_SEMB);
                break;
            case SATA_SIG_PM:
                sata_add_device(controller, i, SATA_DEV_PM);
                break;
            default:
                printk("couldn't determine sata device type, provided type was: 0x%x", device_type);
                break;
            }
        }
    }

}

void ahci_print_controller(ahci_controller_t* controller) {
    printk("ahci controller with id: %d, pci id: %d, BAR: 0x%x, BAR virt: 0x%x, "
           "BAR size: 0x%x, version: 0x%x, capabilities: 0x%x",
        controller->id, controller->pci_dev->id, controller->base_address, controller->base_address_virt,
        controller->address_size, controller->ghc->version, controller->ghc->cap1);

    printk(" - %d control slots per port", (controller->ghc->cap1 >> 8) & 0b1111);


}

void ahci_print_all_controllers() {
    ahci_controller_t* c;
    list_for_each_entry(c, &ahci_controllers) {
        ahci_print_controller(c);
    }
}
