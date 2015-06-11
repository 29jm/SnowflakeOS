#include <kernel/isr.h>
#include <stdio.h>

void isr_handler(registers_t* regs) {
	printf("You have been visited by M. Interruptel\n");
}
