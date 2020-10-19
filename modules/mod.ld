ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x1000;

    .text ALIGN(4):
    {
        src/start.o(.text)
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
