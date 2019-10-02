#include <stdint.h>
#include <string.h>

#include <kernel/idt.h>

static idt_entry_t idt_entries[256];
static idt_pointer_t idt_ptr;

/* Load the IDT, without filling it.
 */
void init_idt() {
	idt_ptr.size = sizeof(idt_entry_t)*256 - 1;
	idt_ptr.offset = (uint32_t) &idt_entries;

	asm ("lidt (%0)\n" :: "r" (&idt_ptr));
}

/* Sets the desired interrupt gate to call a handler at address `base` with code
 * selector `selector`. Restricts the interrupt to kernel use depending on
 * the value of `flags`.
 * See `idt.h` for a description of the different flags.
 */
void idt_set_entry(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
	idt_entries[num].base_low = base & 0xFFFF;
	idt_entries[num].base_high = base >> 16;
	idt_entries[num].selector = selector;
	idt_entries[num].zero = 0;
	idt_entries[num].flags	= flags;
}
