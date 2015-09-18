#include <stdint.h>
#include <string.h>

#include <kernel/idt.h>

static idt_entry_t idt_entries[256];
static idt_entry_ptr_t idt_entry_ptr;

void init_idt() {
	idt_entry_ptr.limit = sizeof(idt_entry_t)*256 - 1;
	idt_entry_ptr.base = (uint32_t) &idt_entries;

	memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

	idt_set_entry(0, (uint32_t) isr0, 0x08, 0x8E);
	idt_set_entry(1, (uint32_t) isr1, 0x08, 0x8E);
	idt_set_entry(2, (uint32_t) isr2, 0x08, 0x8E);
	idt_set_entry(3, (uint32_t) isr3, 0x08, 0x8E);
	idt_set_entry(4, (uint32_t) isr4, 0x08, 0x8E);
	idt_set_entry(5, (uint32_t) isr5, 0x08, 0x8E);
	idt_set_entry(6, (uint32_t) isr6, 0x08, 0x8E);
	idt_set_entry(7, (uint32_t) isr7, 0x08, 0x8E);
	idt_set_entry(8, (uint32_t) isr8, 0x08, 0x8E);
	idt_set_entry(9, (uint32_t) isr9, 0x08, 0x8E);
	idt_set_entry(10, (uint32_t) isr10, 0x08, 0x8E);
	idt_set_entry(11, (uint32_t) isr11, 0x08, 0x8E);
	idt_set_entry(12, (uint32_t) isr12, 0x08, 0x8E);
	idt_set_entry(13, (uint32_t) isr13, 0x08, 0x8E);
	idt_set_entry(14, (uint32_t) isr14, 0x08, 0x8E);
	idt_set_entry(15, (uint32_t) isr15, 0x08, 0x8E);
	idt_set_entry(16, (uint32_t) isr16, 0x08, 0x8E);
	idt_set_entry(17, (uint32_t) isr17, 0x08, 0x8E);
	idt_set_entry(18, (uint32_t) isr18, 0x08, 0x8E);
	idt_set_entry(19, (uint32_t) isr19, 0x08, 0x8E);
	idt_set_entry(20, (uint32_t) isr20, 0x08, 0x8E);
	idt_set_entry(21, (uint32_t) isr21, 0x08, 0x8E);
	idt_set_entry(22, (uint32_t) isr22, 0x08, 0x8E);
	idt_set_entry(23, (uint32_t) isr23, 0x08, 0x8E);
	idt_set_entry(24, (uint32_t) isr24, 0x08, 0x8E);
	idt_set_entry(25, (uint32_t) isr25, 0x08, 0x8E);
	idt_set_entry(26, (uint32_t) isr26, 0x08, 0x8E);
	idt_set_entry(27, (uint32_t) isr27, 0x08, 0x8E);
	idt_set_entry(28, (uint32_t) isr28, 0x08, 0x8E);
	idt_set_entry(29, (uint32_t) isr29, 0x08, 0x8E);
	idt_set_entry(30, (uint32_t) isr30, 0x08, 0x8E);
	idt_set_entry(31, (uint32_t) isr31, 0x08, 0x8E);
	idt_set_entry(48, (uint32_t) isr48, 0x08, 0x8E);

	idt_load((uintptr_t) &idt_entry_ptr);
}

void idt_set_entry(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
	idt_entries[num].base_low = base & 0xFFFF;
	idt_entries[num].base_high = (base >> 16) & 0xFFFF;

	idt_entries[num].selector = selector;
	idt_entries[num].zero = 0;
	idt_entries[num].flags	= flags /* | 0x60 */;
}
