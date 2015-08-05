#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/isr.h>
#include <stdint.h>

typedef int(*sys_handler_t)(registers_t*);

void init_syscall();
void syscall_register_handler(uint32_t num, sys_handler_t handler);

#endif
