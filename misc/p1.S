# sets ecx to 42 and uses a bit of stack
loop:
	mov $42, %ecx
	push %ecx
	# mov $0, %eax
	# int $48
	pop %ecx
	jmp loop
