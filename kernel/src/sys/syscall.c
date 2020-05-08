#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/fb.h>
#include <kernel/wm.h>
#include <kernel/sys.h> // for UNUSED macro

#include <stdio.h>
#include <stdlib.h>

#include <kernel/uapi/uapi_syscall.h>

static void syscall_handler(registers_t* regs);

static void syscall_yield(registers_t* regs);
static void syscall_exit(registers_t* regs);
static void syscall_sleep(registers_t* regs);
static void syscall_putchar(registers_t* regs);
static void syscall_sbrk(registers_t* regs);
static void syscall_wm(registers_t* regs);
static void syscall_info(registers_t* regs);

handler_t syscall_handlers[SYSCALL_NUM] = { 0 };

void init_syscall() {
	isr_register_handler(48, &syscall_handler);

	syscall_handlers[SYS_YIELD] = syscall_yield;
	syscall_handlers[SYS_EXIT] = syscall_exit;
	syscall_handlers[SYS_SLEEP] = syscall_sleep;
	syscall_handlers[SYS_PUTCHAR] = syscall_putchar;
	syscall_handlers[SYS_SBRK] = syscall_sbrk;
	syscall_handlers[SYS_WM] = syscall_wm;
	syscall_handlers[SYS_INFO] = syscall_info;
}

static void syscall_handler(registers_t* regs) {
	if (regs->eax < SYS_MAX && syscall_handlers[regs->eax]) {
		handler_t handler = syscall_handlers[regs->eax];
		regs->eax = 0;
		handler(regs);
	} else {
		printf("[SYS] Unknown syscall %d\n", regs->eax);
	}
}

/* Convention:
 * - Syscall number in eax,
 * - Arguments shall be passed in this order: ebx, ecx, edx, esi,
 * - If more are needed, pack them into a struct pointer,
 * - Values shall be returned first in eax, then in user-given pointers.
 */

static void syscall_yield(registers_t* regs) {
	UNUSED(regs);

	proc_switch_process();
}

static void syscall_exit(registers_t* regs) {
	UNUSED(regs);

	// TODO: exit status?
	proc_exit_current_process();
}

static void syscall_sleep(registers_t* regs) {
	uint32_t ms = regs->ebx;
	proc_sleep(ms);
}

static void syscall_putchar(registers_t* regs) {
	putchar((char) regs->ebx);
}

static void syscall_sbrk(registers_t* regs) {
	uint32_t size = regs->ebx;
	regs->eax = (uint32_t) proc_sbrk(size);
}

/* Sends a command to the window manager:
 *     int32_t syscall_wm(command, param);
 */
static void syscall_wm(registers_t* regs) {
	uint32_t cmd = regs->ebx;

	switch (cmd) {
		case WM_CMD_OPEN: {
				wm_param_open_t* param = (wm_param_open_t*) regs->ecx;
				regs->eax = wm_open_window(param->fb, param->flags);
			} break;
		case WM_CMD_CLOSE:
			wm_close_window(regs->ecx);
			break;
		case WM_CMD_RENDER: {
				wm_param_render_t* param = (wm_param_render_t*) regs->ecx;
				wm_render_window(param->win_id, param->clip);
			} break;
		case WM_CMD_INFO: {
				fb_t* fb = (fb_t*) regs->ecx;
				*fb = fb_get_info();
			} break;
		case WM_CMD_EVENT: {
				wm_param_event_t* param = (wm_param_event_t*) regs->ecx;
				wm_get_event(param->win_id, param->event);
		} break;
		default:
			printf("[WM] Wrong command: %d\n", cmd);
			regs->eax = -1;
			break;
	}
}

/* Returns known system information.
 * TODO: improve.
 */
static void syscall_info(registers_t* regs) {
	regs->eax = memory_usage();
}