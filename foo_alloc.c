#include "reference.h"
#include "treadmill.h"
#include <stdio.h>

extern bool node_Allocate(const Space space, bool atom, Size size_in_char, Target);

/*
  atom is a ??
  type is a Symbol
  size is a unsigned long
 */
extern void* alloc_gc(void* atom, void* size, void* type) {
    void* foo = 0;

    bool hold = node_Allocate(_zero_space, atom, (Size)size, (Target) &foo);

    return foo;
}


extern void xxx_test(void* atom, void* type, void* size) {
    void* holding = 0;

    holding = alloc_gc(atom, size, type);

    printf("results %p\n", holding);
}

extern void yyy_test(void* atom, void* type, void* size) {
    void* holding = 0;

    asm("movq %1,%%rax\n\t"
        "movq %2,%%rbx\n\t"
        "movq %3,%%rcx\n\t"
        "movq %%rax,-8(%%rsp)\n\t"  //atom
        "movq %%rbx,-16(%%rsp)\n\t" //size
        "movq %%rcx,-24(%%rsp)\n\t" //type
        "lea -24(%%rsp),%%rax\n\t"
        "call alloc_gc\n\t"
        "movq %%rax,%0"
        : "=m" (holding)
        : "m" (atom), "m" (size), "m" (type)
        : "%rax", "%rbx", "%rcx", "%rsi", "rdi");

    printf("results %p\n", holding);
}

