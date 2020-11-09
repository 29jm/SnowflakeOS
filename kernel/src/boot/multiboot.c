#include <kernel/multiboot.h>
#include <kernel/sys.h>

void multiboot_print_infos(multiboot_t* boot) {
    if (boot->flags & MULTIBOOT_FRAMEBUFFER) {
        printk("Framebuffer:\n");
        printk("\taddress: 0x%X\n", (uint32_t) boot->framebuffer.address);
        printk("\ttype: %d\n", boot->framebuffer.type);
        printk("\twidth: %d\n", boot->framebuffer.width);
        printk("\theight: %d\n", boot->framebuffer.height);
        printk("\tbpp: %d\n", boot->framebuffer.bpp);
        printk("\tpitch: %d\n", boot->framebuffer.pitch);
    }

    if (boot->flags & MULTIBOOT_FLAG_MEM) {
        printk("Memory: lower=%dkB upper=%dMB\n", boot->mem_lower, boot->mem_upper/1024);
    }

    if (boot->flags & MULTIBOOT_FLAG_CMDLINE) {
        printk("Command line: %s\n", (char*) boot->cmdline);
    }

    if (boot->flags & MULTIBOOT_FLAG_MODS) {
        printk("Modules: count=%d address=%X\n", boot->mods_count, boot->mods_addr);
    }

    if (boot->flags & MULTIBOOT_FLAG_MMAP) {
        printk("BIOS memory map: address=%X  size=%d\n", boot->mmap_addr, boot->mmap_length);
    }
}