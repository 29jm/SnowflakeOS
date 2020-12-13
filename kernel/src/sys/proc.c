#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/gdt.h>
#include <kernel/fpu.h>
#include <kernel/fs.h>
#include <kernel/pipe.h>
#include <kernel/sys.h>

#include <kernel/sched_robin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t irq_handler_end;

process_t* current_process = NULL;
sched_t* scheduler = NULL;

static uint32_t next_pid = 1;

void init_proc() {
    scheduler = sched_robin();
}

/* Creates a process running the code specified at `code` in raw instructions
 * and add it to the process queue, after the currently executing process.
 * `argv` is the array of arguments, NULL terminated.
 */
process_t* proc_run_code(uint8_t* code, uint32_t size, char** argv) {
    static uintptr_t temp_page = 0;

    if (!temp_page) {
        temp_page = (uintptr_t) kamalloc(0x1000, 0x1000);
    }

    // Save arguments before switching directory and losing them
    list_t args = LIST_HEAD_INIT(args);

    while (argv && *argv) {
        list_add_front(&args, strdup(*argv));
        argv++;
    }

    // Allocate one page more than the program size to accomodate static
    // variables.
    // TODO: this assumes .bss sections are marked as progbits
    uint32_t num_code_pages = divide_up(size, 0x1000);
    uint32_t num_stack_pages = PROC_STACK_PAGES;

    process_t* process = kmalloc(sizeof(process_t));
    uintptr_t kernel_stack = (uintptr_t) aligned_alloc(4, 0x1000 * PROC_KERNEL_STACK_PAGES);
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

    /* Setup the (argc, argv) part of the userstack, start by copying the given
     * arguments on that stack. */
    list_t arglist = LIST_HEAD_INIT(arglist);
    char* ustack_char = (char*) (0xC0000000 - 1);

    char* arg;
    list_for_each_entry(arg, &args) {
        uint32_t len = strlen(arg);

        // We need (ustack_char - len) to be 4-bytes aligned
        ustack_char -= ((uintptr_t) ustack_char - len) % 4;
        char* dest = ustack_char - len;

        strncpy(dest, arg, len);
        ustack_char -= len + 1; // Keep pointing to a free byte

        list_add(&arglist, (void*) dest);
        kfree(arg);
    }

    /* Write `argv` to the stack with the pointers created previously.
     * Note that we switch to an int pointer; we're writing addresses here. */
    uint32_t* ustack_int = (uint32_t*) ((uintptr_t) ustack_char & ~0x3);
    uint32_t arg_count = 0;

    list_for_each_entry(arg, &arglist) {
        *(ustack_int--) = (uintptr_t) arg;
        arg_count++;
    }

    // Push program arguments
    uintptr_t argsptr = (uintptr_t) (ustack_int + 1);
    *(ustack_int--) = arg_count ? argsptr : (uintptr_t) NULL;
    *(ustack_int--) = arg_count;

    // Switch to the original page directory
    paging_switch_directory(previous_pd);

    *process = (process_t) {
        .pid = next_pid++,
        .code_len = num_code_pages,
        .stack_len = num_stack_pages,
        .directory = pd_phys,
        .kernel_stack = kernel_stack + PROC_KERNEL_STACK_PAGES * 0x1000 - 4,
        .saved_kernel_stack = kernel_stack + PROC_KERNEL_STACK_PAGES * 0x1000 - 4,
        .initial_user_stack = (uintptr_t) ustack_int,
        .mem_len = 0,
        .sleep_ticks = 0,
        .filetable = LIST_HEAD_INIT(process->filetable),
        .cwd = strdup("/")
    };

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
        "mov %[ustack], %%eax\n"
        "push %%eax\n"         // %esp
        "push $0x202\n"        // %eflags with `IF` bit set
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
        "push $1\n"
        "push $2\n"
        "push $3\n"
        "push $4\n"

        // Save the new process's %esp in %eax
        "mov %%esp, %%eax\n"
        // Restore our stack
        "mov %%ebx, %%esp\n"
        // Update the new process's %esp
        "mov %%eax, %[esp]\n"
        : [esp] "=r" (process->saved_kernel_stack)
        : [kstack] "r" (process->kernel_stack),
          [ustack] "r" (process->initial_user_stack),
          [jmp] "r" (jmp)
        : "%eax", "%ebx"
    );

    scheduler->sched_add(scheduler, process);

    return process;
}

