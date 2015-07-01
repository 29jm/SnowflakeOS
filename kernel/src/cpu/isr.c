#include <kernel/isr.h>
#include <stdio.h>

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

void isr_handler(registers_t* regs) {
	printf("Hadrware Exception %d received: %s\n", regs->int_no, exception_msgs[regs->int_no]);
}
