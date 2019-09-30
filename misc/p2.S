# Loops
loop:
	mov $69, %ecx
	# mov $0, %eax
	# int $0x30
	jmp loop
	mov $1, %eax
	int $48 # exit
