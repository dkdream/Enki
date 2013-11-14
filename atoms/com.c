#include "prefix.inc"

void SUBR(com) {
    Node val = NIL;
    checkArgs(args, "~", 1, t_integer);
    forceArgs(args, &val, 0);
    integer_Create(~(val.integer->value), result.integer);
}
