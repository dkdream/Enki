#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef __attribute__((fastcall)) void* (*Function)(void*,void*);

__attribute__((fastcall)) void* fooness(void* xx, void* yy);
__attribute__((fastcall)) void* fooness(void* xx, void* yy)
{
    if (xx == yy) return (void*)0;
    return (void*)(-1);
}

void bar(Function pointer) {
    void* result = pointer((void*)1, (void*)2);

    printf("0x%p\n", result);
}

void shmay() {
    bar(fooness);
}

/*
## %eax = return value
## %esp = stack pointer
## %ebp = base pointer (frame)
## %ecx = fastcall arg0
## %edx = fastcall arg1
##
        .file   "test_fastcall.c"
        .text
        .p2align 4,,15

.globl fooness
        .type   fooness, @function

fooness:
        xorl    %eax, %eax
        cmpl    %edx, %ecx
        pushl   %ebp
        sete    %al
        movl    %esp, %ebp
        subl    $1, %eax
        popl    %ebp
        ret
        .size   fooness, .-fooness

        .section        .rodata.str1.1,"aMS",@progbits,1
.LC0:
        .string "0x%p\n"
        .text
        .p2align 4,,15

.globl bar
        .type   bar, @function

bar:
        pushl   %ebp       ## push frame
        movl    %esp, %ebp ## stack -> frame
        movl    $2, %edx   ## arg1 = 2
        movl    $1, %ecx   ## arg0 = 1
        subl    $24, %esp  ## alloca(24) (bytes) locals and args
        call    *8(%ebp)   ## call pointer

        movl    $.LC0, (%esp) ## arg0 = "0x%p\n"
        movl    %eax, 4(%esp) ## arg1 = result (
        call    printf
        leave
        ret
        .size   bar, .-bar
        .p2align 4,,15

.globl shmay
        .type   shmay, @function

shmay:
        pushl   %ebp
        movl    %esp, %ebp
        subl    $24, %esp
        movl    $-1, 4(%esp)
        movl    $.LC0, (%esp)
        call    printf
        leave
        ret
        .size   shmay, .-shmay
        .ident  "GCC: (Debian 4.4.5-8) 4.4.5"
        .section        .note.GNU-stack,"",@progbits
*/
