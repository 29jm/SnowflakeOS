.section .text

# The following functions have prototypes of the form
#     uint32_t sysn(uint32_t sys_no, uint32_t ...)
# This prototype ensures the compiler knows to save eax before calling, as eax
# is used for integer return values in cdecl.
# Other registers are restored, and whatever value was in eax is returned.

.global syscall
syscall: # eax
    mov 4(%esp), %eax
    int $0x30
    ret

.global syscall1
syscall1: # eax, ebx
    push %ebx
    mov 8(%esp), %eax
    mov 12(%esp), %ebx
    int $0x30
    pop %ebx
    ret

.global syscall2
syscall2: # eax, ebx, ecx
    push %ebx
    push %ecx
    mov 12(%esp), %eax
    mov 16(%esp), %ebx
    mov 20(%esp), %ecx
    int $0x30
    pop %ecx
    pop %ebx
    ret