#include <kernel/sata.h>

#include <list.h>
#include <stdlib.h>
#include <kernel/sys.h>
#include <kernel/paging.h>

static list_t sata_devs;
static uint8_t next_dev_id = 0;
static bool list_inited = false;



void sata_add_device(ahci_controller_t* c, uint32_t port, uint32_t type) {
    if(!list_inited) {
        sata_devs = LIST_HEAD_INIT(sata_devs);
        list_inited = true;
    }

    sata_device_t* dev = (sata_device_t*)kmalloc(sizeof(sata_device_t));
    dev->id = next_dev_id;
    next_dev_id++;
    dev->controller = c;
    dev->port_num = port;
    dev->type = type;

    if(!list_add(&sata_devs, (void*)dev)) {
        printke("unable to add sata dev");
	kfree(dev);
    }

    sata_setup_memory(dev);
    sata_identify_device(dev);

}

// at this point in time we will only allow one command at a time,
// no adding multiple commands to the command list. And only r/w one sector at a time
void sata_setup_memory(sata_device_t* dev) {
    HBA_port_t* port = (dev->controller->base_address_virt + AHCI_PORTS_START_OFFSET + dev->port_num*AHCI_PORT_SIZE);
    //// make sure the port isn't running
    port->cmd &= ~HBA_PORT_CMD_ST;  // indicate to not process command list
    port->cmd &= ~HBA_PORT_CMD_FRE; // indicate to not post to FIS received area

    // wait for the FIS and command list engine to complete
    while (1) {
        if (port->cmd & HBA_PORT_CMD_FR)
            continue;
        if (port->cmd & HBA_PORT_CMD_CR)
            continue;
        break;
    }

    //// map out area for command list and FIS receive and data base address
    // align on page so that there is no discontinuity in physical memory - size can't exceed one page
    // cmdlist must be 1k aligned, and fis must be 256b aligned, cmdtbl must be 126b aligned
    uint32_t size = HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t) + SATA_BLOCK_SIZE;

    // if its greater than a page, we have an issue
    if (size > 0x1000) {
        printke("Issue setting up sata device id: %d, memory space larger than a page", dev->id);
        return;
    }

    void* cmdlist_base = kamalloc(size, 0x1000);
    // mark page as cache disable
    page_t *page = paging_get_page((uintptr_t)cmdlist_base, false, 0);
    *page |= PAGE_CACHE_DISABLE;

    // set command list base
    uint32_t cmdlist_phys_base = (uint32_t)paging_virt_to_phys((uintptr_t)cmdlist_base);
    port->clb = cmdlist_phys_base;
    port->clbu = 0;

    // set fis received base
    port->fb = cmdlist_phys_base + HBA_CMDLIST_SIZE;
    port->fbu = 0;

    // check alignments
    if(cmdlist_phys_base & 0b1111111111) {
        printke("Issue setting up sata device id: %d, cmdlist not aligned", dev->id);
        return;
    }
    if((cmdlist_phys_base+HBA_CMDLIST_SIZE) & 0b11111111) {
        printke("Issue setting up sata device id: %d, fis receive not aligned", dev->id);
        return;
    }
    if((cmdlist_phys_base+HBA_CMDLIST_SIZE+FIS_REC_SIZE) & 0b1111111) {
        printke("Issue setting up sata device id: %d, cmdtbl not aligned", dev->id);
        return;
    }

    //// zero out memory space and then appropriately fill the data structures
    memset(cmdlist_base,0,size);

    // cmd hdr
    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*)cmdlist_base;
    hdr->prdtl = 1;
    hdr->ctba = cmdlist_phys_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    hdr->ctbau = 0;

    // prd
    HBA_cmd_table_t* tbl = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    tbl->prdt_entry[0].dba = cmdlist_phys_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t));
    tbl->prdt_entry[0].dbau = 0;
    tbl->prdt_entry[0].dbc = SATA_BLOCK_SIZE;

    //// allow port to start running again
    while (port->cmd & HBA_PORT_CMD_CR); // make sure port isn't running

    port->cmd |= HBA_PORT_CMD_FRE; // indicate it can post to FIS
    port->cmd |= HBA_PORT_CMD_ST;  // indicate to start processing command list



    //// store virtual addresses in device struct
    dev->cmdlist_base_virt = cmdlist_base;
    dev->fis_rec_virt = cmdlist_base + (HBA_CMDLIST_SIZE);
    dev->cmd_table_virt = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    dev->data_base_virt = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t));
}


void sata_send_command(sata_device_t* dev) {
    return;
}


// send identify device command
void sata_identify_device(sata_device_t* dev) {
    HBA_port_t* port = (dev->controller->base_address_virt + AHCI_PORTS_START_OFFSET + dev->port_num*AHCI_PORT_SIZE);

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*)dev->cmdlist_base_virt;
    HBA_cmd_table_t* tbl = dev->cmd_table_virt;

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_IDENTIFY_DEV;


    memcpy(tbl,&cmd,sizeof(cmd));

    port->ci = 0x01;

    // spin for a second
    for(int i = 0; i < 1000000; i++);


    char* ptr = dev->data_base_virt;

    printk("DEV IDENT response: ");
    for(int i = 0; i < 512; i++) {
        printk(" - %c",ptr[i]);
    }

    return;
}

void sata_read(sata_device_t* dev) {
    return;
}

void sata_write(sata_device_t* dev) {
    return;
}

void sata_read_device(sata_device_t* dev, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf) {
    //// send read command
    // 
    return;
}
