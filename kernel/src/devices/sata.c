#include <kernel/ahci.h>
#include <kernel/paging.h>
#include <kernel/sata.h>
#include <kernel/sys.h>

#include <list.h>
#include <stdlib.h>

static list_t sata_devs;
static uint32_t next_dev_id = 0;
static bool list_inited = false;

char* sata_types[] = {
    [SATA_DEV_SATA] = "SATA ATA",
    [SATA_DEV_SATAPI] = "SATA ATAPI",
    [SATA_DEV_SEMB] = "SATA SEMB",
    [SATA_DEV_PM] = "SATA PM",

};

bool sata_add_device(ahci_port_t* port, uint32_t type) {
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
        printke("error allocating space for sata device on ahci controller: %d, port: %d",
            port->c->id, port->port_num);
        return false;
    }

    dev->id = next_dev_id;
    next_dev_id++;
    dev->port = port;
    dev->type = type;

    if (!list_add(&sata_devs, (void*) dev)) {
        printke("unable to add sata dev to list");
        kfree(dev);
    }

    sata_identify_device(dev);

    // uncomment below to do a read of the first sector of each drive
    // uint8_t* buf = kmalloc(SATA_BLOCK_SIZE);
    // memset(buf,0,SATA_BLOCK_SIZE);
    // sata_read_device(dev,0,0,buf);
    // printk("read from sata device %d: %s", dev->id,buf);

    return true;
}

// send identify device command
bool sata_identify_device(sata_device_t* dev) {
    HBA_port_t* port = dev->port->port;

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->port->cmdlist_base_virt;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    HBA_cmd_table_t* tbl = dev->port->cmd_table_virt;

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
    char* src = dev->port->data_base_virt + SATA_DEV_MODEL_OFFSET;

    // copy name to device struct, swap bits due to sata endianess
    for (unsigned int i = 0; i < sizeof(dev->model_name); i += 2) {
        dest[i] = src[i + 1];
        dest[i + 1] = src[i];

        if (src[i] < 0x21 || src[i] > 0x7E) // not a printable character
            dest[i + 1] = 0;
        if (src[i + 1] < 0x21 || src[i + 1] > 0x7E) // not a printable character
            dest[i] = 0;
    }

    uint16_t* base = (uint16_t*) dev->port->data_base_virt;

    uint32_t capacity;
    capacity = base[SATA_DEV_LBA_CAP_OFFSET / 2];
    capacity |= (((uint32_t) base[SATA_DEV_LBA_CAP_OFFSET / 2 + 1]) << 16);

    dev->capacity_in_sectors = capacity;

    printk("Identified SATA Device of type %s with name: %s and capacity: 0x%x blocks",
        sata_types[dev->type], dev->model_name, dev->capacity_in_sectors);

    return true;
}

bool sata_read_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf) {
    //// send read command
    HBA_port_t* port = dev->port->port;

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->port->cmdlist_base_virt;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    hdr->c = 1;
    HBA_cmd_table_t* tbl = dev->port->cmd_table_virt;

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

    memcpy(buf, dev->port->data_base_virt, SATA_BLOCK_SIZE);

    return true;
}

bool sata_write_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf) {
    //// send read command
    HBA_port_t* port = dev->port->port;

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->port->cmdlist_base_virt;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    hdr->c = 1;
    HBA_cmd_table_t* tbl = dev->port->cmd_table_virt;

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

    memcpy(dev->port->data_base_virt, buf, SATA_BLOCK_SIZE);

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
