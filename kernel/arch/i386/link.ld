ENTRY(_start)

SECTIONS
{
	KERNEL_VMA = 0x00000000;
	KERNEL_LMA = 1M;

	. = KERNEL_LMA;

	.bootstrap BLOCK(4K) : ALIGN(4K)
	{
		boot.o(.multiboot)
		boot.o(.text) /* paging, long mode*/
	}

	. += KERNEL_VMA;

	.text : AT(ADDR(.text) - KERNEL_VMA)
	{
		*(.text)
		*(.rodata*)
		. = ALIGN(4096);
	}

	.data : AT(ADDR(.data) - KERNEL_VMA)
	{
		*(.data)
		. = ALIGN(4096);
	}

	.eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA)
	{
		*(.eh_frame)
		. = ALIGN(4096);
	}

	.bss : AT(ADDR(.bss) - KERNEL_VMA)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
		. = ALIGN(4096);
	}

	/DISCARD/ :
	{
		*(.comment)
	}
}
