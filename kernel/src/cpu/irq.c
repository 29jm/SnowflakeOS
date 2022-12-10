#include <kernel/irq.h>
#include <kernel/idt.h>
#include <kernel/com.h>
#include <kernel/sys.h>

#include <string.h>

static handler_t irq_handlers[16];

/* This file is responsible for handling hardware interrupts, IRQs.
 */

/* Points IDT entries 32 to 47 to our IRQ handlers.
 * Those are defined in `irq.S`, and are responsible for calling `irq_handler`
 * below.
 */
void init_irq() {
    irq_remap();

    // 0x08 is the GDT selector for the kernel code segment
    idt_set_entry(32, (uint32_t) irq0, 0x08, IDT_INT_KERNEL);
    idt_set_entry(33, (uint32_t) irq1, 0x08, IDT_INT_KERNEL);
    idt_set_entry(34, (uint32_t) irq2, 0x08, IDT_INT_KERNEL);
    idt_set_entry(35, (uint32_t) irq3, 0x08, IDT_INT_KERNEL);
    idt_set_entry(36, (uint32_t) irq4, 0x08, IDT_INT_KERNEL);
    idt_set_entry(37, (uint32_t) irq5, 0x08, IDT_INT_KERNEL);
    idt_set_entry(38, (uint32_t) irq6, 0x08, IDT_INT_KERNEL);
    idt_set_entry(39, (uint32_t) irq7, 0x08, IDT_INT_KERNEL);
    idt_set_entry(40, (uint32_t) irq8, 0x08, IDT_INT_KERNEL);
    idt_set_entry(41, (uint32_t) irq9, 0x08, IDT_INT_KERNEL);
    idt_set_entry(42, (uint32_t) irq10, 0x08, IDT_INT_KERNEL);
    idt_set_entry(43, (uint32_t) irq11, 0x08, IDT_INT_KERNEL);
    idt_set_entry(44, (uint32_t) irq12, 0x08, IDT_INT_KERNEL);
    idt_set_entry(45, (uint32_t) irq13, 0x08, IDT_INT_KERNEL);
    idt_set_entry(46, (uint32_t) irq14, 0x08, IDT_INT_KERNEL);
    idt_set_entry(47, (uint32_t) irq15, 0x08, IDT_INT_KERNEL);

    // We mask every interrupt, unmask them as needed
    for (uint32_t irq = IRQ0; irq <= IRQ15; irq++) {
        // Masking that IRQ prevents PIC2 from raising IRQs
        if (irq == IRQ2) {
            continue;
        }

        irq_mask(irq);
    }

    STI();
}

/* Calls the handler associated with the calling IRQ, if any.
 */
void irq_handler(registers_t* regs) {
    uint32_t irq = regs->int_no;

    // Handle spurious interrupts
    if (irq == IRQ7 || irq == IRQ15) {
        uint16_t isr = irq_get_isr();

        // If it's a real interrupt, the corresponding bit will be set in the ISR
        // Otherwise, it was probably caused by a solar flare, so do not send EOI
        // If faulty IRQ was sent from the slave PIC, still send EOI to the master
        if (!(isr & (1 << (irq - IRQ0)))) {
            if (irq == IRQ15) {
                irq_send_eoi(IRQ0); // Sort of hackish
            }

            return;
        }
    }

    irq_send_eoi(irq);

    handler_t handler = irq_handlers[irq - IRQ0];

    if (handler) {
        handler(regs);
    }
    else {
        printke("unhandled IRQ%d", irq - IRQ0);
    }

}

void irq_send_eoi(uint8_t irq) {
    // Send EOI to PIC2 if necessary
    if (irq > IRQ7) {
        outportb(PIC2_CMD, PIC_EOI);
    }

    // Send to PIC1
    outportb(PIC1_CMD, PIC_EOI);
}

/* Registers a function that'll be called when `irq` is raised by a PIC.
 * There can only be one per IRQ; subsequent calls will do nothing.
 */
void irq_register_handler(uint8_t irq, handler_t handler) {
    if (irq < IRQ0 || irq > IRQ15) {
        printke("tried to register a handler for an invalid IRQ: %d", irq);
        return;
    }

    if (!irq_handlers[irq - IRQ0]) {
        irq_handlers[irq - IRQ0] = handler;
    } else {
        printke("IRQ %d is already registered", irq);
    }

    irq_unmask(irq);
}

void irq_remap() {
    outportb(PIC1_CMD, PIC_RESTART);
    outportb(PIC2_CMD, PIC_RESTART);

    // Remap the vector offset to 32, so that IRQs are called with
    // numbers within 32-47.
    outportb(PIC1_DATA, IRQ0);
    outportb(PIC2_DATA, IRQ8);

    // Setup cascading
    outportb(PIC1_DATA, 4);
    outportb(PIC2_DATA, 2);

    // Finish the setup?
    outportb(PIC1_DATA, 1);
    outportb(PIC2_DATA, 1);
}

uint16_t irq_get_isr() {
    outportb(PIC1, PIC_ISR);
    outportb(PIC2, PIC_ISR);

    return (inportb(PIC2) << 8) | inportb(PIC1);
}

uint8_t irq_get_mask(uint16_t pic) {
    return inportb(pic);
}

void irq_set_mask(uint16_t pic, uint8_t mask) {
    outportb(pic, mask);
}

/* Masks `irq`, where `irq` is in the `IRQ0-IRQ15` range.
 */
void irq_mask(uint32_t irq) {
    uint16_t pic = 0;

    if (irq < IRQ8) {
        pic = PIC1_DATA;
    } else {
        pic = PIC2_DATA;
        irq -= 8; // from range 8-15 to 0-7
    }

    uint8_t mask = irq_get_mask(pic);
    mask |= (1 << (irq - IRQ0));

    irq_set_mask(pic, mask);
}

/* Unmasks an interrupt; see `irq_mask`.
 */
void irq_unmask(uint32_t irq) {
    uint16_t pic = 0;

    if (irq < IRQ8) {
        pic = PIC1_DATA;
    } else {
        pic = PIC2_DATA;
        irq -= 8;
    }

    uint16_t mask = irq_get_mask(pic);
    mask &= ~(1 << (irq - IRQ0));

    irq_set_mask(pic, mask);
}
