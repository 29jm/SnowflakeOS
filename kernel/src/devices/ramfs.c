#include <kernel/ramfs.h>
#include <kernel/sys.h>
#include <stdlib.h>
#include <string.h>

static void read_block(fs_t* fs, uint32_t block, uint8_t* buf) {
    ramfs_t* dev = fs->device.underlying_device;
    memcpy(buf, dev->data + block*1024, 1024);
    // printk("_ramfs_read:");
    // dbg_buffer_dump(buf, 1024);
}

static void write_block(fs_t* fs, uint32_t block, uint8_t* buf) {
    ramfs_t* dev = fs->device.underlying_device;
    printk("fswrite: block %d", block);
    memcpy(dev->data + block*1024, buf, 1024);
}

static void clear_block(fs_t* fs, uint32_t block) {
    ramfs_t* dev = fs->device.underlying_device;
    memset(dev->data + block*1024, 0, 1024);
}

fs_device_t ramfs_new(uint8_t* data, uint32_t size) {
    ramfs_t* ramfs_dev = kmalloc(sizeof(ramfs_t));

    ramfs_dev->data = data;
    ramfs_dev->size = size;

    return (fs_device_t) {
        .underlying_device = ramfs_dev,
        .read_block = read_block,
        .write_block = write_block,
        .clear_block = clear_block
    };
}