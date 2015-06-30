#include <kernel/isr.h>
#include <stdio.h>

void isr_handler(registers_t* regs) {
	printf("Interrupt number %d was triggered\n", regs->int_no);
}
