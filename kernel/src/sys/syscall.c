#include <stdio.h>

#include <kernel/syscall.h>
#include <kernel/isr.h>

static void syscall_handler(registers_t* regs);

sys_handler_t syscall_handlers[256];

void init_syscall() {
	isr_register_handler(48, &syscall_handler);
}

static void syscall_handler(registers_t* regs) {
	if (syscall_handlers[regs->eax]) {
		sys_handler_t handler = syscall_handlers[regs->eax];
		handler(regs);
	}
	else {
		printf("Unknown syscall %d\n", regs->eax);
	}
}
