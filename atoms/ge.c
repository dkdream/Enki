#include "prefix.inc"

void SUBR(ge)
{
    Node left; Node right;
    checkArgs(args, ">=", 2, t_integer, t_integer);
    forceArgs(args, &left, &right, 0);
    if ((left.integer->value) >= (right.integer->value)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
