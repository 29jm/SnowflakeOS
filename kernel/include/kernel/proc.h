#ifndef PROC_H
#define PROC_H

#include <kernel/isr.h>

#include <stdint.h>

typedef struct _proc_t {
    struct _proc_t* next;
    uint32_t pid;
    uintptr_t directory;
    registers_t registers;
} process_t;

void init_proc();
void proc_run_code(uint8_t* code, int len);
void proc_timer_tic_handler(registers_t* regs);
void proc_switch_process(process_t* process);

#endif