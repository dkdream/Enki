#include "reference.h"
#include "pair.h"
#include "treadmill.h"
#include "debug.h"
#include "type.h"
#include <stdio.h>

void enki_test(void* atom, void* type, void* size) {
    void* holding = 0;
#if 0
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
#endif

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
