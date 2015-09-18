#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <kernel/isr.h>

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

void isr_handler(registers_t* regs) {
	assert(regs->int_no < 256);

	if (isr_handlers[regs->int_no]) {
		handler_t handler = isr_handlers[regs->int_no];
		handler(regs);
	}
	else {
		printf("Unhandled hardware exception %d: %s\n", regs->int_no,
			regs->int_no < 32 ? exception_msgs[regs->int_no] : "Unknown");
		abort();
	}
}

void isr_register_handler(uint32_t num, handler_t handler) {
	assert(num < 256);

	if (isr_handlers[num]) {
		printf("Exception handler %d (%s) already registered\n", num,
			exception_msgs[num]);
	}
	else {
		isr_handlers[num] = handler;
	}
}
