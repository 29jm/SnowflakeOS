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
static void syscall_putchar(registers_t* regs);

sys_handler_t syscall_handlers[SYSCALL_NUM] = { 0 };

void init_syscall() {
	isr_register_handler(48, &syscall_handler);

	syscall_handlers[0] = syscall_yield;
	syscall_handlers[1] = syscall_exit;
	syscall_handlers[2] = syscall_wait;
	syscall_handlers[3] = syscall_putchar;
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
	proc_switch_process();
}

static void syscall_exit(registers_t* regs) {
	UNUSED(regs);

	proc_exit_current_process();
}

static void syscall_wait(registers_t* regs) {
	// TODO
	/* This must be implemented by the scheduler, not here:
	 * - IRQs don't fire while in a syscall, so we can't rely on the timer
	 *   increasing
	 * - We can't both task switch and come back to this handler to check
	 *   the time
	 */
}

static void syscall_putchar(registers_t* regs) {
	putchar((char)regs->ebx);
}