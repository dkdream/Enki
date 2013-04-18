        .text
        .align 4
        .type alloc_gc, @function
        .globl alloc_gc
alloc_gc:
        xchg %eax, %esp
        push %ebp
        push %esi
        movl %eax, %ebp
        movl %esp, %esi
        sub $4, %esp
        movl $0, -4(%esi)
        lea -4(%esi), %eax
        push %eax
        push -8(%ebp)
        push -4(%ebp)
        push _zero_space
        call node_Allocate
        add $16, %esp
        xor $1, %eax
        test %al, %al
        je K_1
        movl $0, %eax
        jmp K_2
K_1:
        pop %eax
K_2:
        movl %ebp, %edi
        movl %esi, %esp
        pop %esi
        pop %ebp
        movl %edi, %esp
        ret
        .size alloc_gc, .-alloc_gc
