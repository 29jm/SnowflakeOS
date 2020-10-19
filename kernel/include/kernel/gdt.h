#pragma once

#include <stdint.h>

// `flags` part
#define GDT_GRAN ((1 << 3) << 4) // `limit` is expressed in pages when set
#define GDT_32 ((1 << 2) << 4) // 32 bits protected mode when set, 16 otherwise

// `access` part
#define GDT_RW (1 << 1) // When set, does the right thing
#define GDT_DC (1 << 2) // Never set?
#define GDT_EXEC (1 << 3) // Set for code segments only
#define GDT_S (1 << 4) // Set for code and data segments only
#define GDT_DPL(n) (n << 5)
#define GDT_PRESENT (1 << 7) // Always set

// Used access and flags
#define GDT_ACCESS_KERNEL_CODE (GDT_RW | GDT_EXEC | GDT_S | GDT_PRESENT)
#define GDT_ACCESS_KERNEL_DATA (GDT_RW | GDT_S | GDT_PRESENT)
#define GDT_ACCESS_USER_CODE (GDT_RW | GDT_EXEC | GDT_S | GDT_DPL(3) | GDT_PRESENT)
#define GDT_ACCESS_USER_DATA (GDT_RW | GDT_S | GDT_DPL(3) | GDT_PRESENT)
#define GDT_FLAGS (GDT_GRAN | GDT_32)

// A GDT entry is structured as follows:
// |base 24:31|flags 0:3|limit 16:19|access 0:7|base 16:23|base 0:15|limit 0:15|
// where `access` is |P|DPL 0:1|S|Ex|DC|RW|Ac|
// and `flags` is |Granularity|Size|
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t limit_and_flags;
    uint8_t base_high;
} __attribute__ ((packed)) gdt_entry_t;

typedef struct {
    uint16_t size;
    uint32_t offset;
} __attribute__ ((packed)) gdt_pointer_t;

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
void gdt_set_kernel_stack(uintptr_t stack);

extern void gdt_load(gdt_pointer_t* gdt_ptr);