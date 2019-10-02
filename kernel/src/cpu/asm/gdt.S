.section .text
.align 4

.global gdt_load # void gdt_load(gdt_descriptor_t* ptr)
.type gdt_load, @function
gdt_load:
	mov 4(%esp), %eax
	lgdt (%eax)

	# Reload segment registers
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	ljmp $0x08, $load # Reloads `cs`
load:
	ret
