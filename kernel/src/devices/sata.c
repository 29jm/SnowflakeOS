#include <kernel/ahci.h>
#include <kernel/paging.h>
#include <kernel/sata.h>
#include <kernel/sys.h>

#include <list.h>
#include <stdlib.h>
#include <math.h>

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
    uint8_t* id_data = kamalloc(PAGE_SIZE, PAGE_SIZE);
    HBA_port_t* port = dev->port->port;

    dev->port->command_list->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    dev->port->command_list->prdtl = 1;

    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.countl = 2;
    cmd.command = ATA_CMD_IDENTIFY_DEV;

    // Move command to the command table
    memcpy((void*) dev->port->command_table, (const void*) &cmd, sizeof(cmd));

    // Set PRDT
    dev->port->command_table->prdt_entry[0].dba = paging_virt_to_phys((uintptr_t) &id_data[0]);
    dev->port->command_table->prdt_entry[0].dbau = 0;
    dev->port->command_table->prdt_entry[0].dbc = 1024-1;
    dev->port->command_table->prdt_entry[0].i = 1;

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

    // Necessary for the data to show up
    paging_invalidate_page((uintptr_t) id_data);

    char* dest = dev->model_name;
    char* src = (char*) (id_data + SATA_DEV_MODEL_OFFSET);

    // Copy name to device struct, swap bits due to sata endianess
    for (unsigned int i = 0; i < sizeof(dev->model_name); i += 2) {
        dest[i] = src[i + 1];
        dest[i + 1] = src[i];

        if (src[i] < 0x21 || src[i] > 0x7E) // Not a printable character
            dest[i + 1] = 0;

        if (src[i + 1] < 0x21 || src[i + 1] > 0x7E) // Not a printable character
            dest[i] = 0;
    }

    uint32_t capacity = ((uint32_t*) id_data)[SATA_DEV_LBA_CAP_OFFSET / 4];

    dev->capacity_in_sectors = capacity;

    printk("identified SATA device: type %s named \"%s\", capacity: 0x%x blocks",
        sata_types[dev->type], dev->model_name, dev->capacity_in_sectors);

    kfree(id_data);

    return true;
}

/* Reads `size` bytes from `dev` into `buf` starting with block `blk_h:blk_l`,
 * a 48-bit number (LBA48). `buf` must be located in pages with caching disabled.
 * Returns the number of bytes read.
 *
 * Note:
 * - `size` is limited to `AHCI_PRDT_SIZE`, basically a page.
 * - No out of bounds checks are performed, make sure to read within the disk.
 */
uint32_t sata_read_device(sata_device_t* dev, uint32_t blk_l, uint16_t blk_h, uint32_t size, uint8_t* buf) {
    // Check that we can handle the size
    if (size > AHCI_PRDT_SIZE) {
        printke("invalid read: size too big");
        return 0;
    }

    // Set command header
    dev->port->command_list->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    dev->port->command_list->a = 0;
    dev->port->command_list->w = 0;
    dev->port->command_list->p = 0;
    dev->port->command_list->r = 0;
    dev->port->command_list->b = 0;
    dev->port->command_list->c = 1;
    dev->port->command_list->pmp = 0;
    dev->port->command_list->prdtl = divide_up(size, AHCI_PRDT_SIZE);

    // Set command FIS
    FIS_reg_h2d_t cmd = {0};
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_READ_DMA_EXT;
    cmd.countl = divide_up(size, SATA_BLOCK_SIZE);
    cmd.device = (1 << 6); // LBA48 mode
    cmd.lba0 = (blk_l >> 0)  & 0xFF;
    cmd.lba1 = (blk_l >> 8)  & 0xFF;
    cmd.lba2 = (blk_l >> 16) & 0xFF;
    cmd.lba3 = (blk_l >> 24) & 0xFF;
    cmd.lba4 = (blk_h >> 0)  & 0xFF;
    cmd.lba5 = (blk_h >> 8)  & 0xFF;

    // Move command to table
    memcpy((void*) dev->port->command_table, (const void*) &cmd, sizeof(cmd));

    // Set PRDTs to cover `buf[0:size]`
    uint32_t phys_buf = paging_virt_to_phys((uintptr_t) buf);
    uint32_t remaining = size;
    for (uint32_t i = 0; i < dev->port->command_list->prdtl; i++) {
        uint32_t prdt_size = min(remaining, AHCI_PRDT_SIZE);
        dev->port->command_table->prdt_entry[i].dba = phys_buf;
        dev->port->command_table->prdt_entry[i].dbau = 0;
        dev->port->command_table->prdt_entry[i].dbc = prdt_size-1;
        dev->port->command_table->prdt_entry[i].i = 1;
        phys_buf += prdt_size;
        remaining -= prdt_size;
    }

    // Issue command
    dev->port->port->ci = 0x01;

    // Wait for port to return that the command has finished
    uint32_t spin = 0;
    while ((dev->port->port->ci & 0x1) != 0) {
        if (spin++ > SPIN_WAIT) {
            printk("port appears to be stuck during read for sata dev: %d", dev->id);
            return 0;
        }
    }

    if (dev->port->port->is & (1 << 30)) {
        printke("error attempting to read from sata device id: %d", dev->id);
        return 0;
    }

    uint32_t bytes_read = dev->port->command_list->prdbc;

    if (bytes_read != size) {
        printke("%d bytes read, %d bytes expected", bytes_read, size);
    }

    return bytes_read;
}

