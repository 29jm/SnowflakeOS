#include <stdio.h>
#include <stdlib.h>

#include <kernel/syscall.h>
#include <kernel/isr.h>
#include <kernel/sys.h> // for UNUSED macro

#include <kernel/proc.h>
#include <kernel/timer.h>

static void syscall_handler(registers_t* regs);

static void syscall_yield(registers_t* regs);
static void syscall_exit(registers_t* regs);
static void syscall_wait(registers_t* regs);

sys_handler_t syscall_handlers[SYSCALL_NUM] = { 0 };

void init_syscall() {
	isr_register_handler(48, &syscall_handler);

	syscall_handlers[0] = syscall_yield;
	syscall_handlers[1] = syscall_exit;
	syscall_handlers[2] = syscall_wait;
}

static void syscall_handler(registers_t* regs) {
	if (syscall_handlers[regs->eax]) {
		sys_handler_t handler = syscall_handlers[regs->eax];
		handler(regs);
	} else {
		printf("Unknown syscall %d\n", regs->eax);
	}
}

/* Convention:
 * - Arguments shall be passed in this order:
 *  ebx, ecx, edx,
 *  and if more are needed, ebp shall contain a pointer to them.
 * - Values shall be returned in eax, then if needed in the
 *  same order as the arguments
 */

static void syscall_yield(registers_t* regs) {
	proc_switch_process(regs);
}

static void syscall_exit(registers_t* regs) {
	UNUSED(regs);

	printf("[SYS] Exit process !\n");

	proc_exit_current_process();
}

static void syscall_wait(registers_t* regs) {
	double wait_time = (double)regs->ebx;
	double t_0 = timer_get_time();

	while (wait_time - (timer_get_time() - t_0) > 0) {
		// nop. We could yield, but how long would that take?
	}
}