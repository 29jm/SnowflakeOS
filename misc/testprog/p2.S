	xchgw %bx, %bx
	mov $69, %ecx
loop:
	dec %ecx
	test %ecx,%ecx
	jnz loop
	mov $1, %eax
	int $48 # exit
