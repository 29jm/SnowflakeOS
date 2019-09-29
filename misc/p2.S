# Yields a few times then exits
	xchgw %bx, %bx
	mov $69, %ecx
loop:
	mov $0, %eax
	int $0x30
	dec %ecx
	test %ecx,%ecx
	jnz loop
	mov $1, %eax
	int $48 # exit
