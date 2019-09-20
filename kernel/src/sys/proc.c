#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/timer.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/gdt.h>

#include <string.h>

process_t* current_process;

void init_proc() {
	process_t* kernel_proc = kmalloc(sizeof(process_t));

	*kernel_proc = (process_t) {
		.next = kernel_proc,
		.pid = 0,
		// .directory = paging_get_kernel_directory()
	};

	current_process = NULL;

	timer_register_callback(proc_timer_tic_handler);
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
		.directory = pd_phys,
	};

	// Insert the process in the ring
	if (current_process && current_process->next) {
		process_t* current_last = current_process->next;

		while (current_last->next != current_process) {
			current_last = current_last->next;
		}

		current_last->next = process;
		process->next = current_process;
	}

	// Switch to it
	proc_switch_process(process);
}

void proc_timer_tic_handler(registers_t* regs) {
	if (!current_process || current_process->next == current_process) {
		return;
	}

	current_process->registers = *regs;

	if (current_process->next) {
		proc_switch_process(current_process->next);
	}
}

void proc_switch_process(process_t* process) {
	// Remember the correct kernel stack
	uintptr_t esp0 = 0;
	asm volatile("mov %%esp, %0\n" : "=r"(esp0));
	gdt_set_kernel_stack(esp0);

	// Switch back to the process' page directoy
	paging_switch_directory(process->directory);

	// Setup the stack as if we were coming from usermode because of an interrupt,
	// then interrupt-return to usermode.
	asm volatile (
		"push $0x23\n" // data segment selector
		"push $0xBFFFFFFB\n" // %esp
		"push $512\n" // %eflags: the 9th bit is supposed to allow cli/sti
		"push $0x1B\n" // code segment selector
		"push $0x00000000\n" // %eip
		"iret\n"
	);
}