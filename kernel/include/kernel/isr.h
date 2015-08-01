#ifndef ISR_H
#define ISR_H

#include <stdint.h>

typedef struct {
	uint32_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t int_no, err_code;
	uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*handler_t)(registers_t*);

void isr_register_handler(uint32_t num, handler_t handler);

#endif
