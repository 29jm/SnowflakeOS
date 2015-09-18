#include <stdint.h>
#include <string.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>

static gdt_entry_t gdt_entries[6];
static gdt_entry_ptr_t gdt_ptr;

static tss_entry_t tss = {
	.ss0 = 0x10, /* Kernel Data Segment */
	.esp0 = 0,
	.es = 0x10,  /* Kernel Data Segment */
	.cs = 0x08,  /* Kernel Code Segment */
	.ds = 0x13,  /* Kernel Data Segment */
	.fs = 0x13,  /* Kernel Data Segment */
	.gs = 0x13   /* Kernel Data Segment */
};

void init_gdt() {
	gdt_ptr.limit = (sizeof(gdt_entry_t)*6) - 1;
	gdt_ptr.base = (uint32_t) &gdt_entries;

	gdt_set_entry(0, 0, 0, 0, 0);                // Null segment
	gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
	gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
	gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
	gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

	gdt_write_tss(5, 0x10, 0x00);

	gdt_load((uintptr_t) &gdt_ptr);

	asm("mov $0x2B, %ax\n"
	    "ltr %ax\n");
}

void gdt_set_entry(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
	gdt_entries[num].base_low = (base & 0xFFFF);
	gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	gdt_entries[num].base_high = (base >> 24) & 0xFF;

	gdt_entries[num].limit_low = (limit & 0xFFFF);
	gdt_entries[num].granularity = (limit >> 16) & 0x0F;

	gdt_entries[num].granularity |= granularity & 0xF0;
	gdt_entries[num].access	= access;
}

// Taken from ToaruOS
void gdt_write_tss(uint32_t num, uint32_t ss0, uint32_t esp0) {
	uintptr_t base = (uintptr_t) &tss;
	uintptr_t limit = base + sizeof(tss);

	/* Add the TSS descriptor to the GDT */
	gdt_set_entry(num, base, limit, 0xE9, 0x00);

	memset(&tss, 0x00, sizeof(tss));

	tss.ss0 = ss0;
	tss.esp0 = esp0;
	tss.cs = 0x0b;
	tss.ss = 0x13;
	tss.ds = 0x13;
	tss.es = 0x13;
	tss.fs = 0x13;
	tss.gs = 0x13;

	tss.iomap_base = sizeof(tss);
}

void gdt_set_kernel_stack(uintptr_t stack) {
	tss.esp0 = stack;
}
