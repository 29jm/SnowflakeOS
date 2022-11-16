#include <kernel/paging.h>
#include <kernel/sata.h>
#include <kernel/sys.h>

#include <list.h>
#include <stdlib.h>

static list_t sata_devs;
static uint32_t next_dev_id = 0;
static bool list_inited = false;

bool sata_add_device(ahci_controller_t* c, uint32_t port, uint32_t type) {
    if (!list_inited) {
        sata_devs = LIST_HEAD_INIT(sata_devs);
        list_inited = true;
    }

    if (next_dev_id >= SATA_MAX_DEVS) {
        printk("trying to add a sata device with no slots left");
        return false;
    }

    sata_device_t* dev = (sata_device_t*) kmalloc(sizeof(sata_device_t));
    if (!dev) {
        printke("error allocating space for sata device on ahci controller: %d, port: %d", c->id, port);
        return false;
    }

    dev->id = next_dev_id;
    next_dev_id++;
    dev->controller = c;
    dev->port_num = port;
    dev->type = type;

    if (!list_add(&sata_devs, (void*) dev)) {
        printke("unable to add sata dev to list");
        kfree(dev);
    }

    sata_setup_memory(dev);
    sata_identify_device(dev);

    // uncomment below to do a read of the first sector of each drive
    /* uint8_t* buf = kmalloc(SATA_BLOCK_SIZE); */
    /* memset(buf,0,SATA_BLOCK_SIZE); */
    /* sata_read_device(dev,0,0,buf); */
    /* printk("read from sata device %d: %s", dev->id,buf); */

    return true;
}

// at this point in time we will only allow one command at a time,
// no adding multiple commands to the command list. And only r/w one sector at a time
bool sata_setup_memory(sata_device_t* dev) {
    HBA_port_t* port = (dev->controller->base_address_virt + AHCI_PORTS_START_OFFSET + dev->port_num * AHCI_PORT_SIZE);
    //// make sure the port isn't running
    port->cmd &= ~HBA_PORT_CMD_ST;  // indicate to not process command list
    port->cmd &= ~HBA_PORT_CMD_FRE; // indicate to not post to FIS received area

    // wait for the FIS and command list engine to complete
    uint32_t spin = 0;
    while (1) {
        spin++;
        if (port->cmd & HBA_PORT_CMD_FR)
            continue;
        if (port->cmd & HBA_PORT_CMD_CR)
            continue;
        if (spin > 10000) {
            printk("Port appears to be stuck during device memory setup (waiting for the engines "
                   "to complete) for sata dev: %d",
                dev->id);
            return false;
        }
        break;
    }

    //// map out area for command list and FIS receive and data base address
    // align on page so that there is no discontinuity in physical memory - size can't exceed one
    // page cmdlist must be 1k aligned, and fis must be 256b aligned, cmdtbl must be 126b aligned
    uint32_t size = HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t) + SATA_BLOCK_SIZE;

    // if its greater than a page, we have an issue
    if (size > 0x1000) {
        printke("Issue setting up sata device id: %d, memory space larger than a page", dev->id);
        return false;
        ;
    }

    void* cmdlist_base = kamalloc(size, 0x1000);
    if (!cmdlist_base) {
        printke("error allocating virt mem for sata dev id: %d cmdlist base", dev->id);
        return false;
    }
    // mark page as cache disable
    page_t* page = paging_get_page((uintptr_t) cmdlist_base, false, 0);
    *page |= PAGE_CACHE_DISABLE;

    // set command list base
    uint32_t cmdlist_phys_base = (uint32_t) paging_virt_to_phys((uintptr_t) cmdlist_base);
    if (!cmdlist_phys_base) {
        printke("error getting physical memory location for sata dev id: %d cmdlist base", dev->id);
        return false;
    }
    port->clb = cmdlist_phys_base;
    port->clbu = 0;

    // set fis received base
    port->fb = cmdlist_phys_base + HBA_CMDLIST_SIZE;
    port->fbu = 0;

    // check alignments
    if (cmdlist_phys_base & 0b1111111111) { // 1k align
        printke("Issue setting up sata device id: %d, cmdlist not aligned", dev->id);
        return false;
    }
    if ((cmdlist_phys_base + HBA_CMDLIST_SIZE) & 0b11111111) { // 256 align
        printke("Issue setting up sata device id: %d, fis receive not aligned", dev->id);
        return false;
    }
    if ((cmdlist_phys_base + HBA_CMDLIST_SIZE + FIS_REC_SIZE) & 0b1111111) { // 128 align
        printke("Issue setting up sata device id: %d, cmdtbl not aligned", dev->id);
        return false;
    }

    //// zero out memory space and then appropriately fill the data structures
    memset(cmdlist_base, 0, size);

    // cmd hdr
    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) cmdlist_base;
    hdr->prdtl = 1;
    hdr->ctba = cmdlist_phys_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    hdr->ctbau = 0;

    // prd
    HBA_cmd_table_t* tbl = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    tbl->prdt_entry[0].dba = cmdlist_phys_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t));
    tbl->prdt_entry[0].dbau = 0;
    tbl->prdt_entry[0].dbc = SATA_BLOCK_SIZE;

    //// allow port to start running again
    spin = 0;
    while (port->cmd & HBA_PORT_CMD_CR) { // make sure port isn't running, not sure why this is needed we already indicated the port shouldn't start
        spin++;
        if (spin > 10000) {
            printk("Port appears to be stuck during device memory setup (starting the port again) "
                   "for sata dev: %d",
                dev->id);
            return false;
        }
    }

    port->cmd |= HBA_PORT_CMD_FRE; // indicate it can post to FIS
    port->cmd |= HBA_PORT_CMD_ST;  // indicate to start processing command list

    //// store virtual addresses in device struct
    dev->cmdlist_base_virt = cmdlist_base;
    dev->fis_rec_virt = cmdlist_base + (HBA_CMDLIST_SIZE);
    dev->cmd_table_virt = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    dev->data_base_virt = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t));

    return true;
}

