#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/timer.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/gdt.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t irq_handler_end;

process_t* current_process;
static uint32_t next_pid = 1;

void init_proc() {
	timer_register_callback(&proc_timer_callback);

	proc_enter_usermode();
}

/* Creates a process running the code specified at `code` in raw instructions
 * and add it to the process queue, after the currently executing process.
 */
void proc_run_code(uint8_t* code, uint32_t len) {
	uint32_t num_code_pages = len / 4096 + 1;
	uint32_t num_stack_pages = PROC_KERNEL_STACK_PAGES;

	process_t* process = kmalloc(sizeof(process_t));
	uintptr_t kernel_stack = (uintptr_t) kmalloc(4096) + 4096 - 1;
	uintptr_t pd_phys = pmm_alloc_page();

	// Copy the kernel page directory with a temporary mapping after the heap
	paging_map_page(KERNEL_HEAP_VIRT_MAX, pd_phys, PAGE_RW);
	memcpy((void*) KERNEL_HEAP_VIRT_MAX, (void*) 0xFFFFF000, 4096);
	directory_entry_t* pd = (directory_entry_t*) KERNEL_HEAP_VIRT_MAX;
	pd[1023] = pd_phys | PAGE_PRESENT | PAGE_RW;

	// ">> 22" grabs the address's index in the page directory, see `paging.c`
	for (uint32_t i = 0; i < (KERNEL_BASE_VIRT >> 22); i++) {
		pd[i] = 0; // Unmap everything below the kernel
	}

	paging_unmap_page(KERNEL_HEAP_VIRT_MAX);

	// We can now switch to that directory to modify it easily
	uintptr_t previous_pd = *paging_get_page(0xFFFFF000, false, 0) & PAGE_FRAME;
	paging_switch_directory(pd_phys);

	// Map the code and copy it to physical pages
	// TODO: don't require contiguous pages
	uintptr_t code_phys = pmm_alloc_pages(num_code_pages);
	paging_map_pages(0x00000000, code_phys, num_code_pages,
		PAGE_USER | PAGE_RW);
	memcpy((void*) 0x00000000, (void*) code, len);

	// Map the stack
	uintptr_t stack_phys = pmm_alloc_pages(num_stack_pages);
	paging_map_pages(0xC0000000 - 4096*num_stack_pages, stack_phys,
		num_stack_pages, PAGE_USER | PAGE_RW);

	// Switch to the original page directory
	paging_switch_directory(previous_pd);

	*process = (process_t) {
		.pid = next_pid++,
		.code_len = num_code_pages,
		.stack_len = num_stack_pages,
		.directory = pd_phys,
		.kernel_stack = kernel_stack,
		.esp = kernel_stack,
		.mem_len = 0
	};

	// Insert the process in the ring, create it if empty
	if (current_process && current_process->next) {
		process_t* p = current_process->next;
		current_process->next = process;
		process->next = p;
	} else if (!current_process) {
		current_process = process;
		current_process->next = current_process;
	}

	// We use this label as the return address from `proc_switch_process`
	uint32_t* jmp = &irq_handler_end;

	// Setup the process's kernel stack as if it had already been interrupted
	asm volatile (
		// Save our stack in %ebx
		"mov %%esp, %%ebx\n"

		// Temporarily use the new process's kernel stack
		"mov %[kstack], %%eax\n"
		"mov %%eax, %%esp\n"

		// Stuff popped by `iret`
		"push $0x23\n"         // user ds selector
		"push $0xBFFFFFFB\n"   // %esp
		"push $512\n"          // %eflags with `IF` bit set, equivalent to calling `sti`
		"push $0x1B\n"         // user cs selector
		"push $0x00000000\n"   // %eip
		// Push error code, interrupt number
		"sub $8, %%esp\n"
		// `pusha` equivalent
		"sub $32, %%esp\n"
		// push data segment registers
		"mov $0x20, %%eax\n"
		"push %%eax\n"
		"push %%eax\n"
		"push %%eax\n"
		"push %%eax\n"

		// Push proc_switch_process's `ret` %eip
		"mov %[jmp], %%eax\n"
		"push %%eax\n"
		// Push garbage %ebx, %esi, %edi, %ebp
		"sub $16, %%esp\n"

		// Save the new process's %esp in %eax
		"mov %%esp, %%eax\n"
		// Restore our stack
		"mov %%ebx, %%esp\n"
		// Update the new process's %esp
		"mov %%eax, %[esp]\n"
		: [esp] "=r" (process->esp)
		: [kstack] "r" (kernel_stack),
		  [jmp] "r" (jmp)
		: "%eax", "%ebx"
	);
}

/* Prints the processes currently in the queue in execution order.
 * The second-printed process is the one currently executing.
 */
void proc_print_processes() {
	process_t* p = current_process->next;

	printf("[PROC] Process chain: %d -> ", current_process->pid);

	while (p != current_process) {
		printf("%d -> ", p->pid);
		p = p->next;
	}

	printf("(loop)\n");
}

/* Terminates the currently executing process.
 * Implements the `exit` system call.
 */
void proc_exit_current_process() {
	printf("[PROC] Terminating process %d\n", current_process->pid);

	// Free allocated pages: code, heap, stack, page directory
	directory_entry_t* pd = (directory_entry_t*) 0xFFFFF000;

	for (uint32_t i = 0; i < 768; i++) {
		if (!(pd[i] & PAGE_PRESENT)) {
			continue;
		}

		uintptr_t page = pd[i] & PAGE_FRAME;
		pmm_free_page(page);
	}

	uintptr_t pd_page = pd[1023] & PAGE_FRAME;
	pmm_free_page(pd_page);

	// Remove the process from our circular list
	process_t* next_current = current_process->next;

	if (next_current == current_process) {
		printf("[PROC] Killed last process alive\n");
		abort();
	}

	process_t* p = current_process;

	while (p->next != current_process)
		p = p->next;

	p->next = next_current;

	proc_switch_process();
}

/* Switches process on clock tick.
 */
void proc_timer_callback(registers_t* regs) {
	UNUSED(regs);

	if (current_process->next == current_process) {
		return;
	}

	proc_switch_process();
}

/* Make the first jump to usermode.
 * A special function is needed as our first kernel stack isn't setup to return
 * to any interrupt handler; we have to `iret` ourselves.
 */
void proc_enter_usermode() {
	gdt_set_kernel_stack(current_process->kernel_stack);
	paging_switch_directory(current_process->directory);

	asm (
		"mov $0x23, %eax\n"
		"mov %eax, %ds\n"
		"mov %eax, %es\n"
		"mov %eax, %fs\n"
		"mov %eax, %gs\n"
		"push %eax\n"        // %ss
		"push $0xBFFFFFFB\n" // %esp
		"push $512\n"        // %eflags
		"push $0x1B\n"       // %cs
		"push $0x00000000\n" // %eip
		"iret\n"
	);
}

uint32_t proc_get_current_pid() {
	return current_process->pid;
}

/* Extends the program's writeable memory by `num` pages.
 */
uintptr_t proc_alloc_pages(uint32_t num) {
	printf("[PROC] Allocating %d pages for process %d\n",
		num, current_process->pid);

	uintptr_t end = 0x1000*(current_process->code_len + current_process->mem_len);
	paging_alloc_pages(end, num);
	current_process->mem_len += num;

	return end;
}