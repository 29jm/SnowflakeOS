#include <stdio.h>
#include <stdlib.h>

#include <kernel/syscall.h>
#include <kernel/isr.h>
#include <kernel/sys.h> // for UNUSED macro

#include <kernel/proc.h>

static void syscall_handler(registers_t* regs);

static void syscall_yield(registers_t* regs);
static void syscall_exit(registers_t* regs);

sys_handler_t syscall_handlers[SYSCALL_NUM] = { 0 };

void init_syscall() {
	isr_register_handler(48, &syscall_handler);

	syscall_handlers[0] = syscall_yield;
	syscall_handlers[1] = syscall_exit;
}

static void syscall_handler(registers_t* regs) {
	if (syscall_handlers[regs->eax]) {
		sys_handler_t handler = syscall_handlers[regs->eax];
		handler(regs);
	} else {
		printf("Unknown syscall %d\n", regs->eax);
	}
}

static void syscall_yield(registers_t* regs) {
	proc_switch_process(regs);
}

static void syscall_exit(registers_t* regs) {
	UNUSED(regs);

	proc_exit_current_process();
}