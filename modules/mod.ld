ENTRY(_start)
OUTPUT_FORMAT(binary)

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
