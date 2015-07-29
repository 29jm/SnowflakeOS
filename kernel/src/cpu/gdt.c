#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/paging.h>

#include <stdint.h>

static gdt_entry_t gdt_entries[5];
static gdt_entry_ptr_t gdt_ptr;
static idt_entry_t idt_entries[256];
static idt_entry_ptr_t idt_entry_ptr;

void init_gdt() {
	gdt_ptr.limit = (sizeof(gdt_entry_t)*5) - 1;
	gdt_ptr.base = (uint32_t) &gdt_entries;
	gdt_set_entry(0, 0, 0, 0, 0);                // Null segment
	gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
	gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
	gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
	gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

	gdt_load((uint32_t) &gdt_ptr);
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
