.section .text
.align 4

.global _start
_start:
	mov %eax, %ebx # exit status
	mov $1, %eax   # `exit` syscall
	int $0x30
