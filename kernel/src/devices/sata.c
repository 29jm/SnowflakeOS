#include <kernel/ahci.h>
#include <kernel/paging.h>
#include <kernel/sata.h>
#include <kernel/sys.h>

#include <list.h>
#include <stdlib.h>

#define SPIN_WAIT 10000

static list_t sata_devs;
static uint32_t next_dev_id = 0;
static bool dev_list_initialized = false;

char* sata_types[] = {
    [SATA_DEV_SATA] = "SATA ATA",
    [SATA_DEV_SATAPI] = "SATA ATAPI",
    [SATA_DEV_SEMB] = "SATA SEMB",
    [SATA_DEV_PM] = "SATA PM"
};

bool sata_add_device(ahci_port_t* port, uint32_t type) {
    if (!dev_list_initialized) {
        sata_devs = LIST_HEAD_INIT(sata_devs);
        dev_list_initialized = true;
    }

    if (next_dev_id >= SATA_MAX_DEVS) {
        printk("trying to add a sata device with no slots left");
        return false;
    }

    sata_device_t* dev = (sata_device_t*) kmalloc(sizeof(sata_device_t));
    if (!dev) {
        printke("error allocating space for sata device on ahci controller: %d, port: %d",
            port->ahci_controller->id, port->port_num);
        return false;
    }

    dev->id = next_dev_id++;
    dev->port = port;
    dev->type = type;

    if (!list_add(&sata_devs, (void*) dev)) {
        printke("unable to add sata dev to list");
        kfree(dev);
    }

    sata_identify_device(dev);

    // Uncomment below to do a read of the first sector of each drive
    // uint8_t* buf = kmalloc(SATA_BLOCK_SIZE);
    // memset(buf, 0, SATA_BLOCK_SIZE);
    // sata_read_device(dev, 53, 0, buf);
    // printk("read from sata device %d: %s", dev->id,buf);
    // dbg_buffer_dump(buf, 512);
    // kfree(buf);

    return true;
}

// Send identify device command
bool sata_identify_device(sata_device_t* dev) {
    HBA_port_t* port = dev->port->port;

    dev->port->command_list->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_IDENTIFY_DEV;

    // Move command to the command table
    memcpy((void*) dev->port->command_table, (const void*) &cmd, sizeof(cmd));

    // Issue command
    port->ci = 0x01;

    // Wait for port to return that the command has finished without error
    uint32_t spin = 0;
    while (port->ci != 0x0) {
        spin++;

        if (spin > SPIN_WAIT) {
            printk("Port appears to be stuck during device identification for sata dev: %d", dev->id);
            return false;
        }
    }

    if (port->is & HBA_PORT_IS_TFES) {
        printk("error identifying device %d", dev->id);
        return false;
    }

    char* dest = dev->model_name;
    char* src = (char*) (dev->port->data_base + SATA_DEV_MODEL_OFFSET);

    // Copy name to device struct, swap bits due to sata endianess
    for (unsigned int i = 0; i < sizeof(dev->model_name); i += 2) {
        dest[i] = src[i + 1];
        dest[i + 1] = src[i];

        if (src[i] < 0x21 || src[i] > 0x7E) // Not a printable character
            dest[i + 1] = 0;

        if (src[i + 1] < 0x21 || src[i + 1] > 0x7E) // Not a printable character
            dest[i] = 0;
    }

    uint16_t* base = (uint16_t*) dev->port->data_base;

    uint32_t capacity;
    capacity = base[SATA_DEV_LBA_CAP_OFFSET / 2];
    capacity |= (((uint32_t) base[SATA_DEV_LBA_CAP_OFFSET / 2 + 1]) << 16);

    dev->capacity_in_sectors = capacity;

    printk("identified SATA device: type %s named \"%s\", capacity: 0x%x blocks",
        sata_types[dev->type], dev->model_name, dev->capacity_in_sectors);

    return true;
}

