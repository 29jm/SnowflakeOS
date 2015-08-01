.set MAGIC,            0x1BADB002
.set FLAGS_PAGE_ALIGN, 1 << 0
.set FLAGS_MEMORY,     1 << 1
.set FLAGS_GRAPHICS,   1 << 2
.set FLAGS,            FLAGS_PAGE_ALIGN | FLAGS_MEMORY /*| FLAGS_GRAPHICS*/
.set CHECKSUM,         -(MAGIC + FLAGS)

.set KERNEL_VIRTUAL_BASE, 0xC0000000
.set KERNEL_PAGE_NUMBER,  (KERNEL_VIRTUAL_BASE >> 22)

# Declare a header as in the Multiboot Standard.
.section .multiboot
	.align 4
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
	.long 0x00000000 /* header_addr stub*/
	.long 0x00000000 /* load_addr stub*/
	.long 0x00000000 /* load_end_addr stub*/
	.long 0x00000000 /* bss_end_addr stub*/
	.long 0x00000000 /* entry_addr stub*/
	.long 0	/* Linear graphics */
	.long 0 /* These are in bytes in ToaruOS. Investigate. */
	.long 0
	.long 0

# Reserve a stack for the initial thread.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
	.skip 16384 # 16 KiB
stack_top:

.section .data
.global kernel_directory
.align 0x1000
kernel_directory:
	.long 0x00000083
	.fill (KERNEL_PAGE_NUMBER - 1), 4, 0
	.long 0x00000083
	.fill (1024 - KERNEL_PAGE_NUMBER - 1), 4, 0

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
	mov $(kernel_directory - KERNEL_VIRTUAL_BASE), %ecx
	mov %ecx, %cr3

	mov %cr4, %ecx
	or $0x00000010, %ecx
	mov %ecx, %cr4

	mov %cr0, %ecx
	or $0x80000000, %ecx
	mov %ecx, %cr0

	lea _start_higher_half, %ecx
	jmp *%ecx

_start_higher_half:
	# movl $0, boot_page_directory
	# invlpg 0

	movl $stack_top, %esp

	# Transfer control to the main kernel.
	# Pass the multiboot header adress and magic number (already pushed)
	# See https://www.gnu.org/software/grub/manual/multiboot/html_node/Machine-state.html
	pushl %eax
	add $0xC0000000, %ebx
	pushl %ebx

	cli # disable until we setup handlers
	call kernel_main
	add $12, %esp # cleanup the stack. useless here

	# Hang if kernel_main unexpectedly returns.
	cli
.hang:
	hlt
	jmp .hang
