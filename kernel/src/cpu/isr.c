#include <kernel/fpu.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/sys.h>

#include <assert.h>
#include <stdlib.h>

static char* exception_msgs[] = {
    "Division By Zero",
    "Debugger",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bounds",
    "Invalid Opcode",
    "Coprocessor Not Available",
    "Double fault",
    "Coprocessor Segment Overrun",
    "Invalid Task State Segment",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "Math Fault",
    "Alignement Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

static handler_t isr_handlers[256];

/* This file is responsible for handling software interrupts, ISRs.
 */

/* Points reserved IDT entries to our ISR handlers, along with the entry for the
 * syscall interrupt.
 * ISR handlers are defined in `isr.S` and all call `isr_handler`, defined here.
 */
void init_isr() {
    // 0x08 is the GDT selector for the kernel code segment
    idt_set_entry(0, (uint32_t) isr0, 0x08, IDT_INT_KERNEL);
    idt_set_entry(1, (uint32_t) isr1, 0x08, IDT_INT_KERNEL);
    idt_set_entry(2, (uint32_t) isr2, 0x08, IDT_INT_KERNEL);
    idt_set_entry(3, (uint32_t) isr3, 0x08, IDT_INT_KERNEL);
    idt_set_entry(4, (uint32_t) isr4, 0x08, IDT_INT_KERNEL);
    idt_set_entry(5, (uint32_t) isr5, 0x08, IDT_INT_KERNEL);
    idt_set_entry(6, (uint32_t) isr6, 0x08, IDT_INT_KERNEL);
    idt_set_entry(7, (uint32_t) isr7, 0x08, IDT_INT_KERNEL);
    idt_set_entry(8, (uint32_t) isr8, 0x08, IDT_INT_KERNEL);
    idt_set_entry(9, (uint32_t) isr9, 0x08, IDT_INT_KERNEL);
    idt_set_entry(10, (uint32_t) isr10, 0x08, IDT_INT_KERNEL);
    idt_set_entry(11, (uint32_t) isr11, 0x08, IDT_INT_KERNEL);
    idt_set_entry(12, (uint32_t) isr12, 0x08, IDT_INT_KERNEL);
    idt_set_entry(13, (uint32_t) isr13, 0x08, IDT_INT_KERNEL);
    idt_set_entry(14, (uint32_t) isr14, 0x08, IDT_INT_KERNEL);
    idt_set_entry(15, (uint32_t) isr15, 0x08, IDT_INT_KERNEL);
    idt_set_entry(16, (uint32_t) isr16, 0x08, IDT_INT_KERNEL);
    idt_set_entry(17, (uint32_t) isr17, 0x08, IDT_INT_KERNEL);
    idt_set_entry(18, (uint32_t) isr18, 0x08, IDT_INT_KERNEL);
    idt_set_entry(19, (uint32_t) isr19, 0x08, IDT_INT_KERNEL);
    idt_set_entry(20, (uint32_t) isr20, 0x08, IDT_INT_KERNEL);
    idt_set_entry(21, (uint32_t) isr21, 0x08, IDT_INT_KERNEL);
    idt_set_entry(22, (uint32_t) isr22, 0x08, IDT_INT_KERNEL);
    idt_set_entry(23, (uint32_t) isr23, 0x08, IDT_INT_KERNEL);
    idt_set_entry(24, (uint32_t) isr24, 0x08, IDT_INT_KERNEL);
    idt_set_entry(25, (uint32_t) isr25, 0x08, IDT_INT_KERNEL);
    idt_set_entry(26, (uint32_t) isr26, 0x08, IDT_INT_KERNEL);
    idt_set_entry(27, (uint32_t) isr27, 0x08, IDT_INT_KERNEL);
    idt_set_entry(28, (uint32_t) isr28, 0x08, IDT_INT_KERNEL);
    idt_set_entry(29, (uint32_t) isr29, 0x08, IDT_INT_KERNEL);
    idt_set_entry(30, (uint32_t) isr30, 0x08, IDT_INT_KERNEL);
    idt_set_entry(31, (uint32_t) isr31, 0x08, IDT_INT_KERNEL);

    // Syscall interrupt gate
    idt_set_entry(48, (uint32_t) isr48, 0x08, IDT_INT_USER);
}

/* Calls the handler registered to a specific interrupt, if any.
 * This function is called from the real interrupt handlers set up in
 * `init_isr`, defined in `isr.S`.
 */
void isr_handler(registers_t* regs) {
    assert(regs->int_no < 256);

    fpu_kernel_enter();

    if (isr_handlers[regs->int_no]) {
        handler_t handler = isr_handlers[regs->int_no];
        handler(regs);
    } else {
        printke("unhandled %s %d: %s",
            regs->int_no < 32 ? "exception" : "interrupt", regs->int_no,
            regs->int_no < 32 ? exception_msgs[regs->int_no] : "Unknown");
        // TODO: we're better than this
        abort();
    }

    fpu_kernel_exit();
}

/* Registers a handler to be called when interrupt `num` fires.
 */
void isr_register_handler(uint32_t num, handler_t handler) {
    assert(num < 256);

    if (isr_handlers[num]) {
        printke("interrupt handler %d already registered", num,
            exception_msgs[num]);
    } else {
        isr_handlers[num] = handler;
    }
}
