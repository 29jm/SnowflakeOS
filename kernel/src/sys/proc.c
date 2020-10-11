#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/gdt.h>
#include <kernel/fpu.h>
#include <kernel/fs.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char* name;
	uint8_t* code;
	uint32_t size;
} program_t;

extern uint32_t irq_handler_end;

process_t* current_process = 0;

static uint32_t next_pid = 1;
static list_t* programs;

void init_proc() {
	timer_register_callback(&proc_timer_callback);

	if (!current_process) {
		printf("[PROC] No program to start\n");
		abort();
	}

	printf("[PROC] Initial process: %d\n", current_process->pid);
	proc_enter_usermode();
}

/* Creates a process running the code specified at `code` in raw instructions
 * and add it to the process queue, after the currently executing process.
 */
void proc_run_code(uint8_t* code, uint32_t size) {
	static uintptr_t temp_page = 0;

	if (!temp_page) {
		temp_page = (uintptr_t) kamalloc(0x1000, 0x1000);
	}

	// Allocate one page more than the program size to accomodate static
	// variables. TODO: fix, critical. -> ELF loader?
	uint32_t num_code_pages = divide_up(size, 0x1000) + 1;
	uint32_t num_stack_pages = PROC_KERNEL_STACK_PAGES;

	process_t* process = kmalloc(sizeof(process_t));
	uintptr_t kernel_stack = (uintptr_t) aligned_alloc(4, 0x1000) + 0x1000 - 4;
	uintptr_t pd_phys = pmm_alloc_page();

	// Copy the kernel page directory with a temporary mapping
	page_t* p = paging_get_page(temp_page, false, 0);
	*p = pd_phys | PAGE_PRESENT | PAGE_RW;
	memcpy((void*) temp_page, (void*) 0xFFFFF000, 0x1000);
	directory_entry_t* pd = (directory_entry_t*) temp_page;
	pd[1023] = pd_phys | PAGE_PRESENT | PAGE_RW;

	// ">> 22" grabs the address's index in the page directory, see `paging.c`
	for (uint32_t i = 0; i < (KERNEL_BASE_VIRT >> 22); i++) {
		pd[i] = 0; // Unmap everything below the kernel
	}

	// We can now switch to that directory to modify it easily
	uintptr_t previous_pd = *paging_get_page(0xFFFFF000, false, 0) & PAGE_FRAME;
	paging_switch_directory(pd_phys);

	// Map the code and copy it to physical pages, zero out the excess memory
	// for static variables
	// TODO: don't require contiguous pages
	uintptr_t code_phys = pmm_alloc_pages(num_code_pages);
	paging_map_pages(0x00001000, code_phys, num_code_pages, PAGE_USER | PAGE_RW);
	memcpy((void*) 0x00001000, (void*) code, size);
	memset((uint8_t*) 0x1000 + size, 0, num_code_pages * 0x1000 - size);

	// Map the stack
	uintptr_t stack_phys = pmm_alloc_pages(num_stack_pages);
	paging_map_pages(0xC0000000 - 0x1000 * num_stack_pages, stack_phys,
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
		.mem_len = 0,
		.sleep_ticks = 0,
		.fds = list_new()
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
		"push $0x00001000\n"   // %eip
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

	// Free the file descriptor list
	while (current_process->fds->count) {
		fs_close((uint32_t) list_first(current_process->fds));
		list_remove_at(current_process->fds, 0);
	}

	kfree(current_process->fds);

	// Remove the process from our circular list
	process_t* next_current = current_process->next;

	if (next_current == current_process) {
		printf("[PROC] Killed last process alive\n");
		abort();
	}

	process_t* p = current_process;

	while (p->next != current_process) {
		p = p->next;
	}

	p->next = next_current;

	proc_switch_process();
}

/* Switches process on clock tick.
 */
void proc_timer_callback(registers_t* regs) {
	UNUSED(regs);

	process_t* p = current_process;

	// Avoid switching to a sleeping process if possible
	do {
		if (p->next->sleep_ticks > 0) {
			p->next->sleep_ticks--;
		} else {
			// We don't need to switch process
			if (p->next == current_process) {
				return;
			}

			// We don't need to modify the process queue
			if (p->next == current_process->next) {
				break;
			}

			// We insert the next process between the current one and the one
			// previously scheduled to be switched to.
			process_t* previous = p;
			process_t* next_proc = p->next;
			process_t* moved = current_process->next;

			previous->next = next_proc->next;
			next_proc->next = moved;
			current_process->next = next_proc;

			break;
		}

		p = p->next;
	} while (p != current_process);

	fpu_switch(current_process, current_process->next);
	proc_switch_process();
}

/* Make the first jump to usermode.
 * A special function is needed as our first kernel stack isn't setup to return
 * to any interrupt handler; we have to `iret` ourselves.
 */
void proc_enter_usermode() {
	gdt_set_kernel_stack(current_process->kernel_stack);
	paging_switch_directory(current_process->directory);

	asm volatile (
		"mov $0x23, %eax\n"
		"mov %eax, %ds\n"
		"mov %eax, %es\n"
		"mov %eax, %fs\n"
		"mov %eax, %gs\n"
		"push %eax\n"        // %ss
		"push $0xBFFFFFFB\n" // %esp
		"push $512\n"        // %eflags
		"push $0x1B\n"       // %cs
		"push $0x00001000\n" // %eip
		"iret\n"
	);
}

uint32_t proc_get_current_pid() {
	return current_process->pid;
}

void proc_sleep(uint32_t ms) {
	current_process->sleep_ticks = (uint32_t) ((ms*TIMER_FREQ)/1000.0);
	proc_timer_callback(NULL);
}

/* Extends the program's writeable memory by `size` bytes.
 * Note: the real granularity is by the page, but the program doesn't need the
 * details.
 */
void* proc_sbrk(intptr_t size) {
	uintptr_t end = 0x1000 + 0x1000*current_process->code_len + current_process->mem_len;

	// Bytes available in the last allocated page
	int32_t remaining_bytes = (end % 0x1000) ? (0x1000 - (end % 0x1000)) : 0;

	if (size > 0) {
		// We have to allocate more pages
		if (remaining_bytes < size) {
			uint32_t needed_size = size - remaining_bytes;
			uint32_t num = divide_up(needed_size, 0x1000);

			if (!paging_alloc_pages(align_to(end, 0x1000), num)) {
				return (void*) -1;
			}
		}
	} else if (size < 0) {
		if (end + size < 0x1000*current_process->code_len) {
			return (void*) -1; // Can't deallocate the code
		}

		int32_t taken = 0x1000 - remaining_bytes;

		// We must free at least a page
		if (taken + size < 0) {
			uint32_t freed_size = taken - size; // Account for sign
			uint32_t num = divide_up(freed_size, 0x1000);

			// Align to the beginning of the last mapped page
			uintptr_t virt = end - (end % 0x1000);

			for (uint32_t i = 0; i < num; i++) {
				paging_unmap_page(virt - 0x1000*i);
			}
		}
	}

	current_process->mem_len += size;

	return (void*) end;
}

void proc_register_program(char* name, uint8_t* code, uint32_t size) {
	if (!programs) {
		programs = list_new();
	}

	program_t* prog = kmalloc(sizeof(program_t));

	prog->name = strdup(name);
	prog->code = code;
	prog->size = size;

	list_add(programs, prog);

	// printf("[PROC] Added program %s\n", prog->name);
}

int32_t proc_exec(const char* name) {
	for (uint32_t i = 0; i < programs->count; i++) {
		program_t* prog = list_get_at(programs, i);

		if (!strcmp(prog->name, name)) {
			printf("[proc] exec program %s\n", name);
			proc_run_code(prog->code, prog->size);

			return 0;
		}
	}

	return -1;
}

/* Returns whether the current process posesses the file descriptor.
 * TODO: include mode check?
 */
bool proc_has_fd(uint32_t fd) {
	return list_get_index_of(current_process->fds, (void*) fd) < current_process->fds->count;
}

uint32_t proc_open(const char* path, uint32_t mode) {
	uint32_t fd = fs_open(path, mode);

	if (fd != FS_INVALID_FD) {
		list_add_front(current_process->fds, (void*) fd);
	}

	return fd;
}

void proc_close(uint32_t fd) {
	uint32_t idx = list_get_index_of(current_process->fds, (void*) fd);

	if (idx < current_process->fds->count) {
		list_remove_at(current_process->fds, idx);
	}

	fs_close(fd);
}