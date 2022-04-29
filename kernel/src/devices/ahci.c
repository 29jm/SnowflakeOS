#include <kernel/ahci.h>

#include <kernel/pci.h>
#include <kernel/sys.h>
#include <list.h>
#include <stdlib.h>
#include <kernel/paging.h>
#include <string.h>

static list_t ahci_controllers;
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
    controller->pci_dev = pci_dev;
    init_controller(controller);
    if(!list_add(&ahci_controllers, (void*)controller)) {
        printke("unable to add ahci controller");
    }
}

void init_controller(ahci_controller_t* controller) {
    // get BAR size
	uint32_t bar5, size;
	bar5 = pci_read_config_long(controller->pci_dev->bus,
                                controller->pci_dev->dev,
                                controller->pci_dev->func,
                                HDR0_BAR5);

    bar5 &= ~MM_BAR_INFO_MASK; // mask off the information bits
	
	pci_write_config_long(controller->pci_dev->bus,
	                      controller->pci_dev->dev,
                          controller->pci_dev->func,
                          HDR0_BAR5,0xFFFFFFFF); // all ones to indicate get address size
	
	size = pci_read_config_long(controller->pci_dev->bus,
                                controller->pci_dev->dev,
                                controller->pci_dev->func,
                                HDR0_BAR5);
    
    size &= ~MM_BAR_INFO_MASK; // mask information bits
    size = ~size;       // invert
    size += 1;


	// reset bar
	pci_write_config_long(controller->pci_dev->bus,
	                      controller->pci_dev->dev,
                          controller->pci_dev->func,
                          HDR0_BAR5,bar5);
	
    controller->base_address = (void*)bar5;
	controller->address_size = size;


    ///// map base address into virtual memory - mark as do not cache because phys addr is hardware
    uint32_t num_pages = controller->address_size / 0x1000;
    if(controller->address_size % 0x1000 != 0)
        num_pages++;

    // NOTE: not sure how best to implement getting free virtual
    // memory in kernel space that can be mapped to hardware
    void* virt = kamalloc(0x1000, num_pages*0x1000);
    paging_unmap_pages((uintptr_t)virt,num_pages);

    paging_map_pages((uintptr_t)virt, (uintptr_t)controller->base_address, num_pages, 1<<PAGE_CACHE_DISABLE);

    controller->base_address_virt = virt;

    /////  copy in ghc
    memcpy(&controller->ghc,controller->base_address_virt,sizeof(hba_ghc_t));

}

void ahci_print_controller(ahci_controller_t* controller) {
    printk("ahci controller with pci id: %d, BAR: 0x%x, BAR virt: 0x%x, "
            "BAR size: 0x%x, version: 0x%x",
           controller->pci_dev->id, controller->base_address, controller->base_address_virt,
           controller->address_size, controller->ghc.version);


    // print number of ports
    char ports_str[1024] = {0};
    unsigned int i;
    for(i = 0; i < sizeof(controller->ghc.ports_implemented)*8;i++) {
        if(((controller->ghc.ports_implemented>>i) & 0x01) == true) {
            char tmp[3] = {0};
            sprintf(tmp,"%d,", i);
            strcat(ports_str,tmp);
        }
    }

    ports_str[strlen(ports_str)-1] = 0;
    printk(" - ports implemented: %s",ports_str);
}

void ahci_print_all_controllers() {
    ahci_controller_t* c;
    list_for_each_entry(c, &ahci_controllers) {
        ahci_print_controller(c);
    }
}
