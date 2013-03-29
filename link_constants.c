#include "link_enki.h"

#include <stdio.h>

/*
  internally enki keeps dose not use
  bit encoding of type information
  (so the asm is simpler)
  since C need to know the return type
  we encoded it before returning it.
 */

static void print_ptr(ptr val) {
    if (isNil(val)) {
        printf("nil\n");
        return;
    }

    if (isBoxed(val)) {
        printf("[BOX %p]\n", (void*)val);
        return;
    }

    if (isFixed(val)) {
        printf("%d\n", fromFixed(val));
        return;
    }

    if (isBool(val)) {
        if (fromBool(val)) {
            printf("true\n");
        } else {
            printf("false\n");
        }
        return;
    }

    printf("[unknown %p]\n", (void*) val);
}

int main(int argc, char** argv) {
    if (POINTER_SIZE != PTR_SIZE) {
        fprintf(stderr,
                "sizeof(pointer)=%d and sizeof(ptr)=%d differ\n",
                POINTER_SIZE,
                PTR_SIZE);
    }
    print_ptr(enki_entry());
    return 0;
}
