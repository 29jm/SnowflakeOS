#ifndef PROC_H
#define PROC_H

#include <kernel/isr.h>

#include <stdint.h>

typedef struct _proc_t {
    struct _proc_t* next;
    uint32_t pid;
    // Length of program memory in number of pages
    uint32_t stack_len;
    uint32_t code_len;
    uintptr_t directory;
    uintptr_t kernel_stack;
    registers_t registers;
} process_t;

void init_proc();
void proc_run_code(uint8_t* code, int len);
void proc_print_processes();
void proc_timer_callback(registers_t* regs);
void proc_switch_process(registers_t* regs);
void proc_exit_current_process();

#endif