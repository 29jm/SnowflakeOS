.section .text
.align 4

.global proc_switch_process
proc_switch_process: # void proc_switch_process();
    # Save register state
    push %ebx
    push %esi
    push %edi
    push %ebp

    # current_process->esp = %esp
    mov current_process, %eax
    mov %esp, 24(%eax)

    # %eax = current_process = current_process->next
    mov (%eax), %eax
    mov %eax, current_process

    # Set esp0 to the next process's kernel stack in the TSS
    push %eax
    push 20(%eax) # kernel_stack
    call gdt_set_kernel_stack
    add $4, %esp
    pop %eax

    # Switch to the next process's kernel stack
    mov 24(%eax), %esp

    # Switch page directory
    mov 16(%eax), %ebx # directory
    mov %ebx, %cr3

    # Restore registers from the next process's kernel stack
    pop %ebp
    pop %edi
    pop %esi
    pop %ebx

    ret
