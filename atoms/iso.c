#include "prefix.inc"

void SUBR(iso)
{
    Node depth; Node left; Node right;
    checkArgs(args, "iso", 3, t_integer, NIL, NIL);
    forceArgs(args, &depth, &left, &right, 0);

    if (node_Iso(depth.integer->value, left,right)) {
        ASSIGN(result, true_v);
    } else {
        ASSIGN(result, NIL);
    }
}
