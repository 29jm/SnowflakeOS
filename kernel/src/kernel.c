#include <kernel/ext2.h>
#include <kernel/fb.h>
#include <kernel/fpu.h>
#include <kernel/fs.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/multiboot2.h>
#include <kernel/paging.h>
#include <kernel/pci.h>
#include <kernel/ahci.h>
#include <kernel/sata.h>
#include <kernel/pmm.h>
#include <kernel/proc.h>
#include <kernel/ps2.h>
#include <kernel/ramfs.h>
#include <kernel/serial.h>
#include <kernel/stacktrace.h>
#include <kernel/sys.h>
#include <kernel/syscall.h>
#include <kernel/term.h>
#include <kernel/timer.h>
#include <kernel/wm.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t KERNEL_BEGIN_PHYS;
extern uint32_t KERNEL_END_PHYS;
extern uint32_t KERNEL_SIZE;

void kernel_main(mb2_t* boot, uint32_t magic) {
    init_serial();
    init_fpu();
    init_pmm(boot);
    init_paging(boot);

    if (magic != MB2_MAGIC) {
        printke("invalid magic number from GRUB (%p), ignoring...", magic);
    }

    /* Text mode also requires commenting out the graphics tag in boot.S */
    bool no_graphics = false;

    init_term();

    if (no_graphics) {
        printk("starting in text mode...");
    } else {
        printk("starting in graphical mode...");
        init_fb(boot);
        init_wm();
    }

    printk("SnowflakeOS 0.75 ");
    printk("kernel is %d KiB", ((uint32_t) &KERNEL_SIZE) >> 10);

    init_gdt();
    init_idt();
    init_isr();
    init_irq();

    // init_pci();
    // init_ahci();

    init_syscall();
    init_timer();
    init_ps2();

    /* Load GRUB modules: the disk image, and symbol file for stacktraces */
    mb2_tag_t* tag = boot->tags;

    while (tag->type != MB2_TAG_END) {
        if (tag->type == MB2_TAG_MODULE) {
            mb2_tag_module_t* mod = (mb2_tag_module_t*) tag;
            uint32_t size = mod->mod_end - mod->mod_start;
            char* module_name = (char*) mod->name;

            uint8_t* data = (uint8_t*) kmalloc(size);
            memcpy(data, (void*) mod->mod_start, size);

            if (!strcmp(module_name, "disk")) {
                init_fs(init_ext2(ramfs_new(data, size)));
            } else if (!strcmp(module_name, "symbols")) {
                init_stacktrace(data, size);
            }

            printk("loaded module %s", mod->name);
        }

        tag = (mb2_tag_t*) ((uintptr_t) tag + align_to(tag->size, 8));
    }

    // sata_device_t* dev = sata_get_device_by_name("TESTDRIVE");

    // if (dev) {
    //     init_fs(init_ext2(sata_to_fs_device(dev)));
    // }

    init_proc();

    proc_exec("/background", NULL);
    proc_exec("/terminal", NULL);

    proc_enter_usermode();
}