bool sata_read_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf) {
    // Send read command
    HBA_port_t* port = dev->port->port;

    dev->port->command_list->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    dev->port->command_list->c = 1;
    dev->port->command_list->w = 0;

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_READ_DMA_EXT;
    cmd.countl = dev->port->data_base_size / SATA_BLOCK_SIZE;

    cmd.lba0 = (block_num_l >> 0) & 0xFF;
    cmd.lba1 = (block_num_l >> 8) & 0xFF;
    cmd.lba2 = (block_num_l >> 16) & 0xFF;
    cmd.device = (1 << 6); // lba48 mode
    cmd.lba3 = (block_num_l >> 24) & 0xFF;
    cmd.lba4 = (block_num_h >> 0) & 0xFF;
    cmd.lba5 = (block_num_h >> 8) & 0xFF;

    // Move command to table
    memcpy((void*) dev->port->command_table, (const void*) &cmd, sizeof(cmd));

    // Issue command
    port->ci = 0x01;

    // Wait for port to return that the command has finished
    uint32_t spin = 0;
    while ((port->ci & 0x1) != 0) {
        spin++;

        if (port->is & (1 << 30)) {
            printke("error while reading");
            return false;
        }

        if (spin > SPIN_WAIT) {
            printk("Port appears to be stuck during read for sata dev: %d", dev->id);
            return false;
        }
    }

    if (port->is & (1 << 30)) {
        printke("Error attempting to read from sata device id: %d", dev->id);
        return false;
    }

    memcpy(buf, dev->port->data_base, dev->port->data_base_size);

    return true;
}

bool sata_write_device(sata_device_t* dev, uint32_t block_num_l, uint16_t block_num_h, uint8_t* buf) {
    // Send write command
    HBA_port_t* port = dev->port->port;

    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->port->command_list;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    hdr->c = 1;
    // dev->port->command_list->w = 1; // TODO: I would expect this here
    HBA_cmd_table_t* tbl = dev->port->command_table;

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_WRITE_DMA_EXT;
    cmd.countl = dev->port->data_base_size / SATA_BLOCK_SIZE;

    cmd.lba0 = (block_num_l >> 0) & 0xFF;
    cmd.lba1 = (block_num_l >> 8) & 0xFF;
    cmd.lba2 = (block_num_l >> 16) & 0xFF;
    cmd.device = (1 << 6); // lba48 mode
    cmd.lba3 = (block_num_l >> 24) & 0xFF;
    cmd.lba4 = (block_num_h >> 0) & 0xFF;
    cmd.lba5 = (block_num_h >> 8) & 0xFF;

    memcpy(dev->port->data_base, buf, dev->port->data_base_size);

    // Move command to table
    memcpy((void*) tbl, (const void*) &cmd, sizeof(cmd));

    // Issue command
    port->ci = 0x01;

    // Wait for port to return that the command has finished
    uint32_t spin = 0;
    while (port->ci != 0x0) {
        spin++;
        if (spin > SPIN_WAIT) {
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

static void sata_read_block(fs_t* fs, uint32_t block, uint8_t* buf) {
    sata_device_t* dev = fs->device.underlying_device;
    sata_read_device(dev, 2*block, 0, buf);
}

static void sata_write_block(fs_t* fs, uint32_t block, uint8_t* buf) {
    sata_device_t* dev = fs->device.underlying_device;
    printk("writing to dev %s, block %d", dev->model_name, 2*block);
    sata_write_device(dev, 2*block, 0, buf);
}

static void sata_clear_block(fs_t* fs, uint32_t block) {
    uint8_t zeroes[SATA_BLOCK_SIZE] = {0};
    sata_device_t* dev = fs->device.underlying_device;
    sata_write_device(dev, 2*block, 0, zeroes);
}

fs_device_t sata_to_fs_device(sata_device_t* dev) {
    fs_device_t fs_dev;

    fs_dev.underlying_device = dev;
    fs_dev.read_block = sata_read_block;
    fs_dev.write_block = sata_write_block;
    fs_dev.clear_block = sata_clear_block;

    return fs_dev;
}

sata_device_t* sata_get_device_by_name(const char* name) {
    sata_device_t* dev;

    list_for_each_entry(dev, &sata_devs) {
        if (!strcmp(dev->model_name, name)) {
            return dev;
        }
    }

    return NULL;
}