#pragma once

#include <kernel/list.h>

#include <stdint.h>
#include <stdbool.h>

#define PROC_STACK_PAGES 4
#define PROC_KERNEL_STACK_PAGES 1
#define PROC_MAX_FD 1024

// Add new members to the end to avoid messing with the offsets
typedef struct _proc_t {
    uint32_t pid;
    // Sizes of the exectuable and of the stack in number of pages
    uint32_t stack_len;
    uint32_t code_len;
    uintptr_t directory;
    // Kernel stack used when the process is preempted
    uintptr_t kernel_stack;
    // Kernel stack to restore when the process is switched to
    uintptr_t esp;
    uint32_t mem_len; // Size of program heap in bytes
    uint32_t sleep_ticks;
    uint8_t fpu_registers[512];
    list_t* fds;
    char* cwd;
} process_t;

/* This structure defines the interface of schedulers in SnowflakeOS.
 */
typedef struct _sched_t {
    /* Returns the currently elected process */
    process_t* (*sched_get_current)(struct _sched_t*);
    /* Adds a new process, already initialized, to the process pool */
    void (*sched_add)(struct _sched_t*, process_t*);
    /* Returns the next process that should be run, depending to the specific
       scheduler implemented. Note that it can choose not to change process by
       returning the currently executing process */
    process_t* (*sched_next)(struct _sched_t*);
    /* Removes a process from the process pool. Basically the inverse of
     * `sched_add`. If the removed process was the one currently executing, the
     * scheduler must ensure that `sched_next` keeps working: it'll be called
     * right after.
     */
    void (*sched_exit)(struct _sched_t*, process_t*);
} sched_t;

void init_proc();
process_t* proc_run_code(uint8_t* code, uint32_t size);
void proc_print_processes();
void proc_schedule();
void proc_timer_callback();
void proc_exit_current_process();
void proc_enter_usermode();
void proc_switch_process(process_t* next);
uint32_t proc_get_current_pid();
char* proc_get_cwd();
void proc_sleep(uint32_t ms);
void* proc_sbrk(intptr_t size);
int32_t proc_exec(const char* name);
bool proc_has_fd(uint32_t fd);
uint32_t proc_open(const char* path, uint32_t mode);
void proc_close(uint32_t fd);