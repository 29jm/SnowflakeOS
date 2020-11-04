.section .text
.align 4

.global proc_switch_process
proc_switch_process: # void proc_switch_process(process_t* next);
    # Save register state
    push %ebx
    push %esi
    push %edi
    push %ebp

    # %eax = current_process
    mov current_process, %eax
    # current_process->esp = %esp
    mov %esp, 20(%eax)

    # %eax = next
    # current_process = next
    mov 20(%esp), %eax
    mov %eax, current_process

    # Set esp0 to the next process's kernel stack in the TSS
    push %eax
    push 16(%eax) # kernel_stack
    call gdt_set_kernel_stack
    add $4, %esp
    pop %eax

    # Switch to the next process's saved kernel stack
    mov 20(%eax), %esp

    # Switch page directory
    mov 12(%eax), %ebx # directory
    mov %ebx, %cr3

    # Restore registers from the next process's kernel stack
    pop %ebp
    pop %edi
    pop %esi
    pop %ebx

    ret
