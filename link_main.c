#include "reference.h"
#include "pair.h"
#include "treadmill.h"
#include <stdio.h>

void enki_test(void* atom, void* size) {
    void* holding = 0;

    asm("movl %1,-8(%%esp)\n\t"
        "movl %2,-12(%%esp)\n\t"
        "lea -12(%%esp),%%eax\n\t"
        "call alloc_gc\n\t"
        "movl %%eax,%0"
        : "=r" (holding)
        : "r" (atom), "r" (size)
        : "%eax", "%ebx", "%esi", "edi"
        );

    printf("results %p\n", holding);
}

int main(int argc, char** argv) {
    startEnkiLibrary();

    printf("_zero_space %p\n", _zero_space);
    enki_test((void*)0, (void*)(sizeof(struct pair)));
    printf("done\n");

    stopEnkiLibrary();
    return 0;
}
