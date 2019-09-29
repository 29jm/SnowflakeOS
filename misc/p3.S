main:
	mov $1337, %ecx # for id
	mov $2, %eax
	mov $10, %ebx
	int $0x30
	mov $1, %eax # exit
	int $0x30

