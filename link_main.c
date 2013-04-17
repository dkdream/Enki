#include "tuple.h"
#include <stdio.h>

typedef void *Space;

Space _zero_space = 16;

bool node_Allocate(const Space space,
                   bool atom,
                   Size size,
                   Target target)
{
    printf("space %p atom %d size %u target %p\n",
           space,
           atom,
           size,
           target);

    target.reference[0] = 32;

    return true;
}

void enki_test(void* one, void* two) {
    void* holding = 0;

    asm("movl %1,-8(%%esp)\n\t"
        "movl %2,-12(%%esp)\n\t"
        "lea -12(%%esp),%%eax\n\t"
        "call alloc_gc\n\t"
        "movl %%eax,%0"
        : "=r" (holding)
        : "r" (one), "r" (two)
        : "%eax", "%ebx", "%esi", "edi"
        );

    printf("results %p\n", holding);
}

int main(int argc, char** argv) {
    printf("_zero_space %p\n", _zero_space);
    enki_test(1,2);
    printf("done\n");
    return 0;
}
