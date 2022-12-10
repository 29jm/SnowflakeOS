#pragma once

#include <kernel/isr.h>

#define CLI() asm volatile("cli")
#define STI() asm volatile("sti")

void init_irq();
void irq_handler(registers_t* regs);
void irq_send_eoi(uint8_t irq);
void irq_register_handler(uint8_t irq, handler_t handler);
void irq_remap();
uint16_t irq_get_isr();
uint8_t irq_get_mask(uint16_t pic);
void irq_set_mask(uint16_t pic, uint8_t mask);
void irq_mask(uint32_t irq);
void irq_unmask(uint32_t irq);

// From ASM
void irq0();
void irq1();
void irq2();
void irq3();
void irq4();
void irq5();
void irq6();
void irq7();
void irq8();
void irq9();
void irq10();
void irq11();
void irq12();
void irq13();
void irq14();
void irq15();

#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC1        PIC1_CMD

#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1
#define PIC2        PIC2_CMD

#define PIC_EOI     0x20
#define PIC_RESTART 0x11
#define PIC_ISR     0x0B

#define IRQ0  32
#define IRQ1  33
#define IRQ2  34
#define IRQ3  35
#define IRQ4  36
#define IRQ5  37
#define IRQ6  38
#define IRQ7  39
#define IRQ8  40
#define IRQ9  41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

#define INT_TO_IRQ(num) (num - IRQ0)
#define IRQ_TO_INT(num) (num + IRQ0)
