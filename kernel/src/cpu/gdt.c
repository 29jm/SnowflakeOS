#include <stdint.h>
#include <string.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>

static gdt_entry_t gdt_entries[6];
static gdt_pointer_t gdt_ptr;

static tss_entry_t tss;

/* Loads up and fills the GDT with all the entries we need.
 */
void init_gdt() {
    gdt_ptr.offset = (uint32_t) &gdt_entries;
    gdt_ptr.size = (sizeof(gdt_entries)) - 1;

    gdt_set_entry(0, 0, 0, 0, 0); // Null segment
    gdt_set_entry(1, 0, 0xFFFFFFFF, GDT_ACCESS_KERNEL_CODE, GDT_FLAGS);
    gdt_set_entry(2, 0, 0xFFFFFFFF, GDT_ACCESS_KERNEL_DATA, GDT_FLAGS);
    gdt_set_entry(3, 0, 0xFFFFFFFF, GDT_ACCESS_USER_CODE, GDT_FLAGS);
    gdt_set_entry(4, 0, 0xFFFFFFFF, GDT_ACCESS_USER_DATA, GDT_FLAGS);

    gdt_write_tss(5, 0x10, 0x00);

    gdt_load(&gdt_ptr);

    // 0x2B = 0+5*8bytes | 3 (bottom 2 bits control ring number)
    asm("mov $0x2B, %ax\n" // Flush the TSS
        "ltr %ax\n");
}

/* See `gdt.h` for some "explanation" of the parameters here.
 * For better information, look at the following wiki page:
 * https://wiki.osdev.org/GDT
 */
void gdt_set_entry(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_entries[num].base_low = (base & 0xFFFF); // Lower 16 bits
    gdt_entries[num].base_middle = (base >> 16) & 0xFF; // Middle 8 bits
    gdt_entries[num].base_high = base >> 24; // Upper 8 bits

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].limit_and_flags = (limit >> 16) & 0x0F; // `limit` part
    gdt_entries[num].limit_and_flags |= flags & 0xF0; // `flags` part

    gdt_entries[num].access = access;
}

/* Writes the GDT entry corresponding to a barebones TSS with a specific
 * data segment selector `ss0` and stack pointer `esp0`.
 * Shamelessly taken from ToaruOS :)
 */
void gdt_write_tss(uint32_t num, uint32_t ss0, uint32_t esp0) {
    uintptr_t base = (uintptr_t) &tss;
    uintptr_t limit = base + sizeof(tss);

    /* Add the TSS descriptor to the GDT */
    gdt_set_entry(num, base, limit, 0xE9, 0x00);

    memset(&tss, 0x00, sizeof(tss));

    tss.ss0 = ss0;
    tss.esp0 = esp0;
    tss.iomap_base = sizeof(tss);
}

/* Sets the stack pointer that will be used when the next interrupt happens.
 */
void gdt_set_kernel_stack(uintptr_t stack) {
    tss.esp0 = stack;
}
