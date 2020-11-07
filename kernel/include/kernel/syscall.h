#pragma once

#include <kernel/isr.h>

#include <stdint.h>

#define SYSCALL_NUM 64

void init_syscall();
void syscall_register_handler(uint32_t num, handler_t handler);