/* Terminates the currently executing process.
 * Implements the `exit` system call.
 */
void proc_exit_current_process() {
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

    // Free the kernel stack
    kfree((void*) (current_process->kernel_stack - 0x1000 * PROC_KERNEL_STACK_PAGES + 4));

    // Free the file descriptor list
    while (!list_empty(&current_process->filetable)) {
        ft_entry_t* ent = list_first_entry(&current_process->filetable, ft_entry_t);
        fs_close(ent->inode);
        list_del(list_first(&current_process->filetable));
    }

    // This last line is actually safe, and necessary
    scheduler->sched_exit(scheduler, current_process);
    proc_schedule();
}

/* Runs the scheduler. The scheduler may then decide to elect a new process, or
 * not.
 */
void proc_schedule() {
    process_t* next = scheduler->sched_next(scheduler);

    if (next == current_process) {
        return;
    }

    fpu_switch(current_process, next);
    proc_switch_process(next);
}

/* Called on clock ticks, calls the scheduler.
 */
void proc_timer_callback(registers_t* regs) {
    UNUSED(regs);

    proc_schedule();
}

/* Make the first jump to usermode.
 * A special function is needed as our first kernel stack isn't setup to return
 * to any interrupt handler; we have to `iret` ourselves.
 */
void proc_enter_usermode() {
    CLI(); // Interrupts will be reenabled by `iret`

    current_process = scheduler->sched_get_current(scheduler);

    if (!current_process) {
        printke("no process to run");
        abort();
    }

    timer_register_callback(&proc_timer_callback);
    gdt_set_kernel_stack(current_process->kernel_stack);
    paging_switch_directory(current_process->directory);

    asm volatile (
        "mov $0x23, %%eax\n"
        "mov %%eax, %%ds\n"
        "mov %%eax, %%es\n"
        "mov %%eax, %%fs\n"
        "mov %%eax, %%gs\n"
        "push %%eax\n"        // %ss
        "mov %[ustack], %%eax\n"
        "push %%eax\n"       // %esp
        "push $0x202\n"      // %eflags with IF set
        "push $0x1B\n"       // %cs
        "push $0x00001000\n" // %eip
        "iret\n"
        :: [ustack] "r" (current_process->initial_user_stack)
        : "%eax");
}

/* Returns the filetable entry associated with fd, if any.
 */
ft_entry_t* proc_fd_to_entry(uint32_t fd) {
    ft_entry_t* ent;

    list_for_each_entry(ent, &current_process->filetable) {
        if (ent->fd == fd) {
            return ent;
        }
    }

    return NULL;
}

void proc_add_fd(ft_entry_t* entry) {
    if (!proc_fd_to_entry(entry->fd)) {
        list_add_front(&current_process->filetable, entry);
    }
}

/* Returns a new and unused fd for the current process.
 * TODO: make it use the lowest fd available.
 */
uint32_t proc_next_fd() {
    static uint32_t avail = 3;

    return avail++;
}

uint32_t proc_get_current_pid() {
    if (current_process) {
        return current_process->pid;
    } else {
        return 0;
    }
}

/* Returns a dynamically allocated copy of the current process's current working
 * directory.
 */
char* proc_get_cwd() {
    return strdup(current_process->cwd);
}

