#ifndef PROC_H
#define PROC_H

#include <stdint.h>

#define PROC_KERNEL_STACK_PAGES 10 // In pages

// Add new members to the end to avoid messing with the offsets
typedef struct _proc_t {
    struct _proc_t* next;
    uint32_t pid;
    // Length of program memory in number of pages
    uint32_t stack_len;
    uint32_t code_len;
    uintptr_t directory;
    uintptr_t kernel_stack;
    uintptr_t esp;
    uint32_t mem_len;
} process_t;

void init_proc();
void proc_run_code(uint8_t* code, uint32_t len);
void proc_print_processes();
void proc_timer_callback();
void proc_exit_current_process();
void proc_enter_usermode();
void proc_switch_process();
uint32_t proc_get_current_pid();
uintptr_t proc_alloc_pages(uint32_t num);

#endif