// send identify device command
bool sata_identify_device(sata_device_t* dev) {
    HBA_port_t* port = (dev->controller->base_address_virt + AHCI_PORTS_START_OFFSET + dev->port_num * AHCI_PORT_SIZE);

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->cmdlist_base_virt;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    HBA_cmd_table_t* tbl = dev->cmd_table_virt;

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_IDENTIFY_DEV;

    // move command to
    memcpy((void*) tbl, (const void*) &cmd, sizeof(cmd));

    // issue command
    port->ci = 0x01;

    // wait for port to return that the command has finished without error
    uint32_t spin = 0;
    while (port->ci != 0x0) {
        spin++;
        if (spin > 10000) {
            printk("Port appears to be stuck during device identification for sata dev: %d", dev->id);
            return false;
        }
    }

    if (port->is & HBA_PORT_IS_TFES) {
        printk("error identifying device %d", dev->id);
        return false;
    }

    char* dest = dev->model_name;
    char* src = dev->data_base_virt + SATA_DEV_MODEL_OFFSET;

    // copy name to device struct, swap bits due to sata endianess
    for (unsigned int i = 0; i < sizeof(dev->model_name); i += 2) {
        dest[i] = src[i + 1];
        dest[i + 1] = src[i];

        if (src[i] < 0x21 || src[i] > 0x7E) // not a printable character
            dest[i + 1] = 0;
        if (src[i + 1] < 0x21 || src[i + 1] > 0x7E) // not a printable character
            dest[i] = 0;
    }

    uint16_t* base = (uint16_t*) dev->data_base_virt;

    uint32_t capacity;
    capacity = base[SATA_DEV_LBA_CAP_OFFSET / 2];
    capacity |= (((uint32_t) base[SATA_DEV_LBA_CAP_OFFSET / 2 + 1]) << 16);

    dev->capacity_in_sectors = capacity;

    printk("Identified SATA Device of type 0x%x with name: %s and capacity: 0x%x blocks", dev->type,
        dev->model_name, dev->capacity_in_sectors);

    return true;
}

bool sata_read_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf) {
    //// send read command
    HBA_port_t* port = (dev->controller->base_address_virt + AHCI_PORTS_START_OFFSET + dev->port_num * AHCI_PORT_SIZE);

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->cmdlist_base_virt;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    hdr->c = 1;
    HBA_cmd_table_t* tbl = dev->cmd_table_virt;

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_READ_DMA_EXT;
    cmd.countl = 1;
    cmd.device = (1 << 6); // lba48 mode?

    cmd.lba0 = (block_num_l >> 0) & 0xFF;
    cmd.lba1 = (block_num_l >> 8) & 0xFF;
    cmd.lba2 = (block_num_l >> 16) & 0xFF;
    cmd.lba3 = (block_num_l >> 24) & 0xFF;
    cmd.lba4 = (block_num_h >> 0) & 0xFF;
    cmd.lba5 = (block_num_h >> 8) & 0xFF;

    // move command to table
    memcpy((void*) tbl, (const void*) &cmd, sizeof(cmd));

    // issue command
    port->ci = 0x01;

    // wait for port to return that the command has finished
    uint32_t spin = 0;
    while (port->ci != 0x0) {
        spin++;
        if (spin > 10000) {
            printk("Port appears to be stuck during read for sata dev: %d", dev->id);
            return false;
        }
    }

    if (port->is & (1 << 30)) {
        printke("Error attempting to read from sata device id: %d", dev->id);
        return false;
    }

    memcpy(buf, dev->data_base_virt, SATA_BLOCK_SIZE);

    return true;
}

bool sata_write_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf) {
    //// send read command
    HBA_port_t* port = (dev->controller->base_address_virt + AHCI_PORTS_START_OFFSET + dev->port_num * AHCI_PORT_SIZE);

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->cmdlist_base_virt;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    hdr->c = 1;
    HBA_cmd_table_t* tbl = dev->cmd_table_virt;

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_WRITE_DMA_EXT;
    cmd.countl = 1;
    cmd.device = (1 << 6); // lba48 mode?

    cmd.lba0 = (block_num_l >> 0) & 0xFF;
    cmd.lba1 = (block_num_l >> 8) & 0xFF;
    cmd.lba2 = (block_num_l >> 16) & 0xFF;
    cmd.lba3 = (block_num_l >> 24) & 0xFF;
    cmd.lba4 = (block_num_h >> 0) & 0xFF;
    cmd.lba5 = (block_num_h >> 8) & 0xFF;

    memcpy(dev->data_base_virt, buf, SATA_BLOCK_SIZE);

    // move command to table
    memcpy((void*) tbl, (const void*) &cmd, sizeof(cmd));

    // issue command
    port->ci = 0x01;

    // wait for port to return that the command has finished
    uint32_t spin = 0;
    while (port->ci != 0x0) {
        spin++;
        if (spin > 10000) {
            printk("Port appears to be stuck during write for sata dev: %d", dev->id);
            return false;
        }
    }

    if (port->is & (1 << 30)) {
        printke("Error attempting to write to sata device id: %d", dev->id);
        return false;
    }

    return true;
}
