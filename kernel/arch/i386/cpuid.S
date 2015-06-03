.type detect_cpuid, @function
detect_cpuid:
	push %ebp
	mov %esp, %ebp
	pushfl               # Store the FLAGS-register.
    pop %eax             # Restore the A-register.
    mov %eax, %ecx       # Set the C-register to the A-register.
    xor $2097152, %eax   # Flip the ID-bit, which is bit 21.
    push %eax            # Store the A-register.
    popfl                # Restore the FLAGS-register.
    pushfl               # Store the FLAGS-register.
    pop %eax             # Restore the A-register.
    push %ecx            # Store the C-register.
    popfl                # Restore the FLAGS-register.
    xor %ecx, %eax       # Do a XOR-operation on the A-register and the C-register.
    jz no_cpuid          # The zero flag is set, no CPUID.
has_cpuid:
	mov $1, %eax
	jmp end_detect_cpuid
no_cpuid:
	mov $0, %eax
end_detect_cpuid:
	mov %ebp, %esp
	pop %ebp
	ret

.type detect_longmode, @function
detect_longmode:
	push %ebp
	mov %esp, %ebp
	mov $0x80000000, %eax # Check for longmode detection function
	cpuid
	cmp 0x80000001, %eax
	jb no_longmode        # No function, assume no longmode
has_longmode_detection:   # Thus, we use it
	mov 0x80000001, %eax
	cpuid
	test $536870912, %edx
	jz no_longmode
has_longmode:
	mov $1, %eax
	jmp end_detect_longmode
no_longmode:
	mov $0, %eax
end_detect_longmode:
	mov %ebp, %esp
	pop %ebp
	ret

.global enter_longmode
.type enter_longmode, @function
enter_longmode:
	push %ebp
	mov %esp, %ebp

	mov %ebp, %esp
	pop %ebp
	ret