void proc_sleep(uint32_t ms) {
    current_process->sleep_ticks = (uint32_t) ((ms*TIMER_FREQ)/1000.0);
    proc_schedule();
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

int32_t proc_exec(const char* path, char** argv) {
    /* Read the executable */
    inode_t* in = fs_open(path, O_RDONLY);

    if (!in || in->type != DENT_FILE) {
        return -1;
    }

    uint8_t* data = kmalloc(in->size);
    uint32_t read = fs_read(in, 0, data, in->size);

    if (read == in->size && in->size) {
        process_t* p = proc_run_code(data, in->size, argv);

        /* If the process to be executed has a parent that has a terminal,
         * make it this process's terminal too */
        if (proc_get_current_pid() != 0) {
            ft_entry_t* stdout = proc_fd_to_entry(1);

            if (stdout) {
                stdout->inode = pipe_clone(stdout->inode);
                list_add_front(&p->filetable, stdout);
            }
        }
    } else {
        printke("exec failed while reading the executable");
        return -1;
    }

    return 0;
}

uint32_t proc_open(const char* path, uint32_t flags) {
    inode_t* in = fs_open((char*) path, flags); // TODO

    if (in) {
        ft_entry_t* ent = kmalloc(sizeof(ft_entry_t));
        *ent = (ft_entry_t) {
            .fd = proc_next_fd(),
            .inode = in,
            .mode = 0, // TODO: what's this?
            .offset = 0,
            .size = in->size,
            .index = 0
        };

        list_add_front(&current_process->filetable, ent);

        return ent->fd;
    }

    return 0;
}

void proc_close(uint32_t fd) {
    list_t* iter;
    ft_entry_t* ent;
    list_for_each(iter, ent, &current_process->filetable) {
        if (ent->fd == fd) {
            // TODO: At some point, we'll want to have a fs-layer writelock
            fs_close(ent->inode);
            list_del(iter);
            return;
        }
    }
}

uint32_t proc_read(uint32_t fd, uint8_t* buf, uint32_t size) {
    ft_entry_t* ent = proc_fd_to_entry(fd);

    if (ent) {
        uint32_t read = fs_read(ent->inode, ent->offset, buf, size);
        ent->offset += read;
        return read;
    }

    return 0;
}

int32_t proc_readdir(uint32_t fd, sos_directory_entry_t* dent) {
    ft_entry_t* ent = proc_fd_to_entry(fd);

    if (ent) {
        uint32_t read = fs_readdir(ent->inode, ent->index, dent, dent->entry_size);
        ent->offset += read;

        if (read) {
            ent->index++;
            return 1;
        }

        return 0;
    }

    return -1;
}

uint32_t proc_write(uint32_t fd, uint8_t* buf, uint32_t size) {
    ft_entry_t* ent = proc_fd_to_entry(fd);

    if (ent) {
        uint32_t written = fs_write(ent->inode, buf, size);
        ent->offset += written;
        return written;
    }

    return 0;
}

int32_t proc_fseek(uint32_t fd, int32_t offset, uint32_t whence) {
    ft_entry_t* ent = proc_fd_to_entry(fd);

    if (!ent) {
        return -1;
    }

    switch (whence) {
    case SEEK_SET:
        ent->offset = offset;
        break;
    case SEEK_CUR:
        ent->offset += offset;
        break;
    case SEEK_END:
        ent->offset = ent->size + offset;
        break;
    default:
        return -1;
    }

    return 0;
}

int32_t proc_ftell(uint32_t fd) {
    ft_entry_t* ent = proc_fd_to_entry(fd);

    if (!ent) {
        return -1;
    }

    return ent->offset;
}

int32_t proc_chdir(const char* path) {
    // TODO: replace existence check with `stat` to validate path
    char* npath = fs_normalize_path(path);
    inode_t* in = fs_open(npath, O_RDONLY);

    if (in) {
        kfree(npath);
        return -1;
    }

    kfree(current_process->cwd);
    current_process->cwd = npath;

    return 0;
}