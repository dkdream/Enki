#include "prefix.inc"

void SUBR(box) {
    Node value;
    checkArgs(args, "box", 1, NIL);
    fetchArgs(args, &value, 0);
    pair_Create(value, NIL, result.pair);
}