uint32_t sata_write_device(sata_device_t* dev, uint32_t blk_l, uint16_t blk_h, uint32_t size, uint8_t* buf) {
    // Send write command
    HBA_port_t* port = dev->port->port;
    HBA_cmd_hdr_t* hdr = (HBA_cmd_hdr_t*) dev->port->command_list;
    hdr->cfl = sizeof(FIS_reg_h2d_t) / sizeof(uint32_t);
    hdr->c = 1;
    // dev->port->command_list->w = 1; // TODO: I would expect this here
    HBA_cmd_table_t* tbl = dev->port->command_table;

    FIS_reg_h2d_t cmd = { 0 };
    cmd.fis_type = FIS_TYPE_REG_H2D;
    cmd.c = 1;
    cmd.command = ATA_CMD_WRITE_DMA_EXT;
    cmd.device = (1 << 6); // LBA48 mode
    cmd.countl = divide_up(size, SATA_BLOCK_SIZE);
    cmd.lba0 = (blk_l >> 0)  & 0xFF;
    cmd.lba1 = (blk_l >> 8)  & 0xFF;
    cmd.lba2 = (blk_l >> 16) & 0xFF;
    cmd.lba3 = (blk_l >> 24) & 0xFF;
    cmd.lba4 = (blk_h >> 0)  & 0xFF;
    cmd.lba5 = (blk_h >> 8)  & 0xFF;

    // Set PRDTs to cover `buf[0:size]`
    uint32_t phys_buf = paging_virt_to_phys((uintptr_t) buf);
    uint32_t remaining = size;
    for (uint32_t i = 0; i < dev->port->command_list->prdtl; i++) {
        uint32_t prdt_size = min(remaining, AHCI_PRDT_SIZE);
        dev->port->command_table->prdt_entry[i].dba = phys_buf;
        dev->port->command_table->prdt_entry[i].dbau = 0;
        dev->port->command_table->prdt_entry[i].dbc = prdt_size-1;
        dev->port->command_table->prdt_entry[i].i = 1;
        phys_buf += prdt_size;
        remaining -= prdt_size;
    }

    // Move command to table
    memcpy((void*) tbl, (const void*) &cmd, sizeof(cmd));

    // Issue command
    port->ci = 0x01;

    // Wait for port to return that the command has finished
    uint32_t spin = 0;
    while (port->ci != 0x0) {
        if (spin++ > SPIN_WAIT) {
            printk("Port appears to be stuck during write for sata dev: %d", dev->id);
            return 0;
        }
    }

    if (port->is & (1 << 30)) {
        printke("Error attempting to write to sata device id: %d", dev->id);
        return 0;
    }

    uint32_t bytes_read = dev->port->command_list->prdbc;

    if (bytes_read != size) {
        printke("%d bytes written, %d bytes expected", bytes_read, size);
    }

    return bytes_read;
}

static void sata_read_block(fs_t* fs, uint32_t block, uint8_t* buf) {
    sata_device_t* dev = fs->device.underlying_device;
    sata_read_device(dev, 2*block, 0, 1024, buf);
}

static void sata_write_block(fs_t* fs, uint32_t block, uint8_t* buf) {
    sata_device_t* dev = fs->device.underlying_device;
    printk("writing to dev %s, block %d", dev->model_name, 2*block);
    sata_write_device(dev, 2*block, 0, 1024, buf);
}

static void sata_clear_block(fs_t* fs, uint32_t block) {
    uint8_t zeroes[SATA_BLOCK_SIZE] = {0};
    sata_device_t* dev = fs->device.underlying_device;
    sata_write_device(dev, 2*block, 0, 1024, zeroes);
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