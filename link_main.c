#include "reference.h"
#include "pair.h"
#include "treadmill.h"
#include "debug.h"
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
    ea_global_debug = 10;

    startEnkiLibrary();

    printf("_zero_space %p %u\n", _zero_space, sizeof(struct pair)/sizeof(void*));
    enki_test((void*)0, (void*)(sizeof(struct pair)));
    printf("done\n");

    stopEnkiLibrary();

    ea_global_debug = 0;

    return 0;
}
