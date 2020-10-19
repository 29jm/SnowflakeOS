.section .text
.align 4

.extern main

.global _start
_start:
    call main
    mov %eax, %ebx # exit status
    mov $1, %eax   # `exit` syscall
    int $0x30
