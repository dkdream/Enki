#include "prefix.inc"

void SUBR(gc_scan) {
    Node value;

    checkArgs(args, "gc-scan", 1, t_integer);
    forceArgs(args, &value, 0);

    unsigned int count = value.integer->value;

    space_Scan(_zero_space, count);

    if (!space_CanFlip(_zero_space)) {
        ASSIGN(result, NIL);
    } else {
        space_Flip(_zero_space);
        ASSIGN(result, true_v);
    }
}
