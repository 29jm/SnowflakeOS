#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// `flags`
#define IDT_TYPE_INT 0xE // 32 bit interrupt gate. Interrupts will be disabled
#define IDT_DPL(n) (n << 5) // Required provilege level
#define IDT_PRESENT (1 << 7) // Always set

// Used IDT flags
#define IDT_INT_KERNEL (IDT_TYPE_INT | IDT_DPL(0) | IDT_PRESENT)
#define IDT_INT_USER (IDT_TYPE_INT | IDT_DPL(3) | IDT_PRESENT)

// An IDT entry is structured as follows:
// |base 16:31|flags 0:7|0 0:7|selector 0:15|base 0:15|
// where `flags` is |P|DPL 0:2|S|Type 0:3|
typedef struct {
	uint16_t base_low;
	uint16_t selector; // Code segment selector
	uint8_t zero;
	uint8_t flags;
	uint16_t base_high;
} __attribute__ ((packed)) idt_entry_t;

typedef struct {
	uint16_t size;
	uint32_t offset;
} __attribute__ ((packed)) idt_pointer_t;

void init_idt();
void idt_set_entry(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);

#endif
