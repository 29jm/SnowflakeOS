.set MAGIC,				0x1BADB002
.set FLAGS_PAGE_ALIGN,	1 << 0
.set FLAGS_MEMORY,		1 << 1
.set FLAGS_GRAPHICS,	1 << 2
.set FLAGS,				FLAGS_PAGE_ALIGN | FLAGS_MEMORY /*| FLAGS_GRAPHICS*/
.set CHECKSUM,			-(MAGIC + FLAGS)

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

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp

	# Save %eax that contains the magic number
	pushl %eax

	# Call the global constructors.
	call _init

	# Transfer control to the main kernel.
	# Pass the multiboot header adress and magic number (already pushed)
	# See https://www.gnu.org/software/grub/manual/multiboot/html_node/Machine-state.html
	pushl %ebx

	cli
	call kernel_main
	add $12, %esp # cleanup the stack. useless here

	# Hang if kernel_main unexpectedly returns.
	cli
.Lhang:
	hlt
	jmp .Lhang

.size _start, . - _start
