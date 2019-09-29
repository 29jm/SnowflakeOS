# Yields continuously
loop:
	mov $42, %ecx
	push %ecx
	mov $0, %eax
	int $48
	pop %ecx
	jmp loop
