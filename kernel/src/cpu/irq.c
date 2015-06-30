#include <kernel/irq.h>
#include <kernel/idt.h>
#include <kernel/com.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

static handler_t irq_handlers[256];

void init_irq() {
	irq_remap();

	memset(irq_handlers, 0, 256*sizeof(handler_t));

	idt_set_entry(32, (uint32_t) irq0, 0x08, 0x8E);
	idt_set_entry(33, (uint32_t) irq1, 0x08, 0x8E);
	idt_set_entry(34, (uint32_t) irq2, 0x08, 0x8E);
	idt_set_entry(35, (uint32_t) irq3, 0x08, 0x8E);
	idt_set_entry(36, (uint32_t) irq4, 0x08, 0x8E);
	idt_set_entry(37, (uint32_t) irq5, 0x08, 0x8E);
	idt_set_entry(38, (uint32_t) irq6, 0x08, 0x8E);
	idt_set_entry(39, (uint32_t) irq7, 0x08, 0x8E);
	idt_set_entry(40, (uint32_t) irq8, 0x08, 0x8E);
	idt_set_entry(41, (uint32_t) irq9, 0x08, 0x8E);
	idt_set_entry(42, (uint32_t) irq10, 0x08, 0x8E);
	idt_set_entry(43, (uint32_t) irq11, 0x08, 0x8E);
	idt_set_entry(44, (uint32_t) irq12, 0x08, 0x8E);
	idt_set_entry(45, (uint32_t) irq13, 0x08, 0x8E);
	idt_set_entry(46, (uint32_t) irq14, 0x08, 0x8E);
	idt_set_entry(47, (uint32_t) irq15, 0x08, 0x8E);
}

void irq_handler(registers_t* regs) {
	CLI();

	// Don't annoy the PIC with software interrupts
	if (regs->int_no >= IRQ0 && regs->int_no <= IRQ15) {
		// Received from slave
		if (regs->int_no > IRQ7) {
			outportb(PIC2_CMD, PIC_EOI);
		}

		// Not a bug: EOIs must be sent to both master and slave if the
		// interrupt was on a slave line
		outportb(PIC1_CMD, PIC_EOI);
	}

	if (irq_handlers[regs->int_no]) {
		handler_t handler = irq_handlers[regs->int_no];
		handler(regs);
	}

	STI();
}

void irq_register_handler(uint8_t irq, handler_t handler) {
	CLI();

	if (!irq_handlers[irq]) {
		irq_handlers[irq] = handler;
	}
	else {
		printf("IRQ %d already registered\n", irq);
	}

	STI();
}

void irq_remap() {
	outportb(PIC1_CMD, PIC_RESTART);
	outportb(PIC2_CMD, PIC_RESTART);

	// Remap the vector offset to 32, so that IRQs are called with
	// numbers within 32-47.
	outportb(PIC1_DATA, 32);
	outportb(PIC2_DATA, 40);

	// Setup cascading
	outportb(PIC1_DATA, 4);
	outportb(PIC2_DATA, 2);

	// Finish the setup?
	outportb(PIC1_DATA, 1);
	outportb(PIC2_DATA, 1);
}

uint16_t irq_getmask(uint16_t pic) {
	return inports(pic);
}

void irq_setmask(uint16_t pic, uint16_t mask) {
	outports(pic, mask);
}

// Sets an IRQ number to hide
void irq_mask(uint8_t irq) {
	uint16_t pic = 0;

	if (irq < 8) {
		pic = PIC1_DATA;
	}
	else {
		pic = PIC2_DATA;
		irq -= 8; // from range 8-15 to 0-7
	}

	uint16_t mask = irq_getmask(pic);
	mask |= (1 << irq);
	irq_setmask(pic, mask);
}

void irq_unmask(uint8_t irq) {
	uint16_t pic = 0;
	
	if (irq < 8) {
		pic = PIC1_DATA;
	}
	else {
		pic = PIC2_DATA;
		irq -= 8;
	}

	uint16_t mask = irq_getmask(pic);
	mask &= ~(1 << mask);
	irq_setmask(pic, mask);
}
