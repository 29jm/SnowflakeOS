#include <stdint.h>

typedef struct {
	uint32_t ds;
	uint32_t edi, esi, ebp, useless_esp, ebx, edx, ecx, eax;
	uint32_t int_no, err_code;
	uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t; 
