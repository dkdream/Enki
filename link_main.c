#include "tuple.h"

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
    return true;
}

void enki_test() {
    asm("pusha\n\t"
        "movl $1,-8(%esp)\n\t"
        "movl $2,-12(%esp)\n\t"
        "lea -12(%esp),%eax\n\t"
        "call alloc_gc\n\t"
        "popa");
}

int main(int argc, char** argv) {
    printf("_zero_space %p\n", _zero_space);
    enki_test();
    printf("done\n");
    return 0;
}
