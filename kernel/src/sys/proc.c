#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/timer.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/gdt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

process_t* current_process;

void init_proc() {
	proc_switch_process(NULL);
}

void proc_run_code(uint8_t* code, int len) {
	// TODO: implement and use a kmalloc that returns the physical address.
	// That way hand-mapping them wouldn't be necessary.
	process_t* process = kmalloc(sizeof(process_t));
	uintptr_t pd_phys = (uintptr_t)pmm_alloc_page();
	uintptr_t code_phys = pmm_alloc_aligned_large_page();
	uintptr_t stack_phys = pmm_alloc_aligned_large_page();

	// Copy the kernel page directory
	paging_map_page(0xB0000000, pd_phys, PAGE_RW); // Temporary mapping
	memcpy((void*)0xB0000000, (void*)0xFFFFF000, 4096);
	// Map our code and stack pages (they are 4 MiB pages)
	directory_entry_t* pd = (directory_entry_t*)0xB0000000;
	pd[0] = code_phys | PAGE_PRESENT | PAGE_USER | PAGE_RW | PAGE_LARGE;
	pd[767] = stack_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_LARGE;
	paging_unmap_page(0xB0000000); // Remove the temporary mapping

	// Write the given code to memory. TODO: compute number of pages to allocate
	paging_map_page(0xB0000000, code_phys, PAGE_RW);
	memcpy((void*)0xB0000000, code, len);
	paging_unmap_page(0xB0000000); // todo: zero stack page?

	*process = (process_t) {
		.pid = 42, // TODO: dynamic PID attribution
		.code_len = 1024,
		.stack_len = 1024,
		.directory = pd_phys,
		.registers = (registers_t) {
			.eip = 0x00000000,
			.esp = 0xBFFFFFFB,
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

	printf("Process chain: 0x%X -> ", current_process);

	while (p != current_process) {
		printf("0x%X -> ", p);
		p = p->next;
	}

	printf("\n");
}

void proc_exit_current_process() {
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

	kfree(current_process);
	
	// We set the current process to the one right before the one we want
	// because `proc_switch_process` will grab current_process->next
	current_process = p;

	proc_switch_process(NULL);
}

/* Will be used to implement parallel-looking execution
 */
void proc_timer_tic_handler(registers_t* regs) {
	if (!current_process || current_process->next == current_process) {
		return;
	}

	if (current_process->next) {
		proc_switch_process(regs);
	}
}

/* Saves the execution context of the current process, then switches execution
 * to the next process in the list.
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

	// Remember the correct kernel stack
	uintptr_t esp0 = 0;
	asm volatile("mov %%esp, %0\n" : "=r"(esp0));
	gdt_set_kernel_stack(esp0);

	// Switch back to the process' page directoy
	paging_switch_directory(current_process->directory);

	// Setup the stack as if we were coming from usermode because of an interrupt,
	// then interrupt-return to usermode. We make sure to push the correct value
	// value for %eip
	asm volatile (
		"push $0x23\n" // data segment selector
		"push $0xBFFFFFFB\n" // %esp
		"push $512\n" // %eflags: the 9th bit is supposed to allow cli/sti
		"push $0x1B\n" // code segment selector
	);

	uint32_t eip_val = current_process->registers.eip;

	asm volatile (
		"mov %0, %%eax\n"
		"push %%eax\n"
		:                 // ouput registers
		: "r" (eip_val)   // input registers
		: "%eax");        // now invalid registers

	asm volatile (
		"iret\n"
	);
}