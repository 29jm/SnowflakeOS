.section .text
.align 4

.macro IRQ num remap
	.global irq\num
	irq\num:
		cli
		push $0
		push $\remap
		jmp irq_common_handler
.endm

IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.extern irq_handler
.type irq_handler, @function

irq_common_handler:
	pusha

	push %ds
	push %es
	push %fs
	push %gs

	mov $0x10, %ax

	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp
	call irq_handler
	add $4, %esp

	pop %gs
	pop %fs
	pop %es
	pop %ds

	popa
	add $8, %esp
	iret
