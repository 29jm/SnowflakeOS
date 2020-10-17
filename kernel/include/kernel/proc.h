#pragma once

#include <kernel/list.h>

#include <stdint.h>
#include <stdbool.h>

#define PROC_KERNEL_STACK_PAGES 10 // In pages
#define PROC_MAX_FD 1024

// Add new members to the end to avoid messing with the offsets
typedef struct _proc_t {
	struct _proc_t* next;
	uint32_t pid;
	// Sizes of the exectuable and of the stack in number of pages
	uint32_t stack_len;
	uint32_t code_len;
	uintptr_t directory;
	uintptr_t kernel_stack;
	uintptr_t esp;
	uint32_t mem_len; // Size of program heap in bytes
	uint32_t sleep_ticks;
	uint8_t fpu_registers[512];
	list_t* fds;
	char* cwd;
} process_t;

void init_proc();
void proc_run_code(uint8_t* code, uint32_t size);
void proc_print_processes();
void proc_timer_callback();
void proc_exit_current_process();
void proc_enter_usermode();
void proc_switch_process();
uint32_t proc_get_current_pid();
char* proc_get_cwd();
void proc_sleep(uint32_t ms);
void* proc_sbrk(intptr_t size);
void proc_register_program(char* name, uint8_t* code, uint32_t size);
int32_t proc_exec(const char* name);
bool proc_has_fd(uint32_t fd);
uint32_t proc_open(const char* path, uint32_t mode);
void proc_close(uint32_t fd);