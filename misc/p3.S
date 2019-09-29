# Writes a string, waits 10 seconds, writes a character then exits
main:
	mov $1337, %ecx # for id

	mov $3, %eax # putchar
	mov $72, %ebx # H
	int $0x30
	mov $101, %ebx # e
	int $0x30
	mov $108, %ebx # l
	int $0x30
	mov $108, %ebx # l
	int $0x30
	mov $111, %ebx # o
	int $0x30
	mov $44, %ebx # ,
	int $0x30
	mov $32, %ebx #  
	int $0x30
	mov $119, %ebx # w
	int $0x30
	mov $111, %ebx # o
	int $0x30
	mov $114, %ebx # r
	int $0x30
	mov $108, %ebx # l
	int $0x30
	mov $100, %ebx # d
	int $0x30
	mov $10, %ebx # \n
	int $0x30

	mov $2, %eax
	mov $10, %ebx
	int $0x30
	
	mov $3, %eax
	mov $35, %ebx # #
	int $0x30

	mov $1, %eax # exit
	int $0x30
