ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
	. = 0;

	.text ALIGN(4):
	{
		start.o(.text)
		*(.text*)
	}

	.data ALIGN(4):
	{
		*(.data)
	}

	.rodata ALIGN(4):
	{
		*(.rodata*)
	}
}
