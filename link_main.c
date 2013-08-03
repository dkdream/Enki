#include "reference.h"
#include "pair.h"
#include "treadmill.h"
#include "debug.h"
#include "type.h"
#include <stdio.h>

void enki_test(void* atom, void* type, void* size) {
    void* holding = 0;

    asm("movl %1,%%eax\n\t"
        "movl %2,%%ebx\n\t"
        "movl %3,%%ecx\n\t"
        "movl %%eax,-8(%%esp)\n\t"  //atom
        "movl %%ebx,-12(%%esp)\n\t" //size
        "movl %%ecx,-16(%%esp)\n\t" //type
        "lea -16(%%esp),%%eax\n\t"
        "call alloc_gc\n\t"
        "movl %%eax,%0"
        : "=m" (holding)
        : "m" (atom), "m" (size), "m" (type)
        : "%eax", "%ebx", "%ecx", "%esi", "edi");

    printf("results %p count %u type %p space %p\n",
           holding,
           (unsigned)getCount(holding),
           getType(holding).reference,
           getSpace(holding));
}

int main(int argc, char** argv) {
    ea_global_debug = 0;

    startEnkiLibrary();

    ea_global_debug = 1;
    __scan_cycle = 0;

    printf("                  count %u type %p space %p\n",
           sizeof(struct pair)/sizeof(void*),
           s_pair,
           _zero_space);

    enki_test((void*)0, (void*)s_pair, (void*)(sizeof(struct pair)));

    printf("done\n");

    stopEnkiLibrary();

    ea_global_debug = 0;

    return 0;
}
