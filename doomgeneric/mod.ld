ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 0x1000;

    .text ALIGN(4):
    {
        objs/start.o(.text)
        *(.text*)
    }

    .data ALIGN(4):
    {
        *(.data)
        *(.bss*) /* Make .bss 'PROGBITS' */
    }

    .rodata ALIGN(4):
    {
        *(.rodata*)
    }
}
