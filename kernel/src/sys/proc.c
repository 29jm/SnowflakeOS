#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/timer.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/gdt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static process_t* current_process;
static uint32_t next_pid = 1;

void init_proc() {
	timer_register_callback(&proc_timer_callback);

	proc_switch_process(NULL);
}

void proc_run_code(uint8_t* code, int len) {
	// TODO: implement and use a kmalloc that returns the physical address.
	// That way hand-mapping them wouldn't be necessary.
	process_t* process = kmalloc(sizeof(process_t));
	uintptr_t pd_phys = pmm_alloc_page();
	uintptr_t code_phys = pmm_alloc_aligned_large_page();
	uintptr_t stack_phys = pmm_alloc_aligned_large_page();
	// Don't forget that the stack grows backwards, hence the `+ ...`
	uintptr_t kernel_stack = (uintptr_t)kmalloc(1024*4) + 1024*4; // This ought to be enough for any process...

	// Copy the kernel page directory
	paging_map_page(KERNEL_HEAP_VIRT_MAX, pd_phys, PAGE_RW); // Temporary mapping after the kernel heap
	memcpy((void*)KERNEL_HEAP_VIRT_MAX, (void*)0xFFFFF000, 4096);
	// Map our code and stack pages (they are 4 MiB pages)
	directory_entry_t* pd = (directory_entry_t*)KERNEL_HEAP_VIRT_MAX;
	pd[0] = code_phys | PAGE_PRESENT | PAGE_USER | PAGE_RW | PAGE_LARGE;
	pd[767] = stack_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_LARGE;
	paging_unmap_page(KERNEL_HEAP_VIRT_MAX); // Remove the temporary mapping

	// Write the given code to memory. TODO: compute number of pages to allocate
	paging_map_page(KERNEL_HEAP_VIRT_MAX, code_phys, PAGE_RW);
	memcpy((void*)KERNEL_HEAP_VIRT_MAX, code, len);
	paging_unmap_page(KERNEL_HEAP_VIRT_MAX); // todo: zero stack page?

	*process = (process_t) {
		.pid = next_pid++,
		.code_len = 1024,
		.stack_len = 1024,
		.directory = pd_phys,
		.kernel_stack = kernel_stack, // TODO: check if regs->esp could be used
		.registers = (registers_t) {
			.eip = 0x00000000,
			.useresp = 0xBFFFFFFB,
			.cs = 0x1B,
			.ds = 0x23,
		}
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
}

void proc_print_processes() {
	process_t* p = current_process->next;

	printf("[PROC] Process chain: %d -> ", current_process->pid);

	while (p != current_process) {
		printf("%d -> ", p->pid);
		p = p->next;
	}

	printf("(loop)\n");
}

void proc_exit_current_process() {
	printf("[PROC] Terminating process %d\n", current_process->pid);

	directory_entry_t* pd = (directory_entry_t*) 0xFFFFF000;

	uintptr_t code_page = pd[0] & PAGE_FRAME;
	uintptr_t stack_page = pd[767] & PAGE_FRAME;
	uintptr_t pd_page = pd[1023] & PAGE_FRAME;

	pmm_free_pages(code_page, current_process->code_len); // Large pages
	pmm_free_pages(stack_page, current_process->stack_len);
	pmm_free_page(pd_page);

	// Remove the process from our circular list
	process_t* next_current = current_process->next;

	if (next_current == current_process) {
		printf("[PROC] Killed last process alive\n");
		abort();
	}

	process_t* p = next_current;

	while (p->next != current_process)
		p = p->next;

	p->next = next_current;

	// We set the current process to the one right before the one we want
	// because `proc_switch_process` will grab current_process->next
	current_process = p;

	proc_switch_process(NULL);
}

/* Switches process on clock tick.
 */
void proc_timer_callback(registers_t* regs) {
	if (!current_process || current_process->next == current_process) {
		return;
	}

	if (current_process->next) {
		proc_switch_process(regs);
	}
}

/* Saves the execution context `regs` of the current process, then switches
 * execution to the next process in the list.
 * Passing `regs = NULL` can be useful when the old process is of no interest.
 */
void proc_switch_process(registers_t* regs) {
	if (regs) {
		current_process->registers = *regs;
	}

	current_process = current_process->next;

	if (!current_process) {
		printf("[PROC] No next process in queue\n");
		abort();
	}

	// Print the current PID in the bottom right of the screen
	printf("\x1B[s\x1B[24;78H");
	printf("\x1B[K");
	printf("%d", current_process->pid);
	printf("\x1B[u");

	// This sets the stack pointer that will be used when an inter-privilege
	// interrupt happens
	gdt_set_kernel_stack(current_process->kernel_stack);

	// Switch back to the process' page directoy
	paging_switch_directory(current_process->directory);

	// Setup the stack as if we were coming from usermode because of an interrupt,
	// then interrupt-return to usermode. We make sure to push the correct value
	// value for %esp and %eip
	uint32_t esp_val = current_process->registers.useresp;
	uint32_t eip_val = current_process->registers.eip;

	asm volatile (
		"push $0x23\n"    // user ds selector
		"mov %0, %%eax\n"
		"push %%eax\n"    // %esp
		"push $512\n"     // %eflags with `IF` bit set, equivalent to calling `sti`
		"push $0x1B\n"    // user cs selector
		"mov %1, %%eax\n"
		"push %%eax\n"    // %eip
		"iret\n"
		:                 /* read registers: none */
		: "r" (esp_val), "r" (eip_val) /* inputs %0 and %1 stored anywhere */
		: "%eax"          /* registers clobbered by hand in there */
	);
}