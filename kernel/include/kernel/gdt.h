#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__ ((packed)) gdt_entry_t;

typedef struct {
	uint16_t limit;
	uint32_t base;
} __attribute__ ((packed)) gdt_entry_ptr_t;

typedef struct {
	uint32_t prev_tss;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__ ((packed)) tss_entry_t;

void init_gdt();
void gdt_set_entry(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
void gdt_write_tss(uint32_t num, uint32_t ss0, uint32_t esp0);
void gdt_set_kernel_stack(uint32_t stack);

extern void gdt_load(uintptr_t gdt_ptr);

#endif